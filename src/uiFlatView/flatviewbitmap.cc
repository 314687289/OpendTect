/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:		Feb 2007
 RCS:           $Id: flatviewbitmap.cc,v 1.15 2008-04-08 11:21:25 cvsnanne Exp $
________________________________________________________________________

-*/

#include "flatviewbitmapmgr.h"
#include "flatviewbmp2rgb.h"
#include "array2dbitmapimpl.h"
#include "arrayndimpl.h"
#include "coltabsequence.h"
#include "coltabindex.h"
#include "uirgbarray.h"


FlatView::BitMapMgr::BitMapMgr( const FlatView::Viewer& vwr, bool wva )
    : vwr_(vwr)
    , bmp_(0)
    , pos_(0)
    , data_(0)
    , gen_(0)
    , wva_(wva)
    , sz_(mUdf(int),mUdf(int))
    , wr_(mUdf(double),mUdf(double),mUdf(double),mUdf(double))
{
    setupChg();
}


void FlatView::BitMapMgr::clearAll()
{
    delete bmp_;	bmp_ = 0;
    delete pos_;	pos_ = 0;
    delete data_;	data_ = 0;
    delete gen_;	gen_ = 0;
}


void FlatView::BitMapMgr::setupChg()
{
    clearAll();
    if ( !vwr_.isVisible(wva_) ) return;

    const FlatDataPack& dp = *vwr_.pack( wva_ );
    const FlatPosData& pd = dp.posData();
    const FlatView::Appearance& app = vwr_.appearance();
    const Array2D<float>& arr = dp.data();

    pos_ = new A2DBitMapPosSetup( arr.info(), pd.getPositions(true) );
    pos_->setDim1Positions( pd.range(false).start, pd.range(false).stop );
    data_ = new A2DBitMapInpData( arr );

    if ( !wva_ )
	gen_ = new VDA2DBitMapGenerator( *data_, *pos_ );
    else
    {
	const DataDispPars::WVA& wvapars = app.ddpars_.wva_;
	WVAA2DBitMapGenerator* wvagen
	    		= new WVAA2DBitMapGenerator( *data_, *pos_ );
	wvagen->wvapars().drawwiggles_ = wvapars.wigg_.isVisible();
	wvagen->wvapars().drawmid_ = wvapars.mid_.isVisible();
	wvagen->wvapars().fillleft_ = wvapars.left_.isVisible();
	wvagen->wvapars().fillright_ = wvapars.right_.isVisible();
	wvagen->wvapars().overlap_ = wvapars.overlap_;
	gen_ = wvagen;
    }
    const DataDispPars::Common* pars = &app.ddpars_.wva_;
    if ( !wva_ ) pars = &app.ddpars_.vd_;
    gen_->pars().clipratio_.start = pars->clipperc_.start * 0.01;
    gen_->pars().clipratio_.stop = mIsUdf(pars->clipperc_.stop)
	? mUdf(float)
	: pars->clipperc_.stop * 0.01;
    gen_->pars().midvalue_ = pars->midvalue_;

    gen_->pars().nointerpol_ = pars->blocky_;
    gen_->pars().fliplr_ = app.annot_.x1_.reversed_;
    gen_->pars().fliptb_ = !app.annot_.x2_.reversed_;
    		// in UI pixels, Y is reversed
    gen_->pars().autoscale_ = mIsUdf(pars->rg_.start) ||mIsUdf(pars->rg_.stop);
    if ( !gen_->pars().autoscale_ )
	gen_->pars().scale_ = pars->rg_;
}


Geom::Point2D<int> FlatView::BitMapMgr::dataOffs(
			const Geom::PosRectangle<double>& inpwr,
			const Geom::Size2D<int>& inpsz ) const
{
    Geom::Point2D<int> ret( mUdf(int), mUdf(int) );
    if ( mIsUdf(wr_.left()) ) return ret;

    // First see if we have different zooms:
    const Geom::Size2D<double> wrsz = wr_.size();
    const double xratio = wrsz.width() / sz_.width();
    const double yratio = wrsz.height() / sz_.height();
    const Geom::Size2D<double> inpwrsz = inpwr.size();
    const double inpxratio = inpwrsz.width() / inpsz.width();
    const double inpyratio = inpwrsz.height() / inpsz.height();
    if ( !mIsZero(xratio-inpxratio,mDefEps)
      || !mIsZero(yratio-inpyratio,mDefEps) )
	return ret;

    // Now check whether we have a pan outside buffered area:
    const bool xrev = wr_.right() < wr_.left();
    const bool yrev = wr_.top() < wr_.bottom();
    const double xoffs = fabs(xrev ? inpwr.right() - wr_.right()
	    			   : inpwr.left() - wr_.left()) / xratio;
    const double yoffs = fabs(yrev ? inpwr.top() - wr_.top()
	    			   : inpwr.bottom() - wr_.bottom()) / yratio;
    if ( xoffs <= -0.5 || yoffs <= -0.5 )
	return ret;
    const double maxxoffs = sz_.width() - inpsz.width() + .5;
    const double maxyoffs = sz_.height() - inpsz.height() + .5;
    if ( xoffs >= maxxoffs || yoffs >= maxyoffs )
	return ret;

    // No, we're cool. Return nearest integers
    ret.x = mNINT(xoffs); ret.y = mNINT(yoffs);
    return ret;
}


