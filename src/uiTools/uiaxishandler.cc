/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiaxishandler.cc,v 1.28 2009-04-15 12:13:41 cvssatyaki Exp $";

#include "uiaxishandler.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uifont.h"
#include "linear.h"
#include "draw.h"

#include <math.h>

static const float logof2 = logf(2);

#define mDefInitList \
      gridlineitmgrp_(0) \
    , axislineitm_(0) \
    , endannottextitm_(0) \
    , annottxtitmgrp_(0) \
    , nameitm_(0) \
    , annotlineitmgrp_(0)

uiAxisHandler::uiAxisHandler( uiGraphicsScene* scene,
			      const uiAxisHandler::Setup& su )
    : NamedObject(su.name_)
    , mDefInitList  
    , scene_(scene)
    , setup_(su)
    , height_(su.height_)
    , width_(su.width_)
    , ticsz_(2)
    , beghndlr_(0)
    , endhndlr_(0)
    , ynmtxtvertical_(false)
{
    setRange( StepInterval<float>(0,1,1) );
}


uiAxisHandler::~uiAxisHandler()
{
    if ( axislineitm_ ) scene_->removeItem( axislineitm_ );
    if ( gridlineitmgrp_ ) scene_->removeItem( gridlineitmgrp_ );
    if ( annottxtitmgrp_ ) scene_->removeItem( annottxtitmgrp_ );
    if ( annotlineitmgrp_ ) scene_->removeItem( annotlineitmgrp_ );
    if ( endannottextitm_ ) scene_->removeItem( endannottextitm_ );
    if ( nameitm_ ) scene_->removeItem( nameitm_ );
}


void uiAxisHandler::setRange( const StepInterval<float>& rg, float* astart )
{
    rg_ = rg;
    annotstart_ = astart ? *astart : rg_.start;

    float fsteps = (rg_.stop - rg_.start) / rg_.step;
    if ( fsteps < 0 )
	rg_.step = -rg_.step;
    if ( mIsZero(fsteps,1e-6) )
	{ rg_.start -= rg_.step * 1.5; rg_.stop += rg_.step * 1.5; }
    fsteps = (rg_.stop - rg_.start) / rg_.step;
    if ( fsteps > 50 )
    	rg_.step /= (fsteps / 50);

    rgisrev_ = rg_.start > rg_.stop;
    rgwidth_ = rg_.width();

    reCalc();
}


void uiAxisHandler::reCalc()
{
    pos_.erase(); strs_.erase();

    StepInterval<float> annotrg( rg_ );
    annotrg.start = annotstart_;

    const uiFont& font = FontList().get();
    wdthy_ = 2*font.height();
    BufferString str;

    const int nrsteps = annotrg.nrSteps();
    for ( int idx=0; idx<=nrsteps; idx++ )
    {
	float pos = annotrg.start + idx * rg_.step;
	str = pos; strs_.add( str );
	float relpos = pos - rg_.start;
	if ( rgisrev_ ) relpos = -relpos;
	relpos /= rgwidth_;
	if ( setup_.islog_ )
	    relpos = log( 1 + relpos );
	pos_ += relpos;
	const int wdth = font.width( str );
	if ( idx == 0 )			wdthx_ = font.width( str )+2;
	else if ( wdthx_ < wdth )	wdthx_ = wdth;
    }
    endpos_ = setup_.islog_ ? logof2 : 1;
    newDevSize();
}


void uiAxisHandler::newDevSize()
{
    devsz_ = isHor() ? width_ : height_;
    axsz_ = devsz_ - pixBefore() - pixAfter();
}


void uiAxisHandler::setNewDevSize( int devsz, int anotherdim )
{
    devsz_ = devsz;
    axsz_ = devsz_ - pixBefore() - pixAfter();
    isHor() ? width_ = devsz_ : height_ = devsz_;
    isHor() ? height_ = anotherdim : width_ = anotherdim ;
}


float uiAxisHandler::getVal( int pix ) const
{
    float relpix;
    if ( isHor() )
	{ pix -= pixBefore(); relpix = pix; }
    else
	{ pix -= pixAfter(); relpix = axsz_-pix; }
    relpix /= axsz_;

    if ( setup_.islog_ )
	relpix = expf( relpix * logof2 );

    return rg_.start + (rgisrev_?-1:1) * rgwidth_ * relpix;
}


float uiAxisHandler::getRelPos( float v ) const
{
    float relv = rgisrev_ ? rg_.start - v : v - rg_.start;
    if ( !setup_.islog_ )
	return relv / rgwidth_;

    if ( relv < -0.9 ) relv = -0.9;
    return log( relv + 1 ) / logof2;
}


int uiAxisHandler::getRelPosPix( float relpos ) const
{
    return isHor() ? (int)(pixBefore() + axsz_ * relpos + .5)
		   : (int)(pixAfter() + axsz_ * (1 - relpos) + .5);
}


