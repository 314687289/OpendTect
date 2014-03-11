/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene
 Date:          July 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistrattreewin.h"

#include "compoundkey.h"
#include "ioman.h"
#include "stratunitrepos.h"
#include "uitoolbutton.h"
#include "uicolor.h"
#include "uidialog.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uimain.h"
#include "uimenu.h"
#include "uiselsimple.h"
#include "uimsg.h"
#include "uiobjdisposer.h"
#include "stratreftree.h"
#include "uiparent.h"
#include "uisplitter.h"
#include "uistratreftree.h"
#include "uistratlvllist.h"
#include "uistratutildlgs.h"
#include "uistratdisplay.h"
#include "uiselsimple.h"
#include "uitoolbar.h"
#include "uitreeview.h"

#define	mExpandTxt(domenu)	domenu ? "&Expand all" : "Expand all"
#define	mCollapseTxt(domenu)	domenu ? "&Collapse all" : "Collapse all"

//tricky: want to show action in menu and status on button
#define	mEditTxt(domenu)	domenu ? "&Unlock" : "Toggle read only: locked"
#define	mLockTxt(domenu)	domenu ? "&Lock" : "Toggle read only: editable"

using namespace Strat;

ManagedObjectSet<uiToolButtonSetup> uiStratTreeWin::tbsetups_;

static uiStratTreeWin* stratwin = 0;
const uiStratTreeWin& StratTWin()
{
    if ( !stratwin )
    {
	uiMain& app = uiMain::theMain();
	stratwin = new uiStratTreeWin( app.topLevel() );
    }

    return *stratwin;
}
uiStratTreeWin& StratTreeWin()
{
    return const_cast<uiStratTreeWin&>( StratTWin() );
}


uiStratTreeWin::uiStratTreeWin( uiParent* p )
    : uiMainWin(p,"Manage Stratigraphy", 0, true)
    , needsave_(false)
    , istreedisp_(false)
    , repos_(*new Strat::RepositoryAccess())
    , tb_(0)
{
    IOM().surveyChanged.notify( mCB(this,uiStratTreeWin,forceCloseCB ) );
    IOM().applicationClosing.notify( mCB(this,uiStratTreeWin,forceCloseCB ) );
    if ( RT().isEmpty() )
	setNewRT();

    createMenu();
    createToolBar();
    createGroups();
    setExpCB(0);
    editCB(0);
    moveUnitCB(0);
}


uiStratTreeWin::~uiStratTreeWin()
{
    delete &repos_;
}


void uiStratTreeWin::setNewRT()
{
    BufferStringSet opts;
    opts.add( "<Build from scratch>" );
    Strat::RefTree::getStdNames( opts );

    const bool nortpresent = RT().isEmpty();
    BufferString dlgmsg( "Stratigraphy: " );
    dlgmsg += nortpresent ? "select initial" : "select new";
    uiSelectFromList::Setup su( dlgmsg, opts );
    uiSelectFromList dlg( this, su );
    if ( nortpresent )
	dlg.setButtonText( uiDialog::CANCEL, "" );
    if ( dlg.go() )
    {
	const char* nm = opts.get( dlg.selection() );
	Strat::LevelSet* ls = 0;
	Strat::RefTree* rt = 0;

	if ( dlg.selection() > 0 )
	{
	    ls = Strat::LevelSet::createStd( nm );
	    if ( !ls )
		{ pErrMsg( "Cannot read LevelSet from Std!" ); return; }
	    else
	    {
		rt = Strat::RefTree::createStd( nm );
		if ( !rt )
		    { pErrMsg( "Cannot read RefTree from Std!" ); return; }
	    }
	}
	else
	{
	    rt = new RefTree();
	    ls = new LevelSet();
	}
	Strat::setLVLS( ls );
	Strat::setRT( rt );
	needsave_ = true;
	const Repos::Source dest = Repos::Survey;
	Strat::LVLS().store( dest );
	Strat::RepositoryAccess().writeTree( Strat::RT(), dest );
	if ( tb_ )
	    resetCB( 0 );
    }
}


