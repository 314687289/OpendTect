/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "vishorizon2ddisplay.h"

#include "bendpointfinder.h"
#include "emhorizon2d.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "iopar.h"
#include "keystrs.h"
#include "rowcolsurface.h"
#include "survinfo.h"
#include "visdrawstyle.h"
#include "visevent.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vispointset.h"
#include "visseis2ddisplay.h"
#include "viscoord.h"
#include "vistransform.h"
#include "zaxistransform.h"


namespace visSurvey
{

Horizon2DDisplay::Horizon2DDisplay()
{
    points_.allowNull(true);
    EMObjectDisplay::setLineStyle( LineStyle(LineStyle::Solid,5 ) );
}


Horizon2DDisplay::~Horizon2DDisplay()
{
    setZAxisTransform( 0, 0 );

    for ( int idx=0; idx<sids_.size(); idx++ )
	removeSectionDisplay( sids_[idx] );

    removeEMStuff();
}


void Horizon2DDisplay::setDisplayTransformation( const mVisTrans* nt )
{
    EMObjectDisplay::setDisplayTransformation( nt );

    for ( int idx=0; idx<lines_.size(); idx++ )
	lines_[idx]->setDisplayTransformation( transformation_ );

    for ( int idx=0; idx<points_.size(); idx++ )
    {
	if( points_[idx] )
	    points_[idx]->setDisplayTransformation(transformation_);
    }
}


void Horizon2DDisplay::getMousePosInfo(const visBase::EventInfo& eventinfo,
				       Coord3& mousepos,
				       BufferString& val,
				       BufferString& info) const
{
    EMObjectDisplay::getMousePosInfo( eventinfo, mousepos, val, info );
    const EM::SectionID sid =
		    EMObjectDisplay::getSectionID( &eventinfo.pickedobjids );
    if ( sid<0 ) return;

    mDynamicCastGet( const Geometry::RowColSurface*, rcs,
		     emobject_->sectionGeometry(sid));
    if ( !rcs ) return;

    const StepInterval<int> rowrg = rcs->rowRange();
    RowCol rc;
    for ( rc.row()=rowrg.start; rc.row()<=rowrg.stop; rc.row()+=rowrg.step )
    {
	const StepInterval<int> colrg = rcs->colRange( rc.row() );
	for ( rc.col()=colrg.start; rc.col()<=colrg.stop; rc.col()+=colrg.step )
	{
	    const Coord3 pos = emobject_->getPos( sid, rc.toInt64() );
	    if ( pos.sqDistTo(mousepos) < mDefEps )
	    {
		mDynamicCastGet( const EM::Horizon2D*, h2d, emobject_ );
		info += ", Linename: ";
		info += h2d->geometry().lineName( rc.row() );
		return;
	    }

	}
    }
}


EM::SectionID Horizon2DDisplay::getSectionID(int visid) const
{
    for ( int idx=0; idx<lines_.size(); idx++ )
    {
	if ( lines_[idx]->id()==visid ||
	     (points_[idx] && points_[idx]->id()==visid) )
	    return sids_[idx];
    }

    return -1;
}


const visBase::PolyLine3D* Horizon2DDisplay::getLine(
	const EM::SectionID& sid ) const
{
    for ( int idx=0; idx<sids_.size(); idx++ )
	if ( sids_[idx]==sid ) return lines_[idx];

    return 0;
}


const visBase::PointSet* Horizon2DDisplay::getPointSet(
	const EM::SectionID& sid ) const
{
    for ( int idx=0; idx<sids_.size(); idx++ )
	if ( sids_[idx]==sid ) return points_[idx];

    return 0;
}

void Horizon2DDisplay::setLineStyle( const LineStyle& lst )
{
    // TODO: set the draw style correctly after properly implementing
    // different line styles. Only SOLID is supported now.
    //EMObjectDisplay::drawstyle_->setDrawStyle( visBase::DrawStyle::Lines );

    EMObjectDisplay::setLineStyle( lst );
    for ( int idx=0; idx<lines_.size(); idx++ )
	lines_[idx]->setLineStyle( lst );

    drawstyle_->setLineStyle( lst );
}


bool Horizon2DDisplay::addSection( const EM::SectionID& sid, TaskRunner* tr )
{
    visBase::PolyLine3D* pl = visBase::PolyLine3D::create();
    pl->ref();
    pl->setDisplayTransformation( transformation_ );
    pl->setName( "PolyLine3D" );
    pl->setLineStyle( drawstyle_->lineStyle() );
    addChild( pl->osgNode() );
    lines_ += pl;
    points_ += 0;
    sids_ += sid;

    updateSection( sids_.size()-1 );

    return true;
}


void Horizon2DDisplay::removeSectionDisplay( const EM::SectionID& sid )
{
    const int idx = sids_.indexOf( sid );
    if ( idx<0 ) return;

    removeChild( lines_[idx]->osgNode() );
    lines_[idx]->unRef();
    if ( points_[idx] )
    {
	removeChild( points_[idx]->osgNode() );
	points_[idx]->unRef();
    }

    points_.removeSingle( idx );
    lines_.removeSingle( idx );
    sids_.removeSingle( idx );
}


bool Horizon2DDisplay::withinRanges( const RowCol& rc, float z,
				     const LineRanges& linergs )
{
    if ( rc.row()<linergs.trcrgs.size() && rc.row()<linergs.zrgs.size() )
    {
	for ( int idx=0; idx<linergs.trcrgs[rc.row()].size(); idx++ )
	{
	    if ( idx<linergs.zrgs[rc.row()].size() &&
		 linergs.zrgs[rc.row()][idx].includes(z,true) &&
		 linergs.trcrgs[rc.row()][idx].includes(rc.col(),true) )
		return true;
	}
    }
    return false;
}


class Horizon2DDisplayUpdater : public ParallelTask
{
public:
Horizon2DDisplayUpdater( const Geometry::RowColSurface* rcs,
		const Horizon2DDisplay::LineRanges* lr,
		visBase::VertexShape* shape, visBase::PointSet* points,
		const ZAxisTransform* zaxt, const BufferStringSet& linenames )
    : surf_( rcs )
    , lines_( shape )
    , points_( points )
    , lineranges_( lr )
    , scale_( 1, 1, SI().zScale() )
    , zaxt_( zaxt )
    , linenames_(linenames)
    , crdidx_(0)
{
    eps_ = mMIN(SI().inlDistance(),SI().crlDistance());
    eps_ = (float) mMIN(eps_,SI().zRange(true).step*scale_.z )/4;

    rowrg_ = surf_->rowRange();
    nriter_ = rowrg_.isRev() ? 0 : rowrg_.nrSteps()+1;
}


od_int64 nrIterations() const { return nriter_; }


int getNextRow()
{
    Threads::MutexLocker lock( lock_ );
    if ( curidx_>=nriter_ )
	return mUdf(int);

    const int res = rowrg_.atIndex( curidx_++ );
    return res;
}


bool doPrepare( int nrthreads )
{
    curidx_ = 0;
    nrthreads_ = nrthreads;
    points_->removeAllPoints();
    lines_->removeAllPrimitiveSets();
    return true;
}


bool doFinish( bool res )
{
    return res;
}


bool doWork( od_int64 start, od_int64 stop, int )
{
    RowCol rc;
    while ( true )
    {
	rc.row() = getNextRow();
	if ( mIsUdf(rc.row()) )
	    break;

	const int rowidx = rowrg_.getIndex( rc.row() );
	const char* linenm =
	    linenames_.validIdx(rowidx) ? linenames_.get(rowidx).buf() : 0;
	TypeSet<Coord3> positions;
	const StepInterval<int> colrg = surf_->colRange( rc.row() );

	for ( rc.col()=colrg.start; rc.col()<=colrg.stop; rc.col()+=colrg.step )
	{
	    Coord3 pos = surf_->getKnot( rc );
	    const float zval = mCast(float,pos.z);

	    if ( !pos.isDefined() || (lineranges_ &&
		 !Horizon2DDisplay::withinRanges(rc,zval,*lineranges_)) )
	    {
		if ( positions.size() )
		    sendPositions( positions );
	    }
	    else
	    {
		if ( zaxt_ )
		    pos.z = zaxt_->transform2D( linenm, rc.col(), zval );
		if ( !mIsUdf(pos.z) )
		positions += pos;
		else if ( positions.size() )
		    sendPositions( positions );
	    }
	}

	sendPositions( positions );
    }

    return true;
}

void sendPositions( TypeSet<Coord3>& positions )
{
    if ( !positions.size() )
	return;

    if ( positions.size()==1 )
    {
	points_->addPoint( positions[0] );
    }
    else
    {
	BendPointFinder3D finder( positions, scale_, eps_ );
	finder.executeParallel(nrthreads_<2);
	const TypeSet<int>& bendpoints = finder.bendPoints();
	const int nrbendpoints = bendpoints.size();
	if ( nrbendpoints )
	{
	    TypeSet<int> indices;
	    lock_.lock();
	    for ( int idy=0; idy<nrbendpoints; idy++ )
	    {
		const Coord3& pos = positions[ bendpoints[idy] ];
		lines_->getCoordinates()->setPos( crdidx_, pos );
		indices += crdidx_++;
	    }

	    Geometry::IndexedPrimitiveSet* lineprimitiveset =
				Geometry::IndexedPrimitiveSet::create( true );
	    lineprimitiveset->ref();
	    lineprimitiveset->set( indices.arr(), indices.size() );
	    lines_->addPrimitiveSet( lineprimitiveset );
	    lineprimitiveset->unRef();
	    lock_.unLock();
	}
    }

    positions.erase();
}

protected:
    const Geometry::RowColSurface*	surf_;
    const Horizon2DDisplay::LineRanges*	lineranges_;
    visBase::VertexShape*		lines_;
    visBase::PointSet*			points_;
    const ZAxisTransform*		zaxt_;
    const BufferStringSet&		linenames_;
    Threads::Mutex			lock_;
    int					nrthreads_;
    const Coord3			scale_;
    float				eps_;
    StepInterval<int>			rowrg_;
    int					nriter_;
    int					curidx_;
    int					crdidx_;
};


void Horizon2DDisplay::updateSection( int idx, const LineRanges* lineranges )
{
    if ( !emobject_ ) return;

    visBase::PointSet* ps = points_.validIdx(idx) ? points_[idx] : 0;
    if ( !ps )
    {
	ps = visBase::PointSet::create();
	ps->ref();
	ps->removeAllPoints();
	ps->setDisplayTransformation( transformation_ );
	points_.replace( idx, ps );
	addChild( ps->osgNode() );
    }

    BufferStringSet linenames;
    EM::IOObjInfo info( emobject_->multiID() );
    info.getLineNames( linenames );

    LineRanges linergs;
    mDynamicCastGet(const EM::Horizon2D*,h2d,emobject_);
    const bool redo = h2d && zaxistransform_ && linenames.isEmpty();
    if ( redo )
    {
	const EM::Horizon2DGeometry& emgeo = h2d->geometry();
	for ( int lnidx=0; lnidx<emgeo.nrLines(); lnidx++ )
	{
	    linenames.add( emgeo.lineName(lnidx) );
	    const Pos::GeomID geomid = emgeo.geomID( lnidx );

	    for ( int idy=0; idy<h2d->nrSections(); idy++ )
	    {
		const Geometry::Horizon2DLine* ghl =
		    emgeo.sectionGeometry( h2d->sectionID(idy) );
		if ( ghl )
		{
		    linergs.trcrgs += TypeSet<Interval<int> >();
		    linergs.zrgs += TypeSet<Interval<float> >();
		    const int ridx = linergs.trcrgs.size()-1;

		    linergs.trcrgs[ridx] += ghl->colRange( geomid );
		    linergs.zrgs[ridx] += ghl->zRange( geomid );
		}
	    }
	}
    }

    const EM::SectionID sid = emobject_->sectionID( idx );
    mDynamicCastGet(const Geometry::RowColSurface*,rcs,
		    emobject_->sectionGeometry(sid));
    const LineRanges* lrgs = redo ? &linergs : lineranges;
    visBase::PolyLine3D* pl = lines_.validIdx(idx) ? lines_[idx] : 0;

    if ( !rcs || !pl )
	return;

    Horizon2DDisplayUpdater updater( rcs, lrgs, pl, ps,
				     zaxistransform_, linenames );
    updater.execute();
}


void Horizon2DDisplay::emChangeCB( CallBacker* cb )
{
    EMObjectDisplay::emChangeCB( cb );
    mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);
    if ( cbdata.event==EM::EMObjectCallbackData::PrefColorChange )
	getMaterial()->setColor( emobject_->preferredColor() );
}


