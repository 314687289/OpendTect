/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uidatapointsetcrossplot.cc,v 1.22 2008-12-12 06:01:17 cvssatyaki Exp $
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uidatapointsetcrossplot.cc,v 1.22 2008-12-12 06:01:17 cvssatyaki Exp $";

#include "uidatapointsetcrossplotwin.h"
#include "uidpscrossplotpropdlg.h"
#include "uidatapointset.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uirgbarray.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uispinbox.h"
#include "uitoolbar.h"

#include "draw.h"
#include "datapointset.h"
#include "datainpspec.h"
#include "envvars.h"
#include "iodrawtool.h"
#include "linear.h"
#include "polygon.h"
#include "randcolor.h"
#include "rowcol.h"
#include "statruncalc.h"
#include "sorting.h"
#include "settings.h"

static const int cMaxPtsForMarkers = 20000;
static const int cMaxPtsForDisplay = 100000;

uiDataPointSetCrossPlotter::Setup uiDataPointSetCrossPlotWin::defsetup_;

uiDataPointSetCrossPlotter::Setup::Setup()
    : noedit_(false)
    , markerstyle_(MarkerStyle2D::Square)
    , xstyle_(LineStyle::Solid,1,Color::Black)
    , ystyle_(LineStyle::Solid,1,Color::stdDrawColor(0))
    , y2style_(LineStyle::Dot,1,Color::stdDrawColor(1))
    , minborder_(10,20,20,5)
    , showcc_(true)
    , showregrline_(false)
    , showy1userdefline_(false)
    , showy2userdefline_(false)
{
}


uiDataPointSetCrossPlotter::AxisData::AxisData( uiDataPointSetCrossPlotter& cp,
						uiRect::Side s )
    : uiAxisData( s )
    , cp_( cp )
{
}


void uiDataPointSetCrossPlotter::AxisData::stop()
{
    if ( isreset_ ) return;
    colid_ = cp_.mincolid_ - 1;
    uiAxisData::stop();
}


void uiDataPointSetCrossPlotter::AxisData::setCol( DataPointSet::ColID cid )
{
    if ( axis_ && cid == colid_ )
	return;

    stop();
    colid_ = cid;
    newColID();
}


void uiDataPointSetCrossPlotter::AxisData::newColID()
{
    if ( colid_ < cp_.mincolid_ )
	return;

    renewAxis( cp_.uidps_.userName(colid_), &cp_.scene(), cp_.width_,
	       cp_.height_, 0 );
    handleAutoScale( cp_.uidps_.getRunCalc( colid_ ) );
}


uiDataPointSetCrossPlotter::uiDataPointSetCrossPlotter( uiParent* p,
			    uiDataPointSet& uidps,
			    const uiDataPointSetCrossPlotter::Setup& su )
    : uiGraphicsView(p,"Data PointSet CrossPlotter" )
    , dps_(uidps.pointSet())
    , uidps_(uidps)
    , parent_(p)
    , setup_(su)
    , meh_(scene().getMouseEventHandler())
    , mincolid_(1-uidps.pointSet().nrFixedCols())
    , selrow_(-1)
    , x_(*this,uiRect::Bottom)
    , y_(*this,uiRect::Left)
    , y2_(*this,uiRect::Right)
    , selectionChanged( this )
    , pointsSelected( this )
    , removeRequest( this )
    , doy2_(true)
    , dobd_(false)
    , eachrow_(1)
    , curgrp_(0)
    , selrowisy2_(false)
    , rectangleselection_(true)
    , lsy1_(*new LinStats2D)
    , lsy2_(*new LinStats2D)
    , userdefy1lp_(*new LinePars)
    , userdefy2lp_(*new LinePars)
    , yptitems_(0)
    , y2ptitems_(0)
    , selectedypts_(0)
    , selectedy2pts_(0)
    , selecteditems_(0)
    , odselectedpolygon_(0)
    , selectionpolygonitem_(0)
    , regrlineitm_(0)
    , y1userdeflineitm_(0)
    , y2userdeflineitm_(0)
    , width_(600)
    , height_(500)
    , selectable_(false)
    , isy1selectable_(true)
    , isy2selectable_(false)
    , mousepressed_(false)
{
    x_.defaxsu_.style_ = setup_.xstyle_;
    y_.defaxsu_.style_ = setup_.ystyle_;
    y2_.defaxsu_.style_ = setup_.y2style_;
    x_.defaxsu_.border_ = setup_.minborder_;
    y_.defaxsu_.border_ = setup_.minborder_;
    y2_.defaxsu_.border_ = setup_.minborder_;

    selecteditems_ = new uiGraphicsItemGroup();
    meh_.buttonPressed.notify( mCB(this,uiDataPointSetCrossPlotter,mouseClick));
    meh_.buttonReleased.notify( mCB(this,uiDataPointSetCrossPlotter,mouseRel));
    initDraw();
    drawContent();

    reSize.notify( mCB(this,uiDataPointSetCrossPlotter,reDraw) );
    reDrawNeeded.notify( mCB(this,uiDataPointSetCrossPlotter,reDraw) );
    getMouseEventHandler().buttonPressed.notify(
	    mCB(this,uiDataPointSetCrossPlotter,getSelStarPos) );
    getMouseEventHandler().movement.notify(
	    mCB(this,uiDataPointSetCrossPlotter,drawPolygon) );
    getMouseEventHandler().buttonReleased.notify(
	    mCB(this,uiDataPointSetCrossPlotter,itemsSelected) );

    setStretch( 2, 2 );
    setDragMode( uiGraphicsView::ScrollHandDrag );
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );
}


