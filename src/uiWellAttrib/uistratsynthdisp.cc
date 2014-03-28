/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistratsynthdisp.h"
#include "uisynthgendlg.h"
#include "uistratsynthexport.h"
#include "uiseiswvltsel.h"
#include "uisynthtorealscale.h"
#include "uicombobox.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uiflatviewslicepos.h"
#include "uilabel.h"
#include "uimultiflatviewcontrol.h"
#include "uimsg.h"
#include "uipsviewer2dmainwin.h"
#include "uispinbox.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "stratsynth.h"
#include "stratsynthlevel.h"
#include "syntheticdataimpl.h"
#include "flatviewzoommgr.h"
#include "flatposdata.h"
#include "ptrman.h"
#include "propertyref.h"
#include "prestackgather.h"
#include "survinfo.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "synthseis.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "velocitycalc.h"
#include "wavelet.h"


static const int cMarkerSize = 6;

static const char* sKeySnapLevel()	{ return "Snap Level"; }
static const char* sKeyNrSynthetics()	{ return "Nr of Synthetics"; }
static const char* sKeySyntheticNr()	{ return "Synthetics Nr"; }
static const char* sKeySynthetics()	{ return "Synthetics"; }
static const char* sKeyNone()		{ return "None"; }

uiStratSynthDisp::uiStratSynthDisp( uiParent* p,
				    const Strat::LayerModelProvider& lmp )
    : uiGroup(p,"LayerModel synthetics display")
    , lmp_(lmp)
    , d2tmodels_(0)
    , stratsynth_(new StratSynth(lmp,false))
    , edstratsynth_(new StratSynth(lmp,true))
    , useed_(false)
    , dispeach_(1)
    , dispskipz_(0)
    , dispflattened_(false)
    , selectedtrace_(-1)
    , selectedtraceaux_(0)
    , levelaux_(0)
    , wvltChanged(this)
    , viewChanged(this)
    , modSelChanged(this)
    , synthsChanged(this)
    , dispParsChanged(this)
    , layerPropSelNeeded(this)
    , longestaimdl_(0)
    , lasttool_(0)
    , synthgendlg_(0)
    , prestackwin_(0)
    , currentwvasynthetic_(0)
    , currentvdsynthetic_(0)
    , autoupdate_(true)
    , forceupdate_(false)
    , isbrinefilled_(true)
    , taskrunner_( new uiTaskRunner(this) )
    , relzoomwr_(0,0,1,1)
{
    stratsynth_->setTaskRunner( taskrunner_ );
    edstratsynth_->setTaskRunner( taskrunner_ );

    topgrp_ = new uiGroup( this, "Top group" );
    topgrp_->setFrame( true );
    topgrp_->setStretch( 2, 0 );

    uiLabeledComboBox* datalblcbx =
	new uiLabeledComboBox( topgrp_, "Wiggle View", "" );
    wvadatalist_ = datalblcbx->box();
    wvadatalist_->selectionChanged.notify(
	    mCB(this,uiStratSynthDisp,wvDataSetSel) );
    wvadatalist_->setHSzPol( uiObject::Wide );

    uiToolButton* edbut = new uiToolButton( topgrp_, "edit",
				"Add/Edit Synthetic DataSet",
				mCB(this,uiStratSynthDisp,addEditSynth) );

    edbut->attach( leftOf, datalblcbx );

    uiGroup* dataselgrp = new uiGroup( this, "Data Selection" );
    dataselgrp->attach( rightBorder );
    dataselgrp->attach( ensureRightOf, topgrp_ );

    uiLabeledComboBox* prdatalblcbx =
	new uiLabeledComboBox( dataselgrp, "Variable Density View", "" );
    vddatalist_ = prdatalblcbx->box();
    vddatalist_->selectionChanged.notify(
	    mCB(this,uiStratSynthDisp,vdDataSetSel) );
    vddatalist_->setHSzPol( uiObject::Wide );
    prdatalblcbx->attach( leftBorder );

    uiToolButton* expbut = new uiToolButton( prdatalblcbx, "export",
				"Export Synthetic DataSet(s)",
				mCB(this,uiStratSynthDisp,exportSynth) );
    expbut->attach( rightOf, vddatalist_ );

    datagrp_ = new uiGroup( this, "DataSet group" );
    datagrp_->attach( ensureBelow, topgrp_ );
    datagrp_->attach( ensureBelow, dataselgrp );
    datagrp_->setFrame( true );
    datagrp_->setStretch( 2, 0 );

    uiToolButton* layertb = new uiToolButton( datagrp_, "defraytraceprops",
				    "Specify input for synthetic creation",
				    mCB(this,uiStratSynthDisp,layerPropsPush));

    wvltfld_ = new uiSeisWaveletSel( datagrp_ );
    wvltfld_->newSelection.notify( mCB(this,uiStratSynthDisp,wvltChg) );
    wvltfld_->setFrame( false );
    wvltfld_->attach( rightOf, layertb );
    curSS().setWavelet( wvltfld_->getWavelet() );

    scalebut_ = new uiPushButton( datagrp_, "Scale", false );
    scalebut_->activated.notify( mCB(this,uiStratSynthDisp,scalePush) );
    scalebut_->attach( rightOf, wvltfld_ );

    uiLabeledComboBox* lvlsnapcbx =
	new uiLabeledComboBox( datagrp_, "Snap level" );
    levelsnapselfld_ = lvlsnapcbx->box();
    lvlsnapcbx->attach( rightOf, scalebut_ );
    lvlsnapcbx->setStretch( 2, 0 );
    levelsnapselfld_->selectionChanged.notify(
				mCB(this,uiStratSynthDisp,levelSnapChanged) );
    levelsnapselfld_->addItems( VSEvent::TypeNames() );

    prestackgrp_ = new uiGroup( datagrp_, "Prestack View Group" );
    prestackgrp_->attach( rightOf, lvlsnapcbx, 20 );

    offsetposfld_ = new uiSynthSlicePos( prestackgrp_, "Offset" );
    offsetposfld_->positionChg.notify( mCB(this,uiStratSynthDisp,offsetChged));

    prestackbut_ = new uiToolButton( prestackgrp_, "nonmocorr64",
				"View Offset Direction",
				mCB(this,uiStratSynthDisp,viewPreStackPush) );
    prestackbut_->attach( rightOf, offsetposfld_);

    vwr_ = new uiFlatViewer( this );
    vwr_->rgbCanvas().disableImageSave();
    vwr_->setExtraBorders( uiRect( 0, 0 , 0, 0 ) );
    vwr_->setInitialSize( uiSize(800,300) ); //TODO get hor sz from laymod disp
    vwr_->setStretch( 2, 2 );
    vwr_->attach( ensureBelow, datagrp_ );
    vwr_->dispPropChanged.notify( mCB(this,uiStratSynthDisp,parsChangedCB) );
    FlatView::Appearance& app = vwr_->appearance();
    app.setGeoDefaults( true );
    app.setDarkBG( false );
    app.annot_.allowuserchangereversedaxis_ = false;
    app.annot_.title_.setEmpty();
    app.annot_.x1_.showAll( true );
    app.annot_.x2_.showAll( true );
    app.annot_.x1_.annotinint_ = true;
    app.annot_.x2_.name_ = "TWT (s)";
    app.ddpars_.show( true, true );
    app.ddpars_.wva_.allowuserchangedata_ = false;
    app.ddpars_.vd_.allowuserchangedata_ = false;
    vwr_->viewChanged.notify( mCB(this,uiStratSynthDisp,viewChg) );

    uiFlatViewStdControl::Setup fvsu( this );
    fvsu.withedit(false).withthumbnail(false).withcoltabed(false)
	.tba((int)uiToolBar::Right ).withflip(false).withsnapshot(false);
    control_ = new uiMultiFlatViewControl( *vwr_, fvsu );

    displayPostStackSynthetic( currentwvasynthetic_, true );
    displayPostStackSynthetic( currentvdsynthetic_, false );

    //mTriggerInstanceCreatedNotifier();
}


