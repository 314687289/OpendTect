/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 2-8-1994
-*/

static const char* rcsID = "$Id: iodir.cc,v 1.26 2006-08-16 12:23:25 cvsnanne Exp $";

#include "iodir.h"
#include "iolink.h"
#include "ioman.h"
#include "ascstream.h"
#include "separstr.h"
#include "safefileio.h"
#include "errh.h"
#include "timefun.h"
#include "filegen.h"
#include "filepath.h"


IODir::IODir( const char* dirnm )
	: isok_(false)
	, dirname_(dirnm)
	, curid_(0)
{
    if ( build() ) isok_ = true;
}


IODir::IODir()
	: isok_(false)
	, curid_(0)
{
}


IODir::IODir( const MultiID& ky )
	: isok_(false)
	, curid_(0)
{
    IOObj* ioobj = getObj( ky );
    if ( !ioobj ) return;
    dirname_ = ioobj->dirName();
    delete ioobj;

    if ( build() ) isok_ = true;
}


IODir::~IODir()
{
    deepErase(objs_);
}


//TODO remove this crap and below when release 3.0 appears
// Is just there to keep beta-release users happy
#include "iopar.h"
static bool needwriteomf_hack = false;
static void changeHor2Chronostrat( IOObj* ioobj )
{
    if ( ioobj && strcmp(ioobj->group(),"Horizon") ) return;

    const char* typstr = ioobj->pars().find( "Type" );
    if ( !typstr || *typstr != 'C' ) return;

    ioobj->setTranslator( "ChronoStrat" );
    ioobj->pars().removeWithKey( "Type" );
    needwriteomf_hack = true;
}


bool IODir::build()
{
    needwriteomf_hack = false;
    bool rv = doRead( dirname_, this );
    if ( rv && needwriteomf_hack )
	doWrite();
    needwriteomf_hack = false;
    return rv;
}


IOObj* IODir::getMain( const char* dirnm )
{
    return doRead( dirnm, 0, 1 );
}


const IOObj* IODir::main() const
{
    for ( int idx=0; idx<objs_.size(); idx++ )
    {
	const IOObj* ioobj = objs_[idx];
	if ( ioobj->myKey() == 1 ) return ioobj;
    }
    return 0;
}


#undef mErrRet
#define mErrRet() \


IOObj* IODir::doRead( const char* dirnm, IODir* dirptr, int needid )
{
    FilePath fp( dirnm ); fp.add( ".omf" );
    SafeFileIO sfio( fp.fullPath(), false );
    if ( !sfio.open(true) )
    {
	BufferString msg( "\nError during read of Object Management info!" );
	msg += "\n-> Please check directory (read permissions, existence):\n'";
	msg += dirnm; msg += "'";
	ErrMsg( msg );
	return false;
    }

    IOObj* ret = readOmf( sfio.istrm(), dirnm, dirptr, needid );
    if ( ret )
	sfio.closeSuccess();
    else
	sfio.closeFail();
    return ret;
}


IOObj* IODir::readOmf( std::istream& strm, const char* dirnm,
			IODir* dirptr, int needid )
{
    ascistream astream( strm );
    astream.next();
    FileMultiString fms( astream.value() );
    MultiID dirky( fms[0] );
    if ( dirky == "0" ) dirky = "";
    if ( dirptr )
    {
	dirptr->key_ = dirky;
	dirptr->curid_ = atoi(fms[1]);
	if ( dirptr->curid_ == IOObj::tmpID )
	    dirptr->curid_ = 1;
    }
    astream.next();

    IOObj* retobj = 0;
    while ( astream.type() != ascistream::EndOfFile )
    {
	IOObj* obj = IOObj::get(astream,dirnm,dirky);
	if ( !obj || obj->bad() ) { delete obj; continue; }

	MultiID ky( obj->key() );
	int id = ky.ID( ky.nrKeys()-1 );

	if ( dirptr )
	{
	    changeHor2Chronostrat( obj );
	    retobj = obj;
	    if ( id == 1 ) dirptr->setLinked( obj );
	    dirptr->addObj( obj, false );
	    if ( id < 99999 && id > dirptr->curid_ )
		dirptr->curid_ = id;
	}
	else
	{
	    if ( id != needid )
		delete obj;
	    else
	    {
		changeHor2Chronostrat( obj );
		retobj = obj;
		retobj->setStandAlone( dirnm );
		break;
	    }
	}
    }

    return retobj;
}


IOObj* IODir::getObj( const MultiID& ky )
{
    FileNameString dirnm( IOM().rootDir() );
    int nrkeys = ky.nrKeys();
    for ( int idx=0; idx<nrkeys; idx++ )
    {
	int id = ky.ID( idx );
	IOObj* ioobj = doRead( dirnm, 0, id );
	if ( !ioobj || idx == nrkeys-1 ) return ioobj;
	dirnm = ioobj->dirName();
	delete ioobj;
    }

    return 0;
}


const IOObj* IODir::operator[]( const char* ky ) const
{
    for ( int idx=0; idx<objs_.size(); idx++ )
    {
	const IOObj* ioobj = objs_[idx];
	if ( ioobj->name() == ky )
	    return ioobj;
    }
    return 0;
}


