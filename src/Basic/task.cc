/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2005
-*/

static const char* rcsID = "$Id: task.cc,v 1.18 2009-05-01 19:24:34 cvskris Exp $";

#include "task.h"

#include "threadwork.h"
#include "thread.h"
#include "varlenarray.h"
#include "progressmeter.h"


Task::Task( const char* nm )
    : NamedObject( nm )
    , workcontrolcondvar_( 0 )
    , control_( Task::Run )
{}


void Task::enableWorkControl( bool yn )
{
    const bool isenabled = workcontrolcondvar_;
    if ( isenabled==yn )
	return;

    if ( yn )
	workcontrolcondvar_ = new Threads::ConditionVar;
    else
    {
	delete workcontrolcondvar_;
	workcontrolcondvar_ = 0;
    }
}


void Task::controlWork( Task::Control c )
{
    if ( !workcontrolcondvar_ )
	return;

    workcontrolcondvar_->lock();
    if ( control_!=c )
    {
	control_ = c;
	workcontrolcondvar_->signal(true);
    }

    workcontrolcondvar_->unLock();
}


Task::Control Task::getState() const
{
    if ( !workcontrolcondvar_ )
	return Task::Run;

    workcontrolcondvar_->lock();
    Task::Control res = control_;
    workcontrolcondvar_->unLock();

    return res;
}


bool Task::shouldContinue()
{
    if ( !workcontrolcondvar_ )
	return true;

    workcontrolcondvar_->lock();
    if ( control_==Task::Run )
    {
	workcontrolcondvar_->unLock();
	return true;
    }
    else if ( control_==Task::Stop )
    {
	workcontrolcondvar_->unLock();
	return false;
    }

    while ( control_==Task::Pause )
	workcontrolcondvar_->wait();

    const bool shouldcont = control_==Task::Run;

    workcontrolcondvar_->unLock();
    return shouldcont;
}


void SequentialTask::setProgressMeter( ProgressMeter* pm )
{
    progressmeter_ = pm;
}


int SequentialTask::doStep()
{
    if ( progressmeter_ ) progressmeter_->setStarted();

    const int res = nextStep();
    if ( progressmeter_ )
    {
	progressmeter_->setNrDone( nrDone() );
	progressmeter_->setTotalNr( totalNr() );
	progressmeter_->setNrDoneText( nrDoneText() );
	progressmeter_->setMessage( message() );
	
	if ( res<1 )
	    progressmeter_->setFinished();
    }

    return res;
}



bool SequentialTask::execute()
{
    control_ = Task::Run;

    do
    {
	int res = doStep();
	if ( !res )     return true;
	if ( res < 0 )  break;
    } while ( shouldContinue() );

    return false;
}


class ParallelTaskRunner : public SequentialTask
{
public:
    		ParallelTaskRunner()
		    : task_( 0 )					{}

    void	set( ParallelTask* task, od_int64 start, od_int64 stop,
	    	     int threadid )
    		{
		    task_ = task;
		    start_ = start;
		    stop_ = stop;
		    threadid_ = threadid;;
		}

    int		nextStep()
		{
		    if ( !task_->doWork(start_,stop_,threadid_) )
		    {
			task_->controlWork( Task::Stop );
			return SequentialTask::ErrorOccurred();
		    }

		    return SequentialTask::Finished();
		}

protected:
    od_int64		start_;
    od_int64		stop_;
    int			threadid_;
    ParallelTask*	task_;
};


ParallelTask::ParallelTask( const char* nm )
    : Task( nm )
    , progressmeter_( 0 )
    , nrdonemutex_( 0 )
    , nrdone_( -1 )
    , totalnrcache_( -1 )
{ }


ParallelTask::~ParallelTask()
{ delete nrdonemutex_; }


void ParallelTask::enableNrDoneCounting( bool yn )
{
    const bool isenabled = nrdonemutex_;
    if ( isenabled==yn )
	return;

    if ( !yn )
    {
	nrdonemutex_->lock();
	Threads::Mutex* tmp = nrdonemutex_;
	nrdonemutex_ = 0;
	tmp->unLock();
    }
    else
    {
	nrdonemutex_ = new Threads::Mutex;
    }
}


