#ifndef qhttpconn_h
#define qhttpconn_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2006
 RCS:           $Id: qhttpconn.h,v 1.1 2010-08-30 10:05:10 cvsnanne Exp $
________________________________________________________________________

-*/

#include "odhttp.h"
#include <QHttp>


class QHttpConnector : public QObject
{
    Q_OBJECT
    friend class ODHttp;

protected:

QHttpConnector( QHttp* sender, ODHttp* receiver )
    : sender_(sender), receiver_(receiver)
{
    connect( sender_, SIGNAL(readyRead(const QHttpResponseHeader&)),
	     this, SLOT(readyRead(const QHttpResponseHeader&)) );

    connect( sender_, SIGNAL(stateChanged(int)),
	     this, SLOT(stateChanged(int)) );

    connect( sender_, SIGNAL(requestStarted(int)),
	     this, SLOT(requestStarted(int)) );

    connect( sender_, SIGNAL(requestFinished(int)),
	     this, SLOT(requestFinished(int)) );

    connect( sender_, SIGNAL(done(bool)),
	     this, SLOT(done(bool)) );
}

private slots:

void stateChanged( int state )
{
    receiver_->connectionstate_ = state;
    if ( state == QHttp::Unconnected )
    {
	receiver_->disconnected.trigger( *receiver_ );
	receiver_->setMessage( "Connection closed" );
    }
}


void readyRead( const QHttpResponseHeader& )
{
    receiver_->readyRead.trigger( *receiver_ );
}


void requestStarted( int id )
{
    receiver_->requestid_ = id;
    receiver_->requestStarted.trigger( *receiver_ );
}


void requestFinished( int id, bool error )
{
    receiver_->requestid_ = id;
    receiver_->error_ = error;
    if ( error )
    {
	receiver_->setMessage( sender_->errorString().toAscii().data() );
	return;
    }

    receiver_->requestFinished.trigger( *receiver_ );
}


void done( bool error )
{
    receiver_->error_ = error;
    receiver_->message_ = error ? sender_->errorString().toAscii().data()
				: "Sucessfully finished";
    receiver_->done.trigger( *receiver_ );
}

private:

    QHttp*	sender_;
    ODHttp*	receiver_;
};

#endif
