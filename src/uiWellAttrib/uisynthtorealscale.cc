/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uisynthtorealscale.h"

#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "survinfo.h"
#include "polygon.h"
#include "position.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seisbuf.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "stratlevel.h"
#include "statparallelcalc.h"
#include "picksettr.h"
#include "wavelet.h"
#include "ioman.h"

#include "uislider.h"
#include "uistratseisevent.h"
#include "uiseissel.h"
#include "uiseparator.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uihistogramdisplay.h"
#include "uiaxishandler.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

static const char* sKeyAmplVals = "       [Amplitude values]       ";
static const char* sKeyRMSVals = "[Amplitude RMS values]";


#define mErrRetBool(s)\
{ uiMSG().error(s); return false; }\

#define mErrRet(s)\
{ uiMSG().error(s); return; }\

class uiSynthToRealScaleStatsDisp : public uiGroup
{
public:

uiSynthToRealScaleStatsDisp( uiParent* p, const char* nm, bool left )
    : uiGroup(p,nm)
    , usrval_(mUdf(float))
    , usrValChanged(this)
    , markerlineitem_(0)
{
    uiFunctionDisplay::Setup su;
    su.annoty( false ).noyaxis( true ).noy2axis( true ).drawgridlines( false );
    dispfld_ = new uiHistogramDisplay( this, su );
    dispfld_->xAxis()->setCaption( "" );
    dispfld_->setPrefWidth( 260 );
    dispfld_->setPrefHeight( GetGoldenMinor(260) );

    uiSlider::Setup slsu;
    slsu.withedit( true );
    valueslider_ = new uiSlider( this, slsu, "Value" );
    valueslider_->valueChanged.notify(
	    mCB(this,uiSynthToRealScaleStatsDisp,sliderChgCB) );
    valueslider_->attach( alignedBelow, dispfld_ );
    valueslider_->setStretch( 2, 1 );

    uiLabel* lbl = new uiLabel( this, nm );
    dispfld_->attach( centeredBelow, lbl );
    setHAlignObj( dispfld_ );
}

void updateSlider( float val )
{
    const uiAxisHandler* xaxis = dispfld_->xAxis();
    const StepInterval<float> xrg = xaxis->range();
    valueslider_->setScale( xrg.step/1000, 0 );
    valueslider_->setInterval( xrg );
    valueslider_->setValue( val );
    drawMarkerLine( val );
}

void sliderChgCB( CallBacker* )
{
    usrval_ = valueslider_->getValue();
    drawMarkerLine( usrval_ );
    usrValChanged.trigger();
}

void drawMarkerLine( float val )
{
    const uiAxisHandler* xaxis = dispfld_->xAxis();
    const int valx = xaxis->getPix( val );
    if ( valx < xaxis->getPix( xaxis->range().start) ||
	 valx > xaxis->getPix( xaxis->range().stop) )
	return;

    const uiAxisHandler* yaxis = dispfld_->yAxis(false);
    const int valytop = yaxis->getPix( yaxis->range().start );
    const int valybottom = yaxis->getPix( yaxis->range().stop );

    if ( !markerlineitem_ )
    {
	LineStyle ls( LineStyle::Solid, 2, Color(0,255,0) );
	markerlineitem_ = dispfld_->scene().addItem( new uiLineItem() );
	markerlineitem_->setPenStyle( ls );
	markerlineitem_->setZValue( 3 );
    }

    markerlineitem_->setLine( valx, valytop, valx, valybottom );
    usrval_ = val;
}

    float		usrval_;

    uiHistogramDisplay*	dispfld_;
    uiSlider*		valueslider_;
    uiLineItem*		markerlineitem_;
    Notifier<uiSynthToRealScaleStatsDisp>	usrValChanged;

};


