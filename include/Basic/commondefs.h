#ifndef commondefs_h
#define commondefs_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Mar 2006
 RCS:		$Id: commondefs.h,v 1.23 2009-03-25 07:05:32 cvsranojay Exp $
________________________________________________________________________

 Some very commonly used macros.

-*/

#include "plfdefs.h"

#define mSWAP(x,y,tmp)		{ tmp = x; x = y; y = tmp; }
#define mNINT(x)		( (int)((x)>0 ? (x)+.5 : (x)-.5) )
#define mMAX(x,y)		( (x)>(y) ? (x) : (y) )
#define mMIN(x,y)		( (x)<(y) ? (x) : (y) )

#define mIsZero(x,eps)		( (x) < (eps) && (x) > (-eps) )
#define mIsEqual(x,y,eps)	( (x-y) < (eps) && (x-y) > (-eps) )
#define mIsEqualRel(x,y,e)	( (y) ? ((x)/(y))-1<(e) && ((x)/(y)-1)>(-e) \
				      : mIsZero(x,e) )
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


#define mMaxUserIDLength 127

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

#ifdef BASIC_EXPORTS
# define mBasicGlobal	dll_export
# define mBasicExtern	extern dll_export
#else
# define mBasicGlobal	dll_import
# define mBasicExtern	extern dll_import
#endif

#endif