void Horizon2DDisplay::updateLinesOnSections(
			const ObjectSet<const Seis2DDisplay>& seis2dlist )
{
    if ( !displayonlyatsections_ )
    {
	for ( int sidx=0; sidx<sids_.size(); sidx++ )
	    updateSection( sidx );
	return;
    }

    mDynamicCastGet(const EM::Horizon2D*,h2d,emobject_);
    if ( !h2d ) return;

    LineRanges linergs;
    for ( int lnidx=0; lnidx<h2d->geometry().nrLines(); lnidx++ )
    {
	const Pos::GeomID geomid = h2d->geometry().geomID( lnidx );
	linergs.trcrgs += TypeSet<Interval<int> >();
	linergs.zrgs += TypeSet<Interval<float> >();
	for ( int idx=0; idx<seis2dlist.size(); idx++ )
	{
	    Interval<int> trcrg = h2d->geometry().colRange( geomid );
	    trcrg.limitTo( seis2dlist[idx]->getTraceNrRange() );
	    if ( geomid != seis2dlist[idx]->getGeomID() )
	    {
		const Coord sp0 = seis2dlist[idx]->getCoord( trcrg.start );
		const Coord sp1 = seis2dlist[idx]->getCoord( trcrg.stop );
		if ( !trcrg.width() || !sp0.isDefined() || !sp1.isDefined() )
		    continue;

		const Coord hp0 = h2d->getPos( 0, geomid, trcrg.start );
		const Coord hp1 = h2d->getPos( 0, geomid, trcrg.stop );
		if ( !hp0.isDefined() || !hp1.isDefined() )
		    continue;

		const float maxdist =
			(float) ( 0.1 * sp0.distTo(sp1) / trcrg.width() );
		if ( hp0.distTo(sp0)>maxdist || hp1.distTo(sp1)>maxdist )
		    continue;
	    }

	    linergs.trcrgs[lnidx] += trcrg;
	    linergs.zrgs[lnidx] += seis2dlist[idx]->getZRange( true );
	}
    }

    for ( int sidx=0; sidx<sids_.size(); sidx++ )
	updateSection( sidx, &linergs );
}


