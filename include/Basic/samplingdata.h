#ifndef samplingdata_h
#define samplingdata_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 RCS:		$Id: samplingdata.h,v 1.8 2006-04-27 16:07:06 cvskris Exp $
________________________________________________________________________

-*/

#include "ranges.h"


/*!\brief holds the fundamental sampling info: start and interval. */

template <class T>
class SamplingData
{
public:
		SamplingData( T sa=0, T se=1 )
		: start(sa), step(se)			{}
		SamplingData( const StepInterval<T>& intv )
		: start(intv.start), step(intv.step)	{}

    void	operator+=( const SamplingData& sd )
		{ start += sd.start; step += sd.step; }
    int		operator==( const SamplingData& sd ) const
		{ return start == sd.start && step == sd.step; }
    int		operator!=( const SamplingData& sd ) const
		{ return ! (sd == *this); }

    StepInterval<T> interval( int nrsamp ) const
		{ return StepInterval<T>( start, start+(nrsamp-1)*step, step); }
    template <class IT> float	getIndex( IT val ) const
		{ return (val-start) / (float) step; }
    int		nearestIndex( T x ) const
		{ float fidx = getIndex(x); return mNINT(fidx); }
    template <class IT>
    T		atIndex( IT idx ) const
		{ return start + step * idx; }

    T		start;
    T		step;

};


#endif
