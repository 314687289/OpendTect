#ifndef odhttp_h
#define odhttp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2006
 RCS:           $Id: odhttp.h,v 1.3 2010-09-10 07:09:53 cvsnanne Exp $
________________________________________________________________________

-*/


#include "callback.h"
#include "bufstringset.h"

class QFile;
class QHttp;
class QHttpConnector;

class ODHttp : public CallBacker
{
friend class QHttpConnector;

public:
    			ODHttp();
			~ODHttp();

    int			setHost(const char* host,int port=80);
    int			close();
    void		abort();
    bool		hasPendingRequests() const;

    int			get(const char* cmd,const char* dest=0);
    			//!<When dest=0, read from buffer
    BufferString	readBuffer() const;

    od_int64		bytesAvailable() const;

    const bool		isOK() const		{ return !error_; }
    void		setMessage(const char*);
    const char*		message() const		{ return message_.buf(); }

    Notifier<ODHttp>	requestStarted;
    Notifier<ODHttp>	requestFinished;
    Notifier<ODHttp>	messageReady;
    Notifier<ODHttp>	readyRead;
    Notifier<ODHttp>	done;
    Notifier<ODHttp>	disconnected;

protected:

    QHttp*		qhttp_;
    QHttpConnector*	qhttpconn_;

    TypeSet<int>	getids_;
    ObjectSet<QFile>	qfiles_;

    bool		error_;
    BufferString	message_;
    int			requestid_;
    int			connectionstate_;

    void		reqFinishedCB(CallBacker*);
};

#endif
