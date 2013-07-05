/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiodvw2dwigglevararea.h"

#include "uiattribpartserv.h"
#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uimenuhandler.h"
#include "uiodapplmgr.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uitaskrunner.h"
#include "uitreeview.h"
#include "filepath.h"
#include "ioobj.h"
#include "keystrs.h"

#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "flatposdata.h"
#include "linekey.h"
#include "visvw2dseismic.h"
#include "visvw2ddataman.h"


uiODVW2DWiggleVarAreaTreeItem::uiODVW2DWiggleVarAreaTreeItem()
    : uiODVw2DTreeItem( "Wiggle" )
    , dpid_(DataPack::cNoID())
    , dummyview_(0)
    , menu_(0)
    , selattrmnuitem_("Select &Attribute")
{}


uiODVW2DWiggleVarAreaTreeItem::~uiODVW2DWiggleVarAreaTreeItem()
{
    if ( viewer2D()->viewwin()->nrViewers() )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
	vwr.dataChanged.remove(
		mCB(this,uiODVW2DWiggleVarAreaTreeItem,dataChangedCB) );
    }

    if ( menu_ )
    {
	menu_->createnotifier.remove(
		mCB(this,uiODVW2DWiggleVarAreaTreeItem,createMenuCB) );
	menu_->handlenotifier.remove(
		mCB(this,uiODVW2DWiggleVarAreaTreeItem,handleMenuCB) );
	menu_->unRef();
    }
    
    viewer2D()->dataMgr()->removeObject( dummyview_ );
}


bool uiODVW2DWiggleVarAreaTreeItem::init()
{
    if ( !viewer2D()->viewwin()->nrViewers() )
	return false;

    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    const DataPack* fdpw = vwr.pack( true );
    if ( fdpw )
	dpid_ = fdpw->id();
    const FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;

    vwr.dataChanged.notify(
	    mCB(this,uiODVW2DWiggleVarAreaTreeItem,dataChangedCB) );

    uitreeviewitem_->setCheckable( vwr.isVisible(false) &&
	    			   viewer2D()->selSpec(true).id().isValid() );
    uitreeviewitem_->setChecked( ddp.wva_.show_ );

    checkStatusChange()->notify(
	    mCB(this,uiODVW2DWiggleVarAreaTreeItem,checkCB) );

    dummyview_ = new VW2DSeis();
    viewer2D()->dataMgr()->addObject( dummyview_ );

    return true;
}


bool uiODVW2DWiggleVarAreaTreeItem::select()
{
    if ( !uitreeviewitem_->isSelected() )
	return false;

    viewer2D()->dataMgr()->setSelected( dummyview_ );

    return true;
}


void uiODVW2DWiggleVarAreaTreeItem::checkCB( CallBacker* )
{
    const uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    if ( !vwr.pack(true) )
    {
	if ( !isChecked() ) return;
	const DataPack::ID dpid = viewer2D()->getDataPackID( true );
	if ( dpid != DataPack::cNoID() ) viewer2D()->setUpView( dpid, true );
	return;
    }

    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
	viewer2D()->viewwin()->viewer(ivwr).setVisible( true, isChecked() );
}


void uiODVW2DWiggleVarAreaTreeItem::dataChangedCB( CallBacker* )
{
    if ( !viewer2D()->viewwin()->nrViewers() )
	return;

    const uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    const DataPack* fdpw = vwr.pack( true );
    const FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
    uitreeviewitem_->setCheckable( vwr.isVisible(false) &&
				   viewer2D()->selSpec(true).id().isValid() );
    uitreeviewitem_->setChecked( ddp.wva_.show_ );
    if ( fdpw )	dpid_ = fdpw->id();
}


bool uiODVW2DWiggleVarAreaTreeItem::showSubMenu()
{
    if ( !menu_ )
    {
	menu_ = new uiMenuHandler( getUiParent(), -1 );
	menu_->ref();
	menu_->createnotifier.notify(
		mCB(this,uiODVW2DWiggleVarAreaTreeItem,createMenuCB) );
	menu_->handlenotifier.notify(
		mCB(this,uiODVW2DWiggleVarAreaTreeItem,handleMenuCB) );
    }
    return menu_->executeMenu(uiMenuHandler::fromTree());
}


void uiODVW2DWiggleVarAreaTreeItem::createMenuCB( CallBacker* cb )
{
    selattrmnuitem_.removeItems();
    createSelMenu( selattrmnuitem_ );

     mDynamicCastGet(MenuHandler*,menu,cb);
     if ( selattrmnuitem_.nrItems() )
	  mAddMenuItem( menu, &selattrmnuitem_, true, false );
}


void uiODVW2DWiggleVarAreaTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( handleSelMenu(mnuid) )
	menu->setIsHandled(true);
}


