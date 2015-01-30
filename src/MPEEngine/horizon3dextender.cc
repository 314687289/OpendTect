/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "horizon3dextender.h"

#include "binidsurface.h"
#include "emfault.h"
#include "emhorizon3d.h"
#include "horizon3dtracker.h"
#include "survinfo.h"
#include "mpeengine.h"


namespace MPE
{


void Horizon3DExtender::initClass()
{
    ExtenderFactory().addCreator( create, Horizon3DTracker::keyword() );
}


SectionExtender* Horizon3DExtender::create( EM::EMObject* emobj,
						const EM::SectionID& sid )
{
    mDynamicCastGet(EM::Horizon3D*,hor,emobj)
    return emobj && !hor ? 0 : new Horizon3DExtender( *hor, sid );
}


Horizon3DExtender::Horizon3DExtender( EM::Horizon3D& surface_,
					      const EM::SectionID& sectionid )
   : BaseHorizon3DExtender( surface_, sectionid )
{
}


BaseHorizon3DExtender::BaseHorizon3DExtender( EM::Horizon3D& surface_,
				  const EM::SectionID& sectionid )
    : SectionExtender( sectionid )
    , surface( surface_ )
{}


/*SectionExtender* Horizon3DExtender::create( EM::EMObject* emobj,
					    const EM::SectionID& sid )
{
    mDynamicCastGet(EM::Horizon3D*,hor,emobj)
    return emobj && !hor ? 0 : new Horizon3DExtender( *hor, sid );
}


void Horizon3DExtender::initClass()
{
    ExtenderFactory().addCreator( create, Horizon3DTracker::keyword() );
}*/


void BaseHorizon3DExtender::setDirection( const BinIDValue& bdval )
{ direction =  bdval; }


int BaseHorizon3DExtender::maxNrPosInExtArea() const
{ return mCast( int, getExtBoundary().hrg.totalNr() ); }


void BaseHorizon3DExtender::preallocExtArea()
{
    const TrcKeySampling hrg = getExtBoundary().hrg;
    surface.geometry().sectionGeometry(sid_)->expandWithUdf(hrg.start,hrg.stop);
}


int BaseHorizon3DExtender::nextStep()
{
    const bool alldirs = direction.inl()==0 && direction.crl()==0;

    TypeSet<BinID> sourcenodes;

    for ( int idx=0; idx<startpos_.size(); idx++ )
    {
	BinID dummy = BinID::fromInt64( startpos_[idx] );
	sourcenodes += dummy;
    }

    if ( sourcenodes.size() == 0 )
	return 0;

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
		directions += RowCol(direction);
		directions += RowCol( direction.inl()*-1,
				      direction.crl()*-1 );
	    }
	    else
		directions = RowCol::clockWiseSequence();

	    // const float depth = surface.getPos(sid_,srcbid.toInt64()).z;
	    const EM::PosID pid(surface.id(), sid_, srcbid.toInt64() );
	    for ( int idy=0; idy<directions.size(); idy++ )
	    {
		const EM::PosID neighbor =
		    surface.geometry().getNeighbor(pid, directions[idy] );

		if ( neighbor.sectionID()!=sid_ )
		    continue;

		BinID neighbbid = BinID::fromInt64( neighbor.subID() );
		if ( !getExtBoundary().hrg.includes(neighbbid) )
		    continue;

		//If this is a better route to a node that is already
		//added, replace the route with this one

		const int previndex = addedpos_.indexOf( neighbor.subID() );
		if ( previndex!=-1 )
		{
		    const RowCol step( surface.geometry().step() );
		    const od_int64 serc = addedpossrc_[previndex];
		    const RowCol oldsrc( (RowCol::fromInt64(serc))/step );
		    const RowCol dst( (RowCol::fromInt64(serc))/step );
		    const RowCol cursrc( srcbid/step );

		    const int olddist = (int)oldsrc.sqDistTo(dst);
		    if ( cursrc.sqDistTo( dst )<olddist )
		    {
			addedpossrc_[previndex] = srcbid.toInt64();
		//surface.setPos( neighbor, Coord3(0,0,depth), setundo_ );
			surface.setPos( neighbor,
					Coord3(0,0,getDepth(srcbid,neighbbid)),
					setundo_ );
		    }
		    continue;
		}

		if ( surface.isDefined(neighbor) )
		    continue;

		if ( !isExcludedPos(neighbor.subID()) &&
		     //surface.setPos(neighbor, Coord3(0,0,depth), setundo_) )
		     surface.setPos(neighbor,
			     	    Coord3(0,0,getDepth(srcbid,neighbbid)),
				    setundo_) )
		{
		    addTarget( neighbor.subID(), srcbid.toInt64() );
		    change = true;
		}
	    }
	}
    }

    return 0;
}


float BaseHorizon3DExtender::getDepth( const BinID& srcbid,
					 const BinID& destbid ) const
{
    return (float) surface.getPos( sid_, srcbid.toInt64() ).z;
}


const TrcKeyZSampling& BaseHorizon3DExtender::getExtBoundary() const
{ return extboundary_.isEmpty() ? engine().activeVolume() : extboundary_; }

} // namespace MPE
