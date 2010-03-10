/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          31/05/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uimainwin.cc,v 1.198 2010-03-10 07:18:44 cvsnanne Exp $";

#include "uimainwin.h"
#include "uidialog.h"
#include "uibody.h"
#include "uiobjbody.h"
#include "uifont.h"

#include "uibutton.h"
#include "uidesktopservices.h"
#include "uidockwin.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uimenu.h"
#include "uiparentbody.h"
#include "uistatusbar.h"
#include "uiseparator.h"
#include "uitoolbar.h"

#include "envvars.h"
#include "errh.h"
#include "filepath.h"
#include "helpview.h"
#include "msgh.h"
#include "oddirs.h"
#include "pixmap.h"
#include "thread.h"
#include "timer.h"
#include "iopar.h"
#include "thread.h"

#include <iostream>

#include <QAbstractButton>
#include <QApplication>
#include <QCloseEvent>
#include <QColorDialog>
#include <QDesktopWidget>
#include <QDialog>
#include <QDockWidget>
#include <QFileDialog>
#include <QFontDialog>
#include <QIcon>
#include <QMainWindow>
#include <QMessageBox>
#include <QPixmap>
#include <QSettings>
#include <QStatusBar>
#include <QWidget>

static const QEvent::Type sQEventGuiThread  = (QEvent::Type) (QEvent::User+0);
static const QEvent::Type sQEventPopUpReady = (QEvent::Type) (QEvent::User+1);


class uiMainWinBody : public uiParentBody
		    , public QMainWindow
{
friend class		uiMainWin;
public:
			uiMainWinBody(uiMainWin& handle,uiParent* parnt,
				      const char* nm,bool modal);

    void		construct(int nrstatusflds,bool wantmenubar);

    virtual		~uiMainWinBody();

#define mHANDLE_OBJ     uiMainWin
#define mQWIDGET_BASE   QMainWindow
#define mQWIDGET_BODY   QMainWindow
#define UIBASEBODY_ONLY
#define UIPARENT_BODY_CENTR_WIDGET
#include                "i_uiobjqtbody.h"

public:


    uiStatusBar* 	uistatusbar();
    uiMenuBar* 		uimenubar();

    virtual void        polish() { QMainWindow::ensurePolished(); }

    void		reDraw( bool deep )
			{
			    update();
			    centralWidget_->reDraw( deep );
			}

    void		go()
			{ finalise(); show(); move( handle_.popuparea_ ); }

    virtual void	show() 
			{
			    setWindowTitle( handle_.caption(false) );
			    eventrefnr_ = handle_.beginCmdRecEvent("WinPopUp");
			    QMainWindow::show();

			    if( poptimer.isActive() )
				poptimer.stop();

			    popped_up = false;
			    poptimer.start( 100, true );

			    QEvent* ev = new QEvent( sQEventPopUpReady );
			    QApplication::postEvent( this, ev );

			    if ( modal_ )
				eventloop_.exec();
			}

    void		move(uiMainWin::PopupArea);

    void		close();
    bool		poppedUp() const { return popped_up; }

    bool		touch()
			{
			    if ( popped_up || !finalised() ) return false;

			    if( poptimer.isActive() )
				poptimer.stop();

			    if ( !popped_up )
				poptimer.start( 100, true );

			    return true;
			}

    void		removeDockWin(uiDockWin*);
    void		addDockWin(uiDockWin&,uiMainWin::Dock);

    virtual QMenu*	createPopupMenu()
			{ return menubar ? 0 : QMainWindow::createPopupMenu(); }

    void		addToolBar(uiToolBar*);
    void		removeToolBar(uiToolBar*);
    uiPopupMenu&	getToolbarsMenu()		{ return *toolbarsmnu_;}
    void		updateToolbarsMenu();

    const ObjectSet<uiToolBar>& toolBars() const	{ return toolbars_; }
    const ObjectSet<uiDockWin>& dockWins() const	{ return dockwins_; }

    void		setModal( bool yn )		{ modal_ = yn; }
    bool		isModal() const			{ return modal_; }

    void		setWindowTitle(const char*);

    void		activateInGUIThread(const CallBack&,bool busywait);

protected:

    virtual void	finalise( bool trigger_finalise_start_stop=true );
    void		closeEvent(QCloseEvent*);
    bool		event(QEvent*);

    void		renewToolbarsMenu();
    void		toggleToolbar(CallBacker*);

    void		saveSettings();
    void		readSettings();

    bool		exitapponclose_;

    Threads::Mutex	activatemutex_;
    ObjectSet<CallBack>	activatecbs_;
    int			nractivated_;

    int			eventrefnr_;

    uiStatusBar* 	statusbar;
    uiMenuBar* 		menubar;
    uiPopupMenu*	toolbarsmnu_;
    
    ObjectSet<uiToolBar> toolbars_;
    ObjectSet<uiDockWin> dockwins_;

private:

    QEventLoop		eventloop_;

    int			iconsz_;
    bool		modal_;
    int			looplevel__;
    Qt::WFlags		getFlags(bool hasparent,bool modal) const;

    void 		popTimTick(CallBacker*);
    Timer		poptimer;
    bool		popped_up;
    uiSize		prefsz_;

    bool		deletefrombody_;
    bool		deletefromod_;
};



uiMainWinBody::uiMainWinBody( uiMainWin& uimw, uiParent* p, 
			      const char* nm, bool modal )
	: uiParentBody(nm)
	, QMainWindow(p && p->pbody() ? p->pbody()->qwidget() : 0,
		      getFlags(p,modal) )
	, handle_(uimw)
	, initing(true)
	, centralWidget_(0)
	, statusbar(0)
	, menubar(0)
	, toolbarsmnu_(0)
	, modal_(p && modal)
	, poptimer("Popup timer")
	, popped_up(false)
	, exitapponclose_(false)
        , prefsz_(-1,-1)
	, nractivated_(0)
{
    if ( nm && *nm )
	setObjectName( nm );

    poptimer.tick.notify( mCB(this,uiMainWinBody,popTimTick) );

    iconsz_ = uiObject::iconSize();
    setIconSize( QSize(iconsz_,iconsz_) );

    setWindowModality( p && modal ? Qt::WindowModal : Qt::NonModal );

    deletefrombody_ = deletefromod_ = false;
}


