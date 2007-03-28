/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uirgbarraycanvas.cc,v 1.5 2007-03-28 12:20:46 cvsbert Exp $
 ________________________________________________________________________

-*/

#include "uirgbarraycanvas.h"
#include "uirgbarray.h"
#include "iodrawtool.h"
#include "pixmap.h"

uiRGBArrayCanvas::uiRGBArrayCanvas( uiParent* p, uiRGBArray& a )
    	: uiCanvas(p)
	, rgbarr_(a)
	, newFillNeeded(this)
	, rubberBandUsed(this)
	, border_(0,0,0,0)
	, bgcolor_(Color::NoColor)
	, dodraw_(true)
	, pixmap_(0)
{
    setBGColor( Color::White );
    preDraw.notify( mCB(this,uiRGBArrayCanvas,beforeDraw) );
}


void uiRGBArrayCanvas::setBorders( const uiSize& tl, const uiSize& br )
{
    uiRect newborder;
    newborder.setLeft( tl.width() );
    newborder.setTop( tl.height() );
    newborder.setRight( br.width() );
    newborder.setBottom( br.height() );
    if ( border_ != newborder )
    {
	border_ = newborder;
	setupChg();
    }
}


void uiRGBArrayCanvas::setBGColor( const Color& c )
{
    if ( bgcolor_ != c )
    {
	bgcolor_ = c;
	setBackgroundColor( bgcolor_ );
	setupChg();
    }
}


void uiRGBArrayCanvas::setDrawArr( bool yn )
{
    if ( dodraw_ != yn )
    {
	dodraw_ = yn;
	setupChg();
    }
}


void uiRGBArrayCanvas::forceNewFill()
{
    delete pixmap_; pixmap_ = 0;
    update(); // Force redraw
}


void uiRGBArrayCanvas::setupChg()
{
    update(); // Force redraw
}


void uiRGBArrayCanvas::beforeDraw( CallBacker* )
{
    drawTool().setBackgroundColor( bgcolor_ );
    drawTool().clear();

    const uiSize totsz( width(), height() );
    const int unusedpix = 1; // There seems to be an undrawable border
    arrarea_.setLeft( border_.left() + unusedpix );
    arrarea_.setTop( border_.top() + unusedpix );
    arrarea_.setRight( totsz.width() - border_.right() - 2 * unusedpix );
    arrarea_.setBottom( totsz.height() - border_.bottom() - 2 * unusedpix );

    const int xsz = arrarea_.width() + 1;
    const int ysz = arrarea_.height() + 1;
    if ( xsz < 1 || ysz < 1 )
    {
	rgbarr_.setSize( 1, 1 );
	pixmap_ = new ioPixmap( 1, 1 );
	return;
    }

    if ( rgbarr_.getSize(true) != xsz || rgbarr_.getSize(false) != ysz )
    {
	rgbarr_.setSize( xsz, ysz );
	delete pixmap_; pixmap_ = 0;
    }
}


void uiRGBArrayCanvas::reDrawHandler( uiRect updarea )
{
    updarea_ = updarea;
    if ( !dodraw_ )
	return;

    if ( !pixmap_ )
    {
	rgbarr_.clear( bgcolor_ );
	newFillNeeded.trigger();
	mkNewFill();
	if ( !createPixmap() )
	    return;
    }

    const uiRect pixrect( 0, 0, pixmap_->width()-1, pixmap_->height()-1 );
    if ( pixrect.right() < 1 && pixrect.bottom() < 1 )
	return;

    drawTool().drawPixmap( arrarea_.topLeft(), pixmap_, pixrect );
}


void uiRGBArrayCanvas::rubberBandHandler( uiRect r )
{
    CBCapsule<uiRect> caps( r, this );
    rubberBandUsed.trigger( &caps );
}


bool uiRGBArrayCanvas::createPixmap()
{
    const int xsz = rgbarr_.getSize( true );
    const int ysz = rgbarr_.getSize( false );
    delete pixmap_; pixmap_ = 0;
    if ( xsz < 1 || ysz < 1 )
	return false;

    pixmap_ = new ioPixmap( rgbarr_ );
    return true;
}
