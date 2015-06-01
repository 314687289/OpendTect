/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "autotracker.h"

#include "arraynd.h"
#include "binidvalue.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emtracker.h"
#include "emundo.h"
#include "horizonadjuster.h"
#include "mpeengine.h"
#include "progressmeter.h"
#include "sectionadjuster.h"
#include "sectionextender.h"
#include "sectiontracker.h"
#include "survinfo.h"
#include "thread.h"
#include "threadwork.h"
#include "trckeyvalue.h"


namespace MPE
{


class HorizonTracker : public ParallelTask
{
public:
HorizonTracker( HorizonTrackerMgr& mgr, const TrcKeyValue& seed,
		const TrcKeyValue& srcpos )
    : mgr_(mgr)
    , seed_(seed)
    , srcpos_(srcpos)
{
}

protected:
od_int64 nrIterations() const
{ return neighbors_.size(); }

bool doWork( int64_t start, int64_t stop, int )
{
    for ( int idx=(int)start; idx<=stop; idx++ )
    {
	const TrcKey& target = neighbors_[idx];
	mgr_.addTask( seed_, TrcKeyValue(target) );
    }

    return true;
}

    HorizonTrackerMgr&	mgr_;
    TrcKeyValue		seed_;
    TrcKeyValue		srcpos_;

    TypeSet<TrcKey>	neighbors_;
};



HorizonTrackerMgr::HorizonTrackerMgr()
    : twm_(Threads::WorkManager::twm())
{
    queueid_ = twm_.addQueue(
	Threads::WorkManager::MultiThread, "Horizon Tracker" );
}


HorizonTrackerMgr::~HorizonTrackerMgr()
{
    twm_.removeQueue( queueid_, false );
}


void HorizonTrackerMgr::setSeeds( const TypeSet<TrcKey>& seeds )
{ seeds_ = seeds; }


void HorizonTrackerMgr::addTask( const TrcKeyValue& seed,
				 const TrcKeyValue& source )
{
    Task* task = new HorizonTracker( *this, seed, source );
    twm_.addWork( Threads::Work(*task,true), 0, queueid_,
		  false, false, true );
}


void HorizonTrackerMgr::startFromSeeds()
{
    for ( int idx=0; idx<seeds_.size(); idx++ )
	addTask( seeds_[idx], seeds_[idx] );
}




AutoTracker::AutoTracker( EMTracker& et, const EM::SectionID& sid )
    : Executor("Autotracker")
    , emobject_(*EM::EMM().getObject(et.objectID()))
    , sectionid_(sid)
    , sectiontracker_(et.getSectionTracker(sid,true))
    , nrdone_(0)
    , totalnr_(0)
    , nrflushes_(0)
    , flushcntr_(0)
    , stepcntallowedvar_(-1)
    , stepcntapmtthesld_(-1)
    , burstalertactive_(false)
    , horizon3dundoinfo_(0)
{
    geomelem_ = emobject_.sectionGeometry( sectionid_ );
    extender_ = sectiontracker_->extender();
    adjuster_ = sectiontracker_->adjuster();

    reCalculateTotalNr();

    mDynamicCastGet(HorizonAdjuster*,horadj,sectiontracker_->adjuster());
    if ( horadj )
    {
	if ( horadj->useAbsThreshold() )
	{
	    if ( horadj->getAmplitudeThresholds().size()>0 )
	    {
		stepcntapmtthesld_ = 0;
		stepcntallowedvar_ = -1;
		const float th =
		    horadj->getAmplitudeThresholds()[stepcntapmtthesld_];
		horadj->setAmplitudeThreshold( th );
		execmsg_ = tr("Step: %1").arg(th);
	    }
	}
	else if ( horadj->getAllowedVariances().size()>0 )
	{
	    stepcntallowedvar_ = 0;
	    stepcntapmtthesld_ = -1;
	    const float var = horadj->getAllowedVariances()[stepcntallowedvar_];
	    horadj->setAllowedVariance( var );
	    execmsg_ = tr("Step: %1%").arg(var * 100);
	}
    }

    mDynamicCastGet( EM::Horizon3D*, hor, &emobject_ );
    if ( hor )
    {
	horizon3dundoinfo_ = hor->createArray2D( sectionid_ );
	horizon3dundoorigin_.row() =
	    hor->geometry().sectionGeometry(sid)->rowRange().start;
	horizon3dundoorigin_.col() =
	    hor->geometry().sectionGeometry(sid)->colRange().start;
    }
}


AutoTracker::~AutoTracker()
{
    manageCBbuffer( false );
    geomelem_->trimUndefParts();

    if ( horizon3dundoinfo_ ) //TODO check for real change?
    {
	mDynamicCastGet( EM::Horizon3D*, hor, &emobject_ );
	UndoEvent* undo = new EM::SetAllHor3DPosUndoEvent( hor, sectionid_,
				    horizon3dundoinfo_, horizon3dundoorigin_ );
	EM::EMM().undo().addEvent( undo, "Auto tracking" );
	horizon3dundoinfo_ = 0;
    }

    emobject_.setBurstAlert( false );
    burstalertactive_ = false;

    delete horizon3dundoinfo_;
}


void AutoTracker::reCalculateTotalNr()
{
    PtrMan<EM::EMObjectIterator>iterator =
	emobject_.createIterator( sectionid_, &engine().activeVolume() );

    totalnr_ = extender_->maxNrPosInExtArea();

    while( true )
    {
	const EM::PosID pid = iterator->next();
	if ( pid.objectID()==-1 )
	    break;

	totalnr_--;
	if ( !sectiontracker_->propagatingFromSeedOnly() ||
	     emobject_.isPosAttrib(pid,EM::EMObject::sSeedNode()) )
	    addSeed(pid);
    }

    if ( currentseeds_.isEmpty() )
	totalnr_ = 0;
}


void AutoTracker::manageCBbuffer( bool block )
{
    if ( !block )
    {
	geomelem_->blockCallBacks( false, true );
	nrflushes_ = 0; flushcntr_ = 0;
	return;
    }

    // progressive flushing (i.e. wait one extra cycle for every next flush)
    flushcntr_++;
    if ( flushcntr_ >= nrflushes_ )
    {
	flushcntr_ = 0;
	geomelem_->blockCallBacks( true, true );
	nrflushes_++;
    }
}


void AutoTracker::setNewSeeds( const TypeSet<EM::PosID>& seeds )
{
    currentseeds_.erase();

    for( int idx=0; idx<seeds.size(); ++idx )
	addSeed(seeds[idx]);
}


class AutoTrackerRemoveBlackListed : public ParallelTask
{
public:
    AutoTrackerRemoveBlackListed( EM::EMObject& emobj,
				  EM::SectionID sid,
				  TypeSet<EM::SubID>& list,
				  TypeSet<EM::SubID>& srclist,
				  const SortedTable<EM::SubID,char>& bl )
	: list_( list )
	, sectionid_( sid )
	, emobject_( emobj )
	, srclist_( srclist )
	, blacklist_( bl )
    {}