Qt::WFlags uiMainWinBody::getFlags( bool hasparent, bool modal ) const
{
    return  Qt::WindowFlags( hasparent ? Qt::Dialog : Qt::Window );
}


void uiMainWinBody::construct( int nrstatusflds, bool wantmenubar )
{
    centralWidget_ = new uiGroup( &handle(), "OpendTect Main Window" );
    setCentralWidget( centralWidget_->body()->qwidget() ); 

    centralWidget_->setIsMain(true);
    centralWidget_->setBorder(10);
    centralWidget_->setStretch(2,2);

    if ( nrstatusflds != 0 )
    {
	QStatusBar* mbar= statusBar();
	if ( mbar )
	    statusbar = new uiStatusBar( &handle(),
					  "MainWindow StatusBar handle", *mbar);
	else
	    pErrMsg("No statusbar returned from Qt");

	if ( nrstatusflds > 0 )
	{
	    for( int idx=0; idx<nrstatusflds; idx++ )
		statusbar->addMsgFld();
	}
    }
    if ( wantmenubar )
    {   
	QMenuBar* myBar =  menuBar();

	if ( myBar )
	    menubar = new uiMenuBar( &handle(), "MainWindow MenuBar handle", 
				      *myBar);
	else
	    pErrMsg("No menubar returned from Qt");

	toolbarsmnu_ = new uiPopupMenu( &handle(), "Toolbars" );
    }

    initing = false;
}


void uiMainWinBody::move( uiMainWin::PopupArea pa )
{
    switch( pa )
    {
	case uiMainWin::TopLeft :
	    QWidget::move( 0, 0 ); break;
	case uiMainWin::TopRight :
	    QWidget::move( mUdf(int), 0 ); break;
	case uiMainWin::BottomLeft :
	    QWidget::move( 0, mUdf(int) ); break;
	case uiMainWin::BottomRight :
	    QWidget::move( mUdf(int), mUdf(int) ); break;
    }
}


uiMainWinBody::~uiMainWinBody()
{
    deleteAllChildren(); //delete them now to make sure all ui objects
    			 //are deleted before their body counterparts

    while ( toolbars_.size() )
	delete toolbars_[0];

    if ( toolbarsmnu_ ) toolbarsmnu_->clear();
    delete toolbarsmnu_;

    if ( !deletefromod_ )
    {
	deletefrombody_ = true;
	delete &handle_;
    }
}


void uiMainWinBody::popTimTick( CallBacker* )
{
    if ( popped_up ) { pErrMsg( "huh?" );  return; }
	popped_up = true;
    if ( prefsz_.hNrPics()>0 && prefsz_.vNrPics()>0 )
	resize( prefsz_.hNrPics(), prefsz_.vNrPics() );
}


void uiMainWinBody::finalise( bool trigger_finalise_start_stop )
{
    if ( trigger_finalise_start_stop )
	handle_.finaliseStart.trigger( handle_ );

    centralWidget_->finalise();
    finaliseChildren();

    if ( trigger_finalise_start_stop )
	handle_.finaliseDone.trigger( handle_ );
}


void uiMainWinBody::closeEvent( QCloseEvent* ce )
{
    const int refnr = handle_.beginCmdRecEvent( "Close" );

    if ( handle_.closeOK() )
    {
	handle_.windowClosed.trigger( handle_ );
	ce->accept();
    }
    else
	ce->ignore();

     handle_.endCmdRecEvent( refnr );
}


void uiMainWinBody::close()
{
    if ( !handle_.closeOK() ) return; 

    handle_.windowClosed.trigger( handle_ );

    if ( modal_ )
	eventloop_.exit();

    QMainWindow::hide();

    if ( exitapponclose_ )
	qApp->quit();
}


uiStatusBar* uiMainWinBody::uistatusbar()
{ return statusbar; }

uiMenuBar* uiMainWinBody::uimenubar()
{ return menubar; }


void uiMainWinBody::removeDockWin( uiDockWin* dwin )
{
    if ( !dwin ) return;

    removeDockWidget( dwin->qwidget() );
    dockwins_ -= dwin;
}


void uiMainWinBody::addDockWin( uiDockWin& dwin, uiMainWin::Dock dock )
{
    Qt::DockWidgetArea dwa = Qt::LeftDockWidgetArea;
    if ( dock == uiMainWin::Right ) dwa = Qt::RightDockWidgetArea;
    else if ( dock == uiMainWin::Top ) dwa = Qt::TopDockWidgetArea;
    else if ( dock == uiMainWin::Bottom ) dwa = Qt::BottomDockWidgetArea;
    addDockWidget( dwa, dwin.qwidget() );
    if ( dock == uiMainWin::TornOff )
	dwin.setFloating( true );
    dockwins_ += &dwin;
}


void uiMainWinBody::toggleToolbar( CallBacker* cb )
{
    mDynamicCastGet( uiMenuItem*, itm, cb );
    if ( !itm ) return;

    for ( int idx=0; idx<toolbars_.size(); idx++ )
    {
	uiToolBar& tb = *toolbars_[idx];
	if ( tb.name()==itm->name() )
	    tb.display( tb.isHidden() );
    }
}


void uiMainWinBody::updateToolbarsMenu()
{
    if ( !toolbarsmnu_ ) return;

    const ObjectSet<uiMenuItem>& items = toolbarsmnu_->items();

    for ( int idx=0; idx<toolbars_.size(); idx++ )
    {
	const uiToolBar& tb = *toolbars_[idx];
	uiMenuItem& itm = *const_cast<uiMenuItem*>( items[idx] );
	if ( itm.name()==tb.name() )
	    itm.setChecked( !tb.isHidden() );
    }
}


