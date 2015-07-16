/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id: mpeengine.cc 38753 2015-04-11 21:19:18Z nanne.hemstra@dgbes.com $";

#include "mpeengine.h"

#include "arrayndimpl.h"
#include "autotracker.h"
#include "emeditor.h"
#include "emmanager.h"
#include "emseedpicker.h"
#include "emsurface.h"
#include "emtracker.h"
#include "executor.h"
#include "flatposdata.h"
#include "geomelement.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "sectiontracker.h"
#include "seisdatapack.h"
#include "survinfo.h"

#define mRetErr( msg, retval ) { errmsg_ = msg; return retval; }

MPE::Engine& MPE::engine()
{
    mDefineStaticLocalObject( PtrMan<MPE::Engine>, theinst_,
			      = new MPE::Engine );
    return *theinst_;
}

namespace MPE
{

// MPE::Engine
Engine::Engine()
    : activevolumechange( this )
    , trackeraddremove( this )
    , loadEMObject( this )
    , oneactivetracker_( 0 )
    , activetracker_( 0 )
    , activegeomid_(Survey::GeometryManager::cUndefGeomID())
    , dpm_(DPM(DataPackMgr::SeisID()))
{
    trackers_.allowNull();
    trackermgrs_.allowNull();
    flatcubescontainer_.allowNull();

    init();
}


Engine::~Engine()
{
    deepUnRef( trackers_ );
    deepUnRef( editors_ );
    deepErase( attribcachespecs_ );
    deepErase( attribbackupcachespecs_ );
    deepErase( flatcubescontainer_ );
    deepErase( trackermgrs_ );

    for ( int idx=attribcachedatapackids_.size()-1; idx>=0; idx-- )
	dpm_.release( attribcachedatapackids_[idx] );
    for ( int idx=attribbkpcachedatapackids_.size()-1; idx>=0; idx-- )
	dpm_.release( attribbkpcachedatapackids_[idx] );
}


const TrcKeyZSampling& Engine::activeVolume() const
{ return activevolume_; }


void Engine::setActiveVolume( const TrcKeyZSampling& nav )
{
    activevolume_ = nav;
    activevolumechange.trigger();
}


void Engine::setActive2DLine( Pos::GeomID geomid )
{ activegeomid_ = geomid; }

Pos::GeomID Engine::activeGeomID() const
{ return activegeomid_; }


void Engine::updateSeedOnlyPropagation( bool yn )
{
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	if ( !trackers_[idx] || !trackers_[idx]->isEnabled() )
	    continue;

	EM::ObjectID oid = trackers_[idx]->objectID();
	EM::EMObject* emobj = EM::EMM().getObject( oid );

	for ( int sidx=0; sidx<emobj->nrSections(); sidx++ )
	{
	    EM::SectionID sid = emobj->sectionID( sidx );
	    SectionTracker* sectiontracker =
			trackers_[idx]->getSectionTracker( sid, true );
	    sectiontracker->setSeedOnlyPropagation( yn );
	}
    }
}


void Engine::stopTracking()
{
    for ( int idx=0; idx<trackermgrs_.size(); idx++ )
    {
	HorizonTrackerMgr* htm = trackermgrs_[idx];
	if ( !htm ) continue;

	htm->stop();
    }
}


Executor* Engine::trackInVolume()
{
    ExecutorGroup* res = 0;

    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	EMTracker* tracker = trackers_[idx];
	if ( !tracker || !tracker->isEnabled() )
	    continue;

	EM::ObjectID oid = tracker->objectID();
	EM::EMObject* emobj = EM::EMM().getObject( oid );
	if ( !emobj || emobj->isLocked() )
	    continue;

	emobj->sectionGeometry( emobj->sectionID(0) )->blockCallBacks(true);
	EMSeedPicker* seedpicker = tracker->getSeedPicker( false );
	if ( !seedpicker ) continue;

	TypeSet<TrcKey> seeds;
	seedpicker->getSeeds( seeds );
	HorizonTrackerMgr* htm = trackermgrs_[idx];
	if ( !htm )
	{
	    htm = new HorizonTrackerMgr( *tracker );
	    trackermgrs_.replace( idx, htm );
	}

	htm->setSeeds( seeds );
	htm->startFromSeeds();
    }

    return res;
}


