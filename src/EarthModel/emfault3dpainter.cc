/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Feb 2010
 RCS:		$Id: emfault3dpainter.cc,v 1.8 2010-08-31 14:35:55 cvsjaap Exp $
________________________________________________________________________

-*/

#include "emfault3dpainter.h"

#include "emfault3d.h"
#include "emmanager.h"
#include "emobject.h"
#include "explfaultsticksurface.h"
#include "explplaneintersection.h"
#include "positionlist.h"
#include "survinfo.h"
#include "trigonometry.h"

namespace EM
{

Fault3DPainter::Fault3DPainter( FlatView::Viewer& fv, const EM::ObjectID& oid )
    : viewer_(fv)
    , emid_(oid)
    , markerlinestyle_(LineStyle::Solid,2,Color(0,255,0))
    , markerstyle_(MarkerStyle2D::Square, 4, Color(255,255,0) )
    , activestickid_( mUdf(int) )
    , abouttorepaint_(this)
    , repaintdone_(this)
    , linenabled_(true)
    , knotenabled_(true)
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
    {
	emobj->ref();
	emobj->change.notify( mCB(this,Fault3DPainter,fault3DChangedCB) );
    }
    cs_.setEmpty();
}


Fault3DPainter::~Fault3DPainter()
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
    {
	emobj->change.remove( mCB(this,Fault3DPainter,fault3DChangedCB) );
	emobj->unRef();
    }

    removePolyLine();
    viewer_.handleChange( FlatView::Viewer::Annot );
}


void Fault3DPainter::setCubeSampling( const CubeSampling& cs, bool update )
{ cs_ = cs; }


bool Fault3DPainter::addPolyLine()
{
    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid_ );

    mDynamicCastGet(EM::Fault3D*,emf3d,emobject.ptr());
    if ( !emf3d ) return false;

    for ( int sidx=0; sidx<emf3d->nrSections(); sidx++ )
    {
	int sid = emf3d->sectionID( sidx );

	Fault3DMarker* f3dsectionmarker = new Fault3DMarker;
	f3dmarkers_ += f3dsectionmarker;

	bool stickpained = paintSticks(*emf3d,sid,f3dsectionmarker);
	bool intersecpainted = paintIntersection(*emf3d,sid,f3dsectionmarker);
	if ( !stickpained && !intersecpainted )
	     return false;
    }

    return true;
}


void Fault3DPainter::paint()
{
    removePolyLine();
    addPolyLine();
    viewer_.handleChange( FlatView::Viewer::Annot );
}


