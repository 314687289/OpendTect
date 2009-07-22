#ifndef factory_h
#define factory_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Sep 1994, Aug 2006
 RCS:		$Id: factory.h,v 1.13 2009-07-22 16:01:14 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"
#include "errh.h"

//!Helper class for Factories, Factories are defined later in this file
mClass FactoryBase
{
public:
    virtual			~FactoryBase();
    const BufferStringSet&	getNames(bool username=false) const;
    void			setDefaultName(int idx);
    				//!<idx refers to names in names_,
				//!<or -1 for none
    const char*			getDefaultName() const;
    static char			cSeparator()	{ return ','; }
protected:
    int				indexOf(const char*) const;
    void			addNames(const char*,const char*);
    void			setNames(int,const char*,const char*);

private:
    BufferStringSet		names_;
    BufferStringSet		usernames_;
    BufferStringSet		aliases_;
    BufferString		defaultname_;

};



/*!
Generalized static factory that can deliver instances of T, when no
variable is needed in the creation.

Usage. Each implementation of the base class T must add themselves
to the factory when application starts up, e.g. in an initClass() function:
\code
class A
{
public:
    virtual int		myFunc() 	= 0;
};

class B : public A
{
public:
    static A*		createFunc() { return new B; }
    static void		initClass()
    			{ thefactory.addCreator(createFunc,"MyKeyword",
						"My Name"); }
			    
    int			myFunc();
};

\endcode

Two macros are available to make a static accessfuncion for the factory:
\code
mDefineFactory( ClassName, FunctionName );
\endcode

that will create a static function that returns an instance to
Factory<ClassName>.
If the function is a static member of a class, it has to be defined with
the mDefineFactoryInClass macro.

The static function must be implemented in a src-file with the macro

\code
mImplFactory( ClassName, FunctionName );
\endcode

*/


template <class T>
mClass Factory : public FactoryBase
{
public:
    typedef			T* (*Creator)();
    inline void			addCreator(Creator,const char* nm,
	    				   const char* username = 0);
    				/*!<Name may be not be null
				   If nm is found, old creator is replaced.
				   nm can be a SeparString, separated by
				   cSeparator(), allowing multiple names,
				   where the first name will be the main
				   name that is returned in getNames. */

    inline T*			create(const char* nm) const;
    				//!<Name may be not be null
protected:

    TypeSet<Creator>		creators_;
};


/*!
Generalized static factory that can deliver instances of T, when a
variable is needed in the creation.

Usage. Each implementation of the base class T must add themselves
to the factory when application starts up, e.g. in an initClass() function:
\code
class A
{
public:
    virtual int		myFunc() 	= 0;
};

class B : public A
{
public:
    static A*		createFunc(C* param) { return new B(param); }
    static void		initClass()
    			{ thefactory.addCreator(createFunc,"MyKeyword","My Name"); }
			    
    int			myFunc();
};

\endcode

Two macros are available to make a static accessfuncion for the factory:
\code
mDefineFactory1Param( ClassName, ParamClass, FunctionName );
\endcode

that will create a static function that returns an instance to
Factory1Param<ClassName,ParamClass>. The static function must be implemented
in a src-file with the macro

\code
mImplFactory1Param( ClassName, ParamClass, FunctionName );
\endcode

*/


template <class T, class P>
mClass Factory1Param : public FactoryBase
{
public:
    typedef			T* (*Creator)(P);
    inline void			addCreator(Creator,const char* nm=0,
	    				   const char* usernm = 0);
    				/*!<Name may be be null
				   If nm is found, old creator is replaced.
				   nm can be a SeparString, separated by
				   cSeparator(), allowing multiple names,
				   where the first name will be the main
				   name that is returned in getNames. */
    inline T*			create(const char* nm, P, bool chknm=true)const;
    				//!<Name may be be null, if null name is given
				//!<chknm will be forced to false
protected:

    TypeSet<Creator>		creators_;
};


