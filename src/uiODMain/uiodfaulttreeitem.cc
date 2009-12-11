/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	K. Tingdahl
 Date: 		Jul 2003
___________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodfaulttreeitem.cc,v 1.31 2009-12-11 15:35:52 cvsjaap Exp $";

#include "uiodfaulttreeitem.h"

#include "uimpepartserv.h"
#include "visfaultdisplay.h"
#include "visfaultsticksetdisplay.h"
#include "emfaultstickset.h"
#include "emfault3d.h"
#include "emmanager.h"
#include "ioman.h"
#include "ioobj.h"

#include "mousecursor.h"
#include "randcolor.h"
#include "selector.h"
#include "uiempartserv.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uivispartserv.h"


uiODFaultParentTreeItem::uiODFaultParentTreeItem()
   : uiODTreeItem( "Fault" )
{}

#define mLoadMnuID	0
#define mNewMnuID	1


bool uiODFaultParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Load ..."), mLoadMnuID );
    mnu.insertItem( new uiMenuItem("&New ..."), mNewMnuID );
    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid==mLoadMnuID )
    {
	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectFaults( objs, false );
	MouseCursorChanger uics( MouseCursor::Wait );
	for ( int idx=0; idx<objs.size(); idx++ )
	    addChild( new uiODFaultTreeItem(objs[idx]->id()), false );

	deepUnRef( objs );
    }
    else if ( mnuid == mNewMnuID )
    {
	//applMgr()->mpeServer()->saveUnsaveEMObject();
	RefMan<EM::EMObject> emo =
	    EM::EMM().createTempObject( EM::Fault3D::typeStr() );
	if ( !emo )
	    return false;

	emo->setPreferredColor( getRandomColor(false) );
	BufferString newname = "<New fault ";
	static int faultnr = 1;
	newname += faultnr++;
	newname += ">";
	emo->setName( newname.buf() );
	emo->setFullyLoaded( true );
	addChild( new uiODFaultTreeItem( emo->id() ), false );

	uiVisPartServer* visserv = applMgr()->visServer();
	visserv->showMPEToolbar();
	visserv->turnSeedPickingOn( true );
	return true;
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem* uiODFaultTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::FaultDisplay*,fd,
	    ODMainWin()->applMgr().visServer()->getObject(visid));
    return fd ? new uiODFaultTreeItem( visid, true ) : 0;
}


#define mCommonInit \
    , savemnuitem_("&Save") \
    , saveasmnuitem_("Save as ...") \
    , displaymnuitem_( "&Display ..." ) \
    , displayplanemnuitem_ ( "Fault &planes" ) \
    , displaystickmnuitem_ ( "Fault &sticks" ) \
    , displayintersectionmnuitem_( "&At sections only" ) \
    , singlecolmnuitem_( "Use single &color" ) \
    , removeselectedmnuitem_( "Re&move selection" )



uiODFaultTreeItem::uiODFaultTreeItem( const EM::ObjectID& oid )
    : uiODDisplayTreeItem()
    , emid_( oid )
    mCommonInit
{
    displayplanemnuitem_.checkable = true;
    displaystickmnuitem_.checkable = true;
    displayintersectionmnuitem_.checkable = true;
    singlecolmnuitem_.checkable = true;
}


uiODFaultTreeItem::uiODFaultTreeItem( int id, bool dummy )
    : uiODDisplayTreeItem()
    , emid_(-1)
    , faultdisplay_(0)
    mCommonInit
{
    displayplanemnuitem_.checkable = true;
    displaystickmnuitem_.checkable = true;
    displayintersectionmnuitem_.checkable = true;
    singlecolmnuitem_.checkable = true;
    displayid_ = id;
}


uiODFaultTreeItem::~uiODFaultTreeItem()
{
    if ( faultdisplay_ )
    {
	faultdisplay_->materialChange()->remove(
	    mCB(this,uiODFaultTreeItem,colorChCB));
	faultdisplay_->unRef();
    }
}


