#ifndef arrayndutils_h
#define arrayndutils_h

/*@+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: arrayndutils.h,v 1.3 2000-06-29 10:20:23 bert Exp $
________________________________________________________________________


@$*/
#include <arraynd.h>
#include <enums.h>
#include <databuf.h>
#include <arrayndimpl.h>
#include <mathfunc.h>
#include <math.h>

/*
removeBias( ) - removes the DC component from an ArrayND. If no output is
		given, removeBias( ) will store the result in the input
		ArrayND.
*/


template <class T>
inline bool removeBias( ArrayND<T>* in, ArrayND<T>* out_ = 0)
{
    ArrayND<T>* out = out_ ? out_ : in; 

    T avg = 0;

    if ( out_ && in->size() != out_->size() ) return false;

    const int sz = in->size().getTotalSz();

    T* inpptr = in->getData();

    for ( int idx=0; idx<sz; idx++ )
	avg += inpptr[idx]; 

    in->unlockData();

    avg /= sz;

    T* outptr = out->getData();

    for ( int idx=0; idx<sz; idx++ )
	outptr[idx] = inpptr[idx] - avg;

    out->dataUpdated();
    out->unlockData();

    return true;
}

/* ArrayNDIter is an object that is able to iterate through all samples in a 
   ArrayND. It will stand on the first position when initiated, and move to
   the second at the fist call to next(). next() will return false when
   no more positions are avaliable
*/

class ArrayNDIter
{
public:
				ArrayNDIter( const ArrayNDSize& );

    bool			next();

    const TypeSet<int>&		getPos() const { return position; }
    int				operator[](int) const;

protected:
    bool			inc(int);

    TypeSet<int>		position;
    const ArrayNDSize&		sz;
};

/* ArrayNDWindow will taper the N-dimentional ArrayND with a windowFunction.
   Usage is straightforwar- construct and use. If apply()'s second argument is
   omitted, the result will be placed in the input array. apply() will return
   false if input-, output- and window-size are not equal.
   The only requirement on the windowfunction is that it should give full taper
   at x=+-1 and no taper when x=0. Feel free to implement more functions!!
*/

class ArrayNDWindow
{
public:
    enum WindowType	{ Box, Hamming };
			DeclareEnumUtils(WindowType);

			ArrayNDWindow( const ArrayNDSize&,
					ArrayNDWindow::WindowType = Hamming );

			~ArrayNDWindow();

    bool		setType( ArrayNDWindow::WindowType );
    bool		setType( const char* );

    bool		resize( const ArrayNDSize& );

    template <class Type> bool	apply(  ArrayND<Type>* in,
					ArrayND<Type>* out=0) const;

protected:

    DataBuffer*			window;
    ArrayNDSizeImpl		size;
    WindowType			type;

    bool			buildWindow( );

    class BoxWindow : public MathFunction<float>
    {
    public:
	float	getValue( double x ) const
		{ return fabs(x) > 1 ? 0 : 1; }
    };

    class HammingWindow : public MathFunction<float>
    {
    public:
	float	getValue( double x ) const
		{
		    double rx = fabs( x );
		    if ( rx > 1 )
			return 0;

		    return 0.54 + 0.46 * cos( M_PI * rx );
		}
    };
};
   

template <class Type>
inline bool ArrayNDWindow::apply( ArrayND<Type>* in, ArrayND<Type>* out_) const
{
    ArrayND<Type>* out = out_ ? out_ : in; 

    if ( out_ && in->size() != out_->size() ) return false;

    if ( in->size() != size) return false;

    unsigned long totalSz = size.getTotalSz();

    Type* inData = in->getData();
    Type* outData = out->getData();

    int bytesPerSample = window->bytesPerSample();

    for(unsigned long idx = 0; idx < totalSz; idx++)
        outData[idx] = inData[idx] *
                *((float*)(window->data+bytesPerSample*idx ));

    out->dataUpdated();
    out->unlockData();
    in->unlockData();

    return true;
}
 