uiStratSynthDisp::~uiStratSynthDisp()
{
    delete stratsynth_;
    delete edstratsynth_;
    delete d2tmodels_;
}


void uiStratSynthDisp::addViewerToControl( uiFlatViewer& vwr )
{
    if ( control_ )
	control_->addViewer( vwr );
}


const Strat::LayerModel& uiStratSynthDisp::layerModel() const
{
    return lmp_.getCurrent();
}


void uiStratSynthDisp::layerPropsPush( CallBacker* )
{
    layerPropSelNeeded.trigger();
}


void uiStratSynthDisp::addTool( const uiToolButtonSetup& bsu )
{
    uiButton* tb = bsu.getButton( datagrp_ );
    if ( lasttool_ )
	tb->attach( leftOf, lasttool_ );
    else
	tb->attach( rightBorder );

    tb->attach( ensureRightOf, prestackbut_ );

    lasttool_ = tb;
}


void uiStratSynthDisp::cleanSynthetics()
{
    curSS().clearSynthetics();
    wvadatalist_->setEmpty();
    vddatalist_->setEmpty();
}


void uiStratSynthDisp::updateSyntheticList( bool wva )
{
    uiComboBox* datalist = wva ? wvadatalist_ : vddatalist_;
    BufferString curitem = datalist->text();
    datalist->setEmpty();
    datalist->addItem( sKeyNone() );
    for ( int idx=0; idx<curSS().nrSynthetics(); idx ++)
    {
	const SyntheticData* sd = curSS().getSyntheticByIdx( idx );
	if ( !sd ) continue;

	mDynamicCastGet(const StratPropSyntheticData*,prsd,sd);
	if ( wva && prsd ) continue;
	datalist->addItem( sd->name() );
    }

    datalist->setCurrentItem( curitem );
}


void uiStratSynthDisp::setDisplayZSkip( float zskip, bool withmodchg )
{
    dispskipz_ = zskip;
    if ( withmodchg )
	modelChanged();
}


void uiStratSynthDisp::setDispEach( int de )
{
    dispeach_ = de;
    displayPostStackSynthetic( currentwvasynthetic_, true );
    displayPostStackSynthetic( currentvdsynthetic_, false );
}


void uiStratSynthDisp::setSelectedTrace( int st )
{
    selectedtrace_ = st;

    delete vwr_->removeAuxData( selectedtraceaux_ );
    selectedtraceaux_ = 0;

    const StepInterval<double> xrg = vwr_->getDataPackRange(true);
    const StepInterval<double> zrg = vwr_->getDataPackRange(false);

    const float offset = mCast( float, xrg.start );
    if ( !xrg.includes( selectedtrace_ + offset, true ) )
	return;

    selectedtraceaux_ = vwr_->createAuxData( "Selected trace" );
    selectedtraceaux_->zvalue_ = 2;
    vwr_->addAuxData( selectedtraceaux_ );

    const double ptx = selectedtrace_ + offset;
    const double ptz1 = zrg.start;
    const double ptz2 = zrg.stop;

    Geom::Point2D<double> pt1 = Geom::Point2D<double>( ptx, ptz1 );
    Geom::Point2D<double> pt2 = Geom::Point2D<double>( ptx, ptz2 );

    selectedtraceaux_->poly_ += pt1;
    selectedtraceaux_->poly_ += pt2;
    selectedtraceaux_->linestyle_ =
	LineStyle( LineStyle::Dot, 2, Color::DgbColor() );

    vwr_->handleChange( FlatView::Viewer::Auxdata );
}


void uiStratSynthDisp::setDispMrkrs( const char* lnm,
				     const TypeSet<float>& zvals, Color col,
				     bool dispflattened )
{
    StratSynthLevel* lvl = new StratSynthLevel( lnm, col, &zvals );
    curSS().setLevel( lvl );

    const bool domodelchg = dispflattened_ || dispflattened;
    dispflattened_ = dispflattened;
    if ( domodelchg )
    {
	relzoomwr_ = uiWorldRect( 0, 0, 1, 1 );
	doModelChange();
	control_->zoomMgr().toStart();
	return;
    }

    levelSnapChanged(0);
}


void uiStratSynthDisp::setRelativeViewRect( const uiWorldRect& relwr )
{
    relzoomwr_ = relwr;
    uiWorldRect abswr;
    getAbsoluteViewRect( abswr );
    vwr_->setView( abswr );
}


void uiStratSynthDisp::setAbsoluteViewRect( const uiWorldRect& wr )
{
    uiWorldRect abswr = wr;
    uiWorldRect bbwr = vwr_->boundingBox();
    bbwr.sortCorners();
    abswr.sortCorners();
    if ( mIsZero(bbwr.width(),1e-3) || mIsZero(bbwr.height(),1e-3) ||
	 !bbwr.contains(abswr,1e-3) )
	return;

    const double left = (abswr.left() - bbwr.left())/bbwr.width();
    const double right = (abswr.right() - bbwr.left())/bbwr.width();
    const double top = (abswr.top() - bbwr.top())/bbwr.height();
    const double bottom = (abswr.bottom() - bbwr.top())/bbwr.height();
    relzoomwr_ = uiWorldRect( left, top, right, bottom );
}