int uiAxisHandler::getPix( float pos ) const
{
    return getRelPosPix( getRelPos(pos) );
}


int uiAxisHandler::pixToEdge() const
{
    int ret = setup_.border_.get(setup_.side_);
    if ( setup_.noaxisannot_ ) return ret;

    ret += ticsz_ + (isHor() ? wdthy_ : wdthx_);
    return ret;
}


int uiAxisHandler::pixBefore() const
{
    if ( beghndlr_ ) return beghndlr_->pixToEdge();
    return setup_.border_.get( isHor() ? uiRect::Left : uiRect::Bottom );
}


int uiAxisHandler::pixAfter() const
{
    if ( endhndlr_ ) return endhndlr_->pixToEdge();
    return setup_.border_.get( isHor() ? uiRect::Right : uiRect::Top );
}


Interval<int> uiAxisHandler::pixRange() const
{
    if ( isHor() )
	return Interval<int>( pixBefore(), devsz_ - pixAfter() );
    else
	return Interval<int>( pixAfter(), devsz_ - pixBefore() );
}


void uiAxisHandler::createAnnotItems()
{
    if ( !annottxtitmgrp_ && setup_.noaxisannot_ ) return;
    
    if ( !annottxtitmgrp_ && !setup_.noaxisannot_ )
    {
	annottxtitmgrp_ = new uiGraphicsItemGroup();
	scene_->addItemGrp( annottxtitmgrp_ );
    }
    else if ( annottxtitmgrp_ )
	annottxtitmgrp_->removeAll( true );
    if ( !annotlineitmgrp_ && !setup_.noaxisannot_ )
    {
	annotlineitmgrp_ = new uiGraphicsItemGroup();
	scene_->addItemGrp( annotlineitmgrp_ );
    }
    else if ( annotlineitmgrp_ )
	annotlineitmgrp_->removeAll( true );
    
    if ( setup_.noaxisannot_ ) return;
    for ( int idx=0; idx<pos_.size(); idx++ )
    {
	const float relpos = pos_[idx] / endpos_;
	annotPos( getRelPosPix(relpos), strs_.get(idx), setup_.style_ );
    }
}


void uiAxisHandler::createGridLines()
{
    if ( gridlineitmgrp_ && !setup_.nogridline_ && !setup_.noaxisannot_ )
    {
	if ( gridlineitmgrp_->getSize() > 0 )
	{
	    for ( int idx=0; idx<gridlineitmgrp_->getSize(); idx++ )
	    {
		mDynamicCastGet(uiLineItem*,
				lineitm,gridlineitmgrp_->getUiItem(idx))
		uiRect linerect = lineitm->lineRect();
	    }
	}
    }
    if ( setup_.style_.isVisible() )
    {
	if ( !gridlineitmgrp_ && !setup_.nogridline_ && !setup_.noaxisannot_ )
	{
	    gridlineitmgrp_ = new uiGraphicsItemGroup();
	    scene_->addItemGrp( gridlineitmgrp_ );
	    gridlineitmgrp_->setZValue( 2 );
	}
	else if ( gridlineitmgrp_ )
	    gridlineitmgrp_->removeAll( true );

	if ( setup_.nogridline_ && setup_.noaxisannot_ ) return;
	
	Interval<int> toplot( 0, pos_.size()-1 );
	for ( int idx=0; idx<pos_.size(); idx++ )
	{
	    const float relpos = pos_[idx] / endpos_;
	    if ( relpos>0.01 && relpos<1.01 && (!endhndlr_ || relpos<0.99) )
		drawGridLine( getRelPosPix(relpos) );
	}
    }
}


void uiAxisHandler::plotAxis()
{
    drawAxisLine();

    if ( setup_.nogridline_ ) 
    {
	if ( gridlineitmgrp_ )
	    gridlineitmgrp_->removeAll( true );
    }
    
    createAnnotItems();  

    if ( !name().isEmpty() )
	drawName();

    if ( setup_.noaxisannot_ ) return;
    createGridLines();
}


