#ifndef arrayndalgo_h
#define arrayndalgo_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id$
________________________________________________________________________


@$*/

#include "algomod.h"
#include "arraynd.h"
#include "coord.h"
#include "enums.h"
#include "arrayndslice.h"
#include "mathfunc.h"
#include "periodicvalue.h"


#define mComputeTrendAandB( sz ) { \
	aval = ( (fT)sz * crosssum - sum * (fT)sumindexes ) / \
	       ( (fT)sz * (fT)sumsqidx - (fT)sumindexes * (fT)sumindexes );\
	bval = ( sum * (fT)sumsqidx - (fT)sumindexes * crosssum ) / \
	       ( (fT)sz * (fT)sumsqidx - (fT)sumindexes * (fT)sumindexes ); }


/*!\brief [do not use, helper function] */

template <class T, class fT>
inline bool removeLinPart( const ArrayND<T>& in_, ArrayND<T>* out, bool trend )
{
    ArrayND<T>& out_ = out ? *out : const_cast<ArrayND<T>&>(in_);
    if ( out && in_.info() != out_.info() )
	return false;

    T avg = 0;
    T sum = 0;
    od_int64 sumindexes = 0;
    od_int64 sumsqidx = 0;
    T crosssum = 0;
    T aval = mUdf(T);
    T bval = mUdf(T);

    const od_int64 sz = in_.info().getTotalSz();

    const T* inpptr = in_.getData();
    T* outptr = out_.getData();

    if ( inpptr && outptr )
    {
	int count = 0;
	for ( int idx=0; idx<sz; idx++ )
	{
	    const T value = inpptr[idx];
	    if ( mIsUdf(value) )
		continue;

	    sum += inpptr[idx];
	    count++;
	    if ( !trend )
		continue;

	    sumindexes += idx;
	    sumsqidx += idx * idx;
	    crosssum += inpptr[idx] * (fT)idx;
	}

	if ( count <= 1 )
	    return false;

	if ( trend )
	    mComputeTrendAandB(count)
	else
	    avg = sum / (fT)count;

	for ( int idx=0; idx<sz; idx++ )
	{
	    const T value = inpptr[idx];
	    outptr[idx] = mIsUdf(value) ? mUdf(T)
					: (trend ? value - (aval*(fT)idx+bval)
					:  value - avg);
	}
    }
    else
    {
	ArrayNDIter iter( in_.info() );
	int index = 0, count = 0;

	do
	{
	    const T value = in_.getND( iter.getPos() );
	    index++;
	    if ( mIsUdf(value) )
		continue;

	    sum += value;
	    count++;
	    if ( !trend )
		continue;

	    sumindexes += index;
	    sumsqidx += index * index;
	    crosssum += value * (fT)index;
	} while ( iter.next() );

	iter.reset();
	if ( count <= 1 )
	    return false;

	if ( trend )
	    mComputeTrendAandB(count)
	else
	    avg = sum / (fT)count;

	index = 0;
	do
	{
	    const T inpval = in_.getND( iter.getPos() );
	    const T outval = mIsUdf(inpval) ? mUdf(T)
			   : (trend ? inpval - avg-(aval*(fT)index+bval)
				    : inpval - avg);
	    out_.setND(iter.getPos(), outval );
	    index++;
	} while ( iter.next() );
    }

    return true;
}


/*!\brief Removes the bias from an ArrayND. */

template <class T, class fT>
inline bool removeBias( ArrayND<T>& inout )
{
    return removeLinPart<T,fT>( inout, 0, false );
}

/*!\brief Fills an ArrayND with an unbiased version of another. */

template <class T, class fT>
inline bool removeBias( const ArrayND<T>& in, ArrayND<T>& out )
{
    return removeLinPart<T,fT>( in, &out, false );
}

/*!\brief Removes a linear trend from an ArrayND. */

