/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Jul 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: visinvisiblelinedragger.cc,v 1.2 2008-11-25 15:35:27 cvsbert Exp $";

#include "visinvisiblelinedragger.h"

#include "SoInvisibleLineDragger.h"
#include "vistransform.h"
#include "position.h"

#include <Inventor/nodes/SoSeparator.h>

mCreateFactoryEntry( visBase::InvisibleLineDragger );

namespace visBase
{

InvisibleLineDragger::InvisibleLineDragger()
    : VisualObjectImpl( false )
    , dragger_( new SoInvisibleLineDragger )
    , transform_( 0 )
    , started( this )
    , motion( this )
    , finished( this )
    , needsDirection( this )
    , shape_( 0 )
{
    setMaterial( 0 );
    addChild( dragger_ );

    dragger_->addStartCallback( InvisibleLineDragger::startCB, this );
    dragger_->addMotionCallback( InvisibleLineDragger::motionCB, this );
    dragger_->addFinishCallback( InvisibleLineDragger::finishCB, this );
    dragger_->needsDirection.addCallback( 
	    		InvisibleLineDragger::needsDirectionCB, this );
}


InvisibleLineDragger::~InvisibleLineDragger()
{
    if ( transform_ ) transform_->unRef();
    if ( shape_ ) shape_->unRef();

    dragger_->removeStartCallback( InvisibleLineDragger::startCB, this );
    dragger_->removeMotionCallback( InvisibleLineDragger::motionCB, this );
    dragger_->removeFinishCallback( InvisibleLineDragger::finishCB, this );
    dragger_->needsDirection.removeCallback( 
	    		InvisibleLineDragger::needsDirectionCB, this );
}


void InvisibleLineDragger::setDirection( const Coord3& newdir )
{
    dragger_->setDirection( SbVec3f( newdir.x, newdir.y, newdir.z ) );
}


Coord3 InvisibleLineDragger::getTranslation() const
{
    const SbVec3f transl = dragger_->translation;
    return Coord3( transl[0],transl[1],transl[2] );
}


Coord3 InvisibleLineDragger::getStartPos() const
{
    const SbVec3f pos = dragger_->startPos;
    Coord3 res( pos[0], pos[1], pos[2] );
    return transform_ ? transform_->transformBack( res ) : res;
}


void InvisibleLineDragger::setDisplayTransformation( Transformation* nt )
{
    if ( transform_ )
	transform_->unRef();

    transform_ = nt;

    if ( transform_ )
	transform_->ref();

    shape_->setDisplayTransformation( nt );
}


Transformation* InvisibleLineDragger::getDisplayTransformation()
{ return transform_; }


void InvisibleLineDragger::setShape( DataObject* newshape )
{
    if ( shape_ )
	shape_->unRef();

    shape_ = newshape;
    if ( shape_ )
    {
	shape_->setDisplayTransformation( transform_ );
	shape_->ref();
    }

    dragger_->setPart( SoInvisibleLineDragger::sKeyShape(),
	    	       newshape->getInventorNode());
}


void InvisibleLineDragger::startCB( void* obj, SoDragger* )
{
    ((InvisibleLineDragger*)obj)->started.trigger();
}


void InvisibleLineDragger::motionCB( void* obj, SoDragger* )
{
    ((InvisibleLineDragger*)obj)->motion.trigger();
}


void InvisibleLineDragger::needsDirectionCB( void* obj, void* )
{
    ((InvisibleLineDragger*)obj)->needsDirection.trigger();
}


void InvisibleLineDragger::finishCB( void* obj, SoDragger* )
{
    ((InvisibleLineDragger*)obj)->finished.trigger();
}


}; // namespace visBase
