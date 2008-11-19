/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Sep 2005
___________________________________________________________________

-*/

static const char* rcsID = "$Id: horizon3dseedpicker.cc,v 1.28 2008-11-19 08:28:50 cvsjaap Exp $";

#include "horizon3dseedpicker.h"

#include "autotracker.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "sectiontracker.h"
#include "executor.h"
#include "mpeengine.h"
#include "survinfo.h"
#include "sorting.h"


namespace MPE 
{

Horizon3DSeedPicker::Horizon3DSeedPicker( MPE::EMTracker& t )
    : tracker_( t )
    , addrmseed_( this )
    , surfchange_( this )
    , seedconmode_( defaultSeedConMode() )
    , blockpicking_( false )
    , sectionid_( -1 )
{ }


bool Horizon3DSeedPicker::setSectionID( const EM::SectionID& sid )
{ 
    sectionid_ = sid; 
    return true; 
}


#define mGetHorizon(hor) \
    const EM::ObjectID emobjid = tracker_.objectID(); \
    mDynamicCastGet( EM::Horizon3D*, hor, EM::EMM().getObject(emobjid) ); \
    if ( !hor ) \
	return false;\

bool Horizon3DSeedPicker::startSeedPick()
{
    mGetHorizon(hor);
    didchecksupport_ = hor->enableGeometryChecks( false );
    return true;
}

	
bool Horizon3DSeedPicker::addSeed(const Coord3& seedcrd, bool drop )
{
    addrmseed_.trigger();

    if ( blockpicking_ ) 
	return true;

    const BinID seedbid = SI().transform( seedcrd );
    const HorSampling hrg = engine().activeVolume().hrg;
    const StepInterval<float> zrg = engine().activeVolume().zrg;
    if ( !zrg.includes(seedcrd.z) || !hrg.includes(seedbid) )
	return false;

    EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );
    const EM::PosID pid( emobj->id(), sectionid_, seedbid.getSerialized() );
    propagatelist_.erase();
    propagatelist_ += pid;

    const bool pickedposwasdef = emobj->isDefined( pid );
    if ( !drop || !pickedposwasdef )
    {
	emobj->setPos( pid, seedcrd, true );
	if ( seedconmode_ != DrawBetweenSeeds )
	    tracker_.snapPositions( propagatelist_ );
    }

    emobj->setPosAttrib( pid, EM::EMObject::sSeedNode, true );

    bool res = true;

    // Loop may add new items to the tail of the propagate list.
    for ( int idx=0; idx<propagatelist_.size() && !drop; idx++ )
    {
	seedlist_.erase(); seedpos_.erase(); 
	seedlist_ += propagatelist_[idx];  
	const Coord3 pos = emobj->getPos( propagatelist_[idx] );
	seedpos_ += pos;

	const bool startwasdef = idx ? true : pickedposwasdef;
        res = retrackOnActiveLine( SI().transform(pos), startwasdef ) && res;
    }

    surfchange_.trigger();
    return res;
}


bool Horizon3DSeedPicker::removeSeed( const EM::PosID& pid, bool environment, 
				    bool retrack )
{
    addrmseed_.trigger();
    
    if ( blockpicking_ ) 
	return true;
    
    EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );  
    const Coord3 oldpos = emobj->getPos(pid);
    BinID seedbid = SI().transform( oldpos );

    const bool attribwasdef = emobj->isPosAttrib( pid, EM::EMObject::sSeedNode);
    emobj->setPosAttrib( pid, EM::EMObject::sSeedNode, false );

    if ( environment || nrLineNeighbors(pid)+nrLateralNeighbors(pid)==0 )
	emobj->unSetPos( pid, true );

    const bool repairbridge = retrack || nrLineNeighbors(pid); 
    int res = true;

    if ( environment )
    {
	propagatelist_.erase(); seedlist_.erase(); seedpos_.erase(); 

	res = retrackOnActiveLine( seedbid, true, !retrack );

	if ( repairbridge && nrLateralNeighbors(pid) )
	{
	    if ( !emobj->isDefined( pid ) )
		emobj->setPos( pid, oldpos, true );

	    if  ( attribwasdef )
		emobj->setPosAttrib( pid, EM::EMObject::sSeedNode, true );
	}
    }

    surfchange_.trigger();
    return res;
}