void uiMainWinBody::addToolBar( uiToolBar* tb )
{
    QMainWindow::addToolBar( (Qt::ToolBarArea)tb->prefArea(), tb->qwidget() );
    toolbars_ += tb;
    renewToolbarsMenu();
}


void uiMainWinBody::removeToolBar( uiToolBar* tb )
{
    QMainWindow::removeToolBar( tb->qwidget() );
    toolbars_ -= tb;
    renewToolbarsMenu();
}


void uiMainWinBody::renewToolbarsMenu()
{
    if ( !toolbarsmnu_ ) return;

    toolbarsmnu_->clear();
    for ( int idx=0; idx<toolbars_.size(); idx++ )
    {
	const uiToolBar& tb = *toolbars_[idx];
	uiMenuItem* itm =
	    new uiMenuItem( tb.name(), mCB(this,uiMainWinBody,toggleToolbar) );
	toolbarsmnu_->insertItem( itm );
	itm->setCheckable( true );
	itm->setChecked( !tb.isHidden() );
    }
}


BufferString getSettingsFileName()
{
    FilePath fp( GetSettingsDir() );
    fp.add( "qtsettings" );
    const char* swusr = GetSoftwareUser();
    if ( swusr )
	fp.setExtension( swusr );
    return fp.fullPath();
}


void uiMainWinBody::saveSettings()
{
    const BufferString fnm = getSettingsFileName();
    QSettings settings( fnm.buf(), QSettings::IniFormat );
    settings.beginGroup( NamedObject::name().buf() );
    settings.setValue( "size", size() );
    settings.setValue( "pos", pos() );
    settings.setValue( "state", saveState() );
    settings.endGroup();
}


void uiMainWinBody::readSettings()
{
    const BufferString fnm = getSettingsFileName();
    QSettings settings( fnm.buf(), QSettings::IniFormat );
    settings.beginGroup( NamedObject::name().buf() );
    QSize qsz( settings.value("size",QSize(200,200)).toSize() );
    prefsz_ = uiSize( qsz.width(), qsz.height() );
    restoreState( settings.value("state").toByteArray() );
    settings.endGroup();

    updateToolbarsMenu();
}


void uiMainWinBody::setWindowTitle( const char* txt )
{ QMainWindow::setWindowTitle( uiMainWin::uniqueWinTitle(txt,this) ); }


#define mExecMutex( statements ) \
    activatemutex_.lock(); statements; activatemutex_.unLock();


void uiMainWinBody::activateInGUIThread( const CallBack& cb, bool busywait )
{
    CallBack* actcb = new CallBack( cb );
    mExecMutex( activatecbs_ += actcb );

    QEvent* guithreadev = new QEvent( sQEventGuiThread );
    QApplication::postEvent( this, guithreadev );

    float sleeptime = 0.01;
    while ( busywait )
    {
	mExecMutex( const int idx = activatecbs_.indexOf(actcb) );
	if ( idx < 0 )
	    break;

	Threads::sleep( sleeptime ); 
	if ( sleeptime < 1.28 )
	    sleeptime *= 2;
    }
}


bool uiMainWinBody::event( QEvent* ev )
{
    if ( ev->type() == sQEventGuiThread )
    {
	mExecMutex( CallBack* actcb = activatecbs_[nractivated_++] );
	actcb->doCall( this );
	handle_.activatedone.trigger( actcb->cbObj() );
	mExecMutex( activatecbs_ -= actcb; nractivated_-- );
	delete actcb;
    }
    else if ( ev->type() == sQEventPopUpReady )
    {
	handle_.endCmdRecEvent( eventrefnr_, "WinPopUp" );
    }
    else
	return QMainWindow::event( ev );
    
    return true; 
}


// ----- uiMainWin -----


uiMainWin::uiMainWin( uiParent* p, const uiMainWin::Setup& setup )
    : uiParent(setup.caption_,0)
    , body_(0)
    , parent_(p)
    , popuparea_(Middle)
    , windowClosed(this)
    , activatedone(this)
    , caption_(setup.caption_)
{ 
    body_ = new uiMainWinBody( *this, p, setup.caption_, setup.modal_ ); 
    setBody( body_ );
    body_->construct( setup.nrstatusflds_, setup.withmenubar_ );
    body_->setWindowIconText(
	    setup.caption_.isEmpty() ? "OpendTect" : setup.caption_.buf() );
    body_->setAttribute( Qt::WA_DeleteOnClose, setup.deleteonclose_ );
}


uiMainWin::uiMainWin( uiParent* parnt, const char* nm,
		      int nrstatusflds, bool withmenubar, bool modal )
    : uiParent(nm,0)
    , body_(0)
    , parent_(parnt)
    , popuparea_(Middle)
    , windowClosed(this)
    , activatedone(this)
    , caption_(nm)
{ 
    body_ = new uiMainWinBody( *this, parnt, nm, modal ); 
    setBody( body_ );
    body_->construct( nrstatusflds, withmenubar );
    body_->setWindowIconText( nm && *nm ? nm : "OpendTect" );
}


uiMainWin::uiMainWin( const char* nm, uiParent* parnt )
    : uiParent(nm,0)
    , body_(0)			
    , parent_(parnt)
    , popuparea_(Middle)
    , windowClosed(this)
    , activatedone(this)
    , caption_(nm)
{}


static Threads::Mutex		winlistmutex_;
static ObjectSet<uiMainWin>	orderedwinlist_;

uiMainWin::~uiMainWin()
{
    if ( !body_->deletefrombody_ )
    {
	body_->deletefromod_ = true;
	delete body_;
    }

    winlistmutex_.lock();
    orderedwinlist_ -= this;
    winlistmutex_.unLock();
}


QWidget* uiMainWin::qWidget() const
{ return body_; }

void uiMainWin::provideHelp( const char* winid )
{
    const BufferString fnm = HelpViewer::getURLForWinID( winid );
    static bool shwonly = GetEnvVarYN("DTECT_SHOW_HELPINFO_ONLY");
    if ( !shwonly )
	uiDesktopServices::openUrl( fnm );
}