void uiAxisHandler::drawAxisLine()
{
    if ( setup_.noaxisline_ )
    {
	if( axislineitm_ )
	    axislineitm_->setVisible(false );
	return;
    }
    LineStyle ls( setup_.style_ );
    ls.type_ = LineStyle::Solid;

    const int edgepix = pixToEdge();
    if ( isHor() )
    {
	const int startpix = pixBefore();
	const int endpix = devsz_-pixAfter();
	const int pixpos = setup_.side_ == uiRect::Top
	    		 ? edgepix : height_ - edgepix;
	if ( !axislineitm_ )
	    axislineitm_ = scene_->addLine( startpix, pixpos, endpix, pixpos );
	else
	    axislineitm_->setLine( startpix, pixpos, endpix, pixpos );
	axislineitm_->setPenStyle( ls );
	axislineitm_->setZValue( 3 );
    }
    else
    {
	const int startpix = pixAfter();
	const int endpix = devsz_ - pixBefore();
	const int pixpos = setup_.side_ == uiRect::Left
	    		 ? edgepix : width_ - edgepix;

	if ( !axislineitm_ )
	    axislineitm_ = scene_->addLine( pixpos, startpix, pixpos, endpix );
	else
	    axislineitm_->setLine( pixpos, startpix, pixpos, endpix );
	axislineitm_->setPenStyle( ls );
	axislineitm_->setZValue( 3 );
    }
}


void drawLine( uiLineItem& lineitm, const LinePars& lp,
	       const uiAxisHandler& xah, const uiAxisHandler& yah,
	       const Interval<float>* extxvalrg )
{
    const Interval<int> ypixrg( yah.pixRange() );
    const Interval<float> yvalrg( yah.getVal(ypixrg.start),
	    			  yah.getVal(ypixrg.stop) );
    Interval<int> xpixrg( xah.pixRange() );
    Interval<float> xvalrg( xah.getVal(xpixrg.start), xah.getVal(xpixrg.stop) );
    if ( extxvalrg )
    {
	xvalrg = *extxvalrg;
	xpixrg.start = xah.getPix( xvalrg.start );
	xpixrg.stop = xah.getPix( xvalrg.stop );
	xpixrg.sort();
	xvalrg.start = xah.getVal(xpixrg.start);
	xvalrg.stop = xah.getVal(xpixrg.stop);
    }

    uiPoint from(xpixrg.start,ypixrg.start), to(xpixrg.stop,ypixrg.stop);
    if ( lp.ax == 0 )
    {
	const int ypix = yah.getPix( lp.a0 );
	if ( !ypixrg.includes( ypix ) ) return;
	from.x = xpixrg.start; to.x = xpixrg.stop;
	from.y = to.y = ypix;
    }
    else
    {
	const float xx0 = xvalrg.start; const float yx0 = lp.getValue( xx0 );
 	const float xx1 = xvalrg.stop; const float yx1 = lp.getValue( xx1 );
	const float yy0 = yvalrg.start; const float xy0 = lp.getXValue( yy0 );
 	const float yy1 = yvalrg.stop; const float xy1 = lp.getXValue( yy1 );
	const bool yx0ok = yvalrg.includes( yx0 );
	const bool yx1ok = yvalrg.includes( yx1 );
	const bool xy0ok = xvalrg.includes( xy0 );
	const bool xy1ok = xvalrg.includes( xy1 );

	if ( !yx0ok && !yx1ok && !xy0ok && !xy1ok )
	    return;

	if ( yx0ok )
	{
	    from.x = xah.getPix( xx0 ); from.y = yah.getPix( yx0 );
	    if ( yx1ok )
		{ to.x = xah.getPix( xx1 ); to.y = yah.getPix( yx1 ); }
	    else if ( xy0ok )
		{ to.x = xah.getPix( xy0 ); to.y = yah.getPix( yy0 ); }
	    else if ( xy1ok )
		{ to.x = xah.getPix( xy1 ); to.y = yah.getPix( yy1 ); }
	    else
		return;
	}
	else if ( yx1ok )
	{
	    from.x = xah.getPix( xx1 ); from.y = yah.getPix( yx1 );
	    if ( xy0ok )
		{ to.x = xah.getPix( xy0 ); to.y = yah.getPix( yy0 ); }
	    else if ( xy1ok )
		{ to.x = xah.getPix( xy1 ); to.y = yah.getPix( yy1 ); }
	    else
		return;
	}
	else if ( xy0ok )
	{
	    from.x = xah.getPix( xy0 ); from.y = yah.getPix( yy0 );
	    if ( xy1ok )
		{ to.x = xah.getPix( xy1 ); to.y = yah.getPix( yy1 ); }
	    else
		return;
	}
    }

    lineitm.setLine( from, to, true );
}


void uiAxisHandler::annotAtEnd( const char* txt )
{
    const int edgepix = pixToEdge();
    int xpix, ypix; Alignment al;
    if ( isHor() )
    {
	xpix = devsz_ - pixAfter() - 2;
	ypix = setup_.side_ == uiRect::Top ? edgepix  
	    				   : height_ - edgepix - 2;
	al.set( Alignment::Left,
		setup_.side_==uiRect::Top ? Alignment::Bottom : Alignment::Top);
    }
    else
    {
	xpix = setup_.side_ == uiRect::Left  ? edgepix + 5 
	    				     : width_ - edgepix - 5;
	ypix = pixBefore() + 5;
	al.set( setup_.side_==uiRect::Left ? Alignment::Left : Alignment::Right,
		Alignment::VCenter );
    }


    if ( !endannottextitm_ )
	endannottextitm_ = scene_->addItem( new uiTextItem(txt,al) );
    else
	endannottextitm_->setText( txt );
    endannottextitm_->setPos( uiPoint(xpix,ypix) );
    endannottextitm_->setZValue( 3 );
}


