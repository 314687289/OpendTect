/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "mpeengine.h"

#include "attribsel.h"
#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "attribdataholder.h"
#include "attribstorprovider.h"
#include "bufstringset.h"
#include "ctxtioobj.h"
#include "emeditor.h"
#include "emmanager.h"
#include "emseedpicker.h"
#include "emsurface.h"
#include "emtracker.h"
#include "executor.h"
#include "geomelement.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "linekey.h"
#include "sectiontracker.h"
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

DataHolder::DataHolder()
	: AbstDataHolder()
	, is2d_( false )
	, dcdata_( 0 )
	, d2dhdata_( 0 )
{
    tkzs_.setEmpty();
}


DataHolder::~DataHolder()
{
    releaseMemory();
}


void DataHolder::set3DData( const Attrib::DataCubes* dc )
{
    releaseMemory();
    is2d_ = false;
    dcdata_ = dc;
    refPtr( dcdata_ );
}


void DataHolder::set2DData( const Attrib::Data2DArray* d2h )
{
    releaseMemory();
    is2d_ = true;
    d2dhdata_ = d2h;
    refPtr( d2dhdata_ );
}


int DataHolder::nrCubes() const
{
    if ( !dcdata_ && !d2dhdata_ )
	return 0;
    if ( !is2d_ && dcdata_ )
	return dcdata_->nrCubes();
    if ( is2d_ && d2dhdata_ )
	return d2dhdata_->nrTraces();

    return 0;
}


void DataHolder::releaseMemory()
{
    unRefAndZeroPtr( dcdata_ );
    unRefAndZeroPtr( d2dhdata_ );
}


Engine::Engine()
    : activevolumechange( this )
    , trackeraddremove( this )
    , loadEMObject( this )
    , oneactivetracker_( 0 )
    , activetracker_( 0 )
    , activegeomid_(Survey::GeometryManager::cUndefGeomID())
    , activefaultid_( -1 )
    , activefssid_( -1 )
    , activefaultchanged_( this )
    , activefsschanged_( this )
{
    trackers_.allowNull( true );
    flatcubescontainer_.allowNull( true );
    init();
}


static void releaseDataPack( DataPack::ID dpid )
{
    DPM(DataPackMgr::FlatID()).release( dpid );
    DPM(DataPackMgr::CubeID()).release( dpid );
}


