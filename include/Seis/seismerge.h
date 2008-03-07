#ifndef seismerge_h
#define seismerge_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Mar 2008
 RCS:		$Id: seismerge.h,v 1.2 2008-03-07 10:47:34 cvsbert Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "position.h"
#include "samplingdata.h"
class IOPar;
class SeisTrc;
class SeisTrcBuf;
class SeisTrcReader;
class SeisTrcWriter;


/*!\brief Merges 2D and 3D post-stack data */

class SeisMerger : public Executor
{
public:

    			SeisMerger(const ObjectSet<IOPar>& in,const IOPar& out,
				  bool is2d);
    virtual		~SeisMerger();

    const char*		message() const;
    int			nrDone() const		{ return nrpos_; }
    int			totalNr() const		{ return totnrpos_; }
    const char*		nrDoneText() const	{ return "Positions handled"; }
    int			nextStep();

    bool		stacktrcs_; //!< If not, first trace will be used

protected:

    bool			is2d_;
    ObjectSet<SeisTrcReader>	rdrs_;
    SeisTrcWriter*		wrr_;
    int				currdridx_;
    int				nrpos_;
    int				totnrpos_;
    BufferString		errmsg_;

    BinID			curbid_;
    SeisTrcBuf&			trcbuf_;
    int				nrsamps_;
    SamplingData<float>		sd_;

    SeisTrc*			getNewTrc();
    SeisTrc*			getTrcFrom(SeisTrcReader&);
    void			get3DTraces();
    SeisTrc*			getStacked(SeisTrcBuf&);
    bool			toNextPos();
    int				writeTrc(SeisTrc*);
    int				writeFromBuf();

};


#endif