uiDataPointSetCrossPlotter::~uiDataPointSetCrossPlotter()
{
    delete &lsy1_;
    delete &lsy2_;
    if( selectedypts_ ) delete selectedypts_;
    if( selectedy2pts_ ) delete selectedy2pts_;
    deepErase( y1coords_ );
    deepErase( y2coords_ );
}

#define mHandleAxisAutoScale(axis) \
    axis.handleAutoScale( uidps_.getRunCalc( axis.colid_ ) ); \

void uiDataPointSetCrossPlotter::reDraw( CallBacker* )
{
    setDraw();
    drawContent();
}


void uiDataPointSetCrossPlotter::showY2( bool yn )
{
    y2ptitems_->setVisible( yn );
}


bool uiDataPointSetCrossPlotter::isY2Shown() const
{
     return y2_.axis_ && y2ptitems_ && y2ptitems_->isVisible();
}


void uiDataPointSetCrossPlotter::dataChanged()
{
    mHandleAxisAutoScale( x_ )
    mHandleAxisAutoScale( y_ )
    mHandleAxisAutoScale( y2_ )
    desiredxrg_ = x_.rg_;
    desiredyrg_ = y_.rg_;
    desiredy2rg_ = y2_.rg_;
    calcStats();
    setDraw();
    drawContent();
}


void uiDataPointSetCrossPlotter::removeSelections()
{
    for (  int idx=0; idx<yptitems_->getSize(); idx++ )
	yptitems_->getUiItem(idx)->setPenColor(
		axisHandler(1)->setup().style_.color_ );
    if ( y2ptitems_ && isY2Shown() )
    {
	for (  int idx=0; idx<y2ptitems_->getSize(); idx++ )
	    y2ptitems_->getUiItem(idx)->setPenColor(
		    axisHandler(2)->setup().style_.color_ );
    }
    selcoords_.erase();
    selrowcols_.erase();
    y1rowcols_.erase();
    y2rowcols_.erase();
    if ( odselectedpolygon_ )
    {	
	odselectedpolygon_->erase();
	if ( selectionpolygonitem_ )
	{
	    scene().removeItem( selectionpolygonitem_ );
	    selectionpolygonitem_ = 0;
	}
    }
    pointsSelected.trigger();
    reDrawNeeded.trigger();
}


void uiDataPointSetCrossPlotter::getSelStarPos( CallBacker* )
{
    mousepressed_ = true;
    selstartpos_ = getCursorPos();
    if ( !rectangleselection_ )
    {
	if ( !odselectedpolygon_ )
	    odselectedpolygon_ = new ODPolygon<int>();
	else
	    odselectedpolygon_->erase();
	odselectedpolygon_->add( selstartpos_ );
	if ( !selectionpolygonitem_ )
	{
	    selectionpolygonitem_ = new uiPolygonItem();
	    selectionpolygonitem_->setPenColor( Color(255,0,0) );
	    scene().addItem( selectionpolygonitem_ );
	}
	else
	    selectionpolygonitem_->setPolygon( *odselectedpolygon_ );
    }
}


void uiDataPointSetCrossPlotter::setSelectable( bool y1, bool y2 )
{
    isy1selectable_ = y1;
    isy2selectable_ = y2;
}


void uiDataPointSetCrossPlotter::drawPolygon( CallBacker* )
{
    if ( !selectable_ )
	return;
    if ( rectangleselection_ || !mousepressed_ )
	return;
    if ( odselectedpolygon_ )
	odselectedpolygon_->add( getCursorPos() );
    selectionpolygonitem_->setPolygon( *odselectedpolygon_ );
}


