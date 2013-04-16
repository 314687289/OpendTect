/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          10/12/1999
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimain.h"

#include "uifont.h"
#include "uimainwin.h"
#include "uimsg.h"
#include "uiobjbody.h"

#include "bufstringset.h"
#include "debugmasks.h"
#include "envvars.h"
#include "errh.h"
#include "keyboardevent.h"
#include "mouseevent.h"
#include "oddirs.h"
#include "settings.h"
#include "thread.h"

#include <QApplication>
#include <QKeyEvent>
#include <QIcon>
#include <QCDEStyle>
#include <QWindowsStyle>
#include <QPlastiqueStyle>
#include <QCleanlooksStyle>
#include <QDesktopWidget>

#include <QTreeWidget>
#include <QMenu>

#ifdef __mac__
# include "odlogo128x128.xpm"
  const char** uiMain::XpmIconData = od_logo_128x128;
#else
# include "uimainicon.xpm"
  const char** uiMain::XpmIconData = uimainicon_xpm_data;
#endif
void uiMain::setXpmIconData( const char** xpmdata )
{ 
    XpmIconData = xpmdata;
}

#ifdef __win__
#include <QWindowsVistaStyle>
#endif
#ifdef __mac__
# define __machack__
#include <QMacStyle>
#endif

#ifdef __machack__
# include <CoreServices/CoreServices.h>
# include <ApplicationServices/ApplicationServices.h>

extern "C"
{

typedef struct CPSProcessSerNum
{
        UInt32          lo;
        UInt32          hi;
} CPSProcessSerNum;

extern OSErr    CPSGetCurrentProcess(CPSProcessSerNum *);
extern OSErr    CPSEnableForegroundOperation( CPSProcessSerNum *,
                                              UInt32, UInt32, UInt32, UInt32);
extern OSErr    CPSSetFrontProcess(CPSProcessSerNum *);
extern OSErr    NativePathNameToFSSpec(char *, FSSpec *, unsigned long);

}

#endif


class KeyboardEventFilter : public mQtclass(QObject)
{
public:
    			KeyboardEventFilter(KeyboardEventHandler& kbeh)
			    : kbehandler_(kbeh)				{}
protected:
    bool		eventFilter(mQtclass(QObject*),mQtclass(QEvent*));

    KeyboardEventHandler& kbehandler_;
};


bool KeyboardEventFilter::eventFilter( mQtclass(QObject*) obj,
				       mQtclass(QEvent*) ev )
{
    if ( ev->type()!=mQtclass(QEvent)::KeyPress &&
	 ev->type()!=mQtclass(QEvent)::KeyRelease )
	return false;

    const mQtclass(QKeyEvent*) qke = dynamic_cast<mQtclass(QKeyEvent*)>( ev );
    if ( !qke ) return false;

    KeyboardEvent kbe;
    kbe.key_ = (OD::KeyboardKey) qke->key();
    kbe.modifier_ = OD::ButtonState( (int) qke->modifiers() );
    kbe.isrepeat_ = qke->isAutoRepeat();

    if ( ev->type() == mQtclass(QEvent)::KeyPress )
	kbehandler_.triggerKeyPressed( kbe );
    else 
	kbehandler_.triggerKeyReleased( kbe );

    if ( kbehandler_.isHandled() )
	return true;

    return mQtclass(QObject)::eventFilter( obj, ev );
}


class mQtclass(QtTabletEventFilter) : public mQtclass(QObject)
{
public:
    			mQtclass(QtTabletEventFilter)()
			    : mousepressed_( false )
			    , checklongleft_( false )
			    , lostreleasefixevent_( 0 )
			{}
protected:
    bool		eventFilter(mQtclass(QObject*),mQtclass(QEvent*));

    bool		mousepressed_;
    bool		checklongleft_;

    mQtclass(QMouseEvent*)	lostreleasefixevent_;
    bool			islostreleasefixed_;
    mQtclass(Qt)::MouseButton	mousebutton_;

    Geom::Point2D<int>		lastdragpos_;
};