template <class T, class P0, class P1>
mClass Factory2Param : public FactoryBase
{
public:
    typedef			T* (*Creator)(P0,P1);
    inline void			addCreator(Creator,const char* nm=0,
	    				   const char* usernm = 0);
    				/*!<Name may be be null
				   If nm is found, old creator is replaced.
				   nm can be a SeparString, separated by
				   cSeparator(), allowing multiple names,
				   where the first name will be the main
				   name that is returned in getNames. */
    inline T*			create(const char* nm, P0, P1,
	    			       bool chknm=true)const;
    				//!<Name may be be null, if null name is given
				//!<chknm will be forced to false
protected:

    TypeSet<Creator>		creators_;
};


#define mCreateImpl( donames, createfunc ) \
    if ( donames ) \
    { \
	const int idx = indexOf( name ); \
	return idx==-1 ? 0 : createfunc; \
    } \
 \
    for ( int idx=0; idx<creators_.size(); idx++ ) \
    { \
	T* res = createfunc; \
	if ( res ) return res; \
    } \
 \
    return 0


#define mAddCreator \
    const int idx = indexOf( name );\
\
    if ( idx==-1 )\
    { \
	addNames( name, username ); \
	creators_ += cr; \
    } \
    else\
    {\
	setNames( idx, name, username ); \
	creators_[idx] = cr;\
    }

template <class T> inline
void Factory<T>::addCreator( Creator cr, const char* name,
			     const char* username )
{
    if ( !name ) return;\
    mAddCreator;
}


template <class T> inline
T* Factory<T>::create( const char* name ) const
{
    mCreateImpl( true, creators_[idx]() );
}


template <class T, class P> inline
void Factory1Param<T,P>::addCreator( Creator cr, const char* name,
				     const char* username )
{
    mAddCreator;
}


template <class T, class P> inline
T* Factory1Param<T,P>::create( const char* name, P data, bool chk ) const
{
    mCreateImpl( chk, creators_[idx]( data ) );
}


template <class T, class P0, class P1> inline
void Factory2Param<T,P0,P1>::addCreator( Creator cr, const char* name,
					 const char* username )
{
    mAddCreator;
}


template <class T, class P0, class P1> inline
T* Factory2Param<T,P0,P1>::create( const char* name, P0 p0, P1 p1,
				   bool chk ) const
{
    mCreateImpl( chk, creators_[idx]( p0, p1 ) );
}


#define mDefineFactory( T, funcname ) \
mGlobal ::Factory<T>& funcname()


#define mDefineFactoryInClass( T, funcname ) \
static ::Factory<T>& funcname()


#define mImplFactory( T, funcname ) \
::Factory<T>& funcname() \
{ \
    static PtrMan< ::Factory<T> > inst = new ::Factory<T>; \
    return *inst; \
} 


#define mDefineFactory1Param( T, P, funcname ) \
mGlobal ::Factory1Param<T,P>& funcname()


#define mDefineFactory1ParamInClass( T, P, funcname ) \
static ::Factory1Param<T,P>& funcname()


#define mImplFactory1Param( T, P, funcname ) \
::Factory1Param<T,P>& funcname() \
{ \
    static PtrMan< ::Factory1Param<T,P> > inst = new ::Factory1Param<T,P>; \
    return *inst; \
} 


#define mDefineFactory2Param( T, P0, P1, funcname ) \
mGlobal ::Factory2Param<T,P0,P1>& funcname()


#define mDefineFactory2ParamInClass( T, P0, P1, funcname ) \
static ::Factory2Param<T,P0,P1>& funcname()


#define mImplFactory2Param( T, P0, P1, funcname ) \
::Factory2Param<T,P0,P1>& funcname() \
{ \
    static PtrMan< ::Factory2Param<T,P0,P1> > inst =\
	    new ::Factory2Param<T,P0,P1>; \
    return *inst; \
} 

#undef mCreateImpl
#undef mAddCreator

#endif