void uiDataPointSetCrossPlotter::itemsSelected( CallBacker* )
{
    mousepressed_ = false;
    if ( !selectable_ )
	return;
    if ( !rectangleselection_ && odselectedpolygon_->isSelfIntersecting() )
    {
	uiMSG().error( " Polygon drawn is not a valid one " );
	return;
    }
    uiPoint pt;
    uiRect selectedarea( rectangleselection_ ? selstartpos_ : pt,
	    		 rectangleselection_ ? getCursorPos() : pt );
    const uiAxisHandler* xah = x_.axis_;
    const uiAxisHandler* yah = y_.axis_;
    const uiAxisHandler* y2ah = y2_.axis_;
    uiRect yselectablerg( xah->getPix(desiredxrg_.start),
			  yah->getPix(desiredyrg_.stop),
			  xah->getPix(desiredxrg_.stop),
	    		  yah->getPix(desiredyrg_.start) );
    uiRect y2selectablerg;
    if ( y2ah )
	y2selectablerg = uiRect( xah->getPix(desiredxrg_.start),
			         y2ah->getPix(desiredy2rg_.stop),
			         xah->getPix(desiredxrg_.stop),
			         y2ah->getPix(desiredy2rg_.start) );
    else
	y2selectablerg = uiRect( 0, 0, width(), height() );
    if ( isy1selectable_ )
    {
	for ( int idx=0; idx<yptitems_->getSize(); idx++ )
	{
	    uiGraphicsItem* itm = yptitems_->getUiItem(idx);
	    const uiPoint* itempos = itm->getPos();
	    if ( (rectangleselection_ ? selectedarea.contains(*itempos) :
		    odselectedpolygon_->isInside(*itempos,true,0)) &&
		    y2selectablerg.contains(*itempos) &&
		    yselectablerg.contains(*itempos) )
	    {
		selcoords_ += y1coords_[idx];
		selrowcols_ += y1rowcols_[idx];
		itm->setPenColor( Color::DgbColor );
	    }
	}
    }
    if ( isy2selectable_ && y2ptitems_ && isY2Shown() )
    {
	for ( int idx=0; idx<y2ptitems_->getSize(); idx++ )
	{
	    uiGraphicsItem* itm = y2ptitems_->getUiItem(idx);
	    const uiPoint* itempos = itm->getPos();
	    if ( (rectangleselection_ ? selectedarea.contains(*itempos) :
		    odselectedpolygon_->isInside(*itempos,true,0)) &&
		    y2selectablerg.contains(*itempos) &&
		    yselectablerg.contains(*itempos) )
	    {
		selcoords_ += y2coords_[idx];
		selrowcols_ += y2rowcols_[idx];
		itm->setPenColor( Color(100,230,220) );
	    }
	}
    }
    else if ( !isy2selectable_ )
	y2rowcols_.erase();
    pointsSelected.trigger();
    reDrawNeeded.trigger();
}


static void updLS( const TypeSet<float>& inpxvals,
		   const TypeSet<float>& inpyvals,
		   const uiDataPointSetCrossPlotter::AxisData& axdx,
		   const uiDataPointSetCrossPlotter::AxisData& axdy,
		   LinStats2D& ls )
{
    const int inpsz = inpxvals.size();
    if ( inpsz < 2 ) { ls = LinStats2D(); return; }

    int firstxidx = 0, firstyidx = 0;
    if ( axdx.autoscalepars_.doautoscale_ && axdy.autoscalepars_.doautoscale_ )
    {
	firstxidx = (int)(axdx.autoscalepars_.clipratio_ * inpsz + .5);
	firstyidx = (int)(axdy.autoscalepars_.clipratio_ * inpsz + .5);
	if ( firstxidx < 1 && firstyidx < 1 )
	{
	    ls.use( inpxvals.arr(), inpyvals.arr(), inpsz );
	    return;
	}
    }

    Interval<float> xrg; assign( xrg, axdx.axis_->range() );
    if ( firstxidx > 0 )
    {
	TypeSet<float> sortvals( inpxvals );
	sortFor( sortvals.arr(), inpsz, firstxidx );
	xrg.start = sortvals[firstxidx]; xrg.stop = sortvals[inpsz-firstxidx-1];
    }
    Interval<float> yrg; assign( yrg, axdy.axis_->range() );
    if ( firstyidx > 0 )
    {
	TypeSet<float> sortvals( inpyvals );
	sortFor( sortvals.arr(), inpsz, firstyidx );
	yrg.start = sortvals[firstyidx]; yrg.stop = sortvals[inpsz-firstyidx-1];
    }

    TypeSet<float> xvals, yvals;
    for ( int idx=0; idx<inpxvals.size(); idx++ )
    {
	const float x = inpxvals[idx]; const float y = inpyvals[idx];
	if ( xrg.includes(x) && yrg.includes(y) )
	    { xvals += x; yvals += y; }
    }

    ls.use( xvals.arr(), yvals.arr(), xvals.size() );
}


void uiDataPointSetCrossPlotter::calcStats()
{
    if ( !x_.axis_ || (!y_.axis_ && !y2_.axis_) )
	return;

    const Interval<int> udfrg( 0, 1 );
    const Interval<int> xpixrg( x_.axis_->pixRange() ),
	  		ypixrg( y_.axis_ ? y_.axis_->pixRange() : udfrg ),
			y2pixrg( y2_.axis_ ? y2_.axis_->pixRange() : udfrg );
    TypeSet<float> xvals, yvals, x2vals, y2vals;

    for ( uiDataPointSet::DRowID rid=0; rid<dps_.size(); rid++ )
    {
	if ( dps_.isInactive(rid)
	  || (curgrp_ > 0 && dps_.group(rid) != curgrp_) )
	    continue;

	const float xval = uidps_.getVal( x_.colid_, rid, true );
	if ( mIsUdf(xval) ) continue;

	if ( y_.axis_ )
	{
	    const float yval = uidps_.getVal( y_.colid_, rid, true );
	    if ( !mIsUdf(yval) )
		{ xvals += xval; yvals += yval; }
	}
	if ( y2_.axis_ )
	{
	    const float yval = uidps_.getVal( y2_.colid_, rid, true );
	    if ( !mIsUdf(yval) )
		{ x2vals += xval; y2vals += yval; }
	}
    }

    updLS( xvals, yvals, x_, y_, lsy1_ );
    updLS( x2vals, y2vals, x_, y2_, lsy2_ );
}


