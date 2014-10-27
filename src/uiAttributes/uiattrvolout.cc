/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiattrvolout.h"

#include "uiattrsel.h"
#include "uibatchjobdispatchersel.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uimultoutsel.h"
#include "uiseisioobjinfo.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseistransf.h"
#include "uiseparator.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attriboutput.h"
#include "attribparam.h"
#include "attribsel.h"
#include "attribstorprovider.h"
#include "ctxtioobj.h"
#include "trckeyzsampling.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "nladesign.h"
#include "nlamodel.h"
#include "ptrman.h"
#include "scaler.h"
#include "seisioobjinfo.h"
#include "seismulticubeps.h"
#include "seisselection.h"
#include "seistrc.h"
#include "seis2dline.h"
#include "survinfo.h"
#include "od_helpids.h"

const char* uiAttrVolOut::sKeyMaxCrlRg()  { return "Maximum Crossline Range"; }
const char* uiAttrVolOut::sKeyMaxInlRg()  { return "Maximum Inline Range"; }


uiAttrVolOut::uiAttrVolOut( uiParent* p, const Attrib::DescSet& ad,
			    bool multioutput,
			    const NLAModel* n, const MultiID& id )
    : uiDialog(p,Setup(uiStrings::sEmptyString(),mNoDlgTitle,mNoHelpKey))
    , subselpar_(*new IOPar)
    , sel_(*new Attrib::CurrentSel)
    , ads_(*new Attrib::DescSet(ad))
    , nlamodel_(n)
    , nlaid_(id)
    , todofld_(0)
    , attrselfld_(0)
    , datastorefld_(0)
{
    const bool is2d = ad.is2D();
    const Seis::GeomType gt = Seis::geomTypeOf( is2d, false );

    setCaption( is2d ? tr("Create Data Attribute") :
       ( multioutput ? tr("Create Multi-attribute Output")
		     : tr("Create Volume Attribute")) );

    setHelpKey( is2d ? mODHelpKey(mAttrVolOut2DHelpID)
		     : mODHelpKey(mAttrVolOutHelpID) );

    uiAttrSelData attrdata( ads_, false );
    attrdata.nlamodel_ = nlamodel_;

    if ( !multioutput )
    {
	todofld_ = new uiAttrSel( this, "Quantity to output", attrdata );
	todofld_->selectionDone.notify( mCB(this,uiAttrVolOut,attrSel) );
    }
    else
	attrselfld_ = new uiMultiAttribSel( this, attrdata.attrSet() );

    transffld_ = new uiSeisTransfer( this, uiSeisTransfer::Setup(is2d,false)
			.fornewentry(true).withstep(!is2d).multiline(true) );
    if ( todofld_ )
	transffld_->attach( alignedBelow, todofld_ );
    else
	transffld_->attach( centeredBelow, attrselfld_ );

    objfld_ = new uiSeisSel( this, uiSeisSel::ioContext(gt,false),
				uiSeisSel::Setup(is2d,false) );
    objfld_->selectionDone.notify( mCB(this,uiAttrVolOut,outSelCB) );
    objfld_->attach( alignedBelow, transffld_ );
    objfld_->setConfirmOverwrite( !is2d );

    uiGroup* botgrp = objfld_;
    uiSeparator* sep2 = 0;
    if ( multioutput && !is2d )
    {
	uiSeparator* sep1 = new uiSeparator( this, "PS Start Separator" );
	sep1->attach( stretchedBelow, objfld_ );

	uiCheckBox* cb = new uiCheckBox( this, "Enable Prestack Analysis" );
	cb->activated.notify( mCB(this,uiAttrVolOut,psSelCB) );
	cb->attach( alignedBelow, objfld_ );
	cb->attach( ensureBelow, sep1 );

	IOObjContext ctxt( mIOObjContext(SeisPS3D) );
	ctxt.forread = false;
	ctxt.fixTranslator( "MultiCube" );
	datastorefld_ = new uiIOObjSel( this, ctxt,
					"Output Prestack DataStore" );
	datastorefld_->attach( alignedBelow, cb );

	const Interval<float> offsets( 0, 100 );
	const uiString lbl = tr( "Offset (start/step) %1" )
					.arg( SI().getXYUnitString() );
	offsetfld_ = new uiGenInput( this, lbl,
				     FloatInpIntervalSpec(offsets) );
	offsetfld_->attach( alignedBelow, datastorefld_ );

	sep2 = new uiSeparator( this, "PS End Separator" );
	sep2->attach( stretchedBelow, offsetfld_ );

	psSelCB( cb );
	botgrp = offsetfld_;
    }

    batchfld_ = new uiBatchJobDispatcherSel( this, false,
					     Batch::JobSpec::Attrib );
    IOPar& iop = jobSpec().pars_;
    iop.set( IOPar::compKey(sKey::Output(),sKey::Type()), "Cube" );
    batchfld_->attach( alignedBelow, botgrp );
    if ( sep2 ) batchfld_->attach( ensureBelow, sep2 );
}


