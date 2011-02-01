#ifndef enums_h
#define enums_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		4-2-1994
 Contents:	Enum <--> string conversion
 RCS:		$Id: enums.h,v 1.26 2011-02-01 15:57:57 cvskris Exp $
________________________________________________________________________

-*/

#include "iopar.h"

/*!\brief Some utilities surrounding the often needed enum <-> string table.

The function EnumDef::convert returns the enum (integer) value from a text
string. The first arg is string you wish to convert to the enum, the second
is the array with enum names. Then, the integer value of the first enum value
(also returned when no match is found) and the number of characters to be
matched (0=all). Make absolutely sure the char** definition has a closing
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
    static const char**		StateNames();
    static bool 		parseEnum(const char*, State& );
    static bool 		parseEnum(const IOPar&,const char* key,State&);
    static int 			parseEnumState(const char*);
    static const char*		toString(State);

protected:

    static const char*		StateNames_[];
    static const EnumDef	StateDefinition_;

public:

    enum			Type { Yes, No, Maybe };
    Type			type() const { return type_; }
    void			setType(Type _e_) { type_ = _e_; }
    static const EnumDef&	TypeDef();
    static const char**		TypeNames();
    static bool 		parseEnum(const char*, Type& );
    static bool 		parseEnum(const IOPar&, const char* key,Type& );
    static int 			parseEnumType(const char*);
    static const char* 		toString(Type);

protected:

    static const char*		TypeNames_[];
    static const EnumDef	TypeDefinition_;

    Type			type_;

};

\endcode

and, in myclass.cc:

\code

const EnumDef& MyClass::StateDef()    { return StateDefinition_; }

const EnumDef MyClass::StateDefinition_("My class state",MyClass::Statenames,1);

bool MyClass::parseEnum(const char* txt, State& res ) \
{ \
    const int idx = StateDef().isValidName( txt ) \
        ?  StateDef().convert( txt ) \
	: -1; \
    if ( idx<0 ) \
	return false; \
 \
    res = (State) idx; \
    return true; \
} \
bool MyClass::parseEnum( const IOPar& par, const char* key, State& res ) \
{ return parseEnum( par.find( key ), res ); } \
MyClass::State MyClass::parseEnumState(const char* txt) \
{ \
    return (MyClass::State) StateDef().convert( txt ); \
} \

const char* MyClass::Statenames_[] =
        { "Good", "Bad", "Not very handsome", 0 };


const EnumDef& MyClass::TypeDef()   { return TypeDefinition_; }
const EnumDef MyClass::TypeDefinition_("My class type", MyClass::Typenames, 0 );
bool MyClass::parseEnum(const char* txt, Type& res ) \
{ \
    const int idx = TypeDef().isValidName( txt ) \
        ?  TypeDef().convert( txt ) \
	: -1; \
    if ( idx<0 ) \
	return false; \
 \
    res = (Type) idx; \
    return true; \
} \
bool MyClass::parseEnum( const IOPar& par, const char* key, Type& res ) \
{ return parseEnum( par.find( key ), res ); } \
MyClass::Type MyClass::parseEnumType(const char* txt) \
{ \
    return (MyClass::Type) TypeDef().convert( txt ); \
} \

const char* MyClass::Typenames_[] =
        { "Yes", "No", "Not sure", 0 };

\endcode

-*/


#include "string2.h"
#include "namedobj.h"


/*\brief holds data pertinent for a certain enum */

mClass EnumDef : public NamedObject
{
public:
			EnumDef( const char* nm, const char* s[], short nrs=0 )
				: NamedObject(nm)
				, names_(s)
				, nrsign_(nrs)	{}

    inline bool		isValidName( const char* s ) const
			{ return getIndexInStringArrCI(s,names_,0,nrsign_,-1)
			    	< 0; }
    inline int		convert( const char* s ) const
			{ return getIndexInStringArrCI(s,names_,0,nrsign_,0); }
    inline const char*	convert( int i ) const
			{ return names_[i]; }

    inline int		size() const
			{ int i=0; while ( names_[i] ) i++; return i; }

protected:

    const char**	names_;
    short		nrsign_;

};

