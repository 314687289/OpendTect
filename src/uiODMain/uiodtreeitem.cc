/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		Jul 2003
 RCS:		$Id: uiodtreeitem.cc,v 1.77 2005-04-06 10:54:40 cvsnanne Exp $
___________________________________________________________________

-*/

#include "uiodtreeitemimpl.h"

#include "attribsel.h"
#include "errh.h"
#include "ptrman.h"
#include "ioobj.h"
#include "ioman.h"
#include "uimenu.h"
#include "pickset.h"
#include "survinfo.h"
#include "uilistview.h"
#include "uibinidtable.h"
#include "uivismenu.h"
#include "uisoviewer.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uimsg.h"
#include "uigeninputdlg.h"
#include "uivisemobj.h"
#include "uiempartserv.h"
#include "uiwellpropdlg.h"
#include "uivispartserv.h"
#include "uiwellpartserv.h"
#include "uipickpartserv.h"
#include "uiwellattribpartserv.h"
#include "uiattribpartserv.h"
#include "uimpepartserv.h"
#include "uiseispartserv.h"
#include "uislicesel.h"
#include "uipickszdlg.h"
#include "uicolor.h"

#include "visrandomtrackdisplay.h"
#include "vissurvwell.h"
#include "vissurvpickset.h"
#include "vissurvscene.h"
#include "visplanedatadisplay.h"
#include "uiexecutor.h"
#include "settings.h"
#include "emhorizon.h"
#include "emfault.h"


const char* uiODTreeTop::sceneidkey = "Sceneid";
const char* uiODTreeTop::viewerptr = "Viewer";
const char* uiODTreeTop::applmgrstr = "Applmgr";
const char* uiODTreeTop::scenestr = "Scene";

uiODTreeTop::uiODTreeTop( uiSoViewer* sovwr, uiListView* lv, uiODApplMgr* am,
			    uiTreeFactorySet* tfs_ )
    : uiTreeTopItem(lv)
    , tfs(tfs_)
{
    setProperty<int>( sceneidkey, sovwr->sceneId() );
    setPropertyPtr( viewerptr, sovwr );
    setPropertyPtr( applmgrstr, am );

    tfs->addnotifier.notify( mCB(this,uiODTreeTop,addFactoryCB) );
    tfs->removenotifier.notify( mCB(this,uiODTreeTop,removeFactoryCB) );
}


uiODTreeTop::~uiODTreeTop()
{
    tfs->addnotifier.remove( mCB(this,uiODTreeTop,addFactoryCB) );
    tfs->removenotifier.remove( mCB(this,uiODTreeTop,removeFactoryCB) );
}


int uiODTreeTop::sceneID() const
{
    int sceneid=-1;
    getProperty<int>( sceneidkey, sceneid );
    return sceneid;
}


bool uiODTreeTop::select(int selkey)
{
    applMgr()->visServer()->setSelObjectId(selkey);
    return true;
}


uiODApplMgr* uiODTreeTop::applMgr()
{
    void* res = 0;
    getPropertyPtr( applmgrstr, res );
    return reinterpret_cast<uiODApplMgr*>( res );
}


uiODTreeItem::uiODTreeItem( const char* name__ )
    : uiTreeItem( name__ )
{}


uiODApplMgr* uiODTreeItem::applMgr()
{
    void* res = 0;
    getPropertyPtr( uiODTreeTop::applmgrstr, res );
    return reinterpret_cast<uiODApplMgr*>( res );
}


uiSoViewer* uiODTreeItem::viewer()
{
    void* res = 0;
    getPropertyPtr( uiODTreeTop::viewerptr, res );
    return reinterpret_cast<uiSoViewer*>( res );
}


int uiODTreeItem::sceneID() const
{
    int sceneid=-1;
    getProperty<int>( uiODTreeTop::sceneidkey, sceneid );
    return sceneid;
}


void uiODTreeTop::addFactoryCB(CallBacker* cb)
{
    mCBCapsuleUnpack(int,idx,cb);
    addChild( tfs->getFactory(idx)->create() );
}


void uiODTreeTop::removeFactoryCB(CallBacker* cb)
{
    mCBCapsuleUnpack(int,idx,cb);
    PtrMan<uiTreeItem> dummy = tfs->getFactory(idx)->create();
    const uiTreeItem* child = findChild( dummy->name() );
    if ( children.indexOf(child)==-1 )
	return;

    removeChild( const_cast<uiTreeItem*>(child) );
}


#define mDisplayInit( inherited, creationfunc, checkfunc ) \
\
    if ( displayid==-1 ) \
    {	\
	displayid = applMgr()->visServer()->creationfunc; \
	if ( displayid==-1 ) \
	{\
	    return false;\
	}\
    } \
    else if ( !applMgr()->visServer()->checkfunc ) \
	return false;  \
\
    if ( !inherited::init() ) \
	return false; \



#define mParentShowSubMenu( creation ) \
    uiPopupMenu mnu( getUiParent(), "Action" ); \
    mnu.insertItem( new uiMenuItem("Add"), 0 ); \
    const int mnuid = mnu.exec(); \
    if ( mnuid==0 ) \
	creation; \
 \
    return true; \


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
	    treeitem->addChild( res );
	    return true;
	}
    }

    return false;
}

	

uiODDisplayTreeItem::uiODDisplayTreeItem( )
    : uiODTreeItem(0)
    , displayid(-1)
    , visserv(ODMainWin()->applMgr().visServer())
{
}


uiODDisplayTreeItem::~uiODDisplayTreeItem( )
{
    uiVisMenu* menu = visserv->getMenu( displayid, false );
    if ( menu )
    {
	menu->createnotifier.remove(mCB(this,uiODDisplayTreeItem,createMenuCB));
	menu->handlenotifier.remove(mCB(this,uiODDisplayTreeItem,handleMenuCB));
    }
}


int uiODDisplayTreeItem::selectionKey() const { return displayid; }


bool uiODDisplayTreeItem::init()
{
    if ( !uiTreeItem::init() ) return false;

    visserv->setSelObjectId( displayid );
    uilistviewitem->setChecked( visserv->isOn(displayid) );
    uilistviewitem->stateChanged.notify( mCB(this,uiODDisplayTreeItem,checkCB));

    name_ = createDisplayName();

    uiVisMenu* menu = visserv->getMenu( displayid, true );
    menu->createnotifier.notify( mCB(this,uiODDisplayTreeItem,createMenuCB) );
    menu->handlenotifier.notify( mCB(this,uiODDisplayTreeItem,handleMenuCB) );

    return true;
}