template <class T, class fT>
inline bool removeTrend( ArrayND<T>& inout )
{
    return removeLinPart<T,fT>( inout, 0, true );
}

/*!\brief Fills an ArrayND with a de-trended version of another. */

template <class T, class fT>
inline bool removeTrend( const ArrayND<T>& in, ArrayND<T>& out )
{
    return removeLinPart<T,fT>( in, &out, true );
}


/*!\brief returns the average of all defined values in the Arrray1D.
   Returns UDF if empty or only udfs encountered. */

template <class T>
inline T getAverage( const ArrayND<T>& in )
{
    const int sz = in.info().getTotalSz();
    if ( sz < 1 )
	return mUdf(T);

    T sum = 0; int count = 0;
    for ( int idx=0; idx<sz; idx++ )
    {
	const T val = in.get( idx );
	if ( !mIsUdf(val) )
	    { sum += val; count++; }
    }

    if ( count == 0 )
	return mUdf(T);

    return sum / count;
}


/*!\brief Returns whether there are undefs in the Array1D.  */

template <class fT>
inline bool hasUndefs( const Array1D<fT>& in )
{
    const int sz = in.info().getSize(0);
    for ( int idx=0; idx<sz; idx++ )
    {
	const fT val = in.get( idx );
	if ( mIsUdf(val) )
	    return true;
    }

    return false;
}


/*!
   The function interpUdf fills all the undefined values in a Array1D
   by using an inter- or extrapolation from the defined values.
   It uses the BendPointBasedMathFunction for this.
   Note that even if there is only one defined value, this function will fill
   the entire array by this value.

   Returns whether any substitution was made.
*/

template <class fT>
inline bool interpUdf( Array1D<fT>& in,
	typename BendPointBasedMathFunction<fT,fT>::InterpolType ipoltyp=
			BendPointBasedMathFunction<fT,fT>::Poly )
{
    if ( !hasUndefs(in) )
	return false;

    BendPointBasedMathFunction<fT,fT> data( ipoltyp );
    const int sz = in.info().getSize(0);
    for ( int idx=0; idx<sz; idx++ )
    {
	const fT val = in.get( idx );
	if ( !mIsUdf(val) )
	    data.add( mCast(fT,idx), val );
    }

    for ( int idx=0; idx<sz; idx++ )
    {
	const fT val = in.get( idx );
	if ( mIsUdf(val) )
	    in.set( idx, data.getValue( mCast(fT,idx) ) );
    }

    return true;
}


/*!
\brief Tapers the N-dimentional ArrayND with a windowFunction.

  Usage is straightforward- construct and use. If apply()'s second argument is
  omitted, the result will be placed in the input array. apply() will return
  false if input-, output- and window-size are not equal.
  The only requirement on the windowfunction is that it should give full taper
  at x=+-1 and no taper when x=0. Feel free to implement more functions!!
*/

