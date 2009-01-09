/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          13/02/2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uidockwin.cc,v 1.32 2009-01-09 10:28:17 cvsnanne Exp $";

#include "uidockwin.h"
#include "uigroup.h"
#include "uimainwin.h"
#include "uiparentbody.h"

#include <QDockWidget>

#if defined( __mac__ ) || defined( __win__ )
# define _redraw_hack_
# include "timer.h"
#endif


class uiDockWinBody : public uiParentBody, public QDockWidget
{
public:
			uiDockWinBody( uiDockWin& handle, uiParent* parnt,
				       const char* nm );

    virtual		~uiDockWinBody();
    void		construct();

#define mHANDLE_OBJ     uiDockWin
#define mQWIDGET_BASE   QDockWidget
#define mQWIDGET_BODY   QDockWidget
#define UIBASEBODY_ONLY
#define UIPARENT_BODY_CENTR_WIDGET
#include                "i_uiobjqtbody.h"

protected:

    virtual void	finalise();

#ifdef _redraw_hack_

// the doc windows are not correctly drawn on the mac at pop-up

    virtual void        resizeEvent( QResizeEvent* ev )
    {
	QDockWidget::resizeEvent(ev);

	if ( redrtimer.isActive() ) redrtimer.stop();
	redrtimer.start( 300, true );
    }

// not at moves...
    virtual void        moveEvent( QMoveEvent* ev )
    {
	QDockWidget::moveEvent(ev);

	if ( redrtimer.isActive() ) redrtimer.stop();
	redrtimer.start( 300, true );
    }

    void		redrTimTick( CallBacker* cb )
    {
	//TODO: check this out.  Are scenes deleted or 'just' hidden??
	if ( isHidden() ) return;

	hide();
	show();
    }

    static Timer	redrtimer;
    CallBack		mycallback_;
#endif

};

#ifdef _redraw_hack_
Timer uiDockWinBody::redrtimer;
#endif


uiDockWinBody::uiDockWinBody( uiDockWin& handle__, uiParent* parnt, 
			      const char* nm )
	: uiParentBody( nm )
        , QDockWidget( nm )
	, handle_( handle__ )
	, initing( true )
	, centralWidget_( 0 )
#ifdef _redraw_hack_
	, mycallback_( mCB(this, uiDockWinBody, redrTimTick) )
#endif
{
#ifdef _redraw_hack_
    redrtimer.tick.notify( mycallback_ );
    redrtimer.start( 500, true );
#endif

    QDockWidget::setFeatures( QDockWidget::DockWidgetMovable | 
	    		      QDockWidget::DockWidgetFloatable );
    setObjectName( nm );
}


void uiDockWinBody::construct()
{
    centralWidget_ = new uiGroup( &handle(), "uiDockWin central widget" );
    setWidget( centralWidget_->body()->qwidget() ); 

    centralWidget_->setIsMain(true);
    centralWidget_->setBorder(0);
    centralWidget_->setStretch(2,2);

    initing = false;
}


uiDockWinBody::~uiDockWinBody( )
{
#ifdef _redraw_hack_
    redrtimer.tick.remove( mycallback_ );
#endif
    delete centralWidget_; centralWidget_ = 0;
}


void uiDockWinBody::finalise()
{
    handle_.finaliseStart.trigger( handle_ );
    centralWidget_->finalise();
    finaliseChildren();
    handle_.finaliseDone.trigger( handle_ );
}


// ----- uiDockWin -----
uiDockWin::uiDockWin( uiParent* parnt, const char* nm )
    : uiParent( nm, 0 )
    , body_( 0 )
    , parent_( parnt )
{ 
    body_= new uiDockWinBody( *this, parnt, nm ); 
    setBody( body_ );
    body_->construct();
}


uiDockWin::~uiDockWin()
{ delete body_; }


void uiDockWin::setObject( uiObject* obj )
{
    if ( !obj ) return;
    body_->setWidget( obj->body()->qwidget() );
}


void uiDockWin::setGroup( uiGroup* grp )
{
    if ( !grp ) return;
    setObject( grp->attachObj() );
}


const char* uiDockWin::getDockName() const
{
    static BufferString docknm;
    docknm = mQStringToConstChar( body_->qwidget()->objectName() );
    return docknm;
}

void uiDockWin::setDockName( const char* nm )
{ body_->qwidget()->setObjectName( nm ); }

uiGroup* uiDockWin::topGroup()	    	   
{ return body_->uiCentralWidg(); }


uiMainWin* uiDockWin::mainwin()
{ 
    mDynamicCastGet(uiMainWin*,uimw,parent_);
    return uimw;
}


uiObject* uiDockWin::mainobject()
{ return body_->uiCentralWidg()->mainObject(); }

void uiDockWin::setFloating( bool yn )
{ body_->setFloating( yn ); }

bool uiDockWin::isFloating() const
{ return body_->isFloating(); }

QDockWidget* uiDockWin::qwidget()
{ return body_; }
