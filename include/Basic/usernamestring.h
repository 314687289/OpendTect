#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		12-4-2000
 Contents:	Variable buffer length strings with minimum size.
________________________________________________________________________

-*/

#include "uistring.h"

/*!\brief This string type allows users to specify a name for objects.

  * Should support full international language
  * Never translated
  * Needs to be savable and retrievable to/from files
  * Can be created via operations on other UserNameString's (to fill defaults)
  * Has no trivial conversion from/to OD::String's, but does have that for
    translated strings (uiString's)

*/

mExpClass(Basic) UserNameString
{
public:

    inline		UserNameString()		{ mkIndep(); }
    inline		UserNameString( const uiString& s )
				: impl_(s)		{ mkIndep(); }
    inline		UserNameString( const UserNameString& oth )
				: impl_(oth.impl_)	{ mkIndep(); }
    virtual		~UserNameString()		{}

    UserNameString&	operator=( const UserNameString& oth )
			{ impl_ = oth.impl_; mkIndep(); return *this; }
    UserNameString&	operator=( const uiString& s )
			{ impl_ = s; mkIndep(); return *this; }

    explicit operator	uiString&()			{ return impl_; }
    explicit operator	const uiString&() const		{ return impl_; }

    inline bool		operator==( const UserNameString& oth ) const
			{ return impl_.isEqualTo( oth.impl_ ); }
    inline bool		operator==( const uiString& s ) const
			{ return impl_.isEqualTo( s ); }
    inline bool		operator!=( const UserNameString& oth ) const
			{ return !impl_.isEqualTo( oth.impl_ ); }
    inline bool		operator!=( const uiString& s ) const
			{ return !impl_.isEqualTo( s ); }

    UserNameString&	setEmpty()
			{ impl_.setEmpty(); return *this; }
    UserNameString&	set( const UserNameString& oth )
			{ *this = oth; return *this; }
    UserNameString&	set( const uiString& s )
			{ *this = s; return *this; }

    UserNameString&	insert( const UserNameString& oth )
			{ return join( oth.impl_, true ); }
    UserNameString&	insert( const uiString& s )
			{ return join( s, true ); }
    UserNameString&	add( const UserNameString& oth )
			{ return join( oth.impl_, false ); }
    UserNameString&	add( const uiString& s )
			{ return join( s, false ); }

    static const UserNameString& empty();

protected:

    uiString		impl_;

    inline void		mkIndep()	{ impl_.makeIndependent(); }
    UserNameString&	join(const uiString&,bool before);

};