bool FlatView::BitMapMgr::generate( const Geom::PosRectangle<double>& wr,
				    const Geom::Size2D<int>& sz )
{
    if ( !gen_ )
	return true;

    mObtainDataPackToLocalVar( pack, const FlatDataPack*, DataPackMgr::FlatID,
	                                   vwr_.packID(wva_) );


    if ( !pack ) return true;

    const FlatPosData& pd = pack->posData();
    pos_->setDimRange( 0, Interval<float>(wr.left()-pd.offset(true),
					  wr.right()-pd.offset(true)) );
    pos_->setDimRange( 1, Interval<float>( wr.bottom(), wr.top() ) );

    bmp_ = new A2DBitMapImpl( sz.width(), sz.height() );
    if ( !bmp_ || !bmp_->isOK() || !bmp_->getData() )
    {
	delete bmp_; bmp_ = 0;
	DPM(DataPackMgr::FlatID).release(pack);
	return false;
    }

    wr_ = wr; sz_ = sz;
    A2DBitMapGenerator::initBitMap( *bmp_ );
    gen_->setBitMap( *bmp_ );
    gen_->fill();

    DPM(DataPackMgr::FlatID).release(pack);
    return true;
}


FlatView::BitMap2RGB::BitMap2RGB( const FlatView::Appearance& a,
				  uiRGBArray& arr )
    : app_(a)
    , arr_(arr)
{
}


void FlatView::BitMap2RGB::draw( const A2DBitMap* wva, const A2DBitMap* vd,
       				 const Geom::Point2D<int>& offs )
{
    if ( vd )
	drawVD( *vd, offs );
    if ( wva )
	drawWVA( *wva, offs );
}


void FlatView::BitMap2RGB::drawVD( const A2DBitMap& bmp,
				   const Geom::Point2D<int>& offs )
{
    const Geom::Size2D<int> bmpsz(bmp.info().getSize(0),bmp.info().getSize(1));
    const Geom::Size2D<int> arrsz(arr_.getSize(true),arr_.getSize(false));
    const FlatView::DataDispPars::VD& pars = app_.ddpars_.vd_;
    ColTab::Sequence ctab;
    ColTab::SM().get( pars.ctab_, ctab );

    const int minfill = (int)VDA2DBitMapGenPars::cMinFill;
    const int maxfill = (int)VDA2DBitMapGenPars::cMaxFill;
    ColTab::IndexedLookUpTable ctindex( ctab, maxfill-minfill+1 );

    for ( int ix=0; ix<arrsz.width(); ix++ )
    {
	if ( ix >= bmpsz.width() ) break;
	for ( int iy=0; iy<arrsz.height(); iy++ )
	{
	    if ( iy >= bmpsz.height() ) break;
	    const char bmpval = bmp.get( ix + offs.x, iy + offs.y );
	    if ( bmpval == A2DBitMapGenPars::cNoFill )
		continue;

	    Color col = ctindex.colorForIndex( (int)bmpval-minfill );
	    if ( col.isVisible() )
		arr_.set( ix, iy, col );
	}
    }
}


void FlatView::BitMap2RGB::drawWVA( const A2DBitMap& bmp,
				    const Geom::Point2D<int>& offs )
{
    const Geom::Size2D<int> bmpsz(bmp.info().getSize(0),bmp.info().getSize(1));
    const Geom::Size2D<int> arrsz(arr_.getSize(true),arr_.getSize(false));
    const FlatView::DataDispPars::WVA& pars = app_.ddpars_.wva_;

    for ( int ix=0; ix<arrsz.width(); ix++ )
    {
	if ( ix >= bmpsz.width() ) break;
	for ( int iy=0; iy<arrsz.height(); iy++ )
	{
	    if ( iy >= bmpsz.height() ) break;
	    const char bmpval = bmp.get( ix + offs.x, iy + offs.y );
	    if ( bmpval == A2DBitMapGenPars::cNoFill )
		continue;

	    Color col( pars.wigg_ );
	    if ( bmpval == WVAA2DBitMapGenPars::cLeftFill )
		col = pars.left_;
	    else if ( bmpval == WVAA2DBitMapGenPars::cRightFill )
		col = pars.right_;
	    else if ( bmpval == WVAA2DBitMapGenPars::cZeroLineFill )
		col = pars.mid_;

	    if ( col.isVisible() )
		arr_.set( ix, iy, col );
	}
    }
}