const IOObj* IODir::operator[]( const MultiID& ky ) const
{
    for ( int idx=0; idx<objs_.size(); idx++ )
    {
	const IOObj* ioobj = objs_[idx];
	if ( ioobj->key() == ky
	  || ( ioobj->isLink() && ((IOLink*)ioobj)->link()->key() == ky ) )
		return ioobj;
	
    }

    return 0;
}


bool IODir::create( const char* dirnm, const MultiID& ky, IOObj* mainobj )
{
    if ( !dirnm || !*dirnm || !mainobj ) return false;
    mainobj->key_ = ky;
    mainobj->key_ += getStringFromInt( 0, 1 );
    IODir dir;
    dir.dirname_ = dirnm;
    dir.key_ = ky;

    dir.objs_ += mainobj;
    dir.isok_ = true;
    bool ret = dir.doWrite();
    dir.objs_ -= mainobj;
    dir.isok_ = false;
    return ret;
}


void IODir::reRead()
{
    deepErase(objs_);
    curid_ = 0;
    if ( build() ) isok_ = true;
}


bool IODir::permRemove( const MultiID& ky )
{
    reRead();
    if ( bad() ) return false;

    int sz = objs_.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	IOObj* obj = objs_[idx];
	if ( obj->key() == ky )
	{
	    objs_ -= obj;
	    delete obj;
	    break;
	}
    }
    return doWrite();
}


bool IODir::commitChanges( const IOObj* ioobj )
{
    if ( ioobj->isLink() )
    {
	IOObj* obj = (IOObj*)(*this)[ioobj->key()];
	if ( obj != ioobj ) obj->copyFrom( ioobj );
	return doWrite();
    }

    IOObj* clone = ioobj->clone();
    if ( !clone ) return false;
    reRead();
    if ( bad() ) { delete clone; return false; }

    int sz = objs_.size();
    bool found = false;
    for ( int idx=0; idx<sz; idx++ )
    {
	IOObj* obj = objs_[idx];
	if ( obj->key() == clone->key() )
	{
	    delete objs_.replace( idx, clone );
	    found = true;
	    break;
	}
    }
    if ( !found )
	objs_ += clone;
    return doWrite();
}


bool IODir::addObj( IOObj* ioobj, bool persist )
{
    if ( persist )
    {
	reRead();
	if ( bad() ) return false;
    }
    if ( (*this)[ioobj->key()] )
	ioobj->setKey( newKey() );

    mkUniqueName( ioobj );
    objs_ += ioobj;
    return persist ? doWrite() : true;
}


bool IODir::mkUniqueName( IOObj* ioobj )
{
    if ( (*this)[ioobj->name()] )
    {
	int nr = 1;
	UserIDString nm;

	do {
	    nr++;
	    nm = ioobj->name();
	    nm += " "; nm += nr;
	} while ( (*this)[nm] );

	ioobj->setName( nm );
	return true;
    }
    return false;
}


#define mAddDirWarn(msg) \
    msg += "\n-> Please check write permissions for directory:\n   "; \
    msg += dirname_


#undef mErrRet
#define mErrRet() \
{ \
    BufferString msg( "\nError during write of Object Management info!" ); \
    mAddDirWarn(msg); \
    ErrMsg( msg ); \
    return false; \
}


bool IODir::wrOmf( std::ostream& strm ) const
{
    ascostream astream( strm );
    if ( !astream.putHeader( "Object Management file" ) )
	mErrRet()
    FileMultiString fms( key_ == "" ? "0" : (const char*)key_ );
    for ( int idx=0; idx<objs_.size(); idx++ )
    {
	const MultiID currentkey = objs_[idx]->key();
	int curleafid = currentkey.leafID();
	if ( curleafid != IOObj::tmpID && curleafid > curid_ )
	    curid_ = curleafid;
    }
    fms += curid_;
    astream.put( "ID", fms );
    astream.newParagraph();

    // First the main obj
    const IOObj* mymain = main();
    if ( mymain && !mymain->put(astream) )
	mErrRet()

    // Then the links
    for ( int idx=0; idx<objs_.size(); idx++ )
    {
	const IOObj* obj = objs_[idx];
	if ( obj == mymain ) continue;
	if ( obj->isLink() && !obj->put(astream) )
	    mErrRet()
    }
    // Then the normal objs
    for ( int idx=0; idx<objs_.size(); idx++ )
    {
	const IOObj* obj = objs_[idx];
	if ( obj == mymain ) continue;
	if ( !obj->isLink() && !obj->put(astream) )
	    mErrRet()
    }

    return true;
}


#undef mErrRet
#define mErrRet(addsfiomsg) \
{ \
    BufferString msg( "\nError during write of Object Management info!" ); \
    mAddDirWarn(msg); \
    if ( addsfiomsg ) \
	{ msg += "\n"; msg += sfio.errMsg(); } \
    ErrMsg( msg ); \
    return false; \
}

bool IODir::doWrite() const
{
    FilePath fp( dirname_ ); fp.add( ".omf" );
    SafeFileIO sfio( fp.fullPath(), false );
    if ( !sfio.open(false) )
	mErrRet(true)

    if ( !wrOmf(sfio.ostrm()) )
    {
	sfio.closeFail();
	mErrRet(false)
    }

    if ( !sfio.closeSuccess() )
	mErrRet(true)

    return true;
}


MultiID IODir::newKey() const
{
    MultiID id = key_;
    ((IODir*)this)->curid_++;
    id += getStringFromInt( 0, curid_ );
    return id;
}