bool uiDataPointSetCrossPlotter::selNearest( const MouseEvent& ev )
{
    const uiPoint pt( ev.pos() );
    selrow_ = getRow( y_, pt );
    selrowisy2_ = selrow_ < 0;
    if ( selrowisy2_ )
	selrow_ = getRow( y2_, pt );
    return selrow_ >= 0;
}


static const float cRelTol = 0.01; // 1% of axis size


int uiDataPointSetCrossPlotter::getRow(
	const uiDataPointSetCrossPlotter::AxisData& ad, uiPoint pt ) const
{
    if ( !ad.axis_ ) return -1;

    const float xpix = x_.axis_->getRelPos( x_.axis_->getVal(pt.x) );
    const float ypix = ad.axis_->getRelPos( ad.axis_->getVal(pt.y) );
    int row = -1; float mindistsq = 1e30;
    for ( uiDataPointSet::DRowID rid=0; rid<dps_.size(); rid++ )
    {
	if ( rid % eachrow_ || (curgrp_ > 0 && dps_.group(rid) != curgrp_) )
	    continue;

	const float xval = uidps_.getVal( x_.colid_, rid, true );
	const float yval = uidps_.getVal( ad.colid_, rid, true );
	if ( mIsUdf(xval) || mIsUdf(yval) ) continue;

	const float x = x_.axis_->getRelPos( xval );
	const float y = ad.axis_->getRelPos( yval );
	const float distsq = (x-xpix)*(x-xpix) + (y-ypix)*(y-ypix);
	if ( distsq < mindistsq )
	    { row = rid; mindistsq = distsq; }
    }

    return mindistsq < cRelTol*cRelTol ? row : -1;
}


void uiDataPointSetCrossPlotter::mouseClick( CallBacker* cb )
{
    if ( meh_.isHandled() ) return;

    const MouseEvent& ev = meh_.event();
    const bool isnorm = !ev.ctrlStatus() && !ev.shiftStatus() &&!ev.altStatus();

    if ( isnorm && selNearest(ev) )
    {
	selectionChanged.trigger();
	meh_.setHandled( true );
    }
}


void uiDataPointSetCrossPlotter::mouseRel( CallBacker* cb )
{
    if ( meh_.isHandled() ) return;

    const MouseEvent& ev = meh_.event();
    const bool isdel = ev.ctrlStatus() && !ev.shiftStatus() && !ev.altStatus();
    
    uiPoint scenepos( getScenePos(ev.x(),ev.y()) );
    MouseEvent sceneev( ev.buttonState(), scenepos.x, scenepos.y );

    if ( !setup_.noedit_ && isdel && selNearest(sceneev) )
    {
	removeRequest.trigger();
	meh_.setHandled( true );
	selrow_ = -1;
    }
}


void uiDataPointSetCrossPlotter::setCols( DataPointSet::ColID x,
		    DataPointSet::ColID y, DataPointSet::ColID y2 )
{
    if ( y < mincolid_ && y2 < mincolid_ )
	x = mincolid_ - 1;
    if ( x < mincolid_ )
	y = y2 = mincolid_ - 1;

    x_.setCol( x ); y_.setCol( y ); y2_.setCol( y2 );

    if ( y_.axis_ )
    {
	y_.axis_->setBegin( x_.axis_ );
	x_.axis_->setBegin( y_.axis_ );
    }
    else if ( x_.axis_ )
	x_.axis_->setBegin( 0 );

    if ( y2_.axis_ )
    {
	y2_.axis_->setBegin( x_.axis_ );
	x_.axis_->setEnd( y2_.axis_ );
    }
    else if ( x_.axis_ )
	x_.axis_->setEnd( 0 );

    mHandleAxisAutoScale(x_);
    mHandleAxisAutoScale(y_);
    mHandleAxisAutoScale(y2_);
    desiredxrg_ = x_.rg_;
    desiredyrg_ = y_.rg_;
    desiredy2rg_ = y2_.rg_;
    calcStats();
    setDraw();
    drawContent();
}


void uiDataPointSetCrossPlotter::initDraw()
{
    x_.newDevSize();
    y_.newDevSize();
    y2_.newDevSize();
}


void uiDataPointSetCrossPlotter::setDraw()
{
    if ( x_.axis_ )
	x_.axis_->setNewDevSize( width(), height() );
    if ( y_.axis_ )
	y_.axis_->setNewDevSize( height(), width() );
    if ( doy2_ && y2_.axis_ )
	y2_.axis_->setNewDevSize( height(), width() );
}


void uiDataPointSetCrossPlotter::mkNewFill()
{
    if ( !dobd_ ) return;
}