mExpClass(Algo) ArrayNDWindow
{
public:
    enum WindowType	{ Box, Hamming, Hanning, Blackman, Bartlett,
			  CosTaper5, CosTaper10, CosTaper20 };
			mDeclareEnumUtils(WindowType);

			ArrayNDWindow(const ArrayNDInfo&,bool rectangular,
				      WindowType=Hamming);
			ArrayNDWindow(const ArrayNDInfo&,bool rectangular,
				      const char* winnm,
				      float paramval=mUdf(float));
			~ArrayNDWindow();

    bool		isOK() const	{ return window_; }

    float		getParamVal() const { return paramval_; }
    float*		getValues() const { return window_; }

    void		setValue(int idx,float val) { window_[idx]=val; }
    bool		setType(WindowType);
    bool		setType(const char*,float paramval=mUdf(float));

    bool		resize(const ArrayNDInfo&);

    template <class Type> bool	apply(  ArrayND<Type>* in,
					ArrayND<Type>* out_=0 ) const
    {
	ArrayND<Type>* out = out_ ? out_ : in;

	if ( out_ && in->info() != out_->info() ) return false;
	if ( in->info() != size_ ) return false;

	const od_int64 totalsz = size_.getTotalSz();

	Type* indata = in->getData();
	Type* outdata = out->getData();
	if ( indata && outdata )
	{
	    for ( unsigned long idx = 0; idx<totalsz; idx++ )
	    {
		Type inval = indata[idx];
		outdata[idx] = mIsUdf( inval ) ? inval : inval * window_[idx];
	    }
	}
	else
	{
	    const ValueSeries<Type>* instorage = in->getStorage();
	    ValueSeries<Type>* outstorage = out->getStorage();

	    if ( instorage && outstorage )
	    {
		for ( unsigned long idx = 0; idx < totalsz; idx++ )
		{
		    Type inval = instorage->value(idx);
		    outstorage->setValue(idx,
			    mIsUdf( inval ) ? inval : inval * window_[idx] );
		}
	    }
	    else
	    {
		ArrayNDIter iter( size_ );
		int idx = 0;
		do
		{
		    Type inval = in->getND(iter.getPos());
		    out->setND( iter.getPos(),
			     mIsUdf( inval ) ? inval : inval * window_[idx] );
		    idx++;

		} while ( iter.next() );
	    }
	}

	return true;
    }

protected:

    float*			window_;
    ArrayNDInfoImpl		size_;
    bool			rectangular_;

    BufferString		windowtypename_;
    float			paramval_;

    bool			buildWindow(const char* winnm,float pval);
};