void uiODDisplayTreeItem::updateColumnText( int col )
{
    if ( !col )
	name_ = createDisplayName();

    uiTreeItem::updateColumnText(col);
}


bool uiODDisplayTreeItem::showSubMenu()
{
    return applMgr()->visServer()->showMenu(displayid);
}


void uiODDisplayTreeItem::checkCB(CallBacker*)
{
    applMgr()->visServer()->turnOn( displayid, uilistviewitem->isChecked() );
}


int uiODDisplayTreeItem::uiListViewItemType() const
{
    return uiListViewItem::CheckBox;
}


BufferString uiODDisplayTreeItem::createDisplayName() const
{
    const uiVisPartServer* cvisserv = const_cast<uiODDisplayTreeItem*>(this)->
							applMgr()->visServer();
    const AttribSelSpec* as = cvisserv->getSelSpec( displayid );
    BufferString dispname( as ? as->userRef() : 0 );
    if ( as && as->isNLA() )
    {
	dispname = as->objectRef();
	const char* nodenm = as->userRef();
	if ( IOObj::isKey(as->userRef()) )
	    nodenm = IOM().nameOf( as->userRef(), false );
	dispname += " ("; dispname += nodenm; dispname += ")";
    }

    if ( as && as->id()==AttribSelSpec::attribNotSel )
	dispname = "<right-click>";
    else if ( !as )
	dispname = cvisserv->getObjectName(displayid);
    else if ( as->id()==AttribSelSpec::noAttrib )
	dispname="";

    return dispname;
}



const char* uiODDisplayTreeItem::attrselmnutxt = "Select Attribute";


void uiODDisplayTreeItem::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiVisMenu*,menu,cb)
    if ( visserv->hasAttrib(displayid) )
    {
	uiAttribPartServer* attrserv = applMgr()->attrServer();
	const AttribSelSpec& as = *visserv->getSelSpec(displayid);
	uiPopupMenu* selattrmnu = new uiPopupMenu( menu->getParent(),
						   attrselmnutxt);
	storedstartid = menu->getFreeID();
	storedstopid = storedstartid;
	uiPopupMenu* stored =
	    	attrserv->createStoredCubesSubMenu( storedstopid, as );
	selattrmnu->insertItem( stored );
	for ( int idx=storedstartid; idx<=storedstopid; idx++ )
	    menu->getFreeID();

	attrstartid = menu->getFreeID();
	attrstopid = attrstartid;
	uiPopupMenu* attrib = 
	    	attrserv->createAttribSubMenu( attrstopid, as );
	selattrmnu->insertItem( attrib );
	for ( int idx=attrstartid; idx<=attrstopid; idx++ )
	    menu->getFreeID();

	nlastartid = menu->getFreeID();
	nlastopid = nlastartid;
	uiPopupMenu* nla = 
	    	attrserv->createNLASubMenu( nlastopid, as );
	if ( nla )
	{
	    selattrmnu->insertItem( nla );
	    for ( int idx=nlastartid; idx<=nlastopid; idx++ )
		menu->getFreeID();
	}

	menu->addSubMenu(selattrmnu,9999);
    }
    else
	storedstartid = storedstopid = attrstartid = attrstopid = 
	nlastartid = nlastopid = -1;

    duplicatemnuid = visserv->canDuplicate(displayid)
		    ? menu->addItem( new uiMenuItem("Duplicate") ) 
		    : -1;

    TypeSet<int> sceneids;
    visserv->getChildIds(-1,sceneids);
    bool doshare = false;
    Settings::common().getYN( IOPar::compKey("dTect","Share elements"), 
	    		      doshare );
    if ( doshare && sceneids.size()>1 )
    {
	sharefirstmnuid = menu->getCurrentID();
	uiPopupMenu* sharemnu = new uiPopupMenu( menu->getParent(),
						 "Share with...");
	for ( int idx=0; idx<sceneids.size(); idx++ )
	{
	    if ( sceneids[idx]!=sceneID() )
	    {
		uiMenuItem* itm =
		    	new uiMenuItem(visserv->getObjectName(sceneids[idx]));
		sharemnu->insertItem( itm, menu->getFreeID() );
	    }
	}

	menu->addSubMenu( sharemnu );

	sharelastmnuid = menu->getCurrentID()-1;
    }
    else
    {
	sharefirstmnuid = -1;
	sharelastmnuid = -1;
    }

    removemnuid = menu->addItem( new uiMenuItem("Remove"), -1000 );
    
}


void uiODDisplayTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiVisMenu*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() ) return;
    if ( mnuid==duplicatemnuid )
    {
	menu->setIsHandled(true);
	int newid =visserv->duplicateObject(displayid,sceneID());
	if ( newid!=-1 )
	    uiODDisplayTreeItem::create( this, applMgr(), newid );
    }
    else if ( mnuid==removemnuid )
    {
	menu->setIsHandled(true);
	prepareForShutdown();
	visserv->removeObject( displayid, sceneID() );
	parent->removeChild( this );
    }
    else if ( mnuid>=storedstartid && mnuid<=storedstopid )
    {
	menu->setIsHandled(true);
	handleAttribSubMenu( mnuid-storedstartid, 0 );
    }
    else if ( mnuid>=attrstartid && mnuid<=attrstopid )
    {
	menu->setIsHandled(true);
	handleAttribSubMenu( mnuid-attrstartid, 1 );
    }
    else if ( mnuid>=nlastartid && mnuid<=nlastopid )
    {
	menu->setIsHandled(true);
	handleAttribSubMenu( mnuid-nlastartid, 2 );
    }
    else if ( mnuid>=sharefirstmnuid && mnuid<=sharelastmnuid )
    {
	menu->setIsHandled(true);
	TypeSet<int> sceneids;
	visserv->getChildIds(-1,sceneids);
	int idy=0;
	for ( int idx=0; idx<mnuid-sharefirstmnuid; idx++ )
	{
	    if ( sceneids[idx]!=sceneID() )
	    {
		idy++;
		if ( idy==mnuid-sharefirstmnuid )
		{
		    visserv->shareObject(sceneids[idx], displayid);
		    break;
		}
	    }
	}
    }
}


void uiODDisplayTreeItem::handleAttribSubMenu( int mnuid, int type )
{
    const AttribSelSpec* as = visserv->getSelSpec( displayid );
    AttribSelSpec myas( *as );
    if ( applMgr()->attrServer()->handleAttribSubMenu(mnuid,type,myas) )
    {
	visserv->setSelSpec( displayid, myas );
	visserv->resetColorDataType( displayid );
	visserv->calculateAttrib( displayid, false );
	updateColumnText(0);
    }
}



