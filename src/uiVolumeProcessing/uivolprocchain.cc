/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uivolprocchain.h"

#include "ctxtioobj.h"
#include "datainpspec.h"
#include "ioman.h"
#include "ioobj.h"
#include "settings.h"
#include "volprocchain.h"
#include "volproctrans.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uispinbox.h"
#include "uitable.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"


namespace VolProc
{

const char* uiChain::sKeySettingKey()
{ return "dTect.ProcessVolumeBuilderOnOK"; }   

mImplFactory2Param( uiStepDialog, uiParent*, Step*, uiStepDialog::factory );


// uiStepDialog
uiStepDialog::uiStepDialog( uiParent* p, const char* stepnm, Step* step )
    : uiDialog( p, uiDialog::Setup("Edit step",stepnm,mNoHelpID) )
    , step_(step)
    , multiinpfld_(0)
{
}


void uiStepDialog::addMultiInputFld()
{
    const int nrinp = step_->getNrInputs();
    if ( nrinp == 0 )
	return;

    const int nrrows = nrinp==-1 ? 5 : nrinp;
    uiTable::Setup ts( nrinp, 1 );
    ts.rowgrow(false).defcollbl("Input");
    multiinpfld_ = new uiTable( this, ts, "Step inputs" );
    initTable( nrinp );

    const Chain::Web& web = step_->getChain().getConnections();
    TypeSet<Chain::Connection> connections;
    web.getConnections( step_->getID(), true, connections );
    for ( int idx=0; idx<nrinp; idx++ )
    {
	Step::SlotID inpslotid = step_->getInputSlotID( idx );
	Step::ID outputstepid = Step::cUndefID();
	for ( int cidx=0; cidx<connections.size(); cidx++ )
	{
	    if ( connections[cidx].inputslotid_ == inpslotid )
		outputstepid = connections[cidx].outputstepid_;
	}

	if ( outputstepid == Step::cUndefID() )
	    continue;

	Step* inputstep = step_->getChain().getStepFromID( outputstepid );
	mDynamicCastGet(uiComboBox*,cb,
		multiinpfld_->getCellObject(RowCol(idx,0)));
	if ( inputstep && cb ) cb->setCurrentItem( inputstep->userName() );
    }
}


void uiStepDialog::initTable( int nr )
{
    if ( !multiinpfld_ ) return;

    BufferStringSet stepnames;
    getStepNames( stepnames );
    multiinpfld_->clearTable();
    for ( int idx=0; idx<nr; idx++ )
    {
	uiComboBox* cb = new uiComboBox( 0, "Steps" );
	cb->addItems( stepnames );
	multiinpfld_->setCellObject( RowCol(idx,0), cb );
    }
}


void uiStepDialog::getStepNames( BufferStringSet& names ) const
{
    Chain& chain = step_->getChain();
    for ( int idx=0; idx<chain.nrSteps(); idx++ )
	if ( step_ != chain.getStep(idx) )
	    names.add( chain.getStep(idx)->userName() );
}


void uiStepDialog::addNameFld( uiObject* alignobj )
{
    uiSeparator* sep = 0;
    if ( alignobj )
    {
	sep = new uiSeparator( this, "namefld sep" );
	sep->attach( stretchedBelow, alignobj );
    }
    namefld_ = new uiGenInput( this, "Name for this step", step_->userName() );
    if ( alignobj )
    {
	namefld_->attach( ensureBelow, sep );
	namefld_->attach( alignedBelow, alignobj );
    }
}


void uiStepDialog::addNameFld( uiGroup* aligngrp )
{
    uiObject* alignobj = aligngrp ? aligngrp->attachObj() : 0;
    addNameFld( alignobj );
}


bool uiStepDialog::acceptOK( CallBacker* )
{
    const BufferString nm( namefld_->text() );
    if ( nm.isEmpty() )
    {
	uiMSG().error( "Please enter a name for this step" );
	return false;
    }

    step_->setUserName( nm.buf() );
    for ( int idx=0; idx<step_->getNrInputs(); idx++ )
    {
	mDynamicCastGet(uiComboBox*,cb,
		multiinpfld_->getCellObject(RowCol(idx,0)));
	const char* stepnm = cb ? cb->text() : 0;
	if ( !stepnm ) continue;

	Step* outstep = step_->getChain().getStepFromName( stepnm );
	Chain::Connection connection( outstep->getID(), 0,
				step_->getID(), step_->getInputSlotID(idx) );
	step_->getChain().addConnection( connection );
    }
    
    return true;
}


// uiChain
uiChain::uiChain( uiParent* p, Chain& chn, bool withprocessnow )
    : uiDialog( p, uiDialog::Setup("Volume Builder: Setup",
				   mNoDlgTitle,"103.6.0")
	    .modal(!withprocessnow) )
    , chain_(chn)
{
    chain_.ref();

    uiToolBar* tb = new uiToolBar( this, "Load/Save toolbar", uiToolBar::Right);
    tb->addButton( "open", "Read stored setup", mCB(this,uiChain,readPush));
    tb->addButton( "save", "Save setup", mCB(this,uiChain,savePush) );
    tb->addButton( "saveas", "Save setup as", mCB(this,uiChain,saveAsPush) );

    uiGroup* flowgrp = new uiGroup( this, "Flow group" );

    const CallBack addcb( mCB(this,uiChain,addStepPush) );
    uiLabel* availablelabel = new uiLabel( flowgrp, "Available steps" );
    factorylist_ = new uiListBox( flowgrp,
				  uiStepDialog::factory().getNames(true) );
    factorylist_->setHSzPol( uiObject::Wide );
    factorylist_->selectionChanged.notify( mCB(this,uiChain,factoryClickCB) );
    factorylist_->attach( ensureBelow, availablelabel );
    factorylist_->doubleClicked.notify( addcb );

    addstepbutton_ = new uiToolButton( flowgrp, uiToolButton::RightArrow,
					"Add step", addcb );
    addstepbutton_->attach( centeredRightOf, factorylist_ );

    steplist_ = new uiListBox( flowgrp );
    steplist_->setHSzPol( uiObject::Wide );
    steplist_->attach( rightTo, factorylist_ );
    steplist_->attach( ensureRightOf, addstepbutton_ );
    steplist_->selectionChanged.notify( mCB(this,uiChain,stepClickCB) );
    steplist_->doubleClicked.notify( mCB(this,uiChain,stepDoubleClickCB) );

    uiLabel* label = new uiLabel( flowgrp, "Used steps" );
    label->attach( alignedAbove, steplist_ );
    label->attach( rightTo, availablelabel );

    moveupbutton_ = new uiToolButton( flowgrp, uiToolButton::UpArrow,
				"Move step up", mCB(this,uiChain,moveUpCB) );
    moveupbutton_->attach( rightOf, steplist_ );

    movedownbutton_ = new uiToolButton( flowgrp, uiToolButton::DownArrow,
			    "Move step down", mCB(this,uiChain,moveDownCB) );
    movedownbutton_->attach( alignedBelow, moveupbutton_ );

    propertiesbutton_ = new uiToolButton( flowgrp, "settings",
	    				  "Edit this step",
					  mCB(this,uiChain,propertiesCB) );
    propertiesbutton_->setName( "Settings" );
    propertiesbutton_->attach( alignedBelow, movedownbutton_ );

    removestepbutton_ = new uiToolButton( flowgrp, "trashcan",
	    	"Remove step from flow", mCB(this,uiChain,removeStepPush) );
    removestepbutton_->attach( alignedBelow, propertiesbutton_ );

    flowgrp->setHAlignObj( steplist_ );

    IOObjContext ctxt = VolProcessingTranslatorGroup::ioContext();
    ctxt.forread = false;

    objfld_ = new uiIOObjSel( this, ctxt, "On OK, store As" );
    objfld_->setInput( chain_.storageID() );
    objfld_->setConfirmOverwrite( false );
    objfld_->attach( alignedBelow, flowgrp );

    if ( withprocessnow )
    {
	enableSaveButton( "Process on OK" );
	bool enabled = false;
	Settings::common().getYN( sKeySettingKey(), enabled );
	setSaveButtonChecked( enabled );
    }

    updateList();
    updateButtons();
}


uiChain::~uiChain()
{
    chain_.unRef();
}


void uiChain::setChain( Chain& chn )
{
    chain_.unRef();
    chain_ = chn;
    chain_.ref();

    updateList();
    updateButtons();
}


void uiChain::updObj( const IOObj& ioobj )
{
    chain_.setStorageID( ioobj.key() );
    objfld_->setInput( ioobj.key() );
}


const MultiID& uiChain::storageID() const
{
    return chain_.storageID();
}


bool uiChain::acceptOK( CallBacker* )
{
    const int nrsteps = chain_.nrSteps();
    if ( nrsteps>0 && chain_.getStep(0) &&
	 chain_.getStep(0)->needsInput() )
    {
	if ( !uiMSG().askGoOn("The first step in the chain needs an input, "
                  "and can thus not be first. Proceed anyway?", true ) )
	    return false;
    }

    const Step* laststep = chain_.getStep( nrsteps-1 );
    chain_.setOutputSlot( laststep->getID(), laststep->getOutputSlotID(0) );

    if ( !doSave() )
	return false;

    if ( hasSaveButton() )
    {
	Settings::common().setYN( sKeySettingKey(), saveButtonChecked() );
	Settings::common().write();
    }

    return true;
}


bool uiChain::doSave()
{
    const IOObj* ioobj = objfld_->ioobj( true );
    if ( !ioobj )
	return doSaveAs();

    BufferString errmsg;
    if ( VolProcessingTranslator::store(chain_,ioobj,errmsg) )
    {
	chain_.setStorageID( ioobj->key() );
	return true;
    }

    uiMSG().error( errmsg );
    return false;
}


bool uiChain::doSaveAs()
{
    IOObjContext ctxt = VolProcessingTranslatorGroup::ioContext();
    ctxt.forread = false;
    uiIOObjSelDlg dlg( this, ctxt, "Volume Builder Setup" );
    if ( !dlg.go() || !dlg.nrSel() )
	 return false;

    BufferString errmsg;
    const IOObj* ioobj = dlg.ioObj();
    if ( VolProcessingTranslator::store(chain_,ioobj,errmsg) )
    {
	chain_.setStorageID( ioobj->key() );
	updObj( *dlg.ioObj() );
	return true;
    }

    uiMSG().error( errmsg );
    return false;
}


void uiChain::updateList()
{
    NotifyStopper stopper( steplist_->selectionChanged );
    int idx=0;
    for ( ; idx<chain_.nrSteps(); idx ++ )
    {
	const char* key = chain_.getStep(idx)->factoryKeyword();
	const char* username = chain_.getStep(idx)->userName();
	const char* displayname = username;
	if ( !displayname )
	{
	    displayname = chain_.getStep(idx)->factoryDisplayName();
	    if ( !displayname )
		displayname = key;
	}

	if ( idx>=steplist_->size() )
	    steplist_->addItem( displayname, false);
	else
	    steplist_->setItemText( idx, displayname );
    }

    for ( ; idx<steplist_->size(); )
	steplist_->removeItem( idx );
}


void uiChain::updateButtons()
{
    const bool factoryselected = factorylist_->nextSelected(-1)!=-1;
    const int stepsel = steplist_->nextSelected(-1);

    addstepbutton_->setSensitive( factoryselected );
    removestepbutton_->setSensitive( stepsel!=-1 );

    moveupbutton_->setSensitive( stepsel>0 );
    movedownbutton_->setSensitive( stepsel!=-1 &&
	    stepsel!=steplist_->size()-1 );

    const bool hasdlg = stepsel>=0 &&
	uiStepDialog::factory().hasName(
		chain_.getStep(stepsel)->factoryKeyword());

    propertiesbutton_->setSensitive( hasdlg );
}


bool uiChain::showPropDialog( int idx )
{
    Step* step = chain_.getStep( idx );
    if ( !step ) return false;

    PtrMan<uiStepDialog> dlg = uiStepDialog::factory().create(
	    step->factoryKeyword(), this, step );
    if ( !dlg )
    {
	uiMSG().error( "Internal error. Step cannot be created" );
	return false;
    }

    bool ret = dlg->go();
    if ( ret )
	updateList();

    return ret;
}


void uiChain::readPush( CallBacker* )
{
    IOObjContext ctxt = VolProcessingTranslatorGroup::ioContext();
    ctxt.forread = true;
    uiIOObjSelDlg dlg( this, ctxt );
    dlg.selGrp()->setConfirmOverwrite( false );
    if ( !dlg.go() || !dlg.nrSel() )
	return;

    BufferString errmsg;
    if ( VolProcessingTranslator::retrieve( chain_, dlg.ioObj(), errmsg ) )
    {
	updObj( *dlg.ioObj() );
	updateList();
	return;
    }

    updateList();
    uiMSG().error(errmsg);
}


void uiChain::saveAsPush( CallBacker* )
{ doSaveAs(); }


void uiChain::savePush( CallBacker* )
{ doSave(); }


void uiChain::factoryClickCB(CallBacker*)
{
    if ( factorylist_->nextSelected(-1) == -1 )
	return;

    steplist_->selectAll(false);
    updateButtons();
}


void uiChain::stepClickCB(CallBacker*)
{
    if ( steplist_->nextSelected(-1) == -1 )
	return;

    factorylist_->selectAll(false);
    updateButtons();
}


void uiChain::stepDoubleClickCB(CallBacker*)
{
    factorylist_->selectAll(false);
    updateButtons();

    showPropDialog( steplist_->currentItem() );
}


void uiChain::addStepPush(CallBacker*)
{
    const int sel = factorylist_->nextSelected(-1);
    if ( sel == -1 )
	return;

    const char* steptype = uiStepDialog::factory().getNames(false)[sel]->buf();
    Step* step = Step::factory().create( steptype );
    if ( !step ) return;

    chain_.addStep( step );
    updateList();
    steplist_->selectAll( false );
    int curitm = steplist_->size() - 1;
    steplist_->setCurrentItem( curitm );
    if ( !showPropDialog(curitm) )
    {
	chain_.removeStep( curitm );
	updateList();
	curitm--;
	if ( curitm >= 0 )
	    steplist_->setCurrentItem( curitm );
    }
    updateButtons();
}


void uiChain::removeStepPush(CallBacker*)
{
    int curitm = steplist_->nextSelected(-1);
    if ( curitm < 0 )
	return;

    chain_.removeStep( curitm );
    updateList();
    steplist_->selectAll(false);
    if ( curitm >= steplist_->size() )
	curitm--;
    if ( curitm >= 0 )
	steplist_->setCurrentItem( curitm );

    updateButtons();
}


void uiChain::moveUpCB(CallBacker*)
{
    const int idx = steplist_->nextSelected(-1);

    if ( idx<1 ) return;

    chain_.swapSteps( idx, idx-1 );
    updateList();

    steplist_->setSelected( idx-1, true );
    updateButtons();
}


void uiChain::moveDownCB(CallBacker*)
{
    const int idx = steplist_->nextSelected(-1);
    if ( idx<0 || idx>=chain_.nrSteps() )
	return;

    chain_.swapSteps( idx, idx+1 );
    updateList();
    steplist_->setSelected( idx+1, true );
    updateButtons();
}


void uiChain::propertiesCB(CallBacker*)
{
    const int idx = steplist_->nextSelected(-1);
    if ( idx<0 ) return;

    showPropDialog( idx );
}

} // namespace VolProc
