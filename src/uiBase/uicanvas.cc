/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uicanvas.cc,v 1.25 2006-01-31 16:39:07 cvshelene Exp $
________________________________________________________________________

-*/

#include "uicanvas.h"
#include "errh.h"
#include "uiobj.h"
#include "i_uidrwbody.h"

#include "iodrawtool.h"
#include "uimouse.h"

#ifdef USEQT4
# include <q3scrollview.h>
# include <QRubberBand>
# define mQScrollView	Q3ScrollView
# define mRubberBanding	qrubber
#else
# include <qscrollview.h>
# define mQScrollView	QScrollView
# define mRubberBanding	rbnding
#endif
#include <qpainter.h>


#define mButState( e ) ( e->state() | e->button() )

class uiScrollViewBody;

int uiCanvasDefaults::defaultWidth  = 600;
int uiCanvasDefaults::defaultHeight = 400;


class uiCanvasBody : public uiDrawableObjBody<uiCanvas,QWidget>
{

public:
                        uiCanvasBody( uiCanvas& handle, uiParent* p,
				      const char *nm="uiCanvasBody")
			    : uiDrawableObjBody<uiCanvas,QWidget>
				( handle, p, nm ) 
			    {
				setStretch( 2, 2 );
				setPrefWidth( uiCanvasDefaults::defaultWidth );
				setPrefHeight( uiCanvasDefaults::defaultHeight);
			    }

    virtual             ~uiCanvasBody() {};
};



//! Derived class from QScrollView in order to handle 'n relay Qt's events
/*
    defined locally in .cc file.
*/
class uiScrollViewBody : public uiDrawableObjBody<uiScrollView,mQScrollView>
{
public:

			uiScrollViewBody( uiScrollView& handle,
					  uiParent* p=0,
					  const char *nm="uiScrollViewBody" )
			    : uiDrawableObjBody<uiScrollView,mQScrollView> 
				( handle, p, nm )
			    , rubberstate( OD::NoButton )
#ifdef USEQT4
			    , qrubber( 0 )
#else
			    , rbnding( false )
#endif
			    , aspectrat( 0.0 ), rbidx( 0 ) 
			    {
				setStretch( 2, 2 );
				setPrefContentsWidth(
					    uiCanvasDefaults::defaultWidth );
				setPrefContentsHeight( 
					    uiCanvasDefaults::defaultHeight);
			    }

    OD::ButtonState 	rubberstate;
    float		aspectrat;

    virtual QPaintDevice* mQPaintDevice()	{ return viewport(); }

    uiRect		visibleArea() const;

    void		setPrefContentsWidth( int w )      
			{ 
			    setPrefWidth( w + 2*frameWidth() ); 
			}
    void		setPrefContentsHeight( int h )     
			{ 
			    setPrefHeight( h + 2*frameWidth() ); 
			}

    virtual uiSize	actualsize( bool include_border = true) const
			{
			    uiSize sz= uiObjectBody::actualsize(include_border);
			    if ( include_border ) return sz;

			    int fw = frameWidth();
			    int w = sz.hNrPics()-2*fw;
			    if ( w<1 ) w=1;
			    int h = sz.vNrPics()-2*fw;
			    if ( h<1 ) h=1;
			    return uiSize( w, h, true );
			}


    void		setRubberBandingOn(OD::ButtonState b)
    					{ rubberstate = b; }
    OD::ButtonState	rubberBandingOn() const		{ return rubberstate; }
    void		setAspectRatio(float r)		{ aspectrat = r; }
    float		aspectRatio()			{ return aspectrat; }

    virtual void	reDraw( bool deep )		{ updateContents(); }
protected:

    virtual void	drawContents ( QPainter * p, int clipx,
			    int clipy, int clipw, int cliph );
    virtual void	resizeEvent( QResizeEvent * );


    //! over-ride DrawableObjBody impl, because we want to use drawContents()
    virtual void	paintEvent( QPaintEvent* ev )
			{ mQScrollView::paintEvent(ev); }

    virtual void	contentsMousePressEvent( QMouseEvent * e );
    virtual void	contentsMouseMoveEvent ( QMouseEvent * e );
    virtual void	contentsMouseReleaseEvent ( QMouseEvent * e );
    virtual void	contentsMouseDoubleClickEvent ( QMouseEvent * e );

    uiRect		rubber;
    int			rbidx;
#ifdef USEQT4
    QRubberBand*	qrubber;
#else
    bool		rbnding;
#endif


};


uiRect uiScrollViewBody::visibleArea() const
{

    uiSize vpSize = actualsize(false);

    uiPoint tl( contentsX(), contentsY() );
    return uiRect( tl, mMAX( visibleWidth(), vpSize.hNrPics()),
                       mMAX( visibleHeight(), vpSize.vNrPics()) );
}