HorizonTrackerMgr* Engine::trackInVolume( int idx )
{
    EMTracker* tracker = trackers_[idx];
    if ( !tracker || !tracker->isEnabled() )
	return 0;

    EM::ObjectID oid = tracker->objectID();
    EM::EMObject* emobj = EM::EMM().getObject( oid );
    if ( !emobj || emobj->isLocked() )
	return 0;

    emobj->sectionGeometry( emobj->sectionID(0) )->blockCallBacks(true);
    EMSeedPicker* seedpicker = tracker->getSeedPicker( false );
    if ( !seedpicker ) return 0;

    TypeSet<TrcKey> seeds;
    seedpicker->getSeeds( seeds );
    HorizonTrackerMgr* htm = trackermgrs_[idx];
    if ( !htm )
    {
	htm = new HorizonTrackerMgr( *tracker );
	trackermgrs_.replace( idx, htm );
    }

    htm->setSeeds( seeds );
    htm->startFromSeeds();
    return htm;
}


void Engine::removeSelectionInPolygon( const Selector<Coord3>& selector,
				       TaskRunner* tr )
{
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	if ( !trackers_[idx] || !trackers_[idx]->isEnabled() )
	    continue;

	EM::ObjectID oid = trackers_[idx]->objectID();
	EM::EMM().removeSelected( oid, selector, tr );

	EM::EMObject* emobj = EM::EMM().getObject( oid );
	if ( !emobj->getRemovedPolySelectedPosBox().isEmpty() )
	{
	    emobj->emptyRemovedPolySelectedPosBox();
	}
    }
}


void Engine::getAvailableTrackerTypes( BufferStringSet& res ) const
{ res = TrackerFactory().getNames(); }


int Engine::addTracker( EM::EMObject* obj )
{
    if ( !obj )
	mRetErr( "No valid object", -1 );

    const int idx = getTrackerByObject( obj->id() );

    if ( idx != -1 )
    {
	trackers_[idx]->ref();
	return idx;
    }
	//mRetErr( "Object is already tracked", -1 );

    EMTracker* tracker = TrackerFactory().create( obj->getTypeStr(), obj );
    if ( !tracker )
	mRetErr( "Cannot find this trackertype", -1 );

    tracker->ref();
    trackers_ += tracker;
    trackermgrs_ += 0;
    ObjectSet<FlatCubeInfo>* flatcubes = new ObjectSet<FlatCubeInfo>;
    flatcubescontainer_ += flatcubes;
    trackeraddremove.trigger();

    return trackers_.size()-1;
}


void Engine::removeTracker( int idx )
{
    if ( idx<0 || idx>=trackers_.size() || !trackers_[idx] )
	return;

    const int noofref = trackers_[idx]->nrRefs();
    trackers_[idx]->unRef();

    if ( noofref != 1 )
	return;

    trackers_.replace( idx, 0 );
    trackermgrs_.replace( idx, 0 );

    deepErase( *flatcubescontainer_[idx] );
    flatcubescontainer_.replace( idx, 0 );

    trackeraddremove.trigger();
}


int Engine::nrTrackersAlive() const
{
    int nrtrackers = 0;
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	if ( trackers_[idx] )
	    nrtrackers++;
    }
    return nrtrackers;
}


int Engine::highestTrackerID() const
{ return trackers_.size()-1; }


const EMTracker* Engine::getTracker( int idx ) const
{ return const_cast<Engine*>(this)->getTracker(idx); }


EMTracker* Engine::getTracker( int idx )
{ return idx>=0 && idx<trackers_.size() ? trackers_[idx] : 0; }


int Engine::getTrackerByObject( const EM::ObjectID& oid ) const
{
    if ( oid==-1 ) return -1;

    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	if ( !trackers_[idx] ) continue;

	if ( oid==trackers_[idx]->objectID() )
	    return idx;
    }

    return -1;
}