void uiStratTreeWin::addTool( uiToolButtonSetup* su )
{
    if ( !su ) return;
    tbsetups_ += su;
    if ( stratwin )
	stratwin->tb_->addButton( *su );
}


void uiStratTreeWin::popUp() const
{
    uiStratTreeWin& self = *const_cast<uiStratTreeWin*>(this);
    self.show();
    self.raise();
}


void uiStratTreeWin::createMenu()
{
    uiMenuBar* menubar = menuBar();
    uiMenu* mnu = new uiMenu( this, "&Actions" );
    expandmnuitem_ = new uiAction( mExpandTxt(true),
				     mCB(this,uiStratTreeWin,setExpCB) );
    mnu->insertItem( expandmnuitem_ );
    expandmnuitem_->setIcon( ioPixmap("collapse_tree") );
    mnu->insertSeparator();
    editmnuitem_ = new uiAction( mEditTxt(true),
				   mCB(this,uiStratTreeWin,editCB) );
    mnu->insertItem( editmnuitem_ );
    editmnuitem_->setIcon( ioPixmap("unlock") );
    savemnuitem_ = new uiAction( sSave(), mCB(this,uiStratTreeWin,saveCB) );
    mnu->insertItem( savemnuitem_ );
    savemnuitem_->setIcon( ioPixmap("save") );
    resetmnuitem_ = new uiAction( "&Reset to last saved",
				    mCB(this,uiStratTreeWin,resetCB));
    mnu->insertItem( resetmnuitem_ );
    resetmnuitem_->setIcon( ioPixmap("undo") );
    mnu->insertSeparator();

    saveasmnuitem_ = new uiAction( sSaveAs(),
				     mCB(this,uiStratTreeWin,saveAsCB) );
    mnu->insertItem( saveasmnuitem_ );
    saveasmnuitem_->setIcon( ioPixmap("saveas") );
    menubar->insertItem( mnu );
}

#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton( tb_, fnm, tt, mCB(this,uiStratTreeWin,cbnm) ); \
    tb_->addButton( but );


void uiStratTreeWin::createToolBar()
{
    tb_ = new uiToolBar( this, "Stratigraphy Manager Tools" );
    mDefBut(colexpbut_,"collapse_tree",setExpCB,mCollapseTxt(false));
    colexpbut_->setSensitive( istreedisp_ );
    tb_->addSeparator();
    mDefBut(moveunitupbut_,"uparrow",moveUnitCB,"Move unit up");
    mDefBut(moveunitdownbut_,"downarrow",moveUnitCB,"Move unit down");
    tb_->addSeparator();
    mDefBut(newbut_,"newset",newCB,"New");
    mDefBut(lockbut_,"unlock",editCB,mEditTxt(false));
    lockbut_->setToggleButton( true );
    uiToolButton* uitb;
    mDefBut(uitb,"save",saveCB,"Save");
    mDefBut(uitb,"contexthelp",helpCB,"Help on this window");
    tb_->addSeparator();
    mDefBut( switchviewbut_, "strat_tree", switchViewCB, "Switch View" );
    mDefBut( lithobut_, "lithologies", manLiths, "Manage Lithologies" );
    mDefBut( contentsbut_, "contents", manConts, "Manage Content Types" );

    for ( int idx=0; idx<tbsetups_.size(); idx++ )
	tb_->addButton( *tbsetups_[idx] );
}


void uiStratTreeWin::createGroups()
{
    uiGroup* leftgrp = new uiGroup( this, "LeftGroup" );
    leftgrp->setStretch( 1, 1 );
    uiGroup* rightgrp = new uiGroup( this, "RightGroup" );
    rightgrp->setStretch( 1, 1 );

    uitree_ = new uiStratRefTree( leftgrp );
    CallBack selcb = mCB( this,uiStratTreeWin,unitSelCB );
    CallBack renmcb = mCB(this,uiStratTreeWin,unitRenamedCB);
    uitree_->treeView()->selectionChanged.notify( selcb );
    uitree_->treeView()->itemRenamed.notify( renmcb );
    uitree_->treeView()->display( false );

    if ( !uitree_->haveTimes() )
	uitree_->setEntranceDefaultTimes();

    uistratdisp_ = new uiStratDisplay( leftgrp, *uitree_ );
    uistratdisp_->addControl( tb_ );

    lvllist_ = new uiStratLvlList( rightgrp );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", true );
    splitter->addGroup( leftgrp );
    splitter->addGroup( rightgrp );
}


