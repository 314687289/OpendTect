/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2006
___________________________________________________________________

-*/

static const char* rcsID = "$Id: horizon2dextender.cc,v 1.3 2007-01-31 11:56:10 cvsjaap Exp $";

#include "horizon2dextender.h"

#include "emhorizon2d.h"
#include "survinfo.h"

namespace MPE 
{

Horizon2DExtender::Horizon2DExtender( EM::Horizon2D& hor,
				      const EM::SectionID& sid )
    : SectionExtender( sid )
    , surface_( hor )
    , anglethreshold_( 0.5 )
{}


void Horizon2DExtender::setAngleThreshold( float rad )
{ anglethreshold_ = cos( rad ); }


float Horizon2DExtender::getAngleThreshold() const
{ return acos(anglethreshold_); }


void Horizon2DExtender::setDirection( const BinIDValue& dir )
{
    direction_ = dir;
    xydirection_ = SI().transform( BinID(0,0) ) - SI().transform( dir.binid );
    const double abs = xydirection_.abs();
    alldirs_ = mIsZero( abs, 1e-3 );
    if ( !alldirs_ )
	xydirection_ /= abs;
}


int Horizon2DExtender::nextStep()
{
    for ( int idx=0; idx<startpos_.size(); idx++ )
    {
	addNeighbor( false, startpos_[idx] );
	addNeighbor( true, startpos_[idx] );
    }

    return 0;
}


void Horizon2DExtender::addNeighbor( bool upwards, const RowCol& sourcerc )
{
    const StepInterval<int> colrange =
			    surface_.geometry().colRange( sid_, sourcerc.row );
    EM::SubID neighborsubid;
    Coord3 neighborpos;
    RowCol neighborrc = sourcerc;
    const CubeSampling& boundary = getExtBoundary();

    do 
    {
	neighborrc += RowCol( 0, upwards ? colrange.step : -colrange.step );
	if ( !colrange.includes(neighborrc.col) )
	    return;
	if ( !boundary.isEmpty() && !boundary.hrg.includes(BinID(neighborrc)) )
	    return;
	neighborsubid = neighborrc.getSerialized();
	neighborpos = surface_.getPos( sid_, neighborsubid );
    }
    while ( !Coord(neighborpos).isDefined() );

    if ( neighborpos.isDefined() )
	return;

    const Coord3 sourcepos = surface_.getPos( sid_,sourcerc.getSerialized() );

    if ( !alldirs_ )
    {
	const Coord dir = neighborpos - sourcepos;
	const double dirabs = dir.abs();
	if ( !mIsZero(dirabs,1e-3) )
	{
	    const Coord normdir = dir/dirabs;
	    const double cosangle = normdir.dot(xydirection_);
	    if ( cosangle<anglethreshold_ )
		return;
	}
    }

    Coord3 refpos = surface_.getPos( sid_, neighborsubid );
    refpos.z = sourcepos.z;
    surface_.setPos( sid_, neighborsubid, refpos, true );

    addTarget( neighborsubid, sourcerc.getSerialized() );
}


};  // namespace MPE
