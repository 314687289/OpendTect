/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiodvw2dfaultss2dtreeitem.h"

#include "uiempartserv.h"
#include "uiflatviewstdcontrol.h"
#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uivispartserv.h"
#include "pixmap.h"

#include "emfaultstickset.h"
#include "emmanager.h"
#include "emobject.h"
#include "ioman.h"
#include "ioobj.h"
#include "randcolor.h"

#include "visseis2ddisplay.h"
#include "visvw2ddataman.h"
#include "visvw2dfaultss2d.h"


uiODVw2DFaultSS2DParentTreeItem::uiODVw2DFaultSS2DParentTreeItem()
    : uiODVw2DTreeItem( "FaultStickSet" )
{}


uiODVw2DFaultSS2DParentTreeItem::~uiODVw2DFaultSS2DParentTreeItem()
{
}


bool uiODVw2DFaultSS2DParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sNew(true)), 0 );
    mnu.insertItem( new uiAction(uiStrings::sLoad(true)), 1 );
    return handleSubMenu( mnu.exec() );
}


bool uiODVw2DFaultSS2DParentTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid == 0 )
    {
	RefMan<EM::EMObject> emo =
	    	EM::EMM().createTempObject( EM::FaultStickSet::typeStr() );
	if ( !emo )
	    return false;

	emo->setPreferredColor( getRandomColor(false) );
	emo->setNewName();
	emo->setFullyLoaded( true );
	uiODVw2DFaultSS2DTreeItem* treeitem =
	    new uiODVw2DFaultSS2DTreeItem( emo->id() );
	addChld( treeitem, false, false );
	viewer2D()->viewControl()->setEditMode( true );
	treeitem->select();
    }
    else if ( mnuid == 1 )
    {
	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectFaultStickSets( objs );
	for ( int idx=0; idx<objs.size(); idx++ )
	    addChld( new uiODVw2DFaultSS2DTreeItem(
					objs[idx]->id()), false, false);

	deepUnRef( objs );
    }

    return true;
}


bool uiODVw2DFaultSS2DParentTreeItem::init()
{
    return true;
}


void uiODVw2DFaultSS2DParentTreeItem::tempObjAddedCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::FaultStickSet*,fss,emobj);
    if ( !fss ) return;

    if ( findChild(applMgr()->EMServer()->getName(emid)) )
	return;

    addChld( new uiODVw2DFaultSS2DTreeItem(emid),false,false);
}



uiODVw2DFaultSS2DTreeItem::uiODVw2DFaultSS2DTreeItem( const EM::ObjectID& emid )
    : uiODVw2DTreeItem(0)
    , emid_(emid)
    , fssview_(0)
{
}


uiODVw2DFaultSS2DTreeItem::uiODVw2DFaultSS2DTreeItem( int id, bool )
    : uiODVw2DTreeItem(0)
    , emid_(-1)
    , fssview_(0)
{
    displayid_ = id;
}


uiODVw2DFaultSS2DTreeItem::~uiODVw2DFaultSS2DTreeItem()
{
    detachAllNotifiers();
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
	emobj->unRef();

    viewer2D()->dataMgr()->removeObject( fssview_ );
}


