/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisurvmap.h"

#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uifont.h"
#include "uiworld2ui.h"

#include "trckeyzsampling.h"
#include "draw.h"
#include "survinfo.h"
#include "angles.h"


uiSurveyBoxObject::uiSurveyBoxObject( BaseMapObject* bmo, bool withlabels )
    : uiBaseMapObject(bmo)
{
    for ( int idx=0; idx<4; idx++ )
    {
        uiMarkerItem* markeritem = new uiMarkerItem( MarkerStyle2D::Square );
	markeritem->setZValue( 1 );
	itemgrp_.add( markeritem );
	vertices_ += markeritem;
    }

    Color red( 255, 0, 0, 0);
    LineStyle ls( LineStyle::Solid, 3, red );
    for ( int idx=0; idx<4; idx++ )
    {
	uiLineItem* lineitem = new uiLineItem();
	lineitem->setPenColor( red );
	lineitem->setPenStyle( ls );
	itemgrp_.add( lineitem );
	edges_ += lineitem;
    }

    if ( !withlabels )
	return;

    const mDeclAlignment( postxtalign, HCenter, VCenter );
    for ( int idx=0; idx<4; idx++ )
    {
        uiTextItem* textitem = new uiTextItem();
	textitem->setTextColor( Color::Black() );
	textitem->setAlignment( postxtalign );
	textitem->setFont(
		FontList().get(FontData::key(FontData::GraphicsSmall)) );
	textitem->setZValue( 1 );
	itemgrp_.add( textitem );
	labels_ += textitem;
    }
}


void uiSurveyBoxObject::setSurveyInfo( const SurveyInfo* si )
{
    survinfo_ = si;
}


void uiSurveyBoxObject::setVisibility( bool yn )
{
    itemGrp().setVisible( yn );
}


void uiSurveyBoxObject::update()
{
    if ( !survinfo_ || !transform_ )
	{ setVisibility( false ); return; }

    const SurveyInfo& si = *survinfo_;
    const TrcKeyZSampling& cs = si.sampling( false );
    Coord mapcnr[4];
    mapcnr[0] = si.transform( cs.hrg.start );
    mapcnr[1] = si.transform( BinID(cs.hrg.start.inl(),cs.hrg.stop.crl()) );
    mapcnr[2] = si.transform( cs.hrg.stop );
    mapcnr[3] = si.transform( BinID(cs.hrg.stop.inl(),cs.hrg.start.crl()) );

    uiPoint cpt[4];
    for ( int idx=0; idx<vertices_.size(); idx++ )
    {
        cpt[idx] = transform_->transform( uiWorldPoint(mapcnr[idx].x,
						      mapcnr[idx].y) );
	vertices_[idx]->setPos( cpt[idx] );
    }

    for ( int idx=0; idx<edges_.size(); idx++ )
	edges_[idx]->setLine( cpt[idx], idx!=3 ? cpt[idx+1] : cpt[0] );

    for ( int idx=0; idx<labels_.size(); idx++ )
    {
	const int oppidx = idx < 2 ? idx + 2 : idx - 2;
	const bool bot = cpt[idx].y > cpt[oppidx].y;
        BinID bid = si.transform( mapcnr[idx] );
	Alignment al( Alignment::HCenter,
		      bot ? Alignment::Top : Alignment::Bottom );
	uiPoint txtpos( cpt[idx].x, cpt[idx].y );
	labels_[idx]->setPos( txtpos );
	labels_[idx]->setText( bid.toString() );
	labels_[idx]->setAlignment( al );
    }

    setVisibility( true );
}


uiNorthArrowObject::uiNorthArrowObject( BaseMapObject* bmo, bool withangle )
    : uiBaseMapObject(bmo)
    , angleline_(0),anglelabel_(0)
{
    ArrowStyle arrowstyle( 3, ArrowStyle::HeadOnly );
    arrowstyle.linestyle_.width_ = 3;
    arrow_ = new uiArrowItem;
    arrow_->setArrowStyle( arrowstyle );
    itemgrp_.add( arrow_ );

    if ( !withangle )
	return;

    angleline_ = new uiLineItem;
    angleline_->setPenStyle( LineStyle(LineStyle::Dot,2,Color(255,0,0)) );
    itemgrp_.add( angleline_ );

    mDeclAlignment( txtalign, Right, Bottom );
    anglelabel_ = new uiTextItem();
    anglelabel_->setAlignment( txtalign );
    itemgrp_.add( anglelabel_ );
}


