/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          07/02/2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "ui3dviewer.h"

#include "uicursor.h"
#include "ui3dviewerbody.h"
#include "ui3dindirectviewer.h"
#include "uirgbarray.h"

#include <osgQt/GraphicsWindowQt>
#include <osgViewer/View>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>

#include "envvars.h"
#include "iopar.h"
#include "keybindings.h"
#include "keystrs.h"
#include "math2.h"	
#include "uicursor.h"
#include "ptrman.h"
#include "settings.h"
#include "survinfo.h"
#include "SoAxes.h"
#include "ViewportRegion.h"

#include "uiobjbody.h"
#include "viscamera.h"
#include "viscamerainfo.h"
#include "visdatagroup.h"
#include "visdataman.h"
#include "visscene.h"
#include "vissurvscene.h"

#include <iostream>
#include <math.h>

#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>

#include <Inventor/SbColor.h>
#include <Inventor/SoSceneManager.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/system/gl.h>

#include "i_layout.h"
#include "i_layoutitem.h"

#include <QTabletEvent>
#include <QPainter>
#include "SoPolygonSelect.h"

 
#define col2f(rgb) float(col.rgb())/255

#include "uiosgviewer.h"

#include "uiobjbody.h"
#include "keystrs.h"

#include "survinfo.h"
#include "viscamera.h"
#include "vissurvscene.h"
#include "visdatagroup.h"

static const char* sKeydTectScene()	{ return "dTect.Scene."; }

DefineEnumNames(ui3DViewer,StereoType,0,"StereoType")
{ sKey::None().str(), "RedCyan", "QuadBuffer", 0 };


class uiDirectViewBody : public ui3DViewerBody
{
public:
				uiDirectViewBody(ui3DViewer&,uiParent*);

    const mQtclass(QWidget)* 	qwidget_() const;
    

    virtual uiSize		minimumSize() const 
    				{ return uiSize(200,200); }

protected:
    void			updateActModeCursor();
    osgGA::GUIActionAdapter&	getActionAdapter() { return *graphicswin_.get();}
    osg::GraphicsContext*	getGraphicsContext() {return graphicswin_.get();}

    bool			mousebutdown_;
    float			zoomfactor_;

    //visBase::CameraInfo*	camerainfo_;
    MouseCursor			actmodecursor_;

    osg::ref_ptr<osgQt::GraphicsWindowQt>	graphicswin_;
};


ui3DViewerBody::ui3DViewerBody( ui3DViewer& h, uiParent* parnt )
    : uiObjectBody( parnt, 0 )
    , handle_( h )
    , printpar_(*new IOPar)
    , viewport_( 0 )
{}


ui3DViewerBody::~ui3DViewerBody()
{			
    handle_.destroyed.trigger(handle_);
    delete &printpar_;
    view_.detachView();
    viewport_->unref();
}


uiObject& ui3DViewerBody::uiObjHandle()
{ return handle_; }


osg::Camera* ui3DViewerBody::getOsgCamera()
{
    if ( !view_.getOsgView() )
	return 0;

    return view_.getOsgView()->getCamera();
}


const osg::Camera* ui3DViewerBody::getOsgCamera() const
{
    return const_cast<ui3DViewerBody*>( this )->getOsgCamera();
}


void ui3DViewerBody::reSizeEvent(CallBacker*)
{
    const mQtclass(QWidget)* widget = qwidget();
    if ( !widget )
	return;

    if ( !camera_ )
	return;

    osg::ref_ptr<osg::Camera> osgcamera = getOsgCamera();

    if ( !osgcamera )
	return;

    const double aspectratio = static_cast<double>(widget->width())/
	static_cast<double>(widget->height());

    osgcamera->setProjectionMatrixAsPerspective( 30.0f, aspectratio,
						  1.0f, 10000.0f );
}


class ODDirectWidget : public osgQt::GLWidget, public CallBacker
{
public:
    			ODDirectWidget(QWidget* p)
			    : osgQt::GLWidget(p)
			    , resize( this )
			{}

    bool		event(QEvent* ev)
			{
			    if ( ev->type()==QEvent::Resize )
			    {
				resize.trigger();
			    }

			    return osgQt::GLWidget::event( ev );
			}

    Notifier<ODDirectWidget>	resize;
};




uiDirectViewBody::uiDirectViewBody( ui3DViewer& hndl, uiParent* parnt )
    : ui3DViewerBody( hndl, parnt )
    , mousebutdown_(false)
    , zoomfactor_( 1 )
{
    osg::ref_ptr<osg::DisplaySettings> ds = osg::DisplaySettings::instance();
    ODDirectWidget* glw = new ODDirectWidget( parnt->pbody()->managewidg() );
    glw->resize.notify( mCB( this, ui3DViewerBody, reSizeEvent ) );

    graphicswin_ = new osgQt::GraphicsWindowQt( glw );
    setStretch(2,2);
}


const mQtclass(QWidget)* uiDirectViewBody::qwidget_() const
{ return graphicswin_->getGLWidget(); }


void uiDirectViewBody::updateActModeCursor()
{
    /*
    if ( isViewing() )
	return;

    if ( !isCursorEnabled() )
	return;

    QCursor qcursor;
    uiCursorManager::fillQCursor( actmodecursor_, qcursor );

    getGLWidget()->setCursor( qcursor );
    */
}


bool ui3DViewerBody::isViewing() const
{
    return scene_ && !scene_->isTraversalEnabled( visBase::EventTraversal );
}


#define ROTATE_WIDTH 16
#define ROTATE_HEIGHT 16
#define ROTATE_BYTES ((ROTATE_WIDTH + 7) / 8) * ROTATE_HEIGHT
#define ROTATE_HOT_X 6
#define ROTATE_HOT_Y 8

static unsigned char rotate_bitmap[ROTATE_BYTES] = {
    0xf0, 0xef, 0x18, 0xb8, 0x0c, 0x90, 0xe4, 0x83,
    0x34, 0x86, 0x1c, 0x83, 0x00, 0x81, 0x00, 0xff,
    0xff, 0x00, 0x81, 0x00, 0xc1, 0x38, 0x61, 0x2c,
    0xc1, 0x27, 0x09, 0x30, 0x1d, 0x18, 0xf7, 0x0f
};

static unsigned char rotate_mask_bitmap[ROTATE_BYTES] = {
    0xf0,0xef,0xf8,0xff,0xfc,0xff,0xfc,0xff,0x3c,0xfe,0x1c,0xff,0x00,0xff,0x00,
    0xff,0xff,0x00,0xff,0x00,0xff,0x38,0x7f,0x3c,0xff,0x3f,0xff,0x3f,0xff,0x1f,
    0xf7,0x0f
};