void uiStratTreeWin::setExpCB( CallBacker* )
{
    const bool expand =
	FixedString(expandmnuitem_->text().getFullString()) == mExpandTxt(true);
    uitree_->expand( expand );
    expandmnuitem_->setText( expand ? mCollapseTxt(true) : mExpandTxt(true) );
    expandmnuitem_->setIcon( expand ? ioPixmap("collapse_tree")
				      : ioPixmap("expand_tree") );
    colexpbut_->setPixmap( expand ? ioPixmap("collapse_tree")
				  : ioPixmap("expand_tree") );
    colexpbut_->setToolTip( expand ? mCollapseTxt(false) : mExpandTxt(false) );
}


void uiStratTreeWin::unitSelCB(CallBacker*)
{
    moveUnitCB(0);
}


void uiStratTreeWin::newCB( CallBacker* )
{
    BufferString msg( "This will overwrite the current tree. \n" );
    msg += "Your work will be lost. Continue anyway ?";
    if ( RT().isEmpty() || uiMSG().askGoOn( msg ) )
	setNewRT();
}


void uiStratTreeWin::editCB( CallBacker* )
{
    const bool doedit =
	FixedString(editmnuitem_->text().getFullString()) == mEditTxt(true);
    uitree_->makeTreeEditable( doedit );
    editmnuitem_->setText( doedit ? mLockTxt(true) : mEditTxt(true) );
    editmnuitem_->setIcon( doedit ? ioPixmap("unlock")
				    : ioPixmap("readonly") );
    lockbut_->setPixmap( doedit ? ioPixmap("unlock")
				: ioPixmap("readonly") );
    lockbut_->setToolTip( doedit ? mLockTxt(false) : mEditTxt(false) );
    lockbut_->setOn( !doedit );
    setIsLocked( !doedit );
}


void uiStratTreeWin::setIsLocked( bool yn )
{
    uistratdisp_->setIsLocked( yn );
    lvllist_->setIsLocked( yn );
    lithobut_->setSensitive( !yn );
    contentsbut_->setSensitive( !yn );
    newbut_->setSensitive( !yn );
    saveasmnuitem_->setEnabled( !yn );
    resetmnuitem_->setEnabled( !yn );
}


void uiStratTreeWin::resetCB( CallBacker* )
{
    Strat::RefTree& bcktree = Strat::eRT();
    //for the time beeing, get back the global tree, but we may want to have
    //a snapshot copy of the actual tree we are working on...
    uitree_->setTree( bcktree, true );
    uitree_->expand( true );
    uistratdisp_->setTree();
    lvllist_->setLevels();
}


void uiStratTreeWin::saveCB( CallBacker* )
{
    Strat::eLVLS().store( Repos::Survey );
    if ( uitree_->tree() )
    {
	repos_.writeTree( *uitree_->tree() );
	needsave_ = false;
    }
    else
	uiMSG().error( "Can not find tree" );
}


static const char* infolvltrs[] =
{
    "Survey level",
    "OpendTect data level",
    "User level",
    "Global level",
    0
};