int Engine::getTrackerByObject( const char* objname ) const
{
    if ( !objname || !objname[0] ) return -1;

    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	if ( !trackers_[idx] ) continue;

	if ( trackers_[idx]->objectName() == objname )
	    return idx;
    }

    return -1;
}


void Engine::setActiveTracker( EMTracker* tracker )
{ activetracker_ = tracker; }


void Engine::setActiveTracker( const EM::ObjectID& objid )
{
    const int tridx = getTrackerByObject( objid );
    activetracker_ = trackers_.validIdx(tridx) ? trackers_[tridx] : 0;
}


EMTracker* Engine::getActiveTracker()
{ return activetracker_; }


void Engine::setOneActiveTracker( const EMTracker* tracker )
{ oneactivetracker_ = tracker; }


void Engine::unsetOneActiveTracker()
{ oneactivetracker_ = 0; }


void Engine::getNeededAttribs( TypeSet<Attrib::SelSpec>& res ) const
{
    for ( int trackeridx=0; trackeridx<trackers_.size(); trackeridx++ )
    {
	const EMTracker* tracker = trackers_[trackeridx];
	if ( !tracker ) continue;

	if ( oneactivetracker_ && oneactivetracker_!=tracker )
	    continue;

	TypeSet<Attrib::SelSpec> specs;
	tracker->getNeededAttribs( specs );
	for ( int idx=0; idx<specs.size(); idx++ )
	{
	    const Attrib::SelSpec& as = specs[idx];
	    res.addIfNew( as );
	}
    }
}


TrcKeyZSampling Engine::getAttribCube( const Attrib::SelSpec& as ) const
{
    TrcKeyZSampling res( engine().activeVolume() );
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	if ( !trackers_[idx] ) continue;
	const TrcKeyZSampling cs = trackers_[idx]->getAttribCube( as );
	res.include(cs);
    }

    return res;
}


bool Engine::isSelSpecSame( const Attrib::SelSpec& setupss,
			    const Attrib::SelSpec& clickedss ) const
{
    return setupss.id().asInt()==clickedss.id().asInt() &&
	setupss.isStored()==clickedss.id().isStored() &&
	setupss.isNLA()==clickedss.isNLA() &&
	BufferString(setupss.defString())==
	   BufferString(clickedss.defString()) &&
	setupss.is2D()==clickedss.is2D();
}


int Engine::getCacheIndexOf( const Attrib::SelSpec& as ) const
{
    for ( int idx=0; idx<attribcachespecs_.size(); idx++ )
    {
	if ( attribcachespecs_[idx]->attrsel_.is2D()	   != as.is2D()  ||
	     attribcachespecs_[idx]->attrsel_.isNLA()	   != as.isNLA() ||
	     attribcachespecs_[idx]->attrsel_.id().asInt() != as.id().asInt() ||
	     attribcachespecs_[idx]->attrsel_.id().isStored() != as.isStored() )
	    continue;

	if ( !as.is2D() )
	    return idx;

	if ( attribcachespecs_[idx]->geomid_ != activeGeomID() )
	    continue;

	return idx;
    }

    return -1;
}


DataPack::ID Engine::getAttribCacheID( const Attrib::SelSpec& as ) const
{
    const int idx = getCacheIndexOf(as);
    return attribcachedatapackids_.validIdx(idx)
	? attribcachedatapackids_[idx] : DataPack::cNoID();
}


bool Engine::hasAttribCache( const Attrib::SelSpec& as ) const
{
    const DataPack::ID dpid = getAttribCacheID( as );
    ConstDataPackRef<SeisDataPack> regsdp = dpm_.obtain( dpid );
    return regsdp;
}


