/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2012
________________________________________________________________________

-*/


#include "uiseissingtrcdisp.h"
#include "arrayndimpl.h"
#include "flatposdata.h"
#include "seistrc.h"
#include "survinfo.h"
#include "wavelet.h"


uiSeisSingleTraceDisplay::uiSeisSingleTraceDisplay( uiParent* p )
    : uiFlatViewer(p)
    , compnr_(0)
    , curid_(DataPack::cNoID())
{
    FlatView::Appearance& app = appearance();
    app.annot_.x1_.name_ = toUiString(" ");
    app.annot_.x2_.name_ = toUiString(" ");
    app.annot_.setAxesAnnot( true );
    app.setGeoDefaults( true );
    app.ddpars_.show( true, false );
    app.ddpars_.wva_.overlap_ = 0;
    app.ddpars_.wva_.refline_ = Color::Black();
    app.ddpars_.wva_.mapper_->setup().setNoClipping();
    app.setDarkBG( false );

    setExtraBorders( uiSize(-10,5), uiSize(2,5) );
}


void uiSeisSingleTraceDisplay::cleanUp()
{
    removeAllAuxData();
    removePack( curid_ );
    curid_ = DataPack::cNoID();
}


void uiSeisSingleTraceDisplay::setData( const Wavelet* wvlt )
{
    cleanUp();

    if ( wvlt )
    {
	TypeSet<float> samps; wvlt->getSamples( samps );
	const int wvltsz = samps.size();
	const float zfac = mCast( float, SI().zDomain().userFactor() );

	Array2DImpl<float>* fva2d = new Array2DImpl<float>( 1, wvltsz );
	FlatDataPack* dp = new FlatDataPack( "Wavelet", fva2d );
	OD::memCopy( fva2d->getData(), samps.arr(), wvltsz*sizeof(float) );
	dp->setName( wvlt->name() );
	DPM( DataPackMgr::FlatID() ).add( dp );
	curid_ = dp->id();
	StepInterval<double> posns; posns.setFrom( wvlt->samplePositions() );
	if ( SI().zIsTime() )
	    posns.scale( zfac );
	dp->posData().setRange( false, posns );
    }

    setPack( true, curid_, false );
    addRefZ( 0 );

    handleChange( mCast(unsigned int,All) );
    setViewToBoundingBox();
}


void uiSeisSingleTraceDisplay::setData( const SeisTrc* trc, const char* nm )
{
    cleanUp();

    if ( trc )
    {
	const int trcsz = trc->size();
	const float zfac = mCast( float, SI().zDomain().userFactor() );

	Array2DImpl<float>* fva2d = new Array2DImpl<float>( 1, trcsz );
	FlatDataPack* dp = new FlatDataPack( "Wavelet", fva2d );
	float* ptr = fva2d->getData();
	for ( int idx=0; idx<trcsz; idx++ )
	    *ptr++ = trc->get( idx, compnr_ );
	dp->setName( nm );
	DPM( DataPackMgr::FlatID() ).add( dp );
	curid_ = dp->id();
	StepInterval<double> posns( trc->samplePos(0), trc->samplePos(trcsz-1),
				    trc->info().sampling_.step );
	if ( SI().zIsTime() ) posns.scale( zfac );
	dp->posData().setRange( false, posns );
    }

    setPack( true, curid_, false );

    if ( trc )
    {
	float refz = trc->info().zref_;
	if ( mIsZero(refz,1e-8) || mIsUdf(refz) )
	    refz = trc->info().pick_;
	if ( !mIsZero(refz,1e-8) && !mIsUdf(refz) )
	    addRefZ( refz );
    }

    handleChange( mCast(unsigned int,All) );
    setViewToBoundingBox();
}


void uiSeisSingleTraceDisplay::addRefZ( float zref )
{
    const float zfac = mCast( float, SI().zDomain().userFactor() );
    if ( SI().zIsTime() )
	zref *= zfac;

    const int curnraux = nrAuxData();
    FlatView::AuxData* ad = createAuxData(
				BufferString("Ref Z ",curnraux) );
    ad->poly_ += FlatView::Point( 0, zref );
    ad->markerstyles_ += OD::MarkerStyle2D( OD::MarkerStyle2D::HLine, 20,
				Color::stdDrawColor(curnraux) );
    ad->zvalue_ = 100;
    addAuxData( ad );

    handleChange( Annot );
}