bool Horizon3DSeedPicker::reTrack()
{
    propagatelist_.erase(); seedlist_.erase(); seedpos_.erase(); 
    
    const bool res = retrackOnActiveLine( BinID(-1,-1), false );
    surfchange_.trigger();
    return res;
}


bool Horizon3DSeedPicker::retrackOnActiveLine( const BinID& startbid, 
					     bool startwasdefined,
					     bool eraseonly ) 
{
    BinID dir;
    if ( !lineTrackDirection(dir) ) return false;

    trackbounds_.erase();
    junctions_.erase();

    if ( engine().activeVolume().hrg.includes(startbid) )
    {
	extendSeedListEraseInBetween( false, startbid, startwasdefined, -dir );
	extendSeedListEraseInBetween( false, startbid, startwasdefined, dir );
    }
    else
    {
	// traverse whole active line
	const BinID dummystartbid = engine().activeVolume().hrg.start;
	extendSeedListEraseInBetween( true, dummystartbid, false, -dir );
	extendSeedListEraseInBetween( true, dummystartbid-dir, false, dir );
	
	if ( seedconmode_ != DrawBetweenSeeds )
	    tracker_.snapPositions( seedlist_ );
    }
   
    if ( eraseonly )
	return true;

    const bool res = retrackFromSeedList();
    processJunctions();

    return res;
}


void Horizon3DSeedPicker::processJunctions()
{
    EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );  

    for ( int idx=0; idx<junctions_.size(); idx+=3 )
    {
	const EM::PosID prevpid = junctions_[idx];
	if ( !emobj->isDefined(prevpid) )
	    continue;

	const EM::PosID curpid = junctions_[idx+1];
	emobj->setPosAttrib( curpid, EM::EMObject::sSeedNode, true );

	const EM::PosID nextpid = junctions_[idx+2];
	if ( seedconmode_!=TrackFromSeeds || emobj->isDefined(nextpid) )
	    continue;

	const Coord3 oldcurpos = emobj->getPos( curpid );
	TypeSet<EM::PosID> curpidlisted;
	curpidlisted += curpid;
	tracker_.snapPositions( curpidlisted );
	const Coord3 snappedoldpos = emobj->getPos( curpid );
	
	Coord3 newcurpos = oldcurpos;
	newcurpos.z = emobj->getPos(prevpid).z;
	emobj->setPos( curpid, newcurpos , false );
	tracker_.snapPositions( curpidlisted );
	const Coord3 snappednewpos = emobj->getPos( curpid );
	
	if ( snappednewpos==snappedoldpos && propagatelist_.indexOf(curpid)<0 )
	    propagatelist_ += curpid;

	emobj->setPos( curpid, oldcurpos , true );
    }
}


#define mStoreSeed( onewaytracking ) \
    if ( onewaytracking ) \
	trackbounds_ += curbid; \
    else \
    { \
	seedlist_ += curpid; \
	seedpos_ += emobj->getPos( curpid ); \
    } 

#define mStoreJunction() \
{ \
    junctions_ += prevpid; \
    junctions_ += curpid; \
    const BinID nextbid = curbid + dir; \
    junctions_ += EM::PosID(emobj->id(),sectionid_,nextbid.getSerialized()); \
}