uiODEarthModelSurfaceTreeItem::uiODEarthModelSurfaceTreeItem(
						const MultiID& mid_ )
    : mid(mid_)
    , uivisemobj(0)
{}


uiODEarthModelSurfaceTreeItem::~uiODEarthModelSurfaceTreeItem()
{ 
    delete uivisemobj;
}


#define mDelRet { delete uivisemobj; uivisemobj = 0; return false; }


bool uiODEarthModelSurfaceTreeItem::init()
{
    delete uivisemobj;
    if ( displayid!=-1 )
    {
	uivisemobj = new uiVisEMObject( getUiParent(), displayid, visserv );
	if ( !uivisemobj->isOK() )
	    mDelRet;

	mid = *visserv->getMultiID(displayid);
    }
    else
    {
	uivisemobj = new uiVisEMObject( getUiParent(), mid, sceneID(), visserv);
	displayid = uivisemobj->id();
	if ( !uivisemobj->isOK() )
	    mDelRet;
    }

    if ( !uiODDisplayTreeItem::init() )
	return false; 

    return true;
}


void uiODEarthModelSurfaceTreeItem::checkCB( CallBacker* cb )
{
    uiODDisplayTreeItem::checkCB(cb);

    const int trackerid = applMgr()->mpeServer()->getTrackerID(mid);
    if ( trackerid==-1 )
    {
	prevtrackstatus = false;
        return;
    }

    if ( uilistviewitem->isChecked() )
	applMgr()->mpeServer()->enableTracking(trackerid, prevtrackstatus);
    else
    {
	prevtrackstatus = applMgr()->mpeServer()->isTrackingEnabled(trackerid);
	applMgr()->mpeServer()->enableTracking(trackerid,false);
    }
}


void uiODEarthModelSurfaceTreeItem::prepareForShutdown()
{
    uivisemobj->prepareForShutdown();
}


BufferString uiODEarthModelSurfaceTreeItem::createDisplayName() const
{
    const uiVisPartServer* cvisserv =
       const_cast<uiODEarthModelSurfaceTreeItem*>(this)->applMgr()->visServer();
    const AttribSelSpec* as = cvisserv->getSelSpec( displayid );
    bool hasattr = as && as->id() > -2;
    BufferString dispname;
    if ( hasattr )
    {
	dispname = uiODDisplayTreeItem::createDisplayName();
	dispname += " (";
    }
    dispname += cvisserv->getObjectName( displayid );
    if ( hasattr ) dispname += ")";
    return dispname;
}


#define mIsObject(typestr) \
    !strcmp(uivisemobj->getObjectType(displayid),typestr)

void uiODEarthModelSurfaceTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiVisMenu*,menu,cb);

    const AttribSelSpec* as = visserv->getSelSpec(displayid);
    multidatamnuid = depthvalmnuid = -1;
    uiPopupMenu* attrmnu = menu->getMenu( attrselmnutxt );
    if ( attrmnu )
    {
	multidatamnuid = attrmnu->insertItem( 
					new uiMenuItem("Surface data ...") );

	uiMenuItem* depthvalmnuitm = new uiMenuItem("Depth");
	depthvalmnuid = attrmnu->insertItem( depthvalmnuitm );
	depthvalmnuitm->setChecked( as->id()==AttribSelSpec::noAttrib );
    }

    tracksetupmnuid = toggletrackingmnuid = trackmnuid = -1;
    addsectionmnuid = extendsectionmnuid = relmnuid = savemnuid = -1;

    uiPopupMenu* trackmnu = menu->getMenu( uiVisEMObject::trackingmenutxt );
    if ( uilistviewitem->isChecked() && trackmnu )
    {
	uiMPEPartServer* mps = applMgr()->mpeServer();
	mps->setCurrentAttribDescSet( applMgr()->attrServer()->curDescSet() );

	EM::SectionID section = -1;
	if ( uivisemobj->nrSections()==1 )
	    section = uivisemobj->getSectionID(0);
	else if ( menu->getPath() )
	    section = uivisemobj->getSectionID( menu->getPath() );

	const Coord3& pickedpos = menu->getPickedPos();
	const bool hastracker = applMgr()->mpeServer()->getTrackerID(mid)>=0;

	if ( pickedpos.isDefined() && !hastracker &&
	     applMgr()->EMServer()->isFullResolution(mid) )
	{
	    trackmnuid = menu->getFreeID();
	    trackmnu->insertItem( new uiMenuItem("Start tracking ..."), 
		    		  trackmnuid );
	}
	else if ( hastracker && section != -1 )
	{
	    if ( mIsObject(EM::Horizon::typeStr()) )
	    {
		addsectionmnuid = menu->getFreeID();
		trackmnu->insertItem( new uiMenuItem("Add section ..."),
				      addsectionmnuid );
	    }

#ifdef __debug__
	    extendsectionmnuid = menu->getFreeID();
	    trackmnu->insertItem( new uiMenuItem("Extend section ..."),
		    		  extendsectionmnuid );
#endif

	    tracksetupmnuid = menu->getFreeID();
	    trackmnu->insertItem( new uiMenuItem("Change setup ..."),
		    		  tracksetupmnuid );

	    uiMenuItem* tracktogglemnuitem = new uiMenuItem("Enable tracking");
	    toggletrackingmnuid = menu->getFreeID();
	    trackmnu->insertItem( tracktogglemnuitem, toggletrackingmnuid );
	    const int trackerid = applMgr()->mpeServer()->getTrackerID( mid );
	    tracktogglemnuitem->setChecked(
		    applMgr()->mpeServer()->isTrackingEnabled(trackerid) );

	    if ( mIsObject(EM::Horizon::typeStr()) )
	    {
		relmnuid = menu->getFreeID();
		trackmnu->insertItem( new uiMenuItem("Relations ..."), 
				      relmnuid );
	    }
	}

	uiMenuItem* storemenuitem = new uiMenuItem( "Save" );
	savemnuid = menu->addItem( storemenuitem );
	storemenuitem->setEnabled( applMgr()->EMServer()->isChanged(mid) );
    }

    uiMenuItem* saveattritm = new uiMenuItem("Save attribute ...");
    saveattrmnuid = menu->addItem( saveattritm );
    saveattritm->setEnabled( as && as->id() >= 0 );
    

#ifdef __debug__
    reloadmnuid = menu->addItem( new uiMenuItem("Reload") );
#else
    reloadmnuid = -1;
#endif
}


void uiODEarthModelSurfaceTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiVisMenu*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    EM::SectionID sectionid = -1;
    if ( uivisemobj->nrSections()==1 )
	sectionid = uivisemobj->getSectionID(0);
    else if ( menu->getPath() )
	sectionid = uivisemobj->getSectionID( menu->getPath() );

    if ( mnuid==saveattrmnuid )
    {
	menu->setIsHandled(true);
	applMgr()->EMServer()->storeAuxData( mid, true );
    }
    else if ( mnuid==savemnuid )
    {
	menu->setIsHandled(true);
	applMgr()->EMServer()->storeObject( mid, false );
    }
    else if ( mnuid==trackmnuid )
    {
	menu->setIsHandled(true);
	if ( sectionid < 0 ) return;

	applMgr()->mpeServer()->addTracker( mid, menu->getPickedPos() );
    }
    else if ( mnuid==tracksetupmnuid )
    {
	menu->setIsHandled(true);
	applMgr()->mpeServer()->showSetupDlg( mid, sectionid );
    }
    else if ( mnuid==reloadmnuid )
    {
	menu->setIsHandled(true);
	uiTreeItem* parent__ = parent;

	applMgr()->visServer()->removeObject( displayid, sceneID() );
	delete uivisemobj; uivisemobj = 0;

	if ( !applMgr()->EMServer()->loadSurface(mid) )
	    return;

	uivisemobj = new uiVisEMObject( getUiParent(), mid, sceneID(), visserv);
	displayid = uivisemobj->id();
    }
    else if ( mnuid==depthvalmnuid )
    {
	menu->setIsHandled(true);
	uivisemobj->setDepthAsAttrib();
	updateColumnText(0);
    }
    else if ( mnuid == relmnuid )
    {	
	menu->setIsHandled(true);
	applMgr()->mpeServer()->showRelationsDlg( mid, sectionid );
    }
    else if ( mnuid==toggletrackingmnuid )
    {
	menu->setIsHandled(true);
	const int trackerid = applMgr()->mpeServer()->getTrackerID(mid);
	applMgr()->mpeServer()->enableTracking(trackerid,
		!applMgr()->mpeServer()->isTrackingEnabled(trackerid));
    }
    else if ( mnuid==addsectionmnuid )
    {
	menu->setIsHandled(true);
	//applMgr()->mpeServer()->addNewSection( mid );
    }
    else if ( mnuid==extendsectionmnuid )
    {
	menu->setIsHandled(true);
	//applMgr()->mpeServer()->extendSection( mid, sectionid );
    }
    else if ( mnuid==multidatamnuid )
    {
	menu->setIsHandled(true);
	const bool res = applMgr()->EMServer()->loadAuxData(mid);
	if ( !res ) return;
	uivisemobj->readAuxData();
	visserv->selectTexture( displayid, 0 );
	ODMainWin()->sceneMgr().updateTrees();
    }
}


uiTreeItem* uiODBodyTreeItemFactory::create( int visid, uiTreeItem*) const
{
    const char* objtype = uiVisEMObject::getObjectType(visid);
    return objtype && !strcmp(objtype, "Horizontal Tube")
	? new uiODBodyTreeItem(visid) : 0;
}


uiODBodyParentTreeItem::uiODBodyParentTreeItem()
    : uiODTreeItem("Bodies" )
{}



bool uiODBodyParentTreeItem::showSubMenu()
{
    /*
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("New"), 0 );
    mnu.insertItem( new uiMenuItem("Load ..."), 1);

    const int mnuid = mnu.exec();

    MultiID mid;
    bool success = false;
    
    if ( mnuid==0 )
    {
	success = applMgr()->EMServer()->createStickSet(mid);
    }
    else if ( mnuid==1 )
    {
	success = applMgr()->EMServer()->selectStickSet(mid);
    }

    if ( !success )
	return false;

    addChild( new uiODFaultStickTreeItem(mid) );

    */
    return true;
}


uiODBodyTreeItem::uiODBodyTreeItem( int displayid_ )
    : uivisemobj(0)
    , prevtrackstatus( false )
{ displayid = displayid_; }


uiODBodyTreeItem::uiODBodyTreeItem( const MultiID& mid_ )
    : mid(mid_)
    , uivisemobj(0)
    , prevtrackstatus( false )
{}


uiODBodyTreeItem::~uiODBodyTreeItem()
{ 
    delete uivisemobj;
}


bool uiODBodyTreeItem::init()
{
    delete uivisemobj;
    if ( displayid!=-1 )
    {
	uivisemobj = new uiVisEMObject( getUiParent(), displayid,
					applMgr()->visServer() );
	if ( !uivisemobj->isOK() ) { delete uivisemobj; return false; }

	mid = *applMgr()->visServer()->getMultiID(displayid);
    }
    else
    {
	uivisemobj = new uiVisEMObject( getUiParent(), mid, sceneID(),
					applMgr()->visServer() );
	if ( !uivisemobj->isOK() ) { delete uivisemobj; return false; }
	displayid = uivisemobj->id();
    }

    if ( !uiODDisplayTreeItem::init() )
	return false; 

    return true;
}


void uiODBodyTreeItem::checkCB( CallBacker* cb )
{
    uiODDisplayTreeItem::checkCB(cb);

    const int trackerid = applMgr()->mpeServer()->getTrackerID(mid);
    if ( trackerid==-1 )
    {
	prevtrackstatus = false;
        return;
    }

    if ( uilistviewitem->isChecked() )
	applMgr()->mpeServer()->enableTracking(trackerid, prevtrackstatus);
    else
    {
	prevtrackstatus = applMgr()->mpeServer()->isTrackingEnabled(trackerid);
	applMgr()->mpeServer()->enableTracking(trackerid,false);
    }
}


void uiODBodyTreeItem::prepareForShutdown()
{
    uivisemobj->prepareForShutdown();
}


void uiODBodyTreeItem::createMenuCB(CallBacker*)
{
}


void uiODBodyTreeItem::handleMenuCB(CallBacker*)
{
}


BufferString uiODBodyTreeItem::createDisplayName() const
{
    const uiVisPartServer* visserv =
       const_cast<uiODBodyTreeItem*>(this)->applMgr()->visServer();
    const AttribSelSpec* as = visserv->getSelSpec( displayid );
    bool hasattr = as && as->id() > -2;
    BufferString dispname;
    if ( hasattr )
    {
	dispname = uiODDisplayTreeItem::createDisplayName();
	dispname += " (";
    }
    dispname += visserv->getObjectName( displayid );
    if ( hasattr ) dispname += ")";
    return dispname;
}