void uiStratSynthDisp::getAbsoluteViewRect( uiWorldRect& abswr ) const
{
    uiWorldRect bbwr = vwr_->boundingBox();
    bbwr.sortCorners();
    if ( mIsZero(bbwr.width(),1e-3) || mIsZero(bbwr.height(),1e-3) )
	return;
    const double left = bbwr.left() + relzoomwr_.left()*bbwr.width();
    const double right = bbwr.left() + relzoomwr_.right()*bbwr.width();
    const double top = bbwr.top() + relzoomwr_.top()*bbwr.height();
    const double bottom = bbwr.top() + relzoomwr_.bottom()*bbwr.height();
    abswr = uiWorldRect( left, top, right, bottom );
}


void uiStratSynthDisp::resetRelativeViewRect()
{
    relzoomwr_ = uiWorldRect( 0, 0, 1, 1 );
}


void uiStratSynthDisp::setZoomView( const uiWorldRect& relwr )
{
    relzoomwr_ = relwr;
    uiWorldRect abswr;
    getAbsoluteViewRect( abswr );
    Geom::Point2D<double> centre = abswr.centre();
    Geom::Size2D<double> newsz = abswr.size();
    control_->setActiveVwr( 0 );
    control_->setNewView( centre, newsz );
}


void uiStratSynthDisp::setZDataRange( const Interval<double>& zrg, bool indpth )
{
    Interval<double> newzrg; newzrg.set( zrg.start, zrg.stop );
    if ( indpth && d2tmodels_ && !d2tmodels_->isEmpty() )
    {
	int mdlidx = longestaimdl_;
	if ( !d2tmodels_->validIdx(mdlidx) )
	    mdlidx = d2tmodels_->size()-1;

	const TimeDepthModel& d2t = *(*d2tmodels_)[mdlidx];
	newzrg.start = d2t.getTime( (float)zrg.start );
	newzrg.stop = d2t.getTime( (float)zrg.stop );
    }
    const Interval<double> xrg = vwr_->getDataPackRange( true );
    vwr_->setSelDataRanges( xrg, newzrg );
    vwr_->handleChange( mCast(unsigned int,FlatView::Viewer::All) );
}


void uiStratSynthDisp::levelSnapChanged( CallBacker* )
{
    const StratSynthLevel* lvl = curSS().getLevel();
    if ( !lvl )  return;
    StratSynthLevel* edlvl = const_cast<StratSynthLevel*>( lvl );
    VSEvent::Type tp;
    VSEvent::parseEnumType( levelsnapselfld_->text(), tp );
    edlvl->snapev_ = tp;
    drawLevel();
}


const char* uiStratSynthDisp::levelName()  const
{
    const StratSynthLevel* lvl = curSS().getLevel();
    return lvl ? lvl->name() : 0;
}


void uiStratSynthDisp::displayFRText()
{
    FlatView::AuxData* filltxtdata =
	vwr_->createAuxData( isbrinefilled_ ? "Brine filled"
					    : "Hydrocarbon filled" );
    filltxtdata->namepos_ = 0;
    uiWorldPoint txtpos =
	vwr_->boundingBox().bottomRight() - uiWorldPoint(10,10);
    filltxtdata->poly_ += txtpos;

    vwr_->addAuxData( filltxtdata );
    vwr_->handleChange( FlatView::Viewer::Annot );
}


void uiStratSynthDisp::drawLevel()
{
    delete vwr_->removeAuxData( levelaux_ );

    const StratSynthLevel* lvl = curSS().getLevel();
    const float offset =
	prestackgrp_->sensitive() ? mCast( float, offsetposfld_->getValue() )
				  : 0.0f;
    ObjectSet<const TimeDepthModel> curd2tmodels;
    getCurD2TModel( currentwvasynthetic_, curd2tmodels, offset );
    if ( !curd2tmodels.isEmpty() && lvl )
    {
	SeisTrcBuf& tbuf = const_cast<SeisTrcBuf&>( curTrcBuf() );
	FlatView::AuxData* auxd = vwr_->createAuxData("Level markers");
	curSS().getLevelTimes( tbuf, curd2tmodels );

	auxd->linestyle_.type_ = LineStyle::None;
	for ( int imdl=0; imdl<tbuf.size(); imdl ++ )
	{
	    if ( tbuf.get(imdl)->isNull() )
		continue;
	    const float tval =
		dispflattened_ ? 0 :  tbuf.get(imdl)->info().pick;

	    auxd->markerstyles_ += MarkerStyle2D( MarkerStyle2D::Target,
						  cMarkerSize, lvl->col_ );
	    auxd->poly_ += FlatView::Point( (imdl*dispeach_)+1, tval );
	}
	if ( auxd->isEmpty() )
	    delete auxd;
	else
	{
	    vwr_->addAuxData( auxd );
	    levelaux_ = auxd;
	}
    }

    vwr_->handleChange( FlatView::Viewer::Auxdata );
}


void uiStratSynthDisp::setCurrentWavelet()
{
    currentwvasynthetic_ = 0;
    curSS().setWavelet( wvltfld_->getWavelet() );
    SyntheticData* wvasd = curSS().getSynthetic( wvadatalist_->text() );
    SyntheticData* vdsd = curSS().getSynthetic( vddatalist_->text() );
    if ( !vdsd && !wvasd ) return;
    FixedString wvasynthnm( wvasd ? wvasd->name() : "" );
    FixedString vdsynthnm( vdsd ? vdsd->name() : "" );

    if ( wvasd )
    {
	wvasd->setWavelet( wvltfld_->getName() );
	currentwvasynthetic_ = wvasd;
	if ( synthgendlg_ )
	    synthgendlg_->updateWaveletName();
	currentwvasynthetic_->fillGenParams( curSS().genParams() );
	wvltChanged.trigger();
	updateSynthetic( wvasynthnm, true );
    }

    if ( vdsynthnm == wvasynthnm )
    {
	setCurrentSynthetic( false );
	return;
    }

    mDynamicCastGet(const StratPropSyntheticData*,prsd,vdsd);
    if ( vdsd && !prsd )
    {
	vdsd->setWavelet( wvltfld_->getName() );
	currentvdsynthetic_ = vdsd;
	if ( vdsynthnm != wvasynthnm )
	{
	    currentvdsynthetic_->fillGenParams( curSS().genParams() );
	    updateSynthetic( vdsynthnm, false );
	}
    }
}


void uiStratSynthDisp::wvltChg( CallBacker* )
{
    setCurrentWavelet();
    displaySynthetic( currentwvasynthetic_ );
    displayPostStackSynthetic( currentvdsynthetic_, false );
}


void uiStratSynthDisp::scalePush( CallBacker* )
{
    haveUserScaleWavelet();
}


