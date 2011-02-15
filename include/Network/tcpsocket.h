#ifndef tcpsocket_h
#define tcpsocket_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
 RCS:           $Id: tcpsocket.h,v 1.9 2011-02-15 03:59:22 cvsnanne Exp $
________________________________________________________________________

-*/


#include "callback.h"
#include "bufstring.h"

class QTcpSocket;
class QTcpSocketComm;
class IOPar;


mClass TcpSocket : public CallBacker
{
friend class QTcpSocketComm;

public:
    				TcpSocket();
				TcpSocket(QTcpSocket*,int id);
				~TcpSocket();

    void			connectToHost(const char* host,int port);
    void			disconnectFromHost();
    void			abort();
    int				write(const char*);  
    int				write(const IOPar&);

    void			read(BufferString&) const;
    void			read(IOPar&) const;
    const int			getID() const { return id_; }

    const char*			errorMsg() const;

    Notifier<TcpSocket>		connected;
    Notifier<TcpSocket>		disconnected;
    Notifier<TcpSocket>		hostFound;
    Notifier<TcpSocket>		readyRead;
    Notifier<TcpSocket>		error;
    Notifier<TcpSocket>		stateChanged;

    bool			waitForConnected(int msec);
    				//!<Useful when no event loop available
    bool			waitForReadyRead(int msec);
    				//!<Useful when no event loop available

protected:

    QTcpSocket*			qtcpsocket_;
    QTcpSocketComm*		comm_;
    mutable BufferString	errmsg_;
    const int			id_;
};

#endif
