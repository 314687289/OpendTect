/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visobject.cc,v 1.42 2007-03-06 20:31:54 cvskris Exp $";

#include "visobject.h"

#include "errh.h"
#include "iopar.h"
#include "visdataman.h"
#include "visevent.h"
#include "vismaterial.h"
#include "vistransform.h"

#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>

namespace visBase
{

const char* VisualObjectImpl::sKeyMaterialID()	{ return "Material ID"; }
const char* VisualObjectImpl::sKeyIsOn()	{ return "Is on"; }


VisualObject::VisualObject( bool issel )
    : isselectable(issel)
    , deselnotifier(this)
    , selnotifier(this)
    , rightClick(this)
    , rcevinfo(0)
{}


VisualObject::~VisualObject()
{}


VisualObjectImpl::VisualObjectImpl( bool issel )
    : VisualObject( issel )
    , root_( new SoSeparator )
    , onoff_( new SoSwitch )
    , material_( 0 )
{
    setMaterial( Material::create() );
    onoff_->ref();
    onoff_->addChild( root_ );
    onoff_->whichChild = SO_SWITCH_ALL;
}


VisualObjectImpl::~VisualObjectImpl()
{
    getInventorNode()->unref();
    if ( material_ ) material_->unRef();
}


void VisualObjectImpl::turnOn( bool yn )
{
    if ( onoff_ ) onoff_->whichChild = yn ? SO_SWITCH_ALL : SO_SWITCH_NONE;
    else if ( !yn )
    {
	pErrMsg( "Turning off object without switch");
    }

}


bool VisualObjectImpl::isOn() const
{
    return !onoff_ || onoff_->whichChild.getValue()==SO_SWITCH_ALL;
}


void VisualObjectImpl::setMaterial( Material* nm )
{
    if ( material_ )
    {
	root_->removeChild( material_->getInventorNode() );
	material_->unRef();
    }

    material_ = nm;

    if ( material_ )
    {
	material_->ref();
	root_->insertChild( material_->getInventorNode(), 0 );
    }
}


void VisualObjectImpl::removeSwitch()
{
    root_->ref();
    onoff_->unref();
    onoff_ = 0;
}


SoNode* VisualObjectImpl::getInventorNode() 
{ return onoff_ ? (SoNode*) onoff_ : (SoNode*) root_; }


void VisualObjectImpl::addChild( SoNode* nn )
{ root_->addChild( nn ); }


void VisualObjectImpl::insertChild( int pos, SoNode* nn )
{ root_->insertChild( nn, pos ); }


void VisualObjectImpl::removeChild( SoNode* nn )
{ root_->removeChild( nn ); }


int VisualObjectImpl::childIndex( const SoNode* nn ) const
{ return root_->findChild(nn); }


SoNode* VisualObjectImpl::getChild(int idx)
{ return root_->getChild(idx); }


int VisualObjectImpl::usePar( const IOPar& iopar )
{
    int res = VisualObject::usePar( iopar );
    if ( res != 1 ) return res;

    int matid;
    if ( iopar.get(sKeyMaterialID(),matid) )
    {
	if ( matid==-1 ) setMaterial( 0 );
	else
	{
	    DataObject* mat = DM().getObject( matid );
	    if ( !mat ) return 0;
	    if ( typeid(*mat) != typeid(Material) ) return -1;

	    setMaterial( (Material*)mat );
	}
    }
    else
	setMaterial( 0 );

    bool isonsw;
    if ( iopar.getYN(sKeyIsOn(),isonsw) )
	turnOn( isonsw );

    return 1;
}


void VisualObjectImpl::fillPar( IOPar& iopar,
					 TypeSet<int>& saveids ) const
{
    VisualObject::fillPar( iopar, saveids );
    iopar.set( sKeyMaterialID(), material_ ? material_->id() : -1 );

    if ( material_ && saveids.indexOf(material_->id()) == -1 )
	saveids += material_->id();

    iopar.setYN( sKeyIsOn(), isOn() );
}


void VisualObject::triggerRightClick( const EventInfo* eventinfo )
{
    rcevinfo = eventinfo;
    rightClick.trigger();
}


bool VisualObject::getBoundingBox( Coord3& minpos, Coord3& maxpos ) const
{
    SbViewportRegion vp;
    SoGetBoundingBoxAction action( vp );
    action.apply( const_cast<SoNode*>(getInventorNode()) );
    const SbBox3f bbox = action.getBoundingBox();

    if ( bbox.isEmpty() )
	return false;

    const SbVec3f min = bbox.getMin();
    const SbVec3f max = bbox.getMax();

    minpos.x = min[0]; minpos.y = min[1]; minpos.z = min[2];
    maxpos.x = max[0]; maxpos.y = max[1]; maxpos.z = max[2];

    const Transformation* trans =
	const_cast<VisualObject*>(this)->getDisplayTransformation();
    if ( trans )
    {
	minpos = trans->transformBack( minpos );
	maxpos = trans->transformBack( maxpos );
    }

    return true;
}


const TypeSet<int>* VisualObject::rightClickedPath() const
{
    return rcevinfo ? &rcevinfo->pickedobjids : 0;
}

}; //namespace
