/*+
   * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
   * AUTHOR   : Nageswara
   * DATE     : Mar 2008
 -*/

static const char* rcsID mUsedVar = "$Id$";

#include "uistratamp.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "attriboutput.h"
#include "trckeyzsampling.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "stratamp.h"
#include "survinfo.h"

#include "uiattrsel.h"
#include "uibatchjobdispatchersel.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiioobjsel.h"
#include "uipossubsel.h"
#include "uistrings.h"
#include "od_helpids.h"


static const char* statstrs[] = { "Min", "Max", "Average", "RMS", "Sum", 0 };

uiStratAmpCalc::uiStratAmpCalc( uiParent* p )
    : uiDialog( p, Setup(tr("StratalAmplitude"),mNoDlgTitle,
			 mODHelpKey(mStratAmpCalcHelpID)))
    , horctio1_(*mMkCtxtIOObj(EMHorizon3D))
    , horctio2_(*mMkCtxtIOObj(EMHorizon3D))
    , isoverwrite_(false)
{
    const Attrib::DescSet* ads = Attrib::DSHolder().getDescSet(false,false);
    inpfld_ = new uiAttrSel( this, *ads, "Quantity to output" );
    inpfld_->selectionDone.notify( mCB(this,uiStratAmpCalc,inpSel) );

    winoption_= new uiGenInput( this, tr("Window Option"),
	                        BoolInpSpec(true, tr("Single Horizon"),
				tr("Double Horizon")) );
    winoption_->valuechanged.notify( mCB(this,uiStratAmpCalc,choiceSel) );
    winoption_->attach( alignedBelow, inpfld_ );

    horfld1_ = new uiIOObjSel( this, horctio1_, uiStrings::sHorizon() );
    horfld1_->selectionDone.notify( mCB(this,uiStratAmpCalc,inpSel) );
    horfld1_->attach( alignedBelow, winoption_ );

    horfld2_ = new uiIOObjSel( this, horctio2_, uiStrings::sBottomHor() );
    horfld2_->selectionDone.notify( mCB(this,uiStratAmpCalc,inpSel) );
    horfld2_->attach( alignedBelow, horfld1_ );

    BufferString lbltxt = "Z Offset ";
    lbltxt += SI().getZUnitString(); lbltxt += " Top";
    tophorshiftfld_ = new uiGenInput( this, lbltxt,
	    			      FloatInpSpec(0).setName("Top") );
    tophorshiftfld_->attach( alignedBelow, horfld2_ );
    tophorshiftfld_->setElemSzPol( uiObject::Small );
    bothorshiftfld_ = new uiGenInput( this, uiStrings::sBottom(), 
                                      FloatInpSpec(0) );
    bothorshiftfld_->attach( rightTo, tophorshiftfld_ );
    bothorshiftfld_->setElemSzPol( uiObject::Small );

    rangefld_= new uiPosSubSel( this, uiPosSubSel::Setup(false,false) );
    rangefld_->attach( alignedBelow, tophorshiftfld_ );

    ampoptionfld_ = new uiLabeledComboBox( this, statstrs,
	    				   tr("Amplitude Option") );
    ampoptionfld_->attach( alignedBelow, rangefld_ );

    selfld_= new uiGenInput( this, tr("Add result as an attribute to"),
			     BoolInpSpec(true,
                             uiStrings::sTopHor(),uiStrings::sBottomHor()) );
    selfld_->valuechanged.notify( mCB(this,uiStratAmpCalc,setParFileNameCB) );
    selfld_->attach( alignedBelow, ampoptionfld_ );

    foldfld_ = new uiGenInput( this, tr("Output fold as an extra attribute"),
	    		       BoolInpSpec(false) ) ;
    foldfld_->attach( alignedBelow, selfld_ );

    attribnamefld_ = new uiGenInput( this, uiStrings::sAttribName(),
			             StringInpSpec("Stratal Amplitude") );
    attribnamefld_->valuechanged.notify(
	    			mCB(this,uiStratAmpCalc,setParFileNameCB) );
    attribnamefld_->attach( alignedBelow, foldfld_ );

    batchfld_ = new uiBatchJobDispatcherSel( this, false,
					     Batch::JobSpec::NonODBase );
    batchfld_->attach( alignedBelow, attribnamefld_ );
    batchfld_->jobSpec().prognm_ = "od_stratamp";
    setParFileName();

    postFinalise().notify( mCB(this,uiStratAmpCalc,choiceSel) );
}


uiStratAmpCalc::~uiStratAmpCalc()
{
    delete horctio1_.ioobj; delete &horctio1_;
    delete horctio2_.ioobj; delete &horctio2_;
}


void uiStratAmpCalc::choiceSel( CallBacker* )
{
    usesingle_ = winoption_->getBoolValue();
    horfld1_->setLabelText( usesingle_ ? uiStrings::sHorizon()
				       : uiStrings::sTopHor() );
    horfld2_->display( !usesingle_ );
    selfld_->display( !usesingle_ );
}


void uiStratAmpCalc::inpSel( CallBacker* )
{
    TrcKeySampling hs;
    getAvailableRange( hs );
    TrcKeyZSampling incs( rangefld_->envelope() );
    incs.hsamp_ = hs;
    rangefld_->setInput( incs );
    setParFileName();
}


void uiStratAmpCalc::setParFileName()
{
    BufferString jobnm;
    const bool addtotop = usesingle_ || selfld_->getBoolValue();
    addtotop ? jobnm.add( horfld1_->getInput() )
	     : jobnm.add( horfld2_->getInput() );
    jobnm.add( "_").add( attribnamefld_->text() );
    batchfld_->setJobName( jobnm );
}


