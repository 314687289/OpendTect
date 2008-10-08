#ifndef seisscanner_h
#define seisscanner_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Feb 2004
 RCS:		$Id: seisscanner.h,v 1.13 2008-10-08 15:57:32 cvsbert Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "samplingdata.h"
#include "executor.h"
#include "seistype.h"
class IOPar;
class IOObj;
class SeisTrc;
class CubeSampling;
class SeisTrcReader;
class PosGeomDetector;
namespace PosInfo { class Detector; }

#define mSeisScanMaxNrDistribVals 50000


class SeisScanner : public Executor
{
public:

    			SeisScanner(const IOObj&,Seis::GeomType,
				    int max_nr_trcs=-1);
			~SeisScanner();

    const char*		message() const;
    od_int64		totalNr() const;
    od_int64		nrDone() const;
    const char*		nrDoneText() const;
    int			nextStep();

    void		report(IOPar&) const;
    bool		getSurvInfo(CubeSampling&,Coord crd[3]) const;
    			//!< Z range will be exclusive start/end null samples

    const float*	distribVals() const	{ return distribvals_; }
    int			nrDistribVals() const	{ return nrdistribvals_; }
    Interval<float>	zRange() const		{ return Interval<float>
			( sampling_.start, sampling_.atIndex(nrsamples_-1) ); }
    Interval<float>	valRange() const	{ return valrg_; }
    unsigned int	nrNullTraces() const	{ return nrnulltraces_; }

    void		launchBrowser(const IOPar&,const char* fnm) const;
    			//!< Passed IOPar will be the start of the display

protected:

    SeisTrc&		trc_;
    SeisTrcReader&	rdr_;
    mutable BufferString curmsg_;
    Seis::GeomType	geom_;
    int			maxnrtrcs_;
    int			totalnr_;
    PosInfo::Detector&	dtctor_;

    SamplingData<float>	sampling_;
    int			nrsamples_;
    int			nrnulltraces_;
    int			nrdistribvals_;
    float		distribvals_[mSeisScanMaxNrDistribVals];
    Interval<int>	nonnullsamplerg_;
    Interval<float>	valrg_;
    BinID		invalidsamplebid_;
    int			invalidsamplenr_;

    void		wrapUp();
    bool		doValueWork();
    bool		addTrc();

    const char*		getClipRgStr(float) const;
};


#endif