void uiStratTreeWin::saveAsCB( CallBacker* )
{
    const char* dlgtit = "Save the stratigraphy at:";

    uiDialog savedlg( this, uiDialog::Setup( "Save Stratigraphy",
		    dlgtit, mNoHelpKey ) );
    BufferStringSet bfset( infolvltrs );
    uiListBox saveloclist( &savedlg, bfset );
    savedlg.go();
    if ( savedlg.uiResult() == 1 )
    {
	const BufferString savetxt = saveloclist.getText();
	Repos::Source src = Repos::Survey;
	if ( savetxt == infolvltrs[1] )
	    src = Repos::Data;
	else if ( savetxt == infolvltrs[2] )
	    src = Repos::User;
	else if ( savetxt == infolvltrs[3] )
	    src = Repos::ApplSetup;

	repos_.writeTree( Strat::RT(), src );
    }
}


void uiStratTreeWin::switchViewCB( CallBacker* )
{
    istreedisp_ = !istreedisp_;
    uistratdisp_->display( !istreedisp_ );
    if ( uistratdisp_->control() )
	uistratdisp_->control()->setSensitive( !istreedisp_ );
    uitree_->treeView()->display( istreedisp_ );
    switchviewbut_->setPixmap(
	istreedisp_ ? "stratframeworkgraph" : "strat_tree" );
    colexpbut_->setSensitive( istreedisp_ );
}


void uiStratTreeWin::unitRenamedCB( CallBacker* )
{
    needsave_ = true;
    //TODO requires Qt4 to approve/cancel renaming ( name already in use...)
}


bool uiStratTreeWin::closeOK()
{
    const bool needsave = uitree_->anyChg() || needsave_ ||  lvllist_->anyChg();
    uitree_->setNoChg();
    lvllist_->setNoChg();
    needsave_ = false;

    if ( needsave )
    {
	int res = uiMSG().askSave(
	    "Stratigraphic framework has changed\n Do you want to save it?" );

	if ( res == 1 )
	    saveCB( 0 );
	else if ( res == 0 )
	{
	    Strat::setRT( repos_.readTree() );
	    resetCB( 0 );
	    return true;
	}
	else if ( res == -1 )
	    return false;
    }

    return true;
}


void uiStratTreeWin::forceCloseCB( CallBacker* )
{
    IOM().surveyChanged.remove( mCB(this,uiStratTreeWin,forceCloseCB ) );
    IOM().applicationClosing.remove( mCB(this,uiStratTreeWin,forceCloseCB ) );
    if ( stratwin )
	stratwin->close();

    uiOBJDISP()->go( uistratdisp_ );

    delete lvllist_;
    delete uitree_;
    stratwin = 0;
}


void uiStratTreeWin::moveUnitCB( CallBacker* cb )
{
    if ( cb)
	uitree_->moveUnit( cb == moveunitupbut_ );

    moveunitupbut_->setSensitive( uitree_->canMoveUnit( true ) );
    moveunitdownbut_->setSensitive( uitree_->canMoveUnit( false ) );
}


void uiStratTreeWin::helpCB( CallBacker* )
{
    HelpProvider::provideHelp( "110.0.0" );
}


void uiStratTreeWin::manLiths( CallBacker* )
{
    uiStratLithoDlg dlg( this );
    dlg.go();
    if ( dlg.anyChg() ) needsave_ = true;
    uitree_->updateLithoCol();
}


void uiStratTreeWin::manConts( CallBacker* )
{
    uiStratContentsDlg dlg( this );
    dlg.go();
    if ( dlg.anyChg() ) needsave_ = true;
}


void uiStratTreeWin::changeLayerModelNumber( bool add )
{
    mDefineStaticLocalObject( int, nrlayermodelwin, = 0 );
    bool haschged = false;
    if ( add )
    {
	nrlayermodelwin ++;
	if ( nrlayermodelwin == 1 )
	    haschged = true;
    }
    else
    {
	nrlayermodelwin --;
	if ( nrlayermodelwin == 0 )
	    haschged = true;
    }
    if ( haschged )
    {
	const bool islocked = lockbut_->isOn();
	if ( (!islocked && nrlayermodelwin) || (islocked && !nrlayermodelwin) )
	    { lockbut_->setOn( nrlayermodelwin ); editCB(0); }
	lockbut_->setSensitive( !nrlayermodelwin );
	editmnuitem_->setEnabled( !nrlayermodelwin );
    }
}
