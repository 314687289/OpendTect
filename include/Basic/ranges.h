#ifndef ranges_H
#define ranges_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Ranges
 RCS:		$Id: ranges.h,v 1.34 2005-06-01 08:27:07 cvskris Exp $
________________________________________________________________________

-*/

#include "general.h"
#include "errh.h"


/*!\brief interval of values.

Note that start does not need to be lower than stop. That's why there is a
sort() method.

*/

template <class T>
class Interval
{
public:
    inline			Interval();
    inline			Interval( const T& t1, const T& t2 );
    inline virtual Interval<T>* clone() const;

    inline int		operator==( const Interval<T>& i ) const;
    inline int		operator!=( const Interval<T>& i ) const;

    inline T		width( bool allowrev=true ) const;
    inline T		center() const;
    inline void		shift( const T& len );
    inline void		widen( const T& len, bool allowrev=true );
    inline void		scale(const T&);

    template <class TT>
    inline bool		includes( const TT& t, bool allowrev=true ) const;
    inline void		include( const T& i, bool allowrev=true );
    inline void		include( const Interval<T>& i, bool allowrev=true );

    inline T		atIndex( int idx, const T& step ) const;

    template <class X>
    inline int		getIndex( const X& t, const T& step ) const;

    template <class X>
    inline float	getfIndex( const X& t, const T& step ) const;

    template <class X>
    inline X		limitValue( const X& t ) const;
		

    template <class X>
    inline int		nearestIndex( const X& x, const T& step ) const;
    virtual void	sort( bool asc=true );

    T			start;
    T			stop;

protected:

    inline bool		isRev() const		{ return start > stop; }

};


typedef Interval<int> SampleGate;
typedef Interval<double> TimeGate;
typedef Interval<double> DepthGate;


#define cloneTp	mPolyRet( Interval<T>, StepInterval<T> )

/*!\brief Interval with step. */

template <class T>
class StepInterval : public Interval<T>
{
public:
    inline			StepInterval();
    inline			StepInterval( const T& start, const T& stop,
	    				      const T& step );
    inline virtual cloneTp*	clone() const;

    inline T			atIndex( int idx ) const;
    template <class X>
    inline int			getIndex( const X& t ) const;
    template <class X>
    inline float		getfIndex( const X& t ) const;
    template <class X>
    inline int			nearestIndex( const X& x ) const;
    template <class X>
    inline T			snap( const X& t ) const;

    inline int			nrSteps() const;
    virtual inline void		sort( bool asc=true );
    inline void			scale(const T&);

    inline bool			isCompatible( const StepInterval<T>& b,
	    					T eps=mDefEps) const;
				/*!< epsilon refers to the steps,
				  	i.e eps=0.1 allows b to be 0.1 steps
					apart.
				*/

     T				step;

};


template <class T> class IntervalND
{
public:
    				IntervalND( int ndim_ )
				    : ranges (new Interval<T>[ndim_] )
				    , ndim( ndim_ )
				    , isset( false ) {}

    virtual			~IntervalND() { delete [] ranges; }

    int				nDim() const { return ndim; }
    bool			isSet() const { return isset; }

    const Interval<T>&		getRange(int dim) const { return ranges[dim]; }
    template <class TT> inline
    void 			setRange( const TT& val );
    template <class TT> inline
    void			setRange( const TT& start, const TT& stop);
    template <class TT> inline
    void			include( const TT& val );

    template <class TT> inline
    bool			includes( const TT& val ) const;
    inline bool			intersects( const IntervalND<T>& ) const;

protected:

    int			ndim;
    Interval<T>*	ranges;
    bool		isset;

};


template <class T> template <class TT> inline
void IntervalND<T>::setRange( const TT& val )
{
    for ( int dim=0; dim<ndim; dim++ )
	ranges[dim].start = ranges[dim].stop = val[dim];

    isset = true;
}



template <class T> template <class TT> inline
void IntervalND<T>::setRange( const TT& start, const TT& stop)
{
    for ( int dim=0; dim<ndim; dim++ )
    {
	ranges[dim].start = start[dim];
	ranges[dim].stop = stop[dim];
    }

    isset = true;
}


template <class T> template <class TT> inline
void IntervalND<T>::include( const TT& val )
{
#ifdef __debug__
    if ( !isset )
	pErrMsg("Doing include on undefined IntervalND");
#endif

    for ( int dim=0; dim<ndim; dim++ )
	ranges[dim].include(val[dim]);

    isset = true;
}