void uiDataPointSetCrossPlotter::drawContent( bool withaxis )
{
    if ( withaxis )
    {
	if ( x_.axis_ )
	    x_.axis_->plotAxis();
	if ( y_.axis_ )
	    y_.axis_->plotAxis();
	if ( doy2_ && y2_.axis_ )
	    y2_.axis_->plotAxis();

	if ( !x_.axis_ || !y_.axis_ )
	    return;
    }

    drawData( y_, false );
    if ( y2_.axis_ && doy2_ )
	drawData( y2_, true );
    else if ( y2ptitems_ )
	y2ptitems_->removeAll( true );
}


void uiDataPointSetCrossPlotter::drawData(
    const uiDataPointSetCrossPlotter::AxisData& yad, bool isy2 )
{
    selecteditems_->removeAll( true );
    y1coords_.erase();
    y2coords_.erase();
    isy2 ? y2rowcols_.erase() : y1rowcols_.erase();
    uiAxisHandler& xah = *x_.axis_;
    uiAxisHandler& yah = *yad.axis_;

    int estpts = dps_.size() / eachrow_;
    if ( curgrp_ > 0 ) estpts = cMaxPtsForMarkers;

    MarkerStyle2D mstyle( setup_.markerstyle_ );
    if ( estpts > cMaxPtsForMarkers )
	mstyle.type_ = MarkerStyle2D::None;
    mstyle.color_.setRgb( yah.setup().style_.color_.rgb() );

    const Interval<int> xpixrg( xah.pixRange() ), ypixrg( yah.pixRange() );

    int nrptsdisp = 0; Interval<int> usedxpixrg;
    if ( isy2 ? !y2ptitems_ : !yptitems_ )
    {
	if ( isy2 )
	{
	    y2ptitems_ = new uiGraphicsItemGroup();
	    scene().addItemGrp( y2ptitems_ );
	}
	else
	{
	    yptitems_ = new uiGraphicsItemGroup();
	    scene().addItemGrp( yptitems_ );
	}
    }

    uiGraphicsItemGroup* curitmgrp = isy2 ? y2ptitems_ : yptitems_ ;

    int itmidx = 0;
    for ( uiDataPointSet::DRowID rid=0; rid<dps_.size(); rid++ )
    {
	if ( dps_.isInactive(rid) || (curgrp_>0 && dps_.group(rid)!=curgrp_) )
	    continue;

	const float xval = uidps_.getVal( x_.colid_, rid, true );
	const float yval = uidps_.getVal( yad.colid_, rid, true );
	if ( mIsUdf(xval) || mIsUdf(yval) ) continue;

	const uiPoint pt( xah.getPix(xval), yah.getPix(yval) );
	
	if ( !xpixrg.includes(pt.x) || !ypixrg.includes(pt.y) )
	    continue;

	if ( isy2 )
	     y2rowcols_ += RowCol( rid, yad.colid_ );
	else
	     y1rowcols_ += RowCol( rid, yad.colid_ );

	if ( itmidx >= curitmgrp->getSize() )
	{
	    isy2 ? y2coords_ += new Coord3( dps_.coord(rid), dps_.z(rid) )
		 : y1coords_ += new Coord3( dps_.coord(rid), dps_.z(rid) );

	    uiGraphicsItem* itm = 0;
	    if ( mstyle.type_ == MarkerStyle2D::None )
		itm = new uiPointItem();
	    else
	    {
		mstyle.size_ = dps_.isSelected(rid) ? 4 : 2;
		itm = new uiMarkerItem( mstyle );
	    }

	    itm->setPenColor( yah.setup().style_.color_ );
	    curitmgrp->add( itm );
	}

	curitmgrp->getUiItem( itmidx )->setPos( pt.x, pt.y ); 
	if ( rid % eachrow_ )
	    curitmgrp->getUiItem(itmidx)->setVisible( false ); 
	else
	    curitmgrp->getUiItem(itmidx)->setVisible( isy2 ? isY2Shown() 
		    					   : true ); 

	nrptsdisp++;
	if ( nrptsdisp > 1 )
	    usedxpixrg.include( pt.x );
	else
	    usedxpixrg = Interval<int>( pt.x, pt.x );

	itmidx++;
    }

    if ( itmidx < curitmgrp->getSize() )
    {
	for ( int idx=itmidx; idx<curitmgrp->getSize(); idx++ )
	    curitmgrp->getUiItem(idx)->setVisible(false);
    }

    if ( nrptsdisp < 1 )
	return;

    if ( setup_.showcc_ )
    {
	float fr100 = (y_.axis_ == &yah ? lsy1_ : lsy2_).corrcoeff * 100;
	int r100 = mNINT(fr100);
	BufferString txt( "r=" );
	if ( r100 < 0 )
	    { txt += "-"; r100 = -r100; }
	if ( r100 == 0 )	txt += "0";
	else if ( r100 == 100 )	txt += "1";
	else
	{
	    txt += "0.";
	    txt += r100%10 ? r100 : r100 / 10;
	}
	yah.annotAtEnd( txt );
    }

    if ( setup_.showregrline_ )
	drawRegrLine( yah, usedxpixrg );
    else if ( regrlineitm_ )
	regrlineitm_->setLine( 0, 0, 0, 0 );
    drawY1UserDefLine( usedxpixrg, setup_.showy1userdefline_ );
    drawY2UserDefLine( usedxpixrg, setup_.showy2userdefline_ );
}


