#ifndef seisscanner_h
#define seisscanner_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Feb 2004
 RCS:		$Id: seisscanner.h,v 1.6 2004-07-29 21:41:25 bert Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "samplingdata.h"
#include "executor.h"
class IOPar;
class IOObj;
class SeisTrc;
class CubeSampling;
class SeisTrcReader;
class PosGeomDetector;

#define mMaxNrDistribVals 50000


class SeisScanner : public Executor
{
public:

			SeisScanner(const IOObj&);
			~SeisScanner();

    const char*		message() const;
    int			totalNr() const;
    int			nrDone() const;
    const char*		nrDoneText() const;

    bool		getSurvInfo(CubeSampling&,Coord crd[3]) const;
    			//!< Z range will be exclusive start/end null samples
    void		report(IOPar&) const;

    const float*	distribVals() const	{ return distribvals; }
    int			nrDistribVals() const	{ return nrdistribvals; }
    Interval<float>	zRange() const
    			{ return Interval<float>( sampling.start,
					 sampling.atIndex(nrsamples-1) ); }
    			//!< the range found in the cube

    static const char*	defaultUserInfoFile(const char* translnm=0);
    void		launchBrowser(const char* fnm=0) const;
    			//!< If nulll or empty fnm uses default file name
    			//!< for the current translator

protected:

    int			nextStep();

    bool		first_trace;
    int			chnksz;
    mutable int		totalnr;
    int			expectedcrls;
    int			nrcrlsthisline;
    SeisTrc&		trc;
    SeisTrcReader&	reader;
    PosGeomDetector&	geomdtector;
    mutable BufferString curmsg;

    SamplingData<float>	sampling;
    unsigned int	nrsamples;
    unsigned int	nrnulltraces;
    unsigned int	nrdistribvals;
    float		distribvals[mMaxNrDistribVals];
    Interval<int>	nonnullsamplerg;
    Interval<float>	valrg;
    BinID		invalidsamplebid;
    int			invalidsamplenr;

    void		init();
    void		wrapUp();
    void		handleFirstTrc();
    void		handleTrc();
    void		handleBinIDChange();
    bool		doValueWork();

    const char*		getClipRgStr(float) const;

};


#endif
