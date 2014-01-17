/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2009
-*/

static const char* rcsID mUsedVar = "$Id$";
static const char* rcsPrStr = "$Revision$ $Date$";

#include "odusgserver.h"
#include "odusgbaseadmin.h"
#include "odusginfo.h"

#include "ascstream.h"
#include "dirlist.h"
#include "iopar.h"
#include "oddirs.h"
#include "ptrman.h"
#include "od_iostream.h"
#include "thread.h"
#include "timefun.h"
#include "genc.h"
#include "perthreadrepos.h"

const char* Usage::Server::sKeyPort()		{ return "Port"; }
const char* Usage::Server::sKeyFileBase()	{ return "Usage"; }
int Usage::Server::cDefaulPort()		{ return mUsgServDefaulPort; }


Usage::Server::Server( const IOPar* inpars, od_ostream& strm )
    : logstrm_(strm)
    , pars_(inpars ? *inpars : *new IOPar)
    , port_(mUsgServDefaulPort)
    , thread_(0)
{
    if ( !inpars )
    {
	inpars = getPars();
	if ( inpars ) const_cast<IOPar&>(pars_) = *inpars;
    }

    if ( pars_.isEmpty() )
    {
	logstrm_ << "Cannot start OpendTect Usage server (" << rcsPrStr
		 << "):\n";
	if ( inpars )
	    logstrm_ << "No input parameters" << od_newline;
	else
	    logstrm_ << "Cannot read: " << setupFileName(0) << od_newline;
	return;
    }
    usePar();

    logstrm_ << "OpendTect Usage server (" << rcsPrStr << ")" << od_newline;
    logstrm_ << "\non " << GetLocalHostName();
    if ( port_ > 0 )
	logstrm_ << " (port: " << port_ << ")";
    logstrm_ << "\nStarted: " << Time::getDateTimeString()
	     << od_newline << od_endl;

    Administrator::setLogStream( &logstrm_ );
}


Usage::Server::~Server()
{
    delete const_cast<IOPar*>( &pars_ );
    if ( thread_ )
	{ thread_->waitForFinish(); delete thread_; }
}


const char* Usage::Server::setupFileName( const char* admnm )
{
    mDeclStaticString( ret );
    ret = GetSetupDataFileName(ODSetupLoc_ApplSetupPref,sKeyFileBase(),0);
    if ( admnm && *admnm )
    {
	BufferString clnadmnm( admnm );
	clnadmnm.clean();
	ret += "_"; ret += clnadmnm;
    }
    return ret.buf();
}


IOPar* Usage::Server::getPars()
{
    od_istream strm( setupFileName(0) );
    if ( !strm.isOK() )
	return 0;

    IOPar* iop = new IOPar( "Usage Monitor settings" );
    ascistream astrm( strm );
    iop->getFrom( astrm );
    if ( iop->isEmpty() )
	{ delete iop; iop = 0; }

    return iop;
}


void Usage::Server::usePar()
{
    pars_.get( sKeyPort(), port_ );
}


bool Usage::Server::go( bool inthr )
{
    if ( !inthr )
	return doWork( 0 );

    Threads::Locker lckr( threadlock_ );
    thread_ = new Threads::Thread( mCB(this,Usage::Server,doWork) );
    return true;
}


void Usage::Server::addInfo( Usage::Info& inf )
{
    if ( !thread_ )
	infos_ += &inf;
    else
    {
	Threads::Locker lckr( threadlock_ );
	infos_ += &inf;
    }
}


void Usage::Server::getRemoteInfos()
{
    //TODO get infos from socket
}


bool Usage::Server::doWork( CallBacker* )
{
    Threads::Locker* lckr = 0;
    while ( true )
    {
	if ( thread_ )
	    lckr = new Threads::Locker( threadlock_ );
	else
	    getRemoteInfos();

	while ( !infos_.isEmpty() )
	{
	    PtrMan<Usage::Info> inf = infos_[0];
	    infos_ -= inf;
	    if ( inf->group_.isEmpty() && inf->action_ == "Quit" )
		return inf->delim_ == Usage::Info::Start;

	    Administrator::dispatch( *inf );
	    if ( inf->withreply_ )
		sendReply( *inf );
	}

	if ( lckr )
	    { delete lckr; lckr = 0; }

	Threads::sleep( 0.1 );
    }
}


void Usage::Server::sendReply( const Usage::Info& inf )
{
    //TODO implement
}
