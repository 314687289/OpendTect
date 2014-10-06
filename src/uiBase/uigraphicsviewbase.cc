/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		March 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uigraphicsviewbase.h"

#include "draw.h"
#include "mouseevent.h"
#include "uigraphicsscene.h"
#include "uimouseeventblockerbygesture.h"
#include "uiobjbody.h"

#include <QApplication>
#include <QGesture>
#include <QGraphicsView>
#include <QScrollBar>
#include <QTouchEvent>
#include <QWheelEvent>

mUseQtnamespace

static const int cDefaultWidth  = 1;
static const int cDefaultHeight = 1;


class TouchSceneEventFilter : public QObject
{
public:

bool eventFilter(QObject *obj, QEvent *event )
{
    if ( event->type()==QEvent::TouchBegin ||
	event->type()==QEvent::TouchUpdate ||
	event->type()==QEvent::TouchEnd )
    {
	event->accept();
	return true;
    }

    return false;
}

};


class uiGraphicsViewBody :
    public uiObjBodyImpl<uiGraphicsViewBase,QGraphicsView>
{
public:

uiGraphicsViewBody( uiGraphicsViewBase& hndle, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiGraphicsViewBase,QGraphicsView>(hndle,p,nm)
    , mousehandler_(*new MouseEventHandler)
    , keyboardhandler_(*new KeyboardEventHandler)
    , gestureeventhandler_(*new GestureEventHandler)
    , startpos_(-1,-1)
    , handle_(hndle)
    , currentpinchscale_(0)
    , mouseeventblocker_(*new uiMouseEventBlockerByGestures(1000))
{
    setStretch( 2, 2 );
    setPrefWidth( cDefaultWidth );
    setPrefHeight( cDefaultHeight );
    setTransformationAnchor( QGraphicsView::AnchorUnderMouse );
    viewport()->setAttribute( Qt::WA_AcceptTouchEvents );
    grabGesture( Qt::PinchGesture );
}

~uiGraphicsViewBody()
{
    delete &mousehandler_;
    delete &keyboardhandler_;
    delete &gestureeventhandler_;
    delete &mouseeventblocker_;
}

MouseEventHandler& mouseEventHandler()
{ return mousehandler_; }

KeyboardEventHandler& keyboardEventHandler()
{ return keyboardhandler_; }

GestureEventHandler& gestureEventHandler()
{ return gestureeventhandler_; }

const uiPoint& getStartPos() const	{ return startpos_; }

protected:

    uiPoint			startpos_;
    OD::ButtonState		buttonstate_;
    MouseEventHandler&		mousehandler_;
    KeyboardEventHandler&	keyboardhandler_;
    GestureEventHandler&	gestureeventhandler_;
    uiGraphicsViewBase&		handle_;
    float			currentpinchscale_;

    uiMouseEventBlockerByGestures&   mouseeventblocker_;

    void			wheelEvent(QWheelEvent*);
    void			resizeEvent(QResizeEvent*);
    void			paintEvent(QPaintEvent*);
    void			mouseMoveEvent(QMouseEvent*);
    void			mouseReleaseEvent(QMouseEvent*);
    void			mousePressEvent(QMouseEvent*);
    void			mouseDoubleClickEvent(QMouseEvent*);
    void			keyPressEvent(QKeyEvent*);
    bool			viewportEvent(QEvent*);
    void			scrollContentsBy (int,int);
    bool			event(QEvent*);
    bool			gestureEvent(QGestureEvent*);
};


void uiGraphicsViewBody::mouseMoveEvent( QMouseEvent* ev )
{
    MouseEvent me( buttonstate_, ev->x(), ev->y() );
    mousehandler_.triggerMovement( me );
    QGraphicsView::mouseMoveEvent( ev );
}


bool uiGraphicsViewBody::viewportEvent( QEvent* ev )
{
    if ( mouseeventblocker_.updateFromEvent(ev) )
    {
	ev->accept();
	return true;
    }
    else
    {
	return QGraphicsView::viewportEvent( ev );
    }
}


void uiGraphicsViewBody::mousePressEvent( QMouseEvent* ev )
{
    if ( ev->modifiers() == Qt::ControlModifier )
	handle_.setCtrlPressed( true );

    if ( ev->button() == Qt::RightButton )
    {
	QGraphicsView::DragMode dragmode = dragMode();
	setDragMode( QGraphicsView::NoDrag );

	buttonstate_ = OD::RightButton;
	MouseEvent me( buttonstate_, ev->x(), ev->y() );
	const int refnr = handle_.beginCmdRecEvent( "rightButtonPressed" );
	mousehandler_.triggerButtonPressed( me );
	QGraphicsView::mousePressEvent( ev );
	handle_.endCmdRecEvent( refnr, "rightButtonPressed" );
	setDragMode( dragmode );
	return;
    }
    else if ( ev->button() == Qt::LeftButton )
    {
	uiPoint viewpt = handle_.getScenePos( ev->x(), ev->y() );
	startpos_ = uiPoint( viewpt.x, viewpt.y );
	buttonstate_ = OD::LeftButton;
	MouseEvent me( buttonstate_, ev->x(), ev->y() );
	mousehandler_.triggerButtonPressed( me );
    }
    else
	buttonstate_ = OD::NoButton;

    QGraphicsView::mousePressEvent( ev );
}


void uiGraphicsViewBody::mouseDoubleClickEvent( QMouseEvent* ev )
{
    if ( !ev | handle_.isRubberBandingOn() ) return;
    if ( ev->button() == Qt::LeftButton )
    {
	MouseEvent me( OD::LeftButton, ev->x(), ev->y() );
	mousehandler_.triggerDoubleClick( me );
    }
    QGraphicsView::mouseDoubleClickEvent( ev );
}


void uiGraphicsViewBody::mouseReleaseEvent( QMouseEvent* ev )
{
    if ( !ev ) return;
    if ( ev->button() == Qt::LeftButton )
    {
	buttonstate_ = OD::LeftButton;
	MouseEvent me( buttonstate_, ev->x(), ev->y() );
	mousehandler_.triggerButtonReleased( me );
	if ( handle_.isRubberBandingOn() )
	{
	    uiPoint viewpt = handle_.getScenePos( ev->x(), ev->y() );
	    uiPoint stoppos( viewpt.x, viewpt.y );
	    uiRect selrect( startpos_, stoppos );
	    handle_.scene().setSelectionArea( selrect );
	}
    }

    handle_.setCtrlPressed( false );

    buttonstate_ = OD::NoButton;
    QGraphicsView::mouseReleaseEvent( ev );
}


void uiGraphicsViewBody::keyPressEvent( QKeyEvent* ev )
{
    if ( !ev ) return;

    // TODO: impl modifier
    KeyboardEvent ke;
    ke.key_ = (OD::KeyboardKey)ev->key();
    keyboardhandler_.triggerKeyPressed( ke );
    QGraphicsView::keyPressEvent( ev );
}


void uiGraphicsViewBody::paintEvent( QPaintEvent* ev )
{
    handle_.preDraw.trigger();
    QGraphicsView::paintEvent( ev );
}


void uiGraphicsViewBody::resizeEvent( QResizeEvent* ev )
{
    if ( !ev ) return;

    if ( handle_.scene_ )
    {
	const int sceneborder = handle_.getSceneBorder();
#if defined(__win__) && !defined(__msvc__)
	QSize newsz = ev->size();
	handle_.scene_->setSceneRect( sceneborder,sceneborder,
				      newsz.width()-2*sceneborder,
				      newsz.height()-2*sceneborder );
#else
	handle_.scene_->setSceneRect( sceneborder, sceneborder,
				      width()-2*sceneborder,
				      height()-2*sceneborder );
#endif
    }

    uiSize oldsize( ev->oldSize().width(), ev->oldSize().height() );
    handle_.reSize.trigger( oldsize );
    handle_.reDrawn.trigger();
}


void uiGraphicsViewBody::wheelEvent( QWheelEvent* ev )
{
    if ( ev && handle_.scrollZoomEnabled() )
    {
	const int numsteps = ( ev->delta() / 8 ) / 15;

	QMatrix mat = matrix();
	const QPointF& mousepos = ev->pos();
	mat.translate( (width()/2) - mousepos.x(),
		       (height()/2) - mousepos.y() );

	for ( int idx=0; idx<abs(numsteps); idx++ )
	{
	    if ( numsteps > 0 || (mat.m11()>1 && mat.m22()>1) )
	    {
		if ( numsteps > 0 )
		    mat.scale( 1.2, 1.2 );
		else
		    mat.scale( 1./1.2, 1./1.2 );
	    }
	}

	mat.translate( mousepos.x() - (width()/2),
		       mousepos.y() - (height()/2) );
	setMatrix( mat );
	ev->accept();
    }

    MouseEvent me( OD::NoButton, (int)ev->pos().x(), (int)ev->pos().y(),
			       ev->delta() );
    mousehandler_.triggerWheel( me );
    if ( handle_.scrollZoomEnabled() )
	QGraphicsView::wheelEvent( ev );
}


void uiGraphicsViewBody::scrollContentsBy( int dx, int dy )
{
    QGraphicsView::scrollContentsBy(dx,dy);
    handle_.scrollBarUsed.trigger();
}


bool uiGraphicsViewBody::event( QEvent* ev )
{
    if ( ev->type() == QEvent::Gesture )
         return gestureEvent( static_cast<QGestureEvent*>( ev ) );
    return QGraphicsView::event( ev );
}


bool uiGraphicsViewBody::gestureEvent( QGestureEvent* ev )
{
    if ( QPinchGesture* pinch =
	    static_cast<QPinchGesture*>(ev->gesture(Qt::PinchGesture)) )
    {
	const QPointF qcenter = pinch->centerPoint();
	const QPoint pinchcenter = qwidget()->mapFromGlobal(qcenter.toPoint());
	GestureEvent gevent( mCast(int,pinchcenter.x()),
			     mCast(int,pinchcenter.y()),
			     mCast(float,pinch->scaleFactor()),
			     mCast(float,pinch->rotationAngle()));

	if ( pinch->state() == Qt::GestureStarted )
	    gevent.setState( GestureEvent::Started );
	else if ( pinch->state() == Qt::GestureUpdated )
	{
	    gevent.setState( GestureEvent::Moving );
	    if ( mIsEqual(gevent.scale(),1.0f,mDefEps) )
		return false;

	    if ( mIsEqual(currentpinchscale_,gevent.scale(),mDefEps) )
		return false;

	    currentpinchscale_ = gevent.scale();
	}
	else
	    gevent.setState( GestureEvent::Finished );

	gestureeventhandler_.triggerPinchEvent( gevent );
	ev->accept();
	return true;
    }

    ev->accept();
    return false;
}


uiGraphicsViewBase::uiGraphicsViewBase( uiParent* p, const char* nm )
    : uiObject( p, nm, mkbody(p,nm) )
    , reDrawNeeded(this)
    , reSize(this)
    , reDrawn(this)
    , preDraw(this)
    , rubberBandUsed(this)
    , scrollBarUsed(this)
    , scene_(0)
    , selectedarea_(0)
    , sceneborder_(0)
    , enabscrollzoom_(false)
    , isctrlpressed_(false)
{
    setScrollBarPolicy( true, enabscrollzoom_
				? uiGraphicsViewBase::ScrollBarAsNeeded
				: uiGraphicsViewBase::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, enabscrollzoom_
				? uiGraphicsViewBase::ScrollBarAsNeeded
				: uiGraphicsViewBase::ScrollBarAlwaysOff );
    setScene( *new uiGraphicsScene(nm) );
    setDragMode( uiGraphicsViewBase::NoDrag );
    getMouseEventHandler().buttonReleased.notify(
	    mCB(this,uiGraphicsViewBase,rubberBandCB) );
}


uiGraphicsViewBody& uiGraphicsViewBase::mkbody( uiParent* p, const char* nm )
{
    body_ = new uiGraphicsViewBody( *this, p, nm );
    return *body_;
}


uiGraphicsViewBase::~uiGraphicsViewBase()
{
    delete body_;
    delete scene_;
}


MouseEventHandler& uiGraphicsViewBase::getNavigationMouseEventHandler()
{ return body_->mouseEventHandler(); }

MouseEventHandler& uiGraphicsViewBase::getMouseEventHandler()
{ return scene_->getMouseEventHandler(); }

KeyboardEventHandler& uiGraphicsViewBase::getKeyboardEventHandler()
{ return body_->keyboardEventHandler(); }

GestureEventHandler& uiGraphicsViewBase::gestureEventHandler()
{ return body_->gestureEventHandler(); }

void uiGraphicsViewBase::rePaint()
{ body_->viewport()->repaint(); }

void uiGraphicsViewBase::setDragMode( ODDragMode dragmode )
{
    body_->setDragMode( (QGraphicsView::DragMode)int(dragmode) );
    scene().setMouseEventActive( dragmode==uiGraphicsViewBase::NoDrag );
}


uiGraphicsViewBase::ODDragMode uiGraphicsViewBase::dragMode() const
{ return (ODDragMode)int(body_->dragMode()); }


bool uiGraphicsViewBase::isRubberBandingOn() const
{ return dragMode() == uiGraphicsViewBase::RubberBandDrag; }


void uiGraphicsViewBase::rubberBandCB( CallBacker* )
{
    if ( !isRubberBandingOn() )
	return;

    const MouseEvent& ev = getMouseEventHandler().event();
    if ( ev.rightButton() || ev.middleButton() )
	return;

    selectedarea_ = new uiRect( body_->getStartPos(), getCursorPos() );
    rubberBandUsed.trigger();
}


void uiGraphicsViewBase::setMouseTracking( bool yn )
{ body_->setMouseTracking( yn ); }


bool uiGraphicsViewBase::hasMouseTracking() const
{ return body_->hasMouseTracking(); }


int uiGraphicsViewBase::width() const
{
#if defined(__win__) && !defined(__msvc__)
    const int prefwidth = prefHNrPics();
    const int viewwidth = getViewArea().width();
    return prefwidth > viewwidth ? prefwidth : viewwidth;
#else
    return body_->width();
#endif
}


int uiGraphicsViewBase::height() const
{
#if defined(__win__) && !defined(__msvc__)
    const int prefheight = prefVNrPics();
    const int viewheight = getViewArea().height();
    return prefheight > viewheight ? prefheight : viewheight;
#else
    return body_->height();
#endif
}


void uiGraphicsViewBase::centreOn( uiPoint centre )
{ body_->centerOn( centre.x, centre.y ); }


void uiGraphicsViewBase::setScrollBarPolicy( bool hor, ScrollBarPolicy sbp )
{
    if ( hor )
	body_->setHorizontalScrollBarPolicy(
				      (Qt::ScrollBarPolicy)int(sbp) );
    else
	body_->setVerticalScrollBarPolicy(
				      (Qt::ScrollBarPolicy)int(sbp) );
}


void uiGraphicsViewBase::setViewArea( double x, double y, double w, double h )
{ body_->setSceneRect( x, y, w, h ); }


uiRect uiGraphicsViewBase::getViewArea() const
{
    QRectF qselrect( body_->mapToScene(0,0),
		     body_->mapToScene(width(),height()) );
    return uiRect( (int)qselrect.left(), (int)qselrect.top(),
		   (int)qselrect.right(), (int)qselrect.bottom() );
}


void uiGraphicsViewBase::setScene( uiGraphicsScene& scn )
{
    if ( scene_ ) delete scene_;
    scene_ = &scn;
    scene_->setSceneRect( sceneborder_, sceneborder_,
			  width()-2*sceneborder_, height()-2*sceneborder_ );
    scn.qGraphicsScene()->installEventFilter( new TouchSceneEventFilter );
    body_->setScene( scn.qGraphicsScene() );
}


uiGraphicsScene& uiGraphicsViewBase::scene()
{
    return *scene_;
}


uiRect uiGraphicsViewBase::getSceneRect() const
{
    QRectF scenerect = body_->sceneRect();
    return uiRect( (int)scenerect.left(), (int)scenerect.top(),
		   (int)scenerect.right(), (int)scenerect.bottom() );
}


void uiGraphicsViewBase::setSceneRect( const uiRect& rect )
{ body_->setSceneRect( rect.left(), rect.top(), rect.width(), rect.height() ); }


uiPoint uiGraphicsViewBase::getCursorPos() const
{
    QPoint globalpos( body_->cursor().pos().x(),
				body_->cursor().pos().y() );
    QPoint viewpos( (int)body_->mapFromGlobal(globalpos).x(),
			      (int)body_->mapFromGlobal(globalpos).y() );
    return getScenePos( (float)viewpos.x(), (float)viewpos.y() );
}


uiPoint uiGraphicsViewBase::getScenePos( float x, float y ) const
{
    QPoint viewpos( (int)x, (int)y );
    return uiPoint( (int)body_->mapToScene(viewpos).x(),
		    (int)body_->mapToScene(viewpos).y() );
}

const uiPoint& uiGraphicsViewBase::getStartPos() const
{ return body_->getStartPos(); }


void uiGraphicsViewBase::show()
{ body_->show(); }


void uiGraphicsViewBase::setBackgroundColor( const Color& color )
{
    QBrush brush( QColor(color.r(),color.g(),color.b()) );
    body_->setBackgroundBrush( brush );
}


Color uiGraphicsViewBase::backgroundColor() const
{
    QColor color( body_->backgroundBrush().color() );
    return Color( color.red(), color.green(), color.blue() );
}


void uiGraphicsViewBase::uisetBackgroundColor( const Color& color )
{
    body_->uisetBackgroundColor( color );
}


Color uiGraphicsViewBase::uibackgroundColor() const
{
    return body_->uibackgroundColor();
}


void uiGraphicsViewBase::setNoBackGround()
{
    body_->setAttribute( Qt::WA_NoSystemBackground );
    uisetBackgroundColor( Color( 255, 255, 255, 255 )  );
    scene_->setBackGroundColor( Color( 255, 255, 255, 255 )  );
}


void uiGraphicsViewBase::setSceneAlignment( const Alignment& al )
{
    Qt::Alignment qal;
    if ( al.vPos() == Alignment::Top )
	qal = Qt::AlignTop;
    else if ( al.vPos() == Alignment::Bottom )
	qal = Qt::AlignBottom;
    else
	qal = Qt::AlignVCenter;

    if ( al.hPos() == Alignment::Left )
	qal = qal | Qt::AlignLeft;
    else if ( al.hPos() == Alignment::Right )
	qal = qal | Qt::AlignRight;
    else
	qal = qal | Qt::AlignHCenter;

    body_->setAlignment( qal );
}


void uiGraphicsViewBase::setSceneBorder( int border )
{
    sceneborder_ = border;
}


int uiGraphicsViewBase::getSceneBorder() const
{
    return sceneborder_;
}


uiSize uiGraphicsViewBase::scrollBarSize( bool hor ) const
{
    const QScrollBar* sb = hor ? body_->horizontalScrollBar()
			       : body_->verticalScrollBar();
    return sb ? uiSize( (int)sb->sizeHint().width(),
			(int)sb->sizeHint().height())
	      : uiSize(0,0);
}


void uiGraphicsViewBase::translateText()
{
    uiObject::translateText();
    scene_->translateText();
}
