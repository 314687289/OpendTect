/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Mar 2008
 RCS:		$Id: uiodvw2dfaulttreeitem.cc,v 1.2 2010-06-25 06:12:03 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uiodvw2dfaulttreeitem.h"

#include "uiempartserv.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uiodviewer2d.h"

#include "emfault3d.h"
#include "emmanager.h"
#include "emobject.h"

#include "visvw2dfault.h"
#include "visvw2ddataman.h"


uiODVw2DFaultParentTreeItem::uiODVw2DFaultParentTreeItem()
    : uiODVw2DTreeItem( "Fault" )
{
}


uiODVw2DFaultParentTreeItem::~uiODVw2DFaultParentTreeItem()
{
    applMgr()->EMServer()->tempobjAdded.remove(
	    mCB(this,uiODVw2DFaultParentTreeItem,tempObjAddedCB) );
}


bool uiODVw2DFaultParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Load ..."), 0 );
    handleSubMenu( mnu.exec() );

    return true;
}


bool uiODVw2DFaultParentTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid == 0 )
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
    applMgr()->EMServer()->tempobjAdded.notify(
	    mCB(this,uiODVw2DFaultParentTreeItem,tempObjAddedCB) );

    return true;
}


void uiODVw2DFaultParentTreeItem::tempObjAddedCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj ) return;

    mDynamicCastGet(EM::Fault3D*,f3d,emobj);
    if ( !f3d ) return;

    if ( findChild(applMgr()->EMServer()->getName(emid)) )
	return;

    addChild( new uiODVw2DFaultTreeItem(emid),false,false);
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

    applMgr()->EMServer()->tempobjAbtToDel.remove(
	    mCB(this,uiODVw2DFaultTreeItem,emobjAbtToDelCB) );

    viewer2D()->dataMgr()->removeObject( faultview_ );
}


bool uiODVw2DFaultTreeItem::init()
{
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

    applMgr()->EMServer()->tempobjAbtToDel.notify(
	    mCB(this,uiODVw2DFaultTreeItem,emobjAbtToDelCB) );

    return true;
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
    mnu.insertItem( new uiMenuItem("&Remove ..."), 0 );

    if ( mnu.exec() == 0 )
    {
	parent_->removeChild( this );
    }

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

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj ) return;

    mDynamicCastGet(EM::Fault3D*,f3d,emobj);
    if ( !f3d ) return;

    if ( emid != emid_ ) return;

    parent_->removeChild( this );
}
