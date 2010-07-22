/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
 RCS:		$Id: uiodvw2dhor3dtreeitem.cc,v 1.3 2010-07-22 05:22:40 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uiodvw2dhor3dtreeitem.h"

#include "uiflatviewer.h"
#include "uiflatviewwin.h"
#include "uiempartserv.h"
#include "uigraphicsscene.h"
#include "uilistview.h"
#include "uirgbarraycanvas.h"
#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uivispartserv.h"
#include "pixmap.h"

#include "attribdatapack.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emobject.h"
#include "mpeengine.h"
#include "mouseevent.h"

#include "visvw2ddataman.h"
#include "visvw2dhorizon3d.h"


uiODVw2DHor3DParentTreeItem::uiODVw2DHor3DParentTreeItem()
    : uiODVw2DTreeItem( "Horizon 3D" )
{
}


uiODVw2DHor3DParentTreeItem::~uiODVw2DHor3DParentTreeItem()
{
    applMgr()->EMServer()->tempobjAdded.remove(
	    mCB(this,uiODVw2DHor3DParentTreeItem,tempObjAddedCB) );
}


bool uiODVw2DHor3DParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Load ..."), 0 );
    handleSubMenu( mnu.exec() );
    return true;
}


bool uiODVw2DHor3DParentTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid == 0 )
    {
	TypeSet<MultiID> sortedmids;
	EM::EMM().sortedHorizonsList( sortedmids, true );

	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectHorizons( objs, false );

	for ( int idx=0; idx<objs.size(); idx++ )
	    addChild( new uiODVw2DHor3DTreeItem(objs[idx]->id()), false, false);

	deepUnRef( objs );
    }

    return true;
}


bool uiODVw2DHor3DParentTreeItem::init()
{
    applMgr()->EMServer()->tempobjAdded.notify(
	    mCB(this,uiODVw2DHor3DParentTreeItem,tempObjAddedCB) );

    return true;
}


void uiODVw2DHor3DParentTreeItem::tempObjAddedCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj ) return;

    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj);
    if ( !hor3d ) return;

    if ( findChild(applMgr()->EMServer()->getName(emid)) )
	return;

    addChild( new uiODVw2DHor3DTreeItem(emid), false, false);    
}


uiODVw2DHor3DTreeItem::uiODVw2DHor3DTreeItem( const EM::ObjectID& emid )
    : uiODVw2DTreeItem(0)
    , emid_(emid)
    , horview_(0)
    , oldactivevolupdated_(false)
{}


uiODVw2DHor3DTreeItem::~uiODVw2DHor3DTreeItem()
{
    NotifierAccess* deselnotify = horview_->deSelection();
    if ( deselnotify )
	deselnotify->remove( mCB(this,uiODVw2DHor3DTreeItem,deSelCB) );

    applMgr()->EMServer()->tempobjAbtToDel.remove(
	    mCB(this,uiODVw2DHor3DTreeItem,emobjAbtToDelCB) );

    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer( ivwr );
	MouseEventHandler* meh =
	    		&vwr.rgbCanvas().scene().getMouseEventHandler();
	meh->buttonReleased.remove(
		mCB(this,uiODVw2DHor3DTreeItem,musReleaseInVwrCB) );
	meh->buttonReleased.remove(
		mCB(this,uiODVw2DHor3DTreeItem,msRelEvtCompletedInVwrCB) );
    }

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
	emobj->change.remove( mCB(this,uiODVw2DHor3DTreeItem,emobjChangeCB) );

    viewer2D()->dataMgr()->removeObject( horview_ );
}


bool uiODVw2DHor3DTreeItem::init()
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return false;

    emobj->change.notify( mCB(this,uiODVw2DHor3DTreeItem,emobjChangeCB) );
    displayMiniCtab();

    name_ = applMgr()->EMServer()->getName( emid_ );
    uilistviewitem_->setCheckable(true);
    uilistviewitem_->setChecked( true );
    checkStatusChange()->notify( mCB(this,uiODVw2DHor3DTreeItem,checkCB) );

    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer( ivwr );
	MouseEventHandler* meh =
	    		&vwr.rgbCanvas().scene().getMouseEventHandler();
	meh->buttonReleased.notify(
		mCB(this,uiODVw2DHor3DTreeItem,musReleaseInVwrCB) );
    }

    horview_ = new Vw2DHorizon3D( emid_, viewer2D()->viewwin(),
	    			  viewer2D()->dataEditor() );
    horview_->setSelSpec( &viewer2D()->selSpec(true), true );
    horview_->setSelSpec( &viewer2D()->selSpec(false), false );
    horview_->draw();

    viewer2D()->dataMgr()->addObject( horview_ );

    NotifierAccess* deselnotify = horview_->deSelection();
    if ( deselnotify )
	deselnotify->notify( mCB(this,uiODVw2DHor3DTreeItem,deSelCB) );

    applMgr()->EMServer()->tempobjAbtToDel.notify(
	    mCB(this,uiODVw2DHor3DTreeItem,emobjAbtToDelCB) );

    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer( ivwr );
	MouseEventHandler* meh =
	    		&vwr.rgbCanvas().scene().getMouseEventHandler();
	meh->buttonReleased.notify(
		mCB(this,uiODVw2DHor3DTreeItem,msRelEvtCompletedInVwrCB) );
    }

    return true;
}