bool uiStratSynthDisp::haveUserScaleWavelet()
{
    uiMsgMainWinSetter mws( mainwin() );
    SeisTrcBuf& tbuf = const_cast<SeisTrcBuf&>( curTrcBuf() );
    if ( tbuf.isEmpty() )
    {
	uiMSG().error( "Please generate layer models first.\n"
		"The scaling tool compares the amplitudes at the selected\n"
		"Stratigraphic Level to real amplitudes along a horizon" );
	return false;
    }
    BufferString levelname;
    if ( curSS().getLevel() )
	levelname = curSS().getLevel()->name();
    if ( levelname.isEmpty() || levelname.startsWith( "--" ) )
    {
	uiMSG().error( "Please select a Stratigraphic Level.\n"
		"The scaling tool compares the amplitudes there\n"
		"to real amplitudes along a horizon" );
	return false;
    }

    bool is2d = SI().has2D();
    if ( is2d && SI().has3D() )
    {
	int res = uiMSG().question( "Type of seismic data to use", "2D", "3D",
					"Cancel", "Specify geometry" );
	if ( res < 0 ) return false;
	is2d = res == 1;
    }

    bool rv = false;
    uiSynthToRealScale dlg( this, is2d, tbuf, wvltfld_->getID(), levelname );
    if ( dlg.go() )
    {
	MultiID mid( dlg.selWvltID() );
	if ( mid.isEmpty() )
	    pErrMsg( "Huh" );
	else
	{
	    rv = true;
	    wvltfld_->setInput( mid );
	}
	vwr_->handleChange( mCast(unsigned int,FlatView::Viewer::All) );
    }
    return rv;
}


void uiStratSynthDisp::parsChangedCB( CallBacker* )
{
    if ( currentvdsynthetic_ )
    {
	SynthFVSpecificDispPars& disppars = currentvdsynthetic_->dispPars();
	disppars.ctab_ = vwr_->appearance().ddpars_.vd_.ctab_;
	disppars.vdmapper_ = vwr_->appearance().ddpars_.vd_.mappersetup_;
    }

    if ( currentwvasynthetic_ )
    {
	SynthFVSpecificDispPars& disppars = currentwvasynthetic_->dispPars();
	disppars.wvamapper_ = vwr_->appearance().ddpars_.wva_.mappersetup_;
	disppars.overlap_ = vwr_->appearance().ddpars_.wva_.overlap_;
    }

    dispParsChanged.trigger();
}


void uiStratSynthDisp::viewChg( CallBacker* )
{
    setAbsoluteViewRect( curView(false) );
    viewChanged.trigger();
}


void uiStratSynthDisp::setSnapLevelSensitive( bool yn )
{
    levelsnapselfld_->setSensitive( yn );
}


float uiStratSynthDisp::centralTrcShift() const
{
    if ( !dispflattened_ ) return 0.0;
    bool forward = false;
    int forwardidx = mNINT32( vwr_->curView().centre().x );
    int backwardidx = forwardidx-1;
    const SeisTrcBuf& trcbuf = curTrcBuf();
    if ( !trcbuf.size() ) return 0.0f;
    while ( true )
    {
	if ( backwardidx<0 || forwardidx>=trcbuf.size() )
	    return 0.0f;
	const int centrcidx = forward ? forwardidx : backwardidx;
	const SeisTrc* centtrc = trcbuf.size() ? trcbuf.get( centrcidx ) :  0;
	if ( centtrc && !mIsUdf(centtrc->info().pick) )
	    return centtrc->info().pick;
	forward ? forwardidx++ : backwardidx--;
	forward = !forward;
    }

    return 0.0f;
}


const uiWorldRect& uiStratSynthDisp::curView( bool indpth ) const
{
    mDefineStaticLocalObject( Threads::Lock, lock, (true) );
    Threads::Locker locker( lock );

    mDefineStaticLocalObject( uiWorldRect, timewr, );
    timewr = vwr_->curView();
    if ( !indpth )
	return timewr;

    mDefineStaticLocalObject( uiWorldRect, depthwr, );
    depthwr.setLeft( timewr.left() );
    depthwr.setRight( timewr.right() );
    ObjectSet<const TimeDepthModel> curd2tmodels;
    getCurD2TModel( currentwvasynthetic_, curd2tmodels, 0.0f );
    if ( !curd2tmodels.isEmpty() )
    {
	for ( int idx=0; idx<curd2tmodels.size(); idx++ )
	{
	    const TimeDepthModel& d2t = *curd2tmodels[idx];
	    const double top = d2t.getDepth((float)timewr.top());
	    const double bottom = d2t.getDepth((float)timewr.bottom());
	    if ( idx==0 || top<depthwr.top() )
		depthwr.setTop( top );
	    if ( idx==0 || bottom>depthwr.bottom() )
		depthwr.setBottom( bottom );
	}
    }

    return depthwr;
}


const SeisTrcBuf& uiStratSynthDisp::curTrcBuf() const
{
    ConstDataPackRef<SeisTrcBufDataPack> tbdp = vwr_->obtainPack( true, true );
    if ( !tbdp )
    {
	mDefineStaticLocalObject( SeisTrcBuf, emptybuf, (false) );
	return emptybuf;
    }
    return tbdp->trcBuf();
}


#define mErrRet(s,act) \
{ uiMsgMainWinSetter mws( mainwin() ); if ( s ) uiMSG().error(s); act; }

void uiStratSynthDisp::modelChanged()
{
    doModelChange();
}


void uiStratSynthDisp::displaySynthetic( const SyntheticData* sd )
{
    displayPostStackSynthetic( sd );
    displayPreStackSynthetic( sd );
}

void uiStratSynthDisp::getCurD2TModel( const SyntheticData* sd,
		ObjectSet<const TimeDepthModel>& d2tmodels, float offset ) const
{
    if ( !sd )
	return;

    mDynamicCastGet(const PreStackSyntheticData*,presd,sd);
    if ( !presd )
    {
	d2tmodels = sd->d2tmodels_;
	return;
    }

    d2tmodels.erase();
    StepInterval<float> offsetrg( presd->offsetRange() );
    offsetrg.step = presd->offsetRangeStep();
    int offsidx = offsetrg.getIndex( offset );
    if ( offsidx<0 )
	offsidx = 0;
    const int nroffsets = offsetrg.nrSteps()+1;
    const SeisTrcBuf* tbuf = presd->getTrcBuf( offset );
    if ( !tbuf ) return;
    for ( int trcidx=0; trcidx<tbuf->size(); trcidx++ )
    {
	int d2tmodelidx = ( trcidx*nroffsets ) + offsidx;
	if ( !sd->d2tmodels_.validIdx(d2tmodelidx) )
	{
	    pErrMsg("Cannot find D2T Model for corresponding offset" );
	    d2tmodelidx = trcidx;
	}
	if ( !sd->d2tmodels_.validIdx(d2tmodelidx) )
	{
	    pErrMsg( "huh?" );
	    return;
	}
	d2tmodels += sd->d2tmodels_[d2tmodelidx];
    }
}


