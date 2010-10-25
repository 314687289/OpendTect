/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
 RCS:		$Id: uiodvw2dhor2dtreeitem.cc,v 1.16 2010-10-25 09:41:57 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uiodvw2dhor2dtreeitem.h"

#include "uiattribpartserv.h"
#include "uiflatviewer.h"
#include "uiflatviewwin.h"
#include "uiempartserv.h"
#include "uigraphicsscene.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uimpepartserv.h"
#include "uirgbarraycanvas.h"
#include "uiodapplmgr.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uivispartserv.h"

#include "emhorizon2d.h"
#include "emmanager.h"
#include "emobject.h"
#include "ioman.h"
#include "ioobj.h"
#include "mouseevent.h"
#include "mpeengine.h"
#include "pixmap.h"

#include "visseis2ddisplay.h"
#include "visvw2ddataman.h"
#include "visvw2dhorizon2d.h"


uiODVw2DHor2DParentTreeItem::uiODVw2DHor2DParentTreeItem()
    : uiODVw2DTreeItem( "Horizon 2D" )
{
}


uiODVw2DHor2DParentTreeItem::~uiODVw2DHor2DParentTreeItem()
{
}


bool uiODVw2DHor2DParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&New ..."), 0 );
    mnu.insertItem( new uiMenuItem("&Load ..."), 1 );
    return handleSubMenu( mnu.exec() );
}


bool uiODVw2DHor2DParentTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid == 0 )
    {
	applMgr()->visServer()->reportTrackingSetupActive( true );
	uiMPEPartServer* mps = applMgr()->mpeServer();
	mps->setCurrentAttribDescSet(
				applMgr()->attrServer()->curDescSet(true) );
	mps->addTracker( EM::Horizon2D::typeStr(), -1 );

	const int trackid = mps->activeTrackerID();
	uiODVw2DHor2DTreeItem* hortreeitem = 
	    new uiODVw2DHor2DTreeItem( mps->getEMObjectID(trackid) );
	hortreeitem->createNewOne();
	addChild( hortreeitem, false, false );

    }
    else if ( mnuid == 1 )
    {
	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectHorizons( objs, true );

	for ( int idx=0; idx<objs.size(); idx++ )
	    addChild( new uiODVw2DHor2DTreeItem(objs[idx]->id()), false, false);

	deepUnRef( objs );
    }

    return true;
}


bool uiODVw2DHor2DParentTreeItem::init()
{
    return true;
}


void uiODVw2DHor2DParentTreeItem::tempObjAddedCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj ) return;

    mDynamicCastGet(EM::Horizon2D*,hor2d,emobj);
    if ( !hor2d ) return;

    if ( findChild(applMgr()->EMServer()->getName(emid)) )
	return;

    addChild( new uiODVw2DHor2DTreeItem(emid), false, false);
}


uiODVw2DHor2DTreeItem::uiODVw2DHor2DTreeItem( const EM::ObjectID& emid )
    : uiODVw2DTreeItem(0)
    , horview_(0)
    , emid_( emid )
    , creatednewone_(false)
{}


uiODVw2DHor2DTreeItem::~uiODVw2DHor2DTreeItem()
{
    NotifierAccess* deselnotify = horview_->deSelection();
    if ( deselnotify )
	deselnotify->remove( mCB(this,uiODVw2DHor2DTreeItem,deSelCB) );

    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer( ivwr );
	MouseEventHandler* meh =
	    		&vwr.rgbCanvas().scene().getMouseEventHandler();
	meh->buttonPressed.remove(
		mCB(this,uiODVw2DHor2DTreeItem,mousePressInVwrCB) );
	meh->buttonReleased.remove(
		mCB(this,uiODVw2DHor2DTreeItem,mouseReleaseInVwrCB) );
    }

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
    {
	emobj->change.remove( mCB(this,uiODVw2DHor2DTreeItem,emobjChangeCB) );

	if ( creatednewone_ )
	{
	    const int trackeridx =
		MPE::engine().getTrackerByObject( emobj->id() );
	    if ( trackeridx >= 0 )
		MPE::engine().removeTracker( trackeridx );
	    MPE::engine().removeEditor( emobj->id() );
	}
    }

    viewer2D()->dataMgr()->removeObject( horview_ );
}


bool uiODVw2DHor2DTreeItem::init()
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return false;

    emobj->change.notify( mCB(this,uiODVw2DHor2DTreeItem,emobjChangeCB) );
    displayMiniCtab();

    name_ = applMgr()->EMServer()->getName( emid_ );
    uilistviewitem_->setCheckable(true);
    uilistviewitem_->setChecked( true );
    checkStatusChange()->notify( mCB(this,uiODVw2DHor2DTreeItem,checkCB) );

    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer( ivwr );
	MouseEventHandler* meh = 
	    		&vwr.rgbCanvas().scene().getMouseEventHandler();
	meh->buttonPressed.notify(
		mCB(this,uiODVw2DHor2DTreeItem,mousePressInVwrCB) );
	meh->buttonReleased.notify(
		mCB(this,uiODVw2DHor2DTreeItem,mouseReleaseInVwrCB) );
    }

    horview_ = new Vw2DHorizon2D( emid_, viewer2D()->viewwin(),
	    			  viewer2D()->dataEditor() );
    horview_->setSelSpec( &viewer2D()->selSpec(true), true );
    horview_->setSelSpec( &viewer2D()->selSpec(false), false );
    
    if ( !viewer2D()->lineSetID().isEmpty() )
	horview_->setLineSetID( viewer2D()->lineSetID() );

    horview_->draw();
    viewer2D()->dataMgr()->addObject( horview_ );

    NotifierAccess* deselnotify = horview_->deSelection();
    if ( deselnotify )
	deselnotify->notify( mCB(this,uiODVw2DHor2DTreeItem,deSelCB) );

    return true;
}