uiSynthToRealScale::uiSynthToRealScale( uiParent* p, bool is2d,
					const SeisTrcBuf& tb,
					const MultiID& wid, const char* lvlnm )
    : uiDialog(p,Setup("Scale synthetics","Determine scaling for synthetics",
			mODHelpKey(mSynthToRealScaleHelpID) ))
    , seisev_(*new Strat::SeisEvent)
    , is2d_(is2d)
    , synth_(tb)
    , inpwvltid_(wid)
    , seisfld_(0)
    , horizon_(0)
    , horiter_(0)
    , polygon_(0)
{
#define mNoDealRet(cond,msg) \
    if ( cond ) \
	{ new uiLabel( this, msg ); return; }
    mNoDealRet( Strat::LVLS().isEmpty(), "No Stratigraphic Levels defined" )
    mNoDealRet( tb.isEmpty(), "Please generate models first" )
    mNoDealRet( inpwvltid_.isEmpty(), "Please create a Wavelet first" )
    mNoDealRet( !lvlnm || !*lvlnm || (*lvlnm == '-' && *(lvlnm+1) == '-'),
	    "Please select Stratigraphic Level\nbefore starting this tool" )

    BufferString wintitle( "Determine scaling for synthetics using '" );
    wintitle.add( IOM().nameOf( inpwvltid_ ) ).add( "'" );
    setTitleText( wintitle );

    uiSeisSel::Setup sssu( is2d_, false );
    seisfld_ = new uiSeisSel( this, uiSeisSel::ioContext(sssu.geom_,true),
			      sssu );

    const IOObjContext horctxt( mIOObjContext(EMHorizon3D) );
    uiIOObjSel::Setup horsu( BufferString("Horizon for '",lvlnm,"'") );
    horfld_ = new uiIOObjSel( this, horctxt, horsu );
    horfld_->attach( alignedBelow, seisfld_ );

    IOObjContext polyctxt( mIOObjContext(PickSet) );
    polyctxt.toselect.require_.set( sKey::Type(), sKey::Polygon() );
    uiIOObjSel::Setup polysu( "Within Polygon" ); polysu.optional( true );
    polyfld_ = new uiIOObjSel( this, polyctxt, polysu );
    polyfld_->attach( alignedBelow, horfld_ );

    uiStratSeisEvent::Setup ssesu( true );
    ssesu.fixedlevel( Strat::LVLS().get(lvlnm) );
    evfld_ = new uiStratSeisEvent( this, ssesu );
    evfld_->attach( alignedBelow, polyfld_ );

    uiPushButton* gobut = new uiPushButton( this, "&Extract amplitudes",
				mCB(this,uiSynthToRealScale,goPush), true );
    gobut->attach( alignedBelow, evfld_ );

    uiSeparator* sep = new uiSeparator( this, "separator" );
    sep->attach( stretchedBelow, gobut );

    valislbl_ = new uiLabel( this, sKeyAmplVals );
    valislbl_->setAlignment( Alignment::HCenter );
    valislbl_->attach( centeredBelow, sep );

    uiGroup* statsgrp = new uiGroup( this, "Stats displays" );

    synthstatsfld_ = new uiSynthToRealScaleStatsDisp( statsgrp, "Synthetics",
						      true );
    realstatsfld_ = new uiSynthToRealScaleStatsDisp( statsgrp, "Real Seismics",
						     false );
    realstatsfld_->attach( rightOf, synthstatsfld_ );
    const CallBack setsclcb( mCB(this,uiSynthToRealScale,setScaleFld) );
    synthstatsfld_->usrValChanged.notify( setsclcb );
    statsgrp->attach( centeredBelow, valislbl_ );
    realstatsfld_->usrValChanged.notify( setsclcb );
    statsgrp->setHAlignObj( realstatsfld_ );

    finalscalefld_ = new uiGenInput( this, "", FloatInpSpec() );
    finalscalefld_->attach( centeredBelow, statsgrp );
    new uiLabel( this, "Scaling factor", finalscalefld_ );

    IOObjContext wvltctxt( mIOObjContext(Wavelet) );
    wvltctxt.forread = false;
    wvltfld_ = new uiIOObjSel( this, wvltctxt, "Save scaled Wavelet as" );
    wvltfld_->attach( alignedBelow, finalscalefld_ );

    postFinalise().notify( mCB(this,uiSynthToRealScale,initWin) );
}


uiSynthToRealScale::~uiSynthToRealScale()
{
    delete horiter_;
    if ( horizon_ )
	horizon_->unRef();
    delete polygon_;
    delete &seisev_;
}


void uiSynthToRealScale::initWin( CallBacker* )
{
    updSynthStats();
}

#define mUpdateSlider( type, val ) \
     if ( !mIsUdf(val) ) \
	type->updateSlider( val ); \

