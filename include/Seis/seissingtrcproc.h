#ifndef seissingtrcproc_h
#define seissingtrcproc_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Oct 2001
 RCS:		$Id: seissingtrcproc.h,v 1.12 2004-09-20 16:17:37 bert Exp $
________________________________________________________________________

-*/

#include <executor.h>
class IOObj;
class IOPar;
class Scaler;
class SeisTrc;
class MultiID;
class SeisSelection;
class SeisTrcReader;
class SeisTrcWriter;
class SeisResampler;


/*!\brief Single trace processing executor

When a trace info is read, the selection callback is triggered. You can then
use skipCurTrc(). When the trace is read, the processing callback is triggered.
You can set you own trace as output trace, otherwise the input trace will be
taken.

*/

class SeisSingleTraceProc : public Executor
{
public:

			SeisSingleTraceProc(const SeisSelection&,
					    const IOObj* out,
					    const char* nm="Trace processor",
					    const char* msg="Processing");
			SeisSingleTraceProc(const IOObj* in,const IOObj* out,
					    const char* nm="Trace processor",
					    const IOPar* iniopar=0,
					    const char* msg="Processing");
			SeisSingleTraceProc(ObjectSet<IOObj>,const IOObj*,
					    const char* nm="Trace processor",
					    ObjectSet<IOPar>* iniopars=0,
					    const char* msg="Processing");
    virtual		~SeisSingleTraceProc();

    void		setSelectionCB( const CallBack& cb ) { selcb_ = cb; }
    void		setProcessingCB( const CallBack& cb ) { proccb_ = cb; }
    void		skipCurTrc()		{ skipcurtrc_ = true; }
    			//!< will also be checked after processing CB

    const SeisTrcReader* reader(int idx=0) const { return rdrset_[idx]; }
    const SeisTrcWriter* writer() const		 { return wrr_; }
    SeisTrc&		getTrace()		 { return *worktrc_; }

    void		setTracesPerStep( int n ) { trcsperstep_ = n; }
    			//!< default is 10

    virtual const char*	message() const;
    virtual const char*	nrDoneText() const;
    virtual int		nrDone() const;
    virtual int		totalNr() const;
    virtual int		nextStep();

    int			nrSkipped() const	{ return nrskipped_; }
    int			nrWritten() const	{ return nrwr_; }
    void		setTotalNrIfUnknown( int nr )
			{ if ( totnr_ < 0 ) totnr_ = nr; }
    void		setScaler(Scaler*);
    			//!< Scaler becomes mine.
    void		setResampler(SeisResampler*);
    void		skipNullTraces( bool yn=true )	{ skipnull_ = yn; }

    void		setInput(const IOObj*,const IOObj*,const char*,
				 const IOPar*,const char*);

protected:

    ObjectSet<SeisTrcReader> rdrset_;
    SeisTrcWriter*	wrr_;
    SeisTrc&		intrc_;
    SeisTrc*		worktrc_;
    SeisResampler*	resampler_;
    CallBack		selcb_;
    CallBack		proccb_;
    BufferString	msg_;
    BufferString	curmsg_;
    bool		skipcurtrc_;
    int			nrwr_;
    int			nrskipped_;
    int			totnr_;
    MultiID&		wrrkey_;
    int			trcsperstep_;
    int			currentobj_;
    int			nrobjs_;
    Scaler*		scaler_;
    bool		skipnull_;
    bool		is3d_;

    virtual void	wrapUp();
    void		nextObj();
    bool		init(ObjectSet<IOObj>&,ObjectSet<IOPar>&);
};


#endif