bool uiODFaultTreeItem::init()
{
    if ( displayid_==-1 )
    {
	visSurvey::FaultDisplay* fd = visSurvey::FaultDisplay::create();
	displayid_ = fd->id();
	faultdisplay_ = fd;
	faultdisplay_->ref();

	fd->setEMID( emid_ );
	visserv_->addObject( fd, sceneID(), true );
    }
    else
    {
	mDynamicCastGet( visSurvey::FaultDisplay*, fd,
			 visserv_->getObject(displayid_) );
	if ( !fd )
	    return false;

	faultdisplay_ = fd;
	faultdisplay_->ref();
	emid_ = fd->getEMID();
    }

    faultdisplay_->materialChange()->notify(
	    mCB(this,uiODFaultTreeItem,colorChCB));

    return uiODDisplayTreeItem::init();
}


void uiODFaultTreeItem::colorChCB( CallBacker* )
{
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


bool uiODFaultTreeItem::askContinueAndSaveIfNeeded()
{
    return applMgr()->EMServer()->askUserToSave( emid_ );
}


void uiODFaultTreeItem::prepareForShutdown()
{
    if ( faultdisplay_ )
    {
	faultdisplay_->materialChange()->remove(
	    mCB(this,uiODFaultTreeItem,colorChCB));
	faultdisplay_->unRef();
    }

    faultdisplay_ = 0;
}


void uiODFaultTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);

    if ( menu->menuID()!=displayID() )
	return;

    mDynamicCastGet(visSurvey::FaultDisplay*,fd,
	    ODMainWin()->applMgr().visServer()->getObject(displayID()));
    if ( !fd )
	return;

    mAddMenuItem( menu, &singlecolmnuitem_, faultdisplay_->arePanelsDisplayed(),
		  !faultdisplay_->showingTexture() );
    mAddMenuItem( &displaymnuitem_, &displayplanemnuitem_, true,
		  faultdisplay_->arePanelsDisplayed() );
    mAddMenuItem( &displaymnuitem_, &displaystickmnuitem_, true,
		  faultdisplay_->areSticksDisplayed() );
    mAddMenuItem( &displaymnuitem_, &displayintersectionmnuitem_, true,
		  faultdisplay_->areIntersectionsDisplayed() );
    mAddMenuItem( menu, &displaymnuitem_, true, true );

    const Selector<Coord3>* sel = visserv_->getCoordSelector( sceneID() );
    mAddMenuItem( menu, &removeselectedmnuitem_, sel, true );

    const bool enablesave = applMgr()->EMServer()->isChanged(emid_) &&
			    applMgr()->EMServer()->isFullyLoaded(emid_);

    mAddMenuItem( menu, &savemnuitem_, enablesave, false );
    mAddMenuItem( menu, &saveasmnuitem_, true, false );
}


void uiODFaultTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() || mnuid==-1 )
	return;

    if ( mnuid==saveasmnuitem_.id ||  mnuid==savemnuitem_.id )
    {
	menu->setIsHandled(true);
	bool saveas = mnuid==saveasmnuitem_.id ||
		      applMgr()->EMServer()->getStorageID(emid_).isEmpty();
	if ( !saveas )
	{
	    PtrMan<IOObj> ioobj =
		IOM().get( applMgr()->EMServer()->getStorageID(emid_) );
	    saveas = !ioobj;
	}

	applMgr()->EMServer()->storeObject( emid_, saveas );

	if ( saveas && faultdisplay_ &&
	     !applMgr()->EMServer()->getName(emid_).isEmpty() )
	{
	    faultdisplay_->setName( applMgr()->EMServer()->getName(emid_));
	    updateColumnText( uiODSceneMgr::cNameColumn() );
	}
    }
    else if ( mnuid==displayplanemnuitem_.id )
    {
	menu->setIsHandled(true);

	const bool stickchecked = displaystickmnuitem_.checked;
	const bool planechecked = displayplanemnuitem_.checked;
	faultdisplay_->display( stickchecked || planechecked, !planechecked );
	faultdisplay_->displayIntersections( false );
    }
    else if ( mnuid==displaystickmnuitem_.id )
    {
	menu->setIsHandled(true);
	const bool stickchecked = displaystickmnuitem_.checked;
	const bool planechecked = displayplanemnuitem_.checked;
	faultdisplay_->display( !stickchecked, stickchecked || planechecked );
	faultdisplay_->displayIntersections( false );
    }
    else if ( mnuid==displayintersectionmnuitem_.id )
    {
	menu->setIsHandled(true);
	const bool interchecked = displayintersectionmnuitem_.checked;
	faultdisplay_->display( false, interchecked );
	faultdisplay_->displayIntersections( !interchecked );
    }
    else if ( mnuid==singlecolmnuitem_.id )
    {
	menu->setIsHandled(true);
	faultdisplay_->useTexture( !faultdisplay_->showingTexture(), true );
	visserv_->triggerTreeUpdate();
    }
    else if ( mnuid==removeselectedmnuitem_.id )
    {
	//TODO: Should this code go as we have the trashcan?
	menu->setIsHandled(true);
	const Selector<Coord3>* sel = visserv_->getCoordSelector( sceneID() );
	if ( sel->isOK() )
	    faultdisplay_->removeSelection( *sel, 0 );
	else
	    uiMSG().error( "Invalid selection : self-intersecting polygon" );
    }
}