void ui3DViewerBody::setViewing( bool yn )
{
        /*
    SoQtExaminerViewer::setViewing( yn );
    handle_.viewmodechanged.trigger(handle_);
    updateActModeCursor();
    */
}


void ui3DViewerBody::uisetViewing( bool yn )
{
    if ( scene_ )
	scene_->enableTraversal( visBase::EventTraversal, !yn );
    
    MouseCursor cursor;
    if ( yn )
    {
	cursor.shape_ = MouseCursor::Bitmap;
	
	uiRGBArray* cursorimage = new uiRGBArray( true );
	cursorimage->setSize( ROTATE_WIDTH, ROTATE_HEIGHT );
	cursorimage->putFromBitmap( rotate_bitmap, rotate_mask_bitmap );
	
	cursor.image_ = cursorimage;
	cursor.hotx_ = ROTATE_HOT_X;
	cursor.hoty_ = ROTATE_HOT_Y;
	
    }
    else
    {
	cursor.shape_ = MouseCursor::Arrow;
    }
    
    mQtclass(QCursor) qcursor;
    uiCursorManager::fillQCursor( cursor, qcursor );
    qwidget()->setCursor( qcursor );


//    SoQtExaminerViewer::setViewing( yn );
}


Coord3 ui3DViewerBody::getCameraPosition() const
{
    osg::ref_ptr<const osg::Camera> cam = getOsgCamera();
    if ( !cam )
	return Coord3::udf();

    osg::Vec3 eye, center, up;
    cam->getViewMatrixAsLookAt( eye, center, up );
    return Coord3( eye.x(), eye.y(), eye.z() );
}


void ui3DViewerBody::setSceneID( int sceneid )
{
    visBase::DataObject* obj = visBase::DM().getObject( sceneid );
    mDynamicCastGet(visBase::Scene*,newscene,obj)
    if ( !newscene ) return;

    if ( !view_.getOsgView() )
    {
	camera_ = visBase::Camera::create();

	mDynamicCastGet(osg::Camera*, osgcamera, camera_->osgNode() );
	osgcamera->setGraphicsContext( getGraphicsContext() );
	osgcamera->setClearColor( osg::Vec4(0, 0, 0, 1.0) );
	if ( viewport_ ) viewport_->unref();
	viewport_ = new osg::Viewport(0, 0, 600, 400 );
	viewport_->ref();
	osgcamera->setViewport( viewport_ );

	osg::ref_ptr<osgViewer::View> view = new osgViewer::View;
	view->setCamera( osgcamera );
	view->setSceneData( newscene->osgNode() );
	view->addEventHandler( new osgViewer::StatsHandler );

	osg::ref_ptr<osgGA::TrackballManipulator> manip =
	    new osgGA::TrackballManipulator(
		osgGA::StandardManipulator::DEFAULT_SETTINGS |
		osgGA::StandardManipulator::SET_CENTER_ON_WHEEL_FORWARD_MOVEMENT
	    );

	manip->setAutoComputeHomePosition( false );

	view->setCameraManipulator( manip.get() );
	view_.setOsgView( view );

	// To put exaggerated bounding sphere radius offside
	manip->setMinimumDistance( 0 );

	// Camera projection must be initialized before computing home position
	reSizeEvent( 0 );
    }

    scene_ = newscene;
}


void ui3DViewerBody::align()
{
    /*
    SoCamera* cam = getCamera();
    if ( !cam ) return;
    osg::Vec3f dir;
    cam->orientation.getValue().multVec( osg::Vec3f(0,0,-1), dir );

    osg::Vec3f focalpoint = cam->position.getValue() +
					cam->focalDistance.getValue() * dir;
					osg::Vec3f upvector( dir[0], dir[1], 1 );
    cam->pointAt( focalpoint, upvector );
    */
}


Geom::Size2D<int> ui3DViewerBody::getViewportSizePixels() const
{
    osg::ref_ptr<const osg::Camera> camera = getOsgCamera();
    osg::ref_ptr<const osg::Viewport> vp = camera->getViewport();

    return Geom::Size2D<int>( mNINT32(vp->width()), mNINT32(vp->height()) );
}


void ui3DViewerBody::setCameraPos( const osg::Vec3f& updir, 
				  const osg::Vec3f& focusdir,
				  bool usetruedir )
{
    /*
    osg::Camera* cam = getOsgCamera();
    if ( !cam ) return;

    osg::Vec3f dir;
    cam->orientation.getValue().multVec( osg::Vec3f(0,0,-1), dir );

#define mEps 1e-6
    const float angletofocus = Math::ACos( dir.dot(focusdir) );
    if ( mIsZero(angletofocus,mEps) ) return;

    const float diffangle = fabs(angletofocus) - M_PI_2;
    const int sign = usetruedir || mIsZero(diffangle,mEps) || diffangle < 0 ? 1 : -1;
    osg::Vec3f focalpoint = cam->position.getValue() +
			 cam->focalDistance.getValue() * sign * focusdir;
    cam->pointAt( focalpoint, updir );

    cam->position = focalpoint + cam->focalDistance.getValue() * sign *focusdir;
    viewAll();
    */
}


void ui3DViewerBody::viewPlaneX()
{
    setCameraPos( osg::Vec3f(0,0,1), osg::Vec3f(1,0,0), false );
}


void ui3DViewerBody::viewAll()
{
    if ( !view_.getOsgView() )
	return;

    osg::ref_ptr<osgGA::CameraManipulator> manip =
	static_cast<osgGA::CameraManipulator*>(
	    view_.getOsgView()->getCameraManipulator() );

    if ( !manip )
	return;

    osg::ref_ptr<osgGA::GUIEventAdapter>& ea =
	osgGA::GUIEventAdapter::getAccumulatedEventState();
    if ( !ea )
	return;

    manip->computeHomePosition( getOsgCamera(), true );
    manip->home( *ea, getActionAdapter() );
}


void ui3DViewerBody::viewPlaneY()
{
    setCameraPos( osg::Vec3f(0,0,1), osg::Vec3f(0,1,0), false );
}


void ui3DViewerBody::setBackgroundColor( const Color& col )
{
    getOsgCamera()->setClearColor( osg::Vec4(col2f(r),col2f(g),col2f(b), 1.0) ); }