    int	minThreadSize() const		{ return 20; }
    od_int64  nrIterations() const	{ return list_.size(); }

    bool doPrepare(int nrthreads)
    {
	barrier_.setNrThreads( nrthreads );
	return true;
    }

    bool doWork( od_int64 start, od_int64 stop, int )
    {
	TypeSet<EM::SubID> newlist;
	TypeSet<EM::SubID> newsrclist;
	TypeSet<EM::SubID> removelist;
	for ( int idx=mCast(int,start); idx<=stop; idx++ )
	{
	    char count;
	    if ( blacklist_.get( list_[idx], count ) && count>7 )
	    {
		removelist += list_[idx];
		continue;
	    }

	    newlist += list_[idx];
	    newsrclist += srclist_[idx];
	}

	if ( barrier_.waitForAll(false) )
	{
	    srclist_.erase();
	    list_.erase();
	}

	list_.append( newlist );
	srclist_.append( newsrclist );
	for ( int idx=0; idx<removelist.size(); idx++ )
	{
	    const EM::PosID pid( emobject_.id(), sectionid_, removelist[idx] );
	    emobject_.unSetPos(pid,false);
	}

	barrier_.mutex().unLock();

	return true;
    }

protected:
    EM::SectionID			sectionid_;
    EM::EMObject&			emobject_;
    TypeSet<EM::SubID>&			list_;
    TypeSet<EM::SubID>&			srclist_;
    const SortedTable<EM::SubID,char>	blacklist_;

