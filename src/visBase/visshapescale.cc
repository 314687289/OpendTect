/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          July 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visshapescale.h"
#include "iopar.h"
#include "visdataman.h"

#include "SoShapeScale.h"

#include <Inventor/nodes/SoSeparator.h>

mCreateFactoryEntry( visBase::ShapeScale );

namespace visBase
{

ShapeScale::ShapeScale()
    : root( new SoSeparator )
    , shapescalekit( new SoShapeScale )
    , shape( 0 )
    , node( 0 )
{
    root->ref();
    root->addChild(shapescalekit);
    shapescalekit->screenSize.setValue( 5 );
    shapescalekit->dorotate.setValue( false );
}


ShapeScale::~ShapeScale()
{
    if ( shape ) shape->unRef();
    root->unref();
}


void ShapeScale::setShape( DataObject* no )
{
    if ( shape )
    {
	shape->unRef();
    }

    if ( node )
    {
	root->removeChild(node);
	node = 0;
    }

    shape = no;
    if ( no )
    {
	no->ref();
	node = no->getInventorNode();
    }

    if ( node ) root->addChild( node );
}


void ShapeScale::setShape( SoNode* newnode )
{
    if ( shape )
    {
	shape->unRef();
    }

    if ( node )
	root->removeChild(node);

    shape = 0;
    node = newnode;
    root->addChild( node );
}


void ShapeScale::setScreenSize( float nz )
{
    shapescalekit->screenSize.setValue( nz );
}


float ShapeScale::getScreenSize() const
{
    return shapescalekit->screenSize.getValue();
}


void ShapeScale::setMinScale( float nz )
{
    shapescalekit->minScale.setValue( nz );
}


float ShapeScale::getMinScale() const
{
    return shapescalekit->minScale.getValue();
}


void ShapeScale::setMaxScale( float nz )
{
    shapescalekit->maxScale.setValue( nz );
}


float ShapeScale::getMaxScale() const
{
    return shapescalekit->maxScale.getValue();
}




void ShapeScale::restoreProportions(bool yn)
{
    shapescalekit->restoreProportions = yn;
}


bool ShapeScale::restoresProportions() const
{
    return shapescalekit->restoreProportions.getValue();
}


SoNode* ShapeScale::gtInvntrNode() 
{
    return root;
}


}; // namespace visBase
