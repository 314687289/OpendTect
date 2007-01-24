#ifndef samplingdata_h
#define samplingdata_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 RCS:		$Id: samplingdata.h,v 1.9 2007-01-24 14:22:37 cvskris Exp $
________________________________________________________________________

-*/

#include "ranges.h"


/*!\brief holds the fundamental sampling info: start and interval. */

template <class T>
class SamplingData
{
public:
    inline				SamplingData(T sa=0,T se=1);
    template <class FT>	inline		SamplingData(const SamplingData<FT>&);
    template <class FT>	inline		SamplingData(const StepInterval<FT>&);

    inline void				operator+=(const SamplingData& sd);
    inline int				operator==(const SamplingData& sd)const;
    inline int				operator!=(const SamplingData& sd)const;

    inline StepInterval<T>		interval(int nrsamp) const;
    template <class IT> inline float	getIndex(IT val) const;
    inline int				nearestIndex(T x) const;
    template <class IT> inline T	atIndex(IT idx) const;

    T					start;
    T					step;
};


template <class T> inline
SamplingData<T>::SamplingData( T sa, T se )
    : start(sa), step(se)
{}


template <class T> 
template <class FT> inline
SamplingData<T>::SamplingData( const SamplingData<FT>& templ )
    : start( templ.start ), step( templ.step )
{}


template <class T>
template <class FT> inline
SamplingData<T>::SamplingData( const StepInterval<FT>& intv )
    : start(intv.start), step(intv.step)
{}


template <class T> inline
void SamplingData<T>::operator+=( const SamplingData& sd )
{ start += sd.start; step += sd.step; }


template <class T> inline
int SamplingData<T>::operator==( const SamplingData& sd ) const
{ return start == sd.start && step == sd.step; }


template <class T> inline
int SamplingData<T>::operator!=( const SamplingData& sd ) const
{ return ! (sd == *this); }


template <class T> inline
StepInterval<T> SamplingData<T>::interval( int nrsamp ) const
{ return StepInterval<T>( start, start+(nrsamp-1)*step, step); }


template <class T>
template <class IT> inline
float SamplingData<T>::getIndex( IT val ) const
{ return (val-start) / (float) step; }


template <class T> inline
int SamplingData<T>::nearestIndex( T x ) const
{ float fidx = getIndex(x); return mNINT(fidx); }


template <class T>
template <class IT> inline
T SamplingData<T>::atIndex( IT idx ) const
{ return start + step * idx; }


#endif
