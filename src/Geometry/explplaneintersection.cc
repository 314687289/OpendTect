/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : J.C. Glas
 * DATE     : October 2007
-*/

static const char* rcsID = "$Id: explplaneintersection.cc,v 1.1 2008-05-30 03:49:10 cvskris Exp $";

#include "explplaneintersection.h"

#include "linerectangleclipper.h"
#include "multidimstorage.h"
#include "polygon.h"
#include "positionlist.h"
#include "survinfo.h"
#include "task.h"
#include "trigonometry.h"

namespace Geometry {


struct ExplPlaneIntersectionExtractorPlane
{
    ExplPlaneIntersectionExtractorPlane( const Coord3& normal,
					 const TypeSet<Coord3>& pointsonplane )
	: planecoordsys_( normal, pointsonplane[0], pointsonplane[1] )
    {
	Interval<double> xrg, yrg;
	for ( int idx=0; idx<pointsonplane.size(); idx++ )
	{
	    const Coord pt = planecoordsys_.transform(pointsonplane[idx],false);
	    polygon_.add( pt );
	    if ( !idx )
	    {
		xrg.start = xrg.stop = pt.x;
		yrg.start = yrg.stop = pt.y;
		continue;
	    }

	    xrg.start = mMIN(xrg.start,pt.x); xrg.stop = mMAX(xrg.stop,pt.x);
	    yrg.start = mMIN(yrg.start,pt.y); yrg.stop = mMAX(yrg.stop,pt.y);
	}

	bbox_.setTopLeft( Coord( xrg.start, yrg.start) );
	bbox_.setBottomRight( Coord( xrg.stop, yrg.stop) );
    }

    void cutLine( const Line3& line, double& t0, double& t1,
	          bool& t0change, bool& t1change )
    {
	LineRectangleClipper<double> clipper( bbox_ );
	clipper.setLine( planecoordsys_.transform( line.getPoint( t0 ), false ),
		         planecoordsys_.transform( line.getPoint( t1 ), false));

	if ( clipper.isStartChanged() )
	{
	    t0 = line.closestPoint(
		    planecoordsys_.transform(clipper.getStart()) );
	    t0change = true;
	}
	else
	    t0change = false;

	if ( clipper.isStopChanged() )
	{
	    t1 = line.closestPoint(
		    planecoordsys_.transform(clipper.getStop()) );
	    t1change = true;
	}
	else
	    t1change = false;
    }


    bool isInside( const Line3& line, double t ) const
    {
	return polygon_.isInside(
		planecoordsys_.transform( line.getPoint(t), false ), true, 0 );
    }

