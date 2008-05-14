/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2008
-*/

static const char* rcsID = "$Id: visshapehints.cc,v 1.1 2008-05-14 20:28:21 cvskris Exp $";

#include "visshapehints.h"

#include "Inventor/nodes/SoShapeHints.h"


namespace visBase
{
mCreateFactoryEntry( ShapeHints );


ShapeHints::ShapeHints()
    : shapehints_( new SoShapeHints )
{
    shapehints_->ref();
}


ShapeHints::~ShapeHints()
{
    shapehints_->unref();
}


void ShapeHints::setVertexOrder( VertexOrder vo )
{
    if ( vo==Unknown )
	shapehints_->vertexOrdering.setValue( SoShapeHints::UNKNOWN_ORDERING );
    else if ( vo==ClockWise )
	shapehints_->vertexOrdering.setValue( SoShapeHints::CLOCKWISE );
    else if ( vo==CounterClockWise )
	shapehints_->vertexOrdering.setValue( SoShapeHints::COUNTERCLOCKWISE );
}


ShapeHints::VertexOrder ShapeHints::getVertexOrder() const
{
    if ( shapehints_->vertexOrdering.getValue()==SoShapeHints::UNKNOWN_ORDERING )
	return Unknown;

    if ( shapehints_->vertexOrdering.getValue()==SoShapeHints::CLOCKWISE )
	return ClockWise;

    return CounterClockWise;
}


void ShapeHints::setSolidShape( bool yn )
{
    shapehints_->shapeType.setValue(
	    yn ? SoShapeHints::SOLID : SoShapeHints::UNKNOWN_SHAPE_TYPE );
}


bool ShapeHints::isSolidShape() const
{
    return shapehints_->shapeType.getValue() == SoShapeHints::SOLID;
}


SoNode* ShapeHints::getInventorNode()
{ return shapehints_; }


}; //namespace
