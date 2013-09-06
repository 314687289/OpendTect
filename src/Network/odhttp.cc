/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2006
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "odhttp.h"
#include "qhttpconn.h"
#include "settings.h"
#include "odinst.h"
#include "odplatform.h"

#include <QByteArray>
#include <QEventLoop>
#include <QFile>
#include <QHttp>
#include <QUrl>


class MyHttp : public QHttp
{
public:
MyHttp() : QHttp(), asynchronous_(false)		{}

~MyHttp()
{
    if ( qeventloop_.isRunning() )
	qeventloop_.exit();
}

void init()
{
    qfiles_.allowNull();
}

void startEventLoop()
{
    if ( !asynchronous_ && !qeventloop_.isRunning() )
	qeventloop_.exec();
}

void stopEventLoop()
{
    if ( !asynchronous_ )
	qeventloop_.exit();
}

int _setHost( const char* host, int port )
{
    host_ = host;
    const int id = setHost( host, port );
    startEventLoop();
    return id;
}

int _get( const char* path, const char* dest, BufferString& errmsg )
{
    QFile* qfile = 0;
    if ( dest && *dest )
    {
	qfile = new QFile( dest );
	if ( !qfile->open( QIODevice::WriteOnly ) )
	{
	    errmsg = "Cannot open ";
	    errmsg.add( dest ).add( " for writing." );
	    delete qfile;
	    return -1;
	}
    }

    qfiles_ += qfile;
    QUrl qurl( path );
    const int id = get( qurl.toEncoded(), qfile );
    requestids_ += id;
    startEventLoop();
    return id;
}
    
    
    
int _post( const char* path, const IOPar& postvars, BufferString& errmsg )
{
    qfiles_ += 0;
    QUrl qurl( path );

    QString boundary = "---------------------------193971182219750";
    QByteArray data(QString("--" + boundary + "\r\n").toAscii());
    
    for ( int idx=0; idx<postvars.size(); idx++ )
    {
	QString keystr = postvars.getKey(idx).str();
	QString valstr = postvars.getValue(idx).str();

	data += "Content-Disposition: form-data;";
	data += "name=\"" + keystr + "\"\r\n\r\n";
	data += valstr + "\r\n";
	data += QString("--" + boundary + "--\r\n").toAscii();
    }
 
    QHttpRequestHeader header( sKeyPost() , qurl.toEncoded() );
    header.setValue( sKeyHost(), host_.buf() );
    header.setValue( sKeyContentType(), 
		     "multipart/form-data; boundary=" + boundary );
    header.setValue( sKeyContentLength(), QString::number(data.length()) );
    const int id = request( header, data );
    
    requestids_ += id;
    startEventLoop();
    return id;
}


int _postFileAndData( const char* path, const char* filename, 
		      const IOPar& postvars )
{
    QUrl qurl( path );
    QString boundary = "---------------------------193971182219750";

    BufferString releasename ( OD::Platform::local().shortName(), "_");
    releasename.add( ODInst::getPkgVersion ("base") );
    releasename.add( "_" );

    QByteArray data(QString("--" + boundary + "\r\n").toAscii());
    data += "Content-Disposition: form-data; name=\"dumpfile\";"; 
    data += "filename=\"";
    data += releasename;
    data += "crash.dmp\"\r\n";
    data += "Content-Type: application/octet-stream\r\n\r\n";
 
    QFile file( filename );
    if (!file.open(QIODevice::ReadOnly))
    return 0;
 
    data += file.readAll();
    data += "\r\n";

    QString postarr;
    for ( int idx=0; idx<postvars.size(); idx++ )
    {
	BufferString varstr = postvars.getKey( idx );
	varstr.add( "=" ).add( postvars.getValue( idx ) );
	if ( idx!=postvars.size()-1 )
	    varstr.add( "&" );
			      
	postarr.append( varstr );
    }

    data += QString("--" + boundary + "\r\n").toAscii();
    data += "Content-Disposition: form-data; name=\"report\"\r\n\r\n";
    data += postarr; 
    data += "\r\n";
    data += QString("--" + boundary + "--\r\n").toAscii();
 
    QHttpRequestHeader header( sKeyPost() , qurl.toEncoded() );
    header.setValue( sKeyHost(), host_.buf() );
    header.setValue( sKeyContentType(), 
		     "multipart/form-data; boundary=" + boundary );
    header.setValue( sKeyContentLength(), QString::number(data.length()) );

    const int id = request( header, data );
    file.close();
    
    requestids_ += id;
    startEventLoop();
    return id;
}


void handleFinishedRequest( int reqid )
{
    const int reqidx = requestids_.indexOf( reqid );
    if ( qfiles_.validIdx(reqidx) )
    {
	QFile* qfile = qfiles_[reqidx];
	if ( qfile )
	    qfile->close();
	delete qfiles_.removeSingle( reqidx );
	requestids_.removeSingle( reqidx );
    }

    stopEventLoop();
}

    bool		asynchronous_;

protected:
    QEventLoop	qeventloop_;

    TypeSet<int>	requestids_;
    ObjectSet<QFile>	qfiles_;
    BufferString	host_;
    
