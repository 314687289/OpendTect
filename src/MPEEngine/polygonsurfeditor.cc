/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Yuancheng Liu
 * DATE     : Aug. 2008
___________________________________________________________________

-*/

static const char* rcsID = "$Id: polygonsurfeditor.cc,v 1.5 2008-09-10 21:36:50 cvsyuancheng Exp $";

#include "polygonsurfeditor.h"

#include "empolygonbody.h"
#include "emmanager.h"
#include "polygonsurfaceedit.h"
#include "mpeengine.h"
#include "selector.h"
#include "trigonometry.h"

#include "undo.h"

namespace MPE
{

PolygonBodyEditor::PolygonBodyEditor( EM::PolygonBody& polygonsurf )
    : ObjectEditor(polygonsurf)
{}


ObjectEditor* PolygonBodyEditor::create( EM::EMObject& emobj )
{
    mDynamicCastGet(EM::PolygonBody*,polygonsurf,&emobj);
    return polygonsurf ? new PolygonBodyEditor( *polygonsurf ) : 0;
}


void PolygonBodyEditor::initClass()
{ 
    MPE::EditorFactory().addCreator( create, EM::PolygonBody::typeStr() ); 
}


Geometry::ElementEditor* PolygonBodyEditor::createEditor(
						const EM::SectionID& sid )
{
    const Geometry::Element* ge = emObject().sectionGeometry( sid );
    if ( !ge ) return 0;

    mDynamicCastGet(const Geometry::PolygonSurface*,surface,ge);
    return !surface ? 0 : new Geometry::PolygonSurfEditor( 
	    		  *const_cast<Geometry::PolygonSurface*>(surface) );
}


#define mCompareCoord( crd ) Coord3( crd, crd.z*zfactor )


void PolygonBodyEditor::getInteractionInfo( EM::PosID& nearestpid0,
					    EM::PosID& nearestpid1, 
					    EM::PosID& insertpid,
				    	    const Coord3& mousepos, 
					    float zfactor ) const
{
    nearestpid0 = EM::PosID::udf();
    nearestpid1 = EM::PosID::udf();
    insertpid = EM::PosID::udf();

    int polygon;
    EM::SectionID sid;
    const float mindist = getNearestPolygon( polygon, sid, mousepos, zfactor );
    if ( mIsUdf(mindist) )
    {
	if ( !emObject().nrSections() )
	    return;

	sid = emObject().sectionID( 0 );
	const Geometry::Element* ge = emObject().sectionGeometry( sid );
	if ( !ge ) return;

	mDynamicCastGet(const Geometry::PolygonSurface*,surface,ge);
	if ( !surface ) return;

	const StepInterval<int> rowrange = surface->rowRange();
	if ( !rowrange.isUdf() )
	    return;

	insertpid.setObjectID( emObject().id() );
	insertpid.setSectionID( sid );
	insertpid.setSubID( RowCol(0,0).getSerialized() );
	return;
    }
    
    if ( fabs(mindist)>50 )
    {
	if ( !emObject().nrSections() )
	    return;

	sid = emObject().sectionID( 0 );
	const Geometry::Element* ge = emObject().sectionGeometry( sid );
	if ( !ge ) return;

	mDynamicCastGet(const Geometry::PolygonSurface*,surface,ge);
	if ( !surface ) return;

	const StepInterval<int> rowrange = surface->rowRange();
	if ( rowrange.isUdf() )
	    return;

	insertpid.setObjectID( emObject().id() );
	insertpid.setSectionID( sid );
	const int newpolygon = mindist>0 
	    ? polygon+rowrange.step 
	    : polygon==rowrange.start ? polygon-rowrange.step : polygon;

	insertpid.setSubID( RowCol(newpolygon,0).getSerialized() );
	return;
    }
    
    getPidsOnPolygon( nearestpid0, nearestpid1, insertpid, polygon, sid, 
	    	      mousepos, zfactor );
}


bool PolygonBodyEditor::removeSelection( const Selector<Coord3>& selector )
{
    mDynamicCastGet( EM::PolygonBody*, polygonsurf, &emobject );
    if ( !polygonsurf )
	return false;

    bool change = false;
    for ( int sectidx=polygonsurf->nrSections()-1; sectidx>=0; sectidx--)
    {
	const EM::SectionID currentsid = polygonsurf->sectionID( sectidx );
	const Geometry::Element* ge = polygonsurf->sectionGeometry(currentsid);
	if ( !ge ) continue;

	mDynamicCastGet(const Geometry::PolygonSurface*,surface,ge);
	if ( !surface ) continue;

	const StepInterval<int> rowrange = surface->rowRange();
	if ( rowrange.isUdf() )
	    continue;

	for ( int polygonidx=rowrange.nrSteps(); polygonidx>=0; polygonidx-- )
	{
	    Coord3 avgpos( 0, 0, 0 );
	    int count = 0;
	    const int curpolygon = rowrange.atIndex(polygonidx);
	    const StepInterval<int> colrange = surface->colRange( curpolygon );
	    if ( colrange.isUdf() )
		continue;

	    for ( int knotidx=colrange.nrSteps(); knotidx>=0; knotidx-- )
	    {
		const RowCol rc( curpolygon,colrange.atIndex(knotidx) );
		const Coord3 pos = surface->getKnot( rc );

		if ( !pos.isDefined() || !selector.includes(pos) )
		    continue;

		EM::PolygonBodyGeometry& fg = polygonsurf->geometry();
		const bool res = fg.nrKnots( currentsid,curpolygon)==1
		   ? fg.removePolygon( currentsid, curpolygon, true )
		   : fg.removeKnot( currentsid, rc.getSerialized(), true );

		if ( res ) change = true;
	    }
	}
    }

    if ( change )
    {
	EM::EMM().undo().setUserInteractionEnd(
		EM::EMM().undo().currentEventID() );
    }

    return change;
}


float PolygonBodyEditor::getNearestPolygon( int& polygon, EM::SectionID& sid,
	const Coord3& mousepos, float zfactor ) const
{
    if ( !mousepos.isDefined() )
	return mUdf(float);

    int selsectionidx = -1, selpolygon;
    float mindist;

    for ( int sectionidx=emObject().nrSections()-1; sectionidx>=0; sectionidx--)
    {
	const EM::SectionID currentsid = emObject().sectionID( sectionidx );
	const Geometry::Element* ge = emObject().sectionGeometry( currentsid );
	if ( !ge ) continue;

	mDynamicCastGet(const Geometry::PolygonSurface*,surface,ge);
	if ( !surface ) continue;

	const StepInterval<int> rowrange = surface->rowRange();
	if ( rowrange.isUdf() ) continue;

	for ( int polygonidx=rowrange.nrSteps(); polygonidx>=0; polygonidx-- )
	{
	    Coord3 avgpos( 0, 0, 0 );
	    const int curpolygon = rowrange.atIndex(polygonidx);
	    const StepInterval<int> colrange = surface->colRange( curpolygon );
	    if ( colrange.isUdf() )
		continue;

	    int count = 0;
	    for ( int knotidx=colrange.nrSteps(); knotidx>=0; knotidx-- )
	    {
		const Coord3 pos = surface->getKnot(
			RowCol(curpolygon,colrange.atIndex(knotidx)));

		if ( pos.isDefined() )
		{
		    avgpos += mCompareCoord( pos );
		    count++;
		}
	    }

	    if ( !count ) continue;

	    avgpos /= count;

	    const Plane3 plane( surface->getPolygonNormal(curpolygon),
		    		avgpos, false );
	    const float disttoplane =
		plane.distanceToPoint( mCompareCoord(mousepos), true );

	    if ( selsectionidx==-1 || fabs(disttoplane)<fabs(mindist) )
	    {
		mindist = disttoplane;
		selpolygon = curpolygon;
		selsectionidx = sectionidx;
	    }
	}
    }

    if ( selsectionidx==-1 )
	return mUdf(float);

    sid = emObject().sectionID( selsectionidx );
    polygon = selpolygon;

    return mindist;
}


bool PolygonBodyEditor::setPosition( const EM::PosID& pid, const Coord3& mpos )
{
    if ( !mpos.isDefined() ) return false;

    const Geometry::Element* ge = emObject().sectionGeometry(pid.sectionID());
    mDynamicCastGet( const Geometry::PolygonSurface*, surface, ge );
    if ( !surface ) return false;

    const RowCol rc = pid.getRowCol();
    const StepInterval<int> colrg = surface->colRange( rc.r() );
    if ( colrg.isUdf() ) return false;
	
    const bool addtoundo = changedpids.indexOf(pid) == -1;
    if ( addtoundo )
	changedpids += pid;

    if ( colrg.nrSteps()<3 )
	return emobject.setPos( pid, mpos, addtoundo );
   
    const int previdx = rc.c()==colrg.start ? colrg.stop : rc.c()-colrg.step;
    const int nextidx = rc.c()<colrg.stop ? rc.c()+colrg.step : colrg.start;
    const Coord3 prevpos = surface->getKnot( RowCol(rc.r(), previdx) );
    const Coord3 nextpos = surface->getKnot( RowCol(rc.r(), nextidx) );

    for ( int knot=colrg.start; knot<=colrg.stop; knot += colrg.step )
    {
	const int nextknot = knot<colrg.stop ? knot+colrg.step : colrg.start;
	if ( (knot==previdx && nextknot==rc.c()) ||
	     (knot==rc.c() && nextknot==nextidx) )
	    continue;

	const Coord3 v0 = surface->getKnot( RowCol(rc.r(), knot) );
	const Coord3 v1 = surface->getKnot( RowCol(rc.r(), nextknot) );
	if ( (!sameSide3D(mpos, prevpos, v0, v1, 1e-3) && 
	     !sameSide3D(v0, v1, mpos, prevpos, 1e-3)) ||
	     (!sameSide3D(mpos, nextpos, v0, v1, 1e-3) &&
	     !sameSide3D(v0, v1, mpos, nextpos, 1e-3)) )
	    return false;
    }

    return emobject.setPos( pid, mpos, addtoundo );
}


void PolygonBodyEditor::getPidsOnPolygon(  EM::PosID& nearestpid0,
	EM::PosID& nearestpid1, EM::PosID& insertpid, int polygon,
	const EM::SectionID& sid, const Coord3& mousepos, float zfactor ) const
{
    nearestpid0 = EM::PosID::udf();
    nearestpid1 = EM::PosID::udf();
    insertpid = EM::PosID::udf();
    if ( !mousepos.isDefined() ) return;

    const Geometry::Element* ge = emObject().sectionGeometry( sid );
    mDynamicCastGet(const Geometry::PolygonSurface*,surface,ge);
    if ( !surface ) return;

    const StepInterval<int> colrange = surface->colRange( polygon );
    if ( colrange.isUdf() ) return;
   
    const Coord3 mp = mCompareCoord(mousepos);
    TypeSet<int> knots;
    int nearknotidx = -1;
    Coord3 nearpos;
    float minsqptdist;
    for ( int knotidx=0; knotidx<colrange.nrSteps()+1; knotidx++ )
    {
	const Coord3 pt = 
	    surface->getKnot( RowCol(polygon,colrange.atIndex(knotidx)) );
	if ( !pt.isDefined() )
	    continue;

	const float sqdist =mCompareCoord(pt).sqDistTo(mp);
	if ( mIsZero(sqdist, 1e-4) ) //mousepos is duplicated.
	    return;

	 if ( nearknotidx==-1 || sqdist<minsqptdist )
	 {
	     minsqptdist = sqdist;
	     nearknotidx = knots.size();
	     nearpos = mCompareCoord( pt );
	 }
	 	 
	 knots += colrange.atIndex( knotidx );
    }

    if ( nearknotidx==-1 )
	return;

    nearestpid0.setObjectID( emObject().id() );
    nearestpid0.setSectionID( sid );
    nearestpid0.setSubID( RowCol(polygon,knots[nearknotidx]).getSerialized() );
    if ( knots.size()<=2 )
    {
	insertpid = nearestpid0;
	insertpid.setSubID( RowCol(polygon,knots.size()).getSerialized() );
	return;
    }

    double minsqedgedist = -1;
    int nearedgeidx;
    Coord3 v0, v1;
    for ( int knotidx=0; knotidx<knots.size(); knotidx++ )
    {
	const int col = knots[knotidx];
	const Coord3 p0 = mCompareCoord(surface->getKnot(RowCol(polygon,col)));
	const Coord3 p1 = mCompareCoord( surface->getKnot( RowCol(polygon,
			knots [ knotidx<knots.size()-1 ? knotidx+1 : 0 ]) ) );

	const double t = (mp-p0).dot(p1-p0)/(p1-p0).sqAbs();
	if ( t<0 || t>1 )
	    continue;

	const double sqdist = mp.sqDistTo(p0+t*(p1-p0));
	if ( minsqedgedist==-1 || sqdist<minsqedgedist )
	{
	    minsqedgedist = sqdist;
	    nearedgeidx = knotidx;
	    v0 = p0;
	    v1 = p1;
	}
    }

    bool usenearedge = false;
    if ( minsqedgedist!=-1 )
    {
	if ( nearknotidx==nearedgeidx ||
	     nearknotidx==(nearknotidx<knots.size()-1 ? nearknotidx+1 : 0) ||  	
	     ((v1-nearpos).cross(v0-nearpos)).dot((v1-mp).cross(v0-mp))<0  ||
	     minsqedgedist<minsqptdist )
	    usenearedge = true;
    }

    if ( usenearedge ) //use nearedgeidx only
    {
	if ( nearedgeidx<knots.size()-1 )
	{
	    nearestpid0.setSubID( 
		    RowCol(polygon,knots[nearedgeidx]).getSerialized() );
	    nearestpid1 = nearestpid0;
	    nearestpid1.setSubID( 
		    RowCol(polygon,knots[nearedgeidx+1]).getSerialized() );
	
	    insertpid = nearestpid1;
	}
	else
	{
	    insertpid = nearestpid0;
	    const int nextcol = knots[nearedgeidx]+colrange.step;
	    insertpid.setSubID( RowCol(polygon,nextcol).getSerialized() );
	}
	    
	return;
    }
    else  //use nearknotidx only
    {
	const Coord3 prevpos = mCompareCoord(surface->getKnot( RowCol(polygon,
		    knots[nearknotidx ? nearknotidx-1 : knots.size()-1]) ) );
	const Coord3 nextpos = mCompareCoord(surface->getKnot( RowCol(polygon,
		    knots[nearknotidx<knots.size()-1 ? nearknotidx+1 : 0]) ));

	if ( sameSide3D(mp, prevpos, nearpos, nextpos, 1e-3) ) //insert prevpt
	{
	    if ( nearknotidx )
	    {
		nearestpid1 = nearestpid0;
		nearestpid1.setSubID( 
			RowCol(polygon,knots[nearknotidx-1]).getSerialized() );
		insertpid = nearestpid0;
	    }
	    else
	    {
		insertpid = nearestpid0;
		const int insertcol = knots[nearknotidx]-colrange.step;
		insertpid.setSubID(RowCol(polygon,insertcol).getSerialized());
	    }
	}
	else 
	{
	    if ( nearknotidx<knots.size()-1 )
	    {
		nearestpid1 = nearestpid0;
		nearestpid1.setSubID( 
			RowCol(polygon,knots[nearknotidx+1]).getSerialized() );
		insertpid = nearestpid1;
	    }
	    else
	    {
		insertpid = nearestpid0;
		const int insertcol = knots[nearknotidx]+colrange.step;
		insertpid.setSubID(RowCol(polygon,insertcol).getSerialized());
	    }
	}
    }
}


};  // namespace MPE
