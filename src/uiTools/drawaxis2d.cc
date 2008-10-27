/*+
________________________________________________________________________

 CopyRight:     ( C ) dGB Beheer B.V.
 Author:        Duntao Wei
 Date:          Mar. 2005
 RCS:           $Id: drawaxis2d.cc,v 1.13 2008-10-27 11:12:56 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "drawaxis2d.h"

#include "linear.h"
#include "draw.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uigraphicsitem.h"
#include "uigraphicsitemimpl.h"
#include "uiworld2ui.h"

#define mTickLen	5

#define mDefGrpItmInit \
      xaxlineitmgrp_( 0 ) \
    , yaxlineitmgrp_( 0 ) \
    , xaxgriditmgrp_( 0 ) \
    , yaxgriditmgrp_( 0 ) \
    , xaxtxtitmgrp_( 0 ) \
    , yaxtxtitmgrp_( 0 ) \
    , xaxrectitem_( 0 ) \
    , yaxlineitem_( 0 )

DrawAxis2D::DrawAxis2D( uiGraphicsView* drawview )
    : drawscene_( &drawview->scene() )
    , mDefGrpItmInit
    , drawview_(drawview)
    , inside_(false)
    , drawaxisline_(true)
    , xaxis_( 0, 1 )
    , yaxis_( 0, 1 )
    , xrg_( 0, 1 )
    , yrg_( 0, 1 )
    , useuirect_( false )
    , zValue_( 3 )
{ }


void DrawAxis2D::setDrawScene( uiGraphicsScene* da )
{
    drawscene_ = da; 
}


void DrawAxis2D::setDrawRectangle( const uiRect* ur )
{
    if ( !ur )
	useuirect_ = false;

    uirect_ = *ur;
    useuirect_ = true;
}


void DrawAxis2D::setup( const uiWorldRect& wr )
{
    xrg_.start = wr.left();
    xrg_.stop = wr.right();
    xaxis_ = AxisLayout( Interval<float>(wr.left(),wr.right()) ).sd;

    yrg_.start = wr.top();
    yrg_.stop = wr.bottom();
    yaxis_ = AxisLayout( Interval<float>(wr.top(),wr.bottom()) ).sd;
}


void DrawAxis2D::setup( const StepInterval<float>& xrg,
			const StepInterval<float>& yrg )
{
    xrg_.setFrom( xrg );
    yrg_.setFrom( yrg );
    xaxis_ = SamplingData<double>( xrg.start, xrg.step );
    yaxis_ = SamplingData<double>( yrg.start, yrg.step );
}


void DrawAxis2D::drawAxes( bool xdir, bool ydir,
			   bool topside, bool leftside )
{
    if ( xdir ) drawXAxis( topside );
    if ( ydir ) drawYAxis( leftside );
}

#define mLoopStart( dim ) \
    const int nrsteps = mNINT(dim##rg_.width(false)/dim##axis_.step)+1; \
    for ( int idx=0; idx<nrsteps; idx++ ) \
    { \
	const double dim##pos = dim##axis_.atIndex(idx); \
	if ( !dim##rg_.includes(dim##pos,true) ) \
	    continue;

#define mLoopEnd }


void DrawAxis2D::drawXAxis( bool topside )
{
    const uiRect drawarea = getDrawArea();
    const uiWorld2Ui transform( uiWorldRect(xrg_.start,yrg_.start,
					    xrg_.stop,yrg_.stop),
				drawarea.getPixelSize() );

    int baseline, bias;
    if ( topside )
    {
	baseline = drawarea.top();
	bias = inside_ ? mTickLen : -mTickLen;
    }
    else
    {
	baseline = drawarea.bottom();
	bias = inside_ ? -mTickLen : mTickLen;
    }

    if ( drawaxisline_ )
    {
	if ( !xaxrectitem_ )
	    xaxrectitem_ = drawscene_->addRect( drawarea.left(),
						drawarea.top(),
						drawarea.width(),
						drawarea.height() );
	else
	    xaxrectitem_->setRect( drawarea.left(), drawarea.top(),
		    		   drawarea.width(), drawarea.height() );
	
	xaxrectitem_->setZValue( 5 );
    }

    if ( !xaxlineitmgrp_ )
    {
	xaxlineitmgrp_ = new uiGraphicsItemGroup();
	drawscene_->addItemGrp( xaxlineitmgrp_ );
    }
    else
	xaxlineitmgrp_->removeAll( true );

    if ( !xaxtxtitmgrp_ )
    {
	xaxtxtitmgrp_ = new uiGraphicsItemGroup();
	drawscene_->addItemGrp( xaxtxtitmgrp_ );
    }
    else
	xaxtxtitmgrp_->removeAll( true );
    
    mLoopStart( x )
	BufferString text;
	const double displaypos = getAnnotTextAndPos(true,xpos,&text);
	const int wx = transform.toUiX( displaypos ) + drawarea.left();
	uiLineItem* lineitem = new uiLineItem();
	lineitem->setLine( wx, drawarea.top(), wx, drawarea.top()+bias );
	xaxlineitmgrp_->add( lineitem );

	Alignment al( OD::AlignHCenter, OD::AlignBottom );
	if ( bias<0 ) al.ver_ = OD::AlignBottom;
	uiTextItem* textitem = new uiTextItem();
	textitem->setText( text.buf() );
	textitem->setTextColor( Color::Black );
	textitem->setPos( wx, drawarea.top()+bias );
	textitem->setAlignment( al );
	xaxtxtitmgrp_->add( textitem );
    mLoopEnd
	
    xaxlineitmgrp_->setZValue( zValue_ );
    xaxtxtitmgrp_->setZValue( zValue_ );
}


void DrawAxis2D::drawYAxis( bool leftside )
{
    const uiRect drawarea = getDrawArea();
    const uiWorld2Ui transform(
	    uiWorldRect(xrg_.start,yrg_.start,
			xrg_.stop,yrg_.stop),
	    drawarea.getPixelSize() );

    int baseline, bias;
    if ( leftside )
    {
	baseline = drawarea.left();
	bias = inside_ ? mTickLen : -mTickLen;
    }
    else
    {
	baseline = drawarea.right();
	bias = inside_ ? -mTickLen : mTickLen;
    }

    if ( drawaxisline_ )
    {
	if ( !yaxlineitem_ )
	    yaxlineitem_ = drawscene_->addLine(	drawarea.left(), drawarea.top(),
		    				drawarea.left(), drawarea.bottom() );
	else 
	    yaxlineitem_->setLine( drawarea.left(), drawarea.top(),
		    		   drawarea.left(), drawarea.bottom() );
    }
    
    if ( !yaxlineitmgrp_ )
    {
	yaxlineitmgrp_ = new uiGraphicsItemGroup();
	drawscene_->addItemGrp( yaxlineitmgrp_ );
    }
    else
	yaxlineitmgrp_->removeAll( true );
    if ( !yaxtxtitmgrp_ )
    {
	yaxtxtitmgrp_ = new uiGraphicsItemGroup();
	drawscene_->addItemGrp( yaxtxtitmgrp_ );
    }
    else
	yaxtxtitmgrp_->removeAll( true );
    mLoopStart( y )
	BufferString text;
	const double displaypos =
	    getAnnotTextAndPos( false, ypos, &text );
	const int wy = transform.toUiY( displaypos ) + drawarea.top();
	uiLineItem* lineitem = new uiLineItem();
	lineitem->setLine( drawarea.left(), wy, drawarea.left() + bias, wy );
	yaxlineitmgrp_->add( lineitem ); 

	Alignment al( leftside ? OD::AlignRight : OD::AlignLeft,
		      OD::AlignVCenter );
	if ( bias < 0 ) al.hor_ = OD::AlignRight;
	uiTextItem* textitem = new uiTextItem();
	textitem->setText( text.buf() );
        textitem->setPos( drawarea.left() + bias, wy );
	textitem->setAlignment( al );
	yaxtxtitmgrp_->add( textitem );
    mLoopEnd
	
    yaxlineitmgrp_->setZValue( zValue_ );
    yaxtxtitmgrp_->setZValue( zValue_ );
    
}


void DrawAxis2D::drawGridLines( bool xdir, bool ydir )
{
    const uiRect drawarea = getDrawArea();
    const uiWorld2Ui transform( uiWorldRect(xrg_.start,yrg_.start,
					    xrg_.stop,yrg_.stop),
				drawarea.getPixelSize() );
    if ( xdir )
    {
	const int top = drawarea.top();
	const int bot = drawarea.bottom();// - drawarea.top();
	if ( !xaxgriditmgrp_ )
	{
	    xaxgriditmgrp_ = new uiGraphicsItemGroup();
	    drawscene_->addItemGrp( xaxgriditmgrp_ );
	}
	else
	    xaxgriditmgrp_->removeAll( true );
	mLoopStart( x )
	    const double displaypos = getAnnotTextAndPos( true, xpos );
	    const int wx = transform.toUiX( displaypos ) + drawarea.left();
	    uiLineItem* xgridline = new uiLineItem();
	    xgridline->setLine( wx, top, wx, bot );
	    xaxgriditmgrp_->add( xgridline );
	mLoopEnd

	xaxgriditmgrp_->setZValue( zValue_ );
    }

    if ( ydir )
    {
	const int left = drawarea.left();
	const int right = drawarea.right();// - drawarea.left();
	if ( !yaxgriditmgrp_ )
	{
	    yaxgriditmgrp_ = new uiGraphicsItemGroup();
	    drawscene_->addItemGrp( yaxgriditmgrp_ );
	}
	else
	    yaxgriditmgrp_->removeAll( true );
	mLoopStart( y )
	    const double displaypos = getAnnotTextAndPos(false, ypos);
	    const int wy = transform.toUiY( displaypos ) + drawarea.top();
	    uiLineItem* ygridline = new uiLineItem();
	    ygridline->setLine( left, wy, right, wy );
	    yaxgriditmgrp_->add( ygridline );
	mLoopEnd
	
	yaxgriditmgrp_->setZValue( zValue_ );
    }
}


uiRect DrawAxis2D::getDrawArea() const
{
    if ( useuirect_ )
	return uirect_;

    return uiRect( 0, 0, drawview_->width(), drawview_->height() );
}


double DrawAxis2D::getAnnotTextAndPos( bool isx, double proposedpos,
				     BufferString* text ) const
{
    if ( text ) (*text) = proposedpos;
    return proposedpos;
}
