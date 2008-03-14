
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : Mar 2007
-*/

static const char* rcsID = "$Id: tutseistools.cc,v 1.8 2008-03-14 09:15:35 cvsnageswara Exp $";

#include "cubesampling.h"
#include "tutseistools.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seisioobjinfo.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "ioobj.h"


Tut::SeisTools::SeisTools()
    : Executor("Tutorial tools: Direct Seismic")
    , inioobj_(0), outioobj_(0)
    , rdr_(0), wrr_(0), cs_(*new CubeSampling)
    , trcin_(*new SeisTrc)
    , trcout_(*new SeisTrc)
{
    clear();
}


Tut::SeisTools::~SeisTools()
{
    clear();
    delete &trcin_;
    delete &trcout_;
}


void Tut::SeisTools::clear()
{
    delete inioobj_; inioobj_ = 0;
    delete outioobj_; outioobj_ = 0;
    delete rdr_; rdr_ = 0;
    delete wrr_; wrr_ = 0;

    action_ = Scale;
    factor_ = 1; shift_ = 0;
    weaksmooth_ = false;
    totnr_ = -1; nrdone_ = 0;
}


void Tut::SeisTools::setInput( const IOObj& ioobj )
{ delete inioobj_; inioobj_ = ioobj.clone(); }

void Tut::SeisTools::setOutput( const IOObj& ioobj )
{ delete outioobj_; outioobj_ = ioobj.clone(); }

void Tut::SeisTools::setRange( const CubeSampling& cs )
{ cs_ = cs; }


const char* Tut::SeisTools::message() const
{
    static const char* acts[] = { "Scaling", "Squaring", "Smoothing" };
    return errmsg_.isEmpty() ? acts[action_] : errmsg_.buf();
}


int Tut::SeisTools::totalNr() const
{
    if ( inioobj_ && totnr_ == -1 )
    {
	totnr_ = -2;
	SeisIOObjInfo ioobjinfo( *inioobj_ );
	SeisIOObjInfo::SpaceInfo spinf;
	if ( ioobjinfo.getDefSpaceInfo(spinf) )
	    totnr_ = spinf.expectednrtrcs;
    }
	
    return totnr_ < 0 ? -1 : totnr_;
}


bool Tut::SeisTools::createReader()
{
    rdr_ = new SeisTrcReader( inioobj_ );
    Seis::RangeSelData* sd = new Seis::RangeSelData( cs_ );
    rdr_->setSelData( sd );

    rdr_->forceFloatData( action_ != Smooth );
    if ( !rdr_->prepareWork() )
    {
	errmsg_ = rdr_->errMsg();
	return false;
    }
    return true;
}


bool Tut::SeisTools::createWriter()
{
    wrr_ = new SeisTrcWriter( outioobj_ );
    if ( !wrr_->prepareWork(trcout_) )
    {
	errmsg_ = wrr_->errMsg();
	return false;
    }
    return true;
}


int Tut::SeisTools::nextStep()
{
    if ( !rdr_ )
	return createReader() ? Executor::MoreToDo : Executor::ErrorOccurred;

    int rv = rdr_->get( trcin_.info() );
    if ( rv < 0 )
	{ errmsg_ = rdr_->errMsg(); return Executor::ErrorOccurred; }
    else if ( rv == 0 )
	return Executor::Finished;
    else if ( rv == 1 )
    {
	if ( !rdr_->get(trcin_) )
	    { errmsg_ = rdr_->errMsg(); return Executor::ErrorOccurred; }

	trcout_ = trcin_;
	handleTrace();

	if ( !wrr_ && !createWriter() )
	    return Executor::ErrorOccurred;
	if ( !wrr_->put(trcout_) )
	    { errmsg_ = wrr_->errMsg(); return Executor::ErrorOccurred; }
    }

    return Executor::MoreToDo;
}


void Tut::SeisTools::handleTrace()
{
    switch ( action_ )
    {

    case Scale: {
	SeisTrcPropChg stpc( trcout_ );
	stpc.scale( factor_, shift_ );
    } break;

    case Square: {
	for ( int icomp=0; icomp<trcin_.nrComponents(); icomp++ )
	{
	    for ( int idx=0; idx<trcin_.size(); idx++ )
	    {
		const float v = trcin_.get( idx, icomp );
		trcout_.set( idx, v*v, icomp );
	    }
	}
    } break;

    case Smooth: {
	const int sgate = weaksmooth_ ? 3 : 5;
	const int sgate2 = sgate/2; 
	for ( int icomp=0; icomp<trcin_.nrComponents(); icomp++ )
	{
	    for ( int idx=0; idx<trcin_.size(); idx++ )
	    {
	        float sum = 0;
		int count = 0;
		for( int ismp=idx-sgate2; ismp<=idx+sgate2; ismp++)
		{
		    const float val = trcin_.get( ismp, icomp );
		    if ( !mIsUdf(val) )
		    {
			sum += val;
			count++;
		    }
		}
		if ( count )
		    trcout_.set( idx, sum/count, icomp );
	    }
	}

    } break;

    }

    nrdone_++;
}
