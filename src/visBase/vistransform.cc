/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Feb 2002
 RCS:           $Id: vistransform.cc,v 1.21 2006-08-03 07:06:46 cvshelene Exp $
________________________________________________________________________

-*/

#include "vistransform.h"
#include "iopar.h"
#include "trigonometry.h"

#include <Inventor/nodes/SoMatrixTransform.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/SbLinear.h>

mCreateFactoryEntry( visBase::Transformation );
mCreateFactoryEntry( visBase::Rotation );

namespace visBase
{

const char* Transformation::matrixstr = "Matrix Row ";

Transformation::Transformation()
    : transform_( new SoMatrixTransform )
{
    transform_->ref();
}


Transformation::~Transformation()
{
    transform_->unref();
}


void Transformation::setRotation( const Coord3& vec, float angle )
{
    SbVec3f translation;
    SbRotation rotation;
    SbVec3f scale;
    SbRotation scaleorientation;

    SbMatrix matrix = transform_->matrix.getValue();
    matrix.getTransform( translation, rotation, scale, scaleorientation );
    rotation = SbRotation( SbVec3f( vec.x, vec.y, vec.z ), angle );
    matrix.setTransform( translation, rotation, scale, scaleorientation );

    transform_->matrix.setValue( matrix );
}


void Transformation::setTranslation( const Coord3& vec )
{
    SbVec3f translation;
    SbRotation rotation;
    SbVec3f scale;
    SbRotation scaleorientation;

    SbMatrix matrix = transform_->matrix.getValue();
    matrix.getTransform( translation, rotation, scale, scaleorientation );
    translation = SbVec3f( vec.x, vec.y, vec.z );
    matrix.setTransform( translation, rotation, scale, scaleorientation );

    transform_->matrix.setValue( matrix );
}


Coord3 Transformation::getTranslation() const
{
    SbVec3f translation;
    SbRotation rotation;
    SbVec3f scale;
    SbRotation scaleorientation;

    const SbMatrix matrix = transform_->matrix.getValue();
    matrix.getTransform( translation, rotation, scale, scaleorientation );
    return Coord3( translation[0], translation[1], translation[2] );
}


void Transformation::setScale( const Coord3& vec )
{
    SbVec3f translation;
    SbRotation rotation;
    SbVec3f scale;
    SbRotation scaleorientation;

    SbMatrix matrix = transform_->matrix.getValue();
    matrix.getTransform( translation, rotation, scale, scaleorientation );
    scale = SbVec3f( vec.x, vec.y, vec.z );
    matrix.setTransform( translation, rotation, scale, scaleorientation );

    transform_->matrix.setValue( matrix );
}


Coord3 Transformation::getScale() const
{
    SbVec3f translation;
    SbRotation rotation;
    SbVec3f scale;
    SbRotation scaleorientation;

    const SbMatrix matrix = transform_->matrix.getValue();
    matrix.getTransform( translation, rotation, scale, scaleorientation );
    return Coord3( scale[0], scale[1], scale[2] );
}


void Transformation::reset()
{
    setA( 1, 0, 0, 0,
	  0, 1, 0, 0,
	  0, 0, 1, 0,
	  0, 0, 0, 1 );
}


void Transformation::setA( float a11, float a12, float a13, float a14,
				   float a21, float a22, float a23, float a24,
				   float a31, float a32, float a33, float a34,
				   float a41, float a42, float a43, float a44 )
{
    transform_->matrix.setValue( a11, a21, a31, a41,
	    			a12, a22, a32, a42,
				a13, a23, a33, a43,
				a14, a24, a34, a44 );
}


void Transformation::setA( const SbMatrix& matrix )
{
    transform_->matrix.setValue(matrix);
}


Coord3 Transformation::transform( const Coord3& pos ) const
{
    SbVec3f res( pos.x, pos.y, pos.z );
    transform( res );
    if ( mIsUdf(pos.z) ) res[2] = mUdf(float);

    return Coord3( res[0], res[1], res[2] );
}


void Transformation::transform( SbVec3f& res ) const
{ transform_->matrix.getValue().multVecMatrix( res, res ); } 


void Transformation::transformBack( SbVec3f& res ) const
{
    SbMatrix inverse = transform_->matrix.getValue().inverse();
    inverse.multVecMatrix( res, res );
}


Coord3 Transformation::transformBack( const Coord3& pos ) const
{
    SbVec3f res( pos.x, pos.y, pos.z );
    transformBack( res );
    if ( mIsUdf(pos.z) ) res[2] = mUdf(float);

    return Coord3( res[0], res[1], res[2] );
}


void Transformation::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( par, saveids );
    const SbMat& matrix = transform_->matrix.getValue().getValue();

    BufferString key = matrixstr; key += 1; 
    par.set( key, matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0] );

    key = matrixstr; key += 2;
    par.set( key, matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1] );

    key = matrixstr; key += 3;
    par.set( key, matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2] );

    key = matrixstr; key += 4;
    par.set( key, matrix[0][3], matrix[1][3], matrix[2][3], matrix[3][3] );
}


int Transformation::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res!= 1 ) return res;

    double matrix[4][4];
    BufferString key = matrixstr; key += 1; 
    SbMatrix inverse = transform_->matrix.getValue();
    if ( !par.get( key, matrix[0][0],matrix[1][0],matrix[2][0],matrix[3][0] ))
	return -1;

    key = matrixstr; key += 2;
    if ( !par.get( key, matrix[0][1],matrix[1][1],matrix[2][1],matrix[3][1] ))
	return -1;

    key = matrixstr; key += 3;
    if ( !par.get( key, matrix[0][2],matrix[1][2],matrix[2][2],matrix[3][2] ))
	return -1;

    key = matrixstr; key += 4;
    if ( !par.get( key, matrix[0][3],matrix[1][3],matrix[2][3],matrix[3][3] ))
	return -1;

    setA(   matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0],
	    matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1],
	    matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2],
	    matrix[0][3], matrix[1][3], matrix[2][3], matrix[3][3] );

    return 1;
}

		  
SoNode* Transformation::getInventorNode()
{
    return transform_;
}


Rotation::Rotation()
    : rotation_( new SoRotation )
{
    rotation_->ref();
}


Rotation::~Rotation()
{
    rotation_->unref();
}


void Rotation::set( const Coord3& vec, float angle )
{
    set( Quaternion( vec, angle ) );
}


void Rotation::set( const Quaternion& q )
{
    rotation_->rotation.setValue( q.vec_.x, q.vec_.y, q.vec_.z, q.s_ );
}


Coord3 Rotation::transform( const Coord3& input ) const
{
    const float* rot = rotation_->rotation.getValue().getValue();
    const Quaternion q( rot[3], rot[0], rot[1], rot[2] );
    return q.rotate( input );
}


Coord3 Rotation::transformBack( const Coord3& input ) const
{
    const float* rot = rotation_->rotation.getValue().getValue();
    const Quaternion q( Quaternion(rot[3], rot[0], rot[1], rot[2]).inverse() );
    return q.rotate( input );
}


SoNode* Rotation::getInventorNode() { return rotation_; }



}; // namespace visBase
