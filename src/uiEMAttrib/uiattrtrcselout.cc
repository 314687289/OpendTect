/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Payraudeau
 Date:          September 2005
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiattrtrcselout.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attriboutput.h"
#include "ctxtioobj.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "multiid.h"
#include "ptrman.h"
#include "seis2ddata.h"
#include "seisselection.h"
#include "seistrctr.h"
#include "survinfo.h"

#include "uiattrsel.h"
#include "uiseissubsel.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uibatchjobdispatchersel.h"
#include "od_helpids.h"


using namespace Attrib;

uiAttrTrcSelOut::uiAttrTrcSelOut( uiParent* p, const DescSet& ad,
				  const NLAModel* n, const MultiID& mid,
				  bool usesinglehor )
    : uiAttrEMOut( p, ad, n, mid, "Create Horizon delimited cube output" )
    , ctio_( mkCtxtIOObjHor(ad.is2D()) )
    , ctio2_( mkCtxtIOObjHor(ad.is2D()) )
    , usesinglehor_(usesinglehor)
    , extraztopfld_(0)
    , extrazbotfld_(0)
    , gatefld_(0)
    , mainhorfld_(0)
    , widthfld_(0)
    , addwidthfld_(0)
    , interpfld_(0)
    , nrsampfld_(0)
    , xparsdlg_(0)
{
    setCtrlStyle( RunAndClose );
    setHelpKey( usesinglehor_
        ? mODHelpKey(mAttrTrcSelOutSliceHelpID)
        : mODHelpKey(mAttrTrcSelOutBetweenHelpID) );

    if ( usesinglehor_ )
	createSingleHorUI();
    else
	createTwoHorUI();

    batchfld_->attach( alignedBelow, outpfld_ );
    objSel(0);
    if ( usesinglehor_ && !ads_.is2D() )
	interpSel(0);

    if ( usesinglehor_ || ads_.is2D() )
	cubeBoundsSel(0);
}


void uiAttrTrcSelOut::createSingleHorUI()
{
    ctio_.ctxt.forread = true;
    objfld_ = new uiIOObjSel( this, ctio_, "Calculate along Horizon" );
    objfld_->attach( alignedBelow, attrfld_ );
    objfld_->selectionDone.notify( mCB(this,uiAttrTrcSelOut,objSel) );

    createSubSelFld( this );
    createZIntervalFld( this );
    createOutsideValFld( this );
    if ( !ads_.is2D() )
    {
	createInterpFld( this );
	createNrSampFld( this );
    }
    createCubeBoundsFlds( this );
    createOutputFld( this );
}


void uiAttrTrcSelOut::createTwoHorUI()
{
    xparsdlg_ = new uiDialog( this, uiDialog::Setup(
				"Set Extra Options",mNoDlgTitle,
				mODHelpKey(mAttrTrcSelOutBetweenHelpID)) );
    xparsdlg_->postFinalise().notify( mCB(this,uiAttrTrcSelOut,extraDlgDone) );

    uiIOObjSel::Setup su( "Calculate between top Horizon" );
    su.filldef(false);
    ctio_.ctxt.forread = true;
    objfld_ = new uiIOObjSel( this, ctio_, su );
    objfld_->attach( alignedBelow, attrfld_ );

    su.seltxt( "and bottom Horizon" );
    ctio2_.ctxt.forread = true;
    obj2fld_ = new uiIOObjSel( this, ctio2_, su );
    obj2fld_->setInput( MultiID("") );
    obj2fld_->attach( alignedBelow, objfld_ );
    obj2fld_->selectionDone.notify( mCB(this,uiAttrTrcSelOut,objSel) );

    createExtraZTopFld( this );
    createExtraZBotFld( this );
    createSubSelFld( this );
    createOutsideValFld( this );
    if ( !ads_.is2D() )
    {
	createInterpFld( xparsdlg_ );
	createNrSampFld( xparsdlg_ );
	createAddWidthFld( xparsdlg_ );
	createWidthFld( xparsdlg_ );
	createMainHorFld( xparsdlg_ );
    }

    createCubeBoundsFlds( ads_.is2D() ? (uiParent*) this
				      : (uiParent*) xparsdlg_ );
    createOutputFld( this );

    if ( !ads_.is2D() )
    {
	CallBack cb = mCB(this,uiAttrTrcSelOut,extraParsCB);
	uiPushButton* extrabut =
			new uiPushButton( this, "Extra options", cb, false );
	extrabut->attach( alignedBelow, outpfld_ );
	batchfld_->attach( alignedBelow, extrabut );
    }
}


