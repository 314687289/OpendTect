/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jul 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: viscamerainfo.cc,v 1.7 2009-07-22 16:01:44 cvsbert Exp $";

#include "viscamerainfo.h"
#include "SoCameraInfo.h"

mCreateFactoryEntry( visBase::CameraInfo );

namespace visBase
{

CameraInfo::CameraInfo()
    : camerainfo( new SoCameraInfo )
{
    camerainfo->ref();
}


CameraInfo::~CameraInfo()
{
    camerainfo->unref();
}


void CameraInfo::setInteractive(bool yn)
{
    if ( isInteractive()==yn ) return;
    camerainfo->cameraInfo = camerainfo->cameraInfo.getValue() +
			     (yn ? SoCameraInfo::INTERACTIVE
			        : -SoCameraInfo::INTERACTIVE);
}


bool CameraInfo::isInteractive() const
{
    return (camerainfo->cameraInfo.getValue() & SoCameraInfo::INTERACTIVE);
}


void CameraInfo::setMoving(bool yn)
{
    if ( isMoving()==yn ) return;
    camerainfo->cameraInfo = camerainfo->cameraInfo.getValue() +
			 (yn ? SoCameraInfo::MOVING : -SoCameraInfo::MOVING);
}


bool CameraInfo::isMoving() const
{
    return (camerainfo->cameraInfo.getValue() & SoCameraInfo::MOVING);
}


SoNode* CameraInfo::getInventorNode()
{
    return camerainfo;
}

}; // namespace visBase