    static const char*	sKeyPost();
    static const char*	sKeyHost();
    static const char*	sKeyContentType();
    static const char*	sKeyContentLength();
};

const char* MyHttp::sKeyPost()		{ return "POST"; }
const char* MyHttp::sKeyHost()		{ return "Host"; }
const char* MyHttp::sKeyContentType()   { return "Content-Type"; }
const char* MyHttp::sKeyContentLength() { return "Content-Length"; }

// ODHttp
const char* ODHttp::sKeyUseProxy()	{ return "Use Proxy"; }
const char* ODHttp::sKeyUseAuthentication() { return "Use Authentication"; }
const char* ODHttp::sKeyProxyHost()	{ return "Http Proxy Host"; }
const char* ODHttp::sKeyProxyPort()	{ return "Http Proxy Port"; }
const char* ODHttp::sKeyProxyUserName()	{ return "Http Proxy User Name"; }
const char* ODHttp::sKeyProxyPassword()	{ return "Http Proxy Password"; }


ODHttp::ODHttp()
    : qhttp_(new MyHttp)
    , requestStarted(this)
    , requestFinished(this)
    , messageReady(this)
    , dataReadProgress(this)
    , readyRead(this)
    , done(this)
    , connected(this)
    , disconnected(this)
    , forcedabort_(false)
{
    qhttpconn_ = new QHttpConnector( qhttp_, this );
    qhttp_->init();

    error_ = false;
    requestid_ = 0;

    requestFinished.notify( mCB(this,ODHttp,reqFinishedCB) );
}


ODHttp::~ODHttp()
{
    delete qhttpconn_;
}


void ODHttp::setASynchronous( bool yn )
{ qhttp_->asynchronous_ = yn; }


// ToDo: support username and passwd
int ODHttp::setProxy( const char* host, int port,
		      const char* usrnm, const char* pwd )
{ return qhttp_->setProxy( host, port, usrnm, pwd ); }


void ODHttp::useProxySettings()
{
    Settings& setts = Settings::common();
    bool useproxy = false;
    setts.getYN( ODHttp::sKeyUseProxy(), useproxy );
    if ( !useproxy ) return;

    BufferString host;
    setts.get( ODHttp::sKeyProxyHost(), host );

    int port = 1;
    setts.get( ODHttp::sKeyProxyPort(), port );

    bool auth = false;
    setts.getYN( sKeyUseAuthentication(), auth );

    if ( auth )
    {
	BufferString username;
	setts.get( ODHttp::sKeyProxyUserName(), username );

	BufferString password;
	setts.get( ODHttp::sKeyProxyPassword(), password );
	setProxy( host, port, username, password );
    }
    else
	setProxy( host, port );
}


int ODHttp::setHttpsHost( const char* host, int port )
{
    useProxySettings();
    return qhttp_->setHost( host, QHttp::ConnectionModeHttps );
}


int ODHttp::setHost( const char* host, int port )
{
    useProxySettings();
    return qhttp_->_setHost( host, port );
}


int ODHttp::close()
{ return qhttp_->close(); }

void ODHttp::abort()
{ qhttp_->abort(); }

ODHttp::State ODHttp::state() const
{ return (ODHttp::State)(int)qhttp_->state(); }


int ODHttp::get( const char* path, const char* dest )
{
    const int reqid = qhttp_->_get( path, dest, message_ );
    if ( reqid==-1 )
	error_ = true;

    return reqid;
}


int ODHttp::post( const char* path, const IOPar&postvars )
{
    int res = qhttp_->_post( path, postvars, message_ );
    if ( res==-1 )
	error_ = true;
    
    return res;
}


int ODHttp::postFile( const char* path, const char* filename, 
		      const IOPar& postvars )
{
    int res = qhttp_->_postFileAndData( path, filename, postvars );
    if ( res==-1 )
	error_ = true;
    
    return res;
}


void ODHttp::forceAbort()
{
    forcedabort_ = true;
    qhttp_->abort();
}


wchar_t* ODHttp::readWCharBuffer() const
{
    QByteArray qbytearr = qhttp_->readAll();
    QString trl = QString::fromUtf8( qbytearr );
    wchar_t* warr = new wchar_t [qbytearr.size()+1];
    const int res = trl.toWCharArray( warr );
    return res == 0 ? 0 : warr;
}


const char* ODHttp::readCharBuffer() const
{
    static QByteArray result;
    result = qhttp_->readAll();
    return result.constData();
}


BufferString ODHttp::readBuffer() const
{
    QByteArray result = qhttp_->readAll();
    return result.constData();
}


bool ODHttp::hasPendingRequests() const
{ return qhttp_->hasPendingRequests(); }

void ODHttp::clearPendingRequests()
{ return qhttp_->clearPendingRequests(); }

od_int64 ODHttp::bytesAvailable() const
{ return qhttp_->bytesAvailable(); }


void ODHttp::setMessage( const char* msg )
{
    message_ = msg;
    messageReady.trigger();
}


void ODHttp::reqFinishedCB( CallBacker* )
{
    qhttp_->handleFinishedRequest( requestid_ );
}






