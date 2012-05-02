/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: localsocket.cc,v 1.3 2012-05-02 15:11:43 cvskris Exp $";

#include "localsocket.h"
#include "qlocalsocketcomm.h"
#include <QByteArray>


LocalSocket::LocalSocket()
    : qlocalsocket_(new QLocalSocket)
    , comm_(new QLocalSocketComm(qlocalsocket_,this))
    , connected(this)
    , disconnected(this)
    , error(this)
    , readyRead(this)
{
}


LocalSocket::~LocalSocket()
{
    delete qlocalsocket_;
    delete comm_;
}


const char* LocalSocket::errorMsg() const
{
    errmsg_ = qlocalsocket_->errorString().toAscii().constData();
    return errmsg_.buf();
}


void LocalSocket::connectToServer( const char* host )
{ qlocalsocket_->connectToServer( QString(host) ); }

void LocalSocket::disconnectFromServer()
{ qlocalsocket_->disconnectFromServer(); }

void LocalSocket::abort()
{ qlocalsocket_->abort(); }

bool LocalSocket::waitForConnected( int msec )
{ return qlocalsocket_->waitForConnected( msec ); }

bool LocalSocket::waitForReadyRead( int msec )
{ return qlocalsocket_->waitForReadyRead( msec ); }

int LocalSocket::write( const char* str )
{ return qlocalsocket_->write( str, 1024 ); }


void LocalSocket::read( BufferString& str )
{
    QByteArray ba = qlocalsocket_->readAll();
    str = ba.data();
}