void Horizon2DDisplay::updateSeedsOnSections(
			const ObjectSet<const Seis2DDisplay>& seis2dlist )
{
    for ( int idx=0; idx<posattribmarkers_.size(); idx++ )
    {
	visBase::MarkerSet* markerset = posattribmarkers_[idx];
	for ( int idy=0; idy<markerset->getCoordinates()->size(); idy++ )
	{
	    markerset->turnMarkerOn( idy,!displayonlyatsections_ );
	    const visBase::Coordinates* markercoords =
		markerset->getCoordinates();
	    if ( markercoords->size() )
	    {
		Coord3 markerpos = markercoords->getPos( idy, true );
	         if ( zaxistransform_ )
		    markerpos.z = zaxistransform_->transform( markerpos );
		for ( int idz=0; idz<seis2dlist.size(); idz++ )
		{
		    const float dist = seis2dlist[idz]->calcDist( markerpos );
		    if ( dist < seis2dlist[idz]->maxDist() )
		    {
			 markerset->turnMarkerOn( idy,true );
			break;
		    }
		}
	    }
	}
    }
}


void Horizon2DDisplay::otherObjectsMoved(
		    const ObjectSet<const SurveyObject>& objs, int movedobj )
{
    if ( burstalertison_ ) return;

    bool refresh = movedobj==-1 || movedobj==id();
    ObjectSet<const Seis2DDisplay> seis2dlist;

    for ( int idx=0; idx<objs.size(); idx++ )
    {
	mDynamicCastGet(const Seis2DDisplay*,seis2d,objs[idx]);
	if ( seis2d )
	{
	    seis2dlist += seis2d;
	    if ( movedobj==seis2d->id() )
		refresh = true;
	}
    }

    if ( !refresh ) return;

    updateLinesOnSections( seis2dlist );
    updateSeedsOnSections( seis2dlist );
}