Color ui3DViewerBody::getBackgroundColor() const
{
    const osg::Vec4 col = getOsgCamera()->getClearColor();
    return Color( mNINT32(col.r()*255), mNINT32(col.g()*255), mNINT32(col.b()*255) );
}


void ui3DViewerBody::viewPlaneN()
{
    setCameraPos( osg::Vec3f(0,0,1), osg::Vec3f(0,1,0), true );
}


void ui3DViewerBody::viewPlaneZ()
{
    osg::Camera* cam = getOsgCamera();
    if ( !cam ) return;

    osg::Vec3f curdir;
    //cam->orientation.getValue().multVec( osg::Vec3f(0,0,-1), curdir );
    //setCameraPos( curdir, osg::Vec3f(0,0,-1), true );
}


static void getInlCrlVec( osg::Vec3f& vec, bool inl )
{
    const RCol2Coord& b2c = SI().binID2Coord();
    const RCol2Coord::RCTransform& xtr = b2c.getTransform(true);
    const RCol2Coord::RCTransform& ytr = b2c.getTransform(false);
    const float det = xtr.det( ytr );
    if ( inl )
	vec = osg::Vec3f( ytr.c/det, -xtr.c/det, 0 );
    else
	vec = osg::Vec3f( -ytr.b/det, xtr.b/det, 0 );
    vec.normalize();
}


void ui3DViewerBody::viewPlaneInl()
{
    osg::Vec3f inlvec;
    getInlCrlVec( inlvec, true );
    setCameraPos( osg::Vec3f(0,0,1), inlvec, false );
}


void ui3DViewerBody::viewPlaneCrl()
{
    osg::Vec3f crlvec;
    getInlCrlVec( crlvec, false );
    setCameraPos( osg::Vec3f(0,0,1), crlvec, false );
}


bool ui3DViewerBody::isCameraPerspective() const
{
    return !camera_->isOrthogonal();
}


bool ui3DViewerBody::isCameraOrthographic() const
{
    return camera_->isOrthogonal();
}


void ui3DViewerBody::toggleCameraType()
{
    //TODO
}


void ui3DViewerBody::setHomePos()
{
    if ( !camera_ ) return;

    PtrMan<IOPar> homepospar =
	SI().pars().subselect( ui3DViewer::sKeyHomePos() );
    if ( !homepospar )
	return;

    camera_->usePar( *homepospar );
}



void ui3DViewerBody::resetToHomePosition()
{
}

// Start of SO stuff

class i_uiSoviewLayoutItem : public i_uiLayoutItem
{
public:
			i_uiSoviewLayoutItem(i_LayoutMngr&,uiSoViewerBody&);

protected:

    virtual void 	commitGeometrySet(bool isprefsz);
    uiSoViewerBody&	vwr;
};


class SoTabletEventFilter : public QObject
{
public:
    			SoTabletEventFilter()				{};
protected:
    bool		eventFilter(QObject*,QEvent*);
};


class uiSoViewerBody : public uiObjectBody, public SoQtExaminerViewer
{
public:
				uiSoViewerBody(ui3DViewer&,uiParent*,
					       const char*);
				~uiSoViewerBody();

    virtual const QWidget* 	qwidget_() const	{ return qwidg_; }

    void			setSceneID(int);
    visBase::Scene*		getScene()		{ return scene_; }
    const visBase::Scene*	getScene() const 	{ return scene_; }

    visBase::Camera*		getVisCamera() { return camera_; }

    void			anyWheelStart();
    void			anyWheelStop();

    void			align();
    void			viewPlaneX();
    void			viewPlaneY();
    void			viewPlaneZ();
    void			viewPlaneInl();
    void			viewPlaneCrl();
    void			viewPlaneN();

    void			uiRotateHor(float angle);
    void			uiRotateVer(float angle);
    void			uiZoom(float rel,const SbVec3f*);
    void			setCameraZoom(float);
    float			getCameraZoom();

    bool			isCameraPerspective() const;
    bool			isCameraOrthographic() const;

    void			showAxis( bool yn )
    				{ showaxes_ = yn; scheduleRedraw(); }
    bool			axisShown() const	{ return showaxes_; }
    void			setAxisAnnotColor(const Color&);
    void			mkAxes();

    void			setHomePos();
    void			saveHomePos();
    virtual void		setViewing(SbBool);
    void			uisetViewing(bool);
    void			enableSeek(bool);
    bool			seekEnabled() const;
    virtual SbBool		processSoEvent(const SoEvent* const event);

    KeyBindMan&			keyBindMan()		{ return keybindman_; }

    virtual uiSize		minimumSize() const 
    				{ return uiSize(200,200); }

    SoNode*			getTotalSceneGraph();

protected:
    void			updateActModeCursor();

    virtual i_LayoutItem* 	mkLayoutItem_(i_LayoutMngr&);
    ui3DViewer&     		handle()		{ return handle_; }
    virtual uiObject&		uiObjHandle()		{ return handle_; }
    void			actualRedraw();

    void			setCameraPos(const SbVec3f& rotdir,
	    				     const SbVec3f&, bool usetruedir);
    SbBool			processMouseEvent(const SoMouseButtonEvent*);

    SoTabletEventFilter		tableteventfilter_;

    ui3DViewer&			handle_;
    QWidget*			qwidg_;

    IOPar&			printpar_;
    KeyBindMan&			keybindman_;

    SoOrthographicCamera*	axescamera_;
    SoAxes*			axes_;
    SoDirectionalLight*		axeslight_;
    SoSeparator*		owngraph_;
    bool			showaxes_;
    
    bool			mousebutdown_;
    float			zoomfactor_;

    visBase::CameraInfo*	camerainfo_;
    visBase::Scene*		scene_;
    visBase::Camera*		camera_;
    visBase::DataObjectGroup*	displayroot_;
    MouseCursor			actmodecursor_;

    static void			errorCB(const SoError*,void*);
};