bool uiODVw2DFaultSS2DTreeItem::init()
{
    uitreeviewitem_->setCheckable(true);
    EM::EMObject* emobj = 0;
    if ( displayid_ < 0 )
    {
	emobj = EM::EMM().getObject( emid_ );
	if ( !emobj ) return false;

	fssview_ = VW2DFaultSS2D::create( emid_, viewer2D()->viewwin(),
					 viewer2D()->dataEditor() );
    }
    else
    {
	mDynamicCastGet(VW2DFaultSS2D*,fd,
			viewer2D()->dataMgr()->getObject(displayid_))
	if ( !fd )
	    return false;
	emid_ = fd->emID();
	emobj = EM::EMM().getObject( emid_ );
	if ( !emobj ) return false;

	fssview_ = fd;
    }
    emobj->ref();
    mAttachCB( emobj->change, uiODVw2DFaultSS2DTreeItem::emobjChangeCB );

    name_ = applMgr()->EMServer()->getName( emid_ );
    uitreeviewitem_->setChecked( true );
    mAttachCB( checkStatusChange(), uiODVw2DFaultSS2DTreeItem::checkCB );

    displayMiniCtab();

    if ( viewer2D()->geomID() != Survey::GeometryManager::cUndefGeomID() )
	fssview_->setGeomID( viewer2D()->geomID() );
    
    mAttachCB( fssview_->deSelection(), uiODVw2DFaultSS2DTreeItem::deSelCB );

    fssview_->draw();

    if ( displayid_ < 0 )
	viewer2D()->dataMgr()->addObject( fssview_ );

    mAttachCB( viewer2D()->viewControl()->editPushed(),
	       uiODVw2DFaultSS2DTreeItem::enableKnotsCB );

    return true;
}


void uiODVw2DFaultSS2DTreeItem::displayMiniCtab()
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );

    PtrMan<ioPixmap> pixmap = new ioPixmap( cPixmapWidth(), cPixmapHeight() );
    pixmap->fill( emobj->preferredColor() );
    uitreeviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(), *pixmap );
}


void uiODVw2DFaultSS2DTreeItem::emobjChangeCB( CallBacker* cb )
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


void uiODVw2DFaultSS2DTreeItem::enableKnotsCB( CallBacker* )
{
    if ( viewer2D()->dataMgr()->selectedId() == fssview_->id() )
	fssview_->selected();
}


bool uiODVw2DFaultSS2DTreeItem::select()
{
    uitreeviewitem_->setSelected( true);

    viewer2D()->dataMgr()->setSelected( fssview_ );
    fssview_->selected();
    return true;
}


bool uiODVw2DFaultSS2DTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    uiAction* savemnu = new uiAction(uiStrings::sSave(false));
    mnu.insertItem( savemnu, 0 );
    savemnu->setEnabled( applMgr()->EMServer()->isChanged(emid_) &&
	   		 applMgr()->EMServer()->isFullyLoaded(emid_) );
    mnu.insertItem( new uiAction( uiStrings::sSaveAs(true)), 1 );
    mnu.insertItem( new uiAction( uiStrings::sRemove(true)), 2 );

    const int mnuid = mnu.exec();
    if ( mnuid == 0 || mnuid == 1 )
    {
	bool savewithname = (mnuid == 1) ||
	    		    (EM::EMM().getMultiID( emid_ ).isEmpty());
	if ( !savewithname )
	{
	    PtrMan<IOObj> ioobj = IOM().get( EM::EMM().getMultiID(emid_) );
	    savewithname = !ioobj;
	}

	applMgr()->EMServer()->storeObject( emid_, savewithname );
	name_ = applMgr()->EMServer()->getName( emid_ );
	uiTreeItem::updateColumnText( uiODViewer2DMgr::cNameColumn() );
    }
    else if ( mnuid == 2 )
	parent_->removeChild( this );

    return true;
}


void uiODVw2DFaultSS2DTreeItem::deSelCB( CallBacker* )
{
    //TODO handle on/off MOUSEEVENT
}


void uiODVw2DFaultSS2DTreeItem::checkCB( CallBacker* )
{
    fssview_->enablePainting( isChecked() );
}


void uiODVw2DFaultSS2DTreeItem::emobjAbtToDelCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );
    if ( emid != emid_ ) return;

    EM::EMObject* emobj = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::FaultStickSet*,fss,emobj);
    if ( !fss ) return;

    parent_->removeChild( this );
}


uiTreeItem* uiODVw2DFaultSS2DTreeItemFactory::createForVis(
				const uiODViewer2D& vwr2d, int id ) const
{
    mDynamicCastGet(const VW2DFaultSS2D*,obj,vwr2d.dataMgr()->getObject(id));
    return obj ? new uiODVw2DFaultSS2DTreeItem(id,true) : 0;
}