void uiNorthArrowObject::setSurveyInfo( const SurveyInfo* si )
{
    survinfo_ = si;
}


void uiNorthArrowObject::setVisibility( bool yn )
{
    itemGrp().setVisible( yn );
}


void uiNorthArrowObject::update()
{
    if ( !survinfo_ || !transform_ )
	{ setVisibility( false ); return; }

    float mathang = survinfo_->angleXInl();
    if ( mIsUdf(mathang) ) return;

	    // To [0,pi]
    if ( mathang < 0 )			mathang += M_PIf;
    if ( mathang > M_PIf )		mathang -= M_PIf;
	    // Find angle closest to N, not necessarily X vs inline
    if ( mathang < M_PI_4f )		mathang += M_PI_2f;
    if ( mathang > M_PI_2f+M_PI_4f )	mathang -= M_PI_2f;

    float usrang = Angle::rad2usrdeg( mathang );
    if ( usrang > 180 ) usrang = 360 - usrang;

    const bool northisleft = mathang < M_PI_2f;
    const int arrowlen = 30;
    const int sideoffs = 10;
    const int yarrowtop = 20;

    float dx = arrowlen * tan( M_PI_2f-mathang );
    const int dxpix = mNINT32( dx );
    float worldxmin, worldxmax;
    transform_->getWorldXRange( worldxmin, worldxmax );
    const int xmax = transform_->toUiX( worldxmax );
    const int lastx = xmax - 1 - sideoffs;
    const uiPoint origin( lastx - (northisleft?dxpix:0), arrowlen + yarrowtop );
    const uiPoint arrowtop( origin.x, yarrowtop );

    arrow_->setTailHeadPos( origin, arrowtop );
    if ( !angleline_ || !anglelabel_ )
	return;

    angleline_->setLine( origin, uiPoint(origin.x+dxpix,yarrowtop) );
    float usrang100 = usrang * 100;
    if ( usrang100 < 0 ) usrang100 = -usrang100;
    int iusrang = (int)(usrang100 + .5);
    BufferString angtxt;
    if ( iusrang )
    {
	angtxt += iusrang / 100;
	iusrang = iusrang % 100;
	if ( iusrang )
	{
	    angtxt += ".";
	    angtxt += iusrang / 10; iusrang = iusrang % 10;
	    if ( iusrang )
		angtxt += iusrang;
	}
    }

    anglelabel_->setPos( mCast(float,lastx), mCast(float,yarrowtop) );
    anglelabel_->setText( angtxt );
    setVisibility( true );
}


uiMapScaleObject::uiMapScaleObject( BaseMapObject* bmo )
    : uiBaseMapObject(bmo)
    , scalestyle_(*new LineStyle(LineStyle::Solid,1,Color::Black()))
{
    scalelen_ = (float)( 0.05 * ( SI().maxCoord(false).x - 
				  SI().minCoord(false).x ) );
    scalelen_ = (float)( 100 * mCast(int,scalelen_ / 100) );

    scaleline_ = new uiLineItem;
    itemgrp_.add( scaleline_ );

    leftcornerline_ = new uiLineItem;
    itemgrp_.add( leftcornerline_ );

    rightcornerline_ = new uiLineItem;
    itemgrp_.add( rightcornerline_ );

    mDeclAlignment( txtalign, HCenter, Top );

    scalelabelorigin_ = new uiTextItem;
    scalelabelorigin_->setAlignment( txtalign );
    itemgrp_.add( scalelabelorigin_ );

    scalelabelend_ = new uiTextItem;
    scalelabelend_->setAlignment( txtalign );
    itemgrp_.add( scalelabelend_ );
}


void uiMapScaleObject::setSurveyInfo( const SurveyInfo* si )
{
    survinfo_ = si;
}


void uiMapScaleObject::setVisibility( bool yn )
{
    itemGrp().setVisible( yn );
}