bool Engine::setAttribData( const Attrib::SelSpec& as,
			    DataPack::ID cacheid )
{
    ConstDataPackRef<SeisFlatDataPack> regfdp =
		DPM(DataPackMgr::FlatID()).obtain( cacheid );
    if ( regfdp ) cacheid = regfdp->getSourceDataPack().id();

    const int idx = getCacheIndexOf(as);
    if ( attribcachedatapackids_.validIdx(idx) )
    {
	if ( cacheid == DataPack::cNoID() )
	{
	    dpm_.release( attribcachedatapackids_[idx] );
	    attribcachedatapackids_.removeSingle( idx );
	    delete attribcachespecs_.removeSingle( idx );
	}
	else
	{
	    ConstDataPackRef<SeisDataPack> newdata= dpm_.obtain(cacheid);
	    if ( newdata )
	    {
		dpm_.release( attribcachedatapackids_[idx] );
		attribcachedatapackids_[idx] = cacheid;
		dpm_.obtain( cacheid );
	    }
	}
    }
    else if ( cacheid != DataPack::cNoID() )
    {
	ConstDataPackRef<SeisDataPack> newdata = dpm_.obtain( cacheid );
	if ( newdata )
	{
	    attribcachespecs_ += as.is2D() ?
		new CacheSpecs( as, activeGeomID() ) :
		new CacheSpecs( as ) ;

	    attribcachedatapackids_ += cacheid;
	    dpm_.obtain( cacheid );
	}
    }

    return true;
}


bool Engine::cacheIncludes( const Attrib::SelSpec& as,
			    const TrcKeyZSampling& cs )
{
    ConstDataPackRef<SeisDataPack> cache =
				dpm_.obtain( getAttribCacheID(as) );
    if ( !cache ) return false;

    return true;
/*
    TrcKeyZSampling cachedcs = cache->sampling();
    const float zrgeps = 0.01f * SI().zStep();
    cachedcs.zsamp_.widen( zrgeps );
    return cachedcs.includes( cs );
*/
}


void Engine::swapCacheAndItsBackup()
{
    const TypeSet<DataPack::ID> tempcachedatapackids = attribcachedatapackids_;
    const ObjectSet<CacheSpecs> tempcachespecs = attribcachespecs_;
    attribcachedatapackids_ = attribbkpcachedatapackids_;
    attribcachespecs_ = attribbackupcachespecs_;
    attribbkpcachedatapackids_ = tempcachedatapackids;
    attribbackupcachespecs_ = tempcachespecs;
}


void Engine::updateFlatCubesContainer( const TrcKeyZSampling& cs, const int idx,
					bool addremove )
{
    if ( !(cs.nrInl()==1) && !(cs.nrCrl()==1) )
	return;

    ObjectSet<FlatCubeInfo>& flatcubes = *flatcubescontainer_[idx];

    int idxinquestion = -1;
    for ( int flatcsidx=0; flatcsidx<flatcubes.size(); flatcsidx++ )
	if ( flatcubes[flatcsidx]->flatcs_.defaultDir() == cs.defaultDir() )
	{
	    if ( flatcubes[flatcsidx]->flatcs_.nrInl() == 1 )
	    {
		if ( flatcubes[flatcsidx]->flatcs_.hsamp_.start_.inl() ==
			cs.hsamp_.start_.inl() )
		{
		    idxinquestion = flatcsidx;
		    break;
		}
	    }
	    else if ( flatcubes[flatcsidx]->flatcs_.nrCrl() == 1 )
	    {
		if ( flatcubes[flatcsidx]->flatcs_.hsamp_.start_.crl() ==
		     cs.hsamp_.start_.crl() )
		{
		    idxinquestion = flatcsidx;
		    break;
		}
	    }
	}

    if ( addremove )
    {
	if ( idxinquestion == -1 )
	{
	    FlatCubeInfo* flatcsinfo = new FlatCubeInfo();
	    flatcsinfo->flatcs_.include( cs );
	    flatcubes += flatcsinfo;
	}
	else
	{
	    flatcubes[idxinquestion]->flatcs_.include( cs );
	    flatcubes[idxinquestion]->nrseeds_++;
	}
    }
    else
    {
	if ( idxinquestion == -1 ) return;

	flatcubes[idxinquestion]->nrseeds_--;
	if ( flatcubes[idxinquestion]->nrseeds_ == 0 )
	    flatcubes.removeSingle( idxinquestion );
    }
}