bool mQtclass(QtTabletEventFilter)::eventFilter( mQtclass(QObject*) obj,
						 mQtclass(QEvent*) ev )
{
    const mQtclass(QTabletEvent*) qte =
				dynamic_cast<mQtclass(QTabletEvent*)>( ev );
    if ( qte )
    {
	TabletInfo& ti = TabletInfo::latestState();

	ti.eventtype_ = (TabletInfo::EventType) qte->type();
	ti.pointertype_ = (TabletInfo::PointerType) qte->pointerType();
	ti.device_ = (TabletInfo::TabletDevice) qte->device();
	ti.globalpos_.x = qte->globalX();
	ti.globalpos_.y = qte->globalY();
	ti.pos_.x = qte->x();
	ti.pos_.y = qte->y();
	ti.pressure_ = qte->pressure();
	ti.rotation_ = qte->rotation();
	ti.tangentialpressure_ = qte->tangentialPressure();
	ti.uniqueid_ = qte->uniqueId();
	ti.xtilt_ = qte->xTilt();
	ti.ytilt_ = qte->yTilt();
	ti.z_ = qte->z();

	ti.updatePressData();
	return false;		// Qt will resent it as a QMouseEvent
    }

    const mQtclass(QMouseEvent*) qme =
				dynamic_cast<mQtclass(QMouseEvent*)>( ev );
    const TabletInfo* ti = TabletInfo::currentState();

    if ( !qme )
	return false;

    // Hack to repair missing mouse release events from tablet pen on Linux
    if ( mousepressed_ && !lostreleasefixevent_ && ti && !ti->pressure_ &&
	 qme->type()!=mQtclass(QEvent)::MouseButtonRelease )
    {
	lostreleasefixevent_ = new mQtclass(QMouseEvent)( 
					mQtclass(QEvent)::MouseButtonRelease,
					qme->pos(), qme->globalPos(),
					mousebutton_,
					qme->buttons() & ~mousebutton_,
					qme->modifiers() );
	mQtclass(QApplication)::postEvent( obj, lostreleasefixevent_ );
    }

    if ( qme->type()==mQtclass(QEvent)::MouseButtonPress )
    {
	lostreleasefixevent_ = 0;
	islostreleasefixed_ = false;
	mousebutton_ = qme->button();
    }

    if ( qme == lostreleasefixevent_ )
    {
	if ( !mousepressed_ )
	    return true;

	islostreleasefixed_ = true;
    }
    else if ( qme->type()==mQtclass(QEvent)::MouseButtonRelease )
    {
	if ( islostreleasefixed_ )
	    return true;
    }
    // End of hack

    // Hack to solve mouse/tablet dragging refresh problem
    if ( qme->type() == mQtclass(QEvent)::MouseButtonPress )
	lastdragpos_ = Geom::Point2D<int>::udf();

    if ( qme->type()==mQtclass(QEvent)::MouseMove && mousepressed_ )
    {
	const Geom::Point2D<int> curpos( qme->globalX(), qme->globalY() );
	if ( !lastdragpos_.isDefined() )
	    lastdragpos_ = curpos;
	else if ( lastdragpos_ != curpos )
	{
	    lastdragpos_ = Geom::Point2D<int>::udf();
	    return true;
	}
    }
    // End of hack

    if ( qme->type() == mQtclass(QEvent)::MouseButtonPress )
    {
	mousepressed_ = true;
	if ( qme->button() == mQtclass(Qt)::LeftButton )
	    checklongleft_ = true;
    }

    if ( qme->type() == mQtclass(QEvent)::MouseButtonRelease )
    {
	mousepressed_ = false;
	checklongleft_ = false;
    }
    
    if ( ti && qme->type()==mQtclass(QEvent)::MouseMove && mousepressed_ )
    {
	if ( checklongleft_ &&
	     ti->postPressTime()>500 && ti->maxPostPressDist()<5 )
	{
	    checklongleft_ = false;
	    mQtclass(QEvent*) qev =
				new mQtclass(QEvent)( mUsrEvLongTabletPress );
	    mQtclass(QApplication)::postEvent(
		    		   mQtclass(QApplication)::focusWidget(), qev );
	}

	mQtclass(QWidget*) tlw =
	    		mQtclass(QApplication)::topLevelAt( qme->globalPos() );
	if ( dynamic_cast<mQtclass(QMenu*)>(tlw) )
	    return true;

	mQtclass(QWidget*) fw = mQtclass(QApplication)::focusWidget();
	if ( dynamic_cast<mQtclass(QTreeWidget*)>(fw) )
	    return true;
    }

    return false;
}


#if QT_VERSION >= 0x050000
void myMessageOutput( QtMsgType, const QMessageLogContext &, const QString&);
#else
void myMessageOutput( QtMsgType type, const char *msg );
#endif


const uiFont* uiMain::font_ = 0;
mQtclass(QApplication*) uiMain::app_ = 0;
uiMain*	uiMain::themain_ = 0;

