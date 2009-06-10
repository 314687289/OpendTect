/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uifunctiondisplay.cc,v 1.40 2009-06-10 10:15:48 cvssatyaki Exp $";

#include "uifunctiondisplay.h"
#include "uiaxishandler.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicssaveimagedlg.h"
#include "mouseevent.h"
#include "linear.h"
#include <iostream>

static const int cBoundarySz = 10;

uiFunctionDisplay::uiFunctionDisplay( uiParent* p,
				      const uiFunctionDisplay::Setup& su )
    : uiGraphicsView(p,"Function display viewer")
    , setup_(su)
    , xax_(0)
    , yax_(0)
    , xmarkval_(mUdf(float))
    , ymarkval_(mUdf(float))
    , selpt_(0)
    , ypolyitem_(0)
    , y2polyitem_(0)
    , ypolygonitem_(0)
    , y2polygonitem_(0)
    , ypolylineitem_(0)
    , y2polylineitem_(0)
    , ymarkeritems_(0)
    , y2markeritems_(0)
    , borderrectitem_(0)
    , pointSelected(this)
    , pointChanged(this)
    , mousedown_(false)
{
    disableScrollZoom();
    setPrefWidth( setup_.canvaswidth_ );
    setPrefHeight( setup_.canvasheight_ );
    setStretch( 2, 2 );
    gatherInfo();
    uiAxisHandler::Setup asu( uiRect::Bottom, setup_.canvaswidth_,
	    		      setup_.canvasheight_ );
    asu.noaxisline( setup_.noxaxis_ );
    asu.noaxisannot( asu.noaxisline_ ? true : !setup_.annotx_ );
    asu.nogridline( asu.noaxisline_ ? true : setup_.noxgridline_ );
    asu.border_ = setup_.border_;
    xax_ = new uiAxisHandler( &scene(), asu );
    asu.noaxisline( setup_.noyaxis_ );
    asu.noaxisannot( asu.noaxisline_ ? true : !setup_.annoty_ );
    asu.nogridline( asu.noaxisline_ ? true : setup_.noygridline_ );
    asu.side( uiRect::Left );
    yax_ = new uiAxisHandler( &scene(), asu );
    asu.noaxisline( setup_.noy2axis_ );
    asu.noaxisannot( asu.noaxisline_ ? true : !setup_.annoty2_ );
    asu.nogridline( setup_.noy2gridline_ );
    xax_->setBegin( yax_ ); yax_->setBegin( xax_ );
    asu.side( uiRect::Right );
    y2ax_ = new uiAxisHandler( &scene(), asu );

    if ( setup_.editable_ )
    {
	getMouseEventHandler().buttonPressed.notify(
				mCB(this,uiFunctionDisplay,mousePress) );
	getMouseEventHandler().buttonReleased.notify(
				mCB(this,uiFunctionDisplay,mouseRelease) );
	getMouseEventHandler().movement.notify(
				mCB(this,uiFunctionDisplay,mouseMove) );
	getMouseEventHandler().doubleClick.notify(
				mCB(this,uiFunctionDisplay,mouseDClick) );
    }

    setToolTip( "Press Ctrl-P to save as image" );
    reSize.notify( mCB(this,uiFunctionDisplay,reSized) );
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );
    draw();
}


uiFunctionDisplay::~uiFunctionDisplay()
{
    delete xax_;
    delete yax_;
    delete y2ax_;
    if ( ypolyitem_ ) scene().removeItem( ypolyitem_ );
    if ( y2polyitem_ ) scene().removeItem( y2polyitem_ );
    if ( ymarkeritems_ ) scene().removeItem( ymarkeritems_ );
    if ( y2markeritems_ ) scene().removeItem( y2markeritems_ );
}


void uiFunctionDisplay::reSized( CallBacker* )
{
    draw();
}

void uiFunctionDisplay::saveImageAs( CallBacker* )
{
}


void uiFunctionDisplay::setVals( const float* xvals, const float* yvals,
				 int sz )
{
    xvals_.erase(); yvals_.erase();
    if ( sz < 2 ) return;

    for ( int idx=0; idx<sz; idx++ )
	{ xvals_ += xvals[idx]; yvals_ += yvals[idx]; }

    gatherInfo(); draw();
}


