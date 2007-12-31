/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : June 2007
-*/

static const char* rcsID = "$Id: madio.cc,v 1.4 2007-12-31 15:57:58 cvsbert Exp $";

#include "madio.h"
#include "keystrs.h"
#include "filegen.h"
#include "filepath.h"
#include "strmprov.h"
#include "envvars.h"
#include "oddirs.h"
#include "iopar.h"

const char* ODMad::FileSpec::sKeyMaskFile = "Mask File Name";
const char* ODMad::sKeyMadagascar = "Madagascar";
const char* ODMad::sKeyMadSelKey = "909090";


ODMad::FileSpec::FileSpec( bool fr )
    : forread_(fr)
{
}


bool ODMad::FileSpec::fileNameOK( const char* fnm ) const
{
    if ( !forread_ )
    {
	FilePath fp( fnm );
	if ( !File_isWritable(fp.pathOnly()) )
	{
	    errmsg_ = "Directory '";
	    errmsg_ += fp.pathOnly();
	    errmsg_ += "' is not writable";
	    return false;
	}
    }
    else if ( fnm && *fnm && File_isEmpty(fnm) )
    {
	errmsg_ = "File '";
	errmsg_ += fnm;
	errmsg_ += "' is not readable";
	return false;
    }

    return true;
}


bool ODMad::FileSpec::set( const char* fnm, const char* maskfnm )
{
    if ( !fnm || !*fnm )
	{ errmsg_ = "No file name provided"; return false; }
    FilePath fp( fnm );
    if ( !fp.isAbsolute() )
	fp.set( GetDataDir() ).add( sKeyMadagascar ).add( fnm );
    fnm_ = fp.fullPath();

    if ( !maskfnm || !*maskfnm )
	maskfnm_.setEmpty();
    else
    {
	fp.set( maskfnm );
	if ( !fp.isAbsolute() )
	    fp.set( GetDataDir() ).add( sKeyMadagascar ).add( maskfnm );
	maskfnm_ = fp.fullPath();
    }

    return fileNameOK( fnm_ ) && fileNameOK( maskfnm_ );
}


void ODMad::FileSpec::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type, forread_ ? "Read" : "Write" );
    iop.set( sKey::FileName, fnm_ );
    iop.set( sKeyMaskFile, maskfnm_ );
}


bool ODMad::FileSpec::usePar( const IOPar& iop )
{
    const char* res = iop.find( sKey::Type );
    forread_ = !res || (*res != 'w' && *res != 'W');

    BufferString fnm = fnm_, maskfnm = maskfnm_;
    iop.get( sKey::FileName, fnm );
    iop.get( sKeyMaskFile, maskfnm );
    return set( fnm, maskfnm );
}


const char* ODMad::FileSpec::defPath()
{
    static BufferString* ret = 0;
    if ( ret ) return ret->buf();

    FilePath fp( GetDataDir() ); fp.add( sKeyMadagascar );
    ret = new BufferString( fp.fullPath() );
    return ret->buf();
}


const char* ODMad::FileSpec::madDataPath()
{
    static BufferString* ret = 0;
    if ( ret ) return ret->buf();

    ret = new BufferString( GetEnvVar("DATAPATH") );
    if ( ret->isEmpty() )
	*ret = defPath();
    return ret->buf();
}


StreamData ODMad::FileSpec::open() const
{
    return doOpen( fnm_ );
}


StreamData ODMad::FileSpec::openMask() const
{
    return maskfnm_.isEmpty() ? StreamData() : doOpen( maskfnm_ );
}


#define mErrRet(s1,s2,s3) \
	{ errmsg_ = s1; errmsg_+= s2; errmsg_ += s3; return StreamData(); }

StreamData ODMad::FileSpec::doOpen( const char* fnm ) const
{
    if ( forread_ )
    {
	if ( !File_exists(fnm) )
	    mErrRet("File '",fnm,"' does not exist")
	else if ( File_isEmpty(fnm) )
	    mErrRet("File '",fnm,"' is empty")
    }
    else
    {
	FilePath fp( fnm );
	const BufferString dirnm( fp.pathOnly() );
	if ( !File_isDirectory(dirnm) )
	    mErrRet("Directory '",dirnm,"' does not exist")
	else if ( !File_isWritable(dirnm) )
	    mErrRet("Directory '",dirnm,"' is not writable")
    }

    StreamData ret = forread_ ? StreamProvider(fnm).makeIStream()
			      : StreamProvider(fnm).makeOStream();
    if ( !ret.usable() )
	mErrRet("File '",fnm,"' cannot be opened")

    return ret;
}
