/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R. K. Singh
 * DATE     : Oct 2007
-*/

static const char* rcsID = "$Id: seispsmerge.cc,v 1.10 2008-12-23 11:10:34 cvsdgb Exp $";

#include "seispsmerge.h"
#include "seisselection.h"
#include "posinfo.h"
#include "seisbuf.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "seispswrite.h"
#include "seistrc.h"
#include "ioobj.h"


SeisPSMerger::SeisPSMerger( const ObjectSet<IOObj>& inobjs, const IOObj& out,
       			    const Seis::SelData* sd )
  	: Executor("Merging Pre-Stack data")
	, writer_(0)
	, sd_(sd && !sd->isAll() ? sd->clone() : 0)
	, msg_("Handling gathers")
	, totnr_(-1)
	, nrdone_(0)
{
    HorSampling hs;
    for ( int idx=0; idx<inobjs.size(); idx++ )
    {
	SeisPS3DReader* rdr = SPSIOPF().get3DReader( *inobjs[idx] );
	if ( !rdr ) continue;

	Interval<int> inlrg, crlrg; StepInterval<int> rg;
	rdr->posData().getInlRange( rg ); assign( inlrg, rg );
	rdr->posData().getCrlRange( rg ); assign( crlrg, rg );
	if ( idx == 0 )
	    hs.set( inlrg, crlrg );
	else
	{
    	    hs.include( BinID(inlrg.start,crlrg.start) );
    	    hs.include( BinID(inlrg.stop,crlrg.stop) );
	}

	readers_ += rdr;
    }
    if ( readers_.isEmpty() )
	{ msg_ = "No valid inputs specified"; return; }

    totnr_ = sd_ ? sd_->expectedNrTraces() : hs.totalNr();
    iter_ = new HorSamplingIterator( hs );

    PtrMan<IOObj> outobj = out.clone();
    writer_ = SPSIOPF().get3DWriter( *outobj );
    if ( !writer_ )
	{ deepErase(readers_); msg_ = "Cannot create output writer"; return; }

    SPSIOPF().mk3DPostStackProxy( *outobj );
}


SeisPSMerger::~SeisPSMerger()
{
    delete iter_;
    delete writer_;
    delete sd_;
    deepErase( readers_ );
}


int SeisPSMerger::nextStep()
{
    if ( readers_.isEmpty() )
	return Executor::ErrorOccurred();

    SeisTrcBuf trcbuf( true );
    while ( true )
    {
	if ( !iter_->next(curbid_) )
	    return Executor::Finished();
	if ( sd_ && !sd_->isOK(curbid_) )
	    continue;

	for ( int idx=0; idx<readers_.size(); idx++ )
	{
	    if ( readers_[idx]->getGather(curbid_,trcbuf) )
	    {
		for ( int tdx=0; tdx<trcbuf.size(); tdx++ )
		{
		    if ( !writer_->put(*trcbuf.get(tdx)) )
			return Executor::ErrorOccurred();
		}
		nrdone_ ++;
		return Executor::MoreToDo();
	    }
	}
    }
}