void uiDataPointSetCrossPlotter::drawY1UserDefLine( const Interval<int>& xpixrg,
						    bool draw )
{
    if ( !draw )
    {
	if ( y1userdeflineitm_ )
	{
	    scene().removeItem( y1userdeflineitm_ );
	    y1userdeflineitm_ = 0;
	}
	return;
    }

    const uiAxisHandler& xah = *x_.axis_;
    const uiAxisHandler& yah = *y_.axis_;
    Interval<float> xvalrg( xah.getVal(xpixrg.start), xah.getVal(xpixrg.stop) );
    if ( !y1userdeflineitm_ )
    {
	y1userdeflineitm_ = new uiLineItem();
	scene().addItem( y1userdeflineitm_ );
    }
    drawLine( *y1userdeflineitm_, userdefy1lp_, xah, yah, 0);//&xvalrg );
    y1userdeflineitm_->setPenStyle( y_.defaxsu_.style_ );
}


void uiDataPointSetCrossPlotter::drawY2UserDefLine( const Interval<int>& xpixrg,						    bool draw )
{
    if ( !draw )
    {
	if ( y2userdeflineitm_ )
	{
	    scene().removeItem( y2userdeflineitm_ );
	    y2userdeflineitm_ = 0;
	}
	return;
    }

    const uiAxisHandler& xah = *x_.axis_;
    if ( !y2_.axis_ )
	return;
    const uiAxisHandler& yah = *y2_.axis_;
    Interval<float> xvalrg( xah.getVal(xpixrg.start), xah.getVal(xpixrg.stop) );
    if ( !y2userdeflineitm_ )
    {
	y2userdeflineitm_ = new uiLineItem();
	scene().addItem( y2userdeflineitm_ );
    }
    drawLine( *y2userdeflineitm_, userdefy2lp_, xah, yah, 0 );//&xvalrg );
    y2userdeflineitm_->setPenStyle( y2_.defaxsu_.style_ );
}


void uiDataPointSetCrossPlotter::drawRegrLine( uiAxisHandler& yah,
					       const Interval<int>& xpixrg )
{
    const uiAxisHandler& xah = *x_.axis_;
    const LinStats2D& ls = y_.axis_ == &yah ? lsy1_ : lsy2_;
    const Interval<int> ypixrg( yah.pixRange() );
    Interval<float> xvalrg( xah.getVal(xpixrg.start), xah.getVal(xpixrg.stop) );
    Interval<float> yvalrg( yah.getVal(ypixrg.start), yah.getVal(ypixrg.stop) );
    if ( !regrlineitm_ )
    {
	regrlineitm_ = new uiLineItem();
	scene().addItem( regrlineitm_ );
    }
    drawLine( *regrlineitm_, ls.lp, xah, yah, &xvalrg );
}


