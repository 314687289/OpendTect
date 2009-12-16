/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	K. Tingdahl
 Date: 		Jul 2003
___________________________________________________________________

-*/
static const char* rcsID = "$Id: uioddisplaytreeitem.cc,v 1.36 2009-12-16 16:00:29 cvsjaap Exp $";

#include "uioddisplaytreeitem.h"
#include "uiodattribtreeitem.h"

#include "pixmap.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"
#include "vissurvobj.h"


bool uiODDisplayTreeItem::create( uiTreeItem* treeitem, uiODApplMgr* appl,
				  int displayid )
{
    const uiTreeFactorySet* tfs = ODMainWin()->sceneMgr().treeItemFactorySet();
    if ( !tfs )
	return false;

    for ( int idx=0; idx<tfs->nrFactories(); idx++ )
    {
	mDynamicCastGet(const uiODTreeItemFactory*,itmcreater,
			tfs->getFactory(idx))
	if ( !itmcreater ) continue;

	uiTreeItem* res = itmcreater->create( displayid, treeitem );
	if ( res )
	{
	    treeitem->addChild( res, false );
	    return true;
	}
    }

    return false;
}

static const int cHistogramIdx = 991;
static const int cAttribIdx = 1000;
static const int cDuplicateIdx = 900;
static const int cLinkIdx = 800;
static const int cLockIdx = -900;
static const int cHideIdx = -950;
static const int cRemoveIdx = -1000;

uiODDisplayTreeItem::uiODDisplayTreeItem()
    : uiODTreeItem(0)
    , displayid_(-1)
    , visserv_(ODMainWin()->applMgr().visServer())
    , addattribmnuitem_("&Add attribute",cAttribIdx)
    , duplicatemnuitem_("&Duplicate",cDuplicateIdx)
    , displyhistgram_("&Show histogram ...",cHistogramIdx)		   
    , linkmnuitem_("&Link ...",cLinkIdx)
    , lockmnuitem_("&Lock",cLockIdx)
    , hidemnuitem_("&Hide",cHideIdx )
    , removemnuitem_("&Remove",cRemoveIdx)
{
}


uiODDisplayTreeItem::~uiODDisplayTreeItem()
{
    MenuHandler* menu = visserv_->getMenuHandler();
    if ( menu )
    {
	menu->createnotifier.remove(mCB(this,uiODDisplayTreeItem,createMenuCB));
	menu->handlenotifier.remove(mCB(this,uiODDisplayTreeItem,handleMenuCB));
    }

    ODMainWin()->sceneMgr().remove2DViewer( displayid_ );
}


int uiODDisplayTreeItem::selectionKey() const
{
    return displayid_;
}


bool uiODDisplayTreeItem::shouldSelect( int selkey ) const
{
    return uiTreeItem::shouldSelect( selkey ) && visserv_->getSelAttribNr()==-1;
}


uiODDataTreeItem* uiODDisplayTreeItem::createAttribItem(
					const Attrib::SelSpec* as ) const
{
    const char* parenttype = typeid(*this).name();
    uiODDataTreeItem* res = as
	? uiODDataTreeItem::factory().create( 0, *as, parenttype, false ) : 0;
    if ( !res ) res = new uiODAttribTreeItem( parenttype );
    return res;
}


uiODDataTreeItem* uiODDisplayTreeItem::addAttribItem()
{
    uiODDataTreeItem* newitem = createAttribItem(0);
    visserv_->addAttrib( displayid_ );
    addChild( newitem, false );
    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
    return newitem;
}


bool uiODDisplayTreeItem::init()
{
    if ( !uiTreeItem::init() ) return false;

    if ( visserv_->hasAttrib( displayid_ ) )
    {
	for ( int attrib=0; attrib<visserv_->getNrAttribs(displayid_); attrib++)
	{
	    const Attrib::SelSpec* as = visserv_->getSelSpec(displayid_,attrib);
	    uiODDataTreeItem* item = createAttribItem( as );
	    if ( item )
	    {
		addChild( item, false );
		item->setChecked( visserv_->isAttribEnabled(displayid_,attrib));
	    }
	}
    }

    visserv_->setSelObjectId( displayid_ );
    setChecked( visserv_->isOn(displayid_) );
    checkStatusChange()->notify(mCB(this,uiODDisplayTreeItem,checkCB));

    name_ = createDisplayName();

    MenuHandler* menu = visserv_->getMenuHandler();
    menu->createnotifier.notify( mCB(this,uiODDisplayTreeItem,createMenuCB) );
    menu->handlenotifier.notify( mCB(this,uiODDisplayTreeItem,handleMenuCB) );

    return true;
}


void uiODDisplayTreeItem::updateCheckStatus()
{
    const bool ison = visserv_->isOn( displayid_ );
    const bool ischecked = isChecked();
    if ( ison != ischecked )
	setChecked( ison );

    uiTreeItem::updateCheckStatus();
}


void uiODDisplayTreeItem::updateLockPixmap( bool islocked )
{
    PtrMan<ioPixmap> pixmap = 0;
    if ( islocked )
	pixmap = new ioPixmap( "lock_small.png" );
    else
	pixmap = new ioPixmap();

    uilistviewitem_->setPixmap( 0, *pixmap );
}