void uiMainWin::showCredits( const char* winid )
{
    const BufferString fnm = HelpViewer::getCreditsURLForWinID( winid );
    uiDesktopServices::openUrl( fnm );
}

uiStatusBar* uiMainWin::statusBar()		{ return body_->uistatusbar(); }
uiMenuBar* uiMainWin::menuBar()			{ return body_->uimenubar(); }

void uiMainWin::show()
{
    winlistmutex_.lock();
    orderedwinlist_ -= this;
    orderedwinlist_ += this;
    winlistmutex_.unLock();
    body_->go();
}

void uiMainWin::close()				{ body_->close(); }
void uiMainWin::reDraw(bool deep)		{ body_->reDraw(deep); }
bool uiMainWin::poppedUp() const		{ return body_->poppedUp(); }
bool uiMainWin::touch() 			{ return body_->touch(); }
bool uiMainWin::finalised() const		{ return body_->finalised(); }
void uiMainWin::setExitAppOnClose( bool yn )	{ body_->exitapponclose_ = yn; }
void uiMainWin::showMaximized()			{ body_->showMaximized(); }
void uiMainWin::showMinimized()			{ body_->showMinimized(); }
void uiMainWin::showNormal()			{ body_->showNormal(); }
bool uiMainWin::isMaximized() const		{ return body_->isMaximized(); }
bool uiMainWin::isMinimized() const		{ return body_->isMinimized(); }
bool uiMainWin::isHidden() const		{ return body_->isHidden(); }
bool uiMainWin::isModal() const			{ return body_->isModal(); }


void uiMainWin::setCaption( const char* txt )
{
    caption_ = txt;
    body_->setWindowTitle( txt );
}


const char* uiMainWin::caption( bool unique ) const
{
    static BufferString capt;
    capt = unique ? mQStringToConstChar(body_->windowTitle()) : caption_.buf();
    return capt;
}


void uiMainWin::setDeleteOnClose( bool yn )
{ body_->setAttribute( Qt::WA_DeleteOnClose, yn ); }


void uiMainWin::removeDockWindow( uiDockWin* dwin )
{ body_->removeDockWin( dwin ); }


void uiMainWin::addDockWindow( uiDockWin& dwin, Dock d )
{ body_->addDockWin( dwin, d ); }


void uiMainWin::addToolBar( uiToolBar* tb )
{ body_->addToolBar( tb ); }


void uiMainWin::removeToolBar( uiToolBar* tb )
{ body_->removeToolBar( tb ); }


void uiMainWin::addToolBarBreak()
{ body_->addToolBarBreak(); } 

uiPopupMenu& uiMainWin::getToolbarsMenu() const
{ return body_->getToolbarsMenu(); }


void uiMainWin::updateToolbarsMenu()
{ body_->updateToolbarsMenu(); }


const ObjectSet<uiToolBar>& uiMainWin::toolBars() const
{ return body_->toolBars(); } 
    

const ObjectSet<uiDockWin>& uiMainWin::dockWins() const
{ return body_->dockWins(); } 


uiGroup* uiMainWin::topGroup()	    	   { return body_->uiCentralWidg(); }


void uiMainWin::setShrinkAllowed(bool yn)  
    { if ( topGroup() ) topGroup()->setShrinkAllowed(yn); }
 

bool uiMainWin::shrinkAllowed()	 	   
    { return topGroup() ? topGroup()->shrinkAllowed() : false; }


uiObject* uiMainWin::mainobject()
    { return body_->uiCentralWidg()->mainObject(); }


void uiMainWin::toStatusBar( const char* txt, int fldidx, int msecs )
{
    if ( !txt ) txt = "";
    uiStatusBar* sb = statusBar();
    if ( sb )
	sb->message( txt, fldidx, msecs );
    else if ( *txt )
    	UsrMsg(txt);
}


void uiMainWin::setSensitive( bool yn )
{
    if ( menuBar() ) menuBar()->setSensitive( yn );
    body_->setEnabled( yn );
}


uiMainWin* uiMainWin::gtUiWinIfIsBdy(QWidget* mwimpl)
{
    if ( !mwimpl ) return 0;

    uiMainWinBody* _mwb = dynamic_cast<uiMainWinBody*>( mwimpl );
    if ( !_mwb ) return 0;

    return &_mwb->handle();
}


void uiMainWin::setCornerPos( int x, int y )
{ body_->QWidget::move( x, y ); }


uiRect uiMainWin::geometry() const
{
    QRect qrect = body_->frameGeometry();
    uiRect rect( qrect.left(), qrect.top(), qrect.right(), qrect.bottom() );
    return rect;
}


void uiMainWin::setIcon( const ioPixmap& pm )
{ body_->setWindowIcon( *pm.qpixmap() ); }

void uiMainWin::setIconText( const char* txt)
{ body_->setWindowIconText( txt ); }

void uiMainWin::saveSettings()
{ body_->saveSettings(); }

void uiMainWin::readSettings()
{ body_->readSettings(); }

void uiMainWin::raise()
{ body_->raise(); }

void uiMainWin::activateWindow()
{ body_->activateWindow(); }


uiMainWin* uiMainWin::activeWindow()
{
    QWidget* _aw = qApp->activeWindow();
    if ( !_aw )		return 0;

    uiMainWinBody* _awb = dynamic_cast<uiMainWinBody*>(_aw);
    if ( !_awb )	return 0;

    return &_awb->handle();
}


uiMainWin::ActModalTyp uiMainWin::activeModalType()
{
    QWidget* amw = qApp->activeModalWidget();
    if ( !amw )					return None;

    if ( dynamic_cast<uiMainWinBody*>(amw) ) 	return Main;
    if ( dynamic_cast<QMessageBox*>(amw) ) 	return Message;
    if ( dynamic_cast<QFileDialog*>(amw) ) 	return File;
    if ( dynamic_cast<QColorDialog*>(amw) ) 	return Colour;
    if ( dynamic_cast<QFontDialog*>(amw) ) 	return Font;

    return Unknown;
}
    