bool Fault3DPainter::paintSticks(EM::Fault3D& f3d, const EM::SectionID& sid,
				 Fault3DMarker* f3dmaker )
{
    mDynamicCastGet( Geometry::FaultStickSurface*, fss,
		     f3d.sectionGeometry(sid) );

    if ( !fss || fss->isEmpty() )
	return false;

    if ( cs_.isEmpty() ) return false;

    RowCol rc;
    const StepInterval<int> rowrg = fss->rowRange();
    for ( rc.row=rowrg.start; rc.row<=rowrg.stop; rc.row+=rowrg.step )
    {
	StepInterval<int> colrg = fss->colRange( rc.row );
	FlatView::Annotation::AuxData* stickauxdata =
	 			new FlatView::Annotation::AuxData( 0 );
	stickauxdata->poly_.erase();
	stickauxdata->linestyle_ = markerlinestyle_;
	stickauxdata->linestyle_.color_ = f3d.preferredColor();
	stickauxdata->markerstyles_ += markerstyle_;
	if ( !knotenabled_ )
	    stickauxdata->markerstyles_.erase();
	stickauxdata->enabled_ = linenabled_;

	Coord3 editnormal( 0, 0, 1 ); 
	// Let's assume cs default dir. is 'Z'

	if ( cs_.defaultDir() == CubeSampling::Inl )
	    editnormal = Coord3( SI().binID2Coord().rowDir(), 0 );
	else if ( cs_.defaultDir() == CubeSampling::Crl )
	    editnormal = Coord3( SI().binID2Coord().colDir(), 0 );

	const Coord3 nzednor = editnormal.normalize();
	const Coord3 stkednor = f3d.geometry().getEditPlaneNormal(sid,rc.row);

	const bool equinormal =
	    mIsEqual(nzednor.x,stkednor.x,.001) &&
	    mIsEqual(nzednor.y,stkednor.y,.001) &&
	    mIsEqual(nzednor.z,stkednor.z,.00001);

	if ( !equinormal ) continue;	

	// we need to deal in different way if cs direction is Z
	if ( cs_.defaultDir() != CubeSampling::Z )
	{
	    BinID extrbid1, extrbid2;
	    if ( cs_.defaultDir() == CubeSampling::Inl )
	    {
		extrbid1.inl = extrbid2.inl = cs_.hrg.inlRange().start;
		extrbid1.crl = cs_.hrg.crlRange().start;
		extrbid2.crl = cs_.hrg.crlRange().stop;
	    }
	    else if ( cs_.defaultDir() == CubeSampling::Crl )
	    {
		extrbid1.inl = cs_.hrg.inlRange().start;
		extrbid2.inl = cs_.hrg.inlRange().stop;
		extrbid1.crl = extrbid2.crl = cs_.hrg.crlRange().start;
	    }
	    
	    Coord extrcoord1, extrcoord2;
	    extrcoord1 = SI().transform( extrbid1 );
	    extrcoord2 = SI().transform( extrbid2 );

	    for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col+=colrg.step )
	    {
		const Coord3& pos = fss->getKnot( rc );
		BinID knotbinid = SI().transform( pos );
		if ( pointOnEdge2D(pos.coord(),extrcoord1,extrcoord2,.5)
		     || (cs_.defaultDir()==CubeSampling::Inl
		         && knotbinid.inl==extrbid1.inl)
	   	     || (cs_.defaultDir()==CubeSampling::Crl
			 && knotbinid.crl==extrbid1.crl) )
		{
		    if ( cs_.defaultDir() == CubeSampling::Inl )
			stickauxdata->poly_ += FlatView::Point(
					SI().transform(pos.coord()).crl, pos.z);
		    else if ( cs_.defaultDir() == CubeSampling::Crl )
			stickauxdata->poly_ += FlatView::Point(
					SI().transform(pos.coord()).inl, pos.z);
		}
	    }
	}
	else
	{
	    for ( rc.col=colrg.start; rc.col<=colrg.stop; 
				      rc.col+=colrg.step )
	    {
		const Coord3 pos = fss->getKnot( rc );
		if ( !mIsEqual(pos.z,cs_.zrg.start,.0001) )
		    break;

		BinID binid = SI().transform(pos.coord());
		stickauxdata->poly_ +=
			FlatView::Point( binid.inl, binid.crl );
	    }
	}

	if ( stickauxdata->poly_.size() == 0 )
		delete stickauxdata;
	    else
	    {
		StkMarkerInfo* stkmkrinfo = new StkMarkerInfo;
		stkmkrinfo->marker_ = stickauxdata;
		stkmkrinfo->stickid_ = rc.row;
		f3dmaker->stickmarker_ += stkmkrinfo;
		viewer_.appearance().annot_.auxdata_ += stickauxdata;
	    }
    }

    return true;
}