void uiStratSynthDisp::displayPostStackSynthetic( const SyntheticData* sd,
						     bool wva )
{
    const bool hadpack = vwr_->hasPack( wva );
    if ( hadpack )
	vwr_->removePack( vwr_->packID(wva) );
    vwr_->removeAllAuxData();
    delete d2tmodels_;
    d2tmodels_ = 0;
    if ( !sd )
    {
	SeisTrcBuf* disptbuf = new SeisTrcBuf( true );
	SeisTrcBufDataPack* dp = new SeisTrcBufDataPack( disptbuf, Seis::Line,
					SeisTrcInfo::TrcNr, "Forward Modeling");
	dp->posData().setRange( true, StepInterval<double>(1.0,1.0,1.0) );
	DPM( DataPackMgr::FlatID() ).add( dp );
	vwr_->setPack( wva, dp->id(), !hadpack );
	vwr_->setVisible( wva, false );
	return;
    }

    vwr_->setVisible( wva, true );
    mDynamicCastGet(const PreStackSyntheticData*,presd,sd);
    mDynamicCastGet(const PostStackSyntheticData*,postsd,sd);

    const float offset =
	prestackgrp_->sensitive() ? mCast( float, offsetposfld_->getValue() )
				  : 0.0f;
    const SeisTrcBuf* tbuf = presd ? presd->getTrcBuf( offset, 0 )
				   : &postsd->postStackPack().trcBuf();
    if ( !tbuf ) return;

    SeisTrcBuf* disptbuf = new SeisTrcBuf( true );
    tbuf->copyInto( *disptbuf );
    ObjectSet<const TimeDepthModel> curd2tmodels;
    getCurD2TModel( sd, curd2tmodels, offset );
    ObjectSet<const TimeDepthModel>* zerooffsd2tmodels =
	new ObjectSet<const TimeDepthModel>();
    getCurD2TModel( sd, *zerooffsd2tmodels, 0.0f );
    d2tmodels_ = zerooffsd2tmodels;
    float lasttime =  -mUdf(float);
    for ( int idx=0; idx<curd2tmodels.size(); idx++ )
    {
	if ( curd2tmodels[idx]->getLastTime() > lasttime )
	    longestaimdl_ = idx;
    }

    curSS().decimateTraces( *disptbuf, dispeach_ );
    if ( dispflattened_ )
    {
	curSS().getLevelTimes( *disptbuf, curd2tmodels );
	curSS().flattenTraces( *disptbuf );
    }
    else
        reSampleTraces( sd, *disptbuf );

    curSS().trimTraces( *disptbuf, curd2tmodels, dispskipz_);
    SeisTrcBufDataPack* dp = new SeisTrcBufDataPack( disptbuf, Seis::Line,
				    SeisTrcInfo::TrcNr, "Forward Modeling" );
    DPM( DataPackMgr::FlatID() ).add( dp );
    dp->setName( sd->name() );
    if ( !wva )
	vwr_->appearance().ddpars_.vd_.ctab_ = sd->dispPars().ctab_;
    else
	vwr_->appearance().ddpars_.wva_.overlap_ = sd->dispPars().overlap_;
    ColTab::MapperSetup& mapper =
	wva ? vwr_->appearance().ddpars_.wva_.mappersetup_
	    : vwr_->appearance().ddpars_.vd_.mappersetup_;
    mDynamicCastGet(const StratPropSyntheticData*,prsd,sd);
    SyntheticData* dispsd = const_cast< SyntheticData* > ( sd );
    ColTab::MapperSetup& dispparsmapper =
	!wva ? dispsd->dispPars().vdmapper_ : dispsd->dispPars().wvamapper_;
    const bool rgnotsaved = (mIsZero(dispparsmapper.range_.start,mDefEps) &&
			     mIsZero(dispparsmapper.range_.stop,mDefEps)) ||
			     dispparsmapper.range_.isUdf();
    if ( !rgnotsaved )
	mapper = dispparsmapper;
    else
    {
	const float cliprate = wva ? 0.0f : 0.025f;
	mapper.cliprate_ = Interval<float>(cliprate,cliprate);
	mapper.autosym0_ = true;
	mapper.type_ = ColTab::MapperSetup::Auto;
	mapper.symmidval_ = prsd ? mUdf(float) : 0.0f;
    }

    vwr_->setPack( wva, dp->id(), !hadpack );
    NotifyStopper notstop( vwr_->viewChanged );
    if ( mIsZero(relzoomwr_.left(),1e-3) &&
	 mIsEqual(relzoomwr_.width(),1.0,1e-3) &&
	 mIsEqual(relzoomwr_.height(),1.0,1e-3) )
	vwr_->setViewToBoundingBox();
    else
    {
	uiWorldRect abswr;
	getAbsoluteViewRect( abswr );
	vwr_->setView( abswr );
    }

    if ( rgnotsaved )
    {
	mapper.autosym0_ = false;
	mapper.type_ = ColTab::MapperSetup::Fixed;
	dispparsmapper = mapper;
    }
    displayFRText();
    levelSnapChanged( 0 );
}


void uiStratSynthDisp::reSampleTraces( const SyntheticData* sd,
				       SeisTrcBuf& tbuf ) const
{
    if ( longestaimdl_>=layerModel().size() || longestaimdl_<0 )
	return;
    Interval<float> depthrg = layerModel().sequence(longestaimdl_).zRange();
    const float offset =
	prestackgrp_->sensitive() ? mCast( float, offsetposfld_->getValue() )
				  : 0.0f;
    ObjectSet<const TimeDepthModel> curd2tmodels;
    mDynamicCastGet(const StratPropSyntheticData*,spsd,sd);
    getCurD2TModel( sd, curd2tmodels, offset );
    if ( !curd2tmodels.validIdx(longestaimdl_) )
	return;
    const TimeDepthModel& d2t = *curd2tmodels[longestaimdl_];
    const float reqlastzval =
	d2t.getTime( layerModel().sequence(longestaimdl_).zRange().stop );
    for ( int idx=0; idx<tbuf.size(); idx++ )
    {
	SeisTrc& trc = *tbuf.get( idx );

	const float lastzval = trc.info().sampling.atIndex( trc.size()-1 );
	const int lastsz = trc.size();
	if ( lastzval > reqlastzval )
	    continue;
	const int newsz = trc.info().sampling.nearestIndex( reqlastzval );
	trc.reSize( newsz, true );
	if ( spsd )
	{
	    const float lastval = trc.get( lastsz-1, 0 );
	    for ( int xtrasampidx=lastsz; xtrasampidx<newsz; xtrasampidx++ )
		trc.set( xtrasampidx, lastval, 0 );
	}
    }
}