uiAttrTrcSelOut::~uiAttrTrcSelOut()
{
    delete ctio_.ioobj;
    delete &ctio_;

    if ( !usesinglehor_ )
    {
	delete ctio2_.ioobj;
	delete &ctio2_;
    }
}


void uiAttrTrcSelOut::createZIntervalFld( uiParent* prnt )
{
    const char* gatelabel = "Z Interval required around Horizons";
    gatefld_ = new uiGenInput( prnt, gatelabel,
			FloatInpIntervalSpec().setName("Z Interval Start",0)
					      .setName("Z Interval Stop",1) );
    gatefld_->attach( alignedBelow, seissubselfld_ );
    uiLabel* lbl = new uiLabel( prnt, SI().getZUnitString() );
    lbl->attach( rightOf, (uiObject*)gatefld_ );
}


void uiAttrTrcSelOut::createExtraZTopFld( uiParent* prnt )
{
    extraztopfld_ = new uiGenInput( prnt, "plus", FloatInpSpec(0) );
    extraztopfld_->setElemSzPol(uiObject::Small);
    extraztopfld_->attach( rightOf, objfld_ );
    uiLabel* toplbl = new uiLabel( prnt, SI().getZUnitString() );
    toplbl->attach( rightOf, extraztopfld_ );
}


void uiAttrTrcSelOut::createExtraZBotFld( uiParent* prnt )
{
    extrazbotfld_ = new uiGenInput( prnt, "plus", FloatInpSpec(0) );
    extrazbotfld_->setElemSzPol(uiObject::Small);
    extrazbotfld_->attach( rightOf, obj2fld_ );
    uiLabel* botlbl = new uiLabel( prnt, SI().getZUnitString() );
    botlbl->attach( rightOf, extrazbotfld_ );
}


void uiAttrTrcSelOut::createSubSelFld( uiParent* prnt )
{
    seissubselfld_ = uiSeisSubSel::get( prnt,
	    Seis::SelSetup(ads_.is2D()).onlyrange(false).multiline(true)
				       .withoutz(true).withstep(false) );
    seissubselfld_->attach( alignedBelow, usesinglehor_ ? (uiGroup*)objfld_
						       : (uiGroup*)obj2fld_ );
    mDynamicCastGet( uiSeis2DSubSel* , seis2dsubsel, seissubselfld_ );
    if ( seis2dsubsel )
	seis2dsubsel->lineSel.notify(mCB(this,uiAttrTrcSelOut,lineSel));
}


void uiAttrTrcSelOut::createOutsideValFld( uiParent* prnt )
{
    const char* outsidevallabel = "Value outside computed area";
    outsidevalfld_ = new uiGenInput( prnt, outsidevallabel, FloatInpSpec() );
    outsidevalfld_->attach( alignedBelow, usesinglehor_ ? (uiGroup*)gatefld_
						   : (uiGroup*)seissubselfld_ );
    outsidevalfld_->setValue(0);
}


void uiAttrTrcSelOut::createInterpFld( uiParent* prnt )
{
    const char* interplbl = "Interpolate Horizons";
    const char* flbl = "Full";
    const char* plbl = "Partial";
    interpfld_ = new uiGenInput( prnt, interplbl, BoolInpSpec(true,flbl,plbl) );
    interpfld_->setWithCheck( true );
    interpfld_->setChecked( true );
    interpfld_->valuechanged.notify( mCB(this,uiAttrTrcSelOut,interpSel) );
    interpfld_->checked.notify( mCB(this,uiAttrTrcSelOut,interpSel) );
    if ( usesinglehor_ )
	interpfld_->attach( alignedBelow, outsidevalfld_ );
}


void uiAttrTrcSelOut::createNrSampFld( uiParent* prnt )
{
    const char* nrsamplabel = "Interpolate if hole is smaller than N traces";
    nrsampfld_ = new uiGenInput( prnt, nrsamplabel,
				 IntInpSpec(0).setName("Interpolate") );
    nrsampfld_->attach( alignedBelow, interpfld_ );
}