template<class T>
inline T Array3DInterpolate( const Array3D<T>& array,
			     float p0, float p1, float p2,
			     bool posperiodic = false )
{
    const Array3DInfo& size = array.info();
    int intpos0 = mNINT32( p0 );
    float dist0 = p0 - intpos0;
    int prevpos0 = intpos0;
    if ( dist0 < 0 )
    {
	prevpos0--;
	dist0++;
    }
    if ( posperiodic ) prevpos0 = dePeriodize( prevpos0, size.getSize(0) );

    int intpos1 = mNINT32( p1 );
    float dist1 = p1 - intpos1;
    int prevpos1 = intpos1;
    if ( dist1 < 0 )
    {
	prevpos1--;
	dist1++;
    }
    if ( posperiodic ) prevpos1 = dePeriodize( prevpos1, size.getSize(1) );

    int intpos2 = mNINT32( p2 );
    float dist2 = p2 - intpos2;
    int prevpos2 = intpos2;
    if ( dist2 < 0 )
    {
	prevpos2--;
	dist2++;
    }

    if ( posperiodic ) prevpos2 = dePeriodize( prevpos2, size.getSize(2) );

    if ( !posperiodic && ( prevpos0 < 0 || prevpos0 > size.getSize(0) -2 ||
			 prevpos1 < 0 || prevpos1 > size.getSize(1) -2 ||
			 prevpos2 < 0 || prevpos2 > size.getSize(2) -2 ))
	return mUdf(T);

    if ( !posperiodic && ( !prevpos0 || prevpos0 > size.getSize(0) -3 ||
			 !prevpos1 || prevpos1 > size.getSize(1) -3 ||
			 !prevpos2 || prevpos2 > size.getSize(2) -3 ))
    {
	return linearInterpolate3D(
            array.get( prevpos0  , prevpos1  , prevpos2  ),
	    array.get( prevpos0  , prevpos1  , prevpos2+1),
	    array.get( prevpos0  , prevpos1+1, prevpos2  ),
            array.get( prevpos0  , prevpos1+1, prevpos2+1),
            array.get( prevpos0+1, prevpos1  , prevpos2  ),
            array.get( prevpos0+1, prevpos1  , prevpos2+1),
            array.get( prevpos0+1, prevpos1+1, prevpos2  ),
            array.get( prevpos0+1, prevpos1+1, prevpos2+1),
	    dist0, dist1, dist2 );
    }

    int firstpos0 = prevpos0 - 1;
    int nextpos0 = prevpos0 + 1;
    int lastpos0 = prevpos0 + 2;

    if ( posperiodic ) firstpos0 = dePeriodize( firstpos0, size.getSize(0) );
    if ( posperiodic ) nextpos0 = dePeriodize( nextpos0, size.getSize(0) );
    if ( posperiodic ) lastpos0 = dePeriodize( lastpos0, size.getSize(0) );

    int firstpos1 = prevpos1 - 1;
    int nextpos1 = prevpos1 + 1;
    int lastpos1 = prevpos1 + 2;

    if ( posperiodic ) firstpos1 = dePeriodize( firstpos1, size.getSize(1) );
    if ( posperiodic ) nextpos1 = dePeriodize( nextpos1, size.getSize(1) );
    if ( posperiodic ) lastpos1 = dePeriodize( lastpos1, size.getSize(1) );

    int firstpos2 = prevpos2 - 1;
    int nextpos2 = prevpos2 + 1;
    int lastpos2 = prevpos2 + 2;

    if ( posperiodic ) firstpos2 = dePeriodize( firstpos2, size.getSize(2) );
    if ( posperiodic ) nextpos2 = dePeriodize( nextpos2, size.getSize(2) );
    if ( posperiodic ) lastpos2 = dePeriodize( lastpos2, size.getSize(2) );

    return polyInterpolate3D (
            array.get( firstpos0  , firstpos1  , firstpos2 ),
            array.get( firstpos0  , firstpos1  , prevpos2  ),
            array.get( firstpos0  , firstpos1  , nextpos2  ),
            array.get( firstpos0  , firstpos1  , lastpos2  ),

            array.get( firstpos0  , prevpos1   , firstpos2 ),
            array.get( firstpos0  , prevpos1   , prevpos2  ),
            array.get( firstpos0  , prevpos1   , nextpos2  ),
            array.get( firstpos0  , prevpos1   , lastpos2  ),

            array.get( firstpos0  , nextpos1   , firstpos2 ),
            array.get( firstpos0  , nextpos1   , prevpos2  ),
            array.get( firstpos0  , nextpos1   , nextpos2  ),
            array.get( firstpos0  , nextpos1   , lastpos2  ),

            array.get( firstpos0  , lastpos1   , firstpos2 ),
            array.get( firstpos0  , lastpos1   , prevpos2  ),
            array.get( firstpos0  , lastpos1   , nextpos2  ),
            array.get( firstpos0  , lastpos1   , lastpos2  ),


            array.get( prevpos0  , firstpos1  , firstpos2 ),
            array.get( prevpos0  , firstpos1  , prevpos2  ),
            array.get( prevpos0  , firstpos1  , nextpos2  ),
            array.get( prevpos0  , firstpos1  , lastpos2  ),

            array.get( prevpos0  , prevpos1   , firstpos2 ),
            array.get( prevpos0  , prevpos1   , prevpos2  ),
            array.get( prevpos0  , prevpos1   , nextpos2  ),
            array.get( prevpos0  , prevpos1   , lastpos2  ),

            array.get( prevpos0  , nextpos1   , firstpos2 ),
            array.get( prevpos0  , nextpos1   , prevpos2  ),
            array.get( prevpos0  , nextpos1   , nextpos2  ),
            array.get( prevpos0  , nextpos1   , lastpos2  ),

            array.get( prevpos0  , lastpos1   , firstpos2 ),
            array.get( prevpos0  , lastpos1   , prevpos2  ),
            array.get( prevpos0  , lastpos1   , nextpos2  ),
            array.get( prevpos0  , lastpos1   , lastpos2  ),


            array.get( nextpos0  , firstpos1  , firstpos2 ),
            array.get( nextpos0  , firstpos1  , prevpos2  ),
            array.get( nextpos0  , firstpos1  , nextpos2  ),
            array.get( nextpos0  , firstpos1  , lastpos2  ),

            array.get( nextpos0  , prevpos1   , firstpos2 ),
            array.get( nextpos0  , prevpos1   , prevpos2  ),
            array.get( nextpos0  , prevpos1   , nextpos2  ),
            array.get( nextpos0  , prevpos1   , lastpos2  ),

            array.get( nextpos0  , nextpos1   , firstpos2 ),
            array.get( nextpos0  , nextpos1   , prevpos2  ),
            array.get( nextpos0  , nextpos1   , nextpos2  ),
            array.get( nextpos0  , nextpos1   , lastpos2  ),

            array.get( nextpos0  , lastpos1   , firstpos2 ),
            array.get( nextpos0  , lastpos1   , prevpos2  ),
            array.get( nextpos0  , lastpos1   , nextpos2  ),
            array.get( nextpos0  , lastpos1   , lastpos2  ),


            array.get( lastpos0  , firstpos1  , firstpos2 ),
            array.get( lastpos0  , firstpos1  , prevpos2  ),
            array.get( lastpos0  , firstpos1  , nextpos2  ),
            array.get( lastpos0  , firstpos1  , lastpos2  ),

            array.get( lastpos0  , prevpos1   , firstpos2 ),
            array.get( lastpos0  , prevpos1   , prevpos2  ),
            array.get( lastpos0  , prevpos1   , nextpos2  ),
            array.get( lastpos0  , prevpos1   , lastpos2  ),

            array.get( lastpos0  , nextpos1   , firstpos2 ),
            array.get( lastpos0  , nextpos1   , prevpos2  ),
            array.get( lastpos0  , nextpos1   , nextpos2  ),
            array.get( lastpos0  , nextpos1   , lastpos2  ),

            array.get( lastpos0  , lastpos1   , firstpos2 ),
            array.get( lastpos0  , lastpos1   , prevpos2  ),
            array.get( lastpos0  , lastpos1   , nextpos2  ),
            array.get( lastpos0  , lastpos1   , lastpos2  ),
	    dist0, dist1, dist2 );
}


