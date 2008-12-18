#ifndef enums_H
#define enums_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		4-2-1994
 Contents:	Enum <--> string conversion
 RCS:		$Id: enums.h,v 1.12 2008-12-18 12:30:31 cvsranojay Exp $
________________________________________________________________________

-*/


/*!\brief Some utilities surrounding the often needed enum <-> string table.

The simple C function getEnum returns the enum (integer) value from a text
string. The first arg is string you wish to convert to the enum, the second
is the array with enum names. Then, the integer value of the first enum value
(also returned when no match is found) and the number of characters to be
matched (0=all). Make absolutely sure the char** namearr has a closing
' ... ,0 };'.

Normally, you'll have a class with an enum member. In that case, you'll want to
use the EnumDef classes. These are normally almost hidden by a few
simple macros:
* DeclareEnumUtils(enm) will make sure the enum will have a string conversion.
* DeclareEnumUtilsWithVar(enm,varnm) will also create an instance variable with
  accessors.
* DefineEnumNames(clss,enm,deflen,prettynm) defines the names.
* For namespaces, you can use DeclareNameSpaceEnumUtils only

The 'Declare' macros should be placed in the public section of the class.
Example of usage:

in myclass.h:
\code
#include <enums.h>

class MyClass
{
public:
    enum State  { Good, Bad, Ugly };
		DeclareEnumUtils(State)
    enum Type   { Yes, No, Maybe };
		DeclareEnumUtilsWithVar(Type,type)

    // rest of class

};
\endcode

in myclass.cc:

\code
#include <myclass.h>

DefineEnumNames(MyClass,State,1,"My class state")
	{ "Good", "Bad", "Not very handsome", 0 };
DefineEnumNames(MyClass,Type,0,"My class type")
	{ "Yes", "No", "Not sure", 0 };
\endcode

Note the '1' in the first one telling the EnumDef that only one character needs
to be matched when converting string -> enum. The '0' in the second indicates
that the entire string must match.

This will expand to (added newlines, removed some superfluous stuff:

\code

class MyClass
{
public:

    enum			State { Good, Bad, Ugly };
    static const EnumDef&	StateDef();
    static const char**		StateNamesGet();
    static const char*		StateNames[];

protected:

    static const EnumDef	StateDefinition;

public:

    enum			Type { Yes, No, Maybe };
    Type			type() const { return type_; }
    void			setType(Type _e_) { type_ = _e_; }
    static const EnumDef&	TypeDef();
    static const char**		TypeNamesGet();
    static const char*		TypeNames[];

protected:

    static const EnumDef	TypeDefinition;

    Type			type_;

};

\endcode

and, in myclass.cc:

\code

const EnumDef& MyClass::StateDef()    { return StateDefinition; }
const EnumDef MyClass::StateDefinition      ("My class state",MyClass::Statenames,1);

const char* MyClass::Statenames[] =
        { "Good", "Bad", "Not very handsome", 0 };


const EnumDef& MyClass::TypeDef()   { return TypeDefinition; }
const EnumDef MyClass::TypeDefinition     ( "My class type", MyClass::Typenames, 0 );

const char* MyClass::Typenames[] =
        { "Yes", "No", "Not sure", 0 };

\endcode

-*/


#include "string2.h"

#ifndef __cpp__
mGlobal( int getEnum(const char*,char** namearr,int startnr,
		     int nr_chars_to_match) )
mGlobal( int getEnumDef(const char*,char** namearr,int startnr,
			int nr_chars_to_match,int notfoundval) )
#else

#include "namedobj.h"

extern "C" { int getEnum(const char*,const char**,int,int); }
extern "C" { int getEnumDef(const char*,const char**,int,int,int); }


/*\brief holds data pertinent for a certain enum */

mClass EnumDef : public NamedObject
{
public:
			EnumDef( const char* nm, const char* s[], int nrs=0 )
				: NamedObject(nm)
				, names_(s)
				, nrsign(nrs)	{}

    inline int		convert( const char* s ) const
			{ return getEnum(s,names_,0,nrsign); }
    inline const char*	convert( int i ) const
			{ return names_[i]; }

    inline int		size() const
			{ int i=0; while ( names_[i] ) i++; return i; }

protected:

    const char**	names_;
    int			nrsign;

};


#define DeclareEnumUtils(enm) \
public: \
    static const EnumDef& enm##Def(); \
    static const char* enm##Names[];\
    static const char** enm##NamesGet();\
protected: \
    static const EnumDef enm##Definition; \
public:

#define DeclareNameSpaceEnumUtils(enm) \
    extern const EnumDef& enm##Def(); \
    extern const char* enm##Names[];\
    extern const char** enm##NamesGet();\
    extern const EnumDef enm##Definition;

#define DeclareEnumUtilsWithVar(enm,varnm) \
public: \
    enm varnm() const { return varnm##_; } \
    void set##enm(enm _e_) { varnm##_ = _e_; } \
    DeclareEnumUtils(enm) \
protected: \
    enm varnm##_; \
public:


#define DefineEnumNames(clss,enm,deflen,prettynm) \
const EnumDef clss::enm##Definition \
	( prettynm, clss::enm##Names, deflen ); \
const EnumDef& clss::enm##Def() \
    { return enm##Definition; } \
const char** clss::enm##NamesGet() \
    { return enm##Names; }  \
const char* clss::enm##Names[] =

#define DefineNameSpaceEnumNames(nmspc,enm,deflen,prettynm) \
const EnumDef nmspc::enm##Definition \
	( prettynm, nmspc::enm##Names, deflen ); \
const EnumDef& nmspc::enm##Def() \
    { return nmspc::enm##Definition; } \
const char** nmspc::enm##NamesGet() \
    { return nmspc::enm##Names; }  \
const char* nmspc::enm##Names[] =


#define eString(enm,vr)	(enm##Def().convert((int)vr))
//!< this is the actual enum -> string
#define eEnum(enm,str)	((enm)enm##Def().convert(str))
//!< this is the actual string -> enum
#define eKey(enm)	(enm##Def().name())
//!< this is the 'pretty name' of the enum


#endif /* #ifndef __cpp__ */


#endif