bool Horizon2DDisplay::setEMObject( const EM::ObjectID& newid, TaskRunner* tr )
{
    if ( !EMObjectDisplay::setEMObject( newid, tr ) )
	return false;

    getMaterial()->setColor( emobject_->preferredColor() );
    return true;
}


bool Horizon2DDisplay::setZAxisTransform( ZAxisTransform* zat, TaskRunner* tr )
{
    CallBack cb = mCB(this,Horizon2DDisplay,zAxisTransformChg);
    if ( zaxistransform_ )
    {
	if ( zaxistransform_->changeNotifier() )
	    zaxistransform_->changeNotifier()->remove( cb );
	zaxistransform_->unRef();
	zaxistransform_ = 0;
    }

    zaxistransform_ = zat;
    if ( zaxistransform_ )
    {
	zaxistransform_->ref();
	if ( zaxistransform_->changeNotifier() )
	    zaxistransform_->changeNotifier()->notify( cb );
    }

    return true;
}


void Horizon2DDisplay::zAxisTransformChg( CallBacker* )
{
    for ( int sidx=0; sidx<sids_.size(); sidx++ )
	updateSection( sidx );
}


void Horizon2DDisplay::fillPar( IOPar& par ) const
{
    visSurvey::EMObjectDisplay::fillPar( par );
}


bool Horizon2DDisplay::usePar( const IOPar& par )
{
    return visSurvey::EMObjectDisplay::usePar( par );
}


void Horizon2DDisplay::doOtherObjectsMoved(
	            const ObjectSet<const SurveyObject>& objs, int whichobj )
{
    otherObjectsMoved( objs, whichobj );
}


void Horizon2DDisplay::setPixelDensity( float dpi )
{
    EMObjectDisplay::setPixelDensity( dpi );

    for ( int idx =0; idx<lines_.size(); idx++ )
	lines_[idx]->setPixelDensity( dpi );

    for ( int idx=0; idx<points_.size(); idx++ )
	points_[idx]->setPixelDensity( dpi );
}

}; // namespace visSurvey