void uiFunctionDisplay::setVals( const Interval<float>& xrg, const float* yvals,
				 int sz )
{
    xvals_.erase(); yvals_.erase();
    if ( sz < 2 ) return;

    const float dx = (xrg.stop-xrg.start) / (sz-1);
    for ( int idx=0; idx<sz; idx++ )
	{ xvals_ += xrg.start + idx * dx; yvals_ += yvals[idx]; }

    gatherInfo(); draw();
}


void uiFunctionDisplay::setY2Vals( const float* xvals, const float* yvals,
				   int sz )
{
    y2xvals_.erase(); y2yvals_.erase();
    if ( sz < 2 ) return;

    for ( int idx=0; idx<sz; idx++ )
    {
	y2xvals_ += xvals[idx];
	y2yvals_ += yvals[idx];
    }

    gatherInfo(); draw();
}


void uiFunctionDisplay::setMarkValue( float val, bool is_x )
{
    (is_x ? xmarkval_ : ymarkval_) = val;
}


#define mSetRange( axis, rg ) \
    axlyo.setDataRange( rg ); rg.step = axlyo.sd.step; \
    if ( !mIsEqual(rg.start,axlyo.sd.start,axlyo.sd.step*1e-6) ) \
	axlyo.sd.start += axlyo.sd.step; \
    axis->setRange( rg, &axlyo.sd.start );


void uiFunctionDisplay::gatherInfo()
{
    if ( yvals_.isEmpty() ) return;
    const bool havey2 = !y2xvals_.isEmpty();
    if ( havey2 )
	{ xax_->setEnd( y2ax_ ); y2ax_->setBegin( xax_ ); }

    StepInterval<float> xrg, yrg;
    getRanges( xvals_, yvals_, setup_.xrg_, setup_.yrg_, xrg, yrg );

    AxisLayout axlyo;
    mSetRange( xax_, xrg );
    mSetRange( yax_, yrg );

    if ( havey2 )
    {
	getRanges( y2xvals_, y2yvals_, setup_.xrg_, setup_.y2rg_, xrg, yrg );
	mSetRange( y2ax_, yrg );
    }
}


void uiFunctionDisplay::getRanges(
	const TypeSet<float>& xvals, const TypeSet<float>& yvals,
	const Interval<float>& setupxrg, const Interval<float>& setupyrg,
	StepInterval<float>& xrg, StepInterval<float>& yrg ) const
{
    for ( int idx=0; idx<xvals.size(); idx++ )
    {
	if ( idx == 0 )
	{
	    xrg.start = xrg.stop = xvals[0];
	    yrg.start = yrg.stop = yvals[0];
	}
	else
	{
	    if ( xvals[idx] < xrg.start ) xrg.start = xvals[idx];
	    if ( yvals[idx] < yrg.start ) yrg.start = yvals[idx];
	    if ( xvals[idx] > xrg.stop ) xrg.stop = xvals[idx];
	    if ( yvals[idx] > yrg.stop ) yrg.stop = yvals[idx];
	}
    }

    if ( !mIsUdf(setupxrg.start) ) xrg.start = setupxrg.start;
    if ( !mIsUdf(setupyrg.start) ) yrg.start = setupyrg.start;
    if ( !mIsUdf(setupxrg.stop) ) xrg.stop = setupxrg.stop;
    if ( !mIsUdf(setupyrg.stop) ) yrg.stop = setupyrg.stop;
}


void uiFunctionDisplay::setUpAxis( bool havey2 )
{
    xax_->setNewDevSize( width(), height() );
    yax_->setNewDevSize( height(), width() );
    if ( havey2 ) y2ax_->setNewDevSize( height(), width() );

    xax_->plotAxis();
    yax_->plotAxis();
    if ( havey2 )
	y2ax_->plotAxis();
}


