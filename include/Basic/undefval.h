#ifndef undefval_h
#define undefval_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          13/01/2005
 RCS:           $Id: undefval.h,v 1.7 2006-06-23 14:16:36 cvsjaap Exp $
________________________________________________________________________

-*/

#include "plftypes.h"

//! Undefined value. IEEE gives NaN but that's not exactly what we want
#define __mUndefValue             1e30
//! Check on undefined. Also works when double converted to float and vv
#define __mIsUndefined(x)         (((x)>9.99999e29)&&((x)<1.00001e30))
//! Almost MAXINT so unlikely, but not MAXINT to avoid that
#define __mUndefIntVal            2109876543
//! Almost MAXINT64 therefore unlikely.
#define __mUndefIntVal64          9223344556677889900LL


#ifdef __cpp__


//! Use this macro to get the undefined for simple types
#define mUdf(type) Values::Undef<type>::val()
//! Use this macro to check for undefinedness of simple types
#define mIsUdf(val) Values::isUdf(val)
//! Use this macro to set simple types to undefined
#define mSetUdf(val) Values::setUdf(val)


/*!  \brief Templatized undefined and initialisation (i.e. null) values.  

    Since these are all templates, they can be used much more generic
    than previous solutions with macros.

Use like:

  T x = mUdf(T);
  if ( mIsUdf(x) )
      mSetUdf(y);

*/

namespace Values
{

/*!  \brief Templatized undefined values.  */
template<class T>
class Undef
{
public:
    static T		val();
    static bool		hasUdf();
    static bool		isUdf(T);
    void		setUdf(T&);
};


template<>
class Undef<int32>
{
public:
    static int32	val()			{ return __mUndefIntVal; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( int32 i )	{ return i == __mUndefIntVal; }
    static void		setUdf( int32& i )	{ i = __mUndefIntVal; }
};

template<>
class Undef<uint32>
{
public:
    static uint32	val()			{ return __mUndefIntVal; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( uint32 i )	{ return i == __mUndefIntVal; }
    static void		setUdf( uint32& i )	{ i = __mUndefIntVal; }
};


template<>
class Undef<int64>
{
public:
    static int64	val()			{ return __mUndefIntVal64; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( int64 i )	{ return i == __mUndefIntVal64;}
    static void		setUdf( int64& i )	{ i = __mUndefIntVal64; }
};

template<>
class Undef<uint64>
{
public:
    static uint64	val()			{ return __mUndefIntVal64; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( uint64 i )	{ return i == __mUndefIntVal64;}
    static void		setUdf( uint64& i )	{ i = __mUndefIntVal64; }
};


template<>
class Undef<bool>
{
public:
    static bool		val()			{ return false; }
    static bool		hasUdf()		{ return false; }
    static bool		isUdf( bool b )		{ return false; }
    static void		setUdf( bool& b )	{ b = false; }
};


template<>
class Undef<float>
{
public:
    static float	val()			{ return __mUndefValue; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( float f )	{ return __mIsUndefined(f); }
    static void		setUdf( float& f )	{ f = __mUndefValue; }
};


template<>
class Undef<double>
{
public:
    static double	val()			{ return __mUndefValue; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( double d )	{ return __mIsUndefined(d); }
    static void		setUdf( double& d )	{ d = __mUndefValue; }
};


template<>
class Undef<const char*>
{
public:
    static const char*	val()			{ return ""; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( const char* s )	{ return !s || !*s; }
    static void		setUdf( const char*& )	{}
};


template<>
class Undef<char*>
{
public:
    static const char*	val()			{ return ""; }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( const char* s )	{ return !s || !*s; }
    static void		setUdf( char*& s )	{ if ( s ) *s = '\0'; }
};


template <class T>
bool isUdf( const T& t )
{ 
    return Undef<T>::isUdf(t);  
}

template <class T>
const T& udfVal( const T& t )
{ 
    static T u = Undef<T>::val();
    return u;
}

template <class T>
bool hasUdf()
{ 
    return Undef<T>::hasUdf();  
}

template <class T>
T& setUdf( T& u )
{
    Undef<T>::setUdf( u );
    return u; 
}

}


#else

/* for C, fallback to old style macro's. Do not provide mUdf and mIsUdf to
   ensure explicit thinking about the situation. */

# define scUndefValue		 "1e30"
# define mcUndefValue             __mUndefValue
# define mcIsUndefined(x)         __mIsUndefined(x)


#endif 


#endif