void uiODVW2DWiggleVarAreaTreeItem::createSelMenu( MenuItem& mnu )
{
    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    const DataPack* dp = vwr.pack( false );
    if ( !dp )
	dp = vwr.pack( true );
    if ( !dp ) return;
    
    mDynamicCastGet(const Attrib::Flat3DDataPack*,dp3d,dp);
    mDynamicCastGet(const Attrib::FlatRdmTrcsDataPack*,dprdm,dp);
    mDynamicCastGet(const Attrib::Flat2DDHDataPack*,dp2ddh,dp);

    const Attrib::SelSpec& as = viewer2D()->selSpec( true );
    MenuItem* subitem = 0;
    applMgr()->attrServer()->resetMenuItems();
    if ( dp3d || dprdm )
	subitem = applMgr()->attrServer()->storedAttribMenuItem(as,false,false);
    else if ( dp2ddh )
    {
	BufferString ln;
	dp2ddh->getLineName( ln );
	subitem = applMgr()->attrServer()->stored2DAttribMenuItem( as,
					viewer2D()->lineSetID(), ln, false );
    }
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );
    subitem = applMgr()->attrServer()->calcAttribMenuItem( as, dp2ddh, true );
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );
    if ( dp3d || dprdm )
	subitem = applMgr()->attrServer()->storedAttribMenuItem(as,false,true);
    else if ( dp2ddh )
    {
	BufferString ln;
	dp2ddh->getLineName( ln );
	subitem = applMgr()->attrServer()->stored2DAttribMenuItem( as,
					viewer2D()->lineSetID(), ln, true );
    }
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );
}


bool uiODVW2DWiggleVarAreaTreeItem::handleSelMenu( int mnuid )
{
    const Attrib::SelSpec& as = viewer2D()->selSpec( true );
    Attrib::SelSpec selas( as );

    uiAttribPartServer* attrserv = applMgr()->attrServer();

    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    const DataPack* dp = vwr.pack( true );
    if ( !dp )
	dp = vwr.pack( false );
    if ( !dp ) return false;

    DataPack::ID newid = DataPack::cNoID();

    mDynamicCastGet(const Attrib::Flat3DDataPack*,dp3d,dp);
    mDynamicCastGet(const Attrib::FlatRdmTrcsDataPack*,dprdm,dp);
    mDynamicCastGet(const Attrib::Flat2DDHDataPack*,dp2ddh,dp);


    bool dousemulticomp = false;
    if ( dp3d || dprdm )
    {
	if ( attrserv->handleAttribSubMenu(mnuid,selas,dousemulticomp) )
	{
	    attrserv->setTargetSelSpec( selas );
	    if ( dp3d )
		newid = attrserv->createOutput( dp3d->cube().cubeSampling(),
		    			    DataPack::cNoID() );
	    else
	    {
		const Interval<float> zrg(
			mCast(float,dprdm->posData().range(false).start),
			mCast(float,dprdm->posData().range(false).stop) );

		TypeSet<BinID> bids;
		if ( dprdm->pathBIDs() )
		    bids = *dprdm->pathBIDs();
		newid = attrserv->createRdmTrcsOutput( zrg, &bids, &bids );
	    }
	}
    }
    else if ( dp2ddh )
    {
	BufferString attrbnm; bool stored = false;
	bool steering = false;
	attrserv->info2DAttribSubMenu( mnuid, attrbnm, steering, stored );
	if ( attrbnm.isEmpty() )
	    attrbnm = LineKey::sKeyDefAttrib();

	BufferString ln;
	dp2ddh->getLineName( ln );

	uiTaskRunner uitr( &viewer2D()->viewwin()->viewer() );
	const CubeSampling cs = dp2ddh->getCubeSampling();
	const LineKey lk( ln.buf(), attrbnm );

	if ( !stored )
	{
	    if ( !attrserv->handleAttribSubMenu(mnuid,selas,dousemulticomp) )
		return false;

	    attrserv->setTargetSelSpec( selas );
	    newid = attrserv->create2DOutput( cs, lk, uitr );
	}
	else
	{
	    LineKey lky( viewer2D()->lineSetID(), attrbnm );
	    Attrib::DescID attribid = attrserv->getStoredID( lky, true,
		    					     steering ? 1 : 0 );

	    selas.set( attrbnm, attribid, false, 0 );
	    selas.set2DFlag();

	    const Attrib::DescSet* ds = Attrib::DSHolder().getDescSet( true,
		    						       true );

	    if ( !ds ) return false;
	    selas.setRefFromID( *ds );
	    selas.setUserRef( attrbnm );

	    const Attrib::Desc* targetdesc = ds->getDesc( attribid );
	    if ( !targetdesc ) return false;

	    BufferString defstring;
	    targetdesc->getDefStr( defstring );
	    selas.setDefString( defstring );
	    attrserv->setTargetSelSpec( selas );

	    newid = attrserv->create2DOutput( cs, lk,uitr );
	}
    }

    if ( newid == DataPack::cNoID() ) return true;

    viewer2D()->setSelSpec( &selas, true );

    PtrMan<IOObj> ioobj = attrserv->getIOObj( selas );
    if ( ioobj )
    {
	FilePath fp( ioobj->fullUserExpr(true) );
	fp.setExtension( "par" );
	IOPar iop;
	if ( iop.read(fp.fullPath(),sKey::Pars()) && !iop.isEmpty() )
	{
	    ColTab::MapperSetup mapper;
	    mapper.usePar( iop );

	    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
	    {
		FlatView::DataDispPars& ddp =
		    viewer2D()->viewwin()->viewer(ivwr).appearance().ddpars_;
    		ddp.wva_.mappersetup_ = mapper;
	    }
	}
    }

    viewer2D()->setUpView( newid, true );

    return false;
}


uiTreeItem* uiODVW2DWiggleVarAreaTreeItemFactory::createForVis( 
					    const uiODViewer2D&, int id ) const
{
    return 0;
}

