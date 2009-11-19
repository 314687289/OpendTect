/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: bufstring.cc,v 1.24 2009-11-19 09:59:48 cvsbert Exp $";

#include "bufstring.h"
#include "bufstringset.h"
#include "fixedstring.h"
#include "iopar.h"
#include "general.h"
#include "globexpr.h"

#include <iostream>
#include <stdlib.h>
#include "string2.h"
#include <string.h>


BufferString::BufferString( int sz, bool mknull )
    : minlen_(sz)
    , len_(0)
    , buf_(0)
{
    if ( sz < 1 ) return;

    setBufSize( sz );
    if ( len_ > 0 )
	memset( buf_, 0, len_ );
}


BufferString::BufferString( const BufferString& bs )
    : minlen_(bs.minlen_)
    , len_(0)
    , buf_(0)
{
    if ( !bs.buf_ || !bs.len_ ) return;

    mTryAlloc( buf_, char [bs.len_] );
    if ( buf_ )
    {
	len_ = bs.len_;
	strcpy( buf_, bs.buf_ );
    }
}


BufferString::~BufferString()
{
    destroy();
}


bool BufferString::operator==( const char* s ) const
{
    if ( !s || !(*s) ) return isEmpty();
    if ( isEmpty() ) return false;

    const char* ptr = buf_;
    while ( *s && *ptr )
	if ( *ptr++ != *s++ ) return false;

    return *ptr == *s;
}


char* BufferString::buf()
{
    if ( !buf_ )
	init();

    return buf_;
}


bool BufferString::isEmpty() const
{ return !buf_ || !(*buf_); }


void BufferString::setEmpty()
{
    if ( len_ != minlen_ )
	{ destroy(); init(); }
    else
	buf_[0] = 0;
}


bool BufferString::isEqual( const char* s, bool caseinsens ) const
{
    return caseinsens ? caseInsensitiveEqual(buf(),s,0) : (*this == s);
}


bool BufferString::isStartOf( const char* s, bool caseinsens ) const
{
    return caseinsens ? matchStringCI(buf(),s) : matchString(buf(),s);
}


bool BufferString::matches( const char* s, bool caseinsens ) const
{
    return GlobExpr(s,!caseinsens).matches( buf() );
}


BufferString& BufferString::operator=( const char* s )
{
    if ( buf_ == s ) return *this;

    if ( !s ) s = "";
    setBufSize( (unsigned int)(strlen(s) + 1) );
    char* ptr = buf_;
    while ( *s ) *ptr++ = *s++;
    *ptr = '\0';
    return *this;
}


BufferString& BufferString::add( const char* s )
{
    if ( s && *s )
    {
	const unsigned int newsize = strlen(s) + (buf_ ? strlen(buf_) : 0) +1;
	setBufSize( newsize );

	char* ptr = buf_;
	while ( *ptr ) ptr++;
	    while ( *s ) *ptr++ = *s++;
		*ptr = '\0';
    }
    return *this;
}


unsigned int BufferString::size() const	
{
    return buf_ ? strlen(buf_) : 0;
}


void BufferString::setBufSize( unsigned int newlen )
{
    if ( newlen < minlen_ )
	newlen = minlen_;
    else if ( newlen == len_ )
	return;

    if ( minlen_ > 1 )
    {
	int nrminlens = newlen / minlen_;
	if ( newlen % minlen_ ) nrminlens++;
	newlen = nrminlens * minlen_;
    }
    if ( buf_ && newlen == len_ )
	return;

    char* oldbuf = buf_;
    mTryAlloc( buf_, char [newlen] );
    if ( !buf_ )
	{ buf_ = oldbuf; return; }
    else if ( !oldbuf )
	*buf_ = '\0';
    else
    {
	int newsz = (oldbuf ? strlen( oldbuf ) : 0) + 1;
	if ( newsz > newlen )
	{
	    newsz = newlen;
	    oldbuf[newsz-1] = '\0';
	}
	memcpy( buf_, oldbuf, newsz );
    }

    delete [] oldbuf;
    len_ = newlen;
}


void BufferString::setMinBufSize( unsigned int newlen )
{
    if ( newlen < 0 ) newlen = 0;
    const_cast<unsigned int&>(minlen_) = newlen;
    setBufSize( len_ );
}


