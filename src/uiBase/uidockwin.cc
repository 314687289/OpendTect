/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          13/02/2002
 RCS:           $Id: uidockwin.cc,v 1.23 2007-02-28 07:32:12 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidockwin.h"
#include "uigroup.h"
#include "uiparentbody.h"

#ifdef USEQT3
# include <qdockwindow.h>
#else
# include <QDockWidget>
#endif

#if defined( __mac__ ) || defined( __win__ )
# define _redraw_hack_
# include "timer.h"
#endif


class uiDockWinBody : public uiParentBody
		    , public mQDockWindow
{
public:
			uiDockWinBody( uiDockWin& handle, uiParent* parnt,
				       const char* nm );

    virtual		~uiDockWinBody();
    void		construct();

#define mHANDLE_OBJ     uiDockWin
#define mQWIDGET_BASE   mQDockWindow
#define mQWIDGET_BODY   mQDockWindow
#define UIBASEBODY_ONLY
#define UIPARENT_BODY_CENTR_WIDGET
#include                "i_uiobjqtbody.h"

protected:

    virtual void	finalise();

#ifdef _redraw_hack_

// the doc windows are not correctly drawn on the mac at pop-up

    virtual void        resizeEvent( QResizeEvent* ev )
    {
	mQDockWindow::resizeEvent(ev);

	if ( redrtimer.isActive() ) redrtimer.stop();
	redrtimer.start( 300, true );
    }

// not at moves...
    virtual void        moveEvent( QMoveEvent* ev )
    {
	mQDockWindow::moveEvent(ev);

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
#ifdef USEQT3
	, QDockWindow( InDock, parnt && parnt->pbody() ?  
					parnt->pbody()->qwidget() : 0, nm ) 
#else
        , QDockWidget( nm )
#endif
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

#ifndef USEQT3
    QDockWidget::setFeatures( QDockWidget::AllDockWidgetFeatures );
#endif
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
    { centralWidget_->finalise();  finaliseChildren(); }


uiDockWin::uiDockWin( uiParent* parnt, const char* nm )
    : uiParent( nm, 0 )
    , body_( 0 )
{ 
    body_= new uiDockWinBody( *this, parnt, nm ); 
    setBody( body_ );
    body_->construct();
}


uiDockWin::~uiDockWin()
{ 
    delete body_; 
}


void uiDockWin::setDockName( const char* nm )
{ body_->qwidget()->setName( nm ); }

const char* uiDockWin::getDockName() const
{ return body_->qwidget()->name(); }


uiGroup* uiDockWin::topGroup()	    	   
{ 
    return body_->uiCentralWidg(); 
}


uiObject* uiDockWin::mainobject()
{ 
    return body_->uiCentralWidg()->mainObject(); 
}


void uiDockWin::setResizeEnabled( bool yn )
{
#ifdef USEQT3
    body_->setResizeEnabled( yn );
#endif
}

bool uiDockWin::isResizeEnabled() const
{
#ifdef USEQT3
    return body_->isResizeEnabled();
#else
    return true;
#endif
}


void uiDockWin::setFloating( bool yn )
{
#ifndef USEQT3
    body_->setFloating( yn );
#endif
}


bool uiDockWin::isFloating() const
{
#ifdef USEQT3
    return false;
#else
    return body_->isFloating();
#endif
}


mQDockWindow* uiDockWin::qwidget()
{ return body_; }