void ParallelTask::setProgressMeter( ProgressMeter* pm )
{
    progressmeter_ = pm;
    if ( progressmeter_ )
    {
	progressmeter_->setName( name() );
	progressmeter_->setMessage( message() );
	enableNrDoneCounting( true );
    }

}


void ParallelTask::reportNrDone( int nr )
{ addToNrDone( nr ); }


void ParallelTask::addToNrDone( int nr )
{
    if ( nrdonemutex_ )
    {
	Threads::MutexLocker lock( *nrdonemutex_ );
	if ( nrdone_==-1 )
	    nrdone_ = nr;
	else
	    nrdone_ += nr;
    }

    if ( progressmeter_ )
    {
	progressmeter_->setTotalNr( totalnrcache_ );
	progressmeter_->setNrDoneText( nrDoneText() );
	progressmeter_->setMessage( message() );
	progressmeter_->setNrDone( nrDone() );
    }

}


od_int64 ParallelTask::nrDone() const
{
    if ( nrdonemutex_ )
    {
	 nrdonemutex_->lock();
	 const od_int64 res = nrdone_;
	 nrdonemutex_->unLock();
	 return res;
    }

    if ( !progressmeter_ )
	return -1;

    return progressmeter_->nrDone();
}


Threads::ThreadWorkManager& ParallelTask::twm()
{
    static Threads::ThreadWorkManager twm_( Threads::getNrProcessors()*2 );
    return twm_;
}


bool ParallelTask::execute( bool parallel )
{
    totalnrcache_ = nrIterations();
    if ( totalnrcache_<=0 )
	return true;

    if ( progressmeter_ )
    {
	progressmeter_->setName( name() );
	progressmeter_->setMessage( message() );
	progressmeter_->setTotalNr( totalnrcache_ );
	progressmeter_->setStarted();
    }

    if ( nrdonemutex_ ) nrdonemutex_->lock();
    nrdone_ = -1;
    control_ = Task::Run;
    if ( nrdonemutex_ ) nrdonemutex_->unLock();

    const int minthreadsize = minThreadSize();
    const int maxnrthreads = parallel
	? mMIN( totalnrcache_/minthreadsize, maxNrThreads() )
	: 1;

    if ( Threads::getNrProcessors()==1 || maxnrthreads==1 )
    {
	if ( !doPrepare( 1 ) ) return false;
	bool res = doFinish( doWork( 0, totalnrcache_-1, 0 ) );
	if ( progressmeter_ ) progressmeter_->setFinished();
	return res;
    }

    //Don't take all threads, as we may want to have spare ones.
    const int nrthreads = mMIN(Threads::getNrProcessors(),maxnrthreads);
    const od_int64 size = totalnrcache_;
    if ( !size ) return true;

    mAllocVarLenArr( ParallelTaskRunner, runners, nrthreads );

    od_int64 start = 0;
    ObjectSet<SequentialTask> tasks;
    for ( int idx=nrthreads-1; idx>=0; idx-- )
    {
	if ( start>=size )
	    break;

	const od_int64 threadsize = calculateThreadSize( size, nrthreads, idx );
	if ( threadsize==0 )
	    continue;

	const int stop = start + threadsize-1;
	runners[idx].set( this, start, stop, idx );
	tasks += &runners[idx];
	start = stop+1;
    }

    if ( !doPrepare( tasks.size() ) )
	return false;

    bool res;
    if ( tasks.size()<2 )
	res = doWork( 0, totalnrcache_-1, 0 );
    else
    {
	if ( stopAllOnFailure() )
	    enableWorkControl( true );

	res = twm().addWork( tasks );
    }

    res = doFinish( res );
    if ( progressmeter_ ) progressmeter_->setFinished();
    return res;
}


od_int64 ParallelTask::calculateThreadSize( od_int64 totalnr, int nrthreads,
				            int idx ) const
{
    if ( nrthreads==1 ) return totalnr;

    const od_int64 idealnrperthread = mNINT((float) totalnr/nrthreads);
    const od_int64 nrperthread = idealnrperthread<minThreadSize()
	?  minThreadSize()
	: idealnrperthread;

    const od_int64 start = idx*nrperthread;
    od_int64 nextstart = start + nrperthread;

    if ( nextstart>totalnr )
	nextstart = totalnr;

    if ( idx==nrthreads-1 )
	nextstart = totalnr;

    if ( nextstart<=start )
	return 0;

    return nextstart-start;
}

