/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	K. Tingdahl
 Date: 		May 2006
___________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodwelltreeitem.cc,v 1.55 2009-12-11 13:44:51 cvsbruno Exp $";

#include "uiodwelltreeitem.h"

#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uivispartserv.h"

#include "draw.h"

#include "uiattribpartserv.h"
#include "uicreateattriblogdlg.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodscenemgr.h"
#include "uiwellattribpartserv.h"
#include "uiwellpartserv.h"
#include "mousecursor.h"
#include "survinfo.h"
#include "wellman.h"
#include "welldata.h"

#include "viswelldisplay.h"


uiODWellParentTreeItem::uiODWellParentTreeItem()
    : uiODTreeItem( "Well" )
    , constlogsize_(true)
{
}


static int sLoadIdx	= 0;
static int sWellTieIdx	= 1;
static int sNewWellIdx	= 2;
static int sAttribIdx	= 3;
static int sLogDispSize = 4;

bool uiODWellParentTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
	    	    ODMainWin()->applMgr().visServer()->getObject(sceneID()));
    if ( scene && scene->getZAxisTransform() )
    {
	uiMSG().message( "Cannot add Wells to this scene" );
	return false;
    }

    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Load ..."), sLoadIdx );
    if ( SI().zIsTime()) 
	mnu.insertItem( new uiMenuItem("&Tie Well to Seismic ..."),sWellTieIdx);
    mnu.insertItem( new uiMenuItem("&New WellTrack ..."), sNewWellIdx );
    if ( children_.size() > 1 )
	mnu.insertItem( new uiMenuItem("&Create Attribute Log ..."),sAttribIdx);
    
    if ( children_.size() )
    {
	mnu.insertSeparator();
	uiMenuItem* szmenuitem = new uiMenuItem("Constant Log Size");
	mnu.insertItem( szmenuitem, sLogDispSize );
	szmenuitem->setCheckable( true );
	szmenuitem->setChecked( constlogsize_ );
    }

    if ( children_.size() > 1 )
    {
	mnu.insertSeparator( 40 );
	uiPopupMenu* showmnu = new uiPopupMenu( getUiParent(), "&Show all" );
	showmnu->insertItem( new uiMenuItem("Well names (&Top)"), 41 );
	showmnu->insertItem( new uiMenuItem("Well names (&Bottom)"), 42 );
	showmnu->insertItem( new uiMenuItem("&Markers"), 43 );
	showmnu->insertItem( new uiMenuItem("Marker &Names"), 44 );
	showmnu->insertItem( new uiMenuItem("&Logs"), 45 );
	mnu.insertItem( showmnu );

	uiPopupMenu* hidemnu = new uiPopupMenu( getUiParent(), "&Hide all" );
	hidemnu->insertItem( new uiMenuItem("Well names (&Top)"), 51 );
	hidemnu->insertItem( new uiMenuItem("Well names (&Bottom)"), 52 );
	hidemnu->insertItem( new uiMenuItem("&Markers"), 53 );
	hidemnu->insertItem( new uiMenuItem("Marker &Names"), 54 );
	hidemnu->insertItem( new uiMenuItem("&Logs"), 55 );
	mnu.insertItem( hidemnu );
    }
    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    return mnuid<0 ? false : handleSubMenu( mnuid );
}


#define mGetWellDisplayFromChild(childidx)\
    mDynamicCastGet(uiODWellTreeItem*,itm,children_[childidx]);\
    if ( !itm ) continue;\
    mDynamicCastGet(visSurvey::WellDisplay*,wd,\
		    visserv->getObject(itm->displayID()));\
    if ( !wd ) continue;