    Threads::Barrier			barrier_;
};



int AutoTracker::nextStep()
{
    manageCBbuffer( true );

    if ( !nrdone_ )
    {
	if ( !burstalertactive_ )
	{
	    emobject_.setBurstAlert( true );
	    burstalertactive_ = true;
	}

	extender_->preallocExtArea();
	extender_->setUndo( !horizon3dundoinfo_ );
	adjuster_->setUndo( !horizon3dundoinfo_ );
    }

    extender_->reset();
    extender_->setDirection( BinIDValue(BinID(0,0),mUdf(float)) );
    extender_->setStartPositions(currentseeds_);
    int res;
    while ( (res=extender_->nextStep())>0 )
	;

    TypeSet<EM::SubID> addedpos = extender_->getAddedPositions();
    TypeSet<EM::SubID> addedpossrc = extender_->getAddedPositionsSource();

    //Remove nodes that have failed 8 times before
    AutoTrackerRemoveBlackListed blacklistshrubber( emobject_, sectionid_,
	    addedpos, addedpossrc, blacklist_ );
    blacklistshrubber.execute();

    adjuster_->reset();
    adjuster_->setPositions(addedpos, &addedpossrc);
    while ( (res=adjuster_->nextStep())>0 )
	;

    //Not needed anymore, so we avoid hazzles below if we simply empty it
    addedpossrc.erase();

    //Add positions that have failed to blacklist
    for ( int idx=addedpos.size()-1; idx>=0; idx-- )
    {
	const EM::PosID pid( emobject_.id(), sectionid_, addedpos[idx] );
	if ( !emobject_.isDefined(pid) )
	{
	    char count = 0;
	    blacklist_.get( addedpos[idx], count );
	    blacklist_.set( addedpos[idx], count+1 );
	    addedpos.removeSingle(idx);
	}
    }

    //Some positions failed in the optimization, wich may lead to that
    //others are unsupported. Remove all unsupported nodes.
    sectiontracker_->removeUnSupported( addedpos );

    // Make all new nodes seeds
    currentseeds_ = addedpos;
    nrdone_ += currentseeds_.size();

    int status =  currentseeds_.size() ? MoreToDo() : Finished();
    if ( status == Finished() )
    {
	mDynamicCastGet(HorizonAdjuster*,horadj,sectiontracker_->adjuster());
	if ( !horadj )
	{
	    stepcntallowedvar_ = -1;
	    stepcntapmtthesld_ = -1;
	    return status;
	}

	if ( horadj->useAbsThreshold() )
	{
	    if ( stepcntapmtthesld_ == -1 )
		return status;

	    if ( horadj->getAmplitudeThresholds().size() <=
		 (stepcntapmtthesld_+1) )
	    {
		stepcntapmtthesld_ = -1;
		return status;
	    }

	    stepcntapmtthesld_++;

	    TypeSet<EM::SubID> seedsfromlaststep;
	    if ( sectiontracker_->propagatingFromSeedOnly() )
		seedsfromlaststep = currentseeds_;

	    reCalculateTotalNr();

	    if ( sectiontracker_->propagatingFromSeedOnly() &&
		 currentseeds_.isEmpty() )
		currentseeds_ = seedsfromlaststep;

	    const float th =
		horadj->getAmplitudeThresholds()[stepcntapmtthesld_];
	    horadj->setAmplitudeThreshold( th );
	    execmsg_ = tr("Step: %1").arg(th);
	    nrdone_ = 1;
	}
	else
	{
	    if ( stepcntallowedvar_ == -1 )
		return status;

	    if ( horadj->getAllowedVariances().size() <= (stepcntallowedvar_+1))
	    {
		stepcntallowedvar_ = -1;
		return status;
	    }

	    stepcntallowedvar_++;

	    TypeSet<EM::SubID> seedsfromlaststep;
	    if ( sectiontracker_->propagatingFromSeedOnly() )
		seedsfromlaststep = currentseeds_;

	    reCalculateTotalNr();

	    if ( sectiontracker_->propagatingFromSeedOnly() &&
		 currentseeds_.isEmpty() )
		currentseeds_ = seedsfromlaststep;

	    const float var = horadj->getAllowedVariances()[stepcntallowedvar_];
	    horadj->setAllowedVariance( var );
	    execmsg_ = tr("Step: %1%").arg(var * 100);
	    nrdone_ = 1;
	}

	blacklist_.erase();
	return MoreToDo();
    }

    return status;
}


void AutoTracker::setTrackBoundary( const TrcKeyZSampling& cs )
{ extender_->setExtBoundary( cs ); }


void AutoTracker::unsetTrackBoundary()
{ extender_->unsetExtBoundary(); }


bool AutoTracker::addSeed( const EM::PosID& pid )
{
    if ( pid.sectionID()!=sectionid_ || !emobject_.isAtEdge(pid) )
	return false;

    const Coord3& pos = emobject_.getPos(pid);
    if ( !engine().activeVolume().zsamp_.includes(pos.z,false) )
	return false;

    const BinID bid = SI().transform(pos);
    if ( !engine().activeVolume().hsamp_.includes(bid) )
	return false;

    currentseeds_ += pid.subID();
    return true;
}


uiString AutoTracker::uiMessage() const
{ return execmsg_; }


} // namespace MPE