uiDataPointSetCrossPlotWin::uiDataPointSetCrossPlotWin( uiDataPointSet& uidps )
    : uiMainWin(&uidps,BufferString(uidps.pointSet().name()," Cross-plot"),
	    			    2,false)
    , uidps_(uidps)
    , plotter_(*new uiDataPointSetCrossPlotter(this,uidps,defsetup_))
    , disptb_(*new uiToolBar(this,"Crossplot display toolbar"))
    , maniptb_(*new uiToolBar(this,"Crossplot manipulation toolbar"))
    , grpfld_(0)
    , showSelPts(this)
{
    windowClosed.notify( mCB(this,uiDataPointSetCrossPlotWin,closeNotif) );

    const int nrpts = uidps.pointSet().size();
    const int eachrow = 1 + nrpts / cMaxPtsForDisplay;
    uiGroup* grp = new uiGroup( &disptb_, "Each grp" );
    eachfld_ = new uiSpinBox( grp, 0, "Each" );
    eachfld_->setValue( eachrow );
    eachfld_->setInterval( 1, mUdf(int), 1 );
    eachfld_->valueChanged.notify(
	    		mCB(this,uiDataPointSetCrossPlotWin,eachChg) );
    new uiLabel( grp, "Plot each", eachfld_ );
    plotter_.eachrow_ = eachrow;
    
    selfld_ = new uiComboBox( grp, "Selection Option" );
    selfld_->addItem( "Select only Y1" );
    selfld_->addItem( "Select only Y2" );
    selfld_->addItem( "Select both" );
    selfld_->selectionChanged.notify( mCB(this,uiDataPointSetCrossPlotWin,
					  selOption) );
    selfld_->attach( rightOf, eachfld_ );
    selfld_->setSensitive( false );
    disptb_.addObject( grp->attachObj() );

    showy2tbid_ = disptb_.addButton( "showy2.png",
	    	  mCB(this,uiDataPointSetCrossPlotWin,showY2),
		  "Toggle show Y2", true );
    disptb_.turnOn( showy2tbid_, plotter_.doy2_ );
    setselecttbid_ = disptb_.addButton( "view.png",
	    	  mCB(this,uiDataPointSetCrossPlotWin,setSelectable),
		  "Set Selectable", true );
    disptb_.turnOn( setselecttbid_, true );


    showselptswsbid_ = disptb_.addButton( "picks.png",
	    	  mCB(this,uiDataPointSetCrossPlotWin,showPtsInWorkSpace),
		  "Show selected points in WorkSpace", false );
    disptb_.turnOn( setselecttbid_, true );

    selectionmodechangebutid_ = disptb_.addButton( "rectangleselect.png",
	   mCB(this,uiDataPointSetCrossPlotWin,setSelectionMode) ,
	   "Selection Mode" ); 
    disptb_.turnOn( selectionmodechangebutid_, plotter_.isRubberBandingOn() );

    removeselid_ = disptb_.addButton( " trashcan.png",
	    mCB(this,uiDataPointSetCrossPlotWin,removeSelections), 
	    " Remove all selections " );
    
    setselparameterid_ = disptb_.addButton( " settings.png",
	    mCB(this,uiDataPointSetCrossPlotWin,setSelectionDomain), 
	    " Selection Settings " );
/*
    maniptb_.addButton( "delsel.png",
			mCB(this,uiDataPointSetCrossPlotWin,delSel),
			"Delete selected", false );
*/
    maniptb_.addButton( "xplotprop.png",
			mCB(this,uiDataPointSetCrossPlotWin,editProps),
			"Properties", false );

    const int nrgrps = uidps_.groupNames().size();
    if ( nrgrps > 1 )
    {
	grpfld_ = new uiComboBox( grp, "Group selection" );
	BufferString txt( nrgrps == 2 ? "Both " : "All " );
	txt += uidps_.groupType(); txt += "s";
	grpfld_->addItem( txt );
	for ( int idx=0; idx<uidps_.groupNames().size(); idx++ )
	    grpfld_->addItem( uidps_.groupNames().get(idx) );
	grpfld_->attach( rightOf, selfld_ );
	grpfld_->setCurrentItem( 0 );
	grpfld_->selectionChanged.notify(
			    mCB(this,uiDataPointSetCrossPlotWin,grpChg) );
    }

    disptb_.setSensitive( selectionmodechangebutid_, false );
    disptb_.setSensitive( showselptswsbid_, false );
    plotter_.setPrefWidth( 600 );
    plotter_.setPrefHeight( 500 );
}


void uiDataPointSetCrossPlotWin::removeSelections( CallBacker* )
{
    plotter_.removeSelections();
}


void uiDataPointSetCrossPlotWin::closeNotif( CallBacker* )
{
    defsetup_ = plotter_.setup();
    plotter_.eachrow_ = mUdf(int); // Make sure eachChg knows we are closing
}


void uiDataPointSetCrossPlotWin::setSelectionMode( CallBacker* )
{
    plotter_.rectangleselection_ = !plotter_.rectangleselection_;
    disptb_.setPixmap( selectionmodechangebutid_,
	   	       plotter_.rectangleselection_ ? "rectangleselect.png"
			      			    : "polygonselect.png" );
    plotter_.setDragMode( plotter_.rectangleselection_ ?
	    			uiGraphicsView::RubberBandDrag :
	    			uiGraphicsView::ScrollHandDrag );
}


class uiSetSelDomainDlg : public uiDialog
{
public:

struct RangeInfo
{
    			RangeInfo( const char* lbl, const Interval<float>& rg )
			    : lbl_(lbl), rg_(rg)	{}
    BufferString	lbl_;
    Interval<float>	rg_;
};


uiSetSelDomainDlg( uiParent* p ,const RangeInfo& xinfo,const RangeInfo& yinfo,
		   const RangeInfo& y2info )
    : uiDialog( p, uiDialog::Setup("Refine Selection","Set selectable domain",
				    mNoHelpID) )
    , xrg_( xinfo.rg_ )
    , yrg_( yinfo.rg_ )
    , y2rg_( y2info.rg_ )
    , y2rgfld_( 0 )
{
    xrgfld_ = new uiGenInput( this, BufferString("X Range (",xinfo.lbl_,")"),
			      FloatInpIntervalSpec(xrg_) );
    xrgfld_->valuechanged.notify(mCB(this,uiSetSelDomainDlg,valueChanged));
    
    yrgfld_ = new uiGenInput( this, BufferString("Y Range (",yinfo.lbl_,")"),
	    		      FloatInpIntervalSpec(yrg_) );
    yrgfld_->attach( alignedBelow, xrgfld_ );
    yrgfld_->valuechanged.notify(mCB(this,uiSetSelDomainDlg,valueChanged));

    if ( !y2info.lbl_.isEmpty() )
    {
	y2rgfld_ = new uiGenInput( this, BufferString("Y2 Range (",y2info.lbl_,
		    		  ")"), FloatInpIntervalSpec(y2rg_) );
	y2rgfld_->attach( alignedBelow, yrgfld_ );
	y2rgfld_->valuechanged.notify(mCB(this,uiSetSelDomainDlg,valueChanged));
    }
}


void valueChanged( CallBacker* cb )
{
    mDynamicCastGet(uiGenInput*,fld,cb)
    if ( fld == xrgfld_ ) checkRange( fld, xrg_ );
    else if ( fld == yrgfld_ ) checkRange( fld, yrg_ );
    else if ( fld == y2rgfld_ ) checkRange( fld, y2rg_ );
}


void checkRange( uiGenInput* fld, const Interval<float>& rg )
{
    if ( fld->getFInterval().start < rg.start )
	fld->setValue( rg.start, 0 );
    if ( fld->getFInterval().stop > rg.stop )
	fld->setValue( rg.stop, 1 );
    if ( fld->getFInterval().start > fld->getFInterval().stop )
	fld->setValue( rg );
}

void getSelRanges( Interval<float>& xrg, Interval<float>& yrg,
		   Interval<float>& y2rg )
{
    xrg = xrgfld_->getFInterval();
    yrg = yrgfld_->getFInterval();
    y2rg = y2rgfld_->getFInterval();
}


bool acceptOK( CallBacker* )
{
    checkRange( xrgfld_, xrg_ );
    checkRange( yrgfld_, yrg_ );
    checkRange( y2rgfld_, y2rg_ );
    return true;
}