void uiStratSynthDisp::displayPreStackSynthetic( const SyntheticData* sd )
{
    if ( !prestackwin_ ) return;

    if ( !sd ) return;
    mDynamicCastGet(const PreStack::GatherSetDataPack*,gsetdp,&sd->getPack())
    mDynamicCastGet(const PreStackSyntheticData*,presd,sd)
    if ( !gsetdp || !presd ) return;

    const PreStack::GatherSetDataPack& angledp = presd->angleData();
    prestackwin_->removeGathers();
    TypeSet<PreStackView::GatherInfo> gatherinfos;
    const ObjectSet<PreStack::Gather>& gathers = gsetdp->getGathers();
    const ObjectSet<PreStack::Gather>& anglegathers = angledp.getGathers();
    for ( int idx=0; idx<gathers.size(); idx++ )
    {
	const PreStack::Gather* gather = gathers[idx];
	const PreStack::Gather* anglegather= anglegathers[idx];

	PreStackView::GatherInfo gatherinfo;
	gatherinfo.isstored_ = false;
	gatherinfo.gathernm_ = sd->name();
	gatherinfo.bid_ = gather->getBinID();
	gatherinfo.wvadpid_ = gather->id();
	gatherinfo.vddpid_ = anglegather->id();
	gatherinfo.isselected_ = true;
	gatherinfos += gatherinfo;
    }

    prestackwin_->setGathers( gatherinfos );
}


void uiStratSynthDisp::setPreStackMapper()
{
    for ( int idx=0; idx<prestackwin_->nrViewers(); idx++ )
    {
	uiFlatViewer& vwr = prestackwin_->viewer( idx );
	ColTab::MapperSetup& vdmapper =
	    vwr.appearance().ddpars_.vd_.mappersetup_;
	vdmapper.cliprate_ = Interval<float>(0.0,0.0);
	vdmapper.autosym0_ = false;
	vdmapper.symmidval_ = mUdf(float);
	vdmapper.type_ = ColTab::MapperSetup::Fixed;
	vdmapper.range_ = Interval<float>(0,60);
	vwr.appearance().ddpars_.vd_.ctab_ = "Rainbow";
	ColTab::MapperSetup& wvamapper =
	    vwr.appearance().ddpars_.wva_.mappersetup_;
	wvamapper.cliprate_ = Interval<float>(0.0,0.0);
	wvamapper.autosym0_ = true;
	wvamapper.symmidval_ = 0.0f;
	vwr.handleChange( FlatView::Viewer::DisplayPars );
    }
}


void uiStratSynthDisp::selPreStackDataCB( CallBacker* cb )
{
    BufferStringSet allgnms, selgnms;
    for ( int idx=0; idx<curSS().nrSynthetics(); idx++ )
    {
	const SyntheticData* sd = curSS().getSyntheticByIdx( idx );
	mDynamicCastGet(const PreStackSyntheticData*,presd,sd);
	if ( !presd ) continue;
	allgnms.addIfNew( sd->name() );
    }

    TypeSet<PreStackView::GatherInfo> ginfos = prestackwin_->gatherInfos();

    prestackwin_->getGatherNames( selgnms );
    for ( int idx=0; idx<selgnms.size(); idx++ )
    {
	const int gidx = allgnms.indexOf( selgnms[idx]->buf() );
	if ( gidx<0 )
	    continue;
	allgnms.removeSingle( gidx );
    }

    PreStackView::uiViewer2DSelDataDlg seldlg( prestackwin_, allgnms, selgnms );
    if ( seldlg.go() )
    {
	prestackwin_->removeGathers();
	TypeSet<PreStackView::GatherInfo> newginfos;
	for ( int synthidx=0; synthidx<selgnms.size(); synthidx++ )
	{
	    const SyntheticData* sd =
		curSS().getSynthetic( selgnms[synthidx]->buf() );
	    if ( !sd ) continue;
	    mDynamicCastGet(const PreStackSyntheticData*,presd,sd);
	    mDynamicCastGet(const PreStack::GatherSetDataPack*,gsetdp,
			    &sd->getPack())
	    if ( !gsetdp || !presd ) continue;
	    const PreStack::GatherSetDataPack& angledp = presd->angleData();
	    for ( int idx=0; idx<ginfos.size(); idx++ )
	    {
		PreStackView::GatherInfo ginfo = ginfos[idx];
		ginfo.gathernm_ = sd->name();
		const PreStack::Gather* gather = gsetdp->getGather( ginfo.bid_);
		const PreStack::Gather* anglegather =
		    angledp.getGather( ginfo.bid_);
		ginfo.vddpid_ = anglegather->id();
		ginfo.wvadpid_ = gather->id();
		newginfos.addIfNew( ginfo );
	    }
	}

	prestackwin_->setGathers( newginfos, false );
    }

}


void uiStratSynthDisp::preStackWinClosedCB( CallBacker* )
{
    prestackwin_ = 0;
}


void uiStratSynthDisp::viewPreStackPush( CallBacker* cb )
{
    if ( !currentwvasynthetic_ || !currentwvasynthetic_->isPS() )
	return;
    if ( !prestackwin_ )
    {
	prestackwin_ =
	    new PreStackView::uiSyntheticViewer2DMainWin(this,"Prestack view");
	prestackwin_->seldatacalled_.notify(
		mCB(this,uiStratSynthDisp,selPreStackDataCB) );
	prestackwin_->windowClosed.notify(
		mCB(this,uiStratSynthDisp,preStackWinClosedCB) );
    }

    displayPreStackSynthetic( currentwvasynthetic_ );
    prestackwin_->show();
}


void uiStratSynthDisp::setCurrentSynthetic( bool wva )
{
    SyntheticData* sd = curSS().getSynthetic( wva ? wvadatalist_->text()
						  : vddatalist_->text() );
    if ( wva )
	currentwvasynthetic_ = sd;
    else
	currentvdsynthetic_ = sd;
    SyntheticData* cursynth = wva ? currentwvasynthetic_ : currentvdsynthetic_;

    if ( !cursynth ) return;

    NotifyStopper notstop( wvltfld_->newSelection );
    if ( wva )
    {
	wvltfld_->setInput( cursynth->waveletName() );
	curSS().setWavelet( wvltfld_->getWavelet() );
    }
}

void uiStratSynthDisp::updateFields()
{
    mDynamicCastGet(const PreStackSyntheticData*,pssd,currentwvasynthetic_);
    if ( pssd )
    {
	StepInterval<float> limits( pssd->offsetRange() );
	const float offsetstep = pssd->offsetRangeStep();
	limits.step = mIsUdf(offsetstep) ? 100 : offsetstep;
	offsetposfld_->setLimitSampling( limits );
    }

    prestackgrp_->setSensitive( pssd && pssd->hasOffset() );
    datagrp_->setSensitive( currentwvasynthetic_ );
}


void uiStratSynthDisp::showFRResults()
{
    setCurrentSynthetic( true );
    setCurrentSynthetic( false );
    displaySynthetic( currentwvasynthetic_ );
    displayPostStackSynthetic( currentvdsynthetic_, false );
    levelSnapChanged( 0 );
}


