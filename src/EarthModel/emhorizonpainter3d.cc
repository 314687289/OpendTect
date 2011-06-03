/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
 RCS:		$Id: emhorizonpainter3d.cc,v 1.6 2011-06-03 14:00:21 cvsbruno Exp $
________________________________________________________________________

-*/

#include "emhorizonpainter3d.h"

#include "emhorizon3d.h"
#include "emmanager.h"

namespace EM
{

HorizonPainter3D::HorizonPainter3D( FlatView::Viewer& fv,
				    const EM::ObjectID& oid )
    : viewer_(fv)
    , id_(oid)
    , markerlinestyle_(LineStyle::Solid,2,Color(0,255,0))
    , markerstyle_(MarkerStyle2D::Square, 4, Color::White())
    , linenabled_(true)
    , seedenabled_(true)
    , markerseeds_(0)
    , abouttorepaint_(this)
    , repaintdone_(this)
{
    EM::EMObject* emobj = EM::EMM().getObject( id_ );
    if ( emobj )
    {
	emobj->ref();
	emobj->change.notify( mCB(this,HorizonPainter3D,horChangeCB) );
    }
}


HorizonPainter3D::~HorizonPainter3D()
{
    EM::EMObject* emobj = EM::EMM().getObject( id_ );
    if ( emobj )
    {
	emobj->change.remove( mCB(this,HorizonPainter3D,horChangeCB) );
	emobj->unRef();
    }

    removePolyLine();
    viewer_.handleChange( FlatView::Viewer::Annot );
}


void HorizonPainter3D::setCubeSampling( const CubeSampling& cs, bool update )
{
    cs_ = cs;
}


void HorizonPainter3D::paint()
{
    removePolyLine();
    addPolyLine();
    viewer_.handleChange( FlatView::Viewer::Annot );
}


bool HorizonPainter3D::addPolyLine()
{
    EM::EMObject* emobj = EM::EMM().getObject( id_ );
    if ( !emobj ) return false;
    
    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj);
    if ( !hor3d ) return false;
    
    for ( int ids=0; ids<hor3d->nrSections(); ids++ )
    {
	EM::SectionID sid( ids );
	SectionMarker3DLine* secmarkerln = new SectionMarker3DLine;
	markerline_ += secmarkerln;
	FlatView::Annotation::AuxData* seedauxdata =
	    				new FlatView::Annotation::AuxData( "" );
	
	seedauxdata->enabled_ = seedenabled_;
	seedauxdata->poly_.erase();
	seedauxdata->markerstyles_ += markerstyle_;
	viewer_.appearance().annot_.auxdata_ += seedauxdata;

	markerseeds_ = new Marker3D;
	markerseeds_->marker_ = seedauxdata;
	markerseeds_->sectionid_ = sid;
	
	bool newmarker = true;
	bool coorddefined = true;
	
	Marker3D* marker = 0;
	HorSamplingIterator iter( cs_.hrg );
	
	BinID bid;
	while ( iter.next(bid) )
	{
	    int inlfromcs = bid.inl;
	    const Coord3 crd = hor3d->getPos( sid, bid.toInt64() );
	    EM::PosID posid( id_, sid, bid.toInt64() );
	    
	    if ( !crd.isDefined() )
	    {
		coorddefined = false;
		bid.inl = inlfromcs;
		continue;
	    }
	    else if ( !coorddefined )
	    {
		coorddefined = true;
		newmarker = true;
	    }
	    
	    if ( newmarker )
	    {
		FlatView::Annotation::AuxData* auxdata =
		new FlatView::Annotation::AuxData( "" );
		viewer_.appearance().annot_.auxdata_ += auxdata;
		auxdata->poly_.erase();
		auxdata->linestyle_ = markerlinestyle_;
		Color prefcol = hor3d->preferredColor();
		prefcol.setTransparency( 0 );
		auxdata->linestyle_.color_ = prefcol;
		auxdata->fillcolor_ = prefcol;
		auxdata->enabled_ = linenabled_;
		auxdata->name_ = hor3d->name();
		marker = new Marker3D;
		(*secmarkerln) += marker;
		marker->marker_ = auxdata;
		marker->sectionid_ = sid;
		newmarker = false;
	    }
	    if ( cs_.nrInl() == 1 )
	    {
		marker->marker_->poly_ += FlatView::Point( bid.crl, crd.z );
		if ( hor3d->isPosAttrib(posid,EM::EMObject::sSeedNode()) )
		    markerseeds_->marker_->poly_ +=
					FlatView::Point( bid.crl, crd.z );
	    }
	    else if ( cs_.nrCrl() == 1 )
	    {
		marker->marker_->poly_ += FlatView::Point( bid.inl, crd.z );
		if ( hor3d->isPosAttrib(posid,EM::EMObject::sSeedNode()) )
		    markerseeds_->marker_->poly_ +=
					FlatView::Point( bid.inl, crd.z );
	    }
	}
    }
    
    return true;
}