void BufferString::insertAt( int atidx, const char* str )
{
    const int cursz = size();	// Defined this to avoid weird compiler bug
    if ( atidx >= cursz )	// i.e. do not replace cursz with size()
	{ replaceAt( atidx, str ); return; }
    if ( !str || !*str )
	return;

    if ( atidx < 0 )
    {
	const int lenstr = strlen( str );
	if ( atidx <= -lenstr ) return;
	str += -atidx;
	atidx = 0;
    }

    BufferString rest( buf_ + atidx );
    *(buf_ + atidx) = '\0';
    *this += str;
    *this += rest;
}


void BufferString::replaceAt( int atidx, const char* str, bool cut )
{
    const int strsz = str ? strlen(str) : 0;
    int cursz = size();
    const int nrtopad = atidx - cursz;
    if ( nrtopad > 0 )
    {
	const int newsz = cursz + nrtopad + strsz + 1;
	setBufSize( newsz );
	char* ptr = buf_ + cursz;
	const char* stopptr = buf_ + atidx;
	while ( ptr != stopptr )
	    *ptr++ = ' ';
	buf_[atidx] = '\0';
	cursz = newsz;
    }

    if ( strsz > 0 )
    {
	if ( atidx + strsz >= cursz )
	{
	    cursz = atidx + strsz + 1;
	    setBufSize( cursz );
	    buf_[cursz-1] = '\0';
	}

	for ( int idx=0; idx<strsz; idx++ )
	{
	    const int replidx = atidx + idx;
	    if ( replidx >= 0 )
		buf_[replidx] = *(str + idx);
	}
    }

    if ( cut )
    {
	setBufSize( atidx + strsz + 1 );
	buf_[atidx + strsz] = '\0';
    }
}


bool BufferString::operator >( const char* s ) const
{ return s && buf_ ? strcmp(buf_,s) > 0 : (bool) buf_; }


bool BufferString::operator <( const char* s ) const
{ return s && buf_ ? strcmp(buf_,s) < 0 : (bool) s; }


const BufferString& BufferString::empty()
{
    static BufferString* ret = 0;
    if ( !ret )
    {
	ret = new BufferString( "0" );
	*ret->buf_ = '\0';
    }
    return *ret;
}


void BufferString::init()
{
    len_ = minlen_;
    if ( len_ < 1 )
	buf_ = 0;
    else
    {
	mTryAlloc( buf_, char[len_] );
	if ( buf_ )
	    *buf_ ='\0';
    }
}


std::ostream& operator <<( std::ostream& s, const BufferString& bs )
{
    s << bs.buf();
    return s;
}

std::istream& operator >>( std::istream& s, BufferString& bs )
{
#ifdef __msvc__
// TODO
    s >> bs.buf();
#else
    std::string stdstr; s >> stdstr;
    bs = stdstr.c_str();
#endif
    return s;
}



BufferStringSet::BufferStringSet()
    : ManagedObjectSet<BufferString>(false)
{
}



BufferStringSet::BufferStringSet( const char* arr[], int len )
    : ManagedObjectSet<BufferString>(false)
{
    if ( len < 0 )
	for ( int idx=0; arr[idx]; idx++ )
	    add( arr[idx] );
    else
	for ( int idx=0; idx<len; idx++ )
	    add( arr[idx] );
}


BufferStringSet& BufferStringSet::operator =( const BufferStringSet& bss )
{
    ManagedObjectSet<BufferString>::operator =( bss );
    return *this;
}


bool BufferStringSet::operator ==( const BufferStringSet& bss ) const
{
    if ( size() != bss.size() ) return false;

    for ( int idx=0; idx<size(); idx++ )
	if ( get(idx) != bss.get(idx) )
	    return false;

    return true;
}


int BufferStringSet::indexOf( const char* s ) const
{
    if ( !s ) s = "";
    return ::indexOf( *this, s );
}


// TODO this is crap, find a good algo
static int getMatchDist( const BufferString& bs, const char* s )
{
    const char* ptr1 = bs.buf();
    const char* ptr2 = s;
    int ret = 0;
    while ( *ptr1 && *ptr2 )
    {
	if ( toupper(*ptr1) != toupper(*ptr2) )
	    ret += 1;
	ptr1++; ptr2++;
    }
    return ret;
}


