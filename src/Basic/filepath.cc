/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2004
-*/

static const char* rcsID = "$Id: filepath.cc,v 1.24 2009-02-13 13:31:15 cvsbert Exp $";

#include "filepath.h"
#include "envvars.h"
#include "msgh.h"
#include "string2.h"
#include <iostream>
#include <time.h>

#include "winutils.h"

const char* FilePath::sPrefSep = ":";

FilePath::FilePath( const char* fnm )
{ set( fnm ); }


FilePath::FilePath( const FilePath& fp )
{ *this = fp; }


FilePath& FilePath::operator =( const FilePath& fp )
{
    lvls_ = fp.lvls_;
    prefix_ = fp.prefix_;
    isabs_ = fp.isabs_;
    return *this;
}


FilePath& FilePath::operator =( const char* fnm )
{ return (*this = FilePath(fnm)); }


bool FilePath::operator ==( const FilePath& fp ) const
{
    return   lvls_ == fp.lvls_ && prefix_ == fp.prefix_ && isabs_ == fp.isabs_; 
}


bool FilePath::operator ==( const char* fnm ) const
{ return *this == FilePath(fnm); }


bool FilePath::operator !=( const FilePath& fp ) const
{ return !(*this == fp); }


bool FilePath::operator != ( const char* fnm ) const
{ return !(*this == FilePath(fnm)); }


FilePath& FilePath::set( const char* _fnm )
{
    lvls_.erase(); prefix_ = ""; isabs_ = false;
    if ( !_fnm ) return *this;

    BufferString __fnm
#ifdef __win__
		      ( getCleanWinPath(_fnm) );
#else
		      ( _fnm );
#endif

    const char* fnm = __fnm.buf();

    mSkipBlanks( fnm );
    if ( !*fnm ) return *this;

    const char* ptr = strchr( fnm, *sPrefSep );
    if ( ptr )
    {
	const char* dsptr = strchr( fnm, *dirSep(Local) );
	if ( dsptr > ptr )
	{
	    prefix_ = fnm;
	    *strchr( prefix_.buf(), *sPrefSep ) = '\0';
	    fnm = ptr + 1;
	}
    }

    isabs_ = *fnm == '\\' || *fnm == '/';

    if ( isabs_ ) fnm++;
    addPart( fnm );
    compress();
    return *this;
}


FilePath& FilePath::add( const char* fnm )
{
    if ( !fnm ) return *this;
    int sl = lvls_.size();
    addPart( fnm );
    compress( sl );
    return *this;
}


FilePath& FilePath::insert( const char* fnm )
{
    if ( !fnm || !*fnm ) return *this;
    ObjectSet<BufferString> oldlvls;
    oldlvls.append( lvls_ );
    lvls_.ObjectSet<BufferString>::erase();
    set( fnm );
    lvls_.ObjectSet<BufferString>::append( oldlvls );
    return *this;
}


void FilePath::setFileName( const char* fnm )
{
    if ( !fnm || !*fnm )
    {
	if ( lvls_.size() )
	    delete lvls_.remove( lvls_.size()-1 );
    }
    else if ( lvls_.isEmpty() )
	add( fnm );
    else
    {
	*lvls_[lvls_.size()-1] = fnm;
	compress( lvls_.size()-1 );
    }
}


void FilePath::setPath( const char* pth )
{
    BufferString fnm( lvls_.size() ? lvls_.get(lvls_.size()-1) : 0 );
    set( pth );
    if ( !fnm.isEmpty() )
	add( fnm );
}


void FilePath::setExtension( const char* ext, bool replace )
{
    if ( !ext ) ext = "";
    mSkipBlanks( ext );

    if ( *ext == '.' )
	ext++;
    if ( lvls_.size() < 1 )
    {
	if ( *ext )
	    add( ext );
	return;
    }

    BufferString& fname = *lvls_[lvls_.size()-1];
    char* ptr = strrchr( fname.buf(), '.' );
    if ( ptr && replace )
	strcpy( *ext ? ptr+1 : ptr, ext );
    else if ( *ext )
	{ fname += "."; fname += ext; }
}


bool FilePath::isAbsolute() const
{ return isabs_; }


BufferString FilePath::fullPath( Style f, bool cleanup ) const
{
    const BufferString res = dirUpTo(-1);
    return cleanup ? mkCleanPath(res,f) : res;
}


const char* FilePath::prefix() const
{ return prefix_.buf(); }


int FilePath::nrLevels() const
{ return lvls_.size(); }