template <class T>
inline bool ArrayNDCopy( ArrayND<T>& dest, const ArrayND<T>& src,
			 const TypeSet<int>& copypos, bool srcperiodic=false )
{
    const ArrayNDInfo& destsz = dest.info();
    const ArrayNDInfo& srcsz = src.info();

    const int ndim = destsz.getNDim();
    if ( ndim != srcsz.getNDim() || ndim != copypos.size() ) return false;

    for ( int idx=0; idx<ndim; idx++ )
    {
	if ( !srcperiodic &&
	     copypos[idx] + destsz.getSize(idx) > srcsz.getSize(idx) )
	    return false;
    }

    ArrayNDIter destposition( destsz );
    TypeSet<int> srcposition( ndim, 0 );

    do
    {
	for ( int idx=0; idx<ndim; idx++ )
	{
	    srcposition[idx] = copypos[idx] + destposition[idx];
	    if ( srcperiodic )
		srcposition[idx] =
		    dePeriodize( srcposition[idx], srcsz.getSize(idx) );
	}

	dest.setND( destposition.getPos(), src.get( &srcposition[0] ));


    } while ( destposition.next() );

    return true;
}


template <class T>
inline bool Array3DCopy( Array3D<T>& dest, const Array3D<T>& src,
			 int p0, int p1, int p2, bool srcperiodic=false )
{
    const ArrayNDInfo& destsz = dest.info();
    const ArrayNDInfo& srcsz = src.info();

    const int destsz0 = destsz.getSize(0);
    const int destsz1 = destsz.getSize(1);
    const int destsz2 = destsz.getSize(2);

    const int srcsz0 = srcsz.getSize(0);
    const int srcsz1 = srcsz.getSize(1);
    const int srcsz2 = srcsz.getSize(2);

    if ( !srcperiodic )
    {
	if ( p0 + destsz0 > srcsz0 ||
	     p1 + destsz1 > srcsz1 ||
	     p2 + destsz2 > srcsz2  )
	     return false;
    }

    int idx = 0;
    T* ptr = dest.getData();

    for ( int id0=0; id0<destsz0; id0++ )
    {
	for ( int id1=0; id1<destsz1; id1++ )
	{
	    for ( int id2=0; id2<destsz2; id2++ )
	    {
		ptr[idx++] = src.get( dePeriodize(id0 + p0, srcsz0),
					 dePeriodize(id1 + p1, srcsz1),
					 dePeriodize(id2 + p2, srcsz2));

	    }
	}
    }

    return true;
}