bool uiODWellParentTreeItem::handleSubMenu( int mnuid )
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    if ( mnuid == sLoadIdx )
    {
	ObjectSet<MultiID> emwellids;
	applMgr()->selectWells( emwellids );
	if ( emwellids.isEmpty() )
	    return false;

	for ( int idx=0; idx<emwellids.size(); idx++ )
	    addChild(new uiODWellTreeItem(*emwellids[idx]), false );

	deepErase( emwellids );
    }

    else if ( mnuid == sWellTieIdx )
    {
	 MultiID wid;
	 ODMainWin()->applMgr().wellAttribServer()->createD2TModel( wid );
    }

    else if ( mnuid == sNewWellIdx )
    {
	visSurvey::WellDisplay* wd = visSurvey::WellDisplay::create();
	wd->setupPicking(true);
	BufferString wellname;
	Color color;
	if ( !applMgr()->wellServer()->setupNewWell(wellname,color) )
	    return false;
	wd->setLineStyle( LineStyle(LineStyle::Solid,1,color) );
	wd->setName( wellname );
	visserv->addObject( wd, sceneID(), true );
	addChild( new uiODWellTreeItem(wd->id()), false );
    }

    else if ( mnuid == sAttribIdx )
    {
	ObjectSet<MultiID> wellids;
	BufferStringSet list;
	for ( int idx = 0; idx<children_.size(); idx++ )
	{
	    mGetWellDisplayFromChild( idx );
	    wellids += new MultiID( wd->getMultiID() );
	    list.add( children_[idx]->name() );
	}
	uiCreateAttribLogDlg dlg( getUiParent(), list, applMgr()->attrServer()->
				  curDescSet(false), ODMainWin()->applMgr().
				  wellAttribServer()->getNLAModel(), false );
	if (! dlg.go() )
	    return false;
	for ( int idx=0; idx<wellids.size(); idx++ )
	    ODMainWin()->applMgr().wellAttribServer()->createAttribLog(
		*wellids[idx], dlg.selectedLogIdx() );
    }

    else if ( mnuid == sLogDispSize )
    {
	bool allconst = false;
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mGetWellDisplayFromChild( idx );
	    const bool isconst = wd->logConstantSize();
	    if ( isconst )
	    { allconst = true; break; }
	}
	constlogsize_ = !allconst;
    }
    else if ( ( mnuid>40 && mnuid<46 ) || ( mnuid>50 && mnuid<56 ) )
    {
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mGetWellDisplayFromChild( idx );
	    switch ( mnuid )
	    {
		case 41: wd->showWellTopName( true ); break;
		case 42: wd->showWellBotName( true ); break;
		case 43: wd->showMarkers( true ); break;
		case 44: wd->showMarkerName( true ); break;
		case 45: wd->showLogs( true ); break;
		case 51: wd->showWellTopName( false ); break;
		case 52: wd->showWellBotName( false ); break;
		case 53: wd->showMarkers( false ); break;
		case 54: wd->showMarkerName( false ); break;
		case 55: wd->showLogs( false ); break;
	    }
	}
    }
    else
	handleStandardItems( mnuid );
	
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mGetWellDisplayFromChild( idx );
	wd->setLogConstantSize( constlogsize_ );
    }

    return true;
}


uiTreeItem* uiODWellTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::WellDisplay*,wd, 
		    ODMainWin()->applMgr().visServer()->getObject(visid));
    return wd ? new uiODWellTreeItem(visid) : 0;
}


uiODWellTreeItem::uiODWellTreeItem( int did )
{
    displayid_ = did;
    initMenuItems();
}


uiODWellTreeItem::uiODWellTreeItem( const MultiID& mid_ )
{
    mid = mid_;
    initMenuItems();
}


uiODWellTreeItem::~uiODWellTreeItem()
{
}


void uiODWellTreeItem::initMenuItems()
{
    propertiesmnuitem_.text = "&Properties ...";
    logviewermnuitem_.text = "&2D Log Viewer ...";
    gend2tm_.text = "&Tie Well to Seismic ...";
    nametopmnuitem_.text = "Well name (&Top)";
    namebotmnuitem_.text = "Well name (&Bottom)";
    markermnuitem_.text = "&Markers";
    markernamemnuitem_.text = "Marker &names";
    showlogmnuitem_.text = "&Logs" ;
    attrmnuitem_.text = "&Create attribute log...";
    showmnuitem_.text = "&Show" ;
    editmnuitem_.text = "&Edit Welltrack" ;
    storemnuitem_.text = "&Save";

    nametopmnuitem_.checkable = true;
    namebotmnuitem_.checkable = true;
    markermnuitem_.checkable = true;
    markernamemnuitem_.checkable = true;
    showlogmnuitem_.checkable = true;
    editmnuitem_.checkable = true;
}