uiMainWin* uiMainWin::activeModalWindow()
{
    QWidget* amw = qApp->activeModalWidget();
    if ( !amw )	return 0;

    uiMainWinBody* mwb = dynamic_cast<uiMainWinBody*>( amw );
    if ( !mwb )	return 0;

    return &mwb->handle();
}


const char* uiMainWin::activeModalQDlgTitle()
{
    QWidget* amw = qApp->activeModalWidget();
    if ( !amw )
	return 0;

    static BufferString title;
    title = mQStringToConstChar( amw->windowTitle() );
    return title;
}


#define mGetStandardButton( qmb, buttonnr, stdbutcount, stdbut ) \
\
    int stdbutcount = 0; \
    QMessageBox::StandardButton stdbut = QMessageBox::NoButton; \
    for ( unsigned int idx=QMessageBox::Ok; \
	  qmb && idx<=QMessageBox::RestoreDefaults; idx+=idx ) \
    { \
	const QAbstractButton* abstrbut; \
        abstrbut = qmb->button( (QMessageBox::StandardButton) idx ); \
	if ( !abstrbut ) \
	    continue; \
	if ( stdbutcount == buttonnr ) \
	    stdbut = (QMessageBox::StandardButton) idx; \
	stdbutcount++; \
    }

// buttons() function to get all buttons only available from Qt4.5 :-(

const char* uiMainWin::activeModalQDlgButTxt( int buttonnr )
{
    const ActModalTyp typ = activeModalType();
    QWidget* amw = qApp->activeModalWidget();

    if ( typ == Message )
    {
	const QMessageBox* qmb = dynamic_cast<QMessageBox*>( amw ); 
	mGetStandardButton( qmb, buttonnr, stdbutcount, stdbut );

	static BufferString buttext;
        if ( stdbut )
	    buttext = mQStringToConstChar( qmb->button(stdbut)->text() );
	else if ( !stdbutcount )
	    buttext = mQStringToConstChar( qmb->buttonText(buttonnr) );
	else 
	    buttext = "";

	return buttext;
    }

    if ( typ==Colour || typ==Font )
    {
	if ( buttonnr == 0 ) return "Cancel";
	if ( buttonnr == 1 ) return "OK";
	return "";
    }

    if ( typ == File )
    {
	if ( buttonnr == 0 ) return "Cancel";
	if ( buttonnr == 1 )
	{
	    const QFileDialog* qfd = dynamic_cast<QFileDialog*>( amw );

	    if ( qfd->acceptMode() == QFileDialog::AcceptOpen )
	    {
		return qfd->fileMode()==QFileDialog::Directory ||
		       qfd->fileMode()==QFileDialog::DirectoryOnly
		       ? "Choose" : "Open";
	    }
	    if ( qfd->acceptMode() == QFileDialog::AcceptSave ) return "Save";
	}
	return "";
    }
    
    return 0;
}


int uiMainWin::activeModalQDlgRetVal( int buttonnr )
{
    QWidget* amw = qApp->activeModalWidget();
    const QMessageBox* qmb = dynamic_cast<QMessageBox*>( amw ); 
    mGetStandardButton( qmb, buttonnr, stdbutcount, stdbut );

    return stdbut ? ((int) stdbut) : buttonnr;
}


void uiMainWin::closeActiveModalQDlg( int retval )
{
    if ( activeModalWindow() )
	return;

    QWidget* _amw = qApp->activeModalWidget();
    if ( !_amw ) 
	return;

    QDialog* _qdlg = dynamic_cast<QDialog*>(_amw);
    if ( !_qdlg ) 
	return;

    _qdlg->done( retval );
}


void uiMainWin::getTopLevelWindows( ObjectSet<uiMainWin>& windowlist )
{
    windowlist.erase();
    winlistmutex_.lock();
    for ( int idx=0; idx<orderedwinlist_.size(); idx++ )
    {
	if ( !orderedwinlist_[idx]->isHidden() )
	    windowlist += orderedwinlist_[idx];
    }
    winlistmutex_.unLock();
}


void uiMainWin::getModalSignatures( BufferStringSet& signatures )
{
    signatures.erase();
    QWidgetList toplevelwigs = qApp->topLevelWidgets();

    for ( int idx=0; idx<toplevelwigs.count(); idx++ )
    {
	const QWidget* qw = toplevelwigs.at( idx );
	if ( qw->isWindow() && !qw->isHidden() && qw->isModal() )
	{
	    BufferString qwptrstr;
	    sprintf( qwptrstr.buf(), "%p", qw );
	    signatures.add( qwptrstr );
	}
    }
}


const char* uiMainWin::uniqueWinTitle( const char* txt, QWidget* forwindow )
{
    static BufferString wintitle;
    const QWidgetList toplevelwigs = qApp->topLevelWidgets();

    for ( int count=1; true; count++ )
    {
	bool unique = true;
	wintitle = txt;
	if ( wintitle.isEmpty() )
	    wintitle = "<no title>";

	if ( count>1 )
	{
	    wintitle += "  {"; wintitle += count ; wintitle += "}" ;
	}

	for ( int idx=0; idx<toplevelwigs.count(); idx++ )
	{
	    const QWidget* qw = toplevelwigs.at( idx );
	    if ( !qw->isWindow() || qw->isHidden() || qw==forwindow )
		continue;

	    if ( wintitle==mQStringToConstChar(qw->windowTitle())  )
		unique = false;
	}

	if ( unique ) break;
    }
    return wintitle;
}


bool uiMainWin::grab( const char* filenm, int zoom,
		      const char* format, int quality ) const
{
    WId winid = body_->winId();
    if ( zoom <= 0 )
	winid = QApplication::desktop()->winId();
    else if ( zoom>=2 && qApp->activeModalWidget() )
	winid = qApp->activeModalWidget()->winId();

    QPixmap snapshot = QPixmap::grabWindow( winid );
    return snapshot.save( QString(filenm), format, quality );
}