bool Fault3DPainter::paintIntersection( EM::Fault3D& f3d,
					const EM::SectionID& sid,
					Fault3DMarker* f3dmaker )
{
    PtrMan<Geometry::IndexedShape> faultsurf =
		    new Geometry::ExplFaultStickSurface(
			f3d.geometry().sectionGeometry(sid), SI().zScale() );
    //faultsurf->setCoordList( new Coord3ListImpl, new Coord3ListImpl );
    if ( !faultsurf->update(true,0) )
	return false;
    
    BinID start( cs_.hrg.start.inl, cs_.hrg.start.crl );
    BinID stop(cs_.hrg.stop.inl, cs_.hrg.stop.crl );

    Coord3 p0( SI().transform(start), cs_.zrg.start );
    Coord3 p1( SI().transform(start), cs_.zrg.stop );
    Coord3 p2( SI().transform(stop), cs_.zrg.stop );
    Coord3 p3( SI().transform(stop), cs_.zrg.start );

    TypeSet<Coord3> pts;

    pts += p0; pts += p1; pts += p2; pts += p3;

    const Coord3 normal = (p1-p0).cross(p3-p0).normalize();

    PtrMan<Geometry::ExplPlaneIntersection> intersectn = 
					new Geometry::ExplPlaneIntersection;
    intersectn->setShape( *faultsurf );
    intersectn->addPlane( normal, pts );

    Geometry::IndexedShape* idxshape = intersectn;
    //idxshape->setCoordList( new Coord3ListImpl, new Coord3ListImpl );

    if ( !idxshape->update(true,0) )
	return false;

    if ( !idxshape->getGeometry().size() )
	return false;

    const Geometry::IndexedGeometry* idxgeom = idxshape->getGeometry()[0];
    TypeSet<int> coordindices = idxgeom->coordindices_;

    if ( !coordindices.size() )
	return false;

    Coord3List* clist = idxshape->coordList();
    mDynamicCastGet(Coord3ListImpl*,intersecposlist,clist);
    TypeSet<Coord3> intersecposs;
    if ( intersecposlist->getSize() )
    {
	int nextposid = intersecposlist->nextID( -1 );
	while ( nextposid!=-1 )
	{
	    if ( intersecposlist->isDefined(nextposid) )
		intersecposs += intersecposlist->get( nextposid );
	    nextposid = intersecposlist->nextID( nextposid );
	}
    }

    FlatView::Annotation::AuxData* intsecauxdat = 
	    				new FlatView::Annotation::AuxData( 0 );
    intsecauxdat->poly_.erase();
    intsecauxdat->linestyle_ = markerlinestyle_;
    intsecauxdat->linestyle_.width_ = markerlinestyle_.width_/2;
    intsecauxdat->linestyle_.color_ = f3d.preferredColor();
    intsecauxdat->enabled_ = linenabled_;

    for ( int idx=0; idx<coordindices.size(); idx++ )
    {
	if ( coordindices[idx] == -1 )
	{
	    viewer_.appearance().annot_.auxdata_ += intsecauxdat;
	    f3dmaker->intsecmarker_ += intsecauxdat;
	    intsecauxdat = new FlatView::Annotation::AuxData( 0 );
	    intsecauxdat->poly_.erase();
	    intsecauxdat->linestyle_ = markerlinestyle_;
	    intsecauxdat->linestyle_.width_ = markerlinestyle_.width_/2;
	    intsecauxdat->linestyle_.color_ = f3d.preferredColor();
	    intsecauxdat->enabled_ = linenabled_;
	    continue;
	}

	const Coord3 pos = intersecposs[coordindices[idx]];
	BinID posbid =  SI().transform( pos.coord() );
	if ( cs_.nrZ() == 1 )
	    intsecauxdat->poly_ += FlatView::Point( posbid.inl,
						    posbid.crl );
	else if ( cs_.nrCrl() == 1 )
	    intsecauxdat->poly_ += FlatView::Point( posbid.inl, pos.z );
	else if ( cs_.nrInl() == 1 )
	    intsecauxdat->poly_ += FlatView::Point( posbid.crl, pos.z );
    }

    viewer_.appearance().annot_.auxdata_ += intsecauxdat;
    f3dmaker->intsecmarker_ += intsecauxdat;

    return true;
}


void Fault3DPainter::enableLine( bool yn )
{
    if ( linenabled_ == yn )
	return;

    for ( int markidx=f3dmarkers_.size()-1; markidx>=0; markidx-- )
    {
	Fault3DMarker* marklns = f3dmarkers_[markidx];
	
	for ( int idxstk=marklns->stickmarker_.size()-1; idxstk>=0; idxstk-- )
	{
	    marklns->stickmarker_[idxstk]->marker_->enabled_ = yn;
	}

	for ( int idxitr=marklns->intsecmarker_.size()-1; idxitr>=0; idxitr-- )
	{
	    marklns->intsecmarker_[idxitr]->enabled_ = yn;
	}
    }

    linenabled_ = yn;
    viewer_.handleChange( FlatView::Viewer::Annot );
}