void uiODVw2DHor2DTreeItem::createNewOne()
{
    creatednewone_ = true;
}


void uiODVw2DHor2DTreeItem::displayMiniCtab()
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );

    PtrMan<ioPixmap> pixmap = new ioPixmap( cPixmapWidth(), cPixmapHeight() );
    pixmap->fill( emobj->preferredColor() );
    uilistviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(), *pixmap );
}


void uiODVw2DHor2DTreeItem::emobjChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( const EM::EMObjectCallbackData&,
	    			cbdata, caller, cb );

    mDynamicCastGet(EM::EMObject*,emobject,caller);
    if ( !emobject ) return;

    switch( cbdata.event )
    {
	case EM::EMObjectCallbackData::Undef:
	    break;
	case EM::EMObjectCallbackData::PrefColorChange:
	    {
		displayMiniCtab();
		break;
	    }
	default: break;
    }
}


bool uiODVw2DHor2DTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    uiMenuItem* savemnu = new uiMenuItem("&Save ... ");
    mnu.insertItem( savemnu, 0 );
    savemnu->setEnabled( applMgr()->EMServer()->isChanged(emid_) &&
	   		 applMgr()->EMServer()->isFullyLoaded(emid_) );
    mnu.insertItem( new uiMenuItem("&Save As ..."), 1 );
    uiMenuItem* cngsetup = new uiMenuItem( "Change setup..." );
    mnu.insertItem( cngsetup, 2 );
    cngsetup->setEnabled( MPE::engine().getTrackerByObject(emid_) > -1 );
    mnu.insertItem( new uiMenuItem("&Remove"), 3 );

    applMgr()->mpeServer()->setCurrentAttribDescSet(
	    			applMgr()->attrServer()->curDescSet(false) );
    applMgr()->mpeServer()->setCurrentAttribDescSet(
	    			applMgr()->attrServer()->curDescSet(true) );

    const int mnuid = mnu.exec();
    if ( mnuid == 0 )
    {
	bool savewithname = EM::EMM().getMultiID( emid_ ).isEmpty();
	if ( !savewithname )
	{
	    PtrMan<IOObj> ioobj = IOM().get( EM::EMM().getMultiID(emid_) );
	    savewithname = !ioobj;
	}

	applMgr()->EMServer()->storeObject( emid_, savewithname );
	const MultiID mid = applMgr()->EMServer()->getStorageID(emid_);
	applMgr()->mpeServer()->saveSetup( mid );
	name_ = applMgr()->EMServer()->getName( emid_ );
	uiTreeItem::updateColumnText( uiODViewer2DMgr::cNameColumn() );
    }
    else if ( mnuid == 1 )
    {
	const MultiID oldmid = applMgr()->EMServer()->getStorageID(emid_);
	applMgr()->mpeServer()->prepareSaveSetupAs( oldmid );

	MultiID storedmid;
	applMgr()->EMServer()->storeObject( emid_, true, storedmid );
	name_ = applMgr()->EMServer()->getName( emid_ );

	const MultiID midintree = applMgr()->EMServer()->getStorageID(emid_);
	EM::EMM().getObject(emid_)->setMultiID( storedmid);
	applMgr()->mpeServer()->saveSetupAs( storedmid );
	EM::EMM().getObject(emid_)->setMultiID( midintree );

	uiTreeItem::updateColumnText( uiODViewer2DMgr::cNameColumn() );
    }
    else if ( mnuid == 2 )
    {
	EM::EMObject* emobj = EM::EMM().getObject( emid_ );
	if ( emobj )
	{
	    const int sectionid = emobj->sectionID( emobj->nrSections()-1 );
	    applMgr()->mpeServer()->showSetupDlg( emid_, sectionid );
	}
    }
    else if ( mnuid == 3 )
	parent_->removeChild( this );

    return true;
}


bool uiODVw2DHor2DTreeItem::select()
{
    if ( !uilistviewitem_->isSelected() )
	return false;

    viewer2D()->dataMgr()->setSelected( horview_ );
    horview_->selected( isChecked() );
    return true;
}


void uiODVw2DHor2DTreeItem::deSelCB( CallBacker* )
{
    //TODO handle on/off MOUSEEVENT
}


void uiODVw2DHor2DTreeItem::checkCB( CallBacker* )
{
    horview_->enablePainting( isChecked() );
}


void uiODVw2DHor2DTreeItem::updateSelSpec( const Attrib::SelSpec* selspec,
					   bool wva )
{
    horview_->setSelSpec( selspec, wva );
}


void uiODVw2DHor2DTreeItem::emobjAbtToDelCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj ) return;

    mDynamicCastGet(EM::Horizon2D*,hor2d,emobj);
    if ( !hor2d ) return;

    if ( emid != emid_ ) return;

    parent_->removeChild( this );
}


void uiODVw2DHor2DTreeItem::mousePressInVwrCB( CallBacker* )
{
    if ( !uilistviewitem_->isSelected() || !horview_ )
	return;

    horview_->setSeedPicking( applMgr()->visServer()->isPicking() );
    horview_->setTrackerSetupActive(
	    		applMgr()->visServer()->isTrackingSetupActive() );
}


void uiODVw2DHor2DTreeItem::mouseReleaseInVwrCB( CallBacker* )
{
}
