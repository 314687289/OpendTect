/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Y.C. Liu
 * DATE     : January 2008
-*/

static const char* rcsID = "$Id: gridder2d.cc,v 1.3 2008-06-17 14:23:38 cvskris Exp $";

#include "gridder2d.h"

#include "delaunay.h"
#include "iopar.h"
#include "positionlist.h"
#include "math2.h"
#include "sorting.h"

#define mEpsilon 0.0001

mImplFactory( Gridder2D, Gridder2D::factory );


Gridder2D::Gridder2D()
    : values_( 0 )
    , points_( 0 )
    , inited_( false )
{}


Gridder2D::Gridder2D( const Gridder2D & )
    : values_( 0 )
    , points_( 0 )
    , inited_( false )
{}


bool Gridder2D::operator==( const Gridder2D& b ) const
{
    return name()==b.name();
}


bool Gridder2D::isPointUsable(const Coord& cpt,const Coord& dpt) const
{ return true; }


bool Gridder2D::setPoints( const TypeSet<Coord>& cl )
{
    points_ = &cl;
    inited_ = false;
    return true;
}


bool Gridder2D::setValues( const TypeSet<float>& vl, bool hasudfs )
{
    values_ = &vl;
    if ( hasudfs ) inited_ = false;
    return true;
}


bool Gridder2D::setGridPoint( const Coord& pt )
{
    inited_ = false;
    gridpoint_ = pt;
    return gridpoint_.isDefined();
}


float Gridder2D::getValue() const
{
    if ( !inited_ )
	return mUdf(float);

    if ( !usedvalues_.size() )
	return mUdf(float);

    if ( !values_ )
	return mUdf(float);;

    double valweightsum = 0;
    double weightsum = 0;
    int nrvals = 0;
    const int nrvalues = values_->size();
    for ( int idx=usedvalues_.size()-1; idx>=0; idx-- )
    {
	const int validx = usedvalues_[idx];
	if ( validx>=nrvalues )
	{
	    pErrMsg("Should not happen");
	    return mUdf(float);
	}

	if ( mIsUdf((*values_)[validx]) )
	    continue;

	valweightsum += (*values_)[validx] * weights_[idx];
	weightsum += weights_[idx];
	nrvals++;
    }

    if ( !nrvals )
	return mUdf(float);

    if ( nrvals==usedvalues_.size() )
	return valweightsum;

    return valweightsum/weightsum;
}


bool Gridder2D::isPointUsed( int idx ) const
{
    return usedvalues_.indexOf(idx) != -1;
}


InverseDistanceGridder2D::InverseDistanceGridder2D()
    : radius_( mUdf(float) )
{}


InverseDistanceGridder2D::InverseDistanceGridder2D(
	const InverseDistanceGridder2D& g )
    : radius_( g.radius_ )
{}


Gridder2D* InverseDistanceGridder2D::create()
{
    return new InverseDistanceGridder2D;
}


void InverseDistanceGridder2D::initClass()
{
    Gridder2D::factory().addCreator( create, sName(), sUserName() );
}


Gridder2D* InverseDistanceGridder2D::clone() const
{ return new InverseDistanceGridder2D( *this ); }


bool InverseDistanceGridder2D::operator==( const Gridder2D& b ) const
{
    if ( !Gridder2D::operator==( b ) )
	return false;

    mDynamicCastGet( const InverseDistanceGridder2D*, bidg, &b );
    if ( !bidg )
	return false;

    if ( mIsUdf(radius_) && mIsUdf( bidg->radius_ ) )
	return true;

    return mIsEqual(radius_,bidg->radius_, 1e-5 );
}


void InverseDistanceGridder2D::setSearchRadius( float r )
{
    if ( r<= 0 )
	return;

    radius_ = r;
}


bool InverseDistanceGridder2D::isPointUsable( const Coord& calcpt,
					     const Coord& datapt ) const
{
    if ( !datapt.isDefined() || !calcpt.isDefined() )
	return false;

    return mIsUdf(radius_) || calcpt.sqDistTo( datapt ) < radius_*radius_;
}