void uiFunctionDisplay::getPointSet( TypeSet<uiPoint>& ptlist, bool y2 )
{
    const int nrpts = size();
    const uiPoint closept( xax_->getPix(xax_->range().start),
	    		   y2 ? y2ax_->getPix(yax_->range().start)
	   		      : yax_->getPix(yax_->range().start) );
    if ( y2 ? setup_.fillbelowy2_ : setup_.fillbelow_ )
	ptlist += closept;

    for ( int idx=0; idx<nrpts; idx++ )
    {
	const int xpix = xax_->getPix( y2 ? y2xvals_[idx]
					  : xvals_[idx] );
	const int ypix = y2 ? ( y2ax_->getPix(y2yvals_[idx]) )
	    		    : ( yax_->getPix(yvals_[idx]) );
	const uiPoint pt( xpix, y2 ? y2ax_->getPix(y2yvals_[idx])
	       			   : yax_->getPix(yvals_[idx]) );
	Interval<int> xpixintv( xax_->getPix(xax_->range().start),
				xax_->getPix(xax_->range().stop) );
	Interval<int> ypixintv( y2 ? y2ax_->getPix(y2ax_->range().start)
				   : yax_->getPix(yax_->range().start),
				y2 ? y2ax_->getPix(y2ax_->range().stop)
				   : yax_->getPix(yax_->range().stop) );
	if ( xpixintv.includes(xpix) && ypixintv.includes(ypix) )
	    ptlist += pt;
	
	if ( idx == nrpts-1 && setup_.closepolygon_)
	    ptlist += uiPoint( pt.x, closept.y );
    }
    
}


void uiFunctionDisplay::drawYCurve( const TypeSet<uiPoint>& ptlist )
{
    bool polydrawn = false;
    if ( setup_.fillbelow_ )
    {
	if ( !ypolygonitem_ )
	    ypolygonitem_ = scene().addPolygon( ptlist, setup_.fillbelow_ );
	else
	    ypolygonitem_->setPolygon( ptlist );
	ypolygonitem_->setFillColor( setup_.ycol_ );
	ypolyitem_ = ypolygonitem_;
	polydrawn = true;
    }
    else if ( setup_.drawliney_ )
    {
	if ( !ypolylineitem_ )
	    ypolylineitem_ = scene().addPolyLine( ptlist );
	else
	    ypolylineitem_->setPolyLine( ptlist );
	ypolyitem_ = ypolylineitem_;
	polydrawn = true;
    }

    if ( polydrawn )
    {
	ypolyitem_->setPenColor( setup_.ycol_ );
	ypolyitem_->setZValue( setup_.curvzval_ );
	ypolyitem_->setVisible( true );
    }
    else if ( ypolyitem_ )
	ypolyitem_->setVisible( false );
}


void uiFunctionDisplay::drawY2Curve( const TypeSet<uiPoint>& ptlist,
				     bool havey2 )
{
    bool polydrawn = false;
    if ( setup_.fillbelowy2_ )
    {
	if ( !y2polygonitem_ )
	    y2polygonitem_ = scene().addPolygon( ptlist, setup_.fillbelowy2_ );
	else
	    y2polygonitem_->setPolygon( ptlist );
	y2polygonitem_->setFillColor(
		setup_.fillbelowy2_ ? setup_.y2col_ : Color::NoColor());
	y2polyitem_ = y2polygonitem_;
	polydrawn = true;
    }
    else if ( setup_.drawliney2_ )
    {
	if ( !y2polylineitem_ )
	    y2polylineitem_ = scene().addPolyLine( ptlist );
	else
	    y2polylineitem_->setPolyLine( ptlist );
	y2polyitem_ = y2polylineitem_;
	polydrawn = true;
    }

    if ( polydrawn )
    {
	y2polyitem_->setPenColor( setup_.y2col_ );
	y2polyitem_->setZValue( setup_.curvzval_ );
	y2polyitem_->setVisible( true );
    }
    else if ( y2polyitem_ && !havey2)
	y2polyitem_->setVisible( false );
}