#define DeclareEnumUtils(enm) \
public: \
    static const EnumDef& enm##Def(); \
    static const char** enm##Names();\
    static bool parseEnum##enm(const char*,enm&);  /*legacy */ \
    static bool parseEnum(const char*,enm&); \
    static bool parseEnum(const IOPar&,const char*,enm&); \
    static enm parseEnum##enm(const char*);  \
    static const char* toString(enm); \
    static const char* get##enm##String(enm); /*legacy */ \
protected: \
    static const char* enm##Names_[];\
    static const EnumDef enm##Definition_; \
public:

#define DeclareNameSpaceEnumUtils(enm) \
    mExtern const EnumDef& enm##Def(); \
    mExtern const char** enm##Names();\
    extern const char* enm##Names_[];\
    extern const EnumDef enm##Definition_; \
    mExtern bool parseEnum(const IOPar&,const char*,enm&); \
    mExtern bool parseEnum(const char*,enm&); \
    mExtern bool parseEnum##enm(const char*,enm&); /*legacy */  \
    mExtern enm parseEnum##enm(const char*); \
    mExtern const char* toString(enm); \
    mExtern const char* get##enm##String(enm); /*legacy */ 

#define DeclareEnumUtilsWithVar(enm,varnm) \
public: \
    enm varnm() const { return varnm##_; } \
    void set##enm(enm _e_) { varnm##_ = _e_; } \
    DeclareEnumUtils(enm) \
protected: \
    enm varnm##_; \
public:


#define DefineEnumNames(clss,enm,deflen,prettynm) \
const EnumDef clss::enm##Definition_ \
	( prettynm, clss::enm##Names_, deflen ); \
const EnumDef& clss::enm##Def() \
    { return enm##Definition_; } \
const char** clss::enm##Names() \
    { return enm##Names_; }  \
bool clss::parseEnum##enm(const char* txt, enm& res ) \
{ return clss::parseEnum( txt, res ); } \
bool clss::parseEnum(const char* txt, enm& res ) \
{ \
    const int idx = enm##Def().isValidName( txt ) \
        ?  enm##Def().convert( txt ) \
	: -1; \
    if ( idx<0 ) \
	return false; \
 \
    res = (enm) idx; \
    return true; \
} \
bool clss::parseEnum( const IOPar& par, const char* key, enm& res ) \
{ return parseEnum( par.find( key ), res ); } \
clss::enm clss::parseEnum##enm(const char* txt) \
{ \
    return (clss::enm) enm##Def().convert( txt ); \
} \
const char* clss::get##enm##String( enm theenum ) \
{ return clss::toString( theenum ); } \
const char* clss::toString( enm theenum ) \
{ \
    const int idx = (int) theenum; \
    if ( idx<0 || idx>=enm##Def().size() ) \
        return 0; \
 \
    return enm##Names_[idx]; \
} \
const char* clss::enm##Names_[] =

#define DefineNameSpaceEnumNames(nmspc,enm,deflen,prettynm) \
const EnumDef nmspc::enm##Definition_ \
	( prettynm, nmspc::enm##Names_, deflen ); \
const EnumDef& nmspc::enm##Def() \
    { return nmspc::enm##Definition_; } \
const char** nmspc::enm##Names() \
    { return nmspc::enm##Names_; }  \
bool nmspc::parseEnum##enm(const char* txt, enm& res ) \
{ return nmspc::parseEnum( txt, res ); } \
bool nmspc::parseEnum(const char* txt, enm& res ) \
{ \
    const int idx = enm##Def().isValidName( txt ) \
        ?  enm##Def().convert( txt ) \
	: -1; \
    if ( idx<0 ) \
	return false; \
 \
    res = (enm) idx; \
    return true; \
} \
bool nmspc::parseEnum( const IOPar& par, const char* key, enm& res ) \
{ \
    const char* val = par.find( key ); \
    return nmspc::parseEnum( val, res ); \
} \
nmspc::enm nmspc::parseEnum##enm(const char* txt) \
{ \
    return (nmspc::enm) enm##Def().convert( txt ); \
} \
const char* nmspc::get##enm##String( enm theenum ) \
{ return nmspc::toString( theenum ); } \
const char* nmspc::toString( enm theenum ) \
{ \
    const int idx = (int) theenum; \
    if ( idx<0 || idx>=enm##Def().size() ) \
        return 0; \
 \
    return enm##Names_[idx]; \
} \
const char* nmspc::enm##Names_[] =

#endif