Engine::~Engine()
{
    deepUnRef( trackers_ );
    deepUnRef( editors_ );
    deepUnRef( attribcache_ );
    deepErase( attribcachespecs_ );
    deepUnRef( attribbackupcache_ );
    deepErase( attribbackupcachespecs_ );
    deepErase( flatcubescontainer_ );

    for ( int idx=attribcachedatapackids_.size()-1; idx>=0; idx-- )
	releaseDataPack( attribcachedatapackids_[idx] );
    for ( int idx=attribbkpcachedatapackids_.size()-1; idx>=0; idx-- )
	releaseDataPack( attribbkpcachedatapackids_[idx] );
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

BufferString Engine::active2DLineName() const
{ return Survey::GM().getName( activegeomid_ ); }


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


Executor* Engine::trackInVolume()
{
    ExecutorGroup* res = 0;
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	if ( !trackers_[idx] || !trackers_[idx]->isEnabled() )
	    continue;

	EM::ObjectID oid = trackers_[idx]->objectID();
	if ( EM::EMM().getObject(oid)->isLocked() )
	    continue;

	if ( !res ) res = new ExecutorGroup("Autotrack", false );

	res->add( trackers_[idx]->trackInVolume() );
    }

    return res;
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


void Engine::getNeededAttribs( ObjectSet<const Attrib::SelSpec>& res ) const
{
    for ( int trackeridx=0; trackeridx<trackers_.size(); trackeridx++ )
    {
	const EMTracker* tracker = trackers_[trackeridx];
	if ( !tracker ) continue;

	if ( oneactivetracker_ && oneactivetracker_!=tracker )
	    continue;

	ObjectSet<const Attrib::SelSpec> specs;
	tracker->getNeededAttribs(specs);
	for ( int idx=0; idx<specs.size(); idx++ )
	{
	    const Attrib::SelSpec* as = specs[idx];
	    if ( indexOf(res,*as) < 0 )
		res += as;
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


const DataHolder* Engine::obtainAttribCache( DataPack::ID datapackid )
{
    const DataPack* datapack = DPM(DataPackMgr::FlatID()).obtain( datapackid );
    if ( !datapack )
	datapack = DPM(DataPackMgr::CubeID()).obtain( datapackid );

    RefMan<DataHolder> dh = new DataHolder();

    mDynamicCastGet(const Attrib::CubeDataPack*,cdp,datapack);
    if ( cdp )
    {
	dh->setTrcKeyZSampling( cdp->cube().cubeSampling() );
	dh->set3DData( &cdp->cube() );
    }
    mDynamicCastGet(const Attrib::Flat3DDataPack*,fdp,datapack);
    if ( fdp )
    {
	dh->setTrcKeyZSampling( fdp->cube().cubeSampling() );
	dh->set3DData( &fdp->cube() );
    }

    mDynamicCastGet(const Attrib::Flat2DDHDataPack*,dp2d,datapack);
    if ( dp2d )
    {
	dh->setTrcKeyZSampling( dp2d->getTrcKeyZSampling() );
	dh->set2DData( dp2d->dataarray() );
    }

    if ( !dh->getTrcKeyZSampling().isEmpty() )
    {
	DataHolder* res = dh.set( 0, false );
	res->unRefNoDelete();
	return res;
    }

    releaseDataPack( datapackid );
    return 0;
}


const DataHolder* Engine::getAttribCache(const Attrib::SelSpec& as)
{
    const int idx = getCacheIndexOf(as);
    return idx>=0 && idx<attribcache_.size() ? attribcache_[idx] : 0;
}


bool Engine::setAttribData( const Attrib::SelSpec& as,
			    DataPack::ID cacheid )
{
    const int idx = getCacheIndexOf(as);
    if ( idx>=0 && idx<attribcachedatapackids_.size() )
    {
	if ( cacheid <= DataPack::cNoID() )
	{
	    releaseDataPack( attribcachedatapackids_[idx] );
	    attribcachedatapackids_.removeSingle( idx );
	    attribcache_.removeSingle( idx )->unRef();
	    delete attribcachespecs_.removeSingle( idx );
	}
	else
	{
	    ConstRefMan<DataHolder> newdata = obtainAttribCache( cacheid );
	    if ( newdata )
	    {
		releaseDataPack( attribcachedatapackids_[idx] );
		attribcache_[idx]->unRef();
		attribcachedatapackids_[idx] = cacheid;
		attribcache_.replace( idx, newdata );
		attribcache_[idx]->ref();
	    }
	}
    }
    else if ( cacheid > DataPack::cNoID() )
    {
	ConstRefMan<DataHolder> newdata = obtainAttribCache( cacheid );
	if ( newdata )
	{
	    attribcachespecs_ += as.is2D() ?
		new CacheSpecs( as, activeGeomID() ) :
		new CacheSpecs( as ) ;

	    attribcachedatapackids_ += cacheid;
	    attribcache_ += newdata;
	    attribcache_[attribcache_.size()-1]->ref();
	}
    }

    return true;
}


bool Engine::setAttribData( const Attrib::SelSpec& as,
			    const DataHolder* newdata )
{
    const int idx = getCacheIndexOf(as);
    if ( idx>=0 && idx<attribcache_.size() )
    {
	attribcache_[idx]->unRef();
	if ( !newdata )
	{
	    attribcache_.removeSingle( idx );
	    delete attribcachespecs_.removeSingle( idx );
	}
	else
	{
	    attribcache_.replace( idx, newdata );
	    newdata->ref();
	}
    }
    else if (newdata)
    {
	attribcachespecs_ += as.is2D() ?
	    new CacheSpecs( as, activeGeomID() ) :
	    new CacheSpecs( as ) ;

	attribcache_ += newdata;
	newdata->ref();
    }

    return true;
}


bool Engine::cacheIncludes( const Attrib::SelSpec& as,
			    const TrcKeyZSampling& cs )
{
    ConstRefMan<DataHolder> cache = getAttribCache( as );
    if ( !cache )
	return false;

    TrcKeyZSampling cachedcs = cache->getTrcKeyZSampling();
    const float zrgeps = 0.01f * SI().zStep();
    cachedcs.zsamp_.widen( zrgeps );

    return cachedcs.includes( cs );
}


void Engine::swapCacheAndItsBackup()
{
    const TypeSet<DataPack::ID> tempcachedatapackids = attribcachedatapackids_;
    const ObjectSet<const DataHolder> tempcache = attribcache_;
    const ObjectSet<CacheSpecs> tempcachespecs = attribcachespecs_;
    attribcachedatapackids_ = attribbkpcachedatapackids_;
    attribcache_ = attribbackupcache_;
    attribcachespecs_ = attribbackupcachespecs_;
    attribbkpcachedatapackids_ = tempcachedatapackids;
    attribbackupcache_ = tempcache;
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
		if ( flatcubes[flatcsidx]->flatcs_.hrg.start.inl() ==
			cs.hrg.start.inl() )
		{
		    idxinquestion = flatcsidx;
		    break;
		}
	    }
	    else if ( flatcubes[flatcsidx]->flatcs_.nrCrl() == 1 )
	    {
		if ( flatcubes[flatcsidx]->flatcs_.hrg.start.crl() ==
		     cs.hrg.start.crl() )
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
    deepUnRef( attribcache_ );
    deepErase( attribcachespecs_ );
    deepUnRef( attribbackupcache_ );
    deepErase( attribbackupcachespecs_ );
    deepErase( flatcubescontainer_ );
}

} // namespace MPE
