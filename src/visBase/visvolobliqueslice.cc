/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: visvolobliqueslice.cc,v 1.1 2007-01-05 21:57:25 cvskris Exp $";


#include "visvolobliqueslice.h"

#include "visdataman.h"
#include "vistexture3.h"
#include "visselman.h"
#include "iopar.h"

#include "VolumeViz/nodes/SoObliqueSlice.h"

using namespace visBase;

mCreateFactoryEntry( ObliqueSlice );


ObliqueSlice::ObliqueSlice()
    : slice_( new SoObliqueSlice )
{
    addChild( slice_ );
}


ObliqueSlice::~ObliqueSlice()
{ }


Coord3 ObliqueSlice::getPosOnPlane() const
{ return getNormal()*slice_->plane.getValue().getDistanceFromOrigin(); } 


Coord3 ObliqueSlice::getNormal() const
{
    const SbVec3f normal = slice_->plane.getValue().getNormal();
    return Coord3( normal[0], normal[1], normal[2] ).normalize();
}


void ObliqueSlice::set( const Coord3& normal, const Coord3& pos )
{
    SbPlane plane( SbVec3f(normal.x,normal.y,normal.z ),
	    	   SbVec3f(pos.x,pos.y,pos.z) );
    slice_->plane.setValue(plane);
}