uiSoViewerBody::uiSoViewerBody( ui3DViewer& hndl, uiParent* parnt, 
				const char* nm)
    : uiObjectBody(parnt,nm)
    , SoQtExaminerViewer(parnt && parnt->pbody() ?
			 parnt->pbody()->managewidg() : 0, nm, true)
    , handle_(hndl)
    , scene_(0)
    , camera_(0)
    , camerainfo_(0)
    , displayroot_(0)
    , axes_(0)
    , showaxes_(false)
    , printpar_(*new IOPar)
    , keybindman_(*new KeyBindMan)
    , mousebutdown_(false)
    , zoomfactor_( 1 )
    , axescamera_( 0 )
    , axeslight_( 0 )
    , owngraph_( 0 )
{
    setClearBeforeRender(FALSE, TRUE);
    qwidg_ = getWidget();
    setStretch(2,2);
    //Dont use use any sorted triangle stuff, as they don't work
    //well with remote display (bug 293).
    int trnsptyp = GetEnvVarIVal( "DTECT_COIN_TRANSPARENCY_TYPE",
	    	(int)SoGLRenderAction::SORTED_OBJECT_BLEND );
    setTransparencyType( (SoGLRenderAction::TransparencyType)trnsptyp );

    float hli = GetEnvVarDVal( "DTECT_COIN_HEADLIGHT_INTENSITY", 1 );
    setHeadlight( !mIsZero(hli,mDefEps) );
    getHeadlight()->intensity.setValue( hli );

    setPopupMenuEnabled( GetEnvVarYN("DTECT_COIN_ENABLE_POPUPMENU") );

    SoDebugError::setHandlerCallback( uiSoViewerBody::errorCB, this );
     
    mkAxes();

#ifndef use_SoQtRenderArea
    setDecoration(FALSE);
#endif

    qwidg_->installEventFilter( &tableteventfilter_ );
}


uiSoViewerBody::~uiSoViewerBody() 
{ 
    qwidg_->removeEventFilter( &tableteventfilter_ );
    handle_.destroyed.trigger(handle_);
    if ( displayroot_ ) displayroot_->unRef();
    delete &keybindman_;
    delete &printpar_;

    owngraph_->unref();
}


SbBool uiSoViewerBody::processMouseEvent( const SoMouseButtonEvent* event )
{
    if ( !event || event->getState()!=SoButtonEvent::DOWN )
	return false;

    if ( event->getButton()!=SoMouseButtonEvent::BUTTON4 &&
	 event->getButton()!=SoMouseButtonEvent::BUTTON5 )
	return false;

    mSettUse( get, "dTect", "Zoom factor", zoomfactor_ );
    if ( zoomfactor_<=0 )
	return false;

    SoCamera* cam = getCamera();
    const SbViewVolume vv = cam->getViewVolume();

    const SbVec2f normmousepos =
		  event->getNormalizedPosition(getViewportRegion());
    SbVec3f startpos, stoppos;
    vv.projectPointToLine( normmousepos, startpos, stoppos );
    SbVec3f direction = stoppos-startpos;
    direction.normalize();

    if ( event->getButton() == SoMouseButtonEvent::BUTTON4 )
	uiZoom( 0.1*zoomfactor_, &direction );
    else if ( event->getButton() == SoMouseButtonEvent::BUTTON5 )
	uiZoom( -0.1*zoomfactor_, &direction );

    return true;
}


SbBool uiSoViewerBody::processSoEvent( const SoEvent* const event )
{
    SoPolygonSelect::setActiveSceneManager( getSceneManager() );

    const SoType type( event->getTypeId() );
    const SoKeyboardEvent* keyevent = 0;
    if ( type.isDerivedFrom( SoKeyboardEvent::getClassTypeId() ) )
	keyevent = (SoKeyboardEvent*)event;

    SoMouseButtonEvent* mousebuttonevent = 0;
    if ( type.isDerivedFrom( SoMouseButtonEvent::getClassTypeId() ) )
	mousebuttonevent = (SoMouseButtonEvent*)event;

    if ( mousebuttonevent )
    {
	mousebutdown_ = mousebuttonevent->getState() == SoButtonEvent::DOWN;
	if ( processMouseEvent(mousebuttonevent) )
	    return true;
    }

    const bool stillviewing = isViewing() && !isAnimating();
    const bool hasbutevent = keyevent || mousebuttonevent || mousebutdown_;
    if ( stillviewing && !hasbutevent )
	SoQtRenderArea::processSoEvent( event );

    if ( keyevent && keyevent->getKey() == SoKeyboardEvent::Q )
	return true;

    if ( keyevent && ( keyevent->getKey() == SoKeyboardEvent::PAGE_UP ||
		       keyevent->getKey() == SoKeyboardEvent::PAGE_DOWN ) )
    {
	if ( keyevent->getState() == SoButtonEvent::DOWN ) return true;
	
	const bool isup = keyevent->getKey() == SoKeyboardEvent::PAGE_UP;
	handle_.pageupdown.trigger(isup);
	return true;
    }

    const SoEvent* keyev = 
	keybindman_.processSoEvent( event, isViewing(), isPopupMenuEnabled() );
    if ( !keyev )
	return SoQtExaminerViewer::processSoEvent( event );

    const SbBool res = SoQtExaminerViewer::processSoEvent( keyev );
    if ( !getInteractiveCount() )
	camerainfo_->setInteractive(false); //Will trigger redraw

    mDynamicCastGet(const visSurvey::Scene*,survscene,scene_);
    if ( survscene ) 
    {
	MouseCursor newcursor;
	const MouseCursor* mousecursor = survscene->getMouseCursor();
	if ( mousecursor && mousecursor->shape_!=MouseCursor::NotSet )
	    newcursor = *mousecursor;
	else
	    newcursor.shape_ = MouseCursor::Arrow;

	if ( newcursor!=actmodecursor_ )
	{
	    actmodecursor_ = newcursor;
	    updateActModeCursor();
	}
    }

    return res;
}


void uiSoViewerBody::updateActModeCursor()
{
    if ( isViewing() )
	return;

    if ( !isCursorEnabled() )
	return;

    QCursor qcursor;
    uiCursorManager::fillQCursor( actmodecursor_, qcursor );

    getGLWidget()->setCursor( qcursor );
}


void uiSoViewerBody::setViewing( SbBool yn )
{
    if ( seekEnabled() && !yn )
	enableSeek( false );

    SoQtExaminerViewer::setViewing( yn );
    handle_.viewmodechanged.trigger(handle_);
    updateActModeCursor();
}


void uiSoViewerBody::uisetViewing( bool yn )
{ SoQtExaminerViewer::setViewing( yn ); }

void uiSoViewerBody::enableSeek( bool yn )
{ SoQtExaminerViewer::setSeekMode( yn ); }

bool uiSoViewerBody::seekEnabled() const
{ return SoQtExaminerViewer::isSeekMode(); }


void uiSoViewerBody::setAxisAnnotColor( const Color& col )
{
    axes_->textcolor_.setValue( col2f(r), col2f(g), col2f(b) );
    scheduleRedraw();
}