/*
uiODFaultStickParentTreeItem::uiODFaultStickParentTreeItem()
    : uiODTreeItem("FaultSticks" )
{}


bool uiODFaultStickParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("New"), 0 );
    mnu.insertItem( new uiMenuItem("Load ..."), 1);

    const int mnuid = mnu.exec();

    MultiID mid;
    bool success = false;
    
    if ( mnuid==0 )
    {
	success = applMgr()->EMServer()->createStickSet(mid);
    }
    else if ( mnuid==1 )
    {
	success = applMgr()->EMServer()->selectStickSet(mid);
    }

    if ( !success )
	return false;

    addChild( new uiODFaultStickTreeItem(mid) );

    return true;
}


mMultiIDDisplayConstructor( FaultStick )

bool uiODFaultStickTreeItem::init()
{
    mMultiIDInit( addStickSet, isStickSet );
    return true;
}


bool uiODFaultStickTreeItem::showSubMenu()
{
    return uiODDisplayTreeItem::showSubMenu();
}

*/



uiTreeItem* uiODRandomLineTreeItemFactory::create( int visid, uiTreeItem*) const
{
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd, 
	    	    ODMainWin()->applMgr().visServer()->getObject(visid));
    return rtd ? new uiODRandomLineTreeItem(visid) : 0;
}



uiODRandomLineParentTreeItem::uiODRandomLineParentTreeItem()
    : uiODTreeItem( "Random line" )
{}


bool uiODRandomLineParentTreeItem::showSubMenu()
{
    mParentShowSubMenu( addChild(new uiODRandomLineTreeItem(-1)); );
}


uiODRandomLineTreeItem::uiODRandomLineTreeItem( int id )
{ displayid = id; } 


bool uiODRandomLineTreeItem::init()
{
    if ( displayid==-1 )
    {
	visSurvey::RandomTrackDisplay* rtd =
				    visSurvey::RandomTrackDisplay::create();
	displayid = rtd->id();
	visserv->addObject( rtd, sceneID(), true );
    }
    else
    {
	mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
			visserv->getObject(displayid));
	if ( !rtd ) return false;
    }

    return uiODDisplayTreeItem::init();
}


void uiODRandomLineTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiVisMenu*,menu,cb)
    editnodesmnuid = menu->addItem( new uiMenuItem("Edit nodes ...") );

    uiPopupMenu* insertnodemnu = new uiPopupMenu( menu->getParent(),
						  "Insert node");
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
	    	    visserv->getObject(displayid));

    for ( int idx=0; idx<=rtd->nrKnots(); idx++ )
    {
	BufferString nodename;
	if ( idx==rtd->nrKnots() )
	{
	    nodename = "after node ";
	    nodename += idx-1;
	}
	else
	{
	    nodename = "before node ";
	    nodename += idx;
	}

	const int mnuid = menu->getFreeID();
	uiMenuItem* itm = new uiMenuItem(nodename);
	insertnodemnu->insertItem( itm, mnuid );
	itm->setEnabled(rtd->canAddKnot(idx));
	if ( !idx )
	    insertnodemnuid = mnuid;
    }

    menu->addSubMenu(insertnodemnu);
}


void uiODRandomLineTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiVisMenu*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;
	
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
	    	    visserv->getObject(displayid));

    if ( mnuid==editnodesmnuid )
    {
	editNodes();
	menu->setIsHandled(true);
    }
    else if ( insertnodemnuid!=-1 && mnuid>=insertnodemnuid &&
	      mnuid<=insertnodemnuid+rtd->nrKnots() )
    {
	menu->setIsHandled(true);
	rtd->addKnot(mnuid-insertnodemnuid);
    }
}


void uiODRandomLineTreeItem::editNodes()
{
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
	    	    visserv->getObject(displayid));

    TypeSet<BinID> bidset;
    rtd->getAllKnotPos( bidset );
    uiBinIDTableDlg dlg( getUiParent(), "Specify nodes", bidset );
    if ( dlg.go() )
    {
	bool viewmodeswap = false;
	if ( visserv->isViewMode() )
	{
	    visserv->setViewMode( false );
	    viewmodeswap = true;
	}

	TypeSet<BinID> newbids;
	dlg.getBinIDs( newbids );
	if ( newbids.size() < 2 ) return;
	while ( rtd->nrKnots()>newbids.size() )
	    rtd->removeKnot( rtd->nrKnots()-1 );

	for ( int idx=0; idx<newbids.size(); idx++ )
	{
	    const BinID bid = newbids[idx];
	    if ( idx<rtd->nrKnots() )
		rtd->setManipKnotPos( idx, bid );
	    else
		rtd->addKnot( bid );
	}

	visserv->setSelObjectId( rtd->id() );
	visserv->calculateAttrib( rtd->id(), false );
	visserv->calculateColorAttrib( rtd->id(), false );
	if ( viewmodeswap ) visserv->setViewMode( true );
    }
}



uiODFaultParentTreeItem::uiODFaultParentTreeItem()
   : uiODTreeItem( "Fault" )
{}


bool uiODFaultParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("Load ..."), 0 );
    mnu.insertItem( new uiMenuItem("New ..."), 1 );

    MultiID mid;
    bool success = false;

    const int mnuid = mnu.exec();
    if ( mnuid == 0 )
	success = applMgr()->EMServer()->selectFault(mid);
    else if ( mnuid == 1 )
    {
	uiMPEPartServer* mps = applMgr()->mpeServer();
	mps->setCurrentAttribDescSet( applMgr()->attrServer()->curDescSet() );
	mps->startWizard( EM::Fault::typeStr(), 0 );
	return true;
    }

    if ( !success )
	return false;

    addChild( new uiODFaultTreeItem(mid) );

    return true;
}


uiTreeItem* uiODFaultTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    const char* objtype = uiVisEMObject::getObjectType(visid);
    return objtype && !strcmp(objtype, "Fault")
	? new uiODFaultTreeItem(visid) : 0;
}


uiODFaultTreeItem::uiODFaultTreeItem( const MultiID& mid_ )
    : uiODEarthModelSurfaceTreeItem( mid_ )
{}


uiODFaultTreeItem::uiODFaultTreeItem( int id )
    : uiODEarthModelSurfaceTreeItem( 0 )
{ displayid=id; }


uiODHorizonParentTreeItem::uiODHorizonParentTreeItem()
    : uiODTreeItem( "Horizon" )
{}


