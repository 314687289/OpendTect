/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Y.C. Liu
 Date:          Oct. 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visellipsoid.h"

#include "vistransform.h"
#include "iopar.h"

#include <Inventor/nodes/SoMatrixTransform.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/SbLinear.h>

mCreateFactoryEntry( visBase::Ellipsoid );

namespace visBase
{

Ellipsoid::Ellipsoid()
    : Shape( new SoSphere )
    , position_( new SoMatrixTransform )
    , transformation_( 0 )
{
//    insertNode( position_ );
}


Ellipsoid::~Ellipsoid()
{ setDisplayTransformation(0); }


void Ellipsoid::setDisplayTransformation( const mVisTrans* tf )
{
    if ( transformation_==tf ) return;

    const Coord3 oldpos = getCenterPos();
    const Coord3 oldwidth = getWidth();

    if ( transformation_ )
	transformation_->unRef();

    transformation_ = tf;

    if ( transformation_ )
	transformation_->ref();

    setCenterPos( oldpos );
    setWidth( oldwidth );
}


void Ellipsoid::setCenterPos( const Coord3& pos )
{
    const Coord3 npos = transformation_? transformation_->transform(pos) : pos;
    const Coord3 curscale = getWidth()/2;
    SbMatrix matrix;
    matrix.setTransform( SbVec3f((float) npos.x,(float) npos.y,(float) npos.z),
	  SbRotation(SbVec3f((float) npos.x,(float) npos.y,(float) npos.z),0),
	  SbVec3f((float) curscale.x, (float) curscale.y,(float) curscale.z) );

    position_->matrix.setValue( matrix );
}


Coord3 Ellipsoid::getCenterPos() const
{
    SbVec3f tran;
    SbVec3f scale;
    SbRotation r;
    SbRotation so;
    position_->matrix.getValue().getTransform( tran, r, scale, so );

    Coord3 res;
    res.x = tran[0];
    res.y = tran[1];
    res.z = tran[2];

    return transformation_ ? transformation_->transformBack(res) : res;
}


void Ellipsoid::setWidth( const Coord3& n )
{
    SbMatrix matrix = position_->matrix.getValue();
    SbVec3f tran;
    SbVec3f scale;
    SbRotation r;
    SbRotation so;
    matrix.getTransform( tran, r, scale, so );

    scale[0] =(float) n.x/2; scale[1] =(float) n.y/2; scale[2] =(float) n.z/2;
    matrix.setTransform( tran, r, scale, so );
    position_->matrix.setValue( matrix );
}


Coord3 Ellipsoid::getWidth() const
{
    SbVec3f tran;
    SbVec3f scale;
    SbRotation r;
    SbRotation so;
    position_->matrix.getValue().getTransform( tran, r, scale, so );

    Coord3 res;
    res.x = scale[0]/2;
    res.y = scale[1]/2;
    res.z = scale[2]/2;

    return res;
}


}; //namespace
