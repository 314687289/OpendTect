#ifndef msvcdefs_h
#define msvcdefs_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Mar 2006
 RCS:		$Id: msvcdefs.h,v 1.6 2008-12-10 12:02:41 cvsranojay Exp $
________________________________________________________________________

 For use with Microsoft Visual C++ 8.0

-*/

# include <stdlib.h>

#define snprintf	_snprintf
#define isnan		_isnan

#define strncasecmp	strnicmp
#define strcasecmp	stricmp

#define strtoll		_strtoi64
#define strtoull	_strtoui64
#define strtof		strtod

// Index variable's scope is non-ANSI. This corrects that idiocy.
#define for				if (0) ; else for

# define mMaxFilePathLength		_MAX_PATH

# define mPolyRet(base,clss)		base
# define mTFriend(T,clss)
# define mTTFriend(T,C,clss)
# define mProtected			public
# define mPolyRetDownCast(clss,var)	dynamic_cast<clss>(var)
# define mPolyRetDownCastRef(clss,var)	*(dynamic_cast<clss*>(&var))
# define mDynamicCastGet(typ,out,in) \
	 typ out = 0; try { out = dynamic_cast< typ >( in ); } catch (...) {}


// Pragmas
//
// See http://oakroadsystems.com/tech/msvc.html for full discussion.
// Briefly, we need to compile at level 4 (/W4) because many important
// warnings, and even some errors, have been assigned the low level of
// 4. But level 4 also contains many spurious warnings, so we want to
// compile at /W2 or /W3. The pragmas in this file promote many errors
// and warnings to level 2 or 3.
//
// This file is http://oakroadsystems.com/tech/warnings.html ,
// and it was last modified on 2000-06-05. This file is
//
//       Copyright 1998,2000 by Stan Brown, Oak Road Systems
//                    http://oakroadsystems.com/
//
// License is hereby granted to use this file (or a modified version
// of it) in any source code without payment of any license fee,
// provided this paragraph is retained in its entirety.
//
// If you find this file useful, I'd appreciate your letting me know
// at the above e-mail address. If you find any errors, or have
// improvements to suggest, those will be gratefully received and
// acknowledged.

#pragma warning( disable : 4355 4003 )

#pragma warning(2:4032)     // function arg has different type from declaration
#pragma warning(2:4092)     // 'sizeof' value too big
#pragma warning(2:4132 4268)// const object not initialized
#pragma warning(2:4152)     // pointer conversion between function and data
#pragma warning(2:4239)     // standard doesn't allow this conversion
#pragma warning(2:4701)     // local variable used without being initialized
#pragma warning(2:4706)     // if (a=b) instead of (if a==b)
#pragma warning(2:4709)     // comma in array subscript
#pragma warning(3:4061)     // not all enum values tested in switch statement
#pragma warning(3:4710)     // inline function was not inlined
#pragma warning(3:4121)     // space added for structure alignment
#pragma warning(3:4505)     // unreferenced local function removed
#pragma warning(3:4019)     // empty statement at global scope
#pragma warning(3:4057)     // pointers refer to different base types
#pragma warning(3:4125)     // decimal digit terminates octal escape
#pragma warning(2:4131)     // old-style function declarator
#pragma warning(3:4211)     // extern redefined as static
#pragma warning(3:4213)     // cast on left side of = is non-standard
#pragma warning(3:4222)     // member function at file scope shouldn't be static
#pragma warning(3:4234 4235)// keyword not supported or reserved for future
#pragma warning(3:4504)     // type ambiguous; simplify code
#pragma warning(3:4507)     // explicit linkage specified after default linkage
#pragma warning(3:4515)     // namespace uses itself
#pragma warning(3:4516 4517)// access declarations are deprecated
#pragma warning(3:4670)     // base class of thrown object is inaccessible
#pragma warning(3:4671)     // copy ctor of thrown object is inaccessible
#pragma warning(3:4673)     // thrown object cannot be handled in catch block
#pragma warning(3:4674)     // dtor of thrown object is inaccessible
#pragma warning(3:4705)     // statement has no effect (example: a+1;)


#endif
