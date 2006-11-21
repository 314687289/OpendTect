/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 14-6-1996
-*/

static const char* rcsID = "$Id: executor.cc,v 1.24 2006-11-21 14:00:06 cvsbert Exp $";

#include "executor.h"

#include "errh.h"
#include "oddirs.h"
#include "progressmeter.h"
#include "timefun.h"
#include <iostream>

const int Executor::ErrorOccurred	= -1;
const int Executor::Finished		= 0;
const int Executor::MoreToDo		= 1;
const int Executor::WarningAvailable	= 2;


bool Executor::execute( std::ostream* strm, bool isfirst, bool islast,
		        int delaybetwnsteps )
{
    if ( !strm )
    {
	int rv = MoreToDo;
	while ( rv )
	{
	    rv = doStep();
	    if ( rv < 0 )
	    {
		const char* msg = message();
		if ( msg && *msg ) ErrMsg( msg );
		return false;
	    }
	    if ( delaybetwnsteps )
		Time_sleep( delaybetwnsteps*0.001 );
	}
	return true;
    }

    std::ostream& stream = *strm;
    if ( isfirst )
	stream << GetProjectVersionName() << "\n\n";

    stream << "Process: '" << name() << "'\n";
    stream << "Started: " << Time_getFullDateString() << "\n\n";

    BufferString curmsg, prevmsg;
    prevmsg = message();

    if ( strm )
	stream << '\t' << prevmsg << '\n';

    ProgressMeter progressmeter( stream );
    bool go_on = true;
    bool newmsg = true;
    bool needendl = false;
    int nrdone = 0;
    int rv;
    while ( go_on )
    {
	rv = doStep();
	curmsg = message();
	int newnrdone = nrDone();
	go_on = false;
	switch( rv )
	{
	case ErrorOccurred:
	    stream << "Error: " << curmsg << std::endl;
	break;
	case Finished:
	    stream << "\nFinished: " << Time_getFullDateString() << std::endl;
	break;
	default:
	    go_on = true;
	    if ( curmsg != prevmsg )
	    {
		if ( needendl ) stream << std::endl;
		needendl = false;
		stream << '\t' << curmsg << std::endl;
		newmsg = true;
	    }
	    else if ( newmsg && newnrdone )
	    {
		newmsg = false;
		if ( !nrdone )
		    stream << nrDoneText() << ":\n";
		progressmeter.reset();
		needendl = true;
	    }
	    else if ( newnrdone && newnrdone != nrdone )
	    {
		++progressmeter;
	    }
	break;
	}
	nrdone = newnrdone;
	prevmsg = curmsg;
    }

    progressmeter.finish();
    if ( islast )
	stream << "\n\nEnd of process: '" << name() << "'" << std::endl;
    return rv < 0 ? false : true;
}


int Executor::doStep()
{
    prestep.trigger();
    int res = nextStep();
    if ( res > 0 )
	poststep.trigger();
    return res;
}


ExecutorGroup::ExecutorGroup( const char* nm, bool p )
	: Executor( nm )
	, executors_( *new ObjectSet<Executor> )
	, currentexec_( 0 )
    	, parallel_( p )
    	, sumstart_( 0 )
{
}


ExecutorGroup::~ExecutorGroup()
{
    deepErase( executors_ );
    delete &executors_;
}


void ExecutorGroup::findNextSumStop()
{
    if ( !parallel_ )
    {
	for ( int idx=currentexec_+1; idx<executors_.size(); idx++ )
	{
	    if ( strcmp(executors_[idx]->nrDoneText(),
			executors_[idx-1]->nrDoneText())
	      || strcmp(executors_[idx]->message(),
			executors_[idx-1]->message()) )
	    {
		sumstop_ = idx-1;
		return;
	    }
	}
    }

    sumstop_ = executors_.size()-1;
}


void ExecutorGroup::add( Executor* n )
{
    executors_ += n;
    executorres_ += 1;

    findNextSumStop();
}


int ExecutorGroup::nextStep()
{
    const int nrexecs = executors_.size();
    if ( !nrexecs ) return Finished;

    int res = executorres_[currentexec_] = executors_[currentexec_]->doStep();
    if ( res == ErrorOccurred )
	return ErrorOccurred;
    else if ( parallel_ || res==Finished )
	res = goToNextExecutor() ? MoreToDo : Finished;

    return res;
}

bool ExecutorGroup::goToNextExecutor()
{
    const int nrexecs = executors_.size();
    if ( !nrexecs ) return false;

    for ( int idx=0; idx<nrexecs; idx++ )
    {
	currentexec_++;
	if ( currentexec_==nrexecs )
	    currentexec_ = 0;

	if ( executorres_[currentexec_]>Finished )
	{
	    if ( currentexec_>sumstop_ )
	    {
		sumstart_ = currentexec_;
		findNextSumStop();
	    }

	    return true;
	}
    }

    return false;
}


const char* ExecutorGroup::message() const
{
    return executors_.size() ? executors_[currentexec_]->message()
			    : Executor::message();
}


int ExecutorGroup::totalNr() const
{
    const int nrexecs = executors_.size();
    if ( !nrexecs ) return -1;

    int res = 0;
    for ( int idx=sumstart_; idx<=sumstop_; idx++ )
    {
	const int nr = executors_[idx]->totalNr();
	if ( nr<0 )
	    return -1;

	res += nr;
    }

    return res;
}


int ExecutorGroup::nrDone() const
{
    const int nrexecs = executors_.size();
    if ( nrexecs < 1 )
	return -1;

    int res = 0;
    for ( int idx=sumstart_; idx<=sumstop_; idx++ )
	res += executors_[idx]->nrDone();

    return res;
}


const char* ExecutorGroup::nrDoneText() const
{
    const char* txt = (const char*)nrdonetext_;
    if ( *txt ) return txt;

    if ( executors_.isEmpty() )
	return Executor::nrDoneText();

    return executors_[currentexec_]->nrDoneText();
}
