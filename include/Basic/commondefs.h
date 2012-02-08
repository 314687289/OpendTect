#ifndef commondefs_h
#define commondefs_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Mar 2006
 RCS:		$Id: commondefs.h,v 1.36 2012-02-08 23:07:53 cvsnanne Exp $
________________________________________________________________________

 Some very commonly used macros.

-*/

#include "plfdefs.h"

#define mSWAP(x,y,tmp)		{ tmp = x; x = y; y = tmp; }
#define mRounded(typ,x)		( (typ)((x)>0 ? (x)+.5 : (x)-.5) )
#define mNINT(x)		mRounded(int,x)
#define mMAX(x,y)		( (x)>(y) ? (x) : (y) )
#define mMIN(x,y)		( (x)<(y) ? (x) : (y) )

#define mIsZero(x,eps)		( (x) < (eps) && (x) > (-eps) )
#define mIsEqual(x,y,eps)	( (x-y) < (eps) && (x-y) > (-eps) )
#define mIsEqualRel(x,y,e)	( (y) ? ((x)/(y))-1<(e) && ((x)/(y)-1)>(-e) \
				      : mIsZero(x,e) )
#define mIsEqualWithUdf(x,y,e)	((mIsUdf(x) && mIsUdf(y)) || mIsEqual(x,y,e) )
#define mDefEps			(1e-10)

# define mC_True	1
# define mC_False	0

#ifndef M_PI
# define M_PI		3.14159265358979323846
#endif

#ifndef M_PI_2
# define M_PI_2		1.57079632679489661923
#endif

#ifndef M_PI_4
# define M_PI_4		0.78539816339744830962
#endif

#ifndef M_SQRT1_2
# define M_SQRT1_2	0.70710678118654752440
#endif

#ifndef MAXFLOAT
# define MAXFLOAT	3.4028234663852886e+38F
#endif

#ifndef MAXDOUBLE
# define MAXDOUBLE	1.7976931348623157e+308
#endif

#ifdef __win__
# include <stdio.h>
# undef small
#endif


#define mFromFeetFactor		0.3048
#define mToFeetFactor		3.2808399
#define mToFeetFactorD		3.28083989501312336
#define mToSqMileFactor		0.3861 			//km^2 to mile^2
#define mMileToFeetFactor	5280


#ifdef __msvc__
# include "msvcdefs.h"
#else

# define mMaxFilePathLength	255

# define mPolyRet(base,clss)	clss
# define mTFriend(T,clss)	template <class T> friend class clss
# define mTTFriend(T,C,clss)	template <class T, class C> friend class clss
# define mProtected		protected
# define mPolyRetDownCast(clss,var)	var
# define mPolyRetDownCastRef(clss,var)	var
# define mDynamicCast(typ,out,in)	out = dynamic_cast< typ >( in );
# define mDynamicCastGet(typ,out,in)	typ mDynamicCast(typ,out,in)

#endif

#define mTODOHelpID	"0.0.0"
#define mNoHelpID	"-"

#ifdef __msvc__
# define dll_export	__declspec( dllexport )
# define dll_import	__declspec( dllimport )
#else
# define dll_export
# define dll_import
#endif

#define mClass		class dll_export
#define mStruct		struct dll_export
#define mGlobal		dll_export 
#define mExtern		extern dll_export
#define mExternC	extern "C" dll_export

#if defined(Basic_EXPORTS) || defined(BASIC_EXPORTS)
# define mBasicClass	class dll_export
# define mBasicGlobal	dll_export
# define mBasicExtern	extern dll_export
#else
# define mBasicClass	class dll_import
# define mBasicGlobal	dll_import
# define mBasicExtern	extern dll_import
#endif




#if defined(General_EXPORTS) || defined(GENERAL_EXPORTS)
# define mGeneralClass	class dll_export
# define mGeneralGlobal	dll_export
# define mGeneralExtern	extern dll_export
#else
# define mGeneralClass	class dll_import
# define mGeneralGlobal	dll_import
# define mGeneralExtern	extern dll_import
#endif

#define mIfNotFirstTime(act) \
    static bool _already_visited_ = false; \
    if ( _already_visited_ ) act; \
    _already_visited_ = true


#endif
