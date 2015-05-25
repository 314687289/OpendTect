/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2008
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiisopachmaker.h"

#include "datapointset.h"
#include "emmanager.h"
#include "emhorizon3d.h"
#include "emioobjinfo.h"
#include "emsurfaceauxdata.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "iopar.h"
#include "isopachmaker.h"
#include "multiid.h"
#include "posvecdataset.h"
#include "survinfo.h"

#include "uibatchjobdispatchersel.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

uiIsopachMakerGrp::uiIsopachMakerGrp( uiParent* p, EM::ObjectID horid )
        : uiGroup(p,"Create isopach")
	, ctio_(*mMkCtxtIOObj(EMHorizon3D))
	, basectio_(*mMkCtxtIOObj(EMHorizon3D))
	, baseemobj_(0)
	, horid_(horid)
	, basesel_(0)
	, msecsfld_(0)
{
    baseemobj_ = EM::EMM().getObject( horid_ );
    if ( !baseemobj_ )
    {
	basectio_.ctxt.forread = true;
	basesel_ = new uiIOObjSel( this, basectio_, uiStrings::sHorizon(true) );
    }

    ctio_.ctxt.forread = true;
    horsel_ = new uiIOObjSel( this, ctio_, "Calculate to" );
    horsel_->selectionDone.notify( mCB(this,uiIsopachMakerGrp,toHorSel) );
    if ( !baseemobj_ )
    {
	horsel_->setInput( MultiID("") );
	horsel_->attach( alignedBelow, basesel_ );
    }

    attrnmfld_ = new uiGenInput( this, uiStrings::sAttribName(), 
				 StringInpSpec() );
    attrnmfld_->attach( alignedBelow, horsel_ );
    toHorSel(0);

    if ( SI().zIsTime() )
    {
	msecsfld_ = new uiGenInput( this, tr("Output in"),
				BoolInpSpec(true,tr("Milliseconds"),
                                tr("Seconds")) );
	msecsfld_->attach( alignedBelow, attrnmfld_ );
    }

    setHAlignObj( basesel_ ? basesel_ : horsel_ );
}


BufferString uiIsopachMakerGrp::getHorNm( EM::ObjectID horid )
{
    MultiID mid( EM::EMM().getMultiID( horid ) );
    return EM::EMM().objectName( mid );
}


uiIsopachMakerGrp::~uiIsopachMakerGrp()
{
    delete ctio_.ioobj; delete &ctio_;
    delete basectio_.ioobj; delete &basectio_;
}


void uiIsopachMakerGrp::toHorSel( CallBacker* )
{
    if ( horsel_->ioobj(true) )
	attrnmfld_->setText( BufferString("I: ",horsel_->ioobj()->name()) );
}


bool uiIsopachMakerGrp::chkInputFlds()
{
    if ( basesel_ && !basesel_->commitInput() )
	return false;

    horsel_->commitInput();
    if ( !horsel_->ioobj() ) return false;

    if ( !EM::canOverwrite(horsel_->ioobj()->key()) )
    {
	uiMSG().message( tr("Cannot save attribute on the selected horizon"
			    "\n.Please select a different horizon") );
	return false;
    }

    BufferString attrnm =  attrnmfld_->text();
    if ( attrnm.isEmpty() )
    {
	uiMSG().message( tr("Please enter attribute name") );
	attrnm.add( "I: " ).add( horsel_->ioobj()->name() );
	attrnmfld_->setText( attrnm );
	return false;
    }

    return true;
}


bool uiIsopachMakerGrp::fillPar( IOPar& par )
{
    par.set( IsopachMaker::sKeyHorizonID(), basesel_ ? basesel_->ioobj()->key()
	   					     : baseemobj_->multiID() );
    par.set( IsopachMaker::sKeyCalculateToHorID(), horsel_->ioobj()->key() );
    par.set( IsopachMaker::sKeyAttribName(), attrnmfld_->text() );
    if ( msecsfld_ )
	par.setYN( IsopachMaker::sKeyOutputInMilliSecYN(),
		   msecsfld_->getBoolValue() );

    return true;
}


const char* uiIsopachMakerGrp::attrName() const
{ return attrnmfld_->text(); }


#define mErrRet(s) { uiMSG().error(s); return false; }