void uiSoViewerBody::setSceneID( int sceneid )
{
    visBase::DataObject* obj = visBase::DM().getObject( sceneid );
    mDynamicCastGet(visBase::Scene*,newscene,obj)
    if ( !newscene ) return;

    if ( !displayroot_ )
    {
	displayroot_ = visBase::DataObjectGroup::create();
	displayroot_->ref();

	camerainfo_ = visBase::CameraInfo::create();
	displayroot_->addObject( camerainfo_ );

	camera_ = visBase::Camera::create();
	displayroot_->addObject( camera_ );

	mDynamicCastGet(const visSurvey::Scene*,survscene,newscene);
	if ( survscene ) 
	{
	    Coord minpos = SI().minCoord( true );
	    float middlez = (SI().zRange(true).start + SI().zRange(true).stop)
			  * 0.5 * survscene->getZStretch();
	    Coord3 camerapos( minpos.x, minpos.y - 1000, -middlez );
	    Coord3 pointat( minpos.x, minpos.y, -middlez );
	    camera_->setPosition( camerapos );
	    camera_->pointAt( pointat, Coord3( 0, 1, 1) );
	    camera_->setFarDistance( 1e30 );
	}
    }

    int sceneidx = displayroot_->getFirstIdx( scene_ );
    if ( sceneidx >= 0 )
	displayroot_->removeObject( sceneidx );

    displayroot_->addObject( newscene );
    scene_ = newscene;

    SoNode* root = displayroot_->getInventorNode();
    if ( !root ) { pErrMsg("huh"); return; }

    setSceneGraph( root );
}


i_LayoutItem* uiSoViewerBody::mkLayoutItem_( i_LayoutMngr& mngr )
{ 
    return new i_uiSoviewLayoutItem( mngr, *this );
}


void uiSoViewerBody::align()
{
    SoCamera* cam = getCamera();
    if ( !cam ) return;
    SbVec3f dir;
    cam->orientation.getValue().multVec( SbVec3f(0,0,-1), dir );

    SbVec3f focalpoint = cam->position.getValue() +
					cam->focalDistance.getValue() * dir;
    SbVec3f upvector( dir[0], dir[1], 1 );
    cam->pointAt( focalpoint, upvector );
}


void uiSoViewerBody::setCameraPos( const SbVec3f& updir, 
				   const SbVec3f& focusdir,
				   bool usetruedir )
{
    SoCamera* cam = getCamera();
    if ( !cam ) return;

    SbVec3f dir;
    cam->orientation.getValue().multVec( SbVec3f(0,0,-1), dir );

#define mEps 1e-6
    const float angletofocus = Math::ACos( dir.dot(focusdir) );
    if ( mIsZero(angletofocus,mEps) ) return;

    const float diffangle = fabs(angletofocus) - M_PI_2;
    const int sign = usetruedir || mIsZero(diffangle,mEps) || diffangle < 0 ? 1 : -1;
    SbVec3f focalpoint = cam->position.getValue() +
			 cam->focalDistance.getValue() * sign * focusdir;
    cam->pointAt( focalpoint, updir );

    cam->position = focalpoint + cam->focalDistance.getValue() * sign *focusdir;
    viewAll();
}


void uiSoViewerBody::viewPlaneX()
{
    setCameraPos( SbVec3f(0,0,1), SbVec3f(1,0,0), false );
}


void uiSoViewerBody::viewPlaneY()
{
    setCameraPos( SbVec3f(0,0,1), SbVec3f(0,1,0), false );
}


void uiSoViewerBody::viewPlaneN()
{
    setCameraPos( SbVec3f(0,0,1), SbVec3f(0,1,0), true );
}


void uiSoViewerBody::viewPlaneZ()
{
    SoCamera* cam = getCamera();
    if ( !cam ) return;

    SbVec3f curdir;
    cam->orientation.getValue().multVec( SbVec3f(0,0,-1), curdir );
    setCameraPos( curdir, SbVec3f(0,0,-1), true );
}


static void getInlCrlVec( SbVec3f& vec, bool inl )
{
    const RCol2Coord& b2c = SI().binID2Coord();
    const RCol2Coord::RCTransform& xtr = b2c.getTransform(true);
    const RCol2Coord::RCTransform& ytr = b2c.getTransform(false);
    const float det = xtr.det( ytr );
    if ( inl )
	vec = SbVec3f( ytr.c/det, -xtr.c/det, 0 );
    else
	vec = SbVec3f( -ytr.b/det, xtr.b/det, 0 );
    vec.normalize();
}


void uiSoViewerBody::viewPlaneInl()
{
    SbVec3f inlvec;
    getInlCrlVec( inlvec, true );
    setCameraPos( SbVec3f(0,0,1), inlvec, false );
}


void uiSoViewerBody::viewPlaneCrl()
{
    SbVec3f crlvec;
    getInlCrlVec( crlvec, false );
    setCameraPos( SbVec3f(0,0,1), crlvec, false );
}


void uiSoViewerBody::anyWheelStart()
{
    uisetFocus();
    interactiveCountInc();
}


void uiSoViewerBody::anyWheelStop()
{
    interactiveCountDec();
    if ( !getInteractiveCount() )
	camerainfo_->setInteractive(false); //Will trigger redraw
}


void uiSoViewerBody::uiRotateHor( float angle )
{ 
    // SoQtExaminerViewer::bottomWheelMotion
    if ( isAnimating() ) stopAnimating();

    // compute new camera position. 
    // See SoQtExaminerViewer::rotYWheelMotion
    SoCamera* cam = getCamera();
    if ( !cam ) return; // can happen for empty scenegraph

    SbVec3f dir;
    cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), dir);

    SbVec3f focalpoint = cam->position.getValue() +
	cam->focalDistance.getValue() * dir;

    cam->orientation = SbRotation(SbVec3f(0, 1, 0), angle ) *
	cam->orientation.getValue();

    cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), dir);
    cam->position = focalpoint - cam->focalDistance.getValue() * dir;
}


void uiSoViewerBody::uiRotateVer( float angle )
{ 
    // SoQtExaminerViewer::bottomWheelMotion
    if (  isAnimating() )	stopAnimating();

    // compute new camera position. 
    // See SoQtExaminerViewer::rotXWheelMotion
    SoCamera* cam = getCamera();
    if ( !cam ) return; // can happen for empty scenegraph

    SbVec3f dir;
    cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), dir);

    SbVec3f focalpoint = cam->position.getValue() +
	cam->focalDistance.getValue() * dir;

    cam->orientation = SbRotation(SbVec3f(-1, 0, 0), angle) *
	cam->orientation.getValue();

    cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), dir);
    cam->position = focalpoint - cam->focalDistance.getValue() * dir;
}