void uiODDisplayTreeItem::updateColumnText( int col )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,
	    	    visserv_->getObject(displayid_))
    if ( col==uiODSceneMgr::cNameColumn() )
    {
	name_ = createDisplayName();
	updateLockPixmap( visserv_->isLocked(displayid_) );
    }

    else if ( col==uiODSceneMgr::cColorColumn() )
    {
	if ( !so )
	{
	    uiTreeItem::updateColumnText( col );
	    return;
	}
	
	PtrMan<ioPixmap> pixmap = 0;
	if ( so->hasColor() )
	{
	    pixmap = new ioPixmap( uiODDataTreeItem::cPixmapWidth(),
		    		   uiODDataTreeItem::cPixmapHeight() );
	    pixmap->fill( so->getColor() );
	}

	if ( pixmap ) uilistviewitem_->setPixmap( uiODSceneMgr::cColorColumn(),
						 *pixmap );
    }

    uiTreeItem::updateColumnText( col );
}


bool uiODDisplayTreeItem::showSubMenu()
{
    return visserv_->showMenu( displayid_, uiMenuHandler::fromTree() );
}


void uiODDisplayTreeItem::checkCB( CallBacker* )
{
    if ( !visserv_->isSoloMode() )
	visserv_->turnOn( displayid_, isChecked() );
}


int uiODDisplayTreeItem::uiListViewItemType() const
{
    return uiListViewItem::CheckBox;
}


BufferString uiODDisplayTreeItem::createDisplayName() const
{
    const uiVisPartServer* cvisserv =
		const_cast<uiODDisplayTreeItem*>(this)->applMgr()->visServer();
    return cvisserv->getObjectName( displayid_ );
}


const char* uiODDisplayTreeItem::getLockMenuText() 
{ 
    return visserv_->isLocked(displayid_) ? "Un&lock" : "&Lock";
}


void uiODDisplayTreeItem::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( menu->menuID() != displayID() )
	return;

    if ( visserv_->hasAttrib(displayid_) &&
	 visserv_->canHaveMultipleAttribs(displayid_) )
    {
	mAddMenuItem( menu, &addattribmnuitem_,
		      !visserv_->isLocked(displayid_) &&
		      visserv_->canAddAttrib(displayid_), false );
	mAddMenuItem( menu, &displyhistgram_, true, false );
    }
    else
    {
	mResetMenuItem( &addattribmnuitem_ );
	mResetMenuItem( &displyhistgram_ );
    }

    lockmnuitem_.text = getLockMenuText(); 
    mAddMenuItem( menu, &lockmnuitem_, true, false );
    if ( visserv_->canHaveMultipleAttribs(displayid_) )
    { mAddMenuItem( menu, &linkmnuitem_, true, false ); }
    else
	mResetMenuItem( &linkmnuitem_ );
    
    mAddMenuItemCond( menu, &duplicatemnuitem_, true, false,
		      visserv_->canDuplicate(displayid_) );

    const bool usehide =
       menu->getMenuType()==uiMenuHandler::fromScene() && !visserv_->isSoloMode();
    mAddMenuItemCond( menu, &hidemnuitem_, true, false, usehide );

    mAddMenuItem( menu, &removemnuitem_, !visserv_->isLocked(displayid_),false);
}


void uiODDisplayTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() || mnuid==-1 )
	return;

    if ( mnuid==lockmnuitem_.id )
    {
	menu->setIsHandled(true);
	visserv_->lock( displayid_, !visserv_->isLocked(displayid_) );
	updateLockPixmap( visserv_->isLocked(displayid_) );
	ODMainWin()->sceneMgr().updateStatusBar();
    }
    else if ( mnuid==duplicatemnuitem_.id )
    {
	menu->setIsHandled(true);
	int newid =visserv_->duplicateObject(displayid_,sceneID());
	if ( newid!=-1 )
	    uiODDisplayTreeItem::create( this, applMgr(), newid );
    }
    else if ( mnuid==removemnuitem_.id )
    {
	menu->setIsHandled(true);
	if ( askContinueAndSaveIfNeeded() )
	{
	    prepareForShutdown();

	    mDynamicCastGet( const visSurvey::SurveyObject*, so,
			     visserv_->getObject(displayid_) );
	    if ( ODMainWin()->colTabEd().getSurvObj() == so )
		ODMainWin()->colTabEd().setColTab( 0, mUdf(int), mUdf(int) );

	    visserv_->removeObject( displayid_, sceneID() );
	    parent_->removeChild( this );
	}
    }
    else if ( mnuid==addattribmnuitem_.id )
    {
	uiODDataTreeItem* newitem = addAttribItem();
	newitem->select();
	applMgr()->updateColorTable( newitem->displayID(),
				     newitem->attribNr() );
	menu->setIsHandled(true);
    }
    else if ( mnuid==displyhistgram_.id )
    {
	visserv_->displayMapperRangeEditForAttrbs( displayID() );
    }
    else if ( mnuid==hidemnuitem_.id )
    {
	menu->setIsHandled(true);
	visserv_->turnOn( displayid_, false );
	updateCheckStatus();
    }
    else if ( mnuid==linkmnuitem_.id )
    {
	uiMSG().message( "Not implemented yet" );
    }
}
