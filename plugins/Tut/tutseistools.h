
#ifndef tutseistools_h
#define tutseistools_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : Mar 2007
 * ID       : $Id: tutseistools.h,v 1.3 2007-05-21 11:39:18 cvsbert Exp $
-*/

#include "executor.h"
#include "bufstring.h"
class IOObj;
class SeisTrc;
class SeisTrcReader;
class SeisTrcWriter;


namespace Tut
{

class SeisTools : public Executor
{
public:

    enum Action		{ Scale, Square, Smooth };

    			SeisTools();
    virtual		~SeisTools();
    void		clear();

    const IOObj*	input() const		{ return inioobj_; }
    const IOObj*	output() const		{ return outioobj_; }
    inline Action	action() const		{ return action_; }
    inline float	factor() const		{ return factor_; }
    inline float	shift() const		{ return shift_; }
    inline bool		weakSmoothing() const	{ return weaksmooth_; }

    void		setInput(const IOObj&);
    void		setOutput(const IOObj&);
    inline void		setAction( Action a )	{ action_ = a; }
    inline void		setScale( float f, float s )
						{ factor_ = f; shift_ = s; }
    inline void		setWeakSmoothing( bool yn )
    						{ weaksmooth_ = yn; }

			// Executor compliance functions
    const char*		message() const;
    int			nrDone() const		{ return nrdone_; }
    int			totalNr() const;
    const char*		nrDoneText() const	{ return "Traces handled"; }
			// This is where it actually happens
    int			nextStep();

protected:

    IOObj*		inioobj_;
    IOObj*		outioobj_;
    Action		action_;
    float		factor_;
    float		shift_;
    bool		weaksmooth_;

    SeisTrcReader*	rdr_;
    SeisTrcWriter*	wrr_;
    SeisTrc&		trc_;
    int			nrdone_;
    mutable int		totnr_;
    BufferString	errmsg_;

    bool		createReader();
    bool		createWriter();
    void		handleTrace();

};

} // namespace

#endif