template <class T> template <class TT> inline
bool IntervalND<T>::includes( const TT& val ) const
{
#ifdef __debug__
    if ( !isset )
    {
	pErrMsg("Doing includes on undefined IntervalND");
	return false;
    }
#endif

    for ( int dim=0; dim<ndim; dim++ )
    {
	if ( !ranges[dim].includes(val[dim]) )
	    return false;
    }

    return true;
}


template <class T> inline
bool IntervalND<T>::intersects( const IntervalND<T>& b ) const
{
    if ( !isset || !b.isset || ndim!=b.ndim)
    {
	pErrMsg("Invalid intersection");
	return false;
    }

    T vector[ndim];
    bool isstart[ndim];

    for ( int dim=0; dim<ndim; dim++ )
    {
	vector[dim] = ranges[dim].start;
	isstart[dim] = true;
    }

    do
    {
	if ( b.includes((T*)vector) )
	    return true;

	int dim = 0;
	while ( dim<ndim )
	{
	    if ( isstart[dim] )
	    {
		isstart[dim] = false;
		vector[dim] = ranges[dim].stop;
		break;
	    }
	    else
	    {
		isstart[dim] = true;
		vector[dim] = ranges[dim].start;
		dim++;
	    }
	}

	if ( dim==ndim )
	    break;

    } while ( true );


    return false;
}


template <class T1,class T2>
inline void assign( Interval<T1>& i1, const Interval<T2>& i2 )
{ i1.start = (T1)i2.start; i1.stop = (T1)i2.stop; }

template <class T1,class T2>
inline void assign( StepInterval<T1>& i1, const StepInterval<T2>& i2 )
{ i1.start = (T1)i2.start; i1.stop = (T1)i2.stop; i1.step = (T1)i2.step; }


#define mDefIntNrSteps(typ) \
inline int StepInterval<typ>::nrSteps() const \
{ \
    if ( !step ) return 0; \
    int ret = ((int)start - stop) / step; \
    return ret < 0 ? -ret : ret; \
}

mDefIntNrSteps(int)
mDefIntNrSteps(long)
mDefIntNrSteps(short)
mDefIntNrSteps(char)
mDefIntNrSteps(unsigned int)
mDefIntNrSteps(unsigned long)
mDefIntNrSteps(unsigned short)
mDefIntNrSteps(unsigned char)

#define mDefFNrSteps(typ,eps) \
inline int StepInterval<typ>::nrSteps() const \
{ \
    if ( !step ) return 0; \
    typ ns = ( (start > stop ? start : stop) \
	    - (start > stop ? stop : start) ) \
	      / (step > 0 ? step : -step); \
    return (int)(ns * (1. + eps)); \
}

mDefFNrSteps(float,1e-4)
mDefFNrSteps(double,1e-8)

#define mDefIntisCompat(typ) \
inline bool StepInterval<typ>::isCompatible( const StepInterval<typ>& b, \
	typ ) const \
{ \
    if ( step!=b.step ) return false; \
\
    const typ diff = start - b.start; \
    return !(diff%step);	\
}

mDefIntisCompat(int)
mDefIntisCompat(long)
mDefIntisCompat(short)
mDefIntisCompat(char)
mDefIntisCompat(unsigned int)
mDefIntisCompat(unsigned long)
mDefIntisCompat(unsigned short)
mDefIntisCompat(unsigned char)

#define mDefFltisCompat(typ) \
inline bool StepInterval<typ>::isCompatible( const StepInterval<typ>& b, \
			typ eps ) const \
{ \
    if ( !mIsEqual(step,b.step,mDefEps) ) return false; \
 \
    typ nrsteps = (start - b.start) / step; \
    int nrstepsi = mNINT( nrsteps ); \
    typ diff = nrsteps - nrstepsi; \
    return ( (diff) < (eps) && (diff) > (-eps) ); \
}

mDefFltisCompat(float)
mDefFltisCompat(double)

template <class T> template <class X> inline
int Interval<T>::nearestIndex( const X& x, const T& step ) const
{
    int nr = getIndex(x,step);
    T atindex = atIndex(nr,step);
    X reldiff = (x-atindex)/step;

    if ( reldiff>=0.5 ) return nr+1;
    else if ( reldiff<=-0.5 ) return nr-1;
    return nr;
}