void uiSoViewerBody::uiZoom( float rel, const SbVec3f* extdir )
{ 

    SoCamera* cam = getCamera();
    if ( !cam) return; // can happen for empty scenegraph

    SoType t = cam->getTypeId();

    // This will be in the range of <0, ->>.
    float multiplicator = exp(rel);

    if (t.isDerivedFrom(SoOrthographicCamera::getClassTypeId())) 
    {
	SoOrthographicCamera * oc = (SoOrthographicCamera *)cam;
	oc->height = oc->height.getValue() * multiplicator;
    } 
    else if (t.isDerivedFrom(SoPerspectiveCamera::getClassTypeId())) 

//NOTE: visBase::Camera has a SoPerspectiveCamera* 
    {
	const float oldfocaldist = cam->focalDistance.getValue();
	const float newfocaldist = oldfocaldist * multiplicator;

	float movement = newfocaldist - oldfocaldist;
	const float minmovement = cam->getViewVolume().getDepth()*0.01;

	if ( fabs(movement)<minmovement )
	{
	    if ( movement<0 ) //zoom in
		return;
	    movement = minmovement;
	}

	SbVec3f olddir;
	cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), olddir);
	const SbVec3f& newdir = extdir ? *extdir : olddir;

	cam->position = cam->position.getValue() - movement * newdir;
	cam->focalDistance = oldfocaldist + movement * olddir.dot( newdir );
    } 
    else 
    {
	pErrMsg("huh!"); //assert(0 && "impossible");
    }
}


float uiSoViewerBody::getCameraZoom()
{
//  Only implemented for SoPerspectiveCamera
    SoCamera* cam = getCamera();
    if ( !cam ) return 0;
    return ((SoPerspectiveCamera *)cam)->
	heightAngle.getValue() / 2 * 360 / M_PI;
}


bool uiSoViewerBody::isCameraPerspective() const
{
    const SoType t = getCameraType();
    return t.isDerivedFrom( SoPerspectiveCamera::getClassTypeId() );
}


bool uiSoViewerBody::isCameraOrthographic() const
{
    const SoType t = getCameraType();
    return t.isDerivedFrom( SoOrthographicCamera::getClassTypeId() ); 
}


void uiSoViewerBody::setCameraZoom( float val )
{
//  Only implemented for SoPerspectiveCamera
    SoCamera* cam = getCamera();
    if ( !cam ) return;
    ((SoPerspectiveCamera *)cam)->heightAngle = val * 2 * M_PI / 360;
}


void uiSoViewerBody::errorCB( const SoError* error, void* data )
{
     if ( GetEnvVarYN("DTECT_COIN_SHOW_ERRORS") || 
	  GetEnvVarYN("COIN_DEBUG_DL") )
     {
	 const SbString& str = error->getDebugString();
	     programmerErrMsg( str.getString(), "uiSoViewerBody::errorCB" ,__FILE__,__LINE__);
     }
}


void uiSoViewerBody::actualRedraw()
{
    const SbColor col = getBackgroundColor();
    glClearColor( col[0], col[1], col[2], 0.0f );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    bool ismoving = false;
    bool isinteractive = false;
    
    if ( isAnimating() )
	ismoving = true;
    else if ( getInteractiveCount()>0 )
	isinteractive = true;

    camerainfo_->setInteractive( isinteractive );
    camerainfo_->setMoving( ismoving );

    SoQtExaminerViewer::actualRedraw();

    if ( showaxes_ )
    {
	axescamera_->orientation.setValue(
		getCamera()->orientation.getValue() );

	axeslight_->direction.setValue( getHeadlight()->direction.getValue() );

	SoGLRenderAction* glra = getGLRenderAction();
	if ( glra )
	{
	    glClear(GL_DEPTH_BUFFER_BIT);
	    glra->apply( owngraph_ ); 
	}
    }
}


void uiSoViewerBody::mkAxes()
{

    owngraph_ = new SoSeparator;
    owngraph_->ref();

    axeslight_ = new SoDirectionalLight;
    owngraph_->addChild( axeslight_ );
    axeslight_->intensity.setValue( 1.0 );
	
    axescamera_ = new SoOrthographicCamera;
    owngraph_->addChild( axescamera_ );
    axescamera_->position = SbVec3f(0,0,0);
    axescamera_->height = 10;
    axescamera_->nearDistance = -10;
    axescamera_->farDistance = 10;
    
    ViewportRegion* region = new ViewportRegion;
    region->origin.setValue( SbVec2f(1,0) );
    region->size.setValue( SbVec2f(0.15,0.15) );
    owngraph_->addChild( region );

    axes_ = new SoAxes;
    owngraph_->addChild( axes_ );
}


SoNode* uiSoViewerBody::getTotalSceneGraph() 
{
    if ( showaxes_ )
    {
	SoSeparator* sep = new SoSeparator;
	sep->addChild( getSceneManager()->getSceneGraph() );
	sep->addChild( owngraph_ );
	return sep;
    }

    return getSceneManager()->getSceneGraph();
}


//------------------------------------------------------------------------------


i_uiSoviewLayoutItem::i_uiSoviewLayoutItem( i_LayoutMngr& mgr,
                                            uiSoViewerBody& obj)
    : i_uiLayoutItem(mgr,obj)
    , vwr(obj)
{
}


void i_uiSoviewLayoutItem::commitGeometrySet( bool isprefsz )
{
    const uiRect& geom = curpos( setGeom );
    vwr.setSize( SbVec2s(geom.hNrPics(),geom.vNrPics()) );
    i_uiLayoutItem::commitGeometrySet( isprefsz );
}


//------------------------------------------------------------------------------


bool SoTabletEventFilter::eventFilter( QObject* obj, QEvent* ev )
{
    // Temporarily disabled pressure sensitive polygon selection because of
    // its non-smooth behaviour (on Windows)
/*
    QTabletEvent* qte = dynamic_cast<QTabletEvent*>( ev );
    if ( qte )
	SoPolygonSelect::setTabletPressure( qte->pressure() );
*/

    return false;			// Qt will resent it as a QMouseEvent
}


//------------------------------------------------------------------------------