    ODPolygon<double>		polygon_;
    Plane3CoordSystem		planecoordsys_;
    Geom::Rectangle<double>	bbox_;
};



class ExplPlaneIntersectionExtractor : public ParallelTask
{
public:
ExplPlaneIntersectionExtractor( ExplPlaneIntersection& efss )
    : explsurf_( efss )
    , intersectioncoordids_( 3, 1 )
{
    planes_.allowNull( true );

    for ( int idx=0; idx<efss.nrPlanes(); idx++ )
    {
	const int planeid = explsurf_.planeID( idx );
	const TypeSet<Coord3>& pointsonplane = explsurf_.planePolygon(planeid);
	if ( pointsonplane.size()<3 )
	{
	    planes_ += 0;
	}
	else
	{
	    planes_ += new ExplPlaneIntersectionExtractorPlane(
		    explsurf_.planeNormal( planeid ), pointsonplane );
	}
    }

    intersectioncoordids_.allowDuplicates( false );
    output_ = const_cast<IndexedGeometry*>( explsurf_.getGeometry()[0] );
    output_->removeAll();
}


~ExplPlaneIntersectionExtractor()
{
    deepErase( planes_ );
}

    
int totalNr() const
{
    return explsurf_.getShape()->getGeometry().size();
}


#define mStick	0
#define mKnot	1

bool doWork( int start, int stop, int )
{
    for ( int idx=start; idx<=stop; idx++, reportNrDone() )
    {
	const IndexedGeometry* inputgeom =
	    explsurf_.getShape()->getGeometry()[idx];

	Threads::MutexLocker inputlock( inputgeom->lock_ );

	if ( inputgeom->type_==IndexedGeometry::Triangles )
	{
	    for ( int idy=0; idy<inputgeom->coordindices_.size()-2; idy+=3 )
	    {
		intersectTriangle( inputgeom->coordindices_[idy],
				   inputgeom->coordindices_[idy+1],
				   inputgeom->coordindices_[idy+2] );
	    }
	}
	else if ( inputgeom->type_==IndexedGeometry::TriangleStrip )
	{
	    for ( int idy=0; idy<inputgeom->coordindices_.size(); idy++ )
	    {
		if ( inputgeom->coordindices_[idy]==-1 )
		{
		    idy += 2;
		    continue;
		}

		if ( idy<2 )
		    continue;

		intersectTriangle( inputgeom->coordindices_[idy-2],
				   inputgeom->coordindices_[idy-1],
				   inputgeom->coordindices_[idy] );
	    }
	}
	else if ( inputgeom->type_==IndexedGeometry::TriangleFan )
	{
	    int center = 0;
	    for ( int idy=0; idy<inputgeom->coordindices_.size(); idy++ )
	    {
		if ( inputgeom->coordindices_[idy]==-1 )
		{
		    center = idy+1;
		    idy += 2;
		    continue;
		}

		if ( idy<2 )
		    continue;

		intersectTriangle( inputgeom->coordindices_[center],
				   inputgeom->coordindices_[idy-1],
				   inputgeom->coordindices_[idy] );
	    }
	}
    }

    return true;
}


void intersectTriangle( int lci0, int lci1, int lci2 )
{
    RefMan<const Coord3List> coordlist = explsurf_.getShape()->coordList();

    const float zscale = explsurf_.getZScale();

    Coord3 c0 = coordlist->get( lci0 ); c0.z *= zscale;
    Coord3 c1 = coordlist->get( lci1 ); c1.z *= zscale;
    Coord3 c2 = coordlist->get( lci2 ); c2.z *= zscale;

    const Coord3 trianglenormal = (c1-c0).cross(c2-c0);

    const Plane3 triangleplane( trianglenormal, c0, false );

    for ( int planeidx=explsurf_.nrPlanes()-1; planeidx>=0; planeidx-- )
    {
	const int planeid = explsurf_.planeID( planeidx );
	const Coord3 ptonplane = explsurf_.planePolygon( planeid )[0];
	const Plane3 plane( explsurf_.planeNormal( planeid ), ptonplane, false);

	Line3 intersectionline;
	if ( !triangleplane.intersectWith( plane, intersectionline ) )
	    continue;

	double t[3];
	int startidx=-1, stopidx=-1;

	double testt;
	const bool t0ok = getNearestT(c0,c1,intersectionline,t[0],testt) &&
	    		  testt>=0 && testt<=1;
	const bool t1ok = getNearestT(c1,c2,intersectionline,t[1],testt) &&
	    		  testt>=0 && testt<=1;

	if ( !t0ok && !t1ok ) 
	    continue;

	if ( t0ok && t1ok )
	{
	    startidx = 0;
	    stopidx = 1;
	}
	else
	{
	    const bool t2ok = getNearestT(c2,c0,intersectionline,t[2],testt) &&
			      testt>=0 && testt<=1;
	    if ( !t2ok )
	    	pErrMsg( "hmm" );
	    else
	    {
		startidx = t0ok ? 0 : 1;
		stopidx = 2;
	    }
	}

	if ( startidx==-1 )
	    continue;

	bool startcut = false, stopcut = false;
	if ( planes_[planeidx] )
	{
	    char sum = planes_[planeidx]->isInside(intersectionline,t[startidx])
		     + planes_[planeidx]->isInside(intersectionline,t[stopidx]);
	    if ( sum==0 )
		continue;
	    if ( sum!=2 )
	    {
		planes_[planeidx]->cutLine( intersectionline, t[startidx],
			t[stopidx], startcut, stopcut );
	    }
	}

#define mSetArrPos( tidx ) \
    int arraypos[3]; \
    arraypos[0] = planeid; \
    if ( tidx==0 ) \
    { arraypos[1]=mMIN(lci0,lci1); arraypos[2]=mMAX(lci0,lci1); } \
    else if ( tidx==1 ) \
    { arraypos[1]=mMIN(lci1,lci2); arraypos[2]=mMAX(lci1,lci2); } \
    else { arraypos[1]=mMIN(lci2,lci0); arraypos[2]=mMAX(lci2,lci0); } 

	int ci0;
	if ( !startcut )
	{
	    mSetArrPos( startidx )
	    ci0 = getCoordIndex( arraypos, intersectionline, t[startidx],
		    		 *explsurf_.coordList() );
	}
	else
	{
	    Coord3 point = intersectionline.getPoint( t[startidx] );
	    point.z /= zscale;
	    ci0 = explsurf_.coordList()->add( point );
	}

	int ci1;
	if ( !stopcut )
	{
	    mSetArrPos( stopidx )
	    ci1 = getCoordIndex( arraypos, intersectionline, t[stopidx],
		    		 *explsurf_.coordList() );
	}
	else
	{
	    Coord3 point = intersectionline.getPoint( t[stopidx] );
	    point.z /= zscale;
	    ci1 = explsurf_.coordList()->add( point );
	}

	Threads::MutexLocker reslock( output_->lock_ );

	const int ci0idx = output_->coordindices_.indexOf( ci0 );
	if ( ci0idx==-1 )
	{
	    const int ci1idx = output_->coordindices_.indexOf( ci1 );
	    if ( ci1idx==-1 )
	    {
		if ( output_->coordindices_.size() )
		    output_->coordindices_ += -1;
		output_->coordindices_ += ci0;
		output_->coordindices_ += ci1;
	    }
	    else if ( ci1idx==0 || output_->coordindices_[ci1idx-1]==-1 )
		output_->coordindices_.insert( ci1idx, ci0 );
	    else if ( ci1idx==output_->coordindices_.size()-1 ||
		      output_->coordindices_[ci1idx+1]==-1 )
		output_->coordindices_.insert( ci1idx+1, ci0 );
	    else
		pErrMsg("Hmm");
	}
	else if ( ci0idx==0 || output_->coordindices_[ci0idx-1]==-1 )
	    output_->coordindices_.insert( ci0idx, ci1 );
	else if ( ci0idx==output_->coordindices_.size()-1 ||
		  output_->coordindices_[ci0idx+1]==-1 )
	    output_->coordindices_.insert( ci0idx+1, ci1 );
	else
	    pErrMsg("Hmm");
    }
}


bool getNearestT( const Coord3& c0, const Coord3& c1,
		  const Line3& intersectionline, double& t0, double& t1 ) const
{
    const Line3 line( c0, c1-c0 );
    return intersectionline.closestPoint( line, t0, t1 );
}


int getCoordIndex( const int* arraypos, const Line3& intersectionline,
		   double t, Coord3List& coordlist )
{
    tablelock_.readLock();
    
    int idxs[3];
    if ( intersectioncoordids_.findFirst( arraypos, idxs ) )
    {
	int res = intersectioncoordids_.getRef( idxs, 0 );
	tablelock_.readUnLock();
	return res;
    }

    if ( !tablelock_.convToWriteLock() )
    {
	if ( intersectioncoordids_.findFirst( arraypos, idxs ) )
	{
	    int res = intersectioncoordids_.getRef( idxs, 0 );
	    tablelock_.writeUnLock();
	    return res;
	}
    }

    Coord3 point = intersectionline.getPoint(t);
    point.z /= explsurf_.getZScale();

    const int res = coordlist.add( point );
    intersectioncoordids_.add( &res, arraypos );

    tablelock_.writeUnLock();
    return res;
}