bool uiODHorizonParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("Load ..."), 0 );
    mnu.insertItem( new uiMenuItem("New ..."), 1 );

    MultiID mid;
    bool success = false;

    const int mnuid = mnu.exec();
    if ( !mnuid )
	success = applMgr()->EMServer()->selectHorizon(mid);
    else if ( mnuid == 1 )
    {
	uiMPEPartServer* mps = applMgr()->mpeServer();
	mps->setCurrentAttribDescSet( applMgr()->attrServer()->curDescSet() );
	mps->startWizard( EM::Horizon::typeStr(), 0 );
	return true;
    }

    if ( !success )
	return false;

    addChild( new uiODHorizonTreeItem(mid) );

    return true;
}


uiTreeItem* uiODHorizonTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    const char* objtype = uiVisEMObject::getObjectType(visid);
    return objtype && !strcmp(objtype, "Horizon")
	? new uiODHorizonTreeItem(visid) : 0;
}


uiODHorizonTreeItem::uiODHorizonTreeItem( const MultiID& mid_ )
    : uiODEarthModelSurfaceTreeItem( mid_ )
{}


uiODHorizonTreeItem::uiODHorizonTreeItem( int id )
    : uiODEarthModelSurfaceTreeItem( 0 )
{ displayid=id; }


void uiODHorizonTreeItem::updateColumnText( int col )
{
    if ( col==1 )
    {
	BufferString shift = uivisemobj->getShift();
	uilistviewitem->setText( shift, col );
	return;
    }

    return uiODDisplayTreeItem::updateColumnText( col );
}


uiODWellParentTreeItem::uiODWellParentTreeItem()
    : uiODTreeItem( "Well" )
{}


bool uiODWellParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("Add"), 0 );

    const int mnuid = mnu.exec();

    if ( mnuid<0 ) return false;

    ObjectSet<MultiID> emwellids;
    applMgr()->selectWells( emwellids );
    if ( !emwellids.size() )
	return false;

    for ( int idx=0; idx<emwellids.size(); idx++ )
	addChild( new uiODWellTreeItem(*emwellids[idx]) );

    deepErase( emwellids );
    return true;
}


uiTreeItem* uiODWellTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::WellDisplay*,wd, 
	    	    ODMainWin()->applMgr().visServer()->getObject(visid));
    return wd ? new uiODWellTreeItem(visid) : 0;
}


uiODWellTreeItem::uiODWellTreeItem( int did )
{ displayid=did; }


uiODWellTreeItem::uiODWellTreeItem( const MultiID& mid_ )
{ mid = mid_; }


bool uiODWellTreeItem::init()
{
    if ( displayid==-1 )
    {
	visSurvey::WellDisplay* wd = visSurvey::WellDisplay::create();
	displayid = wd->id();
	visserv->addObject(wd,sceneID(),true);
	if ( !wd->setWellId(mid) )
	{
	    visserv->removeObject(wd,sceneID());
	    return false;
	}
    }
    else
    {
	mDynamicCastGet(visSurvey::WellDisplay*,wd,
			visserv->getObject(displayid));
	if ( !wd )
	    return false;
    }

    return uiODDisplayTreeItem::init();
}
	    
	
void uiODWellTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiVisMenu*,menu,cb);
    mDynamicCastGet(visSurvey::WellDisplay*,wd,visserv->getObject(displayid))

    propertiesmnuid = menu->addItem( new uiMenuItem("Properties ...") );

    uiPopupMenu* showmnu = new uiPopupMenu( menu->getParent(), "Show" );

    uiMenuItem* wellnameitem = new uiMenuItem("Well name");
    namemnuid = menu->getFreeID();
    showmnu->insertItem( wellnameitem, namemnuid );
    wellnameitem->setChecked( wd->wellNameShown() );

    uiMenuItem* markeritem = new uiMenuItem("Markers");
    markermnuid = menu->getFreeID();
    showmnu->insertItem( markeritem, markermnuid );
    markeritem->setChecked( wd->markersShown() );
    markeritem->setEnabled( wd->canShowMarkers() );

    uiMenuItem* markernameitem = new uiMenuItem("Marker names");
    markernamemnuid = menu->getFreeID();
    showmnu->insertItem( markernameitem, markernamemnuid );
    markernameitem->setChecked( wd->canShowMarkers() && wd->markerNameShown() );
    markernameitem->setEnabled( wd->canShowMarkers() );

    uiMenuItem* logsmnuitem = new uiMenuItem("Logs");
    showlogmnuid = menu->getFreeID();
    showmnu->insertItem( logsmnuitem, showlogmnuid );
    logsmnuitem->setChecked( wd->logsShown() );
    logsmnuitem->setEnabled( applMgr()->wellServer()->hasLogs(wd->wellId()) );

    menu->addSubMenu( showmnu );

    selattrmnuid = menu->addItem(  new uiMenuItem("Select Attribute ...") );
    uiMenuItem* sellogmnuitem = new uiMenuItem("Select logs ...");
    sellogmnuid = menu->addItem( sellogmnuitem );
    sellogmnuitem->setEnabled( applMgr()->wellServer()->hasLogs(wd->wellId()) );
}


void uiODWellTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiVisMenu*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    mDynamicCastGet(visSurvey::WellDisplay*,wd,visserv->getObject(displayid))
    const MultiID& wellid = wd->wellId();
    if ( mnuid == selattrmnuid )
    {
	menu->setIsHandled(true);
	applMgr()->wellAttribServer()->setAttribSet( 
				*applMgr()->attrServer()->curDescSet() );
	applMgr()->wellAttribServer()->selectAttribute( wellid );
    }
    else if ( mnuid==sellogmnuid )
    {
	menu->setIsHandled(true);
	int selidx = -1;
	int lognr = 1;
	applMgr()->wellServer()->selectLogs( wellid, selidx, lognr );
	if ( selidx > -1 )
	    wd->displayLog(selidx,lognr,false);
    }
    else if ( mnuid == propertiesmnuid )
    {
	menu->setIsHandled(true);
	uiWellPropDlg dlg( getUiParent(), wd );
	dlg.go();
    }
    else if ( mnuid == namemnuid )
    {
	menu->setIsHandled(true);
	wd->showWellName( !wd->wellNameShown() );
    }
    else if ( mnuid == markermnuid )
    {
	menu->setIsHandled(true);
	wd->showMarkers( !wd->markersShown() );

    }
    else if ( mnuid == markernamemnuid )
    {
	menu->setIsHandled(true);
	wd->showMarkerName( !wd->markerNameShown() );
    }
    else if ( mnuid == showlogmnuid )
    {
	wd->showLogs( !wd->logsShown() );
    }
}