void uiScrollViewBody::drawContents ( QPainter * p, int clipx,
                                            int clipy, int clipw, int cliph )
{

    if ( !drawTool()->setActivePainter( p ))
    {
        pErrMsg( "Could not make Qpainter active." );
        return;
    }

    handlePaintEvent( uiRect( uiPoint(clipx,clipy), clipw, cliph) );
}


void uiScrollViewBody::resizeEvent( QResizeEvent *QREv )
{
    const QSize& os = QREv->oldSize();
    uiSize oldSize( os.width(), os.height(), true );

    const QSize& ns = QREv->size();
    uiSize nwSize( ns.width()  - 2 * frameWidth(), 
                   ns.height() - 2 * frameWidth(), true );

    handleResizeEvent( QREv, oldSize, nwSize );

    viewport()->update();
}


void uiScrollViewBody::contentsMousePressEvent ( QMouseEvent * e )
{
    if ( mButState( e ) == rubberstate )
    {
	rbidx = 0;
#ifdef USEQT4
	qrubber = new QRubberBand( QRubberBand::Rectangle, viewport() );
#else
	rbnding = true;

	QPainter paint; 
	paint.begin( viewport() );
	paint.setRasterOp( NotROP );
	paint.setPen( QPen( Qt::black, 1, Qt::DotLine ) );
	paint.setBrush( Qt::NoBrush );
#endif
	rubber.setRight( e->x() );
	rubber.setLeft( e->x() );
	rubber.setBottom(  e->y() );
	rubber.setTop( e->y() );

	int xvp, yvp;
	contentsToViewport ( rubber.left(), rubber.top(), xvp, yvp );

#ifdef USEQT4
	qrubber->setGeometry( xvp, yvp, rubber.right() - rubber.left(), 
			rubber.bottom() - rubber.top() ); 
#else
	paint.drawRect( xvp, yvp, rubber.right() - rubber.left(), 
			rubber.bottom() - rubber.top() ); 
	paint.end();
#endif
    }
    else
    {
	OD::ButtonState bSt = ( OD::ButtonState )(  mButState( e ) );
	uiMouseEvent evt( bSt, e->x(), e->y() );

	handle_.mousepressed.trigger( evt, handle_ );
    } 
}


void uiScrollViewBody::contentsMouseMoveEvent( QMouseEvent * e )
{
// TODO: start a timer, so no move-events required to continue scrolling

    if ( mRubberBanding && mButState( e ) == rubberstate )
    {
	int xvp, yvp;

#ifndef USEQT4
	QPainter paint; 
	paint.begin( viewport() );
	paint.setRasterOp( NotROP );
	paint.setPen( QPen( Qt::black, 1, Qt::DotLine ) );
	paint.setBrush( Qt::NoBrush );

	contentsToViewport ( rubber.left(), rubber.top(), xvp, yvp ); 
	paint.drawRect( xvp, yvp, rubber.right() - rubber.left(), 
			rubber.bottom() - rubber.top() ); // erases old rubber
#endif
	if( aspectrat )
	{
	    int xfac = e->x() > rubber.left() ? 1 : -1;
	    int yfac = e->y() > rubber.top() ? 1 : -1;

	    while ( (e->x() - rubber.left()) * xfac > (rbidx+1) ) 
		rbidx++;	

	    while ( (e->y() - rubber.top() ) * yfac > ( aspectrat*(rbidx+1) ) )
		rbidx++;	

	    while ( ( ((e->x() - rubber.left())*xfac) < (rbidx-1) ) && 
		    ( ((e->y()- rubber.top())*yfac) < ( aspectrat*(rbidx-1))) )
		rbidx--;

	    rubber.setHNrPics( rbidx*xfac );
	    rubber.setVNrPics( mNINT(rbidx * aspectrat*yfac) );
	}
	else
	{
	    rubber.setRight( e->x() );
	    rubber.setBottom(  e->y() );
	}

	ensureVisible( rubber.right(), rubber.bottom() );

	contentsToViewport ( rubber.left(), rubber.top(), xvp, yvp );
#ifdef USEQT4
	qrubber->setGeometry( xvp, yvp, rubber.right() - rubber.left(), 
			      rubber.bottom() - rubber.top() ); 
#else
	paint.drawRect( xvp, yvp, rubber.right() - rubber.left(), 
			rubber.bottom() - rubber.top() );
	paint.end();
#endif
    }
    else
    {
	OD::ButtonState bSt = ( OD::ButtonState )(  mButState( e ) );
	uiMouseEvent evt( bSt, e->x(), e->y() );

	handle_.mousemoved.trigger( evt, handle_ );
    } 
}


