/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-6-1996
-*/

static const char* rcsID = "$Id: positionlist.cc,v 1.1 2008-02-15 20:28:40 cvskris Exp $";

#include "positionlist.h"


Coord2ListImpl::Coord2ListImpl()
{}


int Coord2ListImpl::nextID( int previd ) const
{
    const int sz = points_.size();
    int res = previd + 1;
    while ( res<sz )
    {
	if ( removedids_.indexOf(res)==-1 )
	    return res;

	res++;
    }

    return -1;
}


Coord Coord2ListImpl::get( int id ) const
{
    if ( id<0 || id>=points_.size() || removedids_.indexOf( id )!=-1 )
	return Coord::udf();
    else
	return points_[id];
}


void Coord2ListImpl::set( int id, const Coord& co )
{
    for ( int idx=points_.size(); idx<=id; idx++ )
    {
	removedids_ += idx;
	points_ += Coord::udf();
    }

    removedids_ -= id;
    points_[id] = co;
}


int Coord2ListImpl::add( const Coord& co )
{
    const int nrremoved = removedids_.size();
    if ( nrremoved )
    {
	const int res = removedids_[nrremoved-1];
	removedids_.remove( nrremoved-1 );
	points_[res] = co;
	return res;
    }

    points_ += co;
    return points_.size()-1;
}


void Coord2ListImpl::remove( int id )
{
    removedids_ += id;
}