void Fault3DPainter::enableKnots( bool yn )
{
    if ( knotenabled_ == yn )
	return;

    for ( int markidx=f3dmarkers_.size()-1; markidx>=0; markidx-- )
    {
	Fault3DMarker* marklns = f3dmarkers_[markidx];

	for ( int idxstk=marklns->stickmarker_.size()-1; idxstk>=0; idxstk-- )
	{
	    if ( !yn )
		marklns->stickmarker_[idxstk]->marker_->markerstyles_.erase();
	    else
		marklns->stickmarker_[idxstk]->marker_->markerstyles_ += 
		    						markerstyle_;
	}
    }

    knotenabled_ = yn;
    viewer_.handleChange( FlatView::Viewer::Annot );
}


void Fault3DPainter::setActiveStick( EM::PosID& pid )
{
    if ( pid.objectID() != emid_ ) return;

    if ( pid.getRowCol().row == activestickid_ ) return;

    for ( int auxdid=0; auxdid<f3dmarkers_[0]->stickmarker_.size(); auxdid++ )
    {
	LineStyle& linestyle = 
	    f3dmarkers_[0]->stickmarker_[auxdid]->marker_->linestyle_;
	if ( f3dmarkers_[0]->stickmarker_[auxdid]->stickid_== activestickid_ )
	    linestyle.width_ = markerlinestyle_.width_;
	else if ( f3dmarkers_[0]->stickmarker_[auxdid]->stickid_ ==
		  pid.getRowCol().row )
	    linestyle.width_ = markerlinestyle_.width_ * 2;
    }

    activestickid_ = pid.getRowCol().row;
    viewer_.handleChange( FlatView::Viewer::Annot );
}


bool Fault3DPainter::hasDiffActiveStick( const EM::PosID* pid ) const
{
    if ( pid->objectID() != emid_ ||
	 pid->getRowCol().row != activestickid_ )
	return true;
    else
	return false;
}


FlatView::Annotation::AuxData* Fault3DPainter::getAuxData(
						const EM::PosID* pid) const
{
    if ( pid->objectID() != emid_ )
	return 0;

    return f3dmarkers_[0]->stickmarker_[activestickid_]->marker_;
}


void Fault3DPainter::getDisplayedSticks( ObjectSet<StkMarkerInfo>& dispstkinfo )
{
    if ( !f3dmarkers_.size() ) return;

    dispstkinfo = f3dmarkers_[0]->stickmarker_;
}


void Fault3DPainter::removePolyLine()
{
    for ( int markidx=f3dmarkers_.size()-1; markidx>=0; markidx-- )
    {
	Fault3DMarker* f3dmarker = f3dmarkers_[markidx];
	for ( int idi=f3dmarker->intsecmarker_.size()-1; idi>=0; idi-- )
	    viewer_.appearance().annot_.auxdata_ -= 
					f3dmarker->intsecmarker_[idi];
	for ( int ids=f3dmarker->stickmarker_.size()-1; ids>=0; ids-- )
	    viewer_.appearance().annot_.auxdata_ -= 
				f3dmarker->stickmarker_[ids]->marker_;
    }

    deepErase( f3dmarkers_ );
}


void Fault3DPainter::repaintFault3D()
{
    removePolyLine();
    addPolyLine();
    viewer_.handleChange( FlatView::Viewer::Annot );
}

void Fault3DPainter::fault3DChangedCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( const EM::EMObjectCallbackData&,
	    			cbdata, caller, cb );
    mDynamicCastGet(EM::EMObject*,emobject,caller);
    if ( !emobject ) return;

    mDynamicCastGet(EM::Fault3D*,emf3d,emobject);
    if ( !emf3d ) return;

    if ( emobject->id() != emid_ ) return;

    switch ( cbdata.event )
    {
	case EM::EMObjectCallbackData::Undef:
	    break;
	case EM::EMObjectCallbackData::PrefColorChange:
	    break;
	case EM::EMObjectCallbackData::PositionChange:
	{
	    if ( emf3d->hasBurstAlert() )
		return;
	    abouttorepaint_.trigger();
	    repaintFault3D();
	    repaintdone_.trigger();
	    break;
	}
	case EM::EMObjectCallbackData::BurstAlert:
	{
	    if (  emobject->hasBurstAlert() )
		return;
	    abouttorepaint_.trigger();
	    repaintFault3D();
	    repaintdone_.trigger();
	    break;
	}
	default: break;
    }
}


} //namespace EM

