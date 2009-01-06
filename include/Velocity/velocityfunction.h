#ifndef velocityfunction_h
#define velocityfunction_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: velocityfunction.h,v 1.2 2009-01-06 10:04:36 cvsranojay Exp $
________________________________________________________________________


-*/

#include "enums.h"
#include "factory.h"
#include "multiid.h"
#include "position.h"
#include "ranges.h"
#include "refcount.h"
#include "samplingdata.h"
#include "thread.h"
#include "veldesc.h"

namespace Attrib { class DataHolder; };

class BinIDValueSet;
class HorSampling;


namespace Vel
{

class FunctionSource;

/*!Velocity versus depth for one location. The source of information is 
   different for each subclass, but is typically user-picks, wells
   or velocity volumes. */

mClass Function
{ mRefCountImpl(Function)
public:
				Function(FunctionSource&);

    virtual const VelocityDesc&	getDesc() const;

    float			getVelocity(float z) const;
    const BinID&		getBinID() const;
    virtual bool		moveTo(const BinID&);

    virtual void		removeCache();

    virtual StepInterval<float>	getAvailableZ() const			= 0;
    void			setDesiredZRange(const StepInterval<float>&);
    const StepInterval<float>&	getDesiredZ() const;
    
protected:

    virtual bool		computeVelocity(float z0, float dz, int nr,
						float* res ) const	= 0;

    FunctionSource&		source_;
    BinID			bid_;
    StepInterval<float>		desiredrg_;

private:

    mutable Threads::Mutex	cachelock_;
    mutable TypeSet<float>*	cache_;
    mutable SamplingData<double> cachesd_;
};


/*!A source of Velocity functions of a certain sort. The FunctionSource
   can create Functions at certian BinID locations. */

mClass FunctionSource : public CallBacker
{ mRefCountImplNoDestructor(FunctionSource);
public:
    mDefineFactory1ParamInClass( FunctionSource, const MultiID&, factory );

    virtual const char*		type() const 				= 0;
    virtual BufferString	userName() const;
    virtual const VelocityDesc&	getDesc() const				= 0;
    virtual void		getSurroundingPositions(const BinID&,
				    BinIDValueSet&) const;
    virtual void		getAvailablePositions(BinIDValueSet&) const {}
    virtual void		getAvailablePositions(HorSampling&) const {}

    const Function*	getFunction(const BinID&);
    virtual Function*	createFunction(const BinID&)		= 0;

    const MultiID&		multiID() const		{ return mid_; }

    virtual NotifierAccess*	changeNotifier()	{ return 0; }
    virtual BinID		changeBinID() const	{ return BinID(-1,-1); }

    virtual void		fillPar(IOPar&) const	{}
    virtual bool		usePar(const IOPar&)	{ return true; }

    const char*			errMsg() const;

protected:

    int				findFunction(const BinID&) const;
    friend			class Function;
    void			removeFunction(Function* v);
    				/*!<Called by function when they are deleted. */

    ObjectSet<Function>	functions_;
    MultiID			mid_;
    BufferString		errmsg_;
};

}; //namespace


#endif