    MultiDimStorage<int>				intersectioncoordids_;
    Threads::ReadWriteLock				tablelock_;

    IndexedGeometry*					output_;
    ObjectSet<ExplPlaneIntersectionExtractorPlane>	planes_;

    ExplPlaneIntersection&				explsurf_;
};


ExplPlaneIntersection::ExplPlaneIntersection()
    : needsupdate_( true )
    , intersection_( 0 )
    , shape_( 0 )
    , shapeversion_( -1 )
    , zscale_( SI().zFactor() )
{ }


ExplPlaneIntersection::~ExplPlaneIntersection()
{
    deepErase( planepts_ );
}


void ExplPlaneIntersection::removeAll()
{
    intersection_ = 0;
    IndexedShape::removeAll();
}


int ExplPlaneIntersection::nrPlanes() const
{ return planeids_.size(); }


int ExplPlaneIntersection::planeID( int idx ) const
{ return planeids_[idx]; }


const Coord3& ExplPlaneIntersection::planeNormal( int id ) const
{ return planenormals_[planeids_.indexOf(id)]; }


const TypeSet<Coord3>& ExplPlaneIntersection::planePolygon( int id ) const
{ return *planepts_[planeids_.indexOf(id)]; }


int ExplPlaneIntersection::addPlane( const Coord3& normal,
				     const TypeSet<Coord3>& pts )
{
    if ( pts.size()<1 )
	return -1;

    int id = 0;
    while ( planeids_.indexOf(id)!=-1 ) id++;

    planeids_ += id;
    planenormals_ += normal;
    planepts_+= new TypeSet<Coord3>( pts );

    const int idx = planeids_.size()-1;
    for ( int idy=0; idy<planepts_[idx]->size(); idy++ )
	(*planepts_[idx])[idy].z *= zscale_;

    needsupdate_ = true;

    return id;
}


bool ExplPlaneIntersection::setPlane( int id, const Coord3& normal,
				      const TypeSet<Coord3>& pts )
{
    if ( pts.size()<1 )
	return false;

    const int idx = planeids_.indexOf( id );

    planenormals_[idx] = normal;
    *planepts_[idx] = pts;

    for ( int idy=0; idy<planepts_[idx]->size(); idy++ )
	(*planepts_[idx])[idy].z *= zscale_;

    needsupdate_ = true;
    return true;
}


void ExplPlaneIntersection::removePlane( int id )
{
    const int idx = planeids_.indexOf( id );

    planeids_.removeFast( idx );
    delete planepts_.removeFast( idx );
    planenormals_.removeFast( idx );

    needsupdate_ = true;
}


void ExplPlaneIntersection::setShape( const IndexedShape& is )
{
    shape_ = &is;
    needsupdate_ = true;
}


const IndexedShape* ExplPlaneIntersection::getShape() const
{ return shape_; }


bool ExplPlaneIntersection::needsUpdate() const
{
    if ( needsupdate_ )
	return true;

    if ( !shape_ )
	return false;

    return shape_->getVersion()!=shapeversion_;
}


bool ExplPlaneIntersection::update( bool forceall, TaskRunner* tr )
{
    if ( !forceall && !needsUpdate() )
	return true;

    if ( !intersection_ )
    {
	intersection_ = new IndexedGeometry( IndexedGeometry::Lines,
					     IndexedGeometry::PerFace,
					     coordlist_ );
	geometrieslock_.writeLock();
	geometries_ += intersection_;
	geometrieslock_.writeUnLock();
    }

    PtrMan<Task> updater = new ExplPlaneIntersectionExtractor( *this );

    if ( (tr && !tr->execute( *updater ) ) || !updater->execute() )
	return false;

    needsupdate_ = false;
    shapeversion_ = shape_->getVersion();
    return true;
}


}; // namespace Geometry