void uiAttrTrcSelOut::createAddWidthFld( uiParent* prnt )
{
    BufferString zlabel = createAddWidthLabel();
    addwidthfld_ = new uiGenInput( prnt, zlabel, BoolInpSpec(false) );
    addwidthfld_->attach( alignedBelow, nrsampfld_ );
    addwidthfld_->valuechanged.notify( mCB(this,uiAttrTrcSelOut,extraWidthSel));
}


void uiAttrTrcSelOut::createWidthFld( uiParent* prnt )
{
    widthfld_ = new uiGenInput( prnt,"Extra interval length", FloatInpSpec() );
    widthfld_->attach( alignedBelow, addwidthfld_ );
    widthfld_->checked.notify( mCB(this,uiAttrTrcSelOut,extraWidthSel) );
}


void uiAttrTrcSelOut::createMainHorFld( uiParent* prnt )
{
    const char* mainhorlabel = "Main Horizon";
    mainhorfld_ = new uiGenInput( prnt, mainhorlabel,
				 BoolInpSpec(true,"Top","Bottom") );
    mainhorfld_->attach( alignedBelow, widthfld_ );
}


void uiAttrTrcSelOut::createCubeBoundsFlds( uiParent* prnt )
{
    const char* choicelbl = "Define Z limits for the output cube";
    setcubeboundsfld_ = new uiGenInput ( prnt, choicelbl, BoolInpSpec(true) );
    setcubeboundsfld_->attach( alignedBelow,
			       mainhorfld_ ? mainhorfld_
					   : nrsampfld_ ? nrsampfld_
							: outsidevalfld_ );
    setcubeboundsfld_->setValue( false );
    setcubeboundsfld_->valuechanged.notify(
	    mCB(this,uiAttrTrcSelOut,cubeBoundsSel) );

    cubeboundsfld_ = new uiGenInput ( prnt, "Z Range", FloatInpIntervalSpec()
							.setName("Z Start",0)
							.setName("Z Stop",1) );
    cubeboundsfld_->attach( alignedBelow, setcubeboundsfld_ );
}


void uiAttrTrcSelOut::createOutputFld( uiParent* prnt )
{
    const Seis::GeomType gt = Seis::geomTypeOf( ads_.is2D(), false );
    outpfld_ = new uiSeisSel( prnt, uiSeisSel::ioContext( gt, false ),
			      uiSeisSel::Setup(gt) );
    const bool noadvdlg = usesinglehor_ || ads_.is2D();
    outpfld_->attach( alignedBelow, noadvdlg ? cubeboundsfld_ : outsidevalfld_);
}


bool uiAttrTrcSelOut::prepareProcessing()
{
    if ( !uiAttrEMOut::prepareProcessing() ) return false;

    if ( !objfld_->commitInput() )
    {
	uiMSG().error( "Please select first Horizon" );
	return false;
    }

    if ( !usesinglehor_ && !obj2fld_->commitInput() )
    {
	uiMSG().error( "Please select second Horizon" );
	return false;
    }

    const IOObj* outioobj = outpfld_->ioobj();
    if ( !outioobj )
	return false;

    mDynamicCastGet(uiSeis2DSubSel*,seis2dsubsel,seissubselfld_);
    if ( ads_.is2D() && seis2dsubsel )
    {
	bool lkexists = false;
	Seis2DDataSet dataset( *outioobj );
	if ( seis2dsubsel->isSingLine() )
	    lkexists = dataset.indexOf( seis2dsubsel->selectedLine() ) >= 0;
	else
	    lkexists = dataset.nrLines();

	if ( lkexists )
	{
	    BufferString msg( "Output attribute already exists. Do you\n"
		    "want to continue and overwrite existing attribute?" );
	    if ( !uiMSG().askOverwrite(msg) ) return false;
	}

	EM::SurfaceIOData data;
	BufferString errmsg;
	if ( !EM::EMM().getSurfaceData(ctio_.ioobj->key(),data,errmsg) )
	{
	    uiMSG().error( errmsg.buf() );
	    return false;
	}
    }

    batchfld_->setJobName( outpfld_->ioobj()->name() );
    return true;
}


