#ifndef bufstring_H
#define bufstring_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		12-4-2000
 Contents:	Variable buffer length strings with minimum size.
 RCS:		$Id: bufstring.h,v 1.20 2006-04-24 12:07:57 cvsnanne Exp $
________________________________________________________________________

-*/

#include "string2.h"
#include <string.h>
#include <stdlib.h>
#include <iosfwd>

/*!\brief String with variable length but guaranteed minimum buffer size.

The minimum buffer size makes life easier in worlds where strcpy etc. rule.
Overhead is 4 extra bytes for variable length and 4 bytes for minimum length.

*/

class BufferString
{
public:
   			BufferString( const char* s=0,
				      unsigned int ml=mMaxUserIDLength )
				: minlen(ml+1)
				    { init(); if ( s ) *this = s; }
   			BufferString( int i,
				      unsigned int ml=mMaxUserIDLength )
				: minlen(ml+1)
				    { init(); *this += i; }
   			BufferString( double d,
				      unsigned int ml=mMaxUserIDLength )
				: minlen(ml+1)
				    { init(); *this += d; }
   			BufferString( float f,
				      unsigned int ml=mMaxUserIDLength )
				: minlen(ml+1)
				    { init(); *this += f; }
			BufferString( const BufferString& bs )
				: minlen(bs.minlen)
				    { init(); *this = bs; }
			~BufferString()
				    { delete [] buf_; }
   inline BufferString&	operator=( const BufferString& bs )
			{ if ( &bs != this ) *this = bs.buf_; return *this; }
   inline BufferString&	operator=( int i )
			{ *buf_ = '\0'; *this += i; return *this; }
   inline BufferString&	operator=( double d )
			{ *buf_ = '\0'; *this += d; return *this; }
   inline BufferString&	operator=( float f )
			{ *buf_ = '\0'; *this += f; return *this; }
   inline BufferString&	operator+=( int i )
			{ *this += getStringFromInt("%d",i); return *this; }
   inline BufferString&	operator+=( double d )
			{ *this += getStringFromDouble("%lg",d); return *this; }
   inline BufferString&	operator+=( float f )
			{ *this += getStringFromFloat("%g",f); return *this; }
   inline		operator const char*() const	{ return buf_; }
   inline char*		buf()				{ return buf_; }
   inline const char*	buf() const			{ return buf_; }
   inline char&		operator [](int idx)		{ return buf_[idx]; }
   inline const char&	operator [](int idx) const	{ return buf_[idx]; }
   inline unsigned int	size() const			{ return strlen(buf_); }
   inline unsigned int	bufSize() const			{ return len; }
   inline void		setBufSize(unsigned int);
   inline bool		operator==( const BufferString& s ) const
				    { return operator ==( s.buf_ ); }
   inline bool		operator!=( const BufferString& s ) const
				    { return operator !=( s.buf_ ); }
   inline bool		operator!=( const char* s ) const
				    { return ! (*this == s); }
   inline bool		operator >( const char* s ) const
				    { return s ? strcmp(buf_,s) > 0 : true; }
   inline bool		operator <( const char* s ) const
				    { return s ? strcmp(buf_,s) < 0 : false; }

   inline BufferString&	operator=(const char*);
   inline BufferString&	operator+=(const char*);
   inline bool		operator==(const char*) const;

   static const BufferString& empty();

protected:

    char*		buf_;
    unsigned int	len;
    const unsigned int	minlen;

private:

    inline void		init()
			{ len = minlen; buf_ = new char [len]; *buf_ ='\0'; }

};

std::ostream& operator <<(std::ostream&,const BufferString&);
std::istream& operator >>(std::istream&,BufferString&);


inline bool BufferString::operator==( const char* s ) const
{
    if ( !s ) return *buf_ == '\0';

    const char* ptr = buf_;
    while ( *s && *ptr )
	if ( *ptr++ != *s++ ) return false;

    return *ptr == *s;
}


inline void BufferString::setBufSize( unsigned int newlen )
{
    if ( newlen < minlen ) newlen = minlen;
    if ( newlen == len ) return;

    char* oldbuf = buf_;
    buf_ = new char [newlen];
    memcpy( buf_, oldbuf, mMIN(newlen,strlen(oldbuf)) );
    delete [] oldbuf;

    len = newlen;
}


inline BufferString& BufferString::operator=( const char* s )
{
    if ( buf_ != s )
    {
	if ( !s ) s = "";
	setBufSize( (unsigned int)(strlen(s) + 1) );
	char* ptr = buf_;
	while ( *s ) *ptr++ = *s++;
	*ptr = '\0';
    }
    return *this;
}


inline BufferString& BufferString::operator +=( const char* s )
{
    if ( s && *s )
    {
	setBufSize( (unsigned int)(strlen(s) + strlen(buf_)) + 1 );

	char* ptr = buf_;
	while ( *ptr ) ptr++;
	while ( *s ) *ptr++ = *s++;
	*ptr = '\0';
    }
    return *this;
}


#endif
