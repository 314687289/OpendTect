#ifndef simpnumer_H
#define simpnumer_H

/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Bert BRil & Kris Tingdahl
 Date:		12-4-1999
 Contents:	'Simple' numerical functions
 RCS:		$Id: simpnumer.h,v 1.15 2003-08-28 11:14:01 bert Exp $
________________________________________________________________________

*/

#include <math.h>

#ifndef M_PI
# define M_PI           3.14159265358979323846  /* pi */
#endif

#ifndef M_PIl
# define M_PIl          3.1415926535897932384626433832795029L
#endif


/*!>
 intpow returns the integer power of an arbitary value. Faster than
 pow( double, double ), more general than IntPowerOf(double,int).
*/

template <class T> inline
T intpow( T x, char y)
{
    T res = 1; while ( y ) { res *= x; y--; }
    return res;
}


/*!>
 isPower determines whether a value is a power of a base, i.e. if
val=base^pow.
 If that is the case, isPower returns pow, if not, it returns zero.
*/


inline
int isPower( int val, int base )
{
    if ( val==base ) return 1;

    if ( val%base )
	return 0;

    int res = isPower( val/base, base );
    if ( res ) return 1 + res;

    return 0;
}


/*!>
 Taper an indexable array from 1 to taperfactor. If lowpos is less 
 than highpos, the samples array[0] to array[lowpos] will be set to zero. 
 If lowpos is more than highpos, the samples array[lowpos]  to array[sz-1]
 will be set to zero. The taper can be either cosine or linear.
*/

class Taper
{
public:
    enum Type { Cosine, Linear };
};


template <class T>
bool taperArray( T* array, int sz, int lowpos, int highpos, Taper::Type type )
{
    if ( lowpos  >= sz || lowpos  < 0 ||
         highpos >= sz || highpos < 0 ||
	 highpos == lowpos )
	return NO;

    int pos = lowpos < highpos ? 0 : sz - 1 ;
    int inc = lowpos < highpos ? 1 : -1 ;

    while ( pos != lowpos )
    {
	array[pos] = 0;
	pos += inc;
    }

    int tapersz = abs( highpos - lowpos ); 
    float taperfactor = type == Taper::Cosine ? M_PI : 0;
    float taperfactorinc = ( type == Taper::Cosine ? M_PI : 1 ) / tapersz;
				

    while ( pos != highpos )
    {
	array[pos] *= type == Taper::Cosine ? .5 + .5 * cos( taperfactor ) 
					    : taperfactor;
	pos += inc;
	taperfactor += taperfactorinc;
    }

    return YES;
}


/*!>
 Gradient from a series of 4 points, sampled like:
 x:  -2  -1  0  1  2
 y: ym2 ym1 y0 y1 y2
 The gradient estimation is done at x=0. y0 is generally not needed, but it
 will be used if there are one or more undefineds.
 The function will return mUndefValue if there are too many missing values.
*/
template <class T>
inline T sampledGradient( T ym2, T ym1, T y0_, T y1_, T y2 )
{
    bool um1 = mIsUndefined(ym1), u0 = mIsUndefined(y0_), u1 = mIsUndefined(y1_);

    if ( mIsUndefined(ym2) || mIsUndefined(y2) )
    {
        if ( um1 || u1 )
	    { if ( !u0 && !(um1 && u1) ) return um1 ? y1_ - y0_ : y0_ - ym1; }
        else
            return (y1_-ym1) * .5;
    }
    else if ( (um1 && u1) || (u0 && (um1 || u1)) )
        return (y2-ym2) * .25;
    else if ( um1 || u1 )
    {
        return um1 ? ((16*y1_)   - 3*y2  - ym2) / 12 - y0_
                   : ((-16*ym1) + 3*ym2 + y2)  / 12 + y0_;
    }
    else
        return (8 * ( y1_ - ym1 ) - y2 + ym2) / 12;

    return mUndefValue;
}


template <class X, class Y, class RT>
inline void getGradient( const X& x, const Y& y, int sz, int firstx, int firsty,
		  RT* gradptr=0, RT* interceptptr=0 )
{
    RT xy_sum = 0, x_sum=0, y_sum=0, xx_sum=0;

    for ( int idx=0; idx<sz; idx++ )
    {
	RT xval = x[idx+firstx];
	RT yval = y[idx+firsty];

	x_sum += xval;
	y_sum += yval;
	xx_sum += xval*xval;
	xy_sum += xval*yval;
    }

    RT grad = ( sz*xy_sum - x_sum*y_sum ) / ( sz*xx_sum - x_sum*x_sum );
    if ( gradptr ) *gradptr = grad;

    if ( interceptptr ) *interceptptr = (y_sum - grad*x_sum)/sz;
}


template <class X>
inline float variance( const X& x, int sz )
{
    if ( sz < 2 ) return mUndefValue;

    float sum=0;
    float sqsum=0;

    for ( int idx=0; idx<sz; idx++ )
    {
	float val = x[idx];

	sum += val;
	sqsum += val*val;
    }

    return (sqsum - sum * sum / sz)/ (sz -1);
}


/*!>
solve3DPoly - finds the roots of the equation 

    z^3+a*z^2+b*z+c = 0

solve3DPoly returns the number of real roots found.

Algorithms taken from NR, page 185.

*/

inline int solve3DPoly( double a, double b, double c, 
			 double& root0,
			 double& root1,
			 double& root2 )
{
    const double a2 = a*a;
    const double q = (a2-3*b)/9;
    const double r = (2*a2*a-9*a*b+27*c)/54;
    const double q3 = q*q*q;
    const double r2 = r*r;
    

    const double minus_a_through_3 = -a/3;

    if ( r2<q3 )
    {
	const double minus_twosqrt_q = -2*sqrt(q);
	const double theta = acos(r/sqrt(q3));
	static const double twopi = 2*M_PI;


	root0 = minus_twosqrt_q*cos(theta/3)+minus_a_through_3; 
	root1=minus_twosqrt_q*cos((theta-twopi)/3)+minus_a_through_3;
	root2=minus_twosqrt_q*cos((theta+twopi)/3)+minus_a_through_3;
	return 3;
    }

    const double A=(r>0?-1:1)*pow(fabs(r)+sqrt(r2-q3),1/3);
    const double B=mIS_ZERO(A)?0:q/A;

    root0 = A+B+minus_a_through_3;

    /*!
    The complex roots can be calculated as follows:
    static const double sqrt3_through_2 = sqrt(3)/2;

    root1 = complex_double( 0.5*(A+B)+minus_a_through_3,
			   sqrt3_through_2*(A-B));

    root2 = complex_double( 0.5*(A+B)+minus_a_through_3,
			   -sqrt3_through_2*(A-B));
    */
			     

    return 1;
}


inline double deg2rad( int deg )
{
    static double deg2radconst = M_PI / 180;
    return deg * deg2radconst;
}


inline float deg2rad( float deg )
{
    static float deg2radconst = M_PI / 180;
    return deg * deg2radconst;
}


inline double deg2rad( double deg )
{
    static double deg2radconst = M_PI / 180;
    return deg * deg2radconst;
}


inline long double deg2rad( long double deg )
{
    static long double deg2radconst = M_PIl / 180;
    return deg * deg2radconst;
}


inline float rad2deg( float rad )
{
    static float rad2degconst = 180/M_PI;
    return rad * rad2degconst;
}


inline double rad2deg( double rad )
{
    static double rad2degconst = 180/M_PI;
    return rad * rad2degconst;
}


inline long double rad2deg( long double rad )
{
    static long double rad2degconst = 180/M_PIl;
    return rad * rad2degconst;
}



#endif
