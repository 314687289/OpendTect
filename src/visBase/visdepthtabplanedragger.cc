/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jul 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visdepthtabplanedragger.h"

#include "vistransform.h"
#include "position.h"
#include "ranges.h"
#include "iopar.h"
#include "keyenum.h"
#include "mouseevent.h"

#include <osgManipulator/TabPlaneDragger>
#include <osg/Geometry>
#include <osg/Switch>
#include <osg/LightModel>
#include <osg/BlendFunc>
#include <osg/Version>

mCreateFactoryEntry( visBase::DepthTabPlaneDragger );

namespace visBase
{


class PlaneDraggerCallbackHandler: public osgManipulator::DraggerCallback
{

public:

    PlaneDraggerCallbackHandler( DepthTabPlaneDragger& dragger )  
	: dragger_( dragger )						{}

    using			osgManipulator::DraggerCallback::receive;
    virtual bool		receive(const osgManipulator::MotionCommand&);

protected:

    void			constrain(bool translatedinline);

    DepthTabPlaneDragger&	dragger_;

    osg::Matrix			initialosgmatrix_;
    Coord3			initialcenter_;
};


bool PlaneDraggerCallbackHandler::receive(
				    const osgManipulator::MotionCommand& cmd )
{
    if ( cmd.getStage()==osgManipulator::MotionCommand::START )
    {
	initialosgmatrix_ = dragger_.osgdragger_->getMatrix();
	initialcenter_ = dragger_.center();
    }

    mDynamicCastGet( const osgManipulator::Scale1DCommand*, s1d, &cmd );
    mDynamicCastGet( const osgManipulator::Scale2DCommand*, s2d, &cmd );
    mDynamicCastGet( const osgManipulator::TranslateInLineCommand*,
		     translatedinline, &cmd );
    mDynamicCastGet( const osgManipulator::TranslateInPlaneCommand*,
		     translatedinplane, &cmd );

    bool ignore = !s1d && !s2d && !translatedinline && !translatedinplane;

    const TabletInfo* ti = TabletInfo::currentState();
    if ( ti && ti->maxPostPressDist()<5 )
	ignore = true;

    if ( ignore )
    {
	dragger_.osgdragger_->setMatrix( initialosgmatrix_ );
	return true;
    }

    if ( cmd.getStage()==osgManipulator::MotionCommand::START )
    {
	dragger_.started.trigger();
    }
    else if ( cmd.getStage()==osgManipulator::MotionCommand::MOVE )
    {
	constrain( translatedinline );
	dragger_.motion.trigger();
    }
    else if ( cmd.getStage()==osgManipulator::MotionCommand::FINISH )
    {
	dragger_.finished.trigger();
	if ( initialosgmatrix_ != dragger_.osgdragger_->getMatrix() )
	    dragger_.changed.trigger();
    }

    return true;
}


void PlaneDraggerCallbackHandler::constrain( bool translatedinline )
{
    Coord3 scale = dragger_.size();
    Coord3 center = dragger_.center();

    for ( int dim=0; dim<3; dim++ )
    {
	if ( translatedinline && dim==dragger_.dim_ )
	{
	    if ( dragger_.spaceranges_[dim].width(false) > 0.0 )
	    {
		if ( center[dim] < dragger_.spaceranges_[dim].start )
		    center[dim] = dragger_.spaceranges_[dim].start;
		if ( center[dim] > dragger_.spaceranges_[dim].stop )
		    center[dim] = dragger_.spaceranges_[dim].stop;
	    }
	}
		
	if ( !translatedinline && dim!=dragger_.dim_ )
	{
	    if ( dragger_.spaceranges_[dim].width(false) > 0.0 )
	    {
		double diff = center[dim] - 0.5*scale[dim] -
			      dragger_.spaceranges_[dim].start;
		if ( diff < 0.0 )
		{
		    center[dim] -= 0.5*diff;
		    scale[dim] += diff;
		}

		diff = center[dim] + 0.5*scale[dim] -
		       dragger_.spaceranges_[dim].stop;
		if ( diff > 0.0 )
		{
		    center[dim] -= 0.5*diff;
		    scale[dim] -= diff;
		}
	    }

	    if ( dragger_.widthranges_[dim].width(false) > 0.0 )
	    {
		double diff = scale[dim] - dragger_.widthranges_[dim].start;
		if ( diff < 0 )
		{
		    if ( center[dim] < initialcenter_[dim] )
			center[dim] -= 0.5*diff;
		    else
			center[dim] += 0.5*diff;

		    scale[dim] -= diff;
		}

		diff = scale[dim] - dragger_.widthranges_[dim].stop;
		if ( diff > 0 )
		{
		    if ( center[dim] > initialcenter_[dim] )
			center[dim] -= 0.5*diff;
		    else
			center[dim] += 0.5*diff;

		    scale[dim] -= diff;
		}
	    }
	}
    }

    dragger_.setOsgMatrix( scale, center );
}


//=============================================================================


DepthTabPlaneDragger::DepthTabPlaneDragger()
    : VisualObjectImpl( false )
    , transform_( 0 )
    , dim_( 2 )
    , started( this )
    , motion( this )
    , changed( this )
    , finished( this )
    , osgdragger_( 0 )
    , osgdraggerplane_( 0 )
    , osgcallbackhandler_( 0 )
{

    initOsgDragger();

    centers_ += center(); centers_ += center(); centers_ += center();
    sizes_ += size(); sizes_ += size(); sizes_ += size();

    setDim(dim_);
}


DepthTabPlaneDragger::~DepthTabPlaneDragger()
{
    osgdragger_->removeDraggerCallback( osgcallbackhandler_ );

    if ( transform_ ) transform_->unRef();
}


void DepthTabPlaneDragger::initOsgDragger()
{
    if ( osgdragger_ )
	return;

#if OSG_MIN_VERSION_REQUIRED(3,1,3)
    osgdragger_ = new osgManipulator::TabPlaneDragger( 12.0 );
    osgdragger_->setIntersectionMask( cIntersectionTraversalMask() );
//    osgdragger_->setActivationMouseButtonMask(
//	    			osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON );
#else
    osgdragger_ = new osgManipulator::TabPlaneDragger();
#endif

    addChild( osgdragger_ );

    osgdragger_->setupDefaultGeometry();
    osgdragger_->setHandleEvents( true );

    osgcallbackhandler_ = new PlaneDraggerCallbackHandler( *this );
    osgdragger_->addDraggerCallback( osgcallbackhandler_ );

    osgdragger_->getOrCreateStateSet()->setAttributeAndModes(
	    	new osg::PolygonOffset(-1.0,1.0), osg::StateAttribute::ON );
    osgdragger_->getOrCreateStateSet()->setRenderingHint(
					    osg::StateSet::TRANSPARENT_BIN );

    for ( int idx=osgdragger_->getNumDraggers()-1; idx>=0; idx-- )
    {
	osgManipulator::Dragger* dragger = osgdragger_->getDragger( idx );
	mDynamicCastGet( osgManipulator::Scale1DDragger*, s1dd, dragger );
	if ( s1dd )
	{
	    s1dd->setColor( osg::Vec4(0.0,0.7,0.0,1.0) );
	    s1dd->setPickColor( osg::Vec4(0.0,1.0,0.0,1.0) );
	}
	mDynamicCastGet( osgManipulator::Scale2DDragger*, s2dd, dragger );
	if ( s2dd )
	{
	    s2dd->setColor( osg::Vec4(0.0,0.7,0.0,1.0) );
	    s2dd->setPickColor( osg::Vec4(0.0,1.0,0.0,1.0) );
	}
    }

    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    vertices->push_back( osg::Vec3(-0.5,0.0,0.5) );
    vertices->push_back( osg::Vec3(-0.5,0.0,-0.5) );
    vertices->push_back( osg::Vec3(0.5,0.0,-0.5) );
    vertices->push_back( osg::Vec3(0.5,0.0,0.5) );

    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    normals->push_back( osg::Vec3(0.0,1.0,0.0) );

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    colors->push_back( osg::Vec4(0.7,0.7,0.7,0.5) );

    osg::ref_ptr<osg::Geometry> plane = new osg::Geometry;
    plane->setVertexArray( vertices.get() );
    plane->setNormalArray( normals.get() );
    plane->setNormalBinding( osg::Geometry::BIND_OVERALL );
    plane->setColorArray( colors.get() );
    plane->setColorBinding( osg::Geometry::BIND_OVERALL );

    plane->addPrimitiveSet( new osg::DrawArrays(GL_QUADS,0,4) );
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable( plane.get() );
    geode->getOrCreateStateSet()->setMode( GL_BLEND, osg::StateAttribute::ON );
    geode->getStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
    geode->getStateSet()->setAttributeAndModes(
	    	new osg::PolygonOffset(1.0,1.0), osg::StateAttribute::ON );

    osgdraggerplane_ = new osg::Switch();
    osgdraggerplane_->addChild( geode.get() );
    osgdragger_->addChild( osgdraggerplane_ );

    showPlane( false );
    showDraggerBorder( true );
}


void DepthTabPlaneDragger::setOsgMatrix( const Coord3& worldscale,
					 const Coord3& worldtrans )
{
    osg::Matrix mat;

    if ( dim_ == 0 )
	mat.makeRotate( osg::Vec3(1,0,0), osg::Vec3(0,1,0) );
    else if ( dim_ == 2 )
	mat.makeRotate( osg::Vec3(0,1,0), osg::Vec3(0,0,1) );

    osg::Vec3d scale, trans;
    mVisTrans::transformDir( transform_, worldscale, scale );
    mVisTrans::transform( transform_, worldtrans, trans );

    mat *= osg::Matrix::scale( scale );
    mat *= osg::Matrix::translate( trans );
    osgdragger_->setMatrix( mat );
}


void DepthTabPlaneDragger::setCenter( const Coord3& newcenter, bool alldims )
{
    centers_[dim_] = newcenter;

    if ( alldims )
    {
	centers_[0] = newcenter;
	centers_[1] = newcenter;
	centers_[2] = newcenter;
    }

    setOsgMatrix( size(), newcenter );
}


Coord3 DepthTabPlaneDragger::center() const
{
    Coord3 res;
    Transformation::transformBack( transform_,
				   osgdragger_->getMatrix().getTrans(), res );
    return res;
}


void DepthTabPlaneDragger::setSize( const Coord3& scale, bool alldims )
{
    const float abs = scale.abs();
    Coord3 newscale( scale[0] ? scale[0] : abs,
    		     scale[1] ? scale[1] : abs,
    		     scale[2] ? scale[2] : abs );
    sizes_[dim_] = newscale;

    if ( alldims )
    {
	sizes_[0] = newscale; sizes_[1] = newscale; sizes_[2] = newscale;
    }

    setOsgMatrix( newscale, center() );
}


Coord3 DepthTabPlaneDragger::size() const
{
    Coord3 scale;
    Transformation::transformBackDir( transform_,
				      osgdragger_->getMatrix().getScale(),
				      scale );

    scale.x = fabs(scale.x); scale.y = fabs(scale.y); scale.z = fabs(scale.z);
    return scale;
}


void DepthTabPlaneDragger::removeScaleTabs()
{
    for ( int idx=osgdragger_->getNumDraggers()-1; idx>=0; idx-- )
    {
	osgManipulator::Dragger* dragger = osgdragger_->getDragger( idx );
	mDynamicCastGet( osgManipulator::Scale1DDragger*, s1dd, dragger );
	mDynamicCastGet( osgManipulator::Scale2DDragger*, s2dd, dragger );
	if ( s1dd || s2dd )
	{
	    osgdragger_->removeChild( dragger );
	    osgdragger_->removeDragger( dragger );
	}
    }
}


void DepthTabPlaneDragger::setDim( int newdim )
{
    centers_[dim_] = center();
    sizes_[dim_] = size();

    dim_ = newdim;

    NotifyStopper stopper( changed );
    setSize( sizes_[dim_], false );
    setCenter( centers_[dim_], false );
    stopper.restore();
}


int DepthTabPlaneDragger::getDim() const
{
    return dim_;
}


void DepthTabPlaneDragger::setSpaceLimits( const Interval<float>& x,
					   const Interval<float>& y,
					   const Interval<float>& z )
{
    spaceranges_[0] = x; spaceranges_[1] = y; spaceranges_[2] = z;
}


void DepthTabPlaneDragger::getSpaceLimits( Interval<float>& x,
					   Interval<float>& y,
					   Interval<float>& z ) const
{
    x = spaceranges_[0]; y = spaceranges_[1]; z = spaceranges_[2];
}


void DepthTabPlaneDragger::setWidthLimits( const Interval<float>& x,
					   const Interval<float>& y,
					   const Interval<float>& z )
{
    widthranges_[0] = x; widthranges_[1] = y; widthranges_[2] = z;
}


void DepthTabPlaneDragger::getWidthLimits( Interval<float>& x,
					   Interval<float>& y,
					   Interval<float>& z ) const
{
    x = widthranges_[0]; y = widthranges_[1]; z = widthranges_[2];
}


void DepthTabPlaneDragger::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transform_==nt ) return;

    const Coord3 centerpos = center();
    const Coord3 savedsize = size();

    if ( transform_ )
    {
	removeChild( const_cast<mVisTrans*>(transform_)->getInventorNode() );
	transform_->unRef();
    }

    transform_ = nt;

    if ( transform_ )
    {
	insertChild(0, const_cast<mVisTrans*>(transform_)->getInventorNode() );
	transform_->ref();
    }

    setSize( savedsize );
    setCenter( centerpos );
}


