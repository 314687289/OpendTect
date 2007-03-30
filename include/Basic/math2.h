#ifndef math2_H
#define math2_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Aug 2005
 RCS:		$Id: math2.h,v 1.3 2007-03-30 15:16:09 cvskris Exp $
________________________________________________________________________

-*/

#ifndef gendefs_H
#include "gendefs.h"
#endif

#ifdef __cpp__
extern "C" {
#endif

int		IsNormalNumber(double);
		/* Returns 0 for for infinite, NaN, and that sort of crap */

double		IntPowerOf(double,int);
double		PowerOf(double,double);
		/*!< PowerOf(-2,2) returns -4. This may be mathematically
		  incorrect, it delivers continuity with negative numbers */

double		ASin(double);
		/*!<Checks the input range before calling asin, and does thus
		    avoid nan's due to roundoff errors. */
double		ACos(double);
		/*!<Checks the input range before calling acos, and does thus
		    avoid nan's due to roundoff errors. */

#ifdef __cpp__
}

inline float	degFromNorth( float azimuth )
		{
		    const float deg = 90 - 57.2957795131 * azimuth;
		    return deg < 0 ? deg + 360 : deg;
		}
#endif


#endif