void Horizon3DSeedPicker::extendSeedListEraseInBetween( 
				    bool wholeline, const BinID& startbid, 
				    bool startwasdefined, const BinID& dir )
{
    eraselist_.erase();
    EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );  
    EM::PosID curpid = EM::PosID( emobj->id(), sectionid_, 
				  startbid.getSerialized() ); 

    bool seedwasadded = emobj->isDefined( curpid ) && !wholeline;

    BinID curbid = startbid;
    bool curdefined = startwasdefined;

    while ( true )
    {
	const BinID prevbid = curbid;
	const EM::PosID prevpid = curpid;
	const bool prevdefined = curdefined;
	
    	curbid += dir;
	
	// reaching end of survey
	if ( !engine().activeVolume().hrg.includes(curbid) )
	{
	    if ( seedconmode_==TrackFromSeeds )
		trackbounds_ += prevbid;
	    
	    if ( seedwasadded && seedconmode_!=TrackFromSeeds )
		return;
		
	    break;
	}
	
	curpid = EM::PosID( emobj->id(), sectionid_, curbid.getSerialized() ); 
	curdefined = emobj->isDefined( curpid );

	// running into a seed point
	if ( emobj->isPosAttrib( curpid, EM::EMObject::sSeedNode ) )
	{
	    const bool onewaytracking = seedwasadded && !prevdefined &&
					seedconmode_==TrackFromSeeds;
	    mStoreSeed( onewaytracking );

	    if ( onewaytracking )
		mStoreJunction();
	    
	    if ( wholeline )
		continue;
	    
	    break;
	}

	// running into a loose end
	if ( !wholeline && !prevdefined && curdefined )
	{
	    mStoreSeed( seedconmode_==TrackFromSeeds );
	    mStoreJunction();
	    break;
	}

	// to erase points attached to start
	if ( curdefined ) 
	    eraselist_ += curpid;
    }

    for ( int idx=0; idx<eraselist_.size(); idx++ )
	emobj->unSetPos( eraselist_[idx], true );
}


bool Horizon3DSeedPicker::retrackFromSeedList()
{
    if ( seedlist_.isEmpty() )
	return true;
    if ( blockpicking_ ) 
	return true;
    if ( seedconmode_ == DrawBetweenSeeds )
	return interpolateSeeds(); 
    
    const TrackPlane::TrackMode tm = engine().trackPlane().getTrackMode();
    engine().setTrackMode( TrackPlane::Extend );

    PtrMan<Executor> execfromeng = engine().trackInVolume();

    mDynamicCastGet( ExecutorGroup*, trkersgrp, execfromeng.ptr() );
    if ( !trkersgrp )
	return false;

    for( int trker=0; trker<trkersgrp->nrExecutors(); ++trker)
    {
	Executor* exectrk = trkersgrp->getExecutor(trker);
	mDynamicCastGet( ExecutorGroup*, sectiongrp, exectrk );
	if ( !sectiongrp )
	    break;
	
	for( int section=0; section<sectiongrp->nrExecutors(); ++section )
	{
	    Executor* exec = sectiongrp->getExecutor(section);
	    if ( !exec )
		break;

	    mDynamicCastGet( AutoTracker*, autotrk, exec );
	    if ( !autotrk )
		break;
	    autotrk->setTrackBoundary( getTrackBox() );
	    autotrk->setNewSeeds( seedlist_ );
	    autotrk->execute();
	    autotrk->unsetTrackBoundary();
	}
    }

    engine().setTrackMode( tm );

    return true;
}


int Horizon3DSeedPicker::nrSeeds() const
{
    EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );  
    const TypeSet<EM::PosID>* seednodelist = 
			emobj->getPosAttribList( EM::EMObject::sSeedNode );
    return seednodelist ? seednodelist->size() : 0;
}

const char* Horizon3DSeedPicker::seedConModeText( int mode, bool abbrev )
{
    if ( mode==TrackFromSeeds && !abbrev )
	return "Tracking in volume";
    else if ( mode==TrackFromSeeds && abbrev )
	return "Volume track";
    else if ( mode==TrackBetweenSeeds )
	return "Line tracking";
    else if ( mode==DrawBetweenSeeds )
	return "Line manual";
    else
	return "Unknown mode";
}

int Horizon3DSeedPicker::minSeedsToLeaveInitStage() const
{ 
    if ( seedconmode_==TrackFromSeeds ) 
	return 1;
    else if ( seedconmode_==TrackBetweenSeeds ) 
	return 2;
    else 
	return 0 ; 
}


bool Horizon3DSeedPicker::doesModeUseVolume() const
{ return seedconmode_==TrackFromSeeds; }


bool Horizon3DSeedPicker::doesModeUseSetup() const
{ return seedconmode_!=DrawBetweenSeeds; }


