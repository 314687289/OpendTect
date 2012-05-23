#ifndef raytracerrunner_h
#define raytracerrunner_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
 RCS:           $Id: raytracerrunner.h,v 1.9 2012-05-23 14:47:21 cvsbruno Exp $
________________________________________________________________________

-*/

#include "ailayer.h"
#include "iopar.h"
#include "task.h"
#include "ranges.h"
#include "raytrace1d.h"

mClass RayTracerRunner : public ParallelTask
{
public:
    				RayTracerRunner(const TypeSet<ElasticModel>&,
						const IOPar& raypar);
    				RayTracerRunner(const IOPar& raypar);
    				~RayTracerRunner();

    const char*			errMsg() const 	{ return errmsg_.buf(); }

    //before exectution only
    void			setOffsets(TypeSet<float> offsets);
    void			addModel(const ElasticModel&,bool dosingle); 

    //available after excution
    ObjectSet<RayTracer1D>& 	rayTracers() 	{ return raytracers_; }

protected:

    IOPar			raypar_;

    bool                        doPrepare(int);
    bool                	doWork(od_int64,od_int64,int);
    od_int64                    nrIterations() const;

    BufferString		errmsg_;

    TypeSet<ElasticModel> aimodels_;
    ObjectSet<RayTracer1D> 	raytracers_;
};

#endif
