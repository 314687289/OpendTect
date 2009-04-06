
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: uimadagascarmain.cc,v 1.27 2009-04-06 07:31:25 cvsranojay Exp $";

#include "uimadagascarmain.h"
#include "uimadiosel.h"
#include "uimadbldcmd.h"
#include "madprocflow.h"
#include "madprocflowtr.h"
#include "madprocexec.h"
#include "madio.h"
#include "uilistbox.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uigeninput.h"
#include "uimenu.h"
#include "uitoolbar.h"
#include "uiseparator.h"
#include "uiioobjsel.h"
#include "uifiledlg.h"
#include "uimsg.h"
#include "cubesampling.h"
#include "pixmap.h"
#include "keystrs.h"
#include "ioman.h"
#include "oddirs.h"
#include "seisioobjinfo.h"
#include "strmprov.h"

const char* sKeySeisOutIDKey = "Output Seismics Key";

uiMadagascarMain::uiMadagascarMain( uiParent* p )
	: uiFullBatchDialog(p,Setup("Madagascar processing").menubar(true)
				   .procprognm("odmadexec")
				   .modal(false))
	, ctio_(*mMkCtxtIOObj(ODMadProcFlow))
	, bldfld_(0)
	, procflow_(*new ODMad::ProcFlow())
	, needsave_(false)
{
    setCtrlStyle( uiDialog::DoAndStay );
    setHelpID( "103.5.0" );
    addStdFields( false, false );
    setHaveCredits( true );
    createMenus();

    uiGroup* maingrp = new uiGroup( uppgrp_, "Main group" );

    infld_ = new uiMadIOSel( maingrp, true );
    infld_->selectionMade.notify( mCB(this,uiMadagascarMain,inpSel) );

    uiGroup* procgrp = crProcGroup( maingrp );
    procgrp->attach( alignedBelow, infld_ );

    outfld_ = new uiMadIOSel( maingrp, false );
    outfld_->selectionMade.notify( mCB(this,uiMadagascarMain,inpSel) );
    outfld_->attach( alignedBelow, procgrp );

    bldfld_ = new uiMadagascarBldCmd( uppgrp_ );
    bldfld_->cmdAvailable.notify( mCB(this,uiMadagascarMain,cmdAvail) );

    uiSeparator* sep = new uiSeparator( uppgrp_, "VSep", false );
    sep->attach( rightTo, maingrp );
    bldfld_->attach( rightTo, sep );
    uppgrp_->setHAlignObj( sep );

    setParFileNmDef( "Mad_Proc" );
    updateCaption();
    finaliseDone.notify( mCB(this,uiMadagascarMain,setButStates) );
}


uiMadagascarMain::~uiMadagascarMain()
{
    delete ctio_.ioobj; delete &ctio_;
    delete &procflow_;
}


#define mInsertItem( txt, func ) \
    mnu->insertItem( new uiMenuItem(txt,mCB(this,uiMadagascarMain,func)) )
#define mAddButton(pm,func,tip) \
    toolbar->addButton( pm, mCB(this,uiMadagascarMain,func), tip )

void uiMadagascarMain::createMenus()
{
    uiMenuBar* menubar = menuBar();
    if ( !menubar ) { pErrMsg("huh?"); return; }

    uiPopupMenu* mnu = new uiPopupMenu( this, "&File" );
    mInsertItem( "&New flow ...", newFlow );
    mInsertItem( "&Open flow ...", openFlow );
    mInsertItem( "&Save flow ...", saveFlow );
    mnu->insertSeparator();
    mInsertItem( "&Export flow ...", exportFlow );
    mnu->insertSeparator();
    mInsertItem( "&Quit", reject );
    menubar->insertItem( mnu );

    uiToolBar* toolbar = new uiToolBar( this, "Flow tools" );
    mAddButton( "newflow.png", newFlow, "Empty this flow" );
    mAddButton( "openflow.png", openFlow, "Open saved flow" );
    mAddButton( "saveflow.png", saveFlow, "Save flow" );
}