const mVisTrans* DepthTabPlaneDragger::getDisplayTransformation() const
{
    return transform_;
}


void DepthTabPlaneDragger::setTransDragKeys( bool depth, int ns )
{
    const bool ctrl = ns & OD::ControlButton;
    const bool shift = ns & OD::ShiftButton;
    const bool alt = ns & OD::AltButton;

    unsigned int mask  = osgGA::GUIEventAdapter::NONE;
    if ( ctrl )  mask |= osgGA::GUIEventAdapter::MODKEY_CTRL;
    if ( shift ) mask |= osgGA::GUIEventAdapter::MODKEY_SHIFT;
    if ( alt )   mask |= osgGA::GUIEventAdapter::MODKEY_ALT;

    for ( int idx=osgdragger_->getNumDraggers()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( osgManipulator::TranslatePlaneDragger*, tpd,
			 osgdragger_->getDragger(idx) );

	if ( tpd && depth )
	{
	    //tpd->getTranslate1DDragger()->setActivationMouseButtonMask( osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON );
	    tpd->getTranslate1DDragger()->setActivationModKeyMask( mask );
	}
	if ( tpd && !depth )
	{
	    //tpd->getTranslate1DDragger()->setActivationMouseButtonMask( osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON );
	    tpd->getTranslate2DDragger()->setActivationModKeyMask( mask );
	}
    }
}


