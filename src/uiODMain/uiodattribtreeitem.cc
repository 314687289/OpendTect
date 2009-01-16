/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		Jul 2003
___________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodattribtreeitem.cc,v 1.24 2009-01-16 05:48:11 cvssatyaki Exp $";

#include "uiodattribtreeitem.h"

#include "attribsel.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "filepath.h"
#include "ioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "ptrman.h"
#include "survinfo.h"

#include "uiattribpartserv.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"
#include "vissurvobj.h"
#include "vissurvscene.h"



const char* uiODAttribTreeItem::sKeySelAttribMenuTxt()
{ return "Select &Attribute"; }


const char* uiODAttribTreeItem::sKeyColSettingsMenuTxt()
{ return "Save &Color Settings"; }


uiODAttribTreeItem::uiODAttribTreeItem( const char* parenttype )
    : uiODDataTreeItem( parenttype )
    , selattrmnuitem_( sKeySelAttribMenuTxt() )
    , colsettingsmnuitem_( sKeyColSettingsMenuTxt() )
{}


uiODAttribTreeItem::~uiODAttribTreeItem()
{}


bool uiODAttribTreeItem::anyButtonClick( uiListViewItem* item )
{
    if ( item!=uilistviewitem_ )
	return uiTreeItem::anyButtonClick( item );

    if ( !select() ) return false;

    uiVisPartServer* visserv = applMgr()->visServer();
    if ( !visserv->canSetColTabSequence( displayID() ) &&
	 !visserv->getColTabMapperSetup( displayID(), attribNr() ) )
	return false;
//  if ( !visserv->isClassification( displayID(), attribNr() ) )
    applMgr()->modifyColorTable( displayID(), attribNr() );

    return true;
}


// TODO: get depthdomain key from scene
#define mCreateDepthDomMnuItemIfNeeded( is2d, needext ) \
{\
    if ( scene && scene->getDataTransform() )\
    {\
	subitem = attrserv->zDomainAttribMenuItem( *as,\
				scene->getZDomainKey(), is2d, needext );\
	mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );\
    }\
}


#define mCreateItemsList( is2d, needext ) \
{ \
    subitem = attrserv->storedAttribMenuItem( *as, is2d ); \
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked ); \
    subitem = attrserv->calcAttribMenuItem( *as, is2d, needext ); \
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked ); \
    subitem = attrserv->nlaAttribMenuItem( *as, is2d, needext ); \
    if ( subitem && subitem->nrItems() ) \
	mAddMenuItem( &mnu, subitem, true, subitem->checked ); \
    mCreateDepthDomMnuItemIfNeeded( is2d, needext ); \
}

void uiODAttribTreeItem::createSelMenu( MenuItem& mnu, int visid, int attrib,
					int sceneid)
{
    const uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( visid, attrib );
    if ( as && visserv->hasAttrib(visid) )
    {
	uiAttribPartServer* attrserv = ODMainWin()->applMgr().attrServer();
	mDynamicCastGet(visSurvey::SurveyObject*,so,visserv->getObject(visid));
	if ( !so ) return;
	Pol2D3D p2d3d = so->getAllowedDataType();
	mDynamicCastGet(visSurvey::Scene*,scene,visserv->getObject(sceneid))

	MenuItem* subitem;
	attrserv->resetMenuItems();
	bool need2dlist = SI().has2D() && p2d3d != Only3D;
	bool need3dlist = SI().has3D() && p2d3d != Only2D;
	if ( need2dlist )
	    mCreateItemsList( true, need3dlist );
	if ( need3dlist )
	    mCreateItemsList( false, need2dlist );
    }
}


void uiODAttribTreeItem::createMenuCB( CallBacker* cb )
{
    const uiVisPartServer* visserv = applMgr()->visServer();

    mDynamicCastGet(uiMenuHandler*,menu,cb);

    selattrmnuitem_.removeItems();
    createSelMenu( selattrmnuitem_, displayID(), attribNr(), sceneID() );

    if ( selattrmnuitem_.nrItems() )
	mAddMenuItem( menu, &selattrmnuitem_,
		      !visserv->isLocked(displayID()), false );

    const uiAttribPartServer* attrserv = applMgr()->attrServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(), attribNr() );
    if ( as && attrserv->getIOObj(*as) )
	mAddMenuItem( menu, &colsettingsmnuitem_, true, false );
    
    uiODDataTreeItem::createMenuCB( cb );
}


void uiODAttribTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDataTreeItem::handleMenuCB( cb );

    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid == colsettingsmnuitem_.id )
    {
	menu->setIsHandled(true);
	applMgr()->saveDefColTab( displayID(), attribNr() );
    }
    else if ( handleSelMenu( mnuid, displayID(), attribNr()) )
    {
	menu->setIsHandled(true);
	updateColumnText( uiODSceneMgr::cNameColumn() );
    }
}


bool uiODAttribTreeItem::handleSelMenu( int mnuid, int visid, int attrib )
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    if ( mnuid==-1 || visserv->isLocked(visid) )
	return false;

    const Attrib::SelSpec* as = visserv->getSelSpec( visid, attrib );
    if ( !as ) return false;

    uiAttribPartServer* attrserv = ODMainWin()->applMgr().attrServer();

    Attrib::SelSpec myas( *as );
    if ( attrserv->handleAttribSubMenu(mnuid,myas) )
    {
	visserv->setSelSpec( visid, attrib, myas );
	visserv->calculateAttrib( visid, attrib, false );
	return true;
    }

    return false;
}


BufferString uiODAttribTreeItem::createDisplayName( int visid, int attrib )
{
    const uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( visid, attrib );
    BufferString dispname( as ? as->userRef() : 0 );
    if ( as && as->isNLA() )
    {
	dispname = as->objectRef();
	const char* nodenm = as->userRef();
	if ( IOObj::isKey(as->userRef()) )
	    nodenm = IOM().nameOf( as->userRef(), false );
	dispname += " ("; dispname += nodenm; dispname += ")";
    }

    if ( as && as->id()==Attrib::SelSpec::cAttribNotSel() )
	dispname = "<right-click>";
    else if ( !as )
	dispname = visserv->getObjectName( visid );
    else if ( as->id() == Attrib::SelSpec::cNoAttrib() )
	dispname="";

    return dispname;
}


BufferString uiODAttribTreeItem::createDisplayName() const
{
    return createDisplayName( displayID(), attribNr() );
}


void uiODAttribTreeItem::updateColumnText( int col )
{
    if ( col==uiODSceneMgr::cColorColumn() )
    {
	uiVisPartServer* visserv = applMgr()->visServer();
	mDynamicCastGet(visSurvey::SurveyObject*,so,
			visserv->getObject( displayID() ))
	if ( !so )
	{
	    uiODDataTreeItem::updateColumnText( col );
	    return;
	}
	
	displayMiniCtab( so->getColTabSequence(attribNr()) );
    }

    uiODDataTreeItem::updateColumnText( col );
}
