#ifndef qnetworkaccessconn_h
#define qnetworkaccessconn_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2012
 RCS:		$Id$
________________________________________________________________________

-*/


#include "odnetworkaccess.h"
#include "odnetworkreply.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

QT_BEGIN_NAMESPACE

class QNAMConnector : public QObject
{
    Q_OBJECT
    friend class ODNetworkAccess;

protected:

QNAMConnector( QNetworkAccessManager* sndr, ODNetworkAccess* receiver )
    : sender_(sndr), receiver_(receiver)
{
    connect( sender_, SIGNAL(finished(QNetworkReply*)),
	     this, SLOT(finished(QNetworkReply*)) );
}

private slots:

void finished( QNetworkReply* reply )
{
    receiver_->finished.trigger();
    receiver_->stopEventLoop();
}


private:

    QNetworkAccessManager*	sender_;
    ODNetworkAccess*		receiver_;
};


class QNetworkReplyConn : public QObject
{
    Q_OBJECT
    friend class ODNetworkReply; 

protected:

QNetworkReplyConn( QNetworkReply* sndr, ODNetworkReply* rec )
    : sender_(sndr), receiver_(rec)
{
    connect( sender_, SIGNAL(downloadProgress(qint64,qint64)),
	     this, SLOT(downloadProgress(qint64,qint64)) );
    connect( sender_, SIGNAL(error(QNetworkReply::NetworkError)),
	     this, SLOT(error(QNetworkReply::NetworkError)) );
    connect( sender_, SIGNAL(finished()),
	     this, SLOT(finished()) );
    connect( sender_, SIGNAL(metaDataChanged()),
	     this, SLOT(metaDataChanged()) );
    connect( sender_, SIGNAL(uploadProgress(qint64,qint64)),
	     this, SLOT(uploadProgress(qint64,qint64)) );

    // From QIODevice
    connect( sender_, SIGNAL(aboutToClose()),
	     this, SLOT(aboutToClose()) );
    connect( sender_, SIGNAL(bytesWritten(qint64)),
	     this, SLOT(bytesWritten(qint64)) );
    connect( sender_, SIGNAL(readyRead()),
	     this, SLOT(readyRead()) );
}

private slots:

void downloadProgress(qint64 nrdone,qint64 totalnr)
{
    if ( totalnr != -1 )
    {
	receiver_->getODNetworkTask()->setNrDone( nrdone );
	receiver_->getODNetworkTask()->setTotalNr( totalnr );
    }
}

void error(QNetworkReply::NetworkError)
{ receiver_->error.trigger(); }

void finished()
{ receiver_->finished.trigger(); }

void metaDataChanged()
{}

void uploadProgress(qint64,qint64)
{}

void aboutToClose()
{}

void bytesWritten(qint64)
{}

void readChannelFinished()
{}

void readyRead()
{ receiver_->readyRead.trigger(); }

private:

    QNetworkReply*		sender_;
    ODNetworkReply*		receiver_;

};

QT_END_NAMESPACE

#endif