void uiFunctionDisplay::drawMarker( const TypeSet<uiPoint>& ptlist, bool isy2 )
{
    //TODO removing of all items not perfirmance effective,
        // some other method should be applied
    if ( isy2 ? !y2markeritems_ : !ymarkeritems_ )
    {
	if ( isy2 )
	{
	    if ( !setup_.drawscattery2_ )
	    {
		if ( y2markeritems_ )
		    y2markeritems_->setVisible( false );
		return;
	    }
	    
	    y2markeritems_ = new uiGraphicsItemGroup();
	    scene().addItemGrp( y2markeritems_ );
	}
	else
	{
	    if ( !setup_.drawscattery1_ )
	    {
		if ( ymarkeritems_ )
		    ymarkeritems_->setVisible( false );
		return;
	    }
	    
	    ymarkeritems_ = new uiGraphicsItemGroup();
	    scene().addItemGrp( ymarkeritems_ );
	}
    }

    uiGraphicsItemGroup* curitmgrp = isy2 ? y2markeritems_ : ymarkeritems_;
    curitmgrp->setVisible( true );
    const MarkerStyle2D mst( MarkerStyle2D::Square, 3,
			     isy2 ? setup_.y2col_ : setup_.ycol_ );
    for ( int idx=0; idx<ptlist.size(); idx++ )
    {
	if ( idx < curitmgrp->getSize() )
	{
	    uiGraphicsItem* itm = curitmgrp->getUiItem(idx);
	    itm->setPos( ptlist[idx].x, ptlist[idx].y );
	    itm->setPenColor( isy2 ? setup_.y2col_ : setup_.ycol_ );
	}
	else
	{
	    uiMarkerItem* markeritem = new uiMarkerItem( mst, false );
	    markeritem->setPos( ptlist[idx].x, ptlist[idx].y );
	    markeritem->setPenColor( isy2 ? setup_.y2col_ : setup_.ycol_ );
	    curitmgrp->add( markeritem );
	}
    }
    if ( ptlist.size() < curitmgrp->getSize() )
    {
	for ( int idx=ptlist.size(); idx<curitmgrp->getSize(); idx++ )
	    curitmgrp->getUiItem(idx)->setVisible( false );
    }
    curitmgrp->setZValue( setup_.curvzval_ );
}


void uiFunctionDisplay::drawBorder()
{
    if ( setup_.drawborder_ )
    {
	if ( !borderrectitem_ )
	{
	    borderrectitem_ = scene().addRect( xAxis()->pixBefore(),
		    yAxis(false)->pixAfter(),
		    width()-yAxis(false)->pixAfter()-yAxis(false)->pixBefore(),
		    height()-xAxis()->pixAfter()-xAxis()->pixBefore() );
	}
	else
	    borderrectitem_->setRect( xAxis()->pixBefore(),
		    		      yAxis(false)->pixAfter(),
		    		      width()-xAxis()->pixAfter() -
				      xAxis()->pixBefore() ,
		    		      height()-yAxis(false)->pixAfter() -
				      yAxis(false)->pixBefore() );
	borderrectitem_->setPos(xAxis()->pixBefore(),yAxis(false)->pixAfter());
	borderrectitem_->setPenStyle( setup_.borderstyle_ );
    }
    if ( borderrectitem_ )
	borderrectitem_->setVisible( setup_.drawborder_ );
}


void uiFunctionDisplay::draw()
{
    if ( yvals_.isEmpty() ) return;
    const bool havey2 = !y2xvals_.isEmpty();

    setUpAxis( havey2 );

    TypeSet<uiPoint> yptlist, y2ptlist;
    getPointSet( yptlist, false );
    if ( havey2 )
	getPointSet( y2ptlist, true );

    drawYCurve( yptlist );
    drawY2Curve( y2ptlist, havey2 );
    drawMarker(yptlist,false);
    if ( havey2 )
	drawMarker(y2ptlist,true);
    else if ( y2markeritems_ )
	y2markeritems_->setVisible( false );
    drawBorder();

    LineStyle ls;
    if ( !mIsUdf(xmarkval_) )
    {
	ls.color_ = setup_.xmarkcol_;
	xax_->setup().style_ = ls;
	xax_->drawGridLine( xax_->getPix(xmarkval_) );
    }
    if ( !mIsUdf(ymarkval_) )
    {
	ls.color_ = setup_.ymarkcol_;
	yax_->setup().style_ = ls;
	yax_->drawGridLine( yax_->getPix(ymarkval_) );
    }
}


#define mGetMousePos()  \
    if ( getMouseEventHandler().isHandled() ) \
	return; \
    const MouseEvent& ev = getMouseEventHandler().event(); \
    if ( !(ev.buttonState() & OD::LeftButton ) || \
	  (ev.buttonState() & OD::MidButton ) || \
	  (ev.buttonState() & OD::RightButton ) ) \
        return; \
    const bool isctrl = isCtrlPressed(); \
    const bool isoth = ev.shiftStatus() || ev.altStatus(); \
    const bool isnorm = !isctrl && !isoth


