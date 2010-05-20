/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: tcpsocket.cc,v 1.6 2010-05-20 09:46:53 cvsranojay Exp $";

#include "tcpsocket.h"
#include "qtcpsocketcomm.h"
#include "iopar.h"

#define mInit \
    , comm_(new QTcpSocketComm(qtcpsocket_,this)) \
    , connected(this) \
    , disconnected(this) \
    , hostFound(this) \
    , readyRead(this) \
    , error(this) \
    , stateChanged(this)

TcpSocket::TcpSocket()
    : qtcpsocket_(new QTcpSocket)
    mInit
{}


TcpSocket::TcpSocket( QTcpSocket* qsocket )
    : qtcpsocket_(qsocket)
    mInit
{}


TcpSocket::~TcpSocket()
{
    delete qtcpsocket_;
    delete comm_;
}


const char* TcpSocket::errorMsg() const
{
    errmsg_ = qtcpsocket_->errorString().toAscii().constData();
    return errmsg_.buf();
}


void TcpSocket::connectToHost( const char* host, int port )
{ qtcpsocket_->connectToHost( QString(host), port ); }

void TcpSocket::disconnectFromHost()
{ qtcpsocket_->disconnectFromHost(); }

void TcpSocket::abort()
{ qtcpsocket_->abort(); }

bool TcpSocket::waitForConnected( int msec )
{ return qtcpsocket_->waitForConnected( msec ); }

bool TcpSocket::waitForReadyRead( int msec )
{ return qtcpsocket_->waitForReadyRead( msec ); }

int TcpSocket::write( const char* str ) const
{ return qtcpsocket_->write( str ); }

void TcpSocket::read( BufferString& str ) const
{
    QByteArray ba = qtcpsocket_->readAll();
    str = ba.data();
}


int TcpSocket::write( const IOPar& par ) const
{
    BufferString buf;
    par.putTo( buf );
    return write( buf.buf() ); 
}


void TcpSocket::read( IOPar& par ) const
{
    BufferString buf;
    read( buf );
    par.getFrom( buf );
}

