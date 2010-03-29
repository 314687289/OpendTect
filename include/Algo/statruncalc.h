#ifndef statruncalc_h
#define statruncalc_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl (org) / Bert Bril (rev)
 Date:          10-12-1999 / Sep 2006
 RCS:           $Id: statruncalc.h,v 1.19 2010-03-29 07:10:16 cvsbert Exp $
________________________________________________________________________

-*/

#include "convert.h"
#include "math2.h"
#include "stattype.h"
#include "sorting.h"
#include "sets.h"


#define mUndefReplacement 0

namespace Stats
{


/*!\brief setup for the Stats::RunCalc object

  medianEvenHandling() is tied to OD_EVEN_MEDIAN_AVERAGE, OD_EVEN_MEDIAN_LOWMID,
  and settings dTect.Even Median.Average and dTect.Even Median.LowMid.
  When medianing over an even number of points, either take the low mid (<0),
  hi mid (>0), or avg the two middles. By default, hi mid is used.
 
 */

mClass RunCalcSetup
{
public:
    			RunCalcSetup( bool weighted=false )
			    : weighted_(weighted)
			    , needextreme_(false)
			    , needsums_(false)
			    , needmed_(false)
			    , needmostfreq_(false)	{}

    RunCalcSetup&	require(Type);

    static int		medianEvenHandling();

    bool		isWeighted() const	{ return weighted_; }
    bool		needExtreme() const	{ return needextreme_; }
    bool		needSums() const	{ return needsums_; }
    bool		needMedian() const	{ return needmed_; }
    bool		needMostFreq() const	{ return needmostfreq_; }

protected:

    template <class T>
    friend class	RunCalc;
    template <class T>
    friend class	WindowedCalc;

    bool		weighted_;
    bool		needextreme_;
    bool		needsums_;
    bool		needmed_;
    bool		needmostfreq_;

};


/*!\brief calculates mean, min, max, etc. as running values.

The idea is that you simply add values and ask for a stat whenever needed.
The clear() method resets the object and makes it able to work with new data.

Adding values can be doing with weight (addValue) or without (operator +=).
You can remove a value; for Min or Max this has no effect as this would
require buffering all data.

The mostFrequent assumes the data contains integer classes. Then the class
that is found most often will be the output. Weighting, again, assumes integer
values. Beware that if you pass data that is not really class-data, the
memory consumption can become large (and the result will be rather
uninteresting).

The variance won't take the decreasing degrees of freedom into consideration
when weights are provided.


The object is ready to use with int, float and double types. If other types
are needed, you may need to specialise an isZero function for each new type.

-*/


template <class T>
class RunCalc
{
public:

			RunCalc( const RunCalcSetup& s )
			    : setup_(s)		{ clear(); }
    inline void		clear();
    const RunCalcSetup&	setup() const		{ return setup_; }
    bool		isWeighted() const	{ return setup_.weighted_; }

    inline RunCalc<T>&	addValue(T data,T weight=1);
    inline RunCalc<T>&	addValues(int sz,const T* data,const T* weights=0);
    inline RunCalc<T>&	replaceValue(T olddata,T newdata,T wt=1);
    inline RunCalc<T>&	removeValue(T data,T weight=1);

    inline double	getValue(Type) const;
    inline int		getIndex(Type) const; //!< only for Median, Min and Max

    inline RunCalc<T>&	operator +=( T t )	{ return addValue(t); }
    inline bool		hasUndefs() const	{ return nrused_ != nradded_; }
    inline int		size( bool used=true ) const
    			{ return used ? nrused_ : nradded_; }
    inline bool		isEmpty() const		{ return size() == 0; }

    inline int		count() const		{ return nrused_; }
    inline double	average() const;
    inline double	variance() const; 
    inline double	normvariance() const;
    inline T		mostFreq() const;
    inline T		sum() const;
    inline T		min(int* index_of_min=0) const;
    inline T		max(int* index_of_max=0) const;
    inline T		extreme(int* index_of_extr=0) const;
    inline T		median(int* index_of_median=0) const;
    inline T		sqSum() const;
    inline double	rms() const;
    inline double	stdDev() const;

    inline T		clipVal(float ratio,bool upper) const;
    			//!< require median; 0 <= ratio <= 1