    Interval<float>	xrg_;
    Interval<float>	yrg_;
    Interval<float>	y2rg_;
    uiGenInput*		xrgfld_;
    uiGenInput*		yrgfld_;
    uiGenInput*		y2rgfld_;
};


void uiDataPointSetCrossPlotWin::setSelectionDomain( CallBacker* )
{
    uiSetSelDomainDlg::RangeInfo xrginfo( plotter_.x_.axis_->name(),
	    				  plotter_.x_.rg_ );
    uiSetSelDomainDlg::RangeInfo yrginfo( plotter_.y_.axis_->name(),
	    				  plotter_.y_.rg_ );
    uiSetSelDomainDlg::RangeInfo y2rginfo( plotter_.y2_.axis_ ?
	    				   plotter_.y2_.axis_->name() : 0,
					   plotter_.y2_.rg_ );
    uiSetSelDomainDlg dlg( this, xrginfo, yrginfo, y2rginfo );
    if ( dlg.go() )
	dlg.getSelRanges( plotter_.desiredxrg_, plotter_.desiredyrg_,
			  plotter_.desiredy2rg_ );
}


void uiDataPointSetCrossPlotWin::setSelectable( CallBacker* cb )
{
    plotter_.setDragMode( !disptb_.isOn(setselecttbid_ )
	   		  ? uiGraphicsView::RubberBandDrag
	   		  : uiGraphicsView::ScrollHandDrag );
    plotter_.setSceneSelectable( !disptb_.isOn(setselecttbid_) );
    selfld_->setSensitive( plotter_.isY2Shown() ? !disptb_.isOn(setselecttbid_)
	    					: false );
    disptb_.setSensitive( selectionmodechangebutid_,
	    		  !disptb_.isOn(setselecttbid_) );
    disptb_.setSensitive( showselptswsbid_,
	    		  !disptb_.isOn(setselecttbid_) );
    if ( plotter_.isRubberBandingOn() )
    plotter_.setDragMode( plotter_.rectangleselection_ ?
	    			uiGraphicsView::RubberBandDrag :
	    			uiGraphicsView::ScrollHandDrag );
}


void uiDataPointSetCrossPlotWin::showY2( CallBacker* )
{
    plotter_.showY2( disptb_.isOn(showy2tbid_) );
    setSelComboSensitive( disptb_.isOn(showy2tbid_) );
}


void uiDataPointSetCrossPlotWin::showPtsInWorkSpace( CallBacker* )
{
    showSelPts.trigger();
}


void uiDataPointSetCrossPlotWin::selOption( CallBacker* )
{
    const int selitem = selfld_->currentItem();
    if ( selitem == 0 )
	plotter_.setSelectable( true, false );
    else if ( selitem == 1 )
	plotter_.setSelectable( false, true );
    else
	plotter_.setSelectable( true, true );
}


void uiDataPointSetCrossPlotWin::eachChg( CallBacker* )
{
    if ( mIsUdf(plotter_.eachrow_) ) return; // window is closing

    int neweachrow = eachfld_->getValue();
    if ( neweachrow < 1 ) neweachrow = 1;
    if ( plotter_.eachrow_ == neweachrow )
	return;
    plotter_.eachrow_ = neweachrow;
    plotter_.drawContent( false );
}


void uiDataPointSetCrossPlotWin::grpChg( CallBacker* )
{
    if ( !grpfld_ ) return;
    plotter_.curgrp_ = grpfld_->currentItem();
    plotter_.dataChanged();
}


void uiDataPointSetCrossPlotWin::setSelComboSensitive( bool yn )
{
    selfld_->setCurrentItem( 0 );
    selfld_->setSensitive( yn );
}


void uiDataPointSetCrossPlotWin::delSel( CallBacker* )
{
}


void uiDataPointSetCrossPlotWin::editProps( CallBacker* )
{
    uiDataPointSetCrossPlotterPropDlg dlg( &plotter_ );
    dlg.go();
}