int BufferStringSet::nearestMatch( const char* s ) const
{
    if ( isEmpty() ) return -1;
    const int sz = size();
    if ( sz < 2 ) return 0;

    if ( !s || !*s )
    {
	int minsz = get(0).size(); int midx = 0;
	for ( int idx=1; idx<sz; idx++ )
	{
	    const int cursz = get(midx).size();
	    if ( cursz < minsz )
		{ minsz = cursz; midx = idx; }
	}
	return midx;
    }

    int midx = ::indexOf( *this, s );
    if ( midx >= 0 ) return midx;

    for ( midx=0; midx<sz; midx++ )
	{ if ( get(midx).isEqual(s,true) ) return midx; }
    for ( midx=0; midx<sz; midx++ )
	{ if ( get(midx).isStartOf(s,true) ) return midx; }

    int mindist = getMatchDist( get(0), s ); midx = 0;
    for ( int idx=1; idx<sz; idx++ )
    {
	const int curdist = getMatchDist( get(idx), s );
	if ( curdist < mindist )
	    { mindist = curdist; midx = idx; }
    }

    return midx;
}


BufferStringSet& BufferStringSet::add( const char* s )
{
    *this += new BufferString(s);
    return *this;
}


BufferStringSet& BufferStringSet::add( const BufferString& s )
{
    *this += new BufferString(s);
    return *this;
}


BufferStringSet& BufferStringSet::add( const BufferStringSet& bss,
				       bool allowdup )
{
    for ( int idx=0; idx<bss.size(); idx++ )
    {
	const char* s = bss.get( idx );
	if ( allowdup || indexOf(s) < 0 )
	    add( s );
    }
    return *this;
}


bool BufferStringSet::addIfNew( const char* s )
{
    if ( indexOf(s) < 0 )
	{ add( s ); return true; }
    return false;
}


bool BufferStringSet::addIfNew( const BufferString& s )
{
    return addIfNew( s.buf() );
}


int BufferStringSet::maxLength() const
{
    int ret = 0;
    for ( int idx=0; idx<size(); idx++ )
    {
	const int len = get(idx).size();
	if ( len > ret )
	    ret = len;
    }
    return ret;
}


void BufferStringSet::sort( BufferStringSet* slave )
{
    int* idxs = getSortIndexes();
    useIndexes( idxs, slave );
    delete [] idxs;
}


void BufferStringSet::useIndexes( int* idxs, BufferStringSet* slave )
{
    if ( !idxs ) return;
    const int sz = size();
    if ( sz < 2 ) return;

    ObjectSet<BufferString> tmp;
    ObjectSet<BufferString>* tmpslave = slave ? new ObjectSet<BufferString> : 0;
    for ( int idx=0; idx<sz; idx++ )
    {
	tmp += (*this)[idx];
	if ( tmpslave )
	    *tmpslave += (*slave)[idx];
    }
    ObjectSet<BufferString>::erase();
    if ( slave )
	slave->ObjectSet<BufferString>::erase();

    for ( int idx=0; idx<sz; idx++ )
    {
	*this += tmp[ idxs[idx] ];
	if ( tmpslave )
	    *slave += (*tmpslave)[ idxs[idx] ];
    }

    delete tmpslave;
}


int* BufferStringSet::getSortIndexes() const
{
    const int sz = size();
    if ( sz < 1 ) return 0;

    mGetIdxArr(int,idxs,sz)
    if ( idxs && sz > 1 )
    {
	int tmp;
	for ( int d=sz/2; d>0; d=d/2 )
	    for ( int i=d; i<sz; i++ )
		for ( int j=i-d; j>=0 && get(idxs[j])>get(idxs[j+d]); j-=d )
		    mSWAP( idxs[j+d], idxs[j], tmp )
    }

    return idxs;
}


void BufferStringSet::fillPar( IOPar& iopar ) const
{
    BufferString key;
    for( int idx=0; idx<size(); idx++ )
    { 
	key = idx;
	iopar.set( key, *(*this)[idx] );
    }
}


void BufferStringSet::usePar( const IOPar& iopar )
{
    BufferString key;
    for( int idx=0; ;idx++ )
    { 
	key = idx;
	if ( !iopar.find(key) ) return;
	add( iopar[key] );
    }
}


bool FixedString::operator==( const char* s ) const
{
    if ( ptr_==s )
	return true;

    if ( !ptr_ || !s )
	return false;

    return !strcmp(ptr_,s);
}


int FixedString::size() const
{
    if ( !ptr_ ) return 0;
    return strlen( ptr_ );
}
