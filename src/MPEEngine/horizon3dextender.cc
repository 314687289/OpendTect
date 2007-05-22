/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: horizon3dextender.cc,v 1.11 2007-05-22 04:08:58 cvsnanne Exp $";

#include "horizon3dextender.h"

#include "binidsurface.h"
#include "emfault.h"
#include "emhorizon3d.h"
#include "emhorizontaltube.h"
#include "geomtube.h"
#include "survinfo.h"
#include "trackplane.h"
#include "mpeengine.h"


namespace MPE 
{


Horizon3DExtender::Horizon3DExtender( EM::Horizon3D& surface_,
				  const EM::SectionID& sectionid )
    : SectionExtender( sectionid )
    , surface( surface_ )
{}


void Horizon3DExtender::setDirection( const BinIDValue& bdval )
{ direction =  bdval; }


int Horizon3DExtender::maxNrPosInExtArea() const
{ return getExtBoundary().hrg.totalNr(); }


void Horizon3DExtender::preallocExtArea()
{
    const HorSampling hrg = getExtBoundary().hrg;
    surface.geometry().sectionGeometry(sid_)->expandWithUdf(hrg.start,hrg.stop);
}


int Horizon3DExtender::nextStep()
{
    const bool alldirs = direction.binid.inl==0 && direction.binid.crl==0;
    
    TypeSet<BinID> sourcenodes;
    BinID dummy;
    for ( int idx=0; idx<startpos_.size(); idx++ )
    {
	dummy.setSerialized( startpos_[idx] );
	sourcenodes += dummy;
    }

    if ( sourcenodes.size() == 0 )
	return 0;

    const BinID sidehash( direction.binid.crl ? SI().inlStep() : 0,
	    		  direction.binid.inl ? SI().crlStep() : 0 );
    const BinID firstnode = sourcenodes[0];
    const BinID lastnode = sourcenodes[sourcenodes.size()-1];

    const bool extend = 
	engine().trackPlane().getTrackMode()==TrackPlane::Extend;

    bool change = true;

    while ( change )
    {
	change = false;
	for ( int idx=0; idx<sourcenodes.size(); idx++ )
	{
	    const BinID& srcbid = sourcenodes[idx];

	    TypeSet<RowCol> directions;
	    if ( !alldirs )
	    {
		directions += RowCol(direction.binid);
		const bool expand =
		    extend && srcbid!=firstnode && srcbid!=lastnode;

		if ( expand )
		{
		    directions += RowCol(direction.binid+sidehash);
		    directions += RowCol(direction.binid-sidehash);
		}
	    }
	    else
		directions = RowCol::clockWiseSequence();

	    const float depth = surface.getPos(sid_,srcbid.getSerialized()).z;
	    const EM::PosID pid(surface.id(), sid_, srcbid.getSerialized() );
	    for ( int idy=0; idy<directions.size(); idy++ )
	    {
		const EM::PosID neighbor =
		    surface.geometry().getNeighbor(pid, directions[idy] );

		if ( neighbor.sectionID()!=sid_ )
		    continue;

		BinID neighborbinid; 
		neighborbinid.setSerialized( neighbor.subID() );
		if ( !getExtBoundary().hrg.includes(neighborbinid) )
		    continue;

		//If this is a better route to a node that is already
		//added, replace the route with this one

		const int previndex = addedpos_.indexOf( neighbor.subID() );
		if ( previndex!=-1 )
		{
		    const RowCol step( surface.geometry().step() );
		    const RowCol oldsrc( (RowCol(addedpossrc_[previndex]))/step);
		    const RowCol dst( (RowCol(addedpos_[previndex]))/step );
		    const RowCol cursrc( srcbid/step );

		    const int olddist = oldsrc.distanceSq(dst);
		    if ( cursrc.distanceSq( dst )<olddist ) 
		    {
			addedpossrc_[previndex] = srcbid.getSerialized();
			surface.setPos( neighbor, Coord3(0,0,depth), true );
		    }
		    continue;
		}

		//This test must be below the one above since the result
		//of this test will ruin the one above.
		if ( extend && surface.isDefined(neighbor) )
		    continue;


		if ( !isExcludedPos(neighbor.subID()) &&
		     surface.setPos(neighbor, Coord3(0,0,depth), true) )
		{
		    addTarget( neighbor.subID(), srcbid.getSerialized() );
		    change = true;
		}
	    }
	}
    }

    return 0;
}


const CubeSampling& Horizon3DExtender::getExtBoundary() const
{ return extboundary_.isEmpty() ? engine().activeVolume() : extboundary_; }



};  // namespace MPE