void uiMapScaleObject::update()
{
    if ( !survinfo_ || !transform_ )
	{ setVisibility( false ); return; }

    const float worldscalelen = scalelen_;
    const int sideoffs = 30;
    const int scalecornerlen = 2;

    float worldxmin, worldxmax;
    transform_->getWorldXRange( worldxmin, worldxmax );

    float worldymin, worldymax;
    transform_->getWorldYRange( worldymin, worldymax );

    const int xmax = transform_->toUiX( worldxmax );
    const int ymin = transform_->toUiY( worldymin );

    const float worldref = worldxmax - worldscalelen;
    const float uiscalelen = (float)xmax - transform_->toUiX( worldref );

    const int lastx = xmax - 1 - sideoffs;
    const int firsty = ymin - 1;

    const Geom::Point2D<float> origin( (float)lastx - uiscalelen, 
				       (float)firsty );
    const Geom::Point2D<float> end( (float)lastx, (float)firsty );
    scaleline_->setLine( origin, end );
    scaleline_->setPenStyle( scalestyle_ );

    leftcornerline_->setLine( origin , 0.0f, (float)scalecornerlen,
				       0.0f, (float)scalecornerlen );
    leftcornerline_->setPenStyle( scalestyle_ );

    rightcornerline_->setLine( end , 0.0f, (float)scalecornerlen,
				     0.0f, (float)scalecornerlen );
    rightcornerline_->setPenStyle( scalestyle_ );

    BufferString label_origin = "0";
    BufferString label_end; label_end.set( worldscalelen, 0 );
    label_end += survinfo_->getXYUnitString( false );

    scalelabelorigin_->setPos( origin );
    scalelabelorigin_->setText( label_origin );

    scalelabelend_->setPos( end );
    scalelabelend_->setText( label_end );

    setVisibility( true );
}


void uiMapScaleObject::setScaleLen( const float scalelen )
{
    scalelen_ = scalelen;
    update();
}

void uiMapScaleObject::setScaleStyle( const LineStyle& ls )
{
    scalestyle_ = ls;
    update();
}

uiSurveyMap::uiSurveyMap( uiParent* p, bool withtitle,
			  bool withnortharrow, bool withmapscale )
    : uiBaseMap(p)
    , survbox_(0)
    , northarrow_(0)
    , mapscale_(0)
    , survinfo_(0)
    , title_(0)
{
    view_.setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    view_.setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );
    const mDeclAlignment( txtalign, Left, Top );

    survbox_ = new uiSurveyBoxObject( 0, true );
    addObject( survbox_ );

    if ( withnortharrow )
    {
	northarrow_ = new uiNorthArrowObject( 0, false );
	addObject( northarrow_ );
    }

    if ( withmapscale )
    {
	mapscale_ = new uiMapScaleObject( 0 );
	addObject( mapscale_ );
    }

    if ( withtitle )
    {
	title_ = view_.scene().addItem(
		new uiTextItem(uiPoint(10,10),"Survey name",txtalign) );
	title_->setPenColor( Color::Black() );
	title_->setFont(
		FontList().get(FontData::key(FontData::GraphicsLarge)) );
    }
}


void uiSurveyMap::setSurveyInfo( const SurveyInfo* si )
{
    survinfo_ = si;

    if ( survbox_ )
	survbox_->setSurveyInfo( survinfo_ );
    if ( northarrow_ )
	northarrow_->setSurveyInfo( survinfo_ );
    if ( mapscale_ )
	mapscale_->setSurveyInfo( survinfo_ );
    if ( title_ )
	title_->setVisible( survinfo_ );

    if ( survinfo_ )
    {
	view_.setViewArea( 0, 0, view_.scene().width(),
				 view_.scene().height() );

	uiBorder border( 20, title_ ? 70 : 20, 20, 20 );
	uiSize sz( (int)view_.scene().width(), (int)view_.scene().height() );
	uiRect rc = border.getRect( sz );
	w2ui_.set( rc, *survinfo_ );
	if ( title_ ) title_->setText( survinfo_->name().buf() );
    }

    uiBaseMap::reDraw();
}


void uiSurveyMap::reDraw( bool )
{
    setSurveyInfo( survinfo_ );
}