void uiStratAmpCalc::setParFileNameCB( CallBacker* )
{
    setParFileName();
}


void uiStratAmpCalc::getAvailableRange( TrcKeySampling& hs )
{
    TrcKeyZSampling cs;
    if ( inpfld_->getRanges(cs) )
	hs.limitTo( cs.hsamp_ );

    if ( horfld1_->commitInput() )
    {
	EM::IOObjInfo eminfo( horctio1_.ioobj->key() );
	TrcKeySampling emhs;
	emhs.set( eminfo.getInlRange(), eminfo.getCrlRange() );
	hs.limitTo( emhs );
    }

    if ( horfld2_->commitInput() )
    {
	EM::IOObjInfo eminfo( horctio2_.ioobj->key() );
	TrcKeySampling emhs;
	emhs.set( eminfo.getInlRange(), eminfo.getCrlRange() );
	hs.limitTo( emhs );
    }
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiStratAmpCalc::checkInpFlds()
{
    if ( inpfld_->isEmpty() )
mErrRet( tr("Missing Input\nPlease select the input attribute / seismics"));
    
    if ( usesingle_ && !horfld1_->commitInput() )
	mErrRet( tr("Missing Input\nPlease select the input Horizon") );

    if ( !usesingle_ )
    {
	if ( !horfld1_->commitInput() || !horfld2_->commitInput() )
	    mErrRet( tr("Missing Input\nPlease Check Top / Bottom Horizon") );
    }

    if ( !usesingle_ && horctio1_.ioobj->key() == horctio2_.ioobj->key() )
	      mErrRet( tr("Select Two Different Horizons") );

    return true;
}


bool uiStratAmpCalc::prepareProcessing()
{
    if ( !checkInpFlds() ) return false;

    const bool addtotop = usesingle_ || selfld_->getBoolValue();
    EM::IOObjInfo eminfo( addtotop ? horctio1_.ioobj->key()
	    			   : horctio2_.ioobj->key() );
    BufferStringSet attrnms;
    eminfo.getAttribNames( attrnms );
    const char* attribnm = attribnamefld_->text();
    isoverwrite_ = false;
    if ( attrnms.isPresent( attribnm ) )
    {
	uiString errmsg = tr("Attribute name %1 already exists, Overwrite?")
			.arg(attribnm);
	if ( !uiMSG().askOverwrite(errmsg) )
	    return false;
	else
	    isoverwrite_ = true;
    }

    return true;
}


bool uiStratAmpCalc::fillPar()
{
    IOPar& iop = batchfld_->jobSpec().pars_;
    iop.setYN( StratAmpCalc::sKeySingleHorizonYN(), usesingle_ );
    iop.set( StratAmpCalc::sKeyTopHorizonID() , horctio1_.ioobj->key() );
    if ( !usesingle_ )
	iop.set( StratAmpCalc::sKeyBottomHorizonID(), horctio2_.ioobj->key() );

    const bool addtotop = usesingle_ || selfld_->getBoolValue();
    iop.setYN( StratAmpCalc::sKeyAddToTopYN(), addtotop );
    iop.set( StratAmpCalc::sKeyAmplitudeOption(), ampoptionfld_->box()->text());
    iop.setYN( StratAmpCalc::sKeyOutputFoldYN(), foldfld_->getBoolValue() );
    iop.set( StratAmpCalc::sKeyTopShift(),
	     tophorshiftfld_->getFValue() / SI().zDomain().userFactor() );
    iop.set( StratAmpCalc::sKeyBottomShift(),
	     bothorshiftfld_->getFValue() / SI().zDomain().userFactor() );
    iop.set( StratAmpCalc::sKeyAttribName(), attribnamefld_->text() );
    iop.setYN( StratAmpCalc::sKeyIsOverwriteYN(), isoverwrite_ );

    TrcKeySampling hs;
    getAvailableRange( hs );
    TrcKeySampling inhs = rangefld_->envelope().hsamp_;
    hs.limitTo( inhs );
    IOPar subselpar;
    hs.fillPar( subselpar );
    subselpar.set( sKey::ZRange(), SI().zRange(false) );
    iop.mergeComp( subselpar, IOPar::compKey(sKey::Output(),sKey::Subsel()) );

    const Attrib::DescID targetid = inpfld_->attribID();
    Attrib::DescSet* clonedset = Attrib::DSHolder().getDescSet(
			    inpfld_->is2D(),false)->optimizeClone( targetid );
    IOPar attrpar( "Attribute Descriptions" );
    if ( !clonedset ) return false;

    clonedset->fillPar( attrpar );
    for ( int idx=0; idx<attrpar.size(); idx++ )
    {
	const char* nm = attrpar.getKey( idx );
	iop.add( IOPar::compKey(Attrib::SeisTrcStorOutput::attribkey(),nm),
		   attrpar .getValue(idx) );
    }

    Attrib::Desc* desc = clonedset->getDesc( targetid );
    BufferString storedid = desc ? desc->getStoredID() : "";
    if ( !storedid.isEmpty() )
	iop.set( "Input Line Set", storedid.buf() );

    const BufferString keybase = IOPar::compKey( Attrib::Output::outputstr(),0);
    const BufferString attribkey =
	    IOPar::compKey( keybase, Attrib::SeisTrcStorOutput::attribkey() );
    iop.set( IOPar::compKey(attribkey,Attrib::DescSet::highestIDStr()), 1 );
    iop.set( IOPar::compKey(attribkey,0), targetid.asInt() );
    iop.set( IOPar::compKey(sKey::Output(), sKey::Type()), sKey::Cube() );
    return true;
}


bool uiStratAmpCalc::acceptOK( CallBacker* )
{
    return prepareProcessing() && fillPar() && batchfld_->start();
}