KeyboardEventHandler* uiMain::keyhandler_ = 0;
KeyboardEventFilter* uiMain::keyfilter_ = 0;
mQtclass(QtTabletEventFilter*) uiMain::tabletfilter_ = 0;


static void initQApplication()
{
    mQtclass(QApplication)::setDesktopSettingsAware( true );

    mQtclass(QCoreApplication)::setOrganizationName( "dGB");
    mQtclass(QCoreApplication)::setOrganizationDomain( "opendtect.org" );
    mQtclass(QCoreApplication)::setApplicationName( "OpendTect" );
#ifdef __mac__

    mQtclass(QCoreApplication)::setAttribute(
	    			mQtclass(Qt)::AA_MacDontSwapCtrlAndMeta, true );
#endif
    
#ifndef __win__
    mQtclass(QCoreApplication)::addLibraryPath( GetBinPlfDir() ); 
    							// Qt plugin libraries
#endif
}


uiMain::uiMain( int& argc, char **argv )
    : mainobj_( 0 )
{
#ifdef __machack__
        ProcessSerialNumber psn;
        CPSProcessSerNum PSN;

        GetCurrentProcess(&psn);
        if (!CPSGetCurrentProcess(&PSN))
        {
            if (!CPSEnableForegroundOperation(&PSN, 0x03, 0x3C, 0x2C, 0x1103))
            {
                if (!CPSSetFrontProcess(&PSN))
                {
                    GetCurrentProcess(&psn);
                }
            }
        }
#endif

    initQApplication();
    init( 0, argc, argv );
    
    QPixmap pixmap( XpmIconData );
    app_->setWindowIcon( QIcon( pixmap ) );
}


uiMain::uiMain( mQtclass(QApplication*) qapp )
    : mainobj_( 0 )
{ 
    initQApplication();
    app_ = qapp;
    app_->setWindowIcon( mQtclass(QIcon)(XpmIconData) );
}


static mQtclass(QStyle*) getStyleFromSettings()
{
    const char* lookpref = Settings::common().find( "dTect.LookStyle" );
    if ( !lookpref || !*lookpref )
	lookpref = GetEnvVar( "OD_LOOK_STYLE" );
    if ( lookpref && *lookpref )
    {
#ifndef QT_NO_STYLE_CDE
	if ( !strcmp(lookpref,"CDE") )
	    return new QCDEStyle;
#endif
#ifndef QT_NO_STYLE_MOTIF
	if ( !strcmp(lookpref,"Motif") )
	    return new mQtclass(QMotifStyle);
#endif
#ifndef QT_NO_STYLE_WINDOWS
	if ( !strcmp(lookpref,"Windows") )
	    return new QWindowsStyle;
#endif
#ifndef QT_NO_STYLE_PLASTIQUE
	if ( !strcmp(lookpref,"Plastique") )
	    return new mQtclass(QPlastiqueStyle);
#endif
#ifndef QT_NO_STYLE_CLEANLOOKS
	if ( !strcmp(lookpref,"Cleanlooks") )
	    return new mQtclass(QCleanlooksStyle);
#endif
    }

    return 0;
}


void uiMain::init( mQtclass(QApplication*) qap, int& argc, char **argv )
{
    if ( app_ ) 
    {
	pErrMsg("You already have a uiMain object!");
	return;
    }
    else
	themain_ = this;

    if ( DBG::isOn(DBG_UI) && !qap )
	DBG::message( "Constructing QApplication ..." );

    if ( qap )
	app_ = qap;
    else
	app_ = new mQtclass(QApplication)( argc, argv );

    KeyboardEventHandler& kbeh = keyboardEventHandler();
    keyfilter_ = new KeyboardEventFilter( kbeh );
    app_->installEventFilter( keyfilter_ );

    tabletfilter_ = new mQtclass(QtTabletEventFilter)();
    app_->installEventFilter( tabletfilter_ );

    if ( DBG::isOn(DBG_UI) && !qap )
	DBG::message( "... done." );
    
#if QT_VERSION >= 0x050000
    qInstallMessageHandler( myMessageOutput );
#else
    qInstallMsgHandler( myMessageOutput );
#endif

    mQtclass(QStyle*) styl = getStyleFromSettings();
#ifdef __win__
    if ( !styl )
	styl = mQtclass(QSysInfo)::WindowsVersion ==
	    					mQtclass(QSysInfo)::WV_VISTA
	    	? new mQtclass(QWindowsVistaStyle)
		: new mQtclass(QWindowsXPStyle);
#else
# ifdef __mac__
    if ( !styl )
	styl = new mQtclass(QMacStyle);
# else
    if ( !styl )
	styl = new mQtclass(QCleanlooksStyle);
# endif
#endif

    mQtclass(QApplication)::setStyle( styl );
    font_ = 0;
    setFont( *font() , true );
}