template <class T> inline Interval<T>::Interval()
{ start = 0; stop = 0; }


template <class T> inline Interval<T>::Interval( const T& t1, const T& t2 )
{ start = t1; stop = t2; }

template <class T> inline
Interval<T>* Interval<T>::clone() const		
{ return new Interval<T>( *this ); }


template <class T> inline
int Interval<T>::operator==( const Interval<T>& i ) const
{ return start == i.start && stop == i.stop; }


template <class T> inline
int Interval<T>::operator!=( const Interval<T>& i ) const
{ return ! (i == *this); }


template <class T> inline
T Interval<T>::width( bool allowrev ) const
{ return allowrev && isRev() ? start - stop : stop - start; }


template <class T> inline
T Interval<T>::center() const
{ return (start+stop)/2; }


template <class T> inline
void Interval<T>::shift( const T& len )
{ start += len; stop += len; }


template <class T> inline
void Interval<T>::widen( const T& len, bool allowrev )
{
    if ( allowrev && isRev() )
	{ start += len; stop -= len; }
    else
	{ start -= len; stop += len; }
}


template <class T> inline
void Interval<T>::scale( const T& factor )
{ start *= factor; stop *= factor; }


template <class T> template <class TT> inline
bool Interval<T>::includes( const TT& t, bool allowrev ) const
{
    return allowrev && isRev()
	? t>=stop && start>=t
	: t>=start && stop>=t;
}


template <class T> inline
void Interval<T>::include( const T& i, bool allowrev )
{
    if ( allowrev && isRev() )
	{ if ( stop>i ) stop=i; if ( start<i ) start=i; }
    else
	{ if ( start>i ) start=i; if ( stop<i ) stop=i; }
}


template <class T> inline
void Interval<T>::include( const Interval<T>& i, bool allowrev )
{ include( i.start, allowrev ); include( i.stop, allowrev ); }


template <class T> inline
T Interval<T>::atIndex( int idx, const T& step ) const
{ return start + step * idx; }


template <class T> template <class X> inline
int Interval<T>::getIndex( const X& t, const T& step ) const
{ return (int)(( t  - start ) / step); }


template <class T> template <class X> inline
float Interval<T>::getfIndex( const X& t, const T& step ) const
{ return (( (float)t  - start ) / step); }


template <class T> template <class X> inline
X Interval<T>::limitValue( const X& t ) const
{
    const bool isrev = isRev();
    if ( (!isrev&&t>stop) || (isrev&&t<stop) ) return stop;
    if ( (!isrev&&t<start) || (isrev&&t>start) ) return start;
    return t;
}
		

template <class T> 
void Interval<T>::sort( bool asc )
{
    if ( (asc && stop<start) || (!asc && start<stop) )
	 Swap(start,stop);
}


template <class T>
StepInterval<T>::StepInterval()
{ step = 1; }


template <class T>
StepInterval<T>::StepInterval( const T& t1, const T& t2, const T& t3 )
    : Interval<T>(t1,t2)
{ step = t3; }


template <class T> inline
cloneTp* StepInterval<T>::clone() const	
{ return new StepInterval<T>( *this ); }


template <class T> inline
T StepInterval<T>::atIndex( int idx ) const
{ return Interval<T>::atIndex(idx,step); }


template <class T> template <class X> inline
int StepInterval<T>::getIndex( const X& t ) const
{ return Interval<T>::getIndex( t, step ); }


template <class T> template <class X> inline
float StepInterval<T>::getfIndex( const X& t ) const
{ return Interval<T>::getfIndex( t, step ); }


template <class T> template <class X> inline
int StepInterval<T>::nearestIndex( const X& x ) const
{ return Interval<T>::nearestIndex( x, step ); }


template <class T> template <class X> inline
T StepInterval<T>::snap( const X& t ) const
{ return atIndex( nearestIndex( t ) ); }


template <class T> inline
void StepInterval<T>::sort( bool asc )
{
    Interval<T>::sort(asc);
    if ( (asc && step < 0) || (!asc && step > 0) )
	step = -step;
}

template <class T> inline
void StepInterval<T>::scale( const T& factor )
{
    Interval<T>::scale( factor );
    step *= factor;
}


#undef cloneTp


#endif