void uiStratSynthDisp::doModelChange()
{
    MouseCursorChanger mcs( MouseCursor::Busy );

    if ( !autoupdate_ && !forceupdate_ ) return;

    if ( curSS().errMsg() )
	mErrRet( curSS().errMsg(), return )
    if ( curSS().infoMsg() )
    {
	uiMsgMainWinSetter mws( mainwin() );
	uiMSG().warning( curSS().infoMsg() );
    }

    StratSynth* ss = useed_ ? stratsynth_ : edstratsynth_;
    ss->clearInfoMsg();
    updateSyntheticList( true );
    updateSyntheticList( false );
    wvadatalist_->setCurrentItem( 1 );
    vddatalist_->setCurrentItem( 1 );
    setCurrentSynthetic( true );
    setCurrentSynthetic( false );

    updateFields();
    displaySynthetic( currentwvasynthetic_ );
    displayPostStackSynthetic( currentvdsynthetic_, false );
}


void uiStratSynthDisp::updateSynthetic( const char* synthnm, bool wva )
{
    FixedString syntheticnm( synthnm );
    uiComboBox* datalist = wva ? wvadatalist_ : vddatalist_;
    if ( !datalist->isPresent(syntheticnm) || syntheticnm == sKeyNone() )
	return;
    if ( !curSS().removeSynthetic(syntheticnm) )
	return;
    SyntheticData* sd = curSS().addSynthetic();
    if ( !sd )
	mErrRet(curSS().errMsg(), return );
    if ( altSS().hasElasticModels() )
    {
	altSS().removeSynthetic( syntheticnm );
	altSS().genParams() = curSS().genParams();
	SyntheticData* altsd = altSS().addSynthetic();
	if ( !altsd )
	    mErrRet(altSS().errMsg(), return );
    }
    updateSyntheticList( wva );
    synthsChanged.trigger();

    datalist->setCurrentItem( sd->name() );
    setCurrentSynthetic( wva );
}


void uiStratSynthDisp::syntheticChanged( CallBacker* cb )
{
    BufferString syntheticnm;
    if ( cb )
    {
	mCBCapsuleUnpack(BufferString,synthname,cb);
	syntheticnm = synthname;
    }
    else
	syntheticnm = wvadatalist_->text();

    const BufferString curvdsynthnm(
	    currentvdsynthetic_ ? currentvdsynthetic_->name().buf() : "" );
    const BufferString curwvasynthnm(
	    currentwvasynthetic_ ? currentwvasynthetic_->name().buf() : "" );
    SyntheticData* cursd = curSS().getSynthetic( syntheticnm );
    if ( !cursd ) return;
    SynthGenParams curgp;
    cursd->fillGenParams( curgp );
    if ( !(curgp == curSS().genParams()) )
    {
	updateSynthetic( syntheticnm, true );
	updateSyntheticList( false );
    }
    else
    {
	wvadatalist_->setCurrentItem( syntheticnm );
	setCurrentSynthetic( true );
    }

    if ( curwvasynthnm == curvdsynthnm )
    {
	vddatalist_->setCurrentItem( currentwvasynthetic_->name() );
	setCurrentSynthetic( false );
    }

    updateFields();
    displaySynthetic( currentwvasynthetic_ );
    displayPostStackSynthetic( currentvdsynthetic_, false );
}


void uiStratSynthDisp::syntheticRemoved( CallBacker* cb )
{
    mCBCapsuleUnpack(BufferString,synthname,cb);
    if ( !curSS().removeSynthetic(synthname) )
	return;
    altSS().removeSynthetic( synthname );
    synthsChanged.trigger();
    updateSyntheticList( true );
    updateSyntheticList( false );
    wvadatalist_->setCurrentItem( 1 );
    vddatalist_->setCurrentItem( 1 );
    setCurrentSynthetic( true );
    setCurrentSynthetic( false );
    displayPostStackSynthetic( currentwvasynthetic_, true );
    displayPostStackSynthetic( currentvdsynthetic_, false );
}


void uiStratSynthDisp::addEditSynth( CallBacker* )
{
    if ( !synthgendlg_ )
    {
	synthgendlg_ = new uiSynthGenDlg( this, curSS());
	synthgendlg_->synthRemoved.notify(
		mCB(this,uiStratSynthDisp,syntheticRemoved) );
	synthgendlg_->synthChanged.notify(
		mCB(this,uiStratSynthDisp,syntheticChanged) );
	synthgendlg_->genNewReq.notify(
			    mCB(this,uiStratSynthDisp,genNewSynthetic) );
    }

    synthgendlg_->updateSynthNames();
    synthgendlg_->putToScreen();
    synthgendlg_->go();
}


void uiStratSynthDisp::exportSynth( CallBacker* )
{
    if ( layerModel().isEmpty() )
	mErrRet( "No valid layer model present", return )
    uiStratSynthExport dlg( this, curSS() );
    dlg.go();
}


void uiStratSynthDisp::offsetChged( CallBacker* )
{
    displayPostStackSynthetic( currentwvasynthetic_, true );
    if ( FixedString(wvadatalist_->text())==vddatalist_->text() &&
	 currentwvasynthetic_ && currentwvasynthetic_->isPS() )
	displayPostStackSynthetic( currentvdsynthetic_, false );
}


const PropertyRefSelection& uiStratSynthDisp::modelPropertyRefs() const
{ return layerModel().propertyRefs(); }


const ObjectSet<const TimeDepthModel>* uiStratSynthDisp::d2TModels() const
{ return d2tmodels_; }


void uiStratSynthDisp::vdDataSetSel( CallBacker* )
{
    setCurrentSynthetic( false );
    displayPostStackSynthetic( currentvdsynthetic_, false );
}


void uiStratSynthDisp::wvDataSetSel( CallBacker* )
{
    setCurrentSynthetic( true );
    updateFields();
    displayPostStackSynthetic( currentwvasynthetic_, true );
    //TODO check if it works doModelChange();
}


const ObjectSet<SyntheticData>& uiStratSynthDisp::getSynthetics() const
{
    return curSS().synthetics();
}


const Wavelet* uiStratSynthDisp::getWavelet() const
{
    return curSS().wavelet();
}


const MultiID& uiStratSynthDisp::waveletID() const
{
    return wvltfld_->getID();
}


void uiStratSynthDisp::genNewSynthetic( CallBacker* )
{
    if ( !synthgendlg_ )
	return;

    MouseCursorChanger mcchger( MouseCursor::Wait );
    SyntheticData* sd = curSS().addSynthetic();
    if ( !sd )
	mErrRet(curSS().errMsg(), return )
    if ( altSS().hasElasticModels() )
    {
	altSS().genParams() = curSS().genParams();
	SyntheticData* altsd = altSS().addSynthetic();
	if ( !altsd )
	    mErrRet(altSS().errMsg(), return );
    }
    updateSyntheticList( true );
    updateSyntheticList( false );
    synthsChanged.trigger();
    synthgendlg_->putToScreen();
    synthgendlg_->updateSynthNames();
}