ObjectSet<TrcKeyZSampling>* Engine::getTrackedFlatCubes( const int idx ) const
{
    if ( (flatcubescontainer_.size()==0) || !flatcubescontainer_[idx] )
	return 0;

    const ObjectSet<FlatCubeInfo>& flatcubes = *flatcubescontainer_[idx];
    if ( flatcubes.size()==0 )
	return 0;

    ObjectSet<TrcKeyZSampling>* flatcbs = new ObjectSet<TrcKeyZSampling>;
    for ( int flatcsidx = 0; flatcsidx<flatcubes.size(); flatcsidx++ )
    {
	TrcKeyZSampling* cs = new TrcKeyZSampling();
	cs->setEmpty();
	cs->include( flatcubes[flatcsidx]->flatcs_ );
	flatcbs->push( cs );
    }
    return flatcbs;
}


DataPack::ID Engine::getSeedPosDataPack( const TrcKey& tk, float z, int nrtrcs,
					const StepInterval<float>& zintv ) const
{
    TypeSet<Attrib::SelSpec> specs; getNeededAttribs( specs );
    if ( specs.isEmpty() ) return DataPack::cNoID();

    DataPackMgr& dpm = DPM( DataPackMgr::SeisID() );
    const DataPack::ID pldpid = getAttribCacheID( specs[0] );
    ConstDataPackRef<SeisDataPack> sdp = dpm.obtain( pldpid );
    if ( !sdp ) return DataPack::cNoID();

    const int globidx = sdp->getNearestGlobalIdx( tk );
    if ( globidx < 0 ) return DataPack::cNoID();

    const int nrz = zintv.nrSteps() + 1;
    Array2DImpl<float>* seeddata = new Array2DImpl<float>( nrtrcs, nrz );
    seeddata->setAll( mUdf(float) );

    const int trcidx0 = globidx - (int)(nrtrcs/2);
    const StepInterval<float>& zsamp = sdp->getZRange();
    const int zidx0 = zsamp.getIndex( z + zintv.start );
    for ( int tidx=0; tidx<nrtrcs; tidx++ )
    {
	const OffsetValueSeries<float> ovs =
			    sdp->getTrcStorage( 0, trcidx0+tidx );
	for ( int zidx=0; zidx<nrz; zidx++ )
	{
	    const float val = ovs[zidx0+zidx];
	    seeddata->set( tidx, zidx, val );
	}
    }

    StepInterval<double> trcrg;
    trcrg.start = tk.trcNr() - (nrtrcs)/2;
    trcrg.stop = tk.trcNr() + (nrtrcs)/2;
    StepInterval<double> zrg;
    zrg.start = mCast(double,zsamp.atIndex(zidx0));
    zrg.stop = mCast(double,zsamp.atIndex(zidx0+nrz-1));
    zrg.step = mCast(double,zsamp.step);

    FlatDataPack* fdp = new FlatDataPack( "Seismics", seeddata );
    fdp->posData().setRange( true, trcrg );
    fdp->posData().setRange( false, zrg );
    DPM(DataPackMgr::FlatID()).add( fdp );
    return fdp->id();
}


ObjectEditor* Engine::getEditor( const EM::ObjectID& id, bool create )
{
    for ( int idx=0; idx<editors_.size(); idx++ )
    {
	if ( editors_[idx]->emObject().id()==id )
	{
	    if ( create ) editors_[idx]->addUser();
	    return editors_[idx];
	}
    }

    if ( !create ) return 0;

    EM::EMObject* emobj = EM::EMM().getObject(id);
    if ( !emobj ) return 0;

    ObjectEditor* editor = EditorFactory().create( emobj->getTypeStr(), *emobj);
    if ( !editor )
	return 0;

    editors_ += editor;
    editor->ref();
    editor->addUser();
    return editor;
}


void Engine::removeEditor( const EM::ObjectID& objid )
{
    ObjectEditor* editor = getEditor( objid, false );
    if ( editor )
    {
	editor->removeUser();
	if ( editor->nrUsers() == 0 )
	{
	    editors_ -= editor;
	    editor->unRef();
	}
    }
}