uiODFaultStickSetParentTreeItem::uiODFaultStickSetParentTreeItem()
   : uiODTreeItem( "FaultStickSet" )
{}


bool uiODFaultStickSetParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Load ..."), mLoadMnuID );
    mnu.insertItem( new uiMenuItem("&New ..."), mNewMnuID );
    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid==mLoadMnuID )
    {
	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectFaultStickSets( objs );
	MouseCursorChanger uics( MouseCursor::Wait );
	for ( int idx=0; idx<objs.size(); idx++ )
	    addChild( new uiODFaultStickSetTreeItem(objs[idx]->id()), false );
	deepUnRef( objs );
    }
    else if ( mnuid == mNewMnuID )
    {
	//applMgr()->mpeServer()->saveUnsaveEMObject();
	RefMan<EM::EMObject> emo =
	    EM::EMM().createTempObject( EM::FaultStickSet::typeStr() );
	if ( !emo )
	    return false;

	emo->setPreferredColor( getRandomColor(false) );
	BufferString newname = "<New sticks ";
	static int faultnr = 1;
	newname += faultnr++;
	newname += ">";
	emo->setName( newname.buf() );
	emo->setFullyLoaded( true );
	addChild( new uiODFaultStickSetTreeItem( emo->id() ), false );

	uiVisPartServer* visserv = applMgr()->visServer();
	visserv->showMPEToolbar();
	visserv->turnSeedPickingOn( true );
	return true;
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem* uiODFaultStickSetTreeItemFactory::create( int visid,
						      uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::FaultStickSetDisplay*,fd,
	    ODMainWin()->applMgr().visServer()->getObject(visid));
    return fd ? new uiODFaultStickSetTreeItem( visid, true ) : 0;
}


#undef mCommonInit
#define mCommonInit \
    , faultsticksetdisplay_(0) \
    , savemnuitem_("&Save") \
    , saveasmnuitem_("Save &as ...") \
    , removeselectedmnuitem_( "Re&move selection" ) \
    , onlyatsectmnuitem_("&Display only at sections") \
    , stickselmnuitem_("S&tick selection")


uiODFaultStickSetTreeItem::uiODFaultStickSetTreeItem( const EM::ObjectID& oid )
    : uiODDisplayTreeItem()
    , emid_( oid )
    mCommonInit
{
    onlyatsectmnuitem_.checkable = true;
    stickselmnuitem_.checkable = true;
}


uiODFaultStickSetTreeItem::uiODFaultStickSetTreeItem( int id, bool dummy )
    : uiODDisplayTreeItem()
    , emid_(-1)
    mCommonInit
{
    displayid_ = id;
    onlyatsectmnuitem_.checkable = true;
    stickselmnuitem_.checkable = true;
}


uiODFaultStickSetTreeItem::~uiODFaultStickSetTreeItem()
{
    if ( faultsticksetdisplay_ )
    {
	faultsticksetdisplay_->materialChange()->remove(
	    mCB(this,uiODFaultStickSetTreeItem,colorChCB) );
	faultsticksetdisplay_->unRef();
    }
}