const char* FilePath::extension() const
{
    if ( lvls_.isEmpty() )
	return 0;

    const char* ret = strrchr( fileName().buf(), '.' );
    if ( ret ) ret++;
    return ret;
}


const BufferString& FilePath::fileName() const
{ return dir(-1); }


BufferString FilePath::pathOnly() const
{ return dirUpTo(lvls_.size()-2); }


const BufferString& FilePath::dir( int nr ) const
{
    if ( nr < 0 || nr >= lvls_.size() )
	nr = lvls_.size()-1;
    return nr < 0 ? BufferString::empty() : *lvls_[nr];
}


BufferString FilePath::dirUpTo( int lvl ) const
{
    if ( lvl < 0 || lvl >= lvls_.size() )
	lvl = lvls_.size() - 1;

    BufferString ret;
    if ( *prefix_.buf() )
    {
	ret = prefix_;
	ret += sPrefSep;
    }
    if ( lvl < 0 )
	return ret;

    if ( isabs_ )
	ret += dirSep(Local);
    if ( lvls_.size() )
	ret += lvls_.get( 0 );

    for ( int idx=1; idx<=lvl; idx++ )
    {
	ret += dirSep(Local);
	ret += lvls_.get( idx );
    }

    return ret;
}


BufferString FilePath::getTempDir()
{
    BufferString fname;

#ifdef __win__

    if ( GetEnvVar("TMP") )
	fname = GetEnvVar( "TMP" );
    else if ( GetEnvVar("TEMP") )
	fname = GetEnvVar( "TEMP" );
    else // make sure we have at least write access...
    {
	fname = GetSpecialFolderLocation( CSIDL_PERSONAL );
	static bool warn = true;
	if ( warn )
	{
	    BufferString msg( "WARNING: You don't have the TEMP or TMP "
		    	      "environment variable set.\nUsing directory '" );
	    		 msg += fname; msg += "'.";
	    UsrMsg( msg );
	    warn = false;
	}
    }

#else

    fname = "/tmp";

#endif

    return fname;
}


BufferString FilePath::getTempName( const char* ext )
{
    FilePath fp( getTempDir() );

    BufferString fname( "od" );
    static int counter = 0;
    int time_stamp = time( (time_t*)0 ) + counter++;
    char uniquestr[80];
    sprintf( uniquestr, "%X%X", GetPID(), (int)time_stamp );
    fname += uniquestr;

    if ( ext && *ext )
    {
	fname += ".";
	fname += ext;
    }

    fp.add( fname );
    return fp.fullPath();
}


BufferString FilePath::mkCleanPath(const char* path, Style stl)
{
    if ( stl == Local )		stl = __iswin__ ? Windows : Unix;

    BufferString ret;
    if ( stl == Windows )	ret = getCleanWinPath( path ) ;
    else			ret = getCleanUnxPath( path ) ;

    return ret;
}


const char* FilePath::dirSep( Style stl )
{
    static const char* wds = "\\";
    static const char* uds = "/";

    if ( stl == Local )
	stl = __iswin__ ? Windows : Unix;

    return stl == Windows ? wds : uds;
} 


void FilePath::addPart( const char* fnm )
{
    if ( !fnm ) return;

    mSkipBlanks( fnm );
    char prev = ' ';
    char buf[mMaxFilePathLength];
    char* bufptr = buf;
    bool remdblsep = false;

    while ( *fnm )
    {
	char cur = *fnm;

	if ( cur != *dirSep(Local) )
	    remdblsep = true;
	else
	{
	    if ( prev != *dirSep(Local) || !remdblsep )
	    {
		*bufptr = '\0';
		if ( buf[0] ) lvls_.add( buf );
		bufptr = buf;
		*bufptr = '\0';
	    }
	    fnm++;
	    continue;
	}

	*bufptr++ = cur;
	fnm++;
	prev = cur;
    }
    *bufptr = '\0';
    if ( buf[0] ) lvls_.add( buf );
}


void FilePath::compress( int startlvl )
{
    for ( int idx=startlvl; idx<lvls_.size(); idx++ )
    {
	const BufferString& bs = *lvls_[idx];
	int remoffs = 99999;
	if ( bs == "." )
	    remoffs = 0;
	else if ( bs == ".." && idx > 0 && *lvls_[idx-1] != ".." )
	    remoffs = 1;

	if ( idx-remoffs >= 0 )
	{
	    lvls_.remove( idx-remoffs, idx );
	    idx -= remoffs + 1;
	}
    }
}
