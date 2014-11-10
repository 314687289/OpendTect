/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          June 2004
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiseisioobjinfo.h"

#include "cbvsreadmgr.h"
#include "ioobj.h"
#include "ptrman.h"
#include "seiscbvs.h"
#include "systeminfo.h"
#include "od_ostream.h"
#include "uimsg.h"
#include "od_strstream.h"


uiSeisIOObjInfo::uiSeisIOObjInfo( const IOObj& ioobj, bool errs )
	: sii(ioobj)
	, doerrs(errs)
{
}


uiSeisIOObjInfo::uiSeisIOObjInfo( const MultiID& key, bool errs )
	: sii(key)
	, doerrs(errs)
{
}


#define mChk(ret) if ( !isOK() ) return ret

bool uiSeisIOObjInfo::provideUserInfo() const
{
    mChk(false);
    if ( is2D() )
	return true;

    PtrMan<Translator> t = ioObj()->createTranslator();
    if ( !t )
	{ pErrMsg("No Translator"); return true; }
    mDynamicCastGet(CBVSSeisTrcTranslator*,trans,t.ptr());
    if ( !trans )
	{ return true; }

    Conn* conn = ioObj()->getConn( Conn::Read );
    if ( !conn || !trans->initRead(conn) )
    {
	if ( doerrs )
	    uiMSG().error( tr("No output cube produced") );
	return false;
    }

    od_ostrstream strm;
    strm << "The cube is available for work.\n\n";
    trans->readMgr()->dumpInfo( strm, false );
    uiMSG().message( strm.result() );

    return true;
}


bool uiSeisIOObjInfo::checkSpaceLeft( const SeisIOObjInfo::SpaceInfo& si ) const
{
    mChk(false);

    const int szmb = expectedMBs( si );
    if ( szmb < 0 ) // Unknown, but probably small
	return true;

    const int avszmb = System::getFreeMBOnDisk( *ioObj() );
#ifdef __win__
    const int szgb = szmb / 1024;
    if ( szgb >= 4 )
    {
	BufferString fsysname = System::getFileSystemName( ioObj()->dirName() );
	if ( fsysname == "FAT32" )
	{
	    uiMSG().error( tr("Target directory has a FAT32 File System.\n"
			      "Files larger than 4GB are not supported") );
	    return false;
	}
    }
#endif

    if ( avszmb == 0 )
    {
	if ( !doerrs ) return false;
	if ( !uiMSG().askContinue( tr("The output disk seems to be full.\n"
				      "Do you want to continue?") ) )
	    return false;
    }
    else if ( szmb > avszmb )
    {
	if ( !doerrs ) return false;
	uiString msg = tr( "The new cube size may exceed the space "
			   "available on disk:\n%1\nDo you want to continue?" );

	uiString explanationmsg = avszmb == 0 ? tr("The disk seems to be full!")
					      : tr("\nEstimated size: %1 MB\n"
						   "Available on disk: %2 MB")
					      .arg(szmb).arg(avszmb);
	
	msg.arg(explanationmsg);
	
	if ( !uiMSG().askContinue( msg ) )
	    return false;
    }
    return true;
}