int DepthTabPlaneDragger::getTransDragKeys( bool depth ) const
{

    int state = OD::NoButton;
    for ( int idx=osgdragger_->getNumDraggers()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( osgManipulator::TranslatePlaneDragger*, tpd,
			 osgdragger_->getDragger(idx) );
	if ( !tpd )
	    continue;

	const osgManipulator::Dragger* dragger;
	if ( depth )
	    dragger = tpd->getTranslate1DDragger();
	else 
	    dragger = tpd->getTranslate2DDragger();

	const unsigned int ctrl  = osgGA::GUIEventAdapter::MODKEY_CTRL;
	if ( (dragger->getActivationModKeyMask() & ctrl) == ctrl )
	    state |= OD::ControlButton;

	const unsigned int shift = osgGA::GUIEventAdapter::MODKEY_SHIFT;
	if ( (dragger->getActivationModKeyMask() & shift) == shift )
	    state |= OD::ShiftButton;

	const unsigned int alt   = osgGA::GUIEventAdapter::MODKEY_ALT;
	if ( (dragger->getActivationModKeyMask() & alt) == alt )
	    state |= OD::AltButton;
    }
    return (OD::ButtonState) state;
}


void DepthTabPlaneDragger::showDraggerBorder( bool yn )
{
    const float borderopacity = yn ? 1.0 : 0.0;
    for ( int idx=osgdragger_->getNumDraggers()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( osgManipulator::TranslatePlaneDragger*, tpd,
			 osgdragger_->getDragger(idx) );
	if ( tpd )
	{
	    tpd->getTranslate2DDragger()->setColor(
				    osg::Vec4(0.5,0.5,0.5,borderopacity) );
	    tpd->getTranslate2DDragger()->setPickColor(
				    osg::Vec4(1.0,1.0,1.0,borderopacity) );
	}
    }
}


bool DepthTabPlaneDragger::isDraggerBorderShown() const
{
    for ( int idx=osgdragger_->getNumDraggers()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( osgManipulator::TranslatePlaneDragger*, tpd,
			 osgdragger_->getDragger(idx) );
	if ( tpd )
	    return tpd->getTranslate2DDragger()->getColor()[3];
    }
    
    return false;
}


void DepthTabPlaneDragger::showPlane( bool yn )
{
    if ( osgdraggerplane_ )
	osgdraggerplane_->setValue( 0, yn );
}


bool DepthTabPlaneDragger::isPlaneShown() const
{ return osgdraggerplane_ && osgdraggerplane_->getValue(0); }


}; // namespace visBase
