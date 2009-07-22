#ifndef seispsmerge_h
#define seispsmerge_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	R. K. Singh
 Date:		Oct 2007
 RCS:		$Id: seispsmerge.h,v 1.7 2009-07-22 16:01:18 cvsbert Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "cubesampling.h"
class IOObj;
class SeisPSReader;
class SeisPSWriter;
namespace Seis { class SelData; }


/*!\brief Pre-Stack seismic data merger

  The order in which the stores are in the ObjectSet is important: the first
  data store at a position will be used.

*/

mClass SeisPSMerger : public Executor
{
public:
			SeisPSMerger(const ObjectSet<IOObj>& in,
				     const IOObj& out,
				     const Seis::SelData* sd=0);
    virtual		~SeisPSMerger();

    virtual const char*	message() const		{ return msg_.buf(); }
    virtual const char*	nrDoneText() const	{ return "Gathers written"; }
    virtual od_int64	nrDone() const		{ return nrdone_; }
    virtual od_int64	totalNr() const		{ return totnr_; }
    virtual int		nextStep();

protected:

    BinID		curbid_;
    Seis::SelData*	sd_;

    HorSamplingIterator*	iter_;
    ObjectSet<SeisPSReader>	readers_;
    SeisPSWriter*		writer_;

    BufferString	msg_;
    int			totnr_;
    int			nrdone_;

    void		init(const HorSampling&);
};


#endif
