/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: faulteditor.cc,v 1.5 2008-05-15 20:26:12 cvskris Exp $";

#include "faulteditor.h"

#include "emfault.h"
#include "faultsticksurfedit.h"
#include "mpeengine.h"
#include "trigonometry.h"

namespace MPE
{

FaultEditor::FaultEditor( EM::Fault& fault )
    : ObjectEditor(fault)
{}


ObjectEditor* FaultEditor::create( EM::EMObject& emobj )
{
    mDynamicCastGet(EM::Fault*,fault,&emobj);
    if ( !fault ) return 0;
    return new FaultEditor(*fault);
}


void FaultEditor::initClass()
{ MPE::EditorFactory().addCreator( create, EM::Fault::typeStr() ); }


Geometry::ElementEditor* FaultEditor::createEditor( const EM::SectionID& sid )
{
    const Geometry::Element* ge = emObject().sectionGeometry( sid );
    if ( !ge ) return 0;

    mDynamicCastGet(const Geometry::FaultStickSurface*,surface,ge);
    if ( !surface ) return 0;
    
    return new Geometry::FaultStickSurfEditor(
			*const_cast<Geometry::FaultStickSurface*>(surface) );
}


#define mCompareCoord( crd ) Coord3( crd, crd.z*zfactor )


void FaultEditor::getInteractionInfo( EM::PosID& nearestpid0,
	EM::PosID& nearestpid1, EM::PosID& insertpid,
	const Coord3& mousepos, float zfactor ) const
{
    nearestpid0 = EM::PosID::udf();
    nearestpid1 = EM::PosID::udf();
    insertpid = EM::PosID::udf();

    int stick;
    EM::SectionID sid;
    const float mindist = getNearestStick( stick, sid, mousepos, zfactor );
    if ( mIsUdf(mindist) )
    {
	if ( !emObject().nrSections() )
	    return;

	sid = emObject().sectionID( 0 );

	insertpid.setObjectID( emObject().id() );
	insertpid.setSectionID( sid );
	insertpid.setSubID( RowCol( 0, 0 ).getSerialized() );
	return;
    }

    if ( fabs(mindist)>50 )
    {
	if ( !emObject().nrSections() )
	    return;

	sid = emObject().sectionID( 0 );
	const Geometry::Element* ge = emObject().sectionGeometry( sid );
	if ( !ge ) return;

	mDynamicCastGet(const Geometry::FaultStickSurface*,surface,ge);
	if ( !surface ) return;

	const StepInterval<int> rowrange = surface->rowRange();
	if ( rowrange.isUdf() )
	    return;

	insertpid.setObjectID( emObject().id() );
	insertpid.setSectionID( sid );

	const int newstick = mindist>0 ? stick : stick-rowrange.step;
	insertpid.setSubID( RowCol( newstick, 0 ).getSerialized() );
	return;
    }

    getPidsOnStick( nearestpid0, nearestpid1, insertpid, stick, sid, mousepos,
	            zfactor );
}


float FaultEditor::getNearestStick( int& stick, EM::SectionID& sid,
				   const Coord3& mousepos, float zfactor ) const
{
    if ( !mousepos.isDefined() )
	return mUdf(float);

    int selsectionidx = -1, selstick;
    float mindist;

    for ( int sectionidx=emObject().nrSections()-1; sectionidx>=0; sectionidx--)
    {
	const EM::SectionID currentsid = emObject().sectionID( sectionidx );
	const Geometry::Element* ge = emObject().sectionGeometry( currentsid );
	if ( !ge ) continue;

	mDynamicCastGet(const Geometry::FaultStickSurface*,surface,ge);
	if ( !surface ) continue;

	const StepInterval<int> rowrange = surface->rowRange();
	if ( rowrange.isUdf() )
	    continue;

	for ( int stickidx=rowrange.nrSteps(); stickidx>=0; stickidx-- )
	{
	    Coord3 avgpos( 0, 0, 0 );
	    int count = 0;
	    const StepInterval<int> colrange = surface->colRange( rowrange.atIndex(stickidx) );
	    if ( colrange.isUdf() )
		continue;

	    for ( int knotidx=colrange.nrSteps(); knotidx>=0; knotidx-- )
	    {
		const Coord3 pos = surface->getKnot(
		  RowCol(rowrange.atIndex(stickidx),colrange.atIndex(knotidx)));

		if ( pos.isDefined() )
		{
		    avgpos += mCompareCoord( pos );
		    count++;
		}
	    }

	    if ( !count )
		continue;

	    avgpos /= count;

	    const Plane3 plane( surface->getEditPlaneNormal(stickidx),
		    		avgpos, false );
	    const float disttoplane =
		plane.distanceToPoint( mCompareCoord(mousepos), true );

	    if ( selsectionidx==-1 || fabs(disttoplane)<fabs(mindist) )
	    {
		mindist = disttoplane;
		selstick = rowrange.atIndex( stickidx );
		selsectionidx = sectionidx;
	    }
	}
    }

    if ( selsectionidx==-1 )
	return mUdf(float);

    sid = emObject().sectionID( selsectionidx );
    stick =  selstick;

    return mindist;
}


void FaultEditor::getPidsOnStick(  EM::PosID& nearestpid0,
	EM::PosID& nearestpid1, EM::PosID& insertpid, int stick,
	const EM::SectionID& sid, const Coord3& mousepos, float zfactor ) const
{
    nearestpid0 = EM::PosID::udf();
    nearestpid1 = EM::PosID::udf();
    insertpid = EM::PosID::udf();

    if ( !mousepos.isDefined() )
	return;

    const Geometry::Element* ge = emObject().sectionGeometry( sid );
    mDynamicCastGet(const Geometry::FaultStickSurface*,surface,ge);

    const StepInterval<int> colrange = surface->colRange( stick );
    const int nrknots = colrange.nrSteps()+1;

    TypeSet<int> definedknots;
    int nearestknotidx = -1;
    float minsqdist;
    for ( int knotidx=0; knotidx<nrknots; knotidx++ )
    {
	const RowCol rc( stick, colrange.atIndex(knotidx));
	const Coord3 pos = surface->getKnot( rc );

	if ( !pos.isDefined() )
	    continue;


	const float sqdist =
	    mCompareCoord(pos).sqDistTo( mCompareCoord(mousepos) );
	if ( nearestknotidx==-1 || sqdist<minsqdist )
	{
	    minsqdist = sqdist;
	    nearestknotidx = definedknots.size();
	}

	definedknots += colrange.atIndex( knotidx );
    }

    if ( nearestknotidx==-1 )
	return;

    nearestpid0.setObjectID( emObject().id() );
    nearestpid0.setSectionID( sid );
    nearestpid0.setSubID(
	RowCol(stick, definedknots[nearestknotidx]).getSerialized() );

    if ( definedknots.size()<=1 )
    {
	const int defcol = definedknots[nearestknotidx];
	const Coord3 pos = surface->getKnot( RowCol(stick, defcol) );
	const int insertcol = defcol + (surface->areSticksVertical() 
	    ? mousepos.z>pos.z ? 1 : -1
	    : mousepos.coord()>pos.coord() ? 1 : -1) * colrange.step;

	insertpid.setObjectID( emObject().id() );
	insertpid.setSectionID( sid );
	insertpid.setSubID( RowCol( stick, insertcol ).getSerialized() );
	return;
    }

    const Coord3 pos =
	surface->getKnot( RowCol(stick,definedknots[nearestknotidx]) );

    Coord3 nextpos = pos, prevpos = pos;

    if ( nearestknotidx )
	prevpos =
	    surface->getKnot( RowCol(stick, definedknots[nearestknotidx-1] ) );

    if ( nearestknotidx<definedknots.size()-1 )
	nextpos =
	    surface->getKnot( RowCol(stick, definedknots[nearestknotidx+1] ) );

    Coord3 v0 = nextpos-prevpos;
    Coord3 v1 = mousepos-pos;

    const float dot = mCompareCoord(v0).dot( mCompareCoord(v1) );
    if ( dot<0 ) //Previous
    {
	if ( nearestknotidx )
	{
	    nearestpid1 = nearestpid0;
	    nearestpid1.setSubID(
		RowCol(stick,definedknots[nearestknotidx-1]).getSerialized() );
	    insertpid = nearestpid0;
	}
	else
	{
	    insertpid = nearestpid0;
	    const int insertcol = definedknots[nearestknotidx]-colrange.step;
	    insertpid.setSubID( RowCol(stick,insertcol).getSerialized() );
	}
    }
    else //Next
    {
	if ( nearestknotidx<definedknots.size()-1 )
	{
	    nearestpid1 = nearestpid0;
	    nearestpid1.setSubID(
		RowCol(stick,definedknots[nearestknotidx+1]).getSerialized() );
	    insertpid = nearestpid1;
	}
	else
	{
	    insertpid = nearestpid0;
	    const int insertcol = definedknots[nearestknotidx]+colrange.step;
	    insertpid.setSubID( RowCol(stick,insertcol).getSerialized() );
	}
    }
}


};  // namespace MPE
