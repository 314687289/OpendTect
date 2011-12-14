/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          August 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: remjobexec.cc,v 1.5 2011-12-14 13:16:41 cvsbert Exp $";


#include "remjobexec.h"

#include "filepath.h"
#include "iopar.h"
#include "oddirs.h"
#include "strmprov.h"
#include "tcpsocket.h"

#define mErrRet( s ) { uiErrorMsg( s ); exit(0); }


RemoteJobExec::RemoteJobExec( const char* host, const int port )
    : socket_(*new TcpSocket)
    , host_(host)
    , par_(*new IOPar)
    , isconnected_(false)
{
    socket_.connectToHost( host_, port );
    socket_.connected.notify( mCB(this,RemoteJobExec,connectedCB) ); 
    socket_.waitForConnected( 2000 );
    ckeckConnection();
}


RemoteJobExec::~RemoteJobExec()
{
    socket_.disconnectFromHost();
    delete &socket_;
    delete &par_;
}


bool RemoteJobExec::launchProc() const
{ 
    if ( !par_.isEmpty() )
	return socket_.write( par_ );

    return false;
}


void RemoteJobExec::addPar( const IOPar& par )
{ par_ = par; }


void RemoteJobExec::connectedCB( CallBacker* )
{
    isconnected_ = true;
}


void RemoteJobExec::ckeckConnection()
{
    BufferString errmsg( "Connection to Daemon on ", host_ );
    errmsg += " failed";
    if ( !isconnected_ )
	mErrRet( errmsg.buf() );
}


void RemoteJobExec::uiErrorMsg( const char* msg )
{
    BufferString cmd = FilePath( GetBinPlfDir(), "od_DispMsg" ).fullPath();
    cmd.add( " --err " ).add( msg );
    ExecOSCmd( cmd.buf() );
}
