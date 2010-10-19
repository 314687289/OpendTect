/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Mar 2008
 RCS:		$Id: uiodvw2dfaulttreeitem.cc,v 1.8 2010-10-19 05:54:37 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiodvw2dfaulttreeitem.h"

#include "uiempartserv.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"

#include "emfault3d.h"
#include "emmanager.h"
#include "emobject.h"
#include "ioman.h"
#include "ioobj.h"
#include "pixmap.h"
#include "randcolor.h"

#include "visvw2dfault.h"
#include "visvw2ddataman.h"


uiODVw2DFaultParentTreeItem::uiODVw2DFaultParentTreeItem()
    : uiODVw2DTreeItem( "Fault" )
{
}


uiODVw2DFaultParentTreeItem::~uiODVw2DFaultParentTreeItem()
{
}


bool uiODVw2DFaultParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&New"), 0 );
    mnu.insertItem( new uiMenuItem("&Load ..."), 1 );
    return handleSubMenu( mnu.exec() );
}


bool uiODVw2DFaultParentTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid == 0 )
    {
	RefMan<EM::EMObject> emo =
	    		EM::EMM().createTempObject( EM::Fault3D::typeStr() );
	if ( !emo )
	    return false;

	emo->setPreferredColor( getRandomColor(false) );
	emo->setNewName();
	emo->setFullyLoaded( true );
	addChild( new uiODVw2DFaultTreeItem(emo->id()), false, false );
    }
    else if ( mnuid == 1 )
    {
	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectFaults( objs, false );

	for ( int idx=0; idx<objs.size(); idx++ )
	    addChild( new uiODVw2DFaultTreeItem(objs[idx]->id()),false,false);

	deepUnRef( objs );
    }

    return true;
}


bool uiODVw2DFaultParentTreeItem::init()
{
    return true;
}


void uiODVw2DFaultParentTreeItem::tempObjAddedCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::Fault3D*,f3d,emobj);
    if ( !f3d ) return;

    if ( findChild(applMgr()->EMServer()->getName(emid)) )
	return;

    addChild( new uiODVw2DFaultTreeItem(emid), false, false );
}


uiODVw2DFaultTreeItem::uiODVw2DFaultTreeItem( const EM::ObjectID& emid )
    : uiODVw2DTreeItem(0)
    , emid_(emid)
    , faultview_(0)
{}


uiODVw2DFaultTreeItem::~uiODVw2DFaultTreeItem()
{
    NotifierAccess* deselnotify = faultview_->deSelection();
    if ( deselnotify )
	deselnotify->remove( mCB(this,uiODVw2DFaultTreeItem,deSelCB) );

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
	emobj->change.remove( mCB(this,uiODVw2DFaultTreeItem,emobjChangeCB) );

    viewer2D()->dataMgr()->removeObject( faultview_ );
}


bool uiODVw2DFaultTreeItem::init()
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return false;

    emobj->change.notify( mCB(this,uiODVw2DFaultTreeItem,emobjChangeCB) );
    displayMiniCtab();

    name_ = applMgr()->EMServer()->getName( emid_ );
    uilistviewitem_->setCheckable(true);
    uilistviewitem_->setChecked( true );
    checkStatusChange()->notify( mCB(this,uiODVw2DFaultTreeItem,checkCB) );

    faultview_ = new VW2DFaut( emid_, viewer2D()->viewwin(),
	    		       viewer2D()->dataEditor() );
    faultview_->draw();
    viewer2D()->dataMgr()->addObject( faultview_ );

    NotifierAccess* deselnotify = faultview_->deSelection();
    if ( deselnotify )
	deselnotify->notify( mCB(this,uiODVw2DFaultTreeItem,deSelCB) );

    return true;
}


void uiODVw2DFaultTreeItem::displayMiniCtab()
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );

    PtrMan<ioPixmap> pixmap = new ioPixmap( cPixmapWidth(), cPixmapHeight() );
    pixmap->fill( emobj->preferredColor() );
    uilistviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(), *pixmap );
}


void uiODVw2DFaultTreeItem::emobjChangeCB( CallBacker* cb )
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



bool uiODVw2DFaultTreeItem::select()
{
    if ( !uilistviewitem_->isSelected() )
	return false;

    viewer2D()->dataMgr()->setSelected( faultview_ );
    faultview_->selected();
    return true;
}


bool uiODVw2DFaultTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Save ..."), 0 );
    mnu.insertItem( new uiMenuItem("&Remove"), 1 );

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
	name_ = applMgr()->EMServer()->getName( emid_ );
	uiTreeItem::updateColumnText( uiODViewer2DMgr::cNameColumn() );
    }
    else if ( mnuid == 1 )
	parent_->removeChild( this );

    return true;
}


void uiODVw2DFaultTreeItem::updateCS( const CubeSampling& cs, bool upd )
{
    if ( upd )
	faultview_->draw();
}


void uiODVw2DFaultTreeItem::deSelCB( CallBacker* )
{
    //TODO handle on/off MOUSEEVENT
}


void uiODVw2DFaultTreeItem::checkCB( CallBacker* )
{
    faultview_->enablePainting( isChecked() );
}


void uiODVw2DFaultTreeItem::emobjAbtToDelCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );
    if ( emid != emid_ ) return;

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::Fault3D*,f3d,emobj);
    if ( !f3d ) return;

    parent_->removeChild( this );
}