void uiSynthToRealScale::setScaleFld( CallBacker* )
{
    const float synthval = synthstatsfld_->usrval_;
    const float realval = realstatsfld_->usrval_;
    if ( mIsUdf(synthval) || mIsUdf(realval) || synthval == 0 )
	finalscalefld_->setValue( mUdf(float) );
    else
	finalscalefld_->setValue( realval / synthval );

    mUpdateSlider( synthstatsfld_, synthval );
    mUpdateSlider( realstatsfld_, realval );
}


bool uiSynthToRealScale::getEvent()
{
    if ( !evfld_->getFromScreen() )
	return false;
    seisev_ = evfld_->event();
    const bool isrms = evfld_->getFullExtrWin().nrSteps() > 0;
    valislbl_->setText( isrms ? sKeyRMSVals : sKeyAmplVals );
    return true;
}


bool uiSynthToRealScale::getHorData( TaskRunner& tr )
{
    delete polygon_; polygon_ = 0;
    if ( horizon_ )
	{ horizon_->unRef(); horizon_ = 0; }

    if ( polyfld_->isChecked() )
    {
	const IOObj* ioobj = polyfld_->ioobj();
	if ( !ioobj ) return false;
	BufferString errmsg;
	polygon_ = PickSetTranslator::getPolygon( *ioobj, errmsg );
	if ( !polygon_ )
	    mErrRetBool( errmsg );
    }

    const IOObj* ioobj = horfld_->ioobj();
    if ( !ioobj ) return false;
    EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded( ioobj->key(), &tr );
    mDynamicCastGet(EM::Horizon3D*,hor,emobj);
    if ( !hor ) return false;
    horizon_ = hor;
    horizon_->ref();
    horiter_ = horizon_->createIterator( horizon_->sectionID(0) );
    return true;
}


float uiSynthToRealScale::getTrcValue( const SeisTrc& trc, float reftm ) const
{
    const int lastsamp = trc.size() - 1;
    const Interval<float> trg( trc.startPos(), trc.samplePos(lastsamp) );
    const StepInterval<float> extrwin( evfld_->getFullExtrWin() );
    const int nrtms = extrwin.nrSteps() + 1;
    const bool calcrms = nrtms > 1;
    float sumsq = 0;
    for ( int itm=0; itm<nrtms; itm++ )
    {
	float extrtm = reftm + extrwin.atIndex( itm );
	float val;
	if ( extrtm <= trg.start )
	    val = trc.get( 0, 0 );
	if ( extrtm >= trg.stop )
	    val = trc.get( lastsamp, 0 );
	else
	    val = trc.getValue( extrtm, 0 );
	if ( calcrms ) val *= val;
	sumsq += val;
    }
    return calcrms ? Math::Sqrt( sumsq / nrtms ) : sumsq;
}


void uiSynthToRealScale::updSynthStats()
{
    if ( !getEvent() )
	return;

    TypeSet<float> vals;
    for ( int idx=0; idx<synth_.size(); idx++ )
    {
	const SeisTrc& trc = *synth_.get( idx );
	const float reftm = seisev_.snappedTime( trc );
	if ( !mIsUdf(reftm) )
	    vals += getTrcValue( trc, reftm );
    }

    uiHistogramDisplay& histfld = *synthstatsfld_->dispfld_;
    histfld.setData( vals.arr(), vals.size() );
    histfld.putN();
    synthstatsfld_->updateSlider( (float)histfld.getStatCalc().average() );
}