bool InverseDistanceGridder2D::init()
{
    usedvalues_.erase();
    weights_.erase();

    if ( !gridpoint_.isDefined() || !points_ || !points_->size() )
	return false;

    const bool useradius = !mIsUdf(radius_);
    const double radius2 = radius_*radius_;


    double weightsum = 0;
    for ( int idx=points_->size()-1; idx>=0; idx-- )
    {
	if ( !(*points_)[idx].isDefined() )
	    continue;

	const double sqdist = gridpoint_.sqDistTo( (*points_)[idx] );
	if ( useradius && sqdist>radius2 )
	    continue;

	if ( mIsZero(sqdist, mEpsilon) )
	{
	    usedvalues_.erase();
	    weights_.erase();

	    usedvalues_ += idx;
	    weights_ += 1;
	    inited_ = true;
	    return true;
	}

	const double dist = sqrt( sqdist );
	const double weight = 1/dist;

	weightsum += weight;
	weights_ += weight;
	usedvalues_ += idx;
    }

    for ( int idx=weights_.size()-1; idx>=0; idx-- )
	weights_[idx] /= weightsum;

    inited_ = true;
    return true;
}


bool InverseDistanceGridder2D::usePar( const IOPar& par )
{
    float r;
    if ( !par.get( sKeySearchRadius(), r ) )
	return false;

    setSearchRadius( r ); 
    return true;
}


void InverseDistanceGridder2D::fillPar( IOPar& par ) const
{
    par.set( sKeySearchRadius(), getSearchRadius() );
}


TriangulatedGridder2D::TriangulatedGridder2D()
    : triangles_( 0 )
    , xrg_( mUdf(float), mUdf(float) )
    , yrg_( mUdf(float), mUdf(float) )
{}


TriangulatedGridder2D::TriangulatedGridder2D( const TriangulatedGridder2D& b )
    : triangles_( 0 )
    , xrg_( b.xrg_ )
    , yrg_( b.yrg_ )
{
    if ( b.triangles_ )
	triangles_ = new DAGTriangleTree( *b.triangles_ );
}


TriangulatedGridder2D::~TriangulatedGridder2D()
{ delete triangles_; }

void TriangulatedGridder2D::setGridArea( const Interval<float>& xrg,
	                                 const Interval<float>& yrg)
{
    xrg_ = xrg;
    yrg_ = yrg;
}


Gridder2D* TriangulatedGridder2D::create()
{
    return new TriangulatedGridder2D;
}


void TriangulatedGridder2D::initClass()
{
    Gridder2D::factory().addCreator( create, sName(), sUserName() );
}


Gridder2D* TriangulatedGridder2D::clone() const
{ return new TriangulatedGridder2D( *this ); }


bool TriangulatedGridder2D::init()
{
    usedvalues_.erase();
    weights_.erase();

    inited_ = false;

    if ( !points_ || !points_->size() || !gridpoint_.isDefined() )
	return true;

    if ( points_->size()==1 )
    {
	usedvalues_ += 0;
	weights_ += 1;
	inited_ = true;
	return true;
    }

    if ( !triangles_ )
    {
	triangles_ = new DAGTriangleTree;
	Interval<double> xrg, yrg;
	if ( !DAGTriangleTree::computeCoordRanges( *points_, xrg, yrg ) ) 
	    return false;

	if ( !triangles_->setCoordList( *points_, true ) )
	    return false;

	xrg.include( xrg_.start ); xrg.include( xrg_.stop );
	yrg.include( yrg_.start ); yrg.include( yrg_.stop );

	if ( !triangles_->setBBox( xrg, yrg ) )
	    return false;

	ParallelDTriangulator triangulator( *triangles_ );
	triangulator.dataIsRandom( false );
	if ( !triangulator.execute( false ) )
	{
	    delete triangles_;
	    triangles_ = false;
	    return false;
	}
    }

    DAGTriangleTree interpoltriangles( *triangles_ );
    int dupid = -1;
    const int gridptid = interpoltriangles.insertPoint( gridpoint_, dupid );
    if ( gridptid==DAGTriangleTree::cNoVertex() )
	return false;

    if ( dupid!=DAGTriangleTree::cNoVertex() )
    {
	usedvalues_ += dupid;
	weights_ += 1;
	inited_ = true;
	return true;
    }

    double weightsum = 0;
    TypeSet<int> connections;
    interpoltriangles.getConnections( gridptid, connections );
    for ( int idx=connections.size()-1; idx>=0; idx-- )
    {
	const Coord& point = interpoltriangles.coordList()[connections[idx]];
	const float sqdist = gridpoint_.sqDistTo( point );
	if ( !sqdist ) //Should not be neccesary here.
	{
	    usedvalues_.erase();
	    weights_.erase();
	    usedvalues_ += connections[idx];
	    weights_ += 1;
	    inited_ = true;

	    return true;
	}

	const float weight = 1/Math::Sqrt(sqdist);

	usedvalues_ += connections[idx];
	weights_ += weight;
	weightsum += weight;
    }
    
    for ( int idx=weights_.size()-1; idx>=0; idx-- )
        weights_[idx] /= weightsum;

    inited_ = true;
    return true;
}