bool uiODFaultStickSetTreeItem::init()
{
    if ( displayid_==-1 )
    {
	visSurvey::FaultStickSetDisplay* fd =
				    visSurvey::FaultStickSetDisplay::create();
	displayid_ = fd->id();
	faultsticksetdisplay_ = fd;
	faultsticksetdisplay_->ref();

	fd->setEMID( emid_ );
	visserv_->addObject( fd, sceneID(), true );
    }
    else
    {
	mDynamicCastGet(visSurvey::FaultStickSetDisplay*,fd,
			visserv_->getObject(displayid_));
	if ( !fd )
	    return false;

	faultsticksetdisplay_ = fd;
	faultsticksetdisplay_->ref();
	emid_ = fd->getEMID();
    }

    faultsticksetdisplay_->materialChange()->notify(
	    mCB(this,uiODFaultStickSetTreeItem,colorChCB) );

    return uiODDisplayTreeItem::init();
}


void uiODFaultStickSetTreeItem::colorChCB( CallBacker* )
{
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


bool uiODFaultStickSetTreeItem::askContinueAndSaveIfNeeded()
{
    return applMgr()->EMServer()->askUserToSave( emid_ );
}


void uiODFaultStickSetTreeItem::prepareForShutdown()
{
    if ( faultsticksetdisplay_ )
    {
	faultsticksetdisplay_->materialChange()->remove(
	    mCB(this,uiODFaultStickSetTreeItem,colorChCB) );
	faultsticksetdisplay_->unRef();
    }

    faultsticksetdisplay_ = 0;
}


void uiODFaultStickSetTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( menu->menuID()!=displayID() )
	return;

    mDynamicCastGet(visSurvey::FaultStickSetDisplay*,fd,
	    ODMainWin()->applMgr().visServer()->getObject(displayID()));
    if ( !fd )
	return;

    mAddMenuItem( menu, &stickselmnuitem_, true,
					     fd->isInStickSelectMode() );

    mAddMenuItem( menu, &onlyatsectmnuitem_, true,
					     fd->displayedOnlyAtSections() );

    const Selector<Coord3>* sel = visserv_->getCoordSelector( sceneID() );
    mAddMenuItem( menu, &removeselectedmnuitem_, sel, true );

    const bool enablesave = applMgr()->EMServer()->isChanged(emid_) &&
			    applMgr()->EMServer()->isFullyLoaded(emid_);
    mAddMenuItem( menu, &savemnuitem_, enablesave, false );
    mAddMenuItem( menu, &saveasmnuitem_, true, false );
}


void uiODFaultStickSetTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() || mnuid==-1 )
	return;

    mDynamicCastGet(visSurvey::FaultStickSetDisplay*,fd,
	    ODMainWin()->applMgr().visServer()->getObject(displayID()));

    if ( mnuid==stickselmnuitem_.id )
    {
	menu->setIsHandled(true);
	if ( fd )
	    fd->setStickSelectMode( !fd->isInStickSelectMode() );
    }
    else if ( mnuid==onlyatsectmnuitem_.id )
    {
	menu->setIsHandled(true);
	if ( fd )
	    fd->setDisplayOnlyAtSections( !fd->displayedOnlyAtSections() );
    }
    else if ( mnuid==saveasmnuitem_.id ||  mnuid==savemnuitem_.id )
    {
	menu->setIsHandled(true);
	bool saveas = mnuid==saveasmnuitem_.id ||
		      applMgr()->EMServer()->getStorageID(emid_).isEmpty();
	if ( !saveas )
	{
	    PtrMan<IOObj> ioobj =
		IOM().get( applMgr()->EMServer()->getStorageID(emid_) );
	    saveas = !ioobj;
	}

	applMgr()->EMServer()->storeObject( emid_, saveas );

	const BufferString emname = applMgr()->EMServer()->getName(emid_);
	if ( saveas && faultsticksetdisplay_ && !emname.isEmpty() )
	{
	    faultsticksetdisplay_->setName( emname );
	    updateColumnText( uiODSceneMgr::cNameColumn() );
	}
    }
    else if ( mnuid==removeselectedmnuitem_.id )
    {
	menu->setIsHandled(true);
	const Selector<Coord3>* sel = visserv_->getCoordSelector( sceneID() );
	if ( sel->isOK() )
	    faultsticksetdisplay_->removeSelection( *sel, 0 );
	else
	    uiMSG().error( "Invalid selection : self-intersecting polygon" );
    }
}