//uiIsopachMakerBatch
uiIsopachMakerBatch::uiIsopachMakerBatch( uiParent* p )
    : uiDialog( p,Setup(tr("Create isopach"),mNoDlgTitle,mTODOHelpKey) )
{
    grp_ = new uiIsopachMakerGrp( this, -1 );
    batchfld_ = new uiBatchJobDispatcherSel( this, false,
					     Batch::JobSpec::NonODBase );
    batchfld_->attach( alignedBelow, grp_ );
    batchfld_->jobSpec().prognm_ = "od_isopach";
}


bool uiIsopachMakerBatch::prepareProcessing()
{
    return grp_->chkInputFlds();
}


bool uiIsopachMakerBatch::fillPar()
{
    IOPar& par = batchfld_->jobSpec().pars_;
    if ( !grp_->fillPar( par ) )
	return false;

    MultiID mid;
    par.get( IsopachMaker::sKeyHorizonID(), mid );
    EM::IOObjInfo eminfo( mid );
    BufferStringSet attrnms;
    eminfo.getAttribNames( attrnms );
    BufferString attrnm;
    par.get( IsopachMaker::sKeyAttribName(), attrnm );
    isoverwrite_ = false;
    if ( attrnms.isPresent( attrnm ) )
    {
	uiString errmsg = tr("Attribute name %1 already exists, Overwrite?")
			.arg(attrnm);
	if ( !uiMSG().askOverwrite(errmsg) )
	    return false;

	isoverwrite_ = true;
    }

    par.setYN( IsopachMaker::sKeyIsOverWriteYN(), isoverwrite_ );
    return true;
}


bool uiIsopachMakerBatch::acceptOK( CallBacker* )
{
    if ( !prepareProcessing() || !fillPar() )
	return false;

    batchfld_->setJobName( grp_->attrName() );
    return batchfld_->start();
}


//uiIsopachMakerDlg
uiIsopachMakerDlg::uiIsopachMakerDlg( uiParent* p, EM::ObjectID emid )
    : uiDialog(p,Setup(tr("Create isopach"),mNoDlgTitle,
                        mODHelpKey(mIsopachMakerHelpID) ))
    , dps_( new DataPointSet(false,true) )
{
    grp_ = new uiIsopachMakerGrp( this, emid );
    BufferString title( "Create isopach" );
    title += " for '";
    title += grp_->getHorNm( emid );
    title += "'";
    setTitleText( title );
}


uiIsopachMakerDlg::~uiIsopachMakerDlg()
{
    delete dps_;
}


bool uiIsopachMakerDlg::acceptOK( CallBacker* )
{
    if ( !grp_->chkInputFlds() )
	return false;

    return doWork();
}


bool uiIsopachMakerDlg::doWork()
{
    IOPar par;
    grp_->fillPar(par);
    MultiID mid1, mid2;
    par.get( IsopachMaker::sKeyHorizonID(), mid1 );
    par.get( IsopachMaker::sKeyCalculateToHorID(), mid2 );
    uiTaskRunner taskrunner( this );
    EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded( mid2, &taskrunner );
    mDynamicCastGet(EM::Horizon3D*,h2,emobj)
    if ( !h2 )
	mErrRet(tr("Cannot load selected horizon"))
    h2->ref();

    EM::ObjectID emidbase = EM::EMM().getObjectID( mid1 );
    EM::EMObject* emobjbase = EM::EMM().getObject( emidbase );
    mDynamicCastGet(EM::Horizon3D*,h1,emobjbase)
    if ( !h1 )
    { h2->unRef(); mErrRet(tr("Cannot find base horizon")) }

    h1->ref();

    int dataidx = -1;
    BufferString attrnm;
    if ( !par.get( IsopachMaker::sKeyAttribName(), attrnm ) )
	return false;

    dataidx = h1->auxdata.addAuxData( attrnm );
    IsopachMaker ipmaker( *h1, *h2, attrnm, dataidx, dps_);
    if ( SI().zIsTime() )
    {
	bool isinmsec = false;
	par.getYN( IsopachMaker::sKeyOutputInMilliSecYN(), isinmsec );
	ipmaker.setUnits( isinmsec );
    }

    bool rv = TaskRunner::execute( &taskrunner, ipmaker );
    h1->unRef(); h2->unRef();
    return rv;
}