void uiAxisHandler::annotPos( int pix, const char* txt, const LineStyle& ls )
{
    if ( setup_.noaxisannot_ ) return;
    const int edgepix = pixToEdge();
    if ( isHor() )
    {
	const bool istop = setup_.side_ == uiRect::Top;
	const int y0 = istop ? edgepix : height_ - edgepix;
	const int y1 = istop ? y0 - ticsz_ : y0 + ticsz_;
	uiLineItem* annotposlineitm = new uiLineItem();
	annotposlineitm->setLine( pix, y0, pix, y1 );
	annotposlineitm->setZValue( 3 );
	annotlineitmgrp_->add( annotposlineitm );
	Alignment al( Alignment::HCenter,
		      istop ? Alignment::Bottom : Alignment::Top );
	uiTextItem* annotpostxtitem =
	    new uiTextItem( uiPoint(pix,y1), txt, al );
	annotpostxtitem->setZValue( 3 );
	annotpostxtitem->setTextColor( ls.color_ );
	annottxtitmgrp_->add( annotpostxtitem );
    }
    else
    {
	const bool isleft = setup_.side_ == uiRect::Left;
	const int x0 = isleft ? edgepix : width_ - edgepix;
	const int x1 = isleft ? x0 - ticsz_ : x0 + ticsz_;
	uiLineItem* annotposlineitm = new uiLineItem();
	annotposlineitm->setLine( x0, pix, x1, pix );
	annotposlineitm->setZValue( 3 );
	Alignment al( isleft ? Alignment::Right : Alignment::Left,
		      Alignment::VCenter );
	uiTextItem* annotpostxtitem =
	    new uiTextItem( uiPoint(x1,pix), txt, al );
	annotpostxtitem->setZValue( 3 );
	annotpostxtitem->setTextColor( ls.color_ );
	annottxtitmgrp_->add( annotpostxtitem );
    }
}


void uiAxisHandler::drawGridLine( int pix )
{
    if ( setup_.nogridline_ ) return;
    const uiAxisHandler* hndlr = beghndlr_ ? beghndlr_ : endhndlr_;
    int endpix = setup_.border_.get( uiRect::across(setup_.side_) );
    if ( hndlr )
	endpix = setup_.side_ == uiRect::Left || setup_.side_ == uiRect::Bottom
	    	? hndlr->pixAfter() : hndlr->pixBefore();
    const int startpix = pixToEdge();

    uiLineItem* lineitem = new uiLineItem();
    switch ( setup_.side_ )
    {
    case uiRect::Top:
	lineitem->setLine( pix, startpix, pix, height_ - endpix );
	break;
    case uiRect::Bottom:
	lineitem->setLine( pix, endpix, pix, height_ - startpix );
	break;
    case uiRect::Left:
	lineitem->setLine( startpix, pix, width_ - endpix, pix );
	break;
    case uiRect::Right:
	lineitem->setLine( endpix, pix, width_ - startpix, pix );
	break;
    }
    lineitem->setPenStyle( setup_.style_ );
    gridlineitmgrp_->add( lineitem );
    gridlineitmgrp_->setVisible( setup_.style_.isVisible() );
}


void uiAxisHandler::drawName() 
{
    uiPoint pt;
    if ( !nameitm_ )
	nameitm_ = scene_->addItem( new uiTextItem(name()) );
    else
	nameitm_->setText( name() );
    nameitm_->setZValue( 3 );
    nameitm_->setTextColor( setup_.style_.color_ );
    if ( isHor() )
    {
	const bool istop = setup_.side_ == uiRect::Top;
	const int x = pixBefore() + axsz_ / 2;
	const int y = istop ? 2 : height_- pixBefore()/2;
	const Alignment al( Alignment::HCenter,
			    istop ? Alignment::Bottom : Alignment::Top );
	nameitm_->setPos( uiPoint(x,y) );
	nameitm_->setAlignment( al );
    }
    else
    {
	const bool isleft = setup_.side_ == uiRect::Left;
	const int x = isleft ? pixBefore() - wdthx_ + 5 : width_-3;
	const int y = height_ / 2;
	const Alignment al( Alignment::Left, Alignment::VCenter );
	nameitm_->setPos( uiPoint(x,y) );
	nameitm_->setAlignment( al );
	if ( !ynmtxtvertical_ )
	    nameitm_->rotate( 90 );
	ynmtxtvertical_ = true;
    }
}
