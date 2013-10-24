#ifndef faulttrace_h
#define faulttrace_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "executor.h"
#include "horsampling.h"
#include "multiid.h"
#include "positionlist.h"
#include "sets.h"
#include "posinfo2dsurv.h"
#include "trigonometry.h"
#include "threadlock.h"

namespace EM { class Fault; class Horizon; }
namespace Geometry { class ExplFaultStickSurface; }
namespace PosInfo { class Line2DData; }
class BinIDValueSet;

/*!
\brief Subclass of Coord3List that represents a fault trace.
*/

mExpClass(EarthModel) FaultTrace : public Coord3List
{
public:

    int			nextID(int) const;
    int			add(const Coord3&);
    int			add(const Coord3&,float trcnr);
    Coord3		get(int) const;
    const TypeSet<int>&	getIndices() const;
    float		getTrcNr(int) const;
    void		set(int,const Coord3&);
    void		set(int,const Coord3&,float);
    void		setIndices(const TypeSet<int>&);
    void		remove(int);
    void		remove(const TypeSet<int>&){};
    bool		isDefined(int) const;
    int			getSize() const	{ return coords_.size(); }
    FaultTrace*		clone();

    bool		isInl() const			{ return isinl_; }
    bool		isEditedOnCrl() const		{ return editedoncrl_; }
    int			lineNr() const			{ return nr_; }
    const Interval<int>& trcRange() const		{ return trcrange_; }
    const Interval<float>& zRange() const		{ return zrange_; }
    void		setIsInl(bool yn)		{ isinl_ = yn; }
    void		setEditedOnCrl(bool yn)		{ editedoncrl_ = yn; }
    void		setLineNr(int nr)		{ nr_ = nr; }

    bool		isCrossing(const BinID&,float,const BinID&,float) const;
    bool		getIntersection(const BinID&,float,const BinID&,float,
	    				BinID&,float&,
					const StepInterval<int>* trcrg=0,
                                        bool snappositive=true) const;
    bool		getHorCrossings(const BinIDValueSet&,
	    				Interval<float>& topzvals,
					Interval<float>& botzvals) const;
    bool		getHorIntersection(const EM::Horizon&,BinID&) const;
    bool		getHorTerminalPos( const EM::Horizon& hor,
					   BinID& pos1bid, float& pos1z,
					   BinID& pos2bid, float& pos2z ) const;
    bool		getImage(const BinID& srcbid,float srcz,
	    			 const Interval<float>& tophorzvals,
				 const Interval<float>& bothorzvals,
				 const StepInterval<int>& trcrg,BinID& imgbid,
				 float& imgz,bool forward) const;

    float		getZValFor(const BinID&) const;
    bool                isOnPosSide(const BinID&,float) const;
    void		addValue(int id,const Coord3&)	{}
    void		computeRange();
    bool                includes(const BinID&) const;
    bool		isOK() const;
    bool		isOnFault(const BinID&,float z,float threshold) const;
    			// threshold dist in measured in BinID units

    enum Act		{ AllowCrossing=0, ForbidCrossing=1, 
			  ForbidCrossHigher=2, ForbidCrossLower=3 };
    static void		getAllActNames(BufferStringSet&);
    static const char*	sKeyFaultAct()	{ return "Fault Act"; }

protected:

    			~FaultTrace() {}

    void		computeTraceSegments();
    Coord		getIntersection(const BinID&,float,
	    				const BinID&,float) const;
    bool		handleUntrimmed(const BinIDValueSet&,Interval<float>&,
	    				const BinID&,const BinID&,bool) const;

    bool		isinl_;
    bool		editedoncrl_;
    int			nr_;
    TypeSet<Coord3>	coords_;
    TypeSet<int>	coordindices_;
    TypeSet<float>	trcnrs_;	// For 2D only;
    Interval<int>	trcrange_;
    Interval<float>	zrange_;
    TypeSet<Line2>	tracesegs_;

    Threads::Lock	lock_;
public:

};


/*!
\brief FaultTrace holder 
*/

mExpClass(EarthModel) FaultTrcHolder
{
public:
			FaultTrcHolder();
			~FaultTrcHolder();

    const FaultTrace*	getTrc(int linenr,bool isinl) const;
    int			indexOf(int linenr,bool isinl) const;

    bool		isEditedOnCrl() const;

    ObjectSet<FaultTrace>	traces_;
			    /*For 3D: one for each inline followed by
				      one for each crossline.
			      For 2D: One for each stick.*/

    HorSampling		hs_;
};


/*!
\brief FaultTrace extractor 
*/

mExpClass(EarthModel) FaultTraceExtractor : public ParallelTask
{
public:
    			FaultTraceExtractor(const EM::Fault&,FaultTrcHolder&);
			~FaultTraceExtractor();

    const char*		message() const;

protected:

    virtual bool	extractFaultTrace(int)			= 0;
    virtual od_int64	nrIterations() const;
    virtual bool	doPrepare(int);
    virtual bool	doWork(od_int64,od_int64,int);
    virtual bool	doFinish(bool)	{ return true; }

    const EM::Fault&	fault_;
    FaultTrcHolder&	holder_;
    bool		editedoncrl_;
    od_int64		totalnr_;
};


mExpClass(EarthModel) FaultTraceExtractor3D : public FaultTraceExtractor
{
public:
    			FaultTraceExtractor3D(const EM::Fault&,FaultTrcHolder&);
			~FaultTraceExtractor3D();

protected:

    bool		extractFaultTrace(int);

    virtual bool	doPrepare(int);

    Geometry::ExplFaultStickSurface* fltsurf_;

};


mExpClass(EarthModel) FaultTraceExtractor2D : public FaultTraceExtractor
{
public:
    			FaultTraceExtractor2D(const EM::Fault&,FaultTrcHolder&,
					      const PosInfo::GeomID&);
			~FaultTraceExtractor2D();

protected:

    bool		extractFaultTrace(int);
    virtual bool	doPrepare(int);
    virtual bool	doFinish(bool);

    PosInfo::GeomID	geomid_;
    PosInfo::Line2DData& linegeom_;
};


/*!
\brief FaultTrace data provider
*/

mExpClass(EarthModel) FaultTrcDataProvider
{
public:
			FaultTrcDataProvider();
			FaultTrcDataProvider(const PosInfo::GeomID&);
			~FaultTrcDataProvider();

    bool		init(const TypeSet<MultiID>&,const HorSampling&,
	    		     TaskRunner* tr=0);

    bool		is2D() const		{ return is2d_; }
    int			nrFaults() const;
    HorSampling		range(int) const;
    int			nrSticks(int fltidx) const;
    bool		isEditedOnCrl(int fltidx) const;

    bool		hasFaults(const BinID&) const;
    const FaultTrace*	getFaultTrace(int,int,bool) const;
    const FaultTrace*	getFaultTrace2D(int fltidx,int stickidx) const;
    bool		isCrossingFault(const BinID& b1,float z1,
	    				const BinID& b2,float z2) const;
    bool		getFaultZVals(const BinID&,TypeSet<float>&) const;
    bool		isOnFault(const BinID&,float z,float threshold) const;

    void		clear();
    bool		isEmpty() const;
    const char*		errMsg() const;

protected:

    bool		calcFaultBBox(const EM::Fault&,HorSampling&) const;
    bool		get2DTraces(const TypeSet<MultiID>&,TaskRunner*);

    ObjectSet<FaultTrcHolder>	holders_;

    PosInfo::GeomID	geomid_;
    BufferString	errmsg_;
    bool		is2d_;
};

#endif