bool uiFunctionDisplay::setSelPt()
{
    const MouseEvent& ev = getMouseEventHandler().event();

    int newsel = -1; float mindistsq = 1e30;
    const float xpix = xax_->getRelPos( xax_->getVal(ev.pos().x) );
    const float ypix = yax_->getRelPos( yax_->getVal(ev.pos().y) );
    for ( int idx=0; idx<xvals_.size(); idx++ )
    {
	const float x = xax_->getRelPos( xvals_[idx] );
	const float y = yax_->getRelPos( yvals_[idx] );
	const float distsq = (x-xpix)*(x-xpix) + (y-ypix)*(y-ypix);
	if ( distsq < mindistsq )
	    { newsel = idx; mindistsq = distsq; }
    }
    selpt_ = -1;
    if ( mindistsq > setup_.ptsnaptol_*setup_.ptsnaptol_ ) return false;
    if ( newsel < 0 || newsel > xvals_.size() - 1 )
    {
	selpt_ = -1;
	return false;
    }

    selpt_ = newsel;
    return true;
}


void uiFunctionDisplay::mousePress( CallBacker* )
{
    if ( mousedown_ ) return; mousedown_ = true;
    mGetMousePos();
    if ( isoth || !setSelPt() ) return;

    if ( isnorm )
	pointSelected.trigger();
}


void uiFunctionDisplay::mouseRelease( CallBacker* )
{
    if ( !mousedown_ ) return; mousedown_ = false;
    mGetMousePos();
    if ( !isctrl || selpt_ <= 0 || selpt_ >= xvals_.size()-1
	 || xvals_.size() < 3 ) return;

    xvals_.remove( selpt_ );
    yvals_.remove( selpt_ );

    selpt_ = -1;
    pointChanged.trigger();
    draw();
}


void uiFunctionDisplay::mouseMove( CallBacker* )
{
    if ( !mousedown_ ) return;
    mGetMousePos();
    if ( !isnorm || selpt_ < 0 ) return;

    float xval = xax_->getVal( ev.pos().x );
    float yval = yax_->getVal( ev.pos().y );

    if ( selpt_ > 0 && xvals_[selpt_-1] >= xval )
	return;
    if ( selpt_ < xvals_.size() - 1 && xvals_[selpt_+1] <= xval )
	return;

    if ( xval > xax_->range().stop )
	xval = xax_->range().stop;
    else if ( xval < xax_->range().start )
	xval = xax_->range().start;

    if ( yval > yax_->range().stop )
	yval = yax_->range().stop;
    else if ( yval < yax_->range().start )
	yval = yax_->range().start;

    xvals_[selpt_] = xval; yvals_[selpt_] = yval;

    pointChanged.trigger();
    draw();
}


void uiFunctionDisplay::mouseDClick( CallBacker* )
{
    mousedown_ = false;
    mGetMousePos();
    if ( !isnorm ) return;

    float xval = xax_->getVal(ev.pos().x);
    float yval = yax_->getVal(ev.pos().y);
    
    if ( xval > xax_->range().stop )
	xval = xax_->range().stop;
    else if ( xval < xax_->range().start )
	xval = xax_->range().start;

    if ( yval > yax_->range().stop )
	yval = yax_->range().stop;
    else if ( yval < yax_->range().start )
	yval = yax_->range().start;

    if ( !xvals_.isEmpty() && xval > xvals_.last() )
    {
	xvals_ += xval; yvals_ += yval;
	selpt_ = xvals_.size()-1;
    }
    else
    {
	for ( int idx=0; idx<xvals_.size(); idx++ )
	{
	    if ( xval > xvals_[idx] )
		continue;

	    if ( xval == xvals_[idx] )
		yvals_[idx] = yval;
	    else
	    {
		xvals_.insert( idx, xval );
		yvals_.insert( idx, yval );
	    }

	    selpt_ = idx;
	    break;
	}
    }

    pointSelected.trigger();
    draw();
}