uiGroup* uiMadagascarMain::crProcGroup( uiGroup* grp )
{
    uiGroup* procgrp = new uiGroup( grp, "Proc group" );
    const CallBack butpushcb( mCB(this,uiMadagascarMain,butPush) );

    uiLabeledListBox* pfld = new uiLabeledListBox( procgrp, "FLOW", false,
						   uiLabeledListBox::LeftMid );
    procsfld_ = pfld->box();
    procsfld_->setPrefWidthInChar( 20 );
    procsfld_->selectionChanged.notify( mCB(this,uiMadagascarMain,selChg) );

    uiButtonGroup* bgrp = new uiButtonGroup( procgrp, "", false );
    bgrp->displayFrame( true );
    upbut_ = new uiToolButton( bgrp, "Up button", butpushcb );
    upbut_->setArrowType( uiToolButton::UpArrow );
    upbut_->setToolTip( "Move current command up" );
    downbut_ = new uiToolButton( bgrp, "Down button", butpushcb );
    downbut_->setArrowType( uiToolButton::DownArrow );
    downbut_->setToolTip( "Move current command down" );
    rmbut_ = new uiToolButton( bgrp, "Remove button", ioPixmap("trashcan.png"),
	    			butpushcb );
    rmbut_->setToolTip( "Remove current command from flow" );
    bgrp->attach( centeredBelow, pfld );

    procgrp->setHAlignObj( pfld );
    return procgrp;
}


void uiMadagascarMain::inpSel( CallBacker* cb )
{
    if ( cb )
	needsave_ = true;

    IOPar& inpar = procflow_.input();
    infld_->fillPar( inpar );
    outfld_->useParIfNeeded( inpar );

    IOPar& outpar = procflow_.output();
    outfld_->fillPar( outpar );
    BufferString inptyp = inpar.find( sKey::Type );
    BufferString outptyp = outpar.find( sKey::Type );

    if ( inptyp==Seis::nameOf(Seis::Vol) && outptyp==Seis::nameOf(Seis::Vol) )
	singmachfld_->setSensitive( true );
    else
    {
	singmachfld_->setValue( true );
	singmachfld_->setSensitive( false );
    }
}

#undef mErrRet
#define mErrRet(s) { uiMSG().error(s); return false; }

void uiMadagascarMain::cmdAvail( CallBacker* cb )
{
    ODMad::Proc* proc = bldfld_->proc();

    if ( !proc ) return;

    if ( bldfld_->isAdd() )
    {
	procsfld_->addItem( proc->getSummary() );
	procflow_ += proc;
	needsave_ = true;
	procsfld_->setCurrentItem( procsfld_->size() - 1 );
    }
    else
    {
	const int curidx = procsfld_->currentItem();
	if ( curidx < 0 ) return;
	needsave_ = true;
	procsfld_->setItemText( curidx, proc->getSummary() );
	ODMad::Proc* prevproc = procflow_.replace( curidx, proc );
	delete prevproc;
    }

    setButStates( this );
}


void uiMadagascarMain::hideReq( CallBacker* cb )
{
    bldfld_->display( false );
}


void uiMadagascarMain::butPush( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,tb,cb)
    int curidx = procsfld_->currentItem();
    const int sz = procsfld_->size();

    if ( tb == rmbut_ )
    {
	if ( curidx < 0 ) return;
	needsave_ = true;
	procsfld_->removeItem( curidx );
	ODMad::Proc* prevproc = procflow_.remove( curidx );
	delete prevproc;
	if ( curidx >= procsfld_->size() )
	    curidx--;
    }
    else if ( tb == upbut_ || tb == downbut_ )
    {
	if ( curidx < 0 ) return;
	const bool isup = tb == upbut_;
	const int newcur = curidx + (isup ? -1 : 1);
	if ( newcur >= 0 && newcur < sz )
	{
	    BufferString tmp( procsfld_->textOfItem(newcur) );
	    procsfld_->setItemText( newcur, procsfld_->getText() );
	    procsfld_->setItemText( curidx, tmp );
	    procflow_.swap( curidx, newcur );
	    curidx = newcur;
	    needsave_ = true;
	}
    }

    if ( curidx >= 0 )
	procsfld_->setCurrentItem( curidx );

    setButStates(0);
}