bool uiAttrTrcSelOut::fillPar()
{
    uiAttrEMOut::fillPar();
    IOPar& iopar = batchfld_->jobSpec().pars_;
    const bool is2d = ads_.is2D();

    const IOObj* outioobj = outpfld_->ioobj( true );
    if ( outioobj )
    {
	BufferString outseisid;
	outseisid += outioobj->key();
	fillOutPar( iopar, Output::tskey(), SeisTrcStorOutput::seisidkey(),
		    outseisid );
    }

    BufferString outnm = outpfld_->getInput();
    iopar.set( sKey::Target(), outnm );

    BufferString tmpkey = IOPar::compKey( LocationOutput::surfidkey(), 0);
    BufferString key = IOPar::compKey( sKey::Geometry(), tmpkey );
    iopar.set( key, ctio_.ioobj->key() );

    if ( !usesinglehor_ )
    {
	tmpkey = IOPar::compKey( LocationOutput::surfidkey(), 1);
	key = IOPar::compKey( sKey::Geometry(), tmpkey );
	iopar.set( key, ctio2_.ioobj->key() );
    }

    PtrMan<IOPar> subselpar = new IOPar;
    seissubselfld_->fillPar( *subselpar );

    HorSampling horsamp; horsamp.usePar( *subselpar );
    if ( horsamp.isEmpty() )
	getComputableSurf( horsamp );

    BufferString typestr;
    subselpar->get( sKey::Type(), typestr );
    const bool issubsel = typestr != sKey::None();
    const bool usesamp = !is2d || issubsel;
    if ( !issubsel && !is2d )
	subselpar->set( sKey::Type(), sKey::Range() );

    if ( usesamp )
    {
	mDynamicCastGet( uiSeis2DSubSel* , seis2dsubsel, seissubselfld_ );
	if ( !is2d || ( seis2dsubsel && seis2dsubsel->isSingLine() ) )
	{
	    key = IOPar::compKey( sKey::Geometry(),
				  SeisTrcStorOutput::inlrangekey() );
	    iopar.set( key, horsamp.start.inl(), horsamp.stop.inl() );

	    key = IOPar::compKey( sKey::Geometry(),
				  SeisTrcStorOutput::crlrangekey() );
	    iopar.set( key, horsamp.start.crl(), horsamp.stop.crl() );
	}
    }

    CubeSampling::removeInfo( *subselpar );
    iopar.mergeComp( *subselpar,
		     IOPar::compKey(sKey::Output(),sKey::Subsel()) );

    Interval<float> zinterval;
    if ( gatefld_ )
	zinterval = gatefld_->getFInterval();
    else
    {
	zinterval.start = extraztopfld_->getfValue();
	zinterval.stop = extrazbotfld_->getfValue();
    }

    key = IOPar::compKey( sKey::Geometry(), "ExtraZInterval" );
    iopar.set( key, zinterval );

    key = IOPar::compKey( sKey::Geometry(), "Outside Value" );
    iopar.set( key, outsidevalfld_->getfValue() );

    int nrsamp = 0;
    if ( interpfld_ && interpfld_->isChecked() )
	nrsamp = interpfld_->getBoolValue() ? mUdf(int)
					   : nrsampfld_->getIntValue();

    key = IOPar::compKey( sKey::Geometry(), "Interpolation Stepout" );
    iopar.set( key, nrsamp );

    if ( !usesinglehor_ && addwidthfld_ && addwidthfld_->getBoolValue() )
    {
	key = IOPar::compKey( sKey::Geometry(), "Artificial Width" );
	iopar.set( key, widthfld_->getfValue() );

	key = IOPar::compKey( sKey::Geometry(), "Leading Horizon" );
	iopar.set( key, mainhorfld_->getBoolValue()? 1 : 2 );
    }

    Interval<float> cubezbounds;
    cubezbounds = setcubeboundsfld_->getBoolValue()
				? cubeboundsfld_->getFInterval()
				: Interval<float>( mUdf(float), mUdf(float) );
    if ( !mIsUdf(cubezbounds.start) )
    {
	key = IOPar::compKey( sKey::Geometry(), "Z Boundaries" );
	iopar.set( key, cubezbounds );
    }

    return true;
}


void uiAttrTrcSelOut::getComputableSurf( HorSampling& horsampling )
{
    EM::SurfaceIOData sd;
    BufferString errmsg;
    if ( !EM::EMM().getSurfaceData(ctio_.ioobj->key(),sd,errmsg) )
	return;

    Interval<int> inlrg(sd.rg.start.inl(), sd.rg.stop.inl());
    Interval<int> crlrg(sd.rg.start.crl(), sd.rg.stop.crl());

    if ( !usesinglehor_ )
    {
	EM::SurfaceIOData sd2;
	if ( !EM::EMM().getSurfaceData(ctio2_.ioobj->key(),sd2,errmsg) )
	    return;

	Interval<int> inlrg2(sd2.rg.start.inl(), sd2.rg.stop.inl());
	Interval<int> crlrg2(sd2.rg.start.crl(), sd2.rg.stop.crl());

	inlrg.start = mMAX( inlrg.start, inlrg2.start);
	inlrg.stop = mMIN( inlrg.stop, inlrg2.stop);
	crlrg.start = mMAX( crlrg.start, crlrg2.start);
	crlrg.stop = mMIN( crlrg.stop, crlrg2.stop);
    }

    horsampling.set( inlrg, crlrg );
}