uiODPickSetParentTreeItem::uiODPickSetParentTreeItem()
    : uiODTreeItem( "PickSet" )
{}


bool uiODPickSetParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("New/Load ..."), 0 );
    if ( children.size() )
	mnu.insertItem( new uiMenuItem("Store ..."), 1);

    const int mnuid = mnu.exec();

    if ( mnuid<0 ) return false;

    if ( mnuid==0 )
    {
	if ( !applMgr()->pickServer()->fetchPickSets() ) return -1;
	PickSetGroup* psg = new PickSetGroup;
	applMgr()->getPickSetGroup( *psg );
	if ( psg->nrSets() )
	{
	    for ( int idx=0; idx<psg->nrSets(); idx++ )
	    {
		//TODO make sure it's not in list already
		addChild( new uiODPickSetTreeItem(psg->get(idx)) );
	    }
	}
	else
	{
	    PickSet pset( psg->name() );
	    pset.color = applMgr()->getPickColor();
	    addChild( new uiODPickSetTreeItem(&pset) );
	    //TODO create pickset
	}
    }
    else if ( mnuid==1 )
    {
	applMgr()->storePickSets();
    }

    return true;
}


uiTreeItem* uiODPickSetTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet( visSurvey::PickSetDisplay*, psd, 
	    	     ODMainWin()->applMgr().visServer()->getObject(visid));
    return psd ? new uiODPickSetTreeItem(visid) : 0;
}


uiODPickSetTreeItem::uiODPickSetTreeItem( const PickSet* ps_ )
    : ps( ps_ )
{}


uiODPickSetTreeItem::uiODPickSetTreeItem( int id )
{ displayid = id; }


bool uiODPickSetTreeItem::init()
{
    if ( displayid==-1 )
    {
	visSurvey::PickSetDisplay* psd = visSurvey::PickSetDisplay::create();
	displayid = psd->id();
	psd->copyFromPickSet(*ps);
	visserv->addObject(psd,sceneID(),true);
    }
    else
    {
	mDynamicCastGet( visSurvey::PickSetDisplay*, psd,
			 visserv->getObject(displayid) );
	if ( !psd )
	    return false;
    }

    return uiODDisplayTreeItem::init();
}


void uiODPickSetTreeItem::updateColumnText( int col )
{
    if ( col==1 )
    {
	mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
			visserv->getObject(displayid));
	BufferString text = psd->nrPicks();
	uilistviewitem->setText( text, col );
	return;
    }

    return uiODDisplayTreeItem::updateColumnText(col);
}




void uiODPickSetTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet( uiVisMenu*, menu, cb );

    renamemnuid = menu->addItem( new uiMenuItem("Rename ...") );
    storemnuid = menu->addItem( new uiMenuItem("Store ...") );
    dirmnuid = menu->addItem( new uiMenuItem("Set directions ...") );

    mDynamicCastGet( visSurvey::PickSetDisplay*, psd,
	    	     visserv->getObject(displayid));
    uiMenuItem* showallitem = new uiMenuItem("Show all");
    showallmnuid = menu->addItem( showallitem );
    showallitem->setChecked( psd->allShown() );

    propertymnuid = menu->addItem( new uiMenuItem("Properties ...") );
}


void uiODPickSetTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( uiVisMenu*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid==renamemnuid )
    {
	menu->setIsHandled(true);
	BufferString newname;
	const char* oldname = visserv->getObjectName( displayid );
	applMgr()->pickServer()->renamePickset( oldname, newname );
	visserv->setObjectName( displayid, newname );
    }
    else if ( mnuid==storemnuid )
    {
	menu->setIsHandled(true);
	mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
			visserv->getObject(displayid));
	PickSet* ps = new PickSet( psd->name() );
	psd->copyToPickSet( *ps );
	applMgr()->pickServer()->storeSinglePickSet( ps );
    }
    else if ( mnuid==dirmnuid )
    {
	menu->setIsHandled(true);
	applMgr()->setPickSetDirs( displayid );
    }
    else if ( mnuid==showallmnuid )
    {
	mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
			visserv->getObject(displayid));
	const bool showall = !psd->allShown();
	psd->showAll( showall );
	mDynamicCastGet( visSurvey::Scene*,scene,visserv->getObject(sceneID()));
	scene->filterPicks(0);
    }
    else if ( mnuid==propertymnuid )
    {
	mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
			applMgr()->visServer()->getObject(displayid))
	uiPickSizeDlg dlg( getUiParent(), psd );
	dlg.go();
    }

    updateColumnText(0);
    updateColumnText(1);
}



uiODPlaneDataTreeItem::uiODPlaneDataTreeItem( int did, int dim_ )
    : dim(dim_)
    , positiondlg(0)
{ displayid = did; }


uiODPlaneDataTreeItem::~uiODPlaneDataTreeItem()
{
    delete positiondlg;
}


bool uiODPlaneDataTreeItem::init()
{
    if ( displayid==-1 )
    {
	visSurvey::PlaneDataDisplay* pdd=visSurvey::PlaneDataDisplay::create();
	displayid = pdd->id();
	pdd->setType( (visSurvey::PlaneDataDisplay::Type) dim );
	visserv->addObject( pdd, sceneID(), true );
    }
    else
    {
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
			visserv->getObject(displayid));
	if ( !pdd ) return false;
    }

    return uiODDisplayTreeItem::init();
}


void uiODPlaneDataTreeItem::updateColumnText( int col )
{
    if ( col==1 )
    {
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
			visserv->getObject(displayid))
	BufferString text = pdd->getManipulationPos();
	uilistviewitem->setText( text, col );
	return;
    }

    return uiODDisplayTreeItem::updateColumnText(col);
}


void uiODPlaneDataTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiVisMenu*,menu,cb);

    positionmnuid = menu->addItem( new uiMenuItem("Position ...") );

    uiSeisPartServer* seisserv = applMgr()->seisServer();
    int type = menu->getMenuType();
    gathersstartid = gathersstopid = -1;
    if ( type == 1 )
    {
	gathersstartid = menu->getFreeID();
	gathersstopid = gathersstartid;
	uiPopupMenu* displaygathermnu = seisserv->
				    createStoredGathersSubMenu( gathersstopid );
	for ( int idx=gathersstartid; idx<=gathersstopid; idx++ )
	    menu->getFreeID();
	if ( displaygathermnu )
	    menu->addSubMenu(displaygathermnu,-500);
    }
}


void uiODPlaneDataTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiVisMenu*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid == positionmnuid )
    {
	menu->setIsHandled(true);
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
			visserv->getObject(displayid))
	delete positiondlg;
	const CubeSampling& sics = SI().sampling();
	positiondlg = new uiSliceSel( getUiParent(), pdd->getCubeSampling(),
				SI().sampling(),
				mCB(this,uiODPlaneDataTreeItem,updatePlanePos), 
				(uiSliceSel::Type)dim );
	positiondlg->windowClosed.notify( 
		mCB(this,uiODPlaneDataTreeItem,posDlgClosed) );
	positiondlg->go();
	applMgr()->enableSceneMenu( false );
    }
    else if ( mnuid>=gathersstartid && mnuid<=gathersstopid )
    {
	menu->setIsHandled(true);
	const Coord3 inlcrlpos = visserv->getMousePos(false);
	const BinID bid( (int)inlcrlpos.x, (int)inlcrlpos.y );
	applMgr()->seisServer()->handleGatherSubMenu( mnuid-gathersstartid,bid);
    }
}


void uiODPlaneDataTreeItem::posDlgClosed( CallBacker* )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
	    	    visserv->getObject(displayid))
    CubeSampling newcs = positiondlg->getCubeSampling();
    bool samepos = newcs == pdd->getCubeSampling();
    if ( positiondlg->uiResult() && !samepos )
    {
	pdd->setCubeSampling( newcs );
	pdd->resetManipulation();
	visserv->calculateAttrib( displayid, false );
	visserv->calculateColorAttrib( displayid, false );
	updateColumnText(0);
    }

    applMgr()->enableSceneMenu( true );
}


void uiODPlaneDataTreeItem::updatePlanePos( CallBacker* cb )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
	    	    visserv->getObject(displayid))
    mDynamicCastGet(uiSliceSel*,dlg,cb)
    if ( !dlg ) return;

    CubeSampling cs = dlg->getCubeSampling();
    pdd->setCubeSampling( cs );
    pdd->resetManipulation();
    visserv->calculateAttrib( displayid, false );
    visserv->calculateColorAttrib( displayid, false );
}


uiTreeItem* uiODInlineTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd, 
	    	    ODMainWin()->applMgr().visServer()->getObject(visid))
    return pdd && pdd->getType()==0 ? new uiODInlineTreeItem(visid) : 0;
}


uiODInlineParentTreeItem::uiODInlineParentTreeItem()
    : uiODTreeItem( "Inline" )
{ }


bool uiODInlineParentTreeItem::showSubMenu()
{
    mParentShowSubMenu( addChild( new uiODInlineTreeItem(-1)); );
}


uiODInlineTreeItem::uiODInlineTreeItem( int id )
    : uiODPlaneDataTreeItem( id, 0 )
{}


uiTreeItem* uiODCrosslineTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, 
	    	     ODMainWin()->applMgr().visServer()->getObject(visid));
    return pdd && pdd->getType()==1 ? new uiODCrosslineTreeItem(visid) : 0;
}


uiODCrosslineParentTreeItem::uiODCrosslineParentTreeItem()
    : uiODTreeItem( "Crossline" )
{ }


bool uiODCrosslineParentTreeItem::showSubMenu()
{
    mParentShowSubMenu( addChild( new uiODCrosslineTreeItem(-1)); );
}


uiODCrosslineTreeItem::uiODCrosslineTreeItem( int id )
    : uiODPlaneDataTreeItem( id, 1 )
{}


uiTreeItem* uiODTimesliceTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, 
	    	     ODMainWin()->applMgr().visServer()->getObject(visid));
    return pdd && pdd->getType()==2 ? new uiODTimesliceTreeItem(visid) : 0;
}


uiODTimesliceParentTreeItem::uiODTimesliceParentTreeItem()
    : uiODTreeItem( "Timeslice" )
{}


bool uiODTimesliceParentTreeItem::showSubMenu()
{
    mParentShowSubMenu( addChild( new uiODTimesliceTreeItem(-1)); );
}


uiODTimesliceTreeItem::uiODTimesliceTreeItem( int id )
    : uiODPlaneDataTreeItem( id, 2 )
{}


uiODSceneTreeItem::uiODSceneTreeItem( const char* name__, int displayid_ )
    : uiODTreeItem( name__ )
    , displayid( displayid_ )
{}


#define mAnnotText	0
#define mAnnotScale	1
#define mSurveyBox	2
#define mBackgroundCol	3
#define mDumpIV		4
#define mSubMnuSnapshot	5


bool uiODSceneTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet(visSurvey::Scene*,scene,visserv->getObject(displayid))

    const bool showcube = scene->isAnnotShown();
    uiMenuItem* anntxt = new uiMenuItem("Annotation text");
    mnu.insertItem( anntxt, mAnnotText );
    anntxt->setChecked( showcube && scene->isAnnotTextShown() );
    anntxt->setEnabled( showcube );

    uiMenuItem* annscale = new uiMenuItem("Annotation scale");
    mnu.insertItem( annscale, mAnnotScale );
    annscale->setChecked( showcube && scene->isAnnotScaleShown() );
    annscale->setEnabled( showcube );

    uiMenuItem* annsurv = new uiMenuItem("Survey box");
    mnu.insertItem( annsurv, mSurveyBox );
    annsurv->setChecked( showcube );

    mnu.insertItem( new uiMenuItem("Background color ..."), mBackgroundCol );


    bool yn = false;
    Settings::common().getYN( IOPar::compKey("dTect","Dump OI Menu"), yn );
    if ( yn )
	mnu.insertItem( new uiMenuItem("Dump OI ..."), mDumpIV );

    yn = true;
    Settings::common().getYN( IOPar::compKey("dTect","Enable snapshot"), yn );
    if ( yn )
	mnu.insertItem( new uiMenuItem("Save snapshot..."), mSubMnuSnapshot );

    const int mnuid=mnu.exec();
    if ( mnuid==mAnnotText )
	scene->showAnnotText( !scene->isAnnotTextShown() );
    else if ( mnuid==mAnnotScale )
	scene->showAnnotScale( !scene->isAnnotScaleShown() );
    else if ( mnuid==mSurveyBox )
	scene->showAnnot( !scene->isAnnotShown() );
    else if ( mnuid==mBackgroundCol )
    {
	Color col = viewer()->getBackgroundColor();
	if ( selectColor(col,getUiParent(),"Color selection",false) )
	    viewer()->setBackgroundColor( col );
    }
    else if ( mnuid==mDumpIV )
	visserv->dumpOI( displayid );
    else if ( mnuid==mSubMnuSnapshot )
	viewer()->renderToFile();

    return true;
}
