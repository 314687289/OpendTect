/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2003
-*/

static const char* rcsID = "$Id: safefileio.cc,v 1.2 2005-04-08 11:37:36 cvsbert Exp $";

#include "safefileio.h"
#include "filegen.h"
#include "filepath.h"
#include "strmprov.h"
#include "dateinfo.h"
#include "hostdata.h"
#include "timefun.h"
#include "errh.h"


SafeFileIO::SafeFileIO( const char* fnm, bool l )
    	: filenm_(fnm)
    	, locked_(l)
    	, removebakonsuccess_(false)
    	, usebakwhenmissing_(true)
    	, lockretries_(10)
    	, lockwaitincr_(0.5)
    	, allowlockremove_(true)
{
    FilePath fp( filenm_ );
    const_cast<BufferString&>(filenm_) = fp.fullPath();

    const BufferString filenmonly( fp.fileName() );
    fp.setFileName( 0 );

    BufferString curfnm( ".lock." ); curfnm += filenmonly;
    FilePath lockfp( fp ); lockfp.add( curfnm );
    const_cast<BufferString&>(lockfnm_) = lockfp.fullPath();

    curfnm = filenmonly; curfnm += ".bak";
    FilePath bakfp( fp ); bakfp.add( curfnm );
    const_cast<BufferString&>(bakfnm_) = bakfp.fullPath();

    curfnm = filenmonly; curfnm += ".new";
    FilePath newfp( fp ); newfp.add( curfnm );
    const_cast<BufferString&>(newfnm_) = newfp.fullPath();
}


bool SafeFileIO::open( bool forread, bool ignorelock )
{
    return forread ? openRead( ignorelock ) : openWrite( ignorelock );
}


bool SafeFileIO::openRead( bool ignorelock )
{
    if ( locked_ && !ignorelock && !waitForLock() )
	return false;
    mkLock( true );

    const char* toopen = filenm_.buf();
    if ( File_isEmpty(toopen) )
    {
	if ( usebakwhenmissing_ )
	    toopen = bakfnm_.buf();
	if ( File_isEmpty(toopen) )
	{
	    errmsg_ = "Input file '"; errmsg_ += filenm_;
	    errmsg_ += "' is not present or empty";
	    rmLock();
	    return false;
	}
	else
	{
	    errmsg_ = "Using backup file '";
	    errmsg_ += bakfnm_; errmsg_ += "'";
	    UsrMsg( errmsg_.buf(), MsgClass::Warning );
	    errmsg_ = "";
	}
    }

    sd_ = StreamProvider( toopen ).makeIStream();
    if ( !sd_.usable() )
    {
	errmsg_ = "Cannot open '"; errmsg_ += toopen;
	errmsg_ += "' for read";
	rmLock();
	return false;
    }

    return true;
}


bool SafeFileIO::openWrite( bool ignorelock )
{
    if ( locked_ && !ignorelock && !waitForLock() )
	return false;
    mkLock( false );

    if ( File_exists( newfnm_.buf() ) )
	File_remove( newfnm_.buf(), NO );

    sd_ = StreamProvider( newfnm_.buf() ).makeOStream();
    if ( !sd_.usable() )
    {
	errmsg_ = "Cannot open '"; errmsg_ += newfnm_;
	errmsg_ += "' for write";
	rmLock();
	return false;
    }

    return true;
}


bool SafeFileIO::commitWrite()
{
    if ( File_isEmpty(newfnm_) )
    {
	errmsg_ = "File '"; errmsg_ += filenm_;
	errmsg_ += "' not overwritten with empty new file";
	return false;
    }
    if ( !File_rename( filenm_, bakfnm_ ) )
    {
	errmsg_ = "Cannot create backup file '";
	errmsg_ += bakfnm_; errmsg_ += "'";
	UsrMsg( errmsg_.buf(), MsgClass::Warning );
	errmsg_ = "";
    }
    if ( !File_rename( newfnm_, filenm_ ) )
    {
	errmsg_ = "Changes in '"; errmsg_ += filenm_;
	errmsg_ += "' could not be commited.";
	return false;
    }
    if ( removebakonsuccess_ )
	File_remove( bakfnm_, NO );

    return true;
}


bool SafeFileIO::doClose( bool keeplock, bool docommit )
{
    bool isread = sd_.istrm;
    sd_.close();
    bool res = true;
    if ( isread || !docommit )
    {
	if ( !isread && File_exists(newfnm_) )
	    File_remove( newfnm_, NO );
    }
    else
	res = commitWrite();

    if ( !keeplock )
	rmLock();

    return res;
}


bool SafeFileIO::haveLock() const
{
    //TODO read the lock file to see whether date and time are feasible
    return File_exists( lockfnm_.buf() );
}


bool SafeFileIO::waitForLock() const
{
    bool havelock = haveLock();
    if ( !havelock )
	return true;

    for ( int idx=0; havelock && idx<lockretries_; idx++ )
    {
	Time_sleep( lockwaitincr_ );
	havelock = haveLock();
    }

    if ( !havelock )
	return true;

    if ( allowlockremove_ )
    {
	File_remove( lockfnm_, NO );
	return true;
    }

    errmsg_ = "File '"; errmsg_ += filenm_;
    errmsg_ = "' is currently locked.\n";
    return false;
}


void SafeFileIO::mkLock( bool forread )
{
    if ( !locked_ ) return;

    StreamData sd = StreamProvider(lockfnm_).makeOStream();
    if ( sd.usable() )
    {
	DateInfo di; BufferString datestr; di.getFullDisp( datestr );
	*sd.ostrm << "Type: " << (forread ? "Read\n" : "Write\n");
	*sd.ostrm << "Date: " << datestr << " (" << di.key() << ")\n";
	*sd.ostrm << "Host: " << HostData::localHostName() << '\n';
	*sd.ostrm << "Process: " << getPID() << '\n';
	const char* ptr = GetPersonalDir();
	*sd.ostrm << "User's HOME: " << (ptr ? ptr : "<none>") << '\n';
	ptr = GetSoftwareUser();
	*sd.ostrm << "DTECT_USER: " << (ptr ? ptr : "<none>") << '\n';
	sd.ostrm->flush();
    }
    sd.close();
}


void SafeFileIO::rmLock()
{
    if ( locked_ && File_exists(lockfnm_) )
	File_remove( lockfnm_, NO );
}