uiAttrVolOut::~uiAttrVolOut()
{
    delete &sel_;
    delete &subselpar_;
    delete &ads_;
}


Batch::JobSpec& uiAttrVolOut::jobSpec()
{
    return batchfld_->jobSpec();
}


void uiAttrVolOut::psSelCB( CallBacker* cb )
{
    mDynamicCastGet(uiCheckBox*,box,cb)
    if ( !box || !datastorefld_ ) return;

    datastorefld_->setSensitive( box->isChecked() );
    offsetfld_->setSensitive( box->isChecked() );
}


#define mSetObjFld(s) \
{ \
    objfld_->setInputText( s ); \
    objfld_->processInput(); \
    outSelCB(0); \
}

void uiAttrVolOut::attrSel( CallBacker* )
{
    TrcKeyZSampling cs;
    const bool is2d = todofld_->is2D();
    if ( todofld_->getRanges(cs) )
	transffld_->selfld->setInput( cs );

    Attrib::Desc* desc = ads_.getDesc( todofld_->attribID() );
    if ( !desc )
    {
	mSetObjFld("")
	if ( is2d )	//it could be 2D neural network
	{
	    Attrib::Desc* firststoreddsc = ads_.getFirstStored();
	    if ( firststoreddsc )
	    {
		const LineKey lk( firststoreddsc->getValParam(
			Attrib::StorageProvider::keyStr())->getStringValue(0) );
		BufferString linenm = lk.lineName();
		if ( !linenm.isEmpty() && *linenm.buf() != '#' )
		    mSetObjFld( LineKey(IOM().nameOf( linenm.buf() ),
				todofld_->getInput()) )

		PtrMan<IOObj> ioobj =
			IOM().get( MultiID(firststoreddsc->getStoredID(true)) );
		if ( ioobj )
		    transffld_->setInput( *ioobj );
	    }
	}
    }
    else
    {
	mSetObjFld( desc->isStored() ? "" : todofld_->getInput() )
	if ( is2d )
	{
	    uiString errmsg;
	    RefMan<Attrib::Provider> prov =
		    Attrib::Provider::create( *desc, errmsg );
	    PtrMan<IOObj> ioobj = 0;
	    if ( prov )
	    {
		MultiID mid( desc->getStoredID(true).buf() );
		ioobj = IOM().get( mid );
	    }

	    if ( ioobj )
		transffld_->setInput( *ioobj );
	}
    }
}


void uiAttrVolOut::outSelCB( CallBacker* )
{
}