void HorizonPainter3D::horChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( const EM::EMObjectCallbackData&,
	    			cbdata, caller, cb );
    mDynamicCastGet(EM::EMObject*,emobject,caller);
    if ( !emobject ) return;

    switch ( cbdata.event )
    {
	case EM::EMObjectCallbackData::Undef:
	    break;
	case EM::EMObjectCallbackData::PrefColorChange:
	    {
		changePolyLineColor();
		break;
	    }
	case EM::EMObjectCallbackData::PositionChange:
	    {
		if ( emobject->hasBurstAlert() )
		    return;
		
		BinID bid;
		bid.fromInt64( cbdata.pid0.subID() );
		if ( cs_.hrg.includes(bid) )
		{
		    changePolyLinePosition( cbdata.pid0 );
		    viewer_.handleChange( FlatView::Viewer::Annot );
		}
		
		break;
	    }
	case EM::EMObjectCallbackData::BurstAlert:
	    {
		if ( emobject->hasBurstAlert() )
		    return;

		abouttorepaint_.trigger();
		repaintHorizon();
		repaintdone_.trigger();
		break;
	    }
	default: break;
    }
}


void HorizonPainter3D::getDisplayedHor( ObjectSet<Marker3D>& disphor )
{
    for ( int secidx=0; secidx<markerline_.size(); secidx++ )
	disphor.append( *markerline_[secidx] );

    if ( seedenabled_ )
	disphor += markerseeds_;
}


void HorizonPainter3D::repaintHorizon()
{
    removePolyLine();
    addPolyLine();
    viewer_.handleChange( FlatView::Viewer::Annot );
}


void HorizonPainter3D::changePolyLineColor()
{
    EM::EMObject* emobj = EM::EMM().getObject( id_ );
    if ( !emobj ) return;
    
    for ( int idx=0; idx<markerline_.size(); idx++ )
    {
	SectionMarker3DLine* secmarkerlines = markerline_[idx];
	Color prefcol = emobj->preferredColor();
	prefcol.setTransparency( 0 );

	for ( int markidx=0; markidx<secmarkerlines->size(); markidx++ )
	    (*secmarkerlines)[markidx]->marker_->linestyle_.color_ = prefcol;
    }

    viewer_.handleChange( FlatView::Viewer::Annot );
}


void HorizonPainter3D::changePolyLinePosition( const EM::PosID& pid )
{
    mDynamicCastGet(EM::Horizon3D*,hor3d,EM::EMM().getObject( id_ ));
    if ( !hor3d ) return;

    if ( id_ != pid.objectID() ) return;

    BinID binid;
    binid.fromInt64( pid.subID() );

    for ( int idx=0; idx<hor3d->nrSections(); idx++ )
    {
	if ( hor3d->sectionID(idx) != pid.sectionID() )
	    continue;

	SectionMarker3DLine* secmarkerlines = markerline_[idx];
	for ( int markidx=0; markidx<secmarkerlines->size(); markidx++ )
	{
	    Coord3 crd = hor3d->getPos( hor3d->sectionID(idx), pid.subID() );
	    FlatView::Annotation::AuxData* auxdata =
					(*secmarkerlines)[markidx]->marker_;
	    for ( int posidx = 0; posidx < auxdata->poly_.size(); posidx ++ )
	    {
		if ( cs_.nrInl() == 1 )
		{
		    if ( binid.crl == auxdata->poly_[posidx].x )
		    {
			auxdata->poly_[posidx].y = crd.z;
			return;
		    }
		}
		else if ( cs_.nrCrl() == 1 )
		{
		    if ( binid.inl == auxdata->poly_[posidx].x )
		    {
			auxdata->poly_[posidx].y = crd.z;
			return;
		    }
		}
	    }
	    
	    if ( crd.isDefined() )
	    {
		if ( cs_.nrInl() == 1 )
		    auxdata->poly_ += FlatView::Point( binid.crl, crd.z );
		else if ( cs_.nrCrl() == 1 )
		    auxdata->poly_ += FlatView::Point( binid.inl, crd.z );
	    }
	}
    }
}


void HorizonPainter3D::removePolyLine()
{
    for ( int markidx=markerline_.size()-1;  markidx>=0; markidx-- )
    {
	SectionMarker3DLine* markerlines = markerline_[markidx];
	for ( int idy=markerlines->size()-1; idy>=0; idy-- )
	    viewer_.appearance().annot_.auxdata_ -=
						(*markerlines)[idy]->marker_;
    }

    deepErase( markerline_ );

    if ( markerseeds_ )
    {
	viewer_.appearance().annot_.auxdata_ -= markerseeds_->marker_;
	delete markerseeds_;
	markerseeds_ = 0;
    }
}


void HorizonPainter3D::enableLine( bool yn )
{
    if ( linenabled_ == yn )
	return;

    for ( int markidx=markerline_.size()-1;  markidx>=0; markidx-- )
    {
	SectionMarker3DLine* markerlines = markerline_[markidx];
	for ( int idy=markerlines->size()-1; idy>=0; idy-- )
	{
	    (*markerlines)[idy]->marker_->enabled_ = yn;
	}
    }

    linenabled_ = yn;
    viewer_.handleChange( FlatView::Viewer::Annot );

}


void HorizonPainter3D::enableSeed( bool yn )
{
    if ( seedenabled_ == yn )
	return;

    if ( !markerseeds_ ) return;
    markerseeds_->marker_->enabled_ = yn;
    seedenabled_ = yn;
    viewer_.handleChange( FlatView::Viewer::Annot );
}

} // namespace EM