class uiSynthToRealScaleRealStatCollector : public Executor
{
public:
uiSynthToRealScaleRealStatCollector( uiSynthToRealScale& d, SeisTrcReader& r )
    : Executor( "Collect Amplitudes" )
    , dlg_(d)
    , rdr_(r)
    , msg_("Collecting")
    , nrdone_(0)
    , totalnr_(-1)
    , seldata_(0)
{
    if ( dlg_.polygon_ )
	seldata_ = new Seis::PolySelData( *dlg_.polygon_ );
    if ( seldata_ )
	totalnr_ = seldata_->expectedNrTraces( dlg_.is2d_ );
    else
	totalnr_ = dlg_.horiter_->approximateSize();
}

~uiSynthToRealScaleRealStatCollector()
{
    delete seldata_;
}

uiString uiMessage() const	{ return msg_; }
uiString uiNrDoneText() const	{ return "Traces handled"; }
od_int64 nrDone() const		{ return nrdone_; }
od_int64 totalNr() const	{ return totalnr_; }

bool getNextPos3D()
{
    while ( true )
    {
	const EM::PosID posid = dlg_.horiter_->next();
	if ( posid.isUdf() )
	    return false;
	const Coord3 crd = dlg_.horizon_->getPos( posid );
	if ( setBinID(crd) )
	{
	    z_ = (float)crd.z;
	    break;
	}
    }

    return true;
}

bool setBinID( const Coord& crd )
{
    bid_ = SI().transform( crd );
    return seldata_ ? seldata_->isOK(bid_) : true;
}


int getTrc3D()
{
    while ( true )
    {
	if ( !getNextPos3D() )
	    return Finished();
	else if ( !rdr_.seisTranslator()->goTo(bid_) )
	    continue;
	else if ( !rdr_.get(trc_) )
	    { msg_ = rdr_.errMsg(); return ErrorOccurred(); }

	break;
    }
    return MoreToDo();
}

int getTrc2D()
{
    if ( !rdr_.get(trc_) )
	return Finished();

    if ( !setBinID(trc_.info().coord) )
	return 2;

    const EM::SubID subid = bid_.toInt64();
    const Coord3 crd = dlg_.horizon_->getPos( 0, subid );
    if ( mIsUdf(crd.z) )
	return 2;

    z_ = (float)crd.z;
    return MoreToDo();
}

int nextStep()
{
    const int res = dlg_.is2d_ ? getTrc2D() : getTrc3D();
    if ( res <= 0 )
	return res;
    nrdone_++;
    if ( res > 1 )
	return MoreToDo();

    const float val = dlg_.getTrcValue( trc_, z_ );
    if ( !mIsUdf(val) )
	vals_ += val;
    return MoreToDo();
}

    uiSynthToRealScale&	dlg_;
    SeisTrcReader&	rdr_;
    Seis::SelData*	seldata_;
    SeisTrc		trc_;
    uiString		msg_;
    od_int64		nrdone_;
    od_int64		totalnr_;
    BinID		bid_;
    float		z_;
    TypeSet<float>	vals_;

};


void uiSynthToRealScale::updRealStats()
{
    if ( !getEvent() )
	return;

    uiTaskRunner taskrunner( this );
    if ( !getHorData(taskrunner) )
	return;

    SeisTrcReader rdr( seisfld_->ioobj() );
    if ( !rdr.prepareWork() )
	mErrRet( "Error opening input seismic data" );

    uiSynthToRealScaleRealStatCollector coll( *this, rdr );
    if ( !TaskRunner::execute( &taskrunner, coll ) )
	return;

    uiHistogramDisplay& histfld = *realstatsfld_->dispfld_;
    histfld.setData( coll.vals_.arr(), coll.vals_.size() );
    histfld.putN();
    realstatsfld_->updateSlider( (float)histfld.getStatCalc().average() );
    setScaleFld( 0 );
}


bool uiSynthToRealScale::acceptOK( CallBacker* )
{
    if ( !evfld_->getFromScreen() )
	return false;

    const float scalefac = finalscalefld_->getfValue();
    if ( mIsUdf(scalefac) )
	{ uiMSG().error( "Please enter the scale factor" ); return false; }

    const IOObj* ioobj = wvltfld_->ioobj();
    if ( !ioobj )
	return false;

    IOObj* inpioobj = IOM().get( inpwvltid_ );
    Wavelet* wvlt = Wavelet::get( inpioobj );
    delete inpioobj;
    if ( !wvlt )
    {
	uiMSG().error( "Cannot save scaled wavelet because:\nThe "
		"original wavelet cannot be read." );
	delete ioobj; return false;
    }

    wvlt->transform( 0, scalefac );
    if ( !wvlt->put(ioobj) )
    {
	uiMSG().error( "Cannot write scaled Wavelet.\n"
			"Please check file permissions" );
	delete ioobj; return false;
    }
    delete wvlt;

    outwvltid_ = ioobj->key();
    Wavelet::markScaled( outwvltid_, inpwvltid_, horfld_->key(),
			 seisfld_->ioobj()->key(), evfld_->levelName() );
    return true;
}
