/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "progressmeter.h"
#include "timefun.h"
#include "od_ostream.h"
#include <limits.h>
#include <stdlib.h>
#include "task.h"


static const char dispchars_[] = ".:=|*#>}ABCDEFGHIJKLMNOPQRSTUVWXYZ";


TextStreamProgressMeter::TextStreamProgressMeter( od_ostream& out,
						  unsigned short rowlen )
    : strm_(out)
    , rowlen_(rowlen)
    , finished_(true)
    , totalnr_(0)
{ reset(); }


TextStreamProgressMeter::~TextStreamProgressMeter()
{
    if ( !finished_ ) setFinished();
}

/*
void TextStreamProgressMeter::setTask( const Task& task )
{
    strm_ <<  "Process: '" << task.name() << "'\n";
    strm_ << "Started: " << Time::getDateTimeString() << "\n\n";
    strm_ << '\t' << task.message() << '\n';
    reset();
}

*/


void TextStreamProgressMeter::setFinished()
{
    Threads::Locker lock( lock_ );
    if ( finished_ )
	return;

    annotate(false);
    finished_ = true;

    strm_ << "\nFinished: "  << Time::getDateTimeString() << od_endl;
    lock.unlockNow();
    reset();
}


void TextStreamProgressMeter::reset()
{
    Threads::Locker lock( lock_ );
    nrdone_ = 0;
    oldtime_ = Time::getMilliSeconds();
    inited_ = false;
    nrdoneperchar_ = 1; distcharidx_ = 0;
    lastannotatednrdone_ = 0;
    nrdotsonline_ = 0;
}


void TextStreamProgressMeter::setStarted()
{
    if ( !inited_ )
    {
	if ( !name_.isEmpty() ) strm_ <<  "Process: '" << name_.buf() << "'\n";
	strm_ << "Started: " << Time::getDateTimeString() << "\n\n";
	if ( !message_.isEmpty() ) strm_ << '\t' << message_.buf() << '\n';
        oldtime_ = Time::getMilliSeconds();
	strm_.flush();
	finished_ = false;
	inited_ = true;
    }
}


void TextStreamProgressMeter::addProgress( int nr )
{
    if ( !inited_ )
	setStarted();

    for ( int idx=0; idx<nr; idx++ )
    {
	nrdone_ ++;
	od_uint64 relprogress = nrdone_ - lastannotatednrdone_;
	if ( !(relprogress % nrdoneperchar_) )
	{
	    strm_ << (relprogress%(10*nrdoneperchar_)
		    ? dispchars_[distcharidx_]
		    : dispchars_[distcharidx_+1]);
	    strm_.flush();
	    nrdotsonline_++;
	}

	if ( nrdotsonline_==rowlen_ )
	    annotate(true);
    }
}


void TextStreamProgressMeter::operator++()
{
    Threads::Locker lock( lock_ );
    addProgress( 1 );
}
	

void TextStreamProgressMeter::setNrDone( od_int64 nrdone )
{
    Threads::Locker lock( lock_ );
    if ( nrdone<=nrdone_ )
    	return;

    addProgress( (int)(nrdone-nrdone_) );
}


void TextStreamProgressMeter::setMessage( const char* message )
{
    if ( message_==message )
	return;

    message_ = message;
}


void TextStreamProgressMeter::setName( const char* newname )
{
    if ( name_==newname )
	return;

    name_ = newname;
    reset();
}


void TextStreamProgressMeter::annotate( bool withrate )
{
    // Show numbers
    strm_ << ' ';
    float percentage = 0;
    if ( totalnr_>0 )
    {
	percentage = ((float) nrdone_)/totalnr_;
	strm_ << mNINT32(percentage*100) << "%";
    }
    else
	strm_ << nrdone_;


    // Show rate
    int newtime = Time::getMilliSeconds();
    int tdiff = newtime - oldtime_;
    if ( withrate && tdiff > 0 )
    {
	od_int64 nrdone = nrdone_ - lastannotatednrdone_;
	od_int64 permsec = (od_int64)(1.e6 * nrdone / tdiff + .5);
	strm_ << " (" << permsec * .001 << "/s)";
    }
    if ( withrate && tdiff>0 && totalnr_>0 )
    {
	const float nrdone = (float) nrdone_ - lastannotatednrdone_;
	const float todo = (float) totalnr_ - nrdone_;
	od_int64 etasec = mNINT64(tdiff * (todo/nrdone) / 1000.f);
	BufferString eta;
	if ( etasec > 3600 )
	{
	    const int hours = (int) etasec/3600;
	    eta.add(hours).add("h:");
	    etasec = etasec%3600;
	}
	if ( etasec > 60 )
	{
	    const int mins = (int) etasec/60;
	    eta.add(mins).add("m:");
	    etasec = etasec%60;
	}

	eta.add(etasec).add("s");

	strm_ << " (" << eta << ")";
    }
    strm_ << od_endl;

    lastannotatednrdone_ = nrdone_;
    oldtime_ = newtime; 
    nrdotsonline_ = 0;
    
    // Adjust display speed if necessary
    if ( tdiff > -1 && tdiff < 5000 )
    {
	distcharidx_++;
	nrdoneperchar_ *= 10;
    }
    else if ( tdiff > 60000 )
    {
	if ( distcharidx_ )
	{
	    distcharidx_--;
	    nrdoneperchar_ /= 10;
	}
    }
}