SyntheticData* uiStratSynthDisp::getSyntheticData( const char* nm )
{
    return curSS().getSynthetic( nm );
}


SyntheticData* uiStratSynthDisp::getCurrentSyntheticData( bool wva ) const
{
    return wva ? currentwvasynthetic_ : currentvdsynthetic_;
}


void uiStratSynthDisp::fillPar( IOPar& par, const StratSynth* stratsynth ) const
{
    IOPar stratsynthpar;
    stratsynthpar.set( sKeySnapLevel(), levelsnapselfld_->currentItem());
    int nr_nonproprefsynths = 0;
    for ( int idx=0; idx<stratsynth->nrSynthetics(); idx++ )
    {
	const SyntheticData* sd = stratsynth->getSyntheticByIdx( idx );
	if ( !sd ) continue;
	mDynamicCastGet(const StratPropSyntheticData*,prsd,sd);
	if ( prsd ) continue;
	nr_nonproprefsynths++;
	SynthGenParams genparams;
	sd->fillGenParams( genparams );
	IOPar synthpar;
	genparams.fillPar( synthpar );
	sd->fillDispPar( synthpar );
	stratsynthpar.mergeComp( synthpar, IOPar::compKey(sKeySyntheticNr(),
				 nr_nonproprefsynths-1) );
    }

    stratsynthpar.set( sKeyNrSynthetics(), nr_nonproprefsynths );
    par.removeWithKey( sKeySynthetics() );
    par.mergeComp( stratsynthpar, sKeySynthetics() );
}


void uiStratSynthDisp::fillPar( IOPar& par, bool useed ) const
{
    fillPar( par, useed ? edstratsynth_ : stratsynth_ );
}


void uiStratSynthDisp::fillPar( IOPar& par ) const
{
    fillPar( par, &curSS() );
}


bool uiStratSynthDisp::prepareElasticModel()
{
    if ( !forceupdate_ && !autoupdate_ )
	return false;
    return curSS().createElasticModels();
}


bool uiStratSynthDisp::usePar( const IOPar& par )
{
    PtrMan<IOPar> stratsynthpar = par.subselect( sKeySynthetics() );
    if ( !curSS().hasElasticModels() )
	return false;
    curSS().clearSynthetics();
    if ( !stratsynthpar )
	curSS().addDefaultSynthetic();
    else
    {
	int nrsynths;
	stratsynthpar->get( sKeyNrSynthetics(), nrsynths );
	currentvdsynthetic_ = 0;
	currentwvasynthetic_ = 0;
	for ( int idx=0; idx<nrsynths; idx++ )
	{
	    PtrMan<IOPar> synthpar =
		stratsynthpar->subselect(IOPar::compKey(sKeySyntheticNr(),idx));
	    if ( !synthpar ) continue;
	    SynthGenParams genparams;
	    genparams.usePar( *synthpar );
	    wvltfld_->setInput( genparams.wvltnm_ );
	    curSS().setWavelet( wvltfld_->getWavelet() );
	    SyntheticData* sd = curSS().addSynthetic( genparams );
	    if ( !sd )
	    {
		mErrRet(curSS().errMsg(),);
		continue;
	    }

	    if ( useed_ )
	    {
		SyntheticData* nonfrsd = stratsynth_->getSyntheticByIdx( idx );
		IOPar synthdisppar;
		if ( nonfrsd )
		    nonfrsd->fillDispPar( synthdisppar );
		sd->useDispPar( synthdisppar );
	    }
	    else
		sd->useDispPar( *synthpar );
	}

	if ( !nrsynths )
	    curSS().addDefaultSynthetic();
    }

    if ( !curSS().nrSynthetics() )
    {
	displaySynthetic( 0 );
	displayPostStackSynthetic( 0, false );
	return false;
    }

    curSS().generateOtherQuantities();
    synthsChanged.trigger();

    if ( stratsynthpar )
    {
	int snaplvl = 0;
	stratsynthpar->get( sKeySnapLevel(), snaplvl );
	levelsnapselfld_->setCurrentItem( snaplvl );
    }

    return true;
}


uiSynthSlicePos::uiSynthSlicePos( uiParent* p, const char* lbltxt )
    : uiGroup( p, "Slice Pos" )
    , positionChg(this)
{
    label_ = new uiLabel( this, lbltxt );
    sliceposbox_ = new uiSpinBox( this, 0, "Slice position" );
    sliceposbox_->valueChanging.notify( mCB(this,uiSynthSlicePos,slicePosChg));
    sliceposbox_->valueChanged.notify( mCB(this,uiSynthSlicePos,slicePosChg));
    sliceposbox_->attach( rightOf, label_ );

    uiLabel* steplabel = new uiLabel( this, "Step" );
    steplabel->attach( rightOf, sliceposbox_ );

    slicestepbox_ = new uiSpinBox( this, 0, "Slice step" );
    slicestepbox_->attach( rightOf, steplabel );

    prevbut_ = new uiToolButton( this, "prevpos", "Previous position",
				mCB(this,uiSynthSlicePos,prevCB) );
    prevbut_->attach( rightOf, slicestepbox_ );
    nextbut_ = new uiToolButton( this, "nextpos", "Next position",
				 mCB(this,uiSynthSlicePos,nextCB) );
    nextbut_->attach( rightOf, prevbut_ );
}


void uiSynthSlicePos::slicePosChg( CallBacker* )
{
    positionChg.trigger();
}


void uiSynthSlicePos::prevCB( CallBacker* )
{
    uiSpinBox* posbox = sliceposbox_;
    uiSpinBox* stepbox = slicestepbox_;
    posbox->setValue( posbox->getValue()-stepbox->getValue() );
}


void uiSynthSlicePos::nextCB( CallBacker* )
{
    uiSpinBox* posbox = sliceposbox_;
    uiSpinBox* stepbox = slicestepbox_;
    posbox->setValue( posbox->getValue()+stepbox->getValue() );
}


void uiSynthSlicePos::setLimitSampling( StepInterval<float> lms )
{
    limitsampling_ = lms;
    sliceposbox_->setInterval( lms.start, lms.stop );
    sliceposbox_->setStep( lms.step );
    slicestepbox_->setValue( lms.step );
    slicestepbox_->setStep( lms.step );
    slicestepbox_->setMinValue( lms.step );
}


void uiSynthSlicePos::setValue( int val ) const
{
    sliceposbox_->setValue( val );
}


int uiSynthSlicePos::getValue() const
{
    return sliceposbox_->getValue();
}