void uiMadagascarMain::setButStates( CallBacker* cb )
{
    const bool havesel = !procsfld_->isEmpty();
    rmbut_->setSensitive( havesel );
    selChg( cb );
    inpSel(0);
}


void uiMadagascarMain::selChg( CallBacker* cb )
{
    const int curidx = procsfld_->isEmpty() ? -1 : procsfld_->currentItem();
    const int sz = procsfld_->size();
    upbut_->setSensitive( sz > 1 && curidx > 0 );
    downbut_->setSensitive( sz > 1 && curidx >= 0 && curidx < sz-1 );

    if ( cb == this || !bldfld_ ) return;
    const ODMad::Proc* proc = curidx < 0 ? 0 : procflow_[curidx];
    bldfld_->setProc( proc );
}


void uiMadagascarMain::newFlow( CallBacker* )
{
    deepErase( procflow_ );
    procflow_.setName( 0 );
    procflow_.input().clear();
    procflow_.output().clear();
    updateCaption();
}


void uiMadagascarMain::openFlow( CallBacker* )
{
    if ( needsave_ )
    {
	if ( !uiMSG().askGoOn("Current flow has not been saved, continue?") )
	    return;
    }

    ctio_.ctxt.forread = true;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( dlg.go() )
    {
	ctio_.setObj( dlg.ioObj()->clone() );
	BufferString emsg;
	deepErase( procflow_ );
	procsfld_->empty();
	if ( !ODMadProcFlowTranslator::retrieve(procflow_,ctio_.ioobj,emsg) )
	    uiMSG().error( emsg );
	else
	{
	    infld_->usePar( procflow_.input() );
	    outfld_->usePar( procflow_.output() );
	    for ( int idx=0; idx<procflow_.size(); idx++ )
		procsfld_->addItem( procflow_[idx]->getSummary() );

	    procsfld_->setCurrentItem( procsfld_->size() - 1 );
	    needsave_ = false;
	    procflow_.setName( ctio_.ioobj->name() );
	    updateCaption();
	}
    }
}


void uiMadagascarMain::saveFlow( CallBacker* )
{
    ctio_.ctxt.forread = false;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( dlg.go() )
    {
	ctio_.setObj( dlg.ioObj()->clone() );
	BufferString emsg;
	if ( !ODMadProcFlowTranslator::store(procflow_,ctio_.ioobj,emsg) )
	    uiMSG().error( emsg );
	else
	{
	    needsave_ = false;
	    procflow_.setName( ctio_.ioobj->name() );
	    updateCaption();
	}
    }
}


void uiMadagascarMain::updateCaption()
{
    const char* flowname = procflow_.name();
    BufferString caption = "Madagascar processing   [";
    caption += flowname && *flowname ? flowname : "New Flow";
    caption += "]";
    setCaption( caption );
}


void uiMadagascarMain::exportFlow( CallBacker* )
{
    IOM().to( ODMad::sKeyMadSelKey() );
    uiFileDialog dlg( this, false );
    dlg.setDirectory( IOM().curDir() );
    if ( !dlg.go() ) return;
    uiMSG().error( "Export needs implementation" );
}


bool uiMadagascarMain::fillPar( IOPar& iop )
{
    BufferString errmsg;
    if ( !procflow_.isOK(errmsg) )
	mErrRet( errmsg.buf() )

    procflow_.fillPar( iop );

    iop.set( sKeySeisOutIDKey, "Output.ID" );
    return true;
}


bool uiMadagascarMain::rejectOK( CallBacker* )
{
    if ( needsave_ )
    {
	if ( !uiMSG().askGoOn("Current flow has not been saved, quit anyway?") )
	    return false;
    }

    return true;
}
