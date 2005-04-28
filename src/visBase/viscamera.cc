/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Feb 2002
 RCS:           $Id: viscamera.cc,v 1.19 2005-04-28 14:23:51 cvsnanne Exp $
________________________________________________________________________

-*/

#include "viscamera.h"
#include "iopar.h"

#include "UTMCamera.h"

mCreateFactoryEntry( visBase::Camera );

namespace visBase
{

const char* Camera::posstr = "Position";
const char* Camera::orientationstr = "Orientation";
const char* Camera::aspectratiostr = "Aspect ratio";
const char* Camera::heightanglestr = "Height Angle";
const char* Camera::neardistancestr = "Near Distance";
const char* Camera::fardistancestr = "Far Distance";
const char* Camera::focaldistancestr = "Focal Distance";


Camera::Camera()
    : camera( new UTMCamera )
{}


Camera::~Camera()
{}


SoNode* Camera::getInventorNode()
{ return camera; }


void Camera::setPosition(const Coord3& pos)
{
    camera->position.setValue( pos.x, pos.y, pos.z );
}


Coord3 Camera::position() const
{
    SbVec3f pos = camera->position.getValue();
    Coord3 res;
    res.x = pos[0];
    res.y = pos[1];
    res.z = pos[2];
    return res;
}


void Camera::setOrientation( const Coord3& dir, float angle )
{
    camera->orientation.setValue( dir.x, dir.y, dir.z, angle );
}


void Camera::getOrientation( Coord3& dir, float& angle )
{
    SbVec3f axis;
    camera->orientation.getValue( axis, angle );
    dir.x = axis[0];
    dir.y = axis[1];
    dir.z = axis[2];
}


void Camera::pointAt( const Coord3& pos )
{
    camera->pointAt( SbVec3f(pos.x,pos.y,pos.z) );
}


void Camera::pointAt(const Coord3& pos, const Coord3& upvector )
{
    camera->pointAt( SbVec3f( pos.x, pos.y, pos.z ),
	    	     SbVec3f( upvector.x, upvector.y, upvector.z ));
}


void Camera::setAspectRatio( float n )
{
    camera->aspectRatio.setValue(n);
}


float Camera::aspectRatio() const
{
    return camera->aspectRatio.getValue();
}


void Camera::setHeightAngle( float n )
{
    camera->heightAngle.setValue(n);
}


float Camera::heightAngle() const
{
    return camera->heightAngle.getValue();
}


void Camera::setNearDistance( float n )
{
    camera->nearDistance.setValue(n);
}


float Camera::nearDistance() const
{
    return camera->nearDistance.getValue();
}


void Camera::setFarDistance( float n )
{
    camera->farDistance.setValue(n);
}


float Camera::farDistance() const
{
    return camera->farDistance.getValue();
}


void Camera::setFocalDistance(float n)
{
    camera->focalDistance.setValue(n);
}


float Camera::focalDistance() const
{
    return camera->focalDistance.getValue();
}


void Camera::setStereoAdjustment(float n)
{
    camera->setStereoAdjustment( n );
}

float Camera::getStereoAdjustment() const
{
    return camera->getStereoAdjustment();
}

void Camera::setBalanceAdjustment(float n)
{
    camera->setBalanceAdjustment( n );
}

float Camera::getBalanceAdjustment() const
{
    return camera->getBalanceAdjustment();
}

int Camera::usePar( const IOPar& iopar )
{
    int res = DataObject::usePar( iopar );
    if ( res != 1 ) return res;

    Coord3 pos;
    if ( iopar.get(posstr,pos) )
	setPosition( pos );

    double angle;
    if ( iopar.get( orientationstr, pos.x, pos.y, pos.z, angle ) )
	camera->orientation.setValue( SbVec3f(pos.x,pos.y,pos.z), angle );

    float val;
    if ( iopar.get(aspectratiostr,val) )
	setAspectRatio( val );

    if ( iopar.get(heightanglestr,val) )
	setHeightAngle( val );

    if ( iopar.get(neardistancestr,val) )
	setNearDistance( val );

    if ( iopar.get(fardistancestr,val) )
	setFarDistance( val );

    if ( iopar.get(focaldistancestr,val) )
	setFocalDistance( val );

    return 1;
}


Coord3 Camera::centerFrustrum()
{
    float distancetopoint = ((( farDistance() - nearDistance() ) / 2) +
                                 nearDistance() );
    Coord3 orientation;
    float angle;
    getOrientation(orientation, angle);
    float vectorlength = ( sqrt (( orientation.x * orientation.x ) +
                                 ( orientation.y * orientation.y ) +
                                 ( orientation.z * orientation.z )));
     
    Coord3 currentposition = position();
    Coord3 pos (((( distancetopoint / vectorlength ) * orientation.x ) +
	                 currentposition.x ),
                     ((( distancetopoint / vectorlength ) * orientation.y ) +
	                 currentposition.y ),
                     ((( distancetopoint / vectorlength ) * orientation.z ) +
                         currentposition.z ));
    return pos;
}

float Camera::frustrumRadius()
{

    float distancetopoint = ((( farDistance() - nearDistance() ) / 2) +
	                        nearDistance() );
    float widthangle = ( heightAngle() * aspectRatio() );
    float height = ( farDistance()  *
                   ( tan(( heightAngle() * 180) / M_PI) / 2 ));
    float width  = ( farDistance()  *
                   ( tan(( widthangle * 180) / M_PI ) / 2 ));
    float distance = farDistance() - distancetopoint;
    float radius = ( sqrt (( distance * distance ) +
                           ( width * width ) +
                           ( height * height )));
    return radius;		
}


void Camera::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( iopar, saveids );

    iopar.set( posstr, position() );
    
    SbVec3f axis;
    float angle;
    camera->orientation.getValue( axis, angle );
    iopar.set( orientationstr, axis[0], axis[1], axis[2], angle );

    iopar.set( aspectratiostr, aspectRatio() );
    iopar.set( heightanglestr, heightAngle() );
    iopar.set( neardistancestr, (int)(nearDistance()+.5) );
    iopar.set( fardistancestr, (int)(farDistance()+.5) );
    iopar.set( focaldistancestr, focalDistance() );
}


void Camera::fillPar( IOPar& iopar, const SoCamera* socamera ) const
{
    TypeSet<int> dummy;
    DataObject::fillPar( iopar, dummy );

    SbVec3f pos = socamera->position.getValue();
    iopar.set( posstr, Coord3(pos[0],pos[1],pos[2]) );
    
    SbVec3f axis;
    float angle;
    socamera->orientation.getValue( axis, angle );
    iopar.set( orientationstr, axis[0], axis[1], axis[2], angle );

    SoType t = socamera->getTypeId();
    if ( t.isDerivedFrom( SoPerspectiveCamera::getClassTypeId() ) )
    {
	SoPerspectiveCamera* pc = (SoPerspectiveCamera*)socamera;
	iopar.set( heightanglestr, pc->heightAngle.getValue() );
    }

    iopar.set( aspectratiostr, socamera->aspectRatio.getValue() );
    iopar.set( neardistancestr, mNINT(socamera->nearDistance.getValue()) );
    iopar.set( fardistancestr, mNINT(socamera->farDistance.getValue()) );
    iopar.set( focaldistancestr, socamera->focalDistance.getValue() );
}

}; // namespace visBase
