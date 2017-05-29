/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2007
________________________________________________________________________

-*/

#include "latlong.h"
#include "survinfo.h"
#include "separstr.h"
#include "coordsystem.h"
#include <math.h>

const double cAvgEarthRadius = 6367450;



bool LatLong::operator==( const LatLong& ll ) const
{
    return mIsEqual(ll.lat_,lat_,mDefEps) &&
	   mIsEqual(ll.lng_,lng_,mDefEps);
}


Coord LatLong::transform( const LatLong& ll, bool towgs84,
			  const Coords::CoordSystem* coordsys )
{
    if ( !coordsys )
	coordsys = SI().getCoordSystem();

    return coordsys->fromGeographic( ll, towgs84 );
}


LatLong LatLong::transform( const Coord& c, bool towgs84,
			    const Coords::CoordSystem* coordsys )
{
    if ( !coordsys )
	coordsys = SI().getCoordSystem();

    return coordsys->toGeographic( c, towgs84 );
}


const char* LatLong::toString() const
{
    mDeclStaticString( ret );
    ret.set( "[" ).add( lat_ ).add( "," ).add( lng_ ).add( "]" );
    return ret.buf();
}


bool LatLong::fromString( const char* s )
{
    if ( !s || !*s ) return false;

    BufferString str( s );
    char* ptrlat = str.getCStr(); mSkipBlanks( ptrlat );
    if ( *ptrlat == '[' ) ptrlat++;
    char* ptrlng = firstOcc( ptrlat, ',' );
    if ( !ptrlng ) return false;
    *ptrlng++ = '\0';
    if ( !*ptrlng ) return false;
    char* ptrend = firstOcc( ptrlng, ']' );
    if ( ptrend ) *ptrend = '\0';

    lat_ = toDouble( ptrlat );
    lng_ = toDouble( ptrlng );
    return true;
}


void LatLong::getDMS( bool lat, int& d, int& m, float& s ) const
{
    double v = lat ? lat_ : lng_;
    d = (int)v;
    v -= d; v *= 60;
    m = (int)v;
    v -= m; v *= 60;
    s = (float)v;
}


void LatLong::setDMS( bool lat, int d, int m, float s )
{
    double& v = lat ? lat_ : lng_;
    const double one60th = 1. / 60.;
    const double one3600th = one60th * one60th;
    v = d + one60th * m + one3600th * s;
}