bool uiAttrVolOut::prepareProcessing()
{
    const IOObj* outioobj = objfld_->ioobj();
    if ( !outioobj )
	return false;

    if ( todofld_ )
    {
	if ( !todofld_->checkOutput(*outioobj) )
	    return false;

	if ( todofld_->is2D() )
	{
	    BufferString outputnm = objfld_->getInput();
	    const int nroccuer = outputnm.count( '|' );
	    outputnm.replace( '|', '_' );
	    if( nroccuer )
	    {
		uiString msg = tr("Invalid charactor '|' "
				  " found in output name. "
				  "It will be renamed to: '%1'"
				  "\nDo you want to continue?")
			     .arg(outputnm.buf());
		    return false;
	    }

	    if ( outputnm.isEmpty() )
	    {
		uiMSG().error(
		tr("No dataset name given. Please provide a valid name. ") );
		return false;
	    }

	    SeisIOObjInfo info( outioobj );
	    BufferStringSet lnms;
	    info.getLineNames( lnms );
	    const bool singline = transffld_->selFld2D()->isSingLine();
	    const char* lnm =
		singline ? transffld_->selFld2D()->selectedLine() : 0;
	    if ( (!singline && lnms.size()) ||
		 (singline && lnms.isPresent(lnm)) )
	    {
		const bool rv = uiMSG().askGoOn(
		    tr("Output attribute already exists."),
		       uiStrings::sOverwrite(), uiStrings::sCancel());
		if ( !rv ) return false;
	    }
	}

	sel_.ioobjkey_ = outioobj->key();
	sel_.attrid_ = todofld_->attribID();
	sel_.outputnr_ = todofld_->outputNr();
	if ( sel_.outputnr_ < 0 && !sel_.attrid_.isValid() )
	{
	    uiMSG().error( tr("Please select the output quantity") );
	    return false;
	}

	if ( todofld_->is3D() )
	{
	    IOObj* chioobj = outioobj->clone();
	    chioobj->pars().set( sKey::Type(), sKey::Attribute() );
	    IOM().commitChanges( *chioobj );
	    delete chioobj;
	}

	Attrib::Desc* seldesc = ads_.getDesc( todofld_->attribID() );
	if ( seldesc )
	{
	    uiMultOutSel multoutdlg( this, *seldesc );
	    if ( multoutdlg.doDisp() )
	    {
		if ( multoutdlg.go() )
		{
		    seloutputs_.erase();
		    multoutdlg.getSelectedOutputs( seloutputs_ );
		    multoutdlg.getSelectedOutNames( seloutnms_ );
		}
		else
		    return false;
	    }
	}
    }

    if ( datastorefld_ && datastorefld_->sensitive() )
    {
	if ( !datastorefld_->ioobj() )
	    return false;

	TypeSet<Attrib::DescID> ids;
	attrselfld_->getSelIds( ids );
	ObjectSet<MultiID> mids; TypeSet<float> offs; TypeSet<int> comps;
	for ( int idx=0; idx<ids.size(); idx++ )
	{
	    mids += new MultiID( outioobj->key() );
	    offs += offsetfld_->getfValue(0) + idx*offsetfld_->getfValue(1);
	    comps += idx;
	}

	BufferString errmsg;
	const bool res = MultiCubeSeisPSReader::writeData(
		datastorefld_->ioobj()->fullUserExpr(false),
		mids, offs, comps, errmsg );
	deepErase( mids );
	if ( !res )
	{
	    uiMSG().error( errmsg );
	    return false;
	}
    }

    uiSeisIOObjInfo ioobjinfo( *outioobj, true );
    SeisIOObjInfo::SpaceInfo spi( transffld_->spaceInfo() );
    subselpar_.setEmpty();
    transffld_->selfld->fillPar( subselpar_ );
    subselpar_.set( "Estimated MBs", ioobjinfo.expectedMBs(spi) );
    return ioobjinfo.checkSpaceLeft(spi);
}


Attrib::DescSet* uiAttrVolOut::getFromToDoFld(
		TypeSet<Attrib::DescID>& outdescids, int& nrseloutputs )
{
    Attrib::DescID nlamodel_id(-1, false);
    if ( nlamodel_ && todofld_ && todofld_->outputNr() >= 0 )
    {
	if ( !nlaid_ || !(*nlaid_) )
	{
	    uiMSG().message(tr("NN needs to be stored before creating volume"));
	    return 0;
	}
	addNLA( nlamodel_id );
    }

    Attrib::DescID targetid = nlamodel_id.isValid() ? nlamodel_id
					    : todofld_->attribID();

    Attrib::Desc* seldesc = ads_.getDesc( targetid );
    if ( seldesc )
    {
	const bool is2d = todofld_->is2D();
	Attrib::DescID multoiid = seldesc->getMultiOutputInputID();
	if ( multoiid != Attrib::DescID::undef() )
	{
	    uiAttrSelData attrdata( ads_ );
	    Attrib::SelInfo attrinf( &attrdata.attrSet(), attrdata.nlamodel_,
				is2d, Attrib::DescID::undef(), false, false );
	    TypeSet<Attrib::SelSpec> targetspecs;
	    if ( !uiMultOutSel::handleMultiCompChain( targetid, multoiid,
				is2d, attrinf, &ads_, this, targetspecs ))
		return 0;
	    for ( int idx=0; idx<targetspecs.size(); idx++ )
		outdescids += targetspecs[idx].id();
	}
    }
    const int outdescidsz = outdescids.size();
    Attrib::DescSet* ret = outdescidsz ? ads_.optimizeClone( outdescids )
			    : ads_.optimizeClone( targetid );
    if ( !ret )
	return 0;

    nrseloutputs = seloutputs_.size() ? seloutputs_.size()
				      : outdescidsz ? outdescidsz : 1;
    if ( !seloutputs_.isEmpty() )
	//TODO make use of the multiple targetspecs (prestack for inst)
	ret->createAndAddMultOutDescs( targetid, seloutputs_,
					     seloutnms_, outdescids );
    else if ( outdescids.isEmpty() )
	outdescids += targetid;

    return ret;
}