ui3DViewer::ui3DViewer( uiParent* parnt, bool direct, const char* nm )
    : uiObject(parnt,nm,mkBody(parnt, direct, nm) )
    , destroyed(this)
    , viewmodechanged(this)
    , pageupdown(this)
    , vmcb(0)
{
    PtrMan<IOPar> homepospar = SI().pars().subselect( sKeyHomePos() );
    if ( homepospar )
	homepos_ = *homepospar;

    setViewing( false );  // switches between view & interact mode

#define mGetProp(get,str,tp,var,func) \
    tp var; \
    res = Settings::common().get(BufferString(sKeydTectScene(),str),var);\
    if ( res ) func( var );

    bool res = false;
    mGetProp( get, sKeyBGColor(), Color, col, setBackgroundColor );
    mGetProp( getYN, sKeyAnimate(), bool, yn, enableAnimation );
}


ui3DViewer::~ui3DViewer()
{
    delete sobody_;
    delete osgbody_;
}


uiObjectBody& ui3DViewer::mkBody( uiParent* parnt, bool direct, const char* nm )
{
    if ( visBase::DataObject::doOsg() )
    {
	osgQt::initQtWindowingSystem();

	osgbody_ = direct 
	    ? (ui3DViewerBody*) new uiDirectViewBody( *this, parnt )
	    : (ui3DViewerBody*) new ui3DIndirectViewBody( *this, parnt );
	sobody_ = 0;

	return *osgbody_; 
    }

    osgbody_ = 0;
    sobody_ = new uiSoViewerBody( *this, parnt, nm );
    return *sobody_; 
}

void ui3DViewer::viewAll()
{
    mDynamicCastGet(visSurvey::Scene*,survscene,getScene());
    if ( !survscene )
    {
	if ( sobody_ ) sobody_->viewAll();
	else osgbody_->viewAll();
    }
    else
    {
	bool showtext = survscene->isAnnotTextShown();
	bool showscale = survscene->isAnnotScaleShown();
	if ( !showtext && !showscale )
	{
	    if ( sobody_ ) sobody_->viewAll();
	    else osgbody_->viewAll();
	}
	else
	{
	    survscene->showAnnotText( false );
	    survscene->showAnnotScale( false );
	    if ( sobody_ ) sobody_->viewAll();
	    else osgbody_->viewAll();
	    survscene->showAnnotText( showtext );
	    survscene->showAnnotScale( showscale );
	}
    }
}


void ui3DViewer::enableAnimation( bool yn )
{ if ( sobody_ ) sobody_->setAnimationEnabled( yn ); }

bool ui3DViewer::isAnimationEnabled() const
{ return sobody_ ? sobody_->isAnimationEnabled() : false; }


void ui3DViewer::setBackgroundColor( const Color& col )
{
    if ( sobody_ )
	sobody_->setBackgroundColor( SbColor(col2f(r),col2f(g),col2f(b)) );
    else
	osgbody_->setBackgroundColor( col );
}


void ui3DViewer::setAxisAnnotColor( const Color& col )
{
    if ( sobody_ ) sobody_->setAxisAnnotColor( col );
}


Color ui3DViewer::getBackgroundColor() const
{
    if ( sobody_ )
    {
	float r, g, b;
	sobody_->getBackgroundColor().getValue( r, g, b );
	return Color( mNINT32(r*255), mNINT32(g*255), mNINT32(b*255) );
    }

    return osgbody_->getBackgroundColor();
}


void ui3DViewer::getAllKeyBindings( BufferStringSet& keys )
{
    if ( sobody_ )
	sobody_->keyBindMan().getAllKeyBindings( keys );
}


void ui3DViewer::setKeyBindings( const char* keybindname )
{
    if ( sobody_ )
	sobody_->keyBindMan().setKeyBindings( keybindname, true );
}


const char* ui3DViewer::getCurrentKeyBindings() const
{
    if ( sobody_ )
	return sobody_->keyBindMan().getCurrentKeyBindings();

    return 0;
}


void ui3DViewer::viewPlane( PlaneType type )
{
    if ( sobody_ )
    {
	switch ( type )
	{
	    case X: sobody_->viewPlaneX(); break;
	    case Y: sobody_->viewPlaneN(); break;
	    case Z: sobody_->viewPlaneZ(); break;
	    case Inl: sobody_->viewPlaneInl(); break;
	    case Crl: sobody_->viewPlaneCrl(); break;
	    case YZ: sobody_->viewPlaneN(); sobody_->viewPlaneZ(); break;
	}
    }
    else
    {
	switch ( type )
	{
	    case X: osgbody_->viewPlaneX(); break;
	    case Y: osgbody_->viewPlaneN(); break;
	    case Z: osgbody_->viewPlaneZ(); break;
	    case Inl: osgbody_->viewPlaneInl(); break;
	    case Crl: osgbody_->viewPlaneCrl(); break;
	    case YZ: osgbody_->viewPlaneN(); osgbody_->viewPlaneZ(); break;
	}
    }

    viewAll();
}


bool ui3DViewer::isCameraPerspective() const
{
    if ( sobody_ )
	return sobody_->isCameraPerspective();
    
    return osgbody_->isCameraPerspective();
}


bool ui3DViewer::setStereoType( StereoType type )
{
    if ( sobody_ )
    {
	SoQtViewer::StereoType soqttype;
	if ( type == RedCyan )
	    soqttype = SoQtViewer::STEREO_ANAGLYPH;
	else if ( type == QuadBuffer )
	    soqttype = SoQtViewer::STEREO_QUADBUFFER;
	else
	    soqttype = SoQtViewer::STEREO_NONE;
	    
	return sobody_->setStereoType( soqttype );
    }
   
    pErrMsg( "Not impl yet" ); 
    return false;
}


ui3DViewer::StereoType ui3DViewer::getStereoType() const
{
    if ( sobody_ )
    {
	SoQtViewer::StereoType soqttype = sobody_->getStereoType();
	if ( soqttype == SoQtViewer::STEREO_ANAGLYPH )
	    return RedCyan;
	else if ( soqttype == SoQtViewer::STEREO_QUADBUFFER )
	    return QuadBuffer;
	else
	    return None;
    }

    return None;
} 


void ui3DViewer::setStereoOffset( float offset )
{
    if ( sobody_ ) sobody_->setStereoOffset( offset );
    else
    {
	pErrMsg( "Not impl yet" ); 
    }
}


float ui3DViewer::getStereoOffset() const
{
    if ( sobody_ )
	return sobody_->getStereoOffset();

    pErrMsg( "Not impl yet" ); 
    return 0;
}


void ui3DViewer::setSceneID( int sceneid )
{
    if ( sobody_ )
	sobody_->setSceneID( sceneid );
    else
	osgbody_->setSceneID( sceneid );
}


int ui3DViewer::sceneID() const
{
    if ( sobody_ )
	return sobody_->getScene() ? sobody_->getScene()->id() : -1;

    return osgbody_->getScene() ? osgbody_->getScene()->id() : -1;
}