bool uiODWellTreeItem::init()
{
    if ( displayid_==-1 )
    {
	visSurvey::WellDisplay* wd = visSurvey::WellDisplay::create();
	displayid_ = wd->id();
	visserv_->addObject( wd, sceneID(), true );
	if ( !wd->setMultiID(mid) )
	{
	    visserv_->removeObject( wd, sceneID() );
	    uiMSG().error("Could not load well");
	    return false;
	}
    }
    else
    {
	mDynamicCastGet(visSurvey::WellDisplay*,wd,
			visserv_->getObject(displayid_));
	if ( !wd )
	    return false;

	mDynamicCastGet(visSurvey::Scene*,scene,
			visserv_->getObject(sceneID()));
	if ( scene )
	    wd->setDisplayTransformation( scene->getUTM2DisplayTransform() );
    }

    return uiODDisplayTreeItem::init();
}
	    
	
void uiODWellTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( menu->menuID()!=displayID() )
	return;

    mDynamicCastGet(visSurvey::WellDisplay*,wd,visserv_->getObject(displayid_));
    const bool islocked = visserv_->isLocked( displayid_ );
    mAddMenuItem( menu, &propertiesmnuitem_, true, false );
    mAddMenuItem( menu, &logviewermnuitem_, true, false );
    if ( SI().zIsTime() )mAddMenuItem( menu, &gend2tm_, true, false );
    mAddMenuItem( menu, &attrmnuitem_, true, false );
    mAddMenuItem( menu, &editmnuitem_, !islocked, wd->isHomeMadeWell() );
    mAddMenuItem( menu, &storemnuitem_, wd->hasChanged(), false );
    mAddMenuItem( menu, &showmnuitem_, true, false );
    mAddMenuItem( &showmnuitem_, &nametopmnuitem_, true,  
	    					wd->wellTopNameShown() );
    mAddMenuItem( &showmnuitem_, &namebotmnuitem_, true,  
	    					wd->wellBotNameShown() );

    mAddMenuItem( &showmnuitem_, &markermnuitem_, wd->canShowMarkers(),
		 wd->markersShown() );
    mAddMenuItem( &showmnuitem_, &markernamemnuitem_, wd->canShowMarkers(),
		  wd->canShowMarkers() && wd->markerNameShown() );
    mAddMenuItem( &showmnuitem_, &showlogmnuitem_,
		  applMgr()->wellServer()->hasLogs(wd->getMultiID()), 
		  wd->logsShown() );
}


void uiODWellTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() || mnuid==-1 )
	return;

    mDynamicCastGet(visSurvey::WellDisplay*,wd,visserv_->getObject(displayid_))
    const MultiID& wellid = wd->getMultiID();
    if ( mnuid == attrmnuitem_.id )
    {
	menu->setIsHandled( true );
	//TODO false set to make it compile: change!
	applMgr()->wellAttribServer()->setAttribSet( 
				*applMgr()->attrServer()->curDescSet(false) );
	applMgr()->wellAttribServer()->createAttribLog( wellid, -1 );
    }
    else if ( mnuid == propertiesmnuitem_.id )
    {
	menu->setIsHandled( true );
	wd->restoreDispProp();
	ODMainWin()->applMgr().wellServer()->editDisplayProperties( wellid );
	updateColumnText( uiODSceneMgr::cColorColumn() );
    }
    else if ( mnuid == logviewermnuitem_.id )
    {
	menu->setIsHandled( true );
	ODMainWin()->applMgr().wellServer()->displayIn2DViewer( wellid );
	updateColumnText( uiODSceneMgr::cColorColumn() );
    }
    else if ( mnuid == nametopmnuitem_.id )
    {
	menu->setIsHandled( true );
	wd->showWellTopName( !wd->wellTopNameShown() );
    }
    else if ( mnuid == namebotmnuitem_.id )
    {
	menu->setIsHandled( true );
	wd->showWellBotName( !wd->wellBotNameShown() );
    }
    else if ( mnuid == markermnuitem_.id )
    {
	menu->setIsHandled( true );
	wd->showMarkers( !wd->markersShown() );
    }
    else if ( mnuid == markernamemnuitem_.id )
    {
	menu->setIsHandled( true );
	wd->showMarkerName( !wd->markerNameShown() );
    }
    else if ( mnuid == showlogmnuitem_.id )
    {
       	menu->setIsHandled( true );
	wd->showLogs( !wd->logsShown() );
    }
    else if ( mnuid == storemnuitem_.id )
    {
	menu->setIsHandled( true );
	const bool res = applMgr()->wellServer()->storeWell(
					wd->getWellCoords(), wd->name(), mid );
	if ( res )
	{
	    wd->setChanged( false );
	    wd->setMultiID( mid );
	}
    }
    else if ( mnuid == editmnuitem_.id )
    {
	menu->setIsHandled( true );
	const bool yn = wd->isHomeMadeWell();
	wd->setupPicking( !yn );
	if ( !yn )
	{
	    MouseCursorChanger cursorchgr( MouseCursor::Wait );
	    wd->showKnownPositions();
	}
    }
    else if ( mnuid == gend2tm_.id )
    {
	menu->setIsHandled( true );
	ODMainWin()->applMgr().wellAttribServer()->createD2TModel( wellid );
    }
}


bool uiODWellTreeItem::askContinueAndSaveIfNeeded()
{
    mDynamicCastGet(visSurvey::WellDisplay*,wd,visserv_->getObject(displayid_));
    if ( wd->hasChanged() )
    {
	BufferString warnstr = "This well has changed since the last save.\n";
	warnstr += "Do you want to save it?";
	int retval = uiMSG().askSave( warnstr.buf() );
	if ( !retval ) return true;
	else if ( retval == -1 ) return false;
	else
	    applMgr()->wellServer()->storeWell( wd->getWellCoords(),
		                                wd->name(), mid );
    }
    return true;
}