template<class T>
inline T Array3DInterpolate( const Array3D<T>& array,
		      float p0, float p1, float p2,
		      bool posperiodic = false )
{
    const Array3DSize& size = array.size();

    int intpos0 = mNINT( p0 );
    float dist0 = p0 - intpos0;
    int prevpos0 = intpos0;
    if ( dist0 < 0 )
    {
	prevpos0--;
	dist0++;
    }
    if ( posperiodic ) prevpos0 = dePeriodize( prevpos0, size.getSize(0) );

    int intpos1 = mNINT( p1 );
    float dist1 = p1 - intpos1;
    int prevpos1 = intpos1;
    if ( dist1 < 0 )
    {
	prevpos1--;
	dist1++;
    }
    if ( posperiodic ) prevpos1 = dePeriodize( prevpos1, size.getSize(1) );

    int intpos2 = mNINT( p2 );
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
    {
	return mUndefValue;
    }

    if ( !posperiodic && ( !prevpos0 || prevpos0 > size.getSize(0) -3 ||
			 !prevpos1 || prevpos1 > size.getSize(1) -3 ||
			 !prevpos2 || prevpos2 > size.getSize(2) -3 ))
    {
	return linearInterpolate3D(
            array.getVal( prevpos0  , prevpos1  , prevpos2  ),
	    array.getVal( prevpos0  , prevpos1  , prevpos2+1),
	    array.getVal( prevpos0  , prevpos1+1, prevpos2  ),
            array.getVal( prevpos0  , prevpos1+1, prevpos2+1),
            array.getVal( prevpos0+1, prevpos1  , prevpos2  ),
            array.getVal( prevpos0+1, prevpos1  , prevpos2+1),
            array.getVal( prevpos0+1, prevpos1+1, prevpos2  ),
            array.getVal( prevpos0+1, prevpos1+1, prevpos2+1),
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
            array.getVal( firstpos0  , firstpos1  , firstpos2 ),
            array.getVal( firstpos0  , firstpos1  , prevpos2  ),
            array.getVal( firstpos0  , firstpos1  , nextpos2  ),
            array.getVal( firstpos0  , firstpos1  , lastpos2  ),

            array.getVal( firstpos0  , prevpos1   , firstpos2 ),
            array.getVal( firstpos0  , prevpos1   , prevpos2  ),
            array.getVal( firstpos0  , prevpos1   , nextpos2  ),
            array.getVal( firstpos0  , prevpos1   , lastpos2  ),

            array.getVal( firstpos0  , nextpos1   , firstpos2 ),
            array.getVal( firstpos0  , nextpos1   , prevpos2  ),
            array.getVal( firstpos0  , nextpos1   , nextpos2  ),
            array.getVal( firstpos0  , nextpos1   , lastpos2  ),

            array.getVal( firstpos0  , lastpos1   , firstpos2 ),
            array.getVal( firstpos0  , lastpos1   , prevpos2  ),
            array.getVal( firstpos0  , lastpos1   , nextpos2  ),
            array.getVal( firstpos0  , lastpos1   , lastpos2  ),


            array.getVal( prevpos0  , firstpos1  , firstpos2 ),
            array.getVal( prevpos0  , firstpos1  , prevpos2  ),
            array.getVal( prevpos0  , firstpos1  , nextpos2  ),
            array.getVal( prevpos0  , firstpos1  , lastpos2  ),

            array.getVal( prevpos0  , prevpos1   , firstpos2 ),
            array.getVal( prevpos0  , prevpos1   , prevpos2  ),
            array.getVal( prevpos0  , prevpos1   , nextpos2  ),
            array.getVal( prevpos0  , prevpos1   , lastpos2  ),

            array.getVal( prevpos0  , nextpos1   , firstpos2 ),
            array.getVal( prevpos0  , nextpos1   , prevpos2  ),
            array.getVal( prevpos0  , nextpos1   , nextpos2  ),
            array.getVal( prevpos0  , nextpos1   , lastpos2  ),

            array.getVal( prevpos0  , lastpos1   , firstpos2 ),
            array.getVal( prevpos0  , lastpos1   , prevpos2  ),
            array.getVal( prevpos0  , lastpos1   , nextpos2  ),
            array.getVal( prevpos0  , lastpos1   , lastpos2  ),


            array.getVal( nextpos0  , firstpos1  , firstpos2 ),
            array.getVal( nextpos0  , firstpos1  , prevpos2  ),
            array.getVal( nextpos0  , firstpos1  , nextpos2  ),
            array.getVal( nextpos0  , firstpos1  , lastpos2  ),

            array.getVal( nextpos0  , prevpos1   , firstpos2 ),
            array.getVal( nextpos0  , prevpos1   , prevpos2  ),
            array.getVal( nextpos0  , prevpos1   , nextpos2  ),
            array.getVal( nextpos0  , prevpos1   , lastpos2  ),

            array.getVal( nextpos0  , nextpos1   , firstpos2 ),
            array.getVal( nextpos0  , nextpos1   , prevpos2  ),
            array.getVal( nextpos0  , nextpos1   , nextpos2  ),
            array.getVal( nextpos0  , nextpos1   , lastpos2  ),

            array.getVal( nextpos0  , lastpos1   , firstpos2 ),
            array.getVal( nextpos0  , lastpos1   , prevpos2  ),
            array.getVal( nextpos0  , lastpos1   , nextpos2  ),
            array.getVal( nextpos0  , lastpos1   , lastpos2  ),


            array.getVal( lastpos0  , firstpos1  , firstpos2 ),
            array.getVal( lastpos0  , firstpos1  , prevpos2  ),
            array.getVal( lastpos0  , firstpos1  , nextpos2  ),
            array.getVal( lastpos0  , firstpos1  , lastpos2  ),

            array.getVal( lastpos0  , prevpos1   , firstpos2 ),
            array.getVal( lastpos0  , prevpos1   , prevpos2  ),
            array.getVal( lastpos0  , prevpos1   , nextpos2  ),
            array.getVal( lastpos0  , prevpos1   , lastpos2  ),

            array.getVal( lastpos0  , nextpos1   , firstpos2 ),
            array.getVal( lastpos0  , nextpos1   , prevpos2  ),
            array.getVal( lastpos0  , nextpos1   , nextpos2  ),
            array.getVal( lastpos0  , nextpos1   , lastpos2  ),

            array.getVal( lastpos0  , lastpos1   , firstpos2 ),
            array.getVal( lastpos0  , lastpos1   , prevpos2  ),
            array.getVal( lastpos0  , lastpos1   , nextpos2  ),
            array.getVal( lastpos0  , lastpos1   , lastpos2  ),
	    dist0, dist1, dist2 );
} 


template <class T>
inline bool ArrayNDCopy( ArrayND<T>& dest, const ArrayND<T>& src,
		   const TypeSet<int>& copypos,
		   bool srcperiodic=false )
{
    const ArrayNDSize& destsz = dest.size(); 
    const ArrayNDSize& srcsz = src.size(); 

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

	dest.setVal( destposition.getPos(), src.getVal( srcposition ) );
		
    } while ( destposition.next() );

    return true;
}    