void uiODVw2DHor3DTreeItem::displayMiniCtab()
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );

    PtrMan<ioPixmap> pixmap = new ioPixmap( cPixmapWidth(), cPixmapHeight() );
    pixmap->fill( emobj->preferredColor() );

    uilistviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(), *pixmap );
}


void uiODVw2DHor3DTreeItem::emobjChangeCB( CallBacker* cb )
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


bool uiODVw2DHor3DTreeItem::select()
{
    if ( !uilistviewitem_->isSelected() )
	return false;

    viewer2D()->dataMgr()->setSelected( horview_ );
    horview_->selected();
    
    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer( ivwr );
	MouseEventHandler* meh =
	    		&vwr.rgbCanvas().scene().getMouseEventHandler();
	meh->buttonReleased.remove(
		mCB(this,uiODVw2DHor3DTreeItem,msRelEvtCompletedInVwrCB) );
    }


    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer( ivwr );
	MouseEventHandler* meh =
	    		&vwr.rgbCanvas().scene().getMouseEventHandler();
	meh->buttonReleased.notify(
		mCB(this,uiODVw2DHor3DTreeItem,msRelEvtCompletedInVwrCB) );
    }


    return true;
}


bool uiODVw2DHor3DTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Remove ..."), 0 );

    if ( mnu.exec() == 0 )
    {
	parent_->removeChild( this );
    }

    return true;
}


void uiODVw2DHor3DTreeItem::checkCB( CallBacker* )
{
    horview_->enablePainting( isChecked() );
}


void uiODVw2DHor3DTreeItem::deSelCB( CallBacker* )
{
    //TODO handle on/off MOUSEEVENT
}


void uiODVw2DHor3DTreeItem::updateSelSpec( const Attrib::SelSpec* selspec,
					   bool wva )
{
    horview_->setSelSpec( selspec, wva );
}


void uiODVw2DHor3DTreeItem::updateCS( const CubeSampling& cs, bool upd )
{
    if ( upd )
	horview_->setCubeSampling( cs, upd );
}


void uiODVw2DHor3DTreeItem::emobjAbtToDelCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj ) return;

    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj);
    if ( !hor3d ) return;

    if ( emid != emid_ ) return;

    parent_->removeChild( this );
}


void uiODVw2DHor3DTreeItem::musReleaseInVwrCB( CallBacker* )
{
    if ( !uilistviewitem_->isSelected() || !horview_ )
	return;

    if ( !viewer2D()->viewwin()->nrViewers() )
	return;

    horview_->setSeedPicking( applMgr()->visServer()->isPicking() );
    horview_->setTrackerSetupActive(
	    		applMgr()->visServer()->isTrackingSetupActive() );

    if ( !applMgr()->visServer()->isPicking() )
	return;

    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer( 0 );
    const FlatDataPack* fdp = vwr.pack( true );
    if ( !fdp )
	fdp = vwr.pack( false );
    if ( !fdp ) return;

    mDynamicCastGet(const Attrib::Flat3DDataPack*,dp3d,fdp);
    if ( !dp3d ) return;

    if ( MPE::engine().activeVolume() != dp3d->cube().cubeSampling() )
    {
	applMgr()->visServer()->updateOldActiVolInuiMPEMan();
	oldactivevolupdated_ = true;
    }

    applMgr()->visServer()->turnQCPlaneOff();
}


void uiODVw2DHor3DTreeItem::msRelEvtCompletedInVwrCB( CallBacker* )
{
    if ( !uilistviewitem_->isSelected() || !horview_ )
	return;

    if ( !viewer2D()->viewwin()->nrViewers() )
	return;

    if ( oldactivevolupdated_ )
       applMgr()->visServer()->restoreActiveVolInuiMPEMan();

    oldactivevolupdated_ = false;
}