void uiMainWin::activateInGUIThread( const CallBack& cb, bool busywait )
{ body_->activateInGUIThread( cb, busywait ); }


/*!\brief Stand-alone dialog window with optional 'Ok', 'Cancel' and
'Save defaults' button.

*/

#define mHandle static_cast<uiDialog&>(handle_)

class uiDialogBody : public uiMainWinBody
{ 	
public:
			uiDialogBody(uiDialog&,uiParent*,
				     const uiDialog::Setup&);

    int			exec(); 

    void		reject( CallBacker* s )	
			{
			    mHandle.cancelpushed_ = s == cnclbut;
			    if ( mHandle.rejectOK(s) )
				done_(0);
			    else
				uiSetResult( -1 );
			}
                        //!< to be called by a 'cancel' button
    void		accept( CallBacker* s )	
			    { if ( mHandle.acceptOK(s) ) done_(1); }
                        //!< to be called by a 'ok' button
    void		done( int i )
			    { if ( mHandle.doneOK(i) ) done_(i); }

    void		uiSetResult( int v ) { reslt = v; }
    int			uiResult(){ return reslt; }

    void		setOkText( const char* txt );
			//!< OK button disabled when set to empty
    void		setCancelText( const char* txt );
			//!< cancel button disabled when set to empty
    void		enableSaveButton( const char* txt )
			    { 
				setup.savetext_ = txt; 
				setup.savebutton_ = true;
			    }
    void		setSaveButtonChecked( bool yn )
			    { setup.savechecked_ = yn;
			      if ( savebut_cb ) savebut_cb->setChecked(yn); }
    void		setButtonSensitive( uiDialog::Button but, bool yn )
			    { 
				switch ( but )
				{
				case uiDialog::OK     :
				    if ( okbut ) okbut->setSensitive(yn); 
				break;
				case uiDialog::CANCEL :
				    if ( cnclbut ) cnclbut->setSensitive(yn); 
				break;
				case uiDialog::SAVE   : 
				    if ( savebut_cb )
					savebut_cb->setSensitive(yn); 
				    if ( savebut_tb )
					savebut_tb->setSensitive(yn); 
				break;
				case uiDialog::HELP   : 
				    if ( helpbut ) helpbut->setSensitive(yn); 
				break;
				case uiDialog::CREDITS :
				    if ( creditsbut )
					creditsbut->setSensitive(yn); 
				break;
				}
			    }

    void		setTitleText( const char* txt );

    bool		saveButtonChecked() const;
    bool		hasSaveButton() const;
    uiButton*		button( uiDialog::Button but ) 
			    { 
				switch ( but )
				{
				case uiDialog::OK     : return okbut;
				break;
				case uiDialog::CANCEL : return cnclbut;
				break;
				case uiDialog::SAVE   : 
				    return savebut_cb
					? (uiButton*) savebut_cb
					: (uiButton*) savebut_tb;
				break;
				case uiDialog::HELP   : return helpbut;
				case uiDialog::CREDITS: return creditsbut;
				break;
				}
				return 0;
			    }

			//! Separator between central dialog and Ok/Cancel bar?
    void		setSeparator( bool yn )	{ setup.separator_ = yn; }
    bool		separator() const	{ return setup.separator_; }
    void		setHelpID( const char* id ) { setup.helpid_ = id; }
    const char*		helpID() const		{ return setup.helpid_; }

    void		setDlgGrp( uiGroup* cw )	{ dlgGroup=cw; }

    void		setHSpacing( int spc )	{ dlgGroup->setHSpacing(spc); }
    void		setVSpacing( int spc )	{ dlgGroup->setVSpacing(spc); }
    void		setBorder( int b )	{ dlgGroup->setBorder( b ); }

    virtual void        addChild( uiBaseObject& child )
			{ 
			    if ( !initing ) 
				dlgGroup->addChild( child );
			    else
				uiMainWinBody::addChild( child );
			}

    virtual void        manageChld_( uiBaseObject& o, uiObjectBody& b )
			{ 
			    if ( !initing ) 
				dlgGroup->manageChld( o, b );
			}

    virtual void  	attachChild ( constraintType tp,
                                              uiObject* child,
                                              uiObject* other, int margin,
					      bool reciprocal )
                        {
                            if ( !child || initing ) return;
			    dlgGroup->attachChild( tp, child, other, margin,
						   reciprocal ); 
                        }
    void		provideHelp(CallBacker*);
    void		showCredits(CallBacker*);

    const uiDialog::Setup& getSetup() const	{ return setup; }

protected:

    virtual const QWidget* managewidg_() const 
			{ 
			    if ( !initing ) 
				return dlgGroup->pbody()->managewidg();
			    return uiMainWinBody::managewidg_();
			}

    int 		reslt;
    bool		childrenInited;

    uiGroup*            dlgGroup;
    uiDialog::Setup	setup;

    uiPushButton*	okbut;
    uiPushButton*	cnclbut;
    uiToolButton*	helpbut;
    uiToolButton*	creditsbut;

    uiCheckBox*		savebut_cb;
    uiToolButton*	savebut_tb;

    uiSeparator*	horSepar;
    uiLabel*		title;

    void		done_(int);

    virtual void	finalise(bool);
    void		closeEvent(QCloseEvent*);

private:

    void		initChildren();
    uiObject*		createChildren();
    void		layoutChildren(uiObject*);

};


uiDialogBody::uiDialogBody( uiDialog& handle, uiParent* parnt,
			    const uiDialog::Setup& s )
    : uiMainWinBody(handle,parnt,s.wintitle_,s.modal_)
    , dlgGroup( 0 )
    , setup( s )
    , okbut( 0 ), cnclbut( 0 ), savebut_cb( 0 ),  savebut_tb( 0 )
    , helpbut( 0 ), creditsbut( 0 ), title( 0 ) , reslt( 0 )
    , childrenInited(false)
{
    setContentsMargins( 10, 2, 10, 2 );
}


