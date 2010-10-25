#ifndef velocityvolumeconversion_h
#define velocityvolumeconversion_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: velocityvolumeconversion.h,v 1.7 2010-10-25 19:19:59 cvskris Exp $
________________________________________________________________________


-*/

#include "cubesampling.h"
#include "task.h"
#include "thread.h"
#include "veldesc.h"

class IOObj;
class SeisTrc;
class SeisTrcReader;
class SeisTrcWriter;
class SeisSequentialWriter;

namespace Vel
{

/*!Reads in a volume with eather Vrms or Vint, and writes out a volume
   with eather Vrms or Vint. */

mClass VolumeConverter : public ParallelTask
{
public:
			VolumeConverter( const IOObj& input,
					 const IOObj& output,
					 const HorSampling& ranges,
					 const VelocityDesc& outdesc );
			~VolumeConverter();
    const char*		errMsg() const { return errmsg_.str(); }

    static const char*	sKeyInput();
    static const char*	sKeyOutput();

protected:
    od_int64		nrIterations() const { return totalnr_; }
    bool		doPrepare(int);
    bool		doFinish(bool);
    bool		doWork(od_int64,od_int64,int);
    const char*		nrDoneText() const { return "Traces written"; }

    char		getNewTrace(SeisTrc&,int threadidx);

    od_int64			totalnr_;
    IOObj*			input_;
    IOObj*			output_;
    VelocityDesc		veldesc_;
    HorSampling			hrg_;
    FixedString			errmsg_;

    SeisTrcReader*		reader_;
    SeisTrcWriter*		writer_;
    SeisSequentialWriter*	sequentialwriter_;

    Threads::ConditionVar	lock_;
};



}; //namespace

#endif