void uiScrollViewBody::contentsMouseReleaseEvent ( QMouseEvent * e )
{
    if ( mRubberBanding && mButState( e ) == rubberstate )
    {
#ifdef USEQT4
	delete qrubber; qrubber = 0;
#else
	int xvp, yvp;
	rbnding = false;

	QPainter paint; 
	paint.begin( viewport() );
	paint.setRasterOp( NotROP );
	paint.setPen( QPen( Qt::black, 1, Qt::DotLine ) );
	paint.setBrush( Qt::NoBrush );

	contentsToViewport ( rubber.left(), rubber.top(), xvp, yvp ); 
	paint.drawRect( xvp, yvp, rubber.right() - rubber.left(), 
			rubber.bottom() - rubber.top() ); 
	paint.end();
#endif
	rubber.checkCorners();
	if( rubber.hNrPics() > 10 && rubber.vNrPics() > 10 )
	    handle_.rubberBandHandler( rubber );
    }
    else
    {
	OD::ButtonState bSt = ( OD::ButtonState )(  mButState( e ) );
	uiMouseEvent evt( bSt, e->x(), e->y() );

	handle_.mousereleased.trigger( evt, handle_ );
    } 
}


void uiScrollViewBody::contentsMouseDoubleClickEvent ( QMouseEvent * e )
{
    OD::ButtonState bSt = ( OD::ButtonState )(  mButState( e ) );
    uiMouseEvent evt( bSt, e->x(), e->y() );
    handle_.mousedoubleclicked.trigger( evt, handle_ );
}



uiCanvas::uiCanvas( uiParent* p, const char *nm )
    : uiDrawableObj( p,nm, mkbody(p,nm) ) {}

uiCanvasBody& uiCanvas::mkbody( uiParent* p,const char* nm)
{
    body_ = new uiCanvasBody( *this,p,nm );
    return *body_;
}



uiScrollView::uiScrollView( uiParent* p, const char *nm )
    : uiDrawableObj( p,nm, mkbody(p,nm) )
    , mousepressed(this)
    , mousemoved( this )
    , mousereleased( this )
    , mousedoubleclicked( this ) {}

uiScrollViewBody& uiScrollView::mkbody( uiParent* p,const char* nm)
{
    body_ = new uiScrollViewBody( *this,p,nm );
    return *body_;
}


void uiScrollView::setScrollBarMode( ScrollBarMode mode, bool hor )
{
    mQScrollView::ScrollBarMode qmode = mQScrollView::Auto;
    if ( mode == AlwaysOff )
	qmode = mQScrollView::AlwaysOff;
    else if ( mode == AlwaysOn )
	qmode = mQScrollView::AlwaysOn;
    if ( hor )
	body_->setHScrollBarMode( qmode );
    else
	body_->setVScrollBarMode( qmode );
}


uiScrollView::ScrollBarMode uiScrollView::getScrollBarMode( bool hor ) const
{
    mQScrollView::ScrollBarMode qmode;
    qmode = hor ? body_->hScrollBarMode() : body_->vScrollBarMode();
    if ( qmode == mQScrollView::AlwaysOff )
	return AlwaysOff;
    if ( qmode == mQScrollView::AlwaysOn )
	return AlwaysOn;
    return Auto;
}


void uiScrollView::updateContents()	
{ body_->viewport()->update(); }


void uiScrollView::updateContents( uiRect area, bool erase )
{ 
    body_->updateContents( area.left(), area.top(),
			   area.hNrPics(), area.vNrPics() );

}

void uiScrollView::resizeContents( int w, int h )
    { body_->resizeContents(w,h); }

void uiScrollView::setContentsPos( uiPoint topLeft )
    { body_->setContentsPos(topLeft.x(),topLeft.y()); }


uiRect uiScrollView::visibleArea() const
    { return body_->visibleArea(); }


void uiScrollView::setPrefWidth( int w )        
    { body_->setPrefWidth( w + 2*frameWidth() ); }


void uiScrollView::setPrefHeight( int h )        
    { body_->setPrefHeight( h + 2*frameWidth() ); }


void uiScrollView::setMaximumWidth( int w )
    { body_->setMaximumWidth( w ); }


void uiScrollView::setMaximumHeight( int h )
    { body_->setMaximumHeight( h ); }


int  uiScrollView::frameWidth() const
    { return body_->frameWidth(); }


void  uiScrollView::setRubberBandingOn(OD::ButtonState st)
    { body_->setRubberBandingOn(st); }

OD::ButtonState uiScrollView::rubberBandingOn() const
    { return body_->rubberBandingOn(); }

void  uiScrollView::setAspectRatio( float r )
    { body_->setAspectRatio(r); }

float  uiScrollView::aspectRatio()
    { return body_->aspectRatio(); }

void uiScrollView::setMouseTracking( bool yn )
    { body_->viewport()->setMouseTracking(yn); }