BufferString uiAttrTrcSelOut::createAddWidthLabel()
{
    BufferString zlabel = "Add fixed interval length to Main Horizon \n";
    BufferString ifinterp = "in case of interpolation conflict";
    BufferString ifnointerp = "in case of holes in second Horizon";
    BufferString text = zlabel;
    text += interpfld_->isChecked()? ifinterp : ifnointerp;
    return text;
}


void uiAttrTrcSelOut::attribSel( CallBacker* )
{
    if ( ads_.is2D() )
    {
	const Desc* desc = ads_.getDesc( attrfld_->attribID() );
	if ( !desc )
	    desc = ads_.getFirstStored();
	if ( desc )
	{
	    LineKey lk( desc->getStoredID(true) );
	    if ( !lk.isEmpty() )
	    {
		PtrMan<IOObj> ioobj = IOM().get( MultiID(lk.lineName()) );
		if ( ioobj )
		    seissubselfld_->setInput( *ioobj );
	    }
	}

	lineSel(0);
    }
}


void uiAttrTrcSelOut::objSel( CallBacker* cb )
{
    if ( !objfld_->commitInput() ||
	 ( !usesinglehor_ && !obj2fld_->commitInput() ) )
	return;

    if ( ads_.is2D() )
    {
	EM::IOObjInfo info( ctio_.ioobj->key() );
	TypeSet<Pos::GeomID> geomids;
	info.getGeomIDs( geomids );
	if ( !usesinglehor_ )
	{
	    EM::IOObjInfo info2( ctio2_.ioobj->key() );
	    TypeSet<Pos::GeomID> geomids2;
	    info2.getGeomIDs( geomids2 );
	    for ( int idx=geomids.size()-1; idx>=0; idx-- )
	    {
		if ( !geomids2.isPresent(geomids[idx]) )
		    geomids.removeSingle( idx );
	    }
	}

	mDynamicCastGet( uiSeis2DSubSel* , seis2dsubsel, seissubselfld_ );
	seis2dsubsel->setInputLines( geomids );
    }

    CubeSampling cs;
    attrfld_->getRanges( cs );

    HorSampling horsampling;
    getComputableSurf( horsampling );

    cs.hrg.limitTo( horsampling );
    seissubselfld_->setInput( cs );
}


void uiAttrTrcSelOut::interpSel( CallBacker* cb )
{
    nrsampfld_->display( interpfld_->isChecked() ? !interpfld_->getBoolValue()
					       : false );
    if ( !addwidthfld_ )
	return;

    BufferString text = createAddWidthLabel();
    addwidthfld_->setTitleText( text );
}


void uiAttrTrcSelOut::extraWidthSel( CallBacker* cb )
{
    if ( !addwidthfld_ )
	return;

    widthfld_->display( addwidthfld_->getBoolValue(), false );
    mainhorfld_->display( addwidthfld_->getBoolValue(), false );
}


void uiAttrTrcSelOut::cubeBoundsSel( CallBacker* cb )
{
    cubeboundsfld_->display( setcubeboundsfld_->getBoolValue() );
}


void uiAttrTrcSelOut::extraParsCB( CallBacker* cb )
{
    xparsdlg_->go();
}


void uiAttrTrcSelOut::extraDlgDone( CallBacker* cb )
{
    if ( !ads_.is2D() )
    {
	interpSel(0);
	extraWidthSel(0);
    }

    cubeBoundsSel(0);
}


CtxtIOObj& uiAttrTrcSelOut::mkCtxtIOObjHor( bool is2d )
{
    return is2d ? *mMkCtxtIOObj( EMHorizon2D ) : *mMkCtxtIOObj( EMHorizon3D );
}


void uiAttrTrcSelOut::lineSel( CallBacker* )
{
    if ( !ads_.is2D() ) return;

    batchfld_->jobSpecUpdated();
}