int uiDialogBody::exec()
{ 
    uiSetResult( 0 );

    if ( setup.fixedsize_ )
	setSizePolicy( QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed) );

    move( handle_.getPopupArea() );
    go();

    return uiResult();
}



void uiDialogBody::setOkText( const char* txt )    
{ 
    setup.oktext_ = txt; 
    if ( okbut ) okbut->setText(txt);
}


void uiDialogBody::setTitleText( const char* txt )    
{ 
    setup.dlgtitle_ = txt; 
    if ( title ) 
    { 
	title->setText(txt); 
	uiObjectBody* tb = dynamic_cast<uiObjectBody*>( title->body() ); 
	if ( tb && !tb->itemInited() )
	    title->setPrefWidthInChar( 
		    mMAX( tb->prefWidthInCharSet(), strlen(txt) + 2 )); 
    }
}

void uiDialogBody::setCancelText( const char* txt ) 
{ 
    setup.canceltext_ = txt; 
    if ( cnclbut ) cnclbut->setText(txt);
}


bool uiDialogBody::hasSaveButton() const
{
    return savebut_cb;
}


bool uiDialogBody::saveButtonChecked() const
{ 
    return savebut_cb ? savebut_cb->isChecked() : false;
}


/*! Hides the box, which also exits the event loop in case of a modal box.  */
void uiDialogBody::done_( int v )
{
    uiSetResult( v );
    close();
}


void uiDialogBody::closeEvent( QCloseEvent* ce )
{
    const int refnr = handle_.beginCmdRecEvent( "Close" );

    reject(0);
    if ( reslt == -1 )
	ce->ignore();
    else
	ce->accept();

    handle_.endCmdRecEvent( refnr );
}


/*!
    Construct OK and Cancel buttons just before the first show.
    This gives chance not to construct them in case OKtext and CancelText have
    been set to ""
*/
void uiDialogBody::finalise( bool ) 
{
    uiMainWinBody::finalise( false ); 

    handle_.finaliseStart.trigger( handle_ );

    dlgGroup->finalise();

    if ( !childrenInited ) 
	initChildren();

    finaliseChildren();

    handle_.finaliseDone.trigger( handle_ );
}


void uiDialogBody::initChildren()
{
    uiObject* lowestobject = createChildren();
    layoutChildren( lowestobject );

    if ( okbut )
    {
	okbut->activated.notify( mCB(this,uiDialogBody,accept) );
	okbut->setDefault();
    }
    if ( cnclbut )
    {
	cnclbut->activated.notify( mCB(this,uiDialogBody,reject) );
	if ( !okbut )
	    cnclbut->setDefault();
    }
    if ( helpbut )
	helpbut->activated.notify( mCB(this,uiDialogBody,provideHelp) );
    if ( creditsbut )
	creditsbut->activated.notify( mCB(this,uiDialogBody,showCredits) );

    childrenInited = true;
}


uiObject* uiDialogBody::createChildren()
{
    if ( !setup.oktext_.isEmpty() )
	okbut = new uiPushButton( centralWidget_, setup.oktext_, true );
    if ( !setup.canceltext_.isEmpty() )
	cnclbut = new uiPushButton( centralWidget_, setup.canceltext_, true );

    if ( setup.savebutton_ && !setup.savetext_.isEmpty() )
    {
	if ( setup.savebutispush_ )
	    savebut_tb = new uiToolButton( centralWidget_, setup.savetext_,
		    			   ioPixmap("savefmt.png") );
	else
	{
	    savebut_cb = new uiCheckBox( centralWidget_, setup.savetext_ );
	    savebut_cb->setChecked( setup.savechecked_ );
	}
    }
    mDynamicCastGet( uiDialog&, dlg, handle_ );
    const BufferString hid( dlg.helpID() );
    if ( !hid.isEmpty() && hid != "-" )
    {
	const ioPixmap pixmap( "contexthelp.png" );
	helpbut = new uiToolButton( centralWidget_, "&Help button", pixmap );
	helpbut->setPrefWidthInChar( 5 );
	static bool shwhid = GetEnvVarYN( "DTECT_SHOW_HELP" );
#ifdef __debug__
	shwhid = true;
#endif
	helpbut->setToolTip( shwhid ? hid.buf() : "Help on this window" );
	if ( dlg.haveCredits() )
	{
	    const ioPixmap pixmap( "credits.png" );
	    creditsbut = new uiToolButton( centralWidget_, "&Credits button",
					    pixmap );
	    creditsbut->setPrefWidthInChar( 5 );
	    creditsbut->setToolTip( "Show credits" );
	    creditsbut->attach( rightOf, helpbut );
	}
    }
    if ( !setup.menubar_ && !setup.dlgtitle_.isEmpty() )
    {
	title = new uiLabel( centralWidget_, setup.dlgtitle_ );

	uiObject* obj = setup.separator_ 
			    ? (uiObject*) new uiSeparator(centralWidget_)
			    : (uiObject*) title;

	if ( obj != title )
	{
	    title->attach( centeredAbove, obj );
	    obj->attach( stretchedBelow, title, -2 );
	}
	if ( setup.mainwidgcentered_ )
	    dlgGroup->attach( centeredBelow, obj );
	else
	    dlgGroup->attach( stretchedBelow, obj );
    }

    uiObject* lowestobj = dlgGroup->mainObject();
    if ( setup.separator_ && ( okbut || cnclbut || savebut_cb || 
			       savebut_tb || helpbut) )
    {
	horSepar = new uiSeparator( centralWidget_ );
	horSepar->attach( stretchedBelow, dlgGroup, -2 );
	lowestobj = horSepar;
    }

    return lowestobj;
}