template <class T>
inline bool Array3DCopy( Array3D<T>& dest, const Array3D<T>& src,
		   int p0, int p1, int p2,
		   bool srcperiodic=false )
{
    const ArrayNDSize& destsz = dest.size(); 
    const ArrayNDSize& srcsz = src.size(); 

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
		ptr[idx++] = src.getVal( dePeriodize(id0 + p0, srcsz0),
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
    const ArrayNDSize& destsz = dest.size(); 
    const ArrayNDSize& srcsz = src.size(); 

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

	dest.setVal( destposition, ptr[ptrpos++] );
		
    } while ( srcposition.next() );

    return true;
}    


template <class T>
inline bool Array3DPaste( Array3D<T>& dest, const Array3D<T>& src,
		   int p0, int p1, int p2,
		   bool destperiodic=false )
{
    const ArrayNDSize& destsz = dest.size(); 
    const ArrayNDSize& srcsz = src.size(); 

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
    T* ptr = src.getData();

    for ( int id0=0; id0<srcsz0; id0++ )
    {
	for ( int id1=0; id1<srcsz1; id1++ )
	{
	    for ( int id2=0; id2<srcsz2; id2++ )
	    {
		dest.setVal( dePeriodize( id0 + p0, destsz0),
			     dePeriodize( id1 + p1, destsz1),
			     dePeriodize( id2 + p2, destsz2), ptr[idx++] );
	    }
	}
    }

    return true;
}
#endif