int Horizon3DSeedPicker::defaultSeedConMode( bool gotsetup ) const 
{ return gotsetup ? defaultSeedConMode() : DrawBetweenSeeds; }


bool Horizon3DSeedPicker::stopSeedPick( bool iscancel )
{
    mGetHorizon(hor);
    hor->enableGeometryChecks( didchecksupport_ );
    return true;
}


bool Horizon3DSeedPicker::lineTrackDirection( BinID& dir, 
					    bool perptotrackdir ) const
{
    const CubeSampling& activevol = engine().activeVolume();
    dir = activevol.hrg.step;
    
    const bool inltracking = activevol.nrInl()==1 && activevol.nrCrl()>1;
    const bool crltracking = activevol.nrCrl()==1 && activevol.nrInl()>1;

    if ( !inltracking && !crltracking )
	return false;

    if ( (!perptotrackdir && inltracking) || (perptotrackdir && crltracking) )
	dir.inl = 0; 
    else
	dir.crl = 0;

    return true;
}


int Horizon3DSeedPicker::nrLateralNeighbors( const EM::PosID& pid ) const
{
    return nrLineNeighbors( pid, true );
}


int Horizon3DSeedPicker::nrLineNeighbors( const EM::PosID& pid, 
				        bool perptotrackdir ) const
{
    EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );  
    BinID bid = SI().transform( emobj->getPos(pid) );

    TypeSet<EM::PosID> neighpid;
    mGetHorizon(hor);
    hor->geometry().getConnectedPos( pid, &neighpid );
   
    BinID dir;
    if ( !lineTrackDirection(dir,perptotrackdir) )
	return -1;

    int total = 0;
    for ( int idx=0; idx<neighpid.size(); idx++ )
    {
	BinID neighbid = SI().transform( emobj->getPos(neighpid[idx]) ); 
	if ( bid.isNeighborTo(neighbid,dir) )
	    total++;
    }
    return total;
}


bool Horizon3DSeedPicker::interpolateSeeds()
{
    BinID dir;
    if ( !lineTrackDirection(dir) ) return false;

    const int step = dir.inl ? dir.inl : dir.crl;

    const int nrseeds = seedlist_.size();
    if ( nrseeds<2 ) 
	return true;

    mVariableLengthArr( int, sortval, nrseeds );
    mVariableLengthArr( int, sortidx, nrseeds );

    for ( int idx=0; idx<nrseeds; idx++ )
    {
	const BinID seedbid = SI().transform( seedpos_[idx] );
	sortval[idx] = dir.inl ? seedbid.inl : seedbid.crl;
	sortidx[idx] = idx;
    }

#ifdef __msvc__
    sort_coupled( sortval.ptr(), sortidx.ptr(), nrseeds );
#else
    sort_coupled( sortval, sortidx, nrseeds );
#endif

    EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );  
    for ( int vtx=0; vtx<nrseeds-1; vtx++ ) 
    { 
	const int diff = sortval[vtx+1] - sortval[vtx]; 
	for ( int idx=step; idx<diff; idx+=step ) 
	{ 
	    const double frac = (double) idx / diff; 
	    const Coord3 interpos = (1-frac) * seedpos_[ sortidx[vtx] ] + 
				       frac  * seedpos_[ sortidx[vtx+1] ];
	    const EM::PosID interpid( emobj->id(), sectionid_, 
				SI().transform(interpos).getSerialized() ); 
	    emobj->setPos( interpid, interpos, true ); 
	} 
    } 
    return true;
}


CubeSampling Horizon3DSeedPicker::getTrackBox() const
{
    CubeSampling trackbox( true );
    trackbox.hrg.init( false );
    for ( int idx=0; idx<seedpos_.size(); idx++ )
    {
	const BinID seedbid = SI().transform( seedpos_[idx] );
	if ( engine().activeVolume().hrg.includes(seedbid) )
	    trackbox.hrg.include( seedbid );
    }

    for ( int idx=0; idx<trackbounds_.size(); idx++ )
	trackbox.hrg.include( trackbounds_[idx] );
    
    return trackbox;
}


}; // namespace MPE