void uiDialogBody::layoutChildren( uiObject* lowestobj )
{
    uiObject* leftbut = okbut;
    uiObject* rightbut = cnclbut;
    uiObject* exitbut = okbut ? okbut : cnclbut;
    uiObject* centerbut = helpbut;
    uiObject* extrabut = savebut_tb;

    if ( !okbut || !cnclbut )
    {
	leftbut = rightbut = 0;
	if ( exitbut )
	{
	    centerbut = exitbut;
	    extrabut = helpbut;
	    leftbut = savebut_tb;
	}
    }

    if ( !centerbut )
    {
	centerbut = extrabut;
	extrabut = 0;
    }

    const int hborderdist = 1;
    const int vborderdist = 5;

#define mCommonLayout(but) \
    but->attach( ensureBelow, lowestobj ); \
    but->attach( bottomBorder, vborderdist )

    if ( leftbut )
    {
	mCommonLayout(leftbut);
	leftbut->attach( leftBorder, hborderdist );
    }

    if ( rightbut )
    {
	mCommonLayout(rightbut);
	rightbut->attach( rightBorder, hborderdist );
	if ( leftbut )
	    rightbut->attach( ensureRightOf, leftbut );
    }

    if ( centerbut )
    {
	mCommonLayout(centerbut);
	centerbut->attach( hCentered );
	if ( leftbut )
	    centerbut->attach( ensureRightOf, leftbut );
	if ( rightbut )
	    centerbut->attach( ensureLeftOf, rightbut );
    }

    if ( savebut_cb )
    {
	savebut_cb->attach( extrabut ? leftOf : rightOf, exitbut );
	if ( centerbut && centerbut != exitbut )
	    centerbut->attach( ensureRightOf, savebut_cb );
	if ( rightbut && rightbut != exitbut )
	    rightbut->attach( ensureRightOf, savebut_cb );
    }

    if ( extrabut )
	extrabut->attach( rightOf, centerbut );
}


void uiDialogBody::provideHelp( CallBacker* )
{
    mDynamicCastGet( uiDialog&, dlg, handle_ );
    uiMainWin::provideHelp( dlg.helpID() );
}


void uiDialogBody::showCredits( CallBacker* )
{
    mDynamicCastGet( uiDialog&, dlg, handle_ );
    uiMainWin::showCredits( dlg.helpID() );
}


#define mBody static_cast<uiDialogBody*>(body_)

uiDialog::uiDialog( uiParent* p, const uiDialog::Setup& s )
	: uiMainWin( s.wintitle_, p )
    	, cancelpushed_(false)
{
    body_= new uiDialogBody( *this, p, s );
    setBody( body_ );
    body_->construct( s.nrstatusflds_, s.menubar_ );
    uiGroup* cw= new uiGroup( body_->uiCentralWidg(), "Dialog box client area");

    cw->setStretch( 2, 2 );
    mBody->setDlgGrp( cw );
    setTitleText( s.dlgtitle_ );
    ctrlstyle_ = DoAndLeave;
}


void uiDialog::setCtrlStyle( uiDialog::CtrlStyle cs )
{
    switch ( cs )
    {
    case DoAndLeave:
	setOkText( "&Ok" );
	setCancelText( "&Cancel" );
    break;
    case DoAndStay:
	setOkText( "&Go" );
	setCancelText( "&Dismiss" );
    break;
    case LeaveOnly:
	setOkText( mBody->finalised() ? "&Dismiss" : "" );
	setCancelText( "&Dismiss" );
    break;
    case DoAndProceed:
	setOkText( "&Proceed" );
	setCancelText( "&Dismiss" );
    break;
    }

    ctrlstyle_ = cs;
}


bool uiDialog::haveCredits() const
{
    return HelpViewer::hasSpecificCredits( helpID() );
}


int uiDialog::go()
{ 
    winlistmutex_.lock();
    orderedwinlist_ -= this;
    orderedwinlist_ += this;
    winlistmutex_.unLock();
    return mBody->exec();
}


const uiDialog::Setup& uiDialog::setup() const	{ return mBody->getSetup(); }
void uiDialog::reject( CallBacker* cb)		{ mBody->reject( cb ); }
void uiDialog::accept( CallBacker*cb)		{ mBody->accept( cb ); }
void uiDialog::done( int i )			{ mBody->done( i ); }
void uiDialog::setHSpacing( int s )		{ mBody->setHSpacing(s); }
void uiDialog::setVSpacing( int s )		{ mBody->setVSpacing(s); }
void uiDialog::setBorder( int b )		{ mBody->setBorder(b); }
void uiDialog::setTitleText( const char* txt )	{ mBody->setTitleText(txt); }
void uiDialog::setOkText( const char* txt )	{ mBody->setOkText(txt); }
void uiDialog::setCancelText( const char* txt )	{ mBody->setCancelText(txt);}
void uiDialog::enableSaveButton(const char* t)  { mBody->enableSaveButton(t); }
uiButton* uiDialog::button(Button b)		{ return mBody->button(b); }
void uiDialog::setSeparator( bool yn )		{ mBody->setSeparator(yn); }
bool uiDialog::separator() const		{ return mBody->separator(); }
void uiDialog::setHelpID( const char* id )	{ mBody->setHelpID(id); }
const char* uiDialog::helpID() const		{ return mBody->helpID(); }
int uiDialog::uiResult() const			{ return mBody->uiResult(); }
void uiDialog::setModal( bool yn )		{ mBody->setModal( yn ); }
bool uiDialog::isModal() const			{ return mBody->isModal(); }

void uiDialog::setButtonSensitive(uiDialog::Button b, bool s ) 
    { mBody->setButtonSensitive(b,s); }
void uiDialog::setSaveButtonChecked(bool b) 
    { mBody->setSaveButtonChecked(b); }
bool uiDialog::saveButtonChecked() const
    { return mBody->saveButtonChecked(); }
bool uiDialog::hasSaveButton() const
    { return mBody->hasSaveButton(); }
void uiDialog::setCaption( const char* txt )
    { caption_ = txt; mBody->setWindowTitle( txt ); }
