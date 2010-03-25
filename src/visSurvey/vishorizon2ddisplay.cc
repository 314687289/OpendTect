/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: vishorizon2ddisplay.cc,v 1.35 2010-03-25 10:14:07 cvsumesh Exp $";

#include "vishorizon2ddisplay.h"

#include "bendpointfinder.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "iopar.h"
#include "keystrs.h"
#include "rowcolsurface.h"
#include "survinfo.h"
#include "visdrawstyle.h"
#include "visevent.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vispointset.h"
#include "visseis2ddisplay.h"
#include "viscoord.h"
#include "vistransform.h"
#include "zaxistransform.h"

mCreateFactoryEntry( visSurvey::Horizon2DDisplay );


namespace visSurvey
{

Horizon2DDisplay::Horizon2DDisplay()
   // : zaxistransform_(0)
{
    points_.allowNull(true);
    EMObjectDisplay::setLineStyle( LineStyle(LineStyle::Solid,2 ) );
}


Horizon2DDisplay::~Horizon2DDisplay()
{
    for ( int idx=0; idx<sids_.size(); idx++ )
    	removeSectionDisplay( sids_[idx] );

    removeEMStuff();
}


void Horizon2DDisplay::setDisplayTransformation( mVisTrans* nt )
{
    EMObjectDisplay::setDisplayTransformation( nt );

    for ( int idx=0; idx<lines_.size(); idx++ )
	lines_[idx]->setDisplayTransformation(transformation_);

    for ( int idx=0; idx<lines_.size(); idx++ )
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
    for ( rc.row=rowrg.start; rc.row<=rowrg.stop; rc.row+=rowrg.step )
    {
	const StepInterval<int> colrg = rcs->colRange( rc.row );
	for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col+=colrg.step )
	{
	    const Coord3 pos = emobject_->getPos( sid, rc.getSerialized() );
	    if ( pos.sqDistTo(mousepos) < mDefEps )
	    {
		mDynamicCastGet( const EM::Horizon2D*, h2d, emobject_ );
		info += ", Linename: ";
		info += h2d->geometry().lineName( rc.row );
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


const visBase::IndexedPolyLine3D* Horizon2DDisplay::getLine( 
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
    EMObjectDisplay::drawstyle_->setDrawStyle( visBase::DrawStyle::Lines );

    EMObjectDisplay::setLineStyle( lst );
    for ( int idx=0; idx<lines_.size(); idx++ )
	lines_[idx]->setRadius( (float)lst.width_/2 );
}


bool Horizon2DDisplay::addSection( const EM::SectionID& sid, TaskRunner* tr )
{
    visBase::IndexedPolyLine3D* pl = visBase::IndexedPolyLine3D::create();
    pl->ref();
    pl->setDisplayTransformation( transformation_ );
    pl->setMaterial( 0 );
    pl->setRadius( drawstyle_->lineStyle().width_/2 );
    addChild( pl->getInventorNode() );
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

    removeChild( lines_[idx]->getInventorNode() );
    lines_[idx]->unRef();
    if ( points_[idx] )
    {
	removeChild( points_[idx]->getInventorNode() );
	points_[idx]->unRef();
    }

    points_.remove( idx );
    lines_.remove( idx );
    sids_.remove( idx );
}


bool Horizon2DDisplay::withinRanges( const RowCol& rc, float z, 
				     const LineRanges& linergs )
{
    if ( rc.row<linergs.trcrgs.size() && rc.row<linergs.zrgs.size() )
    {
	for ( int idx=0; idx<linergs.trcrgs[rc.row].size(); idx++ )
	{
	    if ( idx<linergs.zrgs[rc.row].size() &&
		 linergs.zrgs[rc.row][idx].includes(z) && 
		 linergs.trcrgs[rc.row][idx].includes(rc.col) )
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
		visBase::IndexedShape* shape, visBase::PointSet* points,
       		ZAxisTransform*& zaxt )
    : surf_( rcs )
    , lines_( shape )
    , points_( points )
    , lineranges_( lr )
    , lineci_( 0 )
    , linecii_( 0 )
    , scale_( 1, 1, SI().zScale() )
    , zaxt_( zaxt )
{
    eps_ = mMIN(SI().inlDistance(),SI().crlDistance());
    eps_ = mMIN(eps_,SI().zRange(true).step*scale_.z )/4;

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
    return true;
}

bool doFinish( bool res )
{
    lines_->removeCoordIndexAfter( linecii_-1 );
    lines_->getCoordinates()->removeAfter( lineci_-1 );

    return res;
}


bool doWork( od_int64 start, od_int64 stop, int )
{
    RowCol rc;
    while ( true )
    {
	rc.row = getNextRow();
	if ( mIsUdf(rc.row) )
	    break;

	TypeSet<Coord3> positions;
	const StepInterval<int> colrg = surf_->colRange( rc.row );

	for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col+=colrg.step )
	{
	    Coord3 pos = surf_->getKnot( rc );

	    if ( zaxt_ )
	    {
		const BinIDValue bidval( rc.r(), rc.c(), pos.z );
		pos.z = zaxt_->transform( bidval );
	    }

	    // Skip if survey coordinates not available
	    if ( !Coord(pos).isDefined() )
		continue;

	    if ( !pos.isDefined() || 
		(lineranges_ &&
		!Horizon2DDisplay::withinRanges(rc,pos.z,*lineranges_)) )
	    {
		if ( positions.size() )
		    sendPositions( positions );
	    }
	    else 
	    {
		positions += pos;
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
	points_->getCoordinates()->addPos( positions[0] );
    }
    else
    {
	BendPointFinder3D finder( positions, scale_, eps_ );
	finder.execute(nrthreads_<2);
	const TypeSet<int>& bendpoints = finder.bendPoints();
	const int nrbendpoints = bendpoints.size();
	if ( nrbendpoints )
	{
	    ArrPtrMan<Coord3> usedpos = new Coord3[nrbendpoints];

	    for ( int idy=0; idy<nrbendpoints; idy++ )
		usedpos[idy] = positions[bendpoints[idy]];

	    ArrPtrMan<int> idxs = new int[nrbendpoints+1];

	    lock_.lock();

	    for ( int idy=0; idy<nrbendpoints; idy++ )
		idxs[idy] = idy+lineci_;

	    idxs[nrbendpoints] = -1;
	    lines_->getCoordinates()->setPositions( usedpos.ptr(), nrbendpoints,
						    lineci_ );
	    lines_->setCoordIndices( idxs.ptr(), nrbendpoints+1, linecii_ );
	    lineci_ += nrbendpoints;
	    linecii_ += nrbendpoints+1;

	    lock_.unLock();
	}
    }

    positions.erase();
}

protected:
    const Geometry::RowColSurface*	surf_;
    const Horizon2DDisplay::LineRanges*	lineranges_;
    visBase::IndexedShape*		lines_;
    visBase::PointSet*			points_;
    ZAxisTransform*			zaxt_;
    Threads::Mutex			lock_;
    int					nrthreads_;

    int					lineci_;
    int					linecii_;

    const Coord3			scale_;
    float				eps_;
    StepInterval<int>			rowrg_;
    int					nriter_;
    int					curidx_;
};


void Horizon2DDisplay::updateSection( int idx, const LineRanges* lineranges )
{
    if ( !emobject_ ) return;
    const EM::SectionID sid = emobject_->sectionID( idx );
    mDynamicCastGet(const Geometry::RowColSurface*,rcs,
	    	    emobject_->sectionGeometry(sid));

    visBase::IndexedPolyLine3D* pl = lines_[idx];
    visBase::PointSet* ps = points_[idx];
   
    if ( !ps ) 
    {
	ps = visBase::PointSet::create();
	ps->ref();
	ps->getCoordinates()->removeAfter(-1);
	points_.replace( idx, ps );
	addChild( ps->getInventorNode() );
    }

    Horizon2DDisplayUpdater updater( rcs, lineranges, pl, ps, zaxistransform_ );
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
    if ( !seis2dlist.size() ) return;

    if ( !displayonlyatsections_ )
    {
	for ( int sidx=0; sidx<sids_.size(); sidx++ )
	    updateSection( sidx );
	return;
    }

    mDynamicCastGet( const EM::Horizon2D*, h2d, emobject_ );
    if ( !h2d ) return;
    
    TypeSet<Coord> xy0;
    TypeSet<Coord> xy1;
    for ( int idx=0; idx<seis2dlist.size(); idx++ )
    {
	const Interval<int>& trcrg  = seis2dlist[idx]->getTraceNrRange();
	xy0 += seis2dlist[idx]->getCoord( trcrg.start );
	xy1 += seis2dlist[idx]->getCoord( trcrg.stop );
    }

    LineRanges linergs;
    for ( int lnidx=0; lnidx<h2d->geometry().nrLines(); lnidx++ )
    {
	int lineid = h2d->geometry().lineID( lnidx );
	linergs.trcrgs += TypeSet<Interval<int> >();
	linergs.zrgs += TypeSet<Interval<float> >();
	for ( int idx=0; idx<seis2dlist.size(); idx++ )
	{
	    const Interval<int>& trcrg  = seis2dlist[idx]->getTraceNrRange();
	    RowCol rc( lineid, trcrg.start );
	    Coord pos = emobject_->getPos( 0, rc.getSerialized() ); 
	    if ( !xy0[idx].isDefined() || !mIsEqual(xy0[idx].x,pos.x,mDefEps) 
				       || !mIsEqual(xy0[idx].y,pos.y,mDefEps) )
		continue;

	    rc.col = trcrg.stop;
	    pos = emobject_->getPos( 0, rc.getSerialized() ); 
	    if ( !xy1[idx].isDefined() || !mIsEqual(xy1[idx].x,pos.x,mDefEps) 
				       || !mIsEqual(xy1[idx].y,pos.y,mDefEps) )
		continue;

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
	visBase::DataObjectGroup* group = posattribmarkers_[idx];
	for ( int idy=0; idy<group->size(); idy++ )
	{
	    mDynamicCastGet(visBase::Marker*,marker,group->getObject(idy));
	    if ( !marker ) continue;

	    marker->turnOn( !displayonlyatsections_ );
	    Coord3 pos = marker->centerPos();
	    if ( transformation_ ) 
		pos = transformation_->transform( pos );
	    for ( int idz=0; idz<seis2dlist.size(); idz++ )
	    {
		const float dist = seis2dlist[idz]->calcDist(pos);
		if ( dist < seis2dlist[idz]->maxDist() )
		{
		    marker->turnOn(true);
		    break;
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
    const bool haddatatransform = zaxistransform_;
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


//const ZAxisTransform* Horizon2DDisplay::getZAxisTransform() const
//{ return zaxistransform_; }


void Horizon2DDisplay::zAxisTransformChg( CallBacker* )
{
    // TODO: implement
    for ( int sidx=0; sidx<sids_.size(); sidx++ )
	updateSection( sidx );
}


void Horizon2DDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visSurvey::EMObjectDisplay::fillPar( par, saveids );
}


int Horizon2DDisplay::usePar( const IOPar& par )
{
    return visSurvey::EMObjectDisplay::usePar( par );
}


}; // namespace visSurvey