const char* Engine::errMsg() const
{ return errmsg_.str(); }


BufferString Engine::setupFileName( const MultiID& mid ) const
{
    PtrMan<IOObj> ioobj = IOM().get( mid );
    return ioobj.ptr() ? EM::Surface::getSetupFileName(*ioobj)
		       : BufferString("");
}


void Engine::fillPar( IOPar& iopar ) const
{
    int trackeridx = 0;
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	const EMTracker* tracker = trackers_[idx];
	if ( !tracker ) continue;

	IOPar localpar;
	localpar.set( sKeyObjectID(),EM::EMM().getMultiID(tracker->objectID()));
	localpar.setYN( sKeyEnabled(), tracker->isEnabled() );

	EMSeedPicker* seedpicker =
			const_cast<EMTracker*>(tracker)->getSeedPicker(false);
	if ( seedpicker )
	    localpar.set( sKeySeedConMode(), seedpicker->getSeedConnectMode() );

//	tracker->fillPar( localpar );

	iopar.mergeComp( localpar, toString(trackeridx) );
	trackeridx++;
    }

    iopar.set( sKeyNrTrackers(), trackeridx );
    activevolume_.fillPar( iopar );
}


bool Engine::usePar( const IOPar& iopar )
{
    init();

    TrcKeyZSampling newvolume;
    if ( newvolume.usePar(iopar) )
	setActiveVolume( newvolume );

    /* The setting of the active volume must be above the initiation of the
       trackers to avoid a trigger of dataloading. */

    int nrtrackers = 0;
    iopar.get( sKeyNrTrackers(), nrtrackers );

    for ( int idx=0; idx<nrtrackers; idx++ )
    {
	PtrMan<IOPar> localpar = iopar.subselect( toString(idx) );
	if ( !localpar ) continue;

	if ( !localpar->get(sKeyObjectID(),midtoload) ) continue;
	EM::ObjectID oid = EM::EMM().getObjectID( midtoload );
	EM::EMObject* emobj = EM::EMM().getObject( oid );
	if ( !emobj )
	{
	    loadEMObject.trigger();
	    oid = EM::EMM().getObjectID( midtoload );
	    emobj = EM::EMM().getObject( oid );
	    if ( emobj ) emobj->ref();
	}

	if ( !emobj )
	{
	    PtrMan<Executor> exec =
		EM::EMM().objectLoader( MPE::engine().midtoload );
	    if ( exec ) exec->execute();

	    oid = EM::EMM().getObjectID( midtoload );
	    emobj = EM::EMM().getObject( oid );
	    if ( emobj ) emobj->ref();
	}

	if ( !emobj )
	    continue;

	const int trackeridx = addTracker( emobj );
	emobj->unRefNoDelete();
	if ( trackeridx < 0 ) continue;
	EMTracker* tracker = trackers_[trackeridx];

	bool doenable = true;
	localpar->getYN( sKeyEnabled(), doenable );
	tracker->enable( doenable );

	int seedconmode = -1;
	localpar->get( sKeySeedConMode(), seedconmode );
	EMSeedPicker* seedpicker = tracker->getSeedPicker(true);
	if ( seedpicker && seedconmode!=-1 )
	    seedpicker->setSeedConnectMode( seedconmode );

	// old restore session policy without separate tracking setup file
	tracker->usePar( *localpar );
    }

    return true;
}


void Engine::init()
{
    deepUnRef( trackers_ );
    deepUnRef( editors_ );
    deepErase( attribcachespecs_ );
    deepErase( attribbackupcachespecs_ );
    deepErase( flatcubescontainer_ );
    deepErase( trackermgrs_ );

    for ( int idx=0; idx<attribcachedatapackids_.size(); idx++ )
	dpm_.release( attribcachedatapackids_[idx] );
    for ( int idx=0; idx<attribbkpcachedatapackids_.size(); idx++ )
	dpm_.release( attribbkpcachedatapackids_[idx] );

    attribcachedatapackids_.erase();
    attribbkpcachedatapackids_.erase();
}

} // namespace MPE
