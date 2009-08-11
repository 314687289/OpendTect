#ifndef bendpointfinder_h
#define bendpointfinder_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		Aug 2009
 RCS:		$Id: bendpointfinder.h,v 1.1 2009-08-11 13:00:25 cvskris Exp $
________________________________________________________________________

*/

#include "task.h"
#include "thread.h"
#include "ranges.h"
#include "position.h"


/*!Base class that does the majority of the work finding bendpoints. Adaptions 
   to different data-types are done in subclasses. */

class BendPointFinderBase : public ParallelTask
{
public:

    const TypeSet<int>&		bendPoints() const { return bendpts_; }

protected:
			BendPointFinderBase( int sz, float eps );
    od_int64		nrIterations() const { return sz_; }
    bool		doWork( od_int64, od_int64, int );
    virtual float	getMaxSqDistToLine(int& idx, int start,
	    				   int stop ) const		= 0;
    			/*!<Give the index of the point that is furthest from
			    the line from start to stop.
			    \returns the squre of the largest distance */
    void		findInSegment( int, int );
    bool		doPrepare(int);
    bool		doFinish(bool);

    TypeSet<int>		bendpts_;
    TypeSet<Interval<int> >	queue_;
    Threads::ConditionVar	lock_;
    bool			finished_;
    int				nrwaiting_;
    int				nrthreads_;
    

    int				sz_;
    const float			epssq_;
};


class BendPointFinder2D : public BendPointFinderBase
{
public:
    		BendPointFinder2D(const TypeSet<Coord>&, float eps);
protected:
    float	getMaxSqDistToLine(int& idx, int start, int stop ) const;

    const TypeSet<Coord>&	coords_;
};


class BendPointFinder3D : public BendPointFinderBase
{
public:
    		BendPointFinder3D(const TypeSet<Coord3>&,
				  const Coord3& scale, float eps);
protected:
    float	getMaxSqDistToLine(int& idx, int start, int stop ) const;

    const TypeSet<Coord3>&	coords_;
    const Coord3		scale_;
};
#endif