void ui3DViewer::setViewing( bool yn )
{
    if ( sobody_ )
	sobody_->uisetViewing(yn);
    else
	osgbody_->uisetViewing( yn );
}


bool ui3DViewer::isViewing() const
{
    if ( sobody_ )
	return sobody_->isViewing();

    return osgbody_->isViewing();
}


void ui3DViewer::rotateH( float angle )
    { sobody_->uiRotateHor(angle); }

void ui3DViewer::rotateV( float angle )
    { sobody_->uiRotateVer(angle); }

void ui3DViewer::dolly( float rel )
    { sobody_->uiZoom(rel, 0); }


float ui3DViewer::getCameraZoom()
{
    if ( sobody_ )
	return sobody_->getCameraZoom();

    return 1;
}


const Coord3 ui3DViewer::getCameraPosition() const
{
    if ( sobody_ )
    {
	SoCamera* cam = sobody_->getCamera();
	if ( !cam )
	    return Coord3::udf();

	float x, y, z;
	cam->position.getValue().getValue( x, y, z );
	return Coord3( x, y, z );
    }

    return osgbody_->getCameraPosition();
}


void ui3DViewer::setCameraZoom( float val )
{
    if ( sobody_ )
	sobody_->setCameraZoom( val );
    else
    {
	//TODO
    }
}


void ui3DViewer::anyWheelStart()
    { sobody_->anyWheelStart(); }

void ui3DViewer::anyWheelStop()
    { sobody_->anyWheelStop(); }


void ui3DViewer::align()
{
    if ( sobody_ )
	sobody_->align();
    else
	osgbody_->align();
}


void ui3DViewer::toHomePos()
{
    RefMan<visBase::Camera> camera = sobody_
	? sobody_->getVisCamera()
	: osgbody_->getVisCamera();

    if ( !camera ) return;

    camera->usePar( homepos_ );
}


void ui3DViewer::saveHomePos()
{
    RefMan<visBase::Camera> camera = sobody_
	? sobody_->getVisCamera()
	: osgbody_->getVisCamera();

    if ( !camera ) return;

    TypeSet<int> dummy;
    camera->fillPar( homepos_, dummy );
    homepos_.removeWithKey( sKey::Type() );
    SI().getPars().mergeComp( homepos_, sKeyHomePos() );
    SI().savePars();
}


void ui3DViewer::showRotAxis( bool yn )
{
    if ( sobody_ )
	sobody_->showAxis( yn );
    else
    {
	pErrMsg("Not impl");
    }
}


bool ui3DViewer::rotAxisShown() const
{
    if ( sobody_ )
	return sobody_->axisShown();

    pErrMsg("Not impl");
    return false;
}


void ui3DViewer::toggleCameraType()
{
    if ( sobody_ )
	sobody_->toggleCameraType();
    else
	osgbody_->toggleCameraType();
}


void ui3DViewer::switchSeekMode()
{
    if ( sobody_ )
    {
	if ( !isViewing() ) sobody_->setViewing( true );
	sobody_->enableSeek( !sobody_->seekEnabled() );
    }
    else if ( osgbody_ )
	pErrMsg("Not impl");
}


Geom::Size2D<int> ui3DViewer::getViewportSizePixels() const
{
    if ( sobody_ )
    {
	SbVec2s size = sobody_->getViewportRegion().getViewportSizePixels();
	return Geom::Size2D<int>( size[0], size[1] );
    }

    return osgbody_->getViewportSizePixels();
}


SoNode* ui3DViewer::getSceneGraph() const
{
    return sobody_->getTotalSceneGraph();
}


void ui3DViewer::savePropertySettings() const
{
#define mSaveProp(set,str,func) \
    Settings::common().set( BufferString(sKeydTectScene(),str), func );

    mSaveProp( set, sKeyBGColor(), getBackgroundColor() );
    mSaveProp( setYN, sKeyAnimate(), isAnimationEnabled() );
    Settings::common().write();
}


void ui3DViewer::fillPar( IOPar& par ) const
{
    par.set( sKeySceneID(), getScene()->id() );

    par.set( sKeyBGColor(), (int)getBackgroundColor().rgb() );

    par.set( sKeyStereo(), toString( getStereoType() ) );
    float offset = getStereoOffset();
    par.set( sKeyStereoOff(), offset );

    par.setYN( sKeyPersCamera(), isCameraPerspective() );

    TypeSet<int> dummy;
    RefMan<visBase::Camera> camera = sobody_
	? sobody_->getVisCamera()
	: osgbody_->getVisCamera();

    camera->fillPar( par, dummy );

    par.mergeComp( homepos_, sKeyHomePos() );
}


bool ui3DViewer::usePar( const IOPar& par )
{
    int sceneid;
    if ( !par.get(sKeySceneID(),sceneid) ) return false;

    setSceneID( sceneid );
    if ( !getScene() ) return false;

    int bgcol;
    if ( par.get(sKeyBGColor(),bgcol) )
    {
	Color newcol; newcol.setRgb( bgcol );
	setBackgroundColor( newcol );
    }

    StereoType stereotype;
    if ( parseEnum( par, sKeyStereo(), stereotype ) )
	setStereoType( stereotype );

    float offset;
    if ( par.get( sKeyStereoOff(), offset )  )
	setStereoOffset( offset );

    PtrMan<IOPar> homepos = par.subselect( sKeyHomePos() );

    if ( homepos && homepos_.isEmpty() )
	homepos_ = *homepos;

    RefMan<visBase::Camera> camera = sobody_
	? sobody_->getVisCamera()
	: osgbody_->getVisCamera();

    bool res = camera->usePar( par ) == 1;
    
    PtrMan<IOPar> survhomepospar = SI().pars().subselect( sKeyHomePos() );
    if ( !survhomepospar )
	saveHomePos();

    return res;
}



float ui3DViewer::getHeadOnLightIntensity() const
{
    return ( sobody_ ) ? sobody_->getHeadlight()->intensity.getValue() : -1;
}

void ui3DViewer::setHeadOnLightIntensity( float value )
{
    if ( sobody_ )
	sobody_->getHeadlight()->intensity.setValue( value );
}


visBase::Scene* ui3DViewer::getScene()
{
    if ( sobody_ ) return sobody_->getScene();
    else if ( osgbody_ ) return osgbody_->getScene();
    return 0;
}


const visBase::Scene* ui3DViewer::getScene() const
{ return const_cast<ui3DViewer*>(this)->getScene(); }