uiMain::~uiMain()
{
    delete app_;
    delete font_;

    delete keyhandler_;
    delete keyfilter_;
    delete tabletfilter_;
}


int uiMain::exec()          			
{
    if ( !app_ )  { pErrMsg("Huh?") ; return -1; }
    return app_->exec();
}


void* uiMain::thread()
{ return qApp ? qApp->thread() : 0; }


void uiMain::getCmdLineArgs( BufferStringSet& args ) const
{
    mQtclass(QStringList) qargs = app_->arguments();
    for ( int idx=0; idx<qargs.count(); idx++ )
	args.add( qargs.at(idx).toLatin1() );
}


void uiMain::setTopLevel( uiMainWin* obj )
{
    if ( !obj ) return;
    if ( !app_ )  { pErrMsg("Huh?") ; return; }

    if ( mainobj_ ) mainobj_->setExitAppOnClose( false );
    obj->setExitAppOnClose( true );

    mainobj_ = obj;
}


void uiMain::setFont( const uiFont& fnt, bool PassToChildren )
{   
    font_ = &fnt;
    if ( !app_ )  { pErrMsg("Huh?") ; return; }
    app_->setFont( font_->qFont() );
}


void uiMain::exit( int retcode ) 
{ 
    if ( !app_ )  { pErrMsg("Huh?") ; return; }
    app_->exit( retcode );
}
/*!<
    \brief Tells the application to exit with a return code. 

    After this function has been called, the application leaves the main 
    event loop and returns from the call to exec(). The exec() function
    returns retcode. 

    By convention, retcode 0 means success, any non-zero value indicates 
    an error. 

    Note that unlike the C library exit function, this function does 
    return to the caller - it is event processing that stops

*/


const uiFont* uiMain::font()
{
    if ( !font_ )
    { font_ = &FontList().get( className(*this) );  }

    return font_;
}


Color uiMain::windowColor() const
{
    const mQtclass(QColor&) qcol =
	 mQtclass(QApplication)::palette().color( mQtclass(QPalette)::Window );
    return Color( qcol.red(), qcol.green(), qcol.blue() );
}


uiSize uiMain::desktopSize() const
{
    if ( !app_ || !app_->desktop() )
	return uiSize( mUdf(int), mUdf(int) );

    return uiSize( app_->desktop()->width(), app_->desktop()->height() );
}


uiMain& uiMain::theMain()
{ 
    if ( themain_ ) return *themain_;
    if ( !qApp ) 
    { 
	pFreeFnErrMsg("FATAL: no uiMain and no qApp.","uiMain::theMain()");
	mQtclass(QApplication)::exit( -1 );
    }

    themain_ = new uiMain( qApp );
    return *themain_;
}


KeyboardEventHandler& uiMain::keyboardEventHandler()
{
    if ( !keyhandler_ )
	keyhandler_ = new KeyboardEventHandler();

    return *keyhandler_;
}


void uiMain::flushX()        
{ if ( app_ ) app_->flush(); }


//! waits [msec] milliseconds for new events to occur and processes them.
void uiMain::processEvents( int msec )
{ if ( app_ ) app_->processEvents( mQtclass(QEventLoop)::AllEvents, msec ); }


#if QT_VERSION >= 0x050000
void myMessageOutput( QtMsgType type, const QMessageLogContext &,
		     const QString& msg)
#define mMsg msg.toLatin1().constData()
#else
void myMessageOutput( QtMsgType type, const char *msg )
#define mMsg msg
#endif
{
    switch ( type ) {
	case mQtclass(QtDebugMsg):
	    ErrMsg( mMsg, true );
	    break;
	case mQtclass(QtWarningMsg):
	    ErrMsg( mMsg, true );
	    break;
	case mQtclass(QtFatalMsg):
	    ErrMsg( mMsg );
	    break;
	case mQtclass(QtCriticalMsg):
	    ErrMsg( mMsg );
	    break;
	default:
	    break;
    }
}


bool isMainThread( const void* thread )
{ return uiMain::theMain().thread() == thread; }

bool isMainThreadCurrent()
{ return isMainThread( Threads::currentThread() ); }