template <class T>
inline bool ArrayNDPaste( ArrayND<T>& dest, const ArrayND<T>& src,
			  const TypeSet<int>& pastepos,
			  bool destperiodic=false )
{
    const ArrayNDInfo& destsz = dest.info();
    const ArrayNDInfo& srcsz = src.info();

    const int ndim = destsz.getNDim();
    if ( ndim != srcsz.getNDim() || ndim != pastepos.size() ) return false;

    for ( int idx=0; idx<ndim; idx++ )
    {
	if ( !destperiodic &&
	     pastepos[idx] + srcsz.getSize(idx) > destsz.getSize(idx) )
	    return false;
    }

    ArrayNDIter srcposition( srcsz );
    TypeSet<int> destposition( ndim, 0 );

    int ptrpos = 0;
    T* ptr = src.getData();

    do
    {
	for ( int idx=0; idx<ndim; idx++ )
	{
	    destposition[idx] = pastepos[idx] + srcposition[idx];
	    if ( destperiodic )
		destposition[idx] =
		    dePeriodize( destposition[idx], destsz.getSize(idx) );
	}

	dest( destposition ) =  ptr[ptrpos++];

    } while ( srcposition.next() );

    return true;
}


template <class T>
inline bool Array2DPaste( Array2D<T>& dest, const Array2D<T>& src,
			  int p0, int p1, bool destperiodic=false )
{
    const ArrayNDInfo& destsz = dest.info();
    const ArrayNDInfo& srcsz = src.info();

    const int srcsz0 = srcsz.getSize(0);
    const int srcsz1 = srcsz.getSize(1);

    const int destsz0 = destsz.getSize(0);
    const int destsz1 = destsz.getSize(1);

    if ( !destperiodic )
    {
	if ( p0 + srcsz0 > destsz0 ||
	     p1 + srcsz1 > destsz1  )
	     return false;
    }


    int idx = 0;
    const T* ptr = src.getData();

    for ( int id0=0; id0<srcsz0; id0++ )
    {
	for ( int id1=0; id1<srcsz1; id1++ )
	{
	    dest.set( dePeriodize( id0 + p0, destsz0),
		  dePeriodize( id1 + p1, destsz1),
		  ptr[idx++]);
	}
    }

    return true;
}


template <class T>
inline bool Array3DPaste( Array3D<T>& dest, const Array3D<T>& src,
			  int p0, int p1, int p2,
			  bool destperiodic=false )
{
    const ArrayNDInfo& destsz = dest.info();
    const ArrayNDInfo& srcsz = src.info();

    const int srcsz0 = srcsz.getSize(0);
    const int srcsz1 = srcsz.getSize(1);
    const int srcsz2 = srcsz.getSize(2);

    const int destsz0 = destsz.getSize(0);
    const int destsz1 = destsz.getSize(1);
    const int destsz2 = destsz.getSize(2);

    if ( !destperiodic )
    {
	if ( p0 + srcsz0 > destsz0 ||
	     p1 + srcsz1 > destsz1 ||
	     p2 + srcsz2 > destsz2  )
	     return false;
    }


    int idx = 0;
    const T* ptr = src.getData();

    for ( int id0=0; id0<srcsz0; id0++ )
    {
	for ( int id1=0; id1<srcsz1; id1++ )
	{
	    for ( int id2=0; id2<srcsz2; id2++ )
	    {
		dest.set( dePeriodize( id0 + p0, destsz0),
		      dePeriodize( id1 + p1, destsz1),
		      dePeriodize( id2 + p2, destsz2), ptr[idx++]);
	    }
	}
    }

    return true;
}