    TypeSet<T>		vals_;

protected:

    RunCalcSetup	setup_;

    int			nradded_;
    int			nrused_;
    int			minidx_;
    int			maxidx_;
    T			minval_;
    T			maxval_;
    T			sum_x;
    T			sum_xx;
    T			sum_w;
    T			sum_wx;
    T			sum_wxx;
    TypeSet<int>	clss_;
    TypeSet<T>		clsswt_;

    inline bool		isZero( const T& t ) const	{ return !t; }

    mutable int		curmedidx_;
    inline bool		findMedIdx(T) const;
    inline bool		addExceptMed(T,T);
    inline bool		remExceptMed(T,T);

};

template <>
inline bool RunCalc<float>::isZero( const float& val ) const
{ return mIsZero(val,1e-6); }


template <>
inline bool RunCalc<double>::isZero( const double& val ) const
{ return mIsZero(val,1e-10); }


/*!\brief RunCalc manager which buffers a part of the data.
 
  Allows calculating running stats on a window only. Once the window is full,
  WindowedCalc will replace the first value added (fifo).
 
 */

template <class T>
class WindowedCalc
{
public:
			WindowedCalc( const RunCalcSetup& rcs, int sz )
			    : calc_(rcs)
			    , sz_(sz)
			    , wts_(calc_.isWeighted() ? new T [sz] : 0)
			    , vals_(new T [sz])	{ clear(); }
			~WindowedCalc()	
				{ delete [] vals_; delete [] wts_; }
    inline void		clear();

    inline WindowedCalc& addValue(T data,T weight=1);
    inline WindowedCalc& operator +=( T t )	{ return addValue(t); }

    inline int		getIndex(Type) const;
    			//!< Only use for Min, Max or Median
    inline double	getValue(Type) const;

    inline T		count() const	{ return full_ ? sz_ : posidx_; }

#   define			mRunCalc_DefEqFn(ret,fn) \
    inline ret			fn() const	{ return calc_.fn(); }
    mRunCalc_DefEqFn(double,	average)
    mRunCalc_DefEqFn(double,	variance)
    mRunCalc_DefEqFn(double,	normvariance)
    mRunCalc_DefEqFn(T,		sum)
    mRunCalc_DefEqFn(T,		sqSum)
    mRunCalc_DefEqFn(double,	rms)
    mRunCalc_DefEqFn(double,	stdDev)
    mRunCalc_DefEqFn(T,		mostFreq)
#   undef			mRunCalc_DefEqFn

    inline T			min(int* i=0) const;
    inline T			max(int* i=0) const;
    inline T			extreme(int* i=0) const;
    inline T			median(int* i=0) const;

protected:

    RunCalc<T>	calc_; // has to be before wts_ (see constructor inits)
    const int	sz_;
    T*		wts_;
    T*		vals_;
    int		posidx_;
    bool	empty_;
    bool	full_;
    bool	needcalc_;

