/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: mpeengine.cc,v 1.75 2007-02-13 13:22:37 cvsjaap Exp $";

#include "mpeengine.h"

#include "attribsel.h"
#include "attribdatacubes.h"
#include "bufstringset.h"
#include "ctxtioobj.h"
#include "emeditor.h"
#include "emmanager.h"
#include "emobject.h"
#include "emsurfauxdataio.h"
#include "emseedpicker.h"
#include "emtracker.h"
#include "executor.h"
#include "filepath.h"
#include "geomelement.h"
#include "ioman.h"
#include "ioobject.h"
#include "iopar.h"
#include "linekey.h"
#include "sectiontracker.h"
#include "survinfo.h"


#define mRetErr( msg, retval ) { errmsg_ = msg; return retval; }


MPE::Engine& MPE::engine()
{
    static MPE::Engine* theinst_ = 0;
    if ( !theinst_ )
	theinst_ = new MPE::Engine;
    return *theinst_;
}


namespace MPE 
{

Engine::Engine()
    : trackplanechange( this )
    , activevolumechange( this )
    , trackeraddremove( this )
    , loadEMObject( this )
{
    trackers_.allowNull(true);
    init();
}


Engine::~Engine()
{
    deepErase( trackers_ );
    deepUnRef( editors_ );
    deepUnRef( attribcache_ );
    deepErase( attribcachespecs_ );
    deepUnRef( attribbackupcache_ );
    deepErase( attribbackupcachespecs_ );
    deepErase( trackerfactories_ );
    deepErase( editorfactories_ );
}


const CubeSampling& Engine::activeVolume() const
{ return activevolume_; }


void Engine::setActiveVolume( const CubeSampling& nav )
{
    activevolume_ = nav;

    int dim = 0;
    if ( trackplane_.boundingBox().hrg.start.crl==
	 trackplane_.boundingBox().hrg.stop.crl )
	dim = 1;
    else if ( !trackplane_.boundingBox().zrg.width() )
	dim = 2;

    TrackPlane ntp;
    CubeSampling& ncs = ntp.boundingBox();
    ncs = nav;

    if ( !dim )
	ncs.hrg.start.inl = ncs.hrg.stop.inl =
	    SI().inlRange(true).snap((ncs.hrg.start.inl+ncs.hrg.stop.inl)/2);
    else if ( dim==1 )
	ncs.hrg.start.crl = ncs.hrg.stop.crl =
	    SI().crlRange(true).snap((ncs.hrg.start.crl+ncs.hrg.stop.crl)/2);
    else
	ncs.zrg.start = ncs.zrg.stop = SI().zRange(true).snap(ncs.zrg.center());

    ntp.setTrackMode( trackPlane().getTrackMode() );
    setTrackPlane( ntp, false );
    activevolumechange.trigger();
}


const TrackPlane& Engine::trackPlane() const
{ return trackplane_; }


bool Engine::setTrackPlane( const TrackPlane& ntp, bool dotrack )
{
    trackplane_ = ntp;
    trackplanechange.trigger();
    if ( ntp.getTrackMode()==TrackPlane::Move ||
	    ntp.getTrackMode()==TrackPlane::None )
	dotrack = false;

    return dotrack ? trackAtCurrentPlane() : true;
}


#define mTrackAtCurrentPlane(func) \
    for ( int idx=0; idx<trackers_.size(); idx++ ) \
    { \
	if ( !trackers_[idx] || !trackers_[idx]->isEnabled() ) \
	    continue; \
	EM::ObjectID oid = trackers_[idx]->objectID(); \
	if ( EM::EMM().getObject(oid)->isLocked() ) \
	    continue; \
	if ( !trackers_[idx]->func( trackplane_ ) ) \
	{ \
	    errmsg_ = "Error while tracking "; \
	    errmsg_ += trackers_[idx]->objectName(); \
	    errmsg_ += ": "; \
	    errmsg_ += trackers_[idx]->errMsg(); \
	    return false; \
	} \
    } 

bool Engine::trackAtCurrentPlane()
{
    mTrackAtCurrentPlane(trackSections);
    mTrackAtCurrentPlane(trackIntersections);
    return true;
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


void Engine::setTrackMode( TrackPlane::TrackMode tm )
{
    trackplane_.setTrackMode( tm );
    trackplanechange.trigger();
}


void Engine::getAvailableTrackerTypes( BufferStringSet& res ) const
{
    res.deepErase();
    for ( int idx=0; idx<trackerfactories_.size(); idx++ )
	res.add(trackerfactories_[idx]->emObjectType());
}


int Engine::addTracker( EM::EMObject* obj )
{
    if ( !obj )
	mRetErr( "No valid object", -1 );

    if ( getTrackerByObject(obj->id()) != -1 )
	mRetErr( "Object is already tracked", -1 );

    bool added = false;
    for ( int idx=0; idx<trackerfactories_.size(); idx++ )
    {
	if ( !strcmp(obj->getTypeStr(),trackerfactories_[idx]->emObjectType()) )
	{
	    EMTracker* tracker = trackerfactories_[idx]->create(obj);
	    trackers_ += tracker;
	    added = true;
	    break;
	}
    }

    if ( !added )
	mRetErr( "Cannot find this trackertype", -1 );

    trackeraddremove.trigger();

    return trackers_.size()-1;
}


void Engine::removeTracker( int idx )
{
    if ( idx<0 || idx>=trackers_.size() || !trackers_[idx] )
	return;

    delete trackers_[idx];
    trackers_.replace( idx, 0 );

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

	if ( !strcmp(objname,trackers_[idx]->objectName()) )
	    return idx;
    }

    return -1;
}


void Engine::getNeededAttribs( ObjectSet<const Attrib::SelSpec>& res ) const
{
    for ( int trackeridx=0; trackeridx<trackers_.size(); trackeridx++ )
    {
	const EMTracker* tracker = trackers_[trackeridx];
	if ( !tracker ) continue;

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


CubeSampling Engine::getAttribCube( const Attrib::SelSpec& as ) const
{
    CubeSampling res( engine().activeVolume() );
    for ( int idx=0; idx<trackers_.size(); idx++ )
    {
	if ( !trackers_[idx] ) continue;
	const CubeSampling cs = trackers_[idx]->getAttribCube( as );
	res.include(cs);
    }

    return res;
}
    

const Attrib::DataCubes* Engine::getAttribCache(const Attrib::SelSpec& as) const
{
    Attrib::SelSpec attrselspec(as);
    if ( as.is2D() )
	attrselspec.setUserRef( LineKey(as.userRef()).attrName() );   

    for ( int idx=0; idx<attribcachespecs_.size(); idx++ )
    {
	if ( *attribcachespecs_[idx]==attrselspec )
	    return idx<attribcache_.size() ? attribcache_[idx] : 0;
    }

    return 0;
}


bool Engine::setAttribData( const Attrib::SelSpec& as, 
			    const Attrib::DataCubes* newdata )
{
    bool found = false;
    for ( int idx=0; idx<attribcachespecs_.size(); idx++ )
    {
	if ( *attribcachespecs_[idx]==as )
	{
	    attribcache_[idx]->unRef();
	    if ( !newdata )
	    {
		attribcache_.remove(idx);
		attribcachespecs_.remove(idx);
	    }
	    else
	    {
		attribcache_.replace(idx, newdata);
		newdata->ref();
	    }

	    found = true;
	    break;
	}
    }

    if ( newdata && !found )
    {
	attribcachespecs_ += new Attrib::SelSpec(as);
	attribcache_ += newdata;
	newdata->ref();
    }

    return true;
}


bool Engine::cacheIncludes( const Attrib::SelSpec& as, 
			    const CubeSampling& cs ) const
{
    const Attrib::DataCubes* cache = getAttribCache( as );
    if ( !cache ) 
	return false;

    CubeSampling cachedcs = cache->cubeSampling();
    const float zrgeps = 0.01 * SI().zStep();
    cachedcs.zrg.widen( zrgeps );  
    
    return cachedcs.includes( cs );
}


void Engine::swapCacheAndItsBackup()
{
    const ObjectSet<const Attrib::DataCubes> tempcache = attribcache_;
    const ObjectSet<Attrib::SelSpec> tempcachespecs = attribcachespecs_;
    attribcache_ = attribbackupcache_;
    attribcachespecs_ = attribbackupcachespecs_;
    attribbackupcache_ = tempcache;
    attribbackupcachespecs_ = tempcachespecs;
}


ObjectEditor* Engine::getEditor( const EM::ObjectID& id, bool create )
{
    for ( int idx=0; idx<editors_.size(); idx++ )
    {
	if ( editors_[idx]->emObject().id()==id )
	{
	    editors_[idx]->ref();
	    return editors_[idx];
	}
    }

    if ( !create ) return 0;

    EM::EMObject* emobj = EM::EMM().getObject(id);
    if ( !emobj ) return 0;

    for ( int idx=0; idx<editorfactories_.size(); idx++ )
    {
	if ( strcmp(editorfactories_[idx]->emObjectType(), emobj->getTypeStr()) )
	    continue;

	ObjectEditor* editor = editorfactories_[idx]->create(*emobj);
	editors_ += editor;
	editor->ref();
	return editor;
    }

    return 0;
}


void Engine::removeEditor( const EM::ObjectID& objid )
{
    ObjectEditor* editor = getEditor( objid, false );
    if ( editor )
    {
	editors_ -= editor;
	editor->unRef();
    }
}


const char* Engine::errMsg() const
{ return errmsg_[0] ? (const char*) errmsg_ : 0 ; }


void Engine::addTrackerFactory( TrackerFactory* ntf )
{
    for ( int idx=0; idx<trackerfactories_.size(); idx++ )
    {
	if ( !strcmp(ntf->emObjectType(),trackerfactories_[idx]->emObjectType()))
	{
	    delete ntf;
	    return;
	}
    }

    trackerfactories_ += ntf;
}


void Engine::addEditorFactory( EditorFactory* nef )
{
    for ( int idx=0; idx<editorfactories_.size(); idx++ )
    {
	if ( !strcmp(nef->emObjectType(),editorfactories_[idx]->emObjectType()))
	{
	    delete nef;
	    return;
	}
    }

    editorfactories_ += nef;
}


CubeSampling Engine::getDefaultActiveVolume()
{
    CubeSampling cs;
    cs.hrg.start.inl=(5*SI().inlRange(true).start+3*SI().inlRange(true).stop)/8;
    cs.hrg.start.crl=(5*SI().crlRange(true).start+3*SI().crlRange(true).stop)/8;
    cs.hrg.stop.inl =(3*SI().inlRange(true).start+5*SI().inlRange(true).stop)/8;
    cs.hrg.stop.crl =(3*SI().crlRange(true).start+5*SI().crlRange(true).stop)/8;
    cs.zrg.start = (5*SI().zRange(true).start+3*SI().zRange(true).stop)/ 8;
    cs.zrg.stop = (3*SI().zRange(true).start+5*SI().zRange(true).stop)/8;
    SI().snap( cs.hrg.start, BinID(0,0) );
    SI().snap( cs.hrg.stop, BinID(0,0) );
    float z0 = SI().zRange(true).snap( cs.zrg.start ); cs.zrg.start = z0;
    float z1 = SI().zRange(true).snap( cs.zrg.stop ); cs.zrg.stop = z1;
    return cs;
}



BufferString Engine::setupFileName( const MultiID& mid ) const
{
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( ioobj )
    {
	FilePath surffilenm( ioobj->fullUserExpr(true) );
	BufferString surfstr = surffilenm.fileName();
	BufferString setupstr = EM::dgbSurfDataWriter::createSetupName(surfstr);
	FilePath setupfilenm( setupstr );
	FilePath setuppath( IOObjContext::getDataDirName(IOObjContext::Surf) );
	setupfilenm.setPath( setuppath.fullPath() );
	return BufferString( setupfilenm.fullPath() );
    }

    return BufferString("");
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

	BufferString key( trackeridx );
	iopar.mergeComp( localpar, key );
	trackeridx++;
    }

    iopar.set( sKeyNrTrackers(), trackeridx );
    activevolume_.fillPar( iopar );

    IOPar tppar;
    trackplane_.fillPar( tppar );
    iopar.mergeComp( tppar, sKeyTrackPlane() );
}


bool Engine::usePar( const IOPar& iopar )
{
    init();

    CubeSampling newvolume;
    if ( newvolume.usePar(iopar) )
    {
	setActiveVolume( newvolume );
	PtrMan<IOPar> tppar = iopar.subselect( sKeyTrackPlane() );
	if ( tppar ) trackplane_.usePar( *tppar );
    }

    /* The setting of the active volume must be above the initiation of the
       trackers to avoid a trigger of dataloading. */

    int nrtrackers = 0;
    iopar.get( sKeyNrTrackers(), nrtrackers );

    for ( int idx=0; idx<nrtrackers; idx++ )
    {
	BufferString key( idx );
	PtrMan<IOPar> localpar = iopar.subselect( key );
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
    deepErase( trackers_ );
    deepUnRef( editors_ );
    deepUnRef( attribcache_ );
    deepErase( attribcachespecs_ );
    deepUnRef( attribbackupcache_ );
    deepErase( attribbackupcachespecs_ );
    setActiveVolume( getDefaultActiveVolume() );
}



}; // namespace MPE


#include "emhortubeeditor.h"
#include "emhortubetracker.h"
#include "faulteditor.h"
#include "faulttracker.h"
#include "horizoneditor.h"
#include "horizontracker.h"
#include "horizon2dtracker.h"


void MPE::initStandardClasses()
{
#ifdef __debug__
    MPE::HorizontalTubeEditor::initClass();
    MPE::HorizontalTubeTracker::initClass();
    MPE::FaultEditor::initClass();
    MPE::FaultTracker::initClass();
#endif
    MPE::HorizonEditor::initClass();
    MPE::HorizonTracker::initClass();
    MPE::Horizon2DTracker::initClass();
}