bool uiAttrVolOut::fillPar()
{
    const IOObj* outioobj = objfld_->ioobj();
    if ( !outioobj )
	return false;

    IOPar& iop = jobSpec().pars_;

    Attrib::DescSet* clonedset = 0;
    TypeSet<Attrib::DescID> outdescids;
    int nrseloutputs = 1;

    if ( todofld_ )
	clonedset = getFromToDoFld( outdescids, nrseloutputs );
    else
    {
	attrselfld_->getSelIds( outdescids );
	nrseloutputs = outdescids.size();
	clonedset = ads_.optimizeClone( outdescids );
    }
    if ( !clonedset )
	return false;

    IOPar attrpar( "Attribute Descriptions" );
    clonedset->fillPar( attrpar );
    for ( int idx=0; idx<attrpar.size(); idx++ )
    {
        const char* nm = attrpar.getKey(idx);
        iop.add( IOPar::compKey(Attrib::SeisTrcStorOutput::attribkey(),nm),
		 attrpar.getValue(idx) );
    }

    const BufferString keybase = IOPar::compKey(Attrib::Output::outputstr(),0);
    const BufferString attribkey =
	IOPar::compKey( keybase, Attrib::SeisTrcStorOutput::attribkey() );

    iop.set( IOPar::compKey(attribkey,Attrib::DescSet::highestIDStr()),
		nrseloutputs );
    if ( nrseloutputs != outdescids.size() ) return false;

    for ( int idx=0; idx<nrseloutputs; idx++ )
	iop.set( IOPar::compKey(attribkey,idx), outdescids[idx].asInt() );

    const bool is2d = todofld_ ? todofld_->is2D() : attrselfld_->is2D();
    BufferString outseisid;
    outseisid += outioobj->key();

    iop.set( IOPar::compKey(keybase,Attrib::SeisTrcStorOutput::seisidkey()),
			    outseisid);

    iop.setYN( IOPar::compKey(keybase,SeisTrc::sKeyExtTrcToSI()),
	       transffld_->extendTrcsToSI() );
    iop.mergeComp( subselpar_, IOPar::compKey(sKey::Output(),sKey::Subsel()) );
    Scaler* sc = transffld_->getScaler();
    if ( sc )
    {
	iop.set( IOPar::compKey(keybase,Attrib::Output::scalekey()),
				sc->toString() );
	delete sc;
    }

    BufferString attrnm = todofld_ ? todofld_->getAttrName() : "Multi-attribs";
    iop.set( sKey::Target(), attrnm.buf() );
    BufferString linename;
    if ( is2d )
    {
	MultiID ky;
	Attrib::DescSet descset(true);
	if ( nlamodel_ )
	    descset.usePar( nlamodel_->pars() );

	Attrib::Desc* desc = nlamodel_ ? descset.getFirstStored()
			      : ads_.getDesc( todofld_->attribID() );
	if ( desc )
	{
	    BufferString storedid = desc->getStoredID();
	    if ( !storedid.isEmpty() )
	    {
		LineKey lk( storedid.buf() );
		iop.set( "Input Line Set", lk.lineName() );
		linename = lk.lineName();
	    }
	}
    }

    delete clonedset;
    return true;
}


void uiAttrVolOut::addNLA( Attrib::DescID& id )
{
    BufferString defstr("NN specification=");
    defstr += nlaid_;

    uiString errmsg;
    Attrib::EngineMan::addNLADesc( defstr, id, ads_, todofld_->outputNr(),
			   nlamodel_, errmsg );

    if ( !errmsg.isEmpty() )
        uiMSG().error( errmsg );
}


bool uiAttrVolOut::acceptOK( CallBacker* )
{
    if ( !prepareProcessing() || !fillPar() )
	return false;

    batchfld_->setJobName( objfld_->getInput() );
    return batchfld_->start();
}