    inline void	fillCalc(RunCalc<T>&) const;
};



template <class T> inline
void RunCalc<T>::clear()
{
    sum_x = sum_w = sum_xx = sum_wx = sum_wxx = 0;
    nradded_ = nrused_ = minidx_ = maxidx_ = curmedidx_ = 0;
    minval_ = maxval_ = mUdf(T);
    clss_.erase(); clsswt_.erase(); vals_.erase();
}


template <class T> inline
bool RunCalc<T>::addExceptMed( T val, T wt )
{
    nradded_++;
    if ( mIsUdf(val) )
	return false;
    if ( setup_.weighted_ && (mIsUdf(wt) || isZero(wt)) )
	return false;

    if ( setup_.needextreme_ )
    {
	if ( nrused_ == 0 )
	    minval_ = maxval_ = val;
	else
	{
	    if ( val < minval_ ) { minval_ = val; minidx_ = nradded_ - 1; }
	    if ( val > maxval_ ) { maxval_ = val; maxidx_ = nradded_ - 1; }
	}
    }

    if ( setup_.needmostfreq_ )
    {
	int ival; Conv::set( ival, val );
	int setidx = clss_.indexOf( ival );

	if ( setidx < 0 )
	    { clss_ += ival; clsswt_ += wt; }
	else
	    clsswt_[setidx] += wt;
    }

    if ( setup_.needsums_ )
    {
	sum_x += val;
	sum_xx += val * val;
	if ( setup_.weighted_ )
	{
	    sum_w += wt;
	    sum_wx += wt * val;
	    sum_wxx += wt * val * val;
	}
    }

    nrused_++;
    return true;
}


template <class T> inline
bool RunCalc<T>::remExceptMed( T val, T wt )
{
    nradded_--;
    if ( mIsUdf(val) )
	return false;
    if ( setup_.weighted_ && (mIsUdf(wt) || isZero(wt)) )
	return false;

    if ( setup_.needmostfreq_ )
    {
	int ival; Conv::set( ival, val );
	int setidx = clss_.indexOf( ival );
	int iwt = 1;
	if ( setup_.weighted_ )
	    Conv::set( iwt, wt );

	if ( setidx >= 0 )
	{
	    clsswt_[setidx] -= wt;
	    if ( clsswt_[setidx] <= 0 )
	    {
		clss_.remove( setidx );
		clsswt_.remove( setidx );
	    }
	}
    }

    if ( setup_.needsums_ )
    {
	sum_x -= val;
	sum_xx -= val * val;
	if ( setup_.weighted_ )
	{
	    sum_w -= wt;
	    sum_wx -= wt * val;
	    sum_wxx -= wt * val * val;
	}
    }

    nrused_--;
    return true;
}


template <class T> inline
RunCalc<T>& RunCalc<T>::addValue( T val, T wt )
{
    if ( !addExceptMed(val,wt) || !setup_.needmed_ )
	return *this;

    int iwt = 1;
    if ( setup_.weighted_ )
	Conv::set( iwt, wt );
    for ( int idx=0; idx<iwt; idx++ )
	vals_ += val;

    return *this;
}


template <class T> inline
RunCalc<T>& RunCalc<T>::addValues( int sz, const T* data, const T* weights )
{
    for ( int idx=0; idx<sz; idx++ )
	addValue( data[idx], weights ? weights[idx] : 1 );
    return *this;
}


template <class T> inline
bool RunCalc<T>::findMedIdx( T val ) const
{
    if ( curmedidx_ >= vals_.size() )
	curmedidx_ = 0;

    if ( vals_[curmedidx_] != val ) // oh well, need to search anyway
    {
	curmedidx_ = vals_.indexOf( val );
	if ( curmedidx_ < 0 )
	    { curmedidx_ = 0; return false; }
    }

    return true;
}


template <class T> inline
RunCalc<T>& RunCalc<T>::replaceValue( T oldval, T newval, T wt )
{
    remExceptMed( oldval, wt );
    addExceptMed( newval, wt );
    if ( !setup_.needmed_ || vals_.isEmpty() )
	return *this;

    int iwt = 1;
    if ( setup_.weighted_ )
	Conv::set( iwt, wt );

    const bool newisudf = mIsUdf(newval);
    for ( int idx=0; idx<iwt; idx++ )
    {
	if ( !findMedIdx(oldval) )
	    break;

	if ( newisudf )
	    vals_.remove( curmedidx_ );
	else
	    vals_[curmedidx_] = newval;
	curmedidx_++;
    }

    return *this;
}


template <class T> inline
RunCalc<T>& RunCalc<T>::removeValue( T val, T wt )
{
    remExceptMed( val, wt );
    if ( !setup_.needmed_ || vals_.isEmpty() )
	return *this;

    int iwt = 1;
    if ( setup_.weighted_ )
	Conv::set( iwt, wt );

    for ( int idx=0; idx<iwt; idx++ )
    {
	if ( findMedIdx(val) )
	    vals_.remove( curmedidx_ );
    }

    return *this;
}


template <class T> inline
double RunCalc<T>::getValue( Stats::Type t ) const
{
    switch ( t )
    {
	case Count:		return count();
	case Average:		return average();
	case Median:		return median();
	case RMS:		return rms();
	case StdDev:		return stdDev();
	case Variance:		return variance();
	case NormVariance:	return normvariance();			
	case Min:		return min();
	case Max:		return max();
	case Extreme:		return extreme();
	case Sum:		return sum();
	case SqSum:		return sqSum();
	case MostFreq:		return mostFreq();
    }

    return 0;
}


template <class T> inline
int RunCalc<T>::getIndex( Type t ) const
{
    int ret;
    switch ( t )
    {
	case Min:		(void)min( &ret );	break;
	case Max:		(void)max( &ret );	break;
	case Extreme:		(void)extreme( &ret );	break;
	case Median:		(void)median( &ret );	break;
	default:		ret = 0;	break;
    }
    return ret;
}


#undef mRunCalc_ChkEmpty
#define mRunCalc_ChkEmpty(typ) \
    if ( nrused_ < 1 ) return mUdf(typ);

template <class T>
inline double RunCalc<T>::stdDev() const
{
    mRunCalc_ChkEmpty(double);

    double v = variance();
    return v > 0 ? Math::Sqrt( v ) : 0;
}


template <class T>
inline double RunCalc<T>::average() const
{
    mRunCalc_ChkEmpty(double);

    if ( !setup_.weighted_ )
	return ((double)sum_x) / nrused_;

    return isZero(sum_w) ? mUdf(double) : ((double)sum_wx) / sum_w;
}


template <class T>
inline T RunCalc<T>::mostFreq() const
{
    if ( clss_.isEmpty() )
	return mUdf(T);

    T maxwt = clsswt_[0]; int ret = clss_[0];
    for ( int idx=1; idx<clss_.size(); idx++ )
    {
	if ( clsswt_[idx] > maxwt )
	    { maxwt = clsswt_[idx]; ret = clss_[idx]; }
    }

    return ret;
}


template <class T>
inline T RunCalc<T>::median( int* idx_of_med ) const
{
    if ( idx_of_med ) *idx_of_med = 0;
    const int sz = vals_.size();
    if ( sz < 2 )
	return sz < 1 ? mUdf(T) : vals_[0];

    int mididx = sz / 2;
    T* valarr = const_cast<T*>( vals_.arr() );
    if ( !idx_of_med )
	quickSort( valarr, sz );
    else
    {
	mGetIdxArr(int,idxs,sz)
	quickSort( valarr, idxs, sz );
	*idx_of_med = idxs[ mididx ];
	delete [] idxs;
    }

    if ( sz%2 == 0 )
    {
	const int policy = setup_.medianEvenHandling();
	if ( mididx == 0 )
	    return policy ? (policy < 0 ? vals_[0] : vals_[1])
					: (vals_[0] + vals_[1]) / 2;
	if ( policy == 0 )
	    return (vals_[mididx] + vals_[mididx-1]) / 2;
	else if ( policy == 1 )
	   mididx--;
    }

    return vals_[ mididx ];
}


template <class T>
inline T RunCalc<T>::sum() const
{
    mRunCalc_ChkEmpty(T);
    return sum_x;
}



template <class T>
inline T RunCalc<T>::sqSum() const
{
    mRunCalc_ChkEmpty(T);
    return sum_xx;
}


template <class T>
inline double RunCalc<T>::rms() const
{
    mRunCalc_ChkEmpty(double);

    if ( !setup_.weighted_ )
	return Math::Sqrt( ((double)sum_xx) / nrused_ );

    return isZero(sum_w) ? mUdf(double) : Math::Sqrt( ((double)sum_wxx)/sum_w );
}


template <class T>
inline double RunCalc<T>::variance() const 
{
    if ( nrused_ < 2 ) return 0;

    if ( !setup_.weighted_ )
	return ( sum_xx - (sum_x * ((double)sum_x) / nrused_) )
	     / (nrused_ - 1);

    return (sum_wxx - (sum_wx * ((double)sum_wx) / sum_w) )
	 / ( (nrused_-1) * ((double)sum_w) / nrused_ );
}


template <class T>
inline double RunCalc<T>::normvariance() const
{
    if ( nrused_ < 2 ) return 0;

    double fact = 0.1;
    double avg = average();
    double var = variance();
    return var / (avg*avg + fact*var);
}


template <class T>
inline T RunCalc<T>::min( int* index_of_min ) const
{
    if ( index_of_min ) *index_of_min = minidx_;
    mRunCalc_ChkEmpty(T);
    return minval_;
}


template <class T>
inline T RunCalc<T>::max( int* index_of_max ) const
{
    if ( index_of_max ) *index_of_max = maxidx_;
    mRunCalc_ChkEmpty(T);
    return maxval_;
}


template <class T>
inline T RunCalc<T>::extreme( int* index_of_extr ) const
{
    if ( index_of_extr ) *index_of_extr = 0;
    mRunCalc_ChkEmpty(T);

    const T maxcmp = maxval_ < 0 ? -maxval_ : maxval_;
    const T mincmp = minval_ < 0 ? -minval_ : minval_;
    if ( maxcmp < mincmp )
    {
	if ( index_of_extr ) *index_of_extr = minidx_;
	return minval_;
    }
    else
    {
	if ( index_of_extr ) *index_of_extr = maxidx_;
	return maxval_;
    }
}


template <class T>
inline T RunCalc<T>::clipVal( float ratio, bool upper ) const
{
    mRunCalc_ChkEmpty(T);
    (void)median();
    const int lastidx = vals_.size();
    const float fidx = ratio * lastidx;
    const int idx = fidx <= 0 ? 0 : (fidx > lastidx ? lastidx : (int)fidx);
    return vals_[upper ? lastidx - idx : idx];
}


template <class T> inline
void WindowedCalc<T>::clear()
{
    posidx_ = 0; empty_ = true; full_ = false;
    needcalc_ = calc_.setup().needSums() || calc_.setup().needMostFreq();
    calc_.clear();
}


template <class T> inline
void WindowedCalc<T>::fillCalc( RunCalc<T>& calc ) const
{
    if ( empty_ ) return;

    const int stopidx = full_ ? sz_ : posidx_;
    for ( int idx=posidx_; idx<stopidx; idx++ )
	calc.addValue( vals_[idx], wts_ ? wts_[idx] : 1 );
    for ( int idx=0; idx<posidx_; idx++ )
	calc.addValue( vals_[idx], wts_ ? wts_[idx] : 1 );
}


template <class T> inline
double WindowedCalc<T>::getValue( Type t ) const
{
    switch ( t )
    {
    case Min:		return min();
    case Max:		return max();
    case Extreme:	return max();
    case Median:	return median();
    default:		break;
    }
    return calc_.getValue( t );
}


template <class T> inline
int WindowedCalc<T>::getIndex( Type t ) const
{
    int ret;
    switch ( t )
    {
	case Min:		(void)min( &ret );	break;
	case Max:		(void)max( &ret );	break;
	case Extreme:		(void)extreme( &ret );	break;
	case Median:		(void)median( &ret );	break;
	default:		ret = 0;	break;
    }
    return ret;
}


template <class T> inline
T WindowedCalc<T>::min( int* index_of_min ) const
{
    RunCalc<T> calc( RunCalcSetup().require(Stats::Min) );
    fillCalc( calc );
    return calc.min( index_of_min );
}


template <class T> inline
T WindowedCalc<T>::max( int* index_of_max ) const
{
    RunCalc<T> calc( RunCalcSetup().require(Stats::Max) );
    fillCalc( calc );
    return calc.max( index_of_max );
}


template <class T> inline
T WindowedCalc<T>::extreme( int* index_of_extr ) const
{
    RunCalc<T> calc( RunCalcSetup().require(Stats::Extreme) );
    fillCalc( calc );
    return calc.extreme( index_of_extr );
}


template <class T> inline
T WindowedCalc<T>::median( int* index_of_med ) const
{
    RunCalc<T> calc( RunCalcSetup().require(Stats::Median) );
    fillCalc( calc );
    return calc.median( index_of_med );
}


template <class T>
inline WindowedCalc<T>&	WindowedCalc<T>::addValue( T val, T wt )
{
    if ( needcalc_ )
    {
	if ( !full_ )
	    calc_.addValue( val, wt );
	else
	{
	    if ( !wts_ || wt == wts_[posidx_] )
		calc_.replaceValue( vals_[posidx_], val, wt );
	    else
	    {
		calc_.removeValue( vals_[posidx_], wts_[posidx_] );
		calc_.addValue( val, wt );
	    }
	}
    }

    vals_[posidx_] = val;
    if ( wts_ ) wts_[posidx_] = wt;

    posidx_++;
    if ( posidx_ >= sz_ )
	{ full_ = true; posidx_ = 0; }

    empty_ = false;
    return *this;
}

#undef mRunCalc_ChkEmpty

}; // namespace Stats

#endif