/*!
  \brief Polynomial trend with order 0 (mean), 1 (linear) or 2 (parabolic)
  The trend is derived from a set of values with positions
  and can be applied thereafter on any other position
*/

mExpClass(Algo) PolyTrend
{
public:
				PolyTrend();

    bool			operator==(const PolyTrend&) const;

				enum Order	{ None, Order0, Order1, Order2};
				mDeclareEnumUtils(Order)

    static const char*		sKeyOrder()	{ return "Polynomial Order"; }

    void			setOrder( PolyTrend::Order t )	{ order_ = t; }
    template <class IDXABLE> bool	set(const TypeSet<Coord>&,
					    const IDXABLE& valuelistj);
				/*!< Use after the order is set!
				     Sets the trend from a list of values
				     tied to a list of coordinates */

    Order			getOrder() const	{ return order_; }

    template <class T> void	apply(const Coord& pos,bool dir,T&) const;
				/*!<Applies the trend to a value tied to
				    a position */
				/*!<\param dir: true for detrend,
						false for restore */

protected:

    Order			order_;
    double			f0_;
    double			f1_;
    double			f2_;
    double			f11_;
    double			f12_;
    double			f22_;
    Coord			posc_;

    void			initOrder0(const TypeSet<double>&);
    void			initOrder1(const TypeSet<Coord>&,
					   const TypeSet<double>&);
    void			initOrder2(const TypeSet<Coord>&,
					   const TypeSet<double>&);
    void			initCenter(const TypeSet<Coord>&);

};



template <class IDXABLE> inline
bool PolyTrend::set( const TypeSet<Coord>& poslist, const IDXABLE& vals )
{
    int sz = poslist.size();
    if ( order_ == PolyTrend::None )
	return false;

    f0_ = f1_ = f2_ = f11_ = f12_ = f22_ = posc_.x = posc_.y = 0.;
    TypeSet<Coord> posnoudf;
    TypeSet<double> valsnoudf;
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( !poslist[idx].isDefined() || mIsUdf(vals[idx]) )
	    continue;

	posnoudf += poslist[idx];
	valsnoudf += (double) vals[idx];
    }

    sz = valsnoudf.size();
    if ( order_ == Order2 && sz > 5 )
	initOrder2( posnoudf, valsnoudf );
    else if ( order_ == Order1 && sz > 2 )
	initOrder1( posnoudf, valsnoudf );
    else
	initOrder0( valsnoudf );

    return true;
}


template <class T> inline
void PolyTrend::apply( const Coord& pos, bool dir, T& val ) const
{
    if ( order_ == None || !pos.isDefined() || mIsUdf(val) )
	return;

    const double fact = dir ? -1. : 1;
    double inp = (double) val;
    inp += fact * f0_;
    if ( order_ == Order0 )
    {
	val = (T)inp;
	return;
    }

    const double dx = pos.x - posc_.x;
    const double dy = pos.y - posc_.y;
    inp += fact * ( f1_ * dx + f2_ * dy );
    if ( order_ == Order1 )
    {
	val = (T)inp;
	return;
    }

    const double dx2 = dx * dx;
    const double dxy = dx * dy;
    const double dyy = dy * dy;
    inp += fact * ( f11_ * dx2 + f12_ * dxy + f22_ * dyy );
    val = (T)inp;
}

#endif

