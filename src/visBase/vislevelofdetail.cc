/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: vislevelofdetail.cc,v 1.7 2005-02-07 12:45:40 nanne Exp $
________________________________________________________________________

-*/

#include "vislevelofdetail.h"

#include <Inventor/nodes/SoLevelOfDetail.h>

mCreateFactoryEntry( visBase::LevelOfDetail );

namespace visBase
{

LevelOfDetail::LevelOfDetail()
    : lod(new SoLevelOfDetail)
{
    lod->ref();
}


LevelOfDetail::~LevelOfDetail()
{
    lod->removeAllChildren();
    for ( int idx=0; idx<children.size(); idx++ )
	children[idx]->unRef();

    lod->unref();
}


void LevelOfDetail::addChild( DataObject* obj, float m )
{
    int nrkids = lod->getNumChildren();
    if ( nrkids )
	lod->screenArea.set1Value( nrkids-1, m );
    lod->addChild( obj->getInventorNode() );
    children += obj;
    obj->ref();
}


SoNode* LevelOfDetail::getInventorNode()
{ return lod; }

}; // namespace visBase
