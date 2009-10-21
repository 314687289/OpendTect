/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltiewavelet.cc,v 1.33 2009-10-21 09:36:10 cvsbruno Exp $";

#include "uiwelltiewavelet.h"

#include "flatposdata.h"
#include "ioman.h"
#include "pixmap.h"
#include "survinfo.h"
#include "wavelet.h"
#include "welltiedata.h"
#include "welltiesetup.h"
#include "welltiegeocalculator.h"

#include "uiaxishandler.h"
#include "uiseiswvltattr.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uiflatviewer.h"
#include "uifunctiondisplay.h"
#include "uilabel.h"
#include "uimsg.h"

#include <complex>

namespace WellTie
{

#define mErrRet(msg,act) \
{ uiMSG().error(msg); act; }
uiWaveletView::uiWaveletView( uiParent* p, WellTie::DataHolder* dh )
	: uiGroup(p)
	, dataholder_(dh)  
	, wvltctio_(*mMkCtxtIOObj(Wavelet))
	, activeWvltChged(this)		
{
    createWaveletFields( this );

    for ( int idx=0; idx<dataholder_->wvltset().size(); idx++ )
    {
	uiwvlts_ += new uiWavelet( this, dataholder_->wvltset()[idx], idx==0 );
	uiwvlts_[idx]->attach( ensureBelow, activewvltfld_ );
	if ( idx ) uiwvlts_[idx]->attach( rightOf, uiwvlts_[idx-1] );
	uiwvlts_[idx]->wvltChged.notify( mCB( 
				    this,uiWaveletView,activeWvltChanged ) );
    }
} 


uiWaveletView::~uiWaveletView()
{
    for ( int idx=0; idx<uiwvlts_.size(); idx++ )
	uiwvlts_[idx]->wvltChged.remove( 
				mCB(this,uiWaveletView,activeWvltChanged) );
    deepErase( uiwvlts_ );
}


void uiWaveletView::createWaveletFields( uiGroup* grp )
{
    grp->setHSpacing( 40 );
    
    activewvltfld_ = new uiGenInput( this, "Active Wavelet : ",
	 			BoolInpSpec(true,"Initial","Estimated")  );
    activewvltfld_->attach( hCentered );
    activewvltfld_->valuechanged.notify(
	   		 mCB(this, uiWaveletView, activeWvltChanged) );
}


void uiWaveletView::redrawWavelets()
{
    for ( int idx=0; idx<uiwvlts_.size(); idx++ )
	uiwvlts_[idx]->drawWavelet();
}


void uiWaveletView::activeWvltChanged( CallBacker* )
{
    const bool isinitactive = activewvltfld_->getBoolValue();
    dataholder_->dpms()->isinitwvltactive_ = activewvltfld_->getBoolValue();
    uiwvlts_[0]->setAsActive( isinitactive );
    uiwvlts_[1]->setAsActive( !isinitactive );
    activeWvltChged.trigger();
}



uiWavelet::uiWavelet( uiParent* p, Wavelet* wvlt, bool isactive )
    : uiGroup(p)
    , isactive_(isactive)  
    , wvlt_(wvlt)
    , wvltpropdlg_(0)		 
    , wvltChged(this)							 
{
    viewer_ = new uiFlatViewer( this );
    uiLabel* wvltlbl = new uiLabel( this, wvlt->name() );
    wvltlbl->attach( alignedAbove, viewer_);
    
    wvltbuts_ += new uiToolButton( this, "Properties", "info.png",
	    mCB(this,uiWavelet,dispProperties) );
    wvltbuts_[0]->setToolTip( "Properties" );
    wvltbuts_[0]->attach( alignedBelow, viewer_ );

    wvltbuts_ += new uiToolButton( this, "Rotate", "phase.png",
	    mCB(this,uiWavelet,rotatePhase) );
    wvltbuts_[1]->setToolTip( "Rotate Phase" );
    wvltbuts_[1]->attach( rightOf, wvltbuts_[0] );

    wvltbuts_ += new uiToolButton( this, "Rotate", "phase.png",
	    mCB(this,uiWavelet,taper) );
    wvltbuts_[2]->setToolTip( "Taper Wavelet" );
    wvltbuts_[2]->attach( rightOf, wvltbuts_[1] );

    initWaveletViewer();
    drawWavelet();
}


uiWavelet::~uiWavelet()
{
    const TypeSet<DataPack::ID> ids = viewer_->availablePacks();
    for ( int idx=ids.size()-1; idx>=0; idx-- )
	DPM( DataPackMgr::FlatID() ).release( ids[idx] );
    delete wvltpropdlg_;
}


void uiWavelet::initWaveletViewer()
{
    FlatView::Appearance& app = viewer_->appearance();
    app.annot_.x1_.name_ = "Amplitude";
    app.annot_.x2_.name_ =  "Time";
    app.annot_.setAxesAnnot( false );
    app.setGeoDefaults( true );
    app.ddpars_.show( true, false );
    app.ddpars_.wva_.overlap_ = 0;
    app.ddpars_.wva_.clipperc_.start = app.ddpars_.wva_.clipperc_.stop = 0;
    app.ddpars_.wva_.left_ = Color::NoColor();
    app.ddpars_.wva_.right_ = Color::Black();
    app.ddpars_.wva_.mid_ = Color::Black();
    app.ddpars_.wva_.symmidvalue_ = mUdf(float);
    app.setDarkBG( false );
    viewer_->setInitialSize( uiSize(80,100) );
    viewer_->setStretch( 1, 2 );
}


void uiWavelet::rotatePhase( CallBacker* )
{
    Wavelet* orgwvlt = new Wavelet( *wvlt_ );
    uiSeisWvltRotDlg dlg( this, wvlt_ );
    dlg.acting.notify( mCB(this,uiWavelet,wvltChanged) );
    if ( !dlg.go() )
    {
	memcpy(wvlt_->samples(),orgwvlt->samples(),wvlt_->size()*sizeof(float));
	wvltChanged(0);
    }
    dlg.acting.remove( mCB(this,uiWavelet,wvltChanged) );
}


void uiWavelet::taper( CallBacker* )
{
    Wavelet* orgwvlt = new Wavelet( *wvlt_ );
    uiSeisWvltTaperDlg dlg( this, wvlt_ );
    dlg.acting.notify( mCB(this,uiWavelet,wvltChanged) );
    if ( !dlg.go() )
    {
	memcpy(wvlt_->samples(),orgwvlt->samples(),wvlt_->size()*sizeof(float));
	wvltChanged(0);
    }
}


void uiWavelet::wvltChanged( CallBacker* )
{
    drawWavelet();
    if ( isactive_ ) wvltChged.trigger();
}


void uiWavelet::dispProperties( CallBacker* )
{
    delete wvltpropdlg_; wvltpropdlg_=0;
    wvltpropdlg_ = new uiWaveletDispPropDlg( this, wvlt_ );
    wvltpropdlg_ ->go();
}


void uiWavelet::setAsActive( bool isactive )
{
    isactive_ = isactive;
}


void uiWavelet::drawWavelet()
{
    if ( !wvlt_ ) return;

    const int wvltsz = wvlt_->size();
    Array2DImpl<float>* fva2d = new Array2DImpl<float>( 1, wvltsz );
    memcpy( fva2d->getData(), wvlt_->samples(), wvltsz * sizeof(float) );
    
    FlatDataPack* dp = new FlatDataPack( "Wavelet", fva2d );
    DPM( DataPackMgr::FlatID() ).add( dp ); dp->setName( wvlt_->name() );
    viewer_->setPack( true, dp->id(), false );
    DPM( DataPackMgr::FlatID() ).addAndObtain( dp );
    
    StepInterval<double> posns; posns.setFrom( wvlt_->samplePositions() );
    if ( SI().zIsTime() ) posns.scale( SI().zFactor() );
    dp->posData().setRange( false, posns );
    
    viewer_->setPack( true, dp->id(), false );
    viewer_->handleChange( uiFlatViewer::All );
}


}; //namespace WellTie
