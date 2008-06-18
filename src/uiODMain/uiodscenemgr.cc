/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodscenemgr.cc,v 1.133 2008-06-18 22:18:51 cvskris Exp $
________________________________________________________________________

-*/

#include "uiodscenemgr.h"
#include "scene.xpm"

#include "uiodapplmgr.h"
#include "uiodmenumgr.h"
#include "uiodscenetreeitem.h"
#include "uiempartserv.h"
#include "uivispartserv.h"
#include "uiattribpartserv.h"

#include "uibutton.h"
#include "uidockwin.h"
#include "uilabel.h"
#include "uislider.h"
#include "uisoviewer.h"
#include "uilistview.h"
#include "uiworkspace.h"
#include "uistatusbar.h"
#include "uithumbwheel.h"
#include "uiflatviewer.h"
#include "uigeninputdlg.h"
#include "uiprintscenedlg.h"
#include "uiflatviewstdcontrol.h"
#include "uiflatviewdockwin.h"
#include "uiflatviewmainwin.h"
#include "uitreeitemmanager.h"
#include "uimsg.h"

#include "coltabsequence.h"
#include "ptrman.h"
#include "settings.h"
#include "sorting.h"
#include "survinfo.h"

// For factories
#include "uiodhortreeitem.h"
#include "uiodfaulttreeitem.h"
#include "uiodscenetreeitem.h"
#include "uioddatatreeitem.h"
#include "uiodpicksettreeitem.h"
#include "uiodseis2dtreeitem.h"
#include "uiodplanedatatreeitem.h"
#include "uiodmarchingcubessurfacetreeitem.h"
#include "uiodemsurftreeitem.h"
#include "uiodrandlinetreeitem.h"
#include "uiodvolrentreeitem.h"
#include "uiodwelltreeitem.h"

#define mPosField	0
#define mValueField	1
#define mNameField	2
#define mStatusField	3

static const int cWSWidth = 600;
static const int cWSHeight = 500;
static const int cMinZoom = 1;
static const int cMaxZoom = 150;
static const char* scenestr = "Scene ";

#define mWSMCB(fn) mCB(this,uiODSceneMgr,fn)
#define mDoAllScenes(memb,fn,arg) \
    for ( int idx=0; idx<scenes_.size(); idx++ ) \
	scenes_[idx]->memb->fn( arg )



uiODSceneMgr::uiODSceneMgr( uiODMain* a )
    : appl_(*a)
    , wsp_(new uiWorkSpace(a,"OpendTect work space"))
    , vwridx_(0)
    , lasthrot_(0), lastvrot_(0), lastdval_(0)
    , tifs_(new uiTreeFactorySet)
    , sceneClosed(this)
    , treeToBeAdded(this)
{
    tifs_->addFactory( new uiODInlineTreeItemFactory, 1000,
	    	       SurveyInfo::No2D );
    tifs_->addFactory( new uiODCrosslineTreeItemFactory, 2000,
	    	       SurveyInfo::No2D );
    tifs_->addFactory( new uiODTimesliceTreeItemFactory, 3000,
	    	       SurveyInfo::No2D );
    tifs_->addFactory( new uiODVolrenTreeItemFactory, 3100, SurveyInfo::No2D );
    tifs_->addFactory( new uiODRandomLineTreeItemFactory, 3500,
	    	       SurveyInfo::No2D );
    tifs_->addFactory( new Seis2DTreeItemFactory, 4000, SurveyInfo::Only2D );
    tifs_->addFactory( new uiODPickSetTreeItemFactory, 5000,
	    	       SurveyInfo::Both2DAnd3D );
    tifs_->addFactory( new uiODHorizonTreeItemFactory, 6000,
	    	       SurveyInfo::Both2DAnd3D );
    tifs_->addFactory( new uiODHorizon2DTreeItemFactory, 6500,
		       SurveyInfo::Only2D );
    tifs_->addFactory( new uiODFaultTreeItemFactory, 7000, SurveyInfo::No2D );
    tifs_->addFactory( new uiODMarchingCubesTreeItemFactory, 7500,
	    	       SurveyInfo::No2D );
    tifs_->addFactory( new uiODWellTreeItemFactory, 8000,
	    	       SurveyInfo::Both2DAnd3D );

    wsp_->changed.notify( mCB(this,uiODSceneMgr,wspChanged) );
    wsp_->setPrefWidth( cWSWidth );
    wsp_->setPrefHeight( cWSHeight );

    uiGroup* leftgrp = new uiGroup( &appl_, "Left group" );

    dollywheel = new uiThumbWheel( leftgrp, "Dolly", false );
    dollywheel->wheelMoved.notify( mWSMCB(dWheelMoved) );
    dollywheel->wheelPressed.notify( mWSMCB(anyWheelStart) );
    dollywheel->wheelReleased.notify( mWSMCB(anyWheelStop) );
    uiLabel* dollylbl = new uiLabel( leftgrp, "Mov" );
    dollylbl->attach( centeredBelow, dollywheel );

    uiLabel* dummylbl = new uiLabel( leftgrp, "" );
    dummylbl->attach( centeredBelow, dollylbl );
    dummylbl->setStretch( 0, 2 );
    vwheel = new uiThumbWheel( leftgrp, "vRotate", false );
    vwheel->wheelMoved.notify( mWSMCB(vWheelMoved) );
    vwheel->wheelPressed.notify( mWSMCB(anyWheelStart) );
    vwheel->wheelReleased.notify( mWSMCB(anyWheelStop) );
    vwheel->attach( centeredBelow, dummylbl );

    uiLabel* rotlbl = new uiLabel( &appl_, "Rot" );
    rotlbl->attach( centeredBelow, leftgrp );

    hwheel = new uiThumbWheel( &appl_, "hRotate", true );
    hwheel->wheelMoved.notify( mWSMCB(hWheelMoved) );
    hwheel->wheelPressed.notify( mWSMCB(anyWheelStart) );
    hwheel->wheelReleased.notify( mWSMCB(anyWheelStop) );
    hwheel->attach( leftAlignedBelow, wsp_ );

    zoomslider_ = new uiSliderExtra( &appl_, "Zoom", "Zoom Slider" );
    zoomslider_->sldr()->valueChanged.notify( mWSMCB(zoomChanged) );
    zoomslider_->sldr()->setMinValue( cMinZoom );
    zoomslider_->sldr()->setMaxValue( cMaxZoom );
    zoomslider_->setStretch( 0, 0 );
    zoomslider_->attach( rightAlignedBelow, wsp_ );

    leftgrp->attach( leftOf, wsp_ );
    appl_.finaliseDone.notify( mCB(this,uiODSceneMgr,afterFinalise) );
}


uiODSceneMgr::~uiODSceneMgr()
{
    cleanUp( false );
    delete tifs_;
    delete wsp_;
}


void uiODSceneMgr::initMenuMgrDepObjs()
{
    if ( scenes_.isEmpty() )
	addScene(true);
}


void uiODSceneMgr::cleanUp( bool startnew )
{
    wsp_->closeAll();
    // closeAll() cascades callbacks which remove the scene from set

    visServ().deleteAllObjects();
    vwridx_ = 0;
    if ( startnew ) addScene(true);
}


uiODSceneMgr::Scene& uiODSceneMgr::mkNewScene()
{
    uiODSceneMgr::Scene& scn = *new uiODSceneMgr::Scene( wsp_ );
    scn.wsgrp_->closed().notify( mWSMCB(removeScene) );
    scenes_ += &scn;
    vwridx_++;
    BufferString vwrnm( "Viewer Scene ", vwridx_ );
    scn.sovwr_->setName( vwrnm );
    return scn;
}


int uiODSceneMgr::addScene( bool maximized )
{
    Scene& scn = mkNewScene();
    const int sceneid = visServ().addScene();
    scn.sovwr_->setSceneID( sceneid );
    BufferString title( scenestr );
    title += vwridx_;
    scn.wsgrp_->setTitle( title );
    visServ().setObjectName( sceneid, title );
    scn.sovwr_->display();
    scn.sovwr_->viewAll();
    scn.sovwr_->setHomePos();
    scn.sovwr_->viewmodechanged.notify( mWSMCB(viewModeChg) );
    scn.sovwr_->pageupdown.notify( mCB(this,uiODSceneMgr,pageUpDownPressed) );
    scn.wsgrp_->display( true, false, maximized );
    actMode(0);
    setZoomValue( scn.sovwr_->getCameraZoom() );
    treeToBeAdded.trigger( sceneid );
    initTree( scn, vwridx_ );

    if ( scenes_.size()>1 && scenes_[0] )
    {
	scn.sovwr_->setStereoType( scenes_[0]->sovwr_->getStereoType() );
	scn.sovwr_->setStereoOffset(
		scenes_[0]->sovwr_->getStereoOffset() );
	scn.sovwr_->showRotAxis( scenes_[0]->sovwr_->rotAxisShown() );
	if ( !scenes_[0]->sovwr_->isCameraPerspective() )
	    scn.sovwr_->toggleCameraType();
    }

    return sceneid;
}


void uiODSceneMgr::afterFinalise( CallBacker* )
{
    dollywheel->display( true );
    hwheel->display( true );
    vwheel->display( true );
}


void uiODSceneMgr::removeScene( CallBacker* cb )
{
    mDynamicCastGet(uiGroupObj*,grp,cb)
    if ( !grp ) return;
    int idxnr = -1;
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	if ( grp == scenes_[idx]->wsgrp_->mainObject() )
	{
	    idxnr = idx;
	    break;
	}
    }
    if ( idxnr < 0 ) return;

    uiODSceneMgr::Scene* scene = scenes_[idxnr];
    scene->itemmanager_->prepareForShutdown();
    visServ().removeScene( scene->itemmanager_->sceneID() );
    appl_.removeDockWindow( scene->treeWin() );

    scene->wsgrp_->closed().remove( mWSMCB(removeScene) );
    scenes_ -= scene;
    sceneClosed.trigger( scene->itemmanager_->sceneID() );
    delete scene;
}


void uiODSceneMgr::setSceneName( int sceneid, const char* nm )
{
    visServ().setObjectName( sceneid, nm );

    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	Scene& scene = *scenes_[idx];
	if ( scene.itemmanager_->sceneID() == sceneid )
	{
	    scene.wsgrp_->setTitle( nm );
	    scene.treeWin()->setDockName( nm );
	    uiTreeItem* itm = scene.itemmanager_->findChild( sceneid );
	    if ( itm )
		itm->updateColumnText( uiODSceneMgr::cNameColumn() );
	    return;
	}
    }
}


const char* uiODSceneMgr::getSceneName( int sceneid )
{ return visServ().getObjectName( sceneid ); }


void uiODSceneMgr::storePositions()
{
    //TODO remember the scene's positions
    // mDoAllScenes(sovwr_,storePosition,);
}


void uiODSceneMgr::getScenePars( IOPar& iopar )
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	IOPar iop;
	scenes_[idx]->sovwr_->fillPar( iop );
	BufferString key = idx;
	iopar.mergeComp( iop, key );
    }
}


void uiODSceneMgr::useScenePars( const IOPar& sessionpar )
{
    for ( int idx=0; ; idx++ )
    {
	BufferString key = idx;
	PtrMan<IOPar> scenepar = sessionpar.subselect( key );
	if ( !scenepar || !scenepar->size() )
	{
	    if ( !idx ) continue;
	    break;
	}

	Scene& scn = mkNewScene();
  	if ( !scn.sovwr_->usePar(*scenepar) )
	{
	    removeScene( scn.wsgrp_->mainObject() );
	    continue;
	}
    
	BufferString title( scenestr );
	title += vwridx_;
  	scn.wsgrp_->setTitle( title );
	scn.sovwr_->display();
	scn.sovwr_->viewmodechanged.notify( mWSMCB(viewModeChg) );
	scn.sovwr_->pageupdown.notify(mCB(this,uiODSceneMgr,pageUpDownPressed));
	scn.wsgrp_->display( true, false );
	setZoomValue( scn.sovwr_->getCameraZoom() );
	initTree( scn, vwridx_ );
    }

    rebuildTrees();
}


void uiODSceneMgr::viewModeChg( CallBacker* cb )
{
    if ( scenes_.isEmpty() ) return;

    mDynamicCastGet(uiSoViewer*,sovwr_,cb)
    if ( sovwr_ ) setToViewMode( sovwr_->isViewing() );
}


void uiODSceneMgr::setToViewMode( bool yn )
{
    mDoAllScenes(sovwr_,setViewing,yn);
    visServ().setViewMode( yn , false );
    menuMgr().updateViewMode( yn );
    updateStatusBar();
}


void uiODSceneMgr::actMode( CallBacker* )
{
    setToViewMode( false );
}


void uiODSceneMgr::viewMode( CallBacker* )
{
    setToViewMode( true );
}


void uiODSceneMgr::pageUpDownPressed( CallBacker* cb )
{
    mCBCapsuleUnpack(bool,up,cb);
    applMgr().pageUpDownPressed( up );
}


void uiODSceneMgr::updateStatusBar()
{
    if ( visServ().isViewMode() )
    {
	appl_.statusBar()->message( "", mPosField );
	appl_.statusBar()->message( "", mValueField );
	appl_.statusBar()->message( "", mNameField );
	appl_.statusBar()->message( "", mStatusField );
	appl_.statusBar()->setBGColor( mStatusField,
				   appl_.statusBar()->getBGColor(mPosField) );
    }

    const Coord3 xytpos = visServ().getMousePos( true );
    const bool haspos = !mIsUdf( xytpos.x );

    BufferString msg;
    if ( haspos  )
    {
	const BinID bid( SI().transform( Coord(xytpos.x,xytpos.y) ) );
	msg = bid.inl; msg += "/"; msg += bid.crl;
	msg += "   (";
	msg += mNINT(xytpos.x); msg += ",";
	msg += mNINT(xytpos.y); msg += ",";
//	msg += SI().zIsTime() ? mNINT(xytpos.z * 1000) : xytpos.z;
	const float zfact = visServ().zFactor();
	const float zval = SI().zIsTime() && visServ().zFactor()>100 
	    				? mNINT(xytpos.z*zfact) : xytpos.z;
	msg += zval; msg += ")";
    }

    appl_.statusBar()->message( msg, mPosField );

    const BufferString valstr = visServ().getMousePosVal();
    if ( haspos )
    {
	msg = valstr.isEmpty() ? "" : "Value = ";
	msg += valstr;
    }
    else
	msg = "";

    appl_.statusBar()->message( msg, mValueField );

    msg = haspos ? visServ().getMousePosString() : "";
    if ( msg.isEmpty() )
    {
	const int selid = visServ().getSelObjectId();
	msg = visServ().getInteractionMsg( selid );
    }
    appl_.statusBar()->message( msg, mNameField );

    visServ().getPickingMessage( msg );
    appl_.statusBar()->message( msg, mStatusField );

    appl_.statusBar()->setBGColor( mStatusField, visServ().isPicking() ?
	    Color(255,0,0) : appl_.statusBar()->getBGColor(mPosField) );
}


void uiODSceneMgr::setKeyBindings()
{
    if ( scenes_.isEmpty() ) return;

    BufferStringSet keyset;
    scenes_[0]->sovwr_->getAllKeyBindings( keyset );

    StringListInpSpec* inpspec = new StringListInpSpec( keyset );
    inpspec->setText( scenes_[0]->sovwr_->getCurrentKeyBindings(), 0 );
    uiGenInputDlg dlg( &appl_, "Select Mouse Controls", "Select", inpspec );
    if ( dlg.go() )
	mDoAllScenes(sovwr_,setKeyBindings,dlg.text());
}


int uiODSceneMgr::getStereoType() const
{
    return scenes_.size() ? (int)scenes_[0]->sovwr_->getStereoType() : 0;
}


void uiODSceneMgr::setStereoType( int type )
{
    if ( scenes_.isEmpty() ) return;

    uiSoViewer::StereoType stereotype = (uiSoViewer::StereoType)type;
    const float stereooffset = scenes_[0]->sovwr_->getStereoOffset();
    for ( int ids=0; ids<scenes_.size(); ids++ )
    {
	uiSoViewer& sovwr_ = *scenes_[ids]->sovwr_;
	if ( !sovwr_.setStereoType(stereotype) )
	{
	    uiMSG().error( "No support for this type of stereo rendering" );
	    return;
	}
	if ( type )
	    sovwr_.setStereoOffset( stereooffset );
    }
}


void uiODSceneMgr::tile()		{ wsp_->tile(); }
void uiODSceneMgr::tileHorizontal()	{ wsp_->tileHorizontal(); }
void uiODSceneMgr::tileVertical()	{ wsp_->tileVertical(); }
void uiODSceneMgr::cascade()		{ wsp_->cascade(); }


void uiODSceneMgr::layoutScenes()
{
    const int nrgrps = scenes_.size();
    if ( nrgrps == 1 && scenes_[0] )
	scenes_[0]->wsgrp_->display( true, false, true );
    else if ( scenes_[0] )
	tile();
}


void uiODSceneMgr::toHomePos( CallBacker* )
{ mDoAllScenes(sovwr_,toHomePos,); }
void uiODSceneMgr::saveHomePos( CallBacker* )
{ mDoAllScenes(sovwr_,saveHomePos,); }
void uiODSceneMgr::viewAll( CallBacker* )
{ mDoAllScenes(sovwr_,viewAll,); }
void uiODSceneMgr::align( CallBacker* )
{ mDoAllScenes(sovwr_,align,); }

void uiODSceneMgr::viewX( CallBacker* )
{ mDoAllScenes(sovwr_,viewPlane,uiSoViewer::X); }
void uiODSceneMgr::viewY( CallBacker* )
{ mDoAllScenes(sovwr_,viewPlane,uiSoViewer::Y); }
void uiODSceneMgr::viewZ( CallBacker* )
{ mDoAllScenes(sovwr_,viewPlane,uiSoViewer::Z); }
void uiODSceneMgr::viewInl( CallBacker* )
{ mDoAllScenes(sovwr_,viewPlane,uiSoViewer::Inl); }
void uiODSceneMgr::viewCrl( CallBacker* )
{ mDoAllScenes(sovwr_,viewPlane,uiSoViewer::Crl); }


void uiODSceneMgr::showRotAxis( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,tb,cb)
    mDoAllScenes(sovwr_,showRotAxis,tb?tb->isOn():false);
}


void uiODSceneMgr::mkSnapshot( CallBacker* )
{
    ObjectSet<uiSoViewer> viewers;
    getSoViewers( viewers );
    if ( viewers.size() == 0 ) return;

    uiPrintSceneDlg dlg( &appl_, viewers );
    dlg.go();
    // TODO: save settings in iopar
}


void uiODSceneMgr::selectionMode( CallBacker* )
{
    uiVisPartServer::SelectionMode selmode = visServ().getSelectionMode();
    if ( selmode==uiVisPartServer::Off )
	selmode = uiVisPartServer::Polygon;
    else
	selmode = uiVisPartServer::Off;

    visServ().setSelectionMode( selmode );
}



void uiODSceneMgr::soloMode( CallBacker* )
{
    TypeSet< TypeSet<int> > dispids;
    int selectedid;
    for ( int idx=0; idx<scenes_.size(); idx++ )
	dispids += scenes_[idx]->itemmanager_->getDisplayIds( selectedid,  
						 !menuMgr().isSoloModeOn() );
    
    visServ().setSoloMode( menuMgr().isSoloModeOn(), dispids, selectedid );
}


void uiODSceneMgr::setZoomValue( float val )
{
    zoomslider_->sldr()->setValue( val );
}


void uiODSceneMgr::zoomChanged( CallBacker* )
{
    const float zmval = zoomslider_->sldr()->getValue();
    mDoAllScenes(sovwr_,setCameraZoom,zmval);
}


void uiODSceneMgr::anyWheelStart( CallBacker* )
{ mDoAllScenes(sovwr_,anyWheelStart,); }
void uiODSceneMgr::anyWheelStop( CallBacker* )
{ mDoAllScenes(sovwr_,anyWheelStop,); }


void uiODSceneMgr::wheelMoved( CallBacker* cb, int wh, float& lastval )
{
    mDynamicCastGet(uiThumbWheel*,wheel,cb)
    if ( !wheel ) { pErrMsg("huh") ; return; }

    const float whlval = wheel->lastMoveVal();
    const float diff = lastval - whlval;

    if ( diff )
    {
	for ( int idx=0; idx<scenes_.size(); idx++ )
	{
	    if ( wh == 1 )
		scenes_[idx]->sovwr_->rotateH( diff );
	    else if ( wh == 2 )
		scenes_[idx]->sovwr_->rotateV( -diff );
	    else
		scenes_[idx]->sovwr_->dolly( diff );
	}
    }

    lastval = whlval;
}


void uiODSceneMgr::hWheelMoved( CallBacker* cb )
{ wheelMoved(cb,1,lasthrot_); }
void uiODSceneMgr::vWheelMoved( CallBacker* cb )
{ wheelMoved(cb,2,lastvrot_); }
void uiODSceneMgr::dWheelMoved( CallBacker* cb )
{ wheelMoved(cb,0,lastdval_); }


void uiODSceneMgr::switchCameraType( CallBacker* )
{
    ObjectSet<uiSoViewer> vwrs;
    getSoViewers( vwrs );
    if ( vwrs.isEmpty() ) return;
    mDoAllScenes(sovwr_,toggleCameraType,);
    const bool isperspective = vwrs[0]->isCameraPerspective();
    menuMgr().setCameraPixmap( isperspective );
    zoomslider_->setSensitive( isperspective );
}


void uiODSceneMgr::getSoViewers( ObjectSet<uiSoViewer>& vwrs )
{
    vwrs.erase();
    for ( int idx=0; idx<scenes_.size(); idx++ )
	vwrs += scenes_[idx]->sovwr_;
}


uiODTreeTop* uiODSceneMgr::getTreeItemMgr( const uiListView* lv ) const
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	if ( scenes_[idx]->lv_ == lv )
	    return scenes_[idx]->itemmanager_;
    }
    return 0;
}


void uiODSceneMgr::getSceneNames( BufferStringSet& nms, int& active )
{
    wsp_->getWindowNames( nms );
    const char* activenm = wsp_->getActiveWin();
    active = nms.indexOf( activenm );
}


void uiODSceneMgr::wspChanged( CallBacker* )
{ menuMgr().updateWindowsMenu(); }

void uiODSceneMgr::setActiveScene( const char* scenenm )
{ wsp_->setActiveWin( scenenm ); }


void uiODSceneMgr::initTree( Scene& scn, int vwridx )
{
    BufferString capt( "Tree scene " ); capt += vwridx;
    uiDockWin* dw = new uiDockWin( &appl_, capt );

    scn.lv_ = new uiListView( dw, capt );
    scn.lv_->addColumn( "Elements" );
    scn.lv_->addColumn( "Color" );
    scn.lv_->setColumnWidthMode( cNameColumn(), uiListView::Manual );
    scn.lv_->setColumnWidthMode( cColorColumn(), uiListView::Manual );

    scn.lv_->setColumnWidth( cColorColumn(), 38 );
    scn.lv_->setColumnWidth( cNameColumn(), 152 );
    scn.lv_->setPrefWidth( 190 );

    scn.lv_->setStretch( 2, 2 );

    scn.itemmanager_ = new uiODTreeTop( scn.sovwr_, scn.lv_, &applMgr(), tifs_);

    TypeSet<int> idxs;
    TypeSet<int> placeidxs;
    for ( int idx=0; idx<tifs_->nrFactories(); idx++ )
    {
	SurveyInfo::Pol2D pol2d = (SurveyInfo::Pol2D)tifs_->getPol2D( idx );
	if ( SI().getSurvDataType() == SurveyInfo::Both2DAnd3D ||
	     pol2d == SurveyInfo::Both2DAnd3D ||
	     pol2d == SI().getSurvDataType() )
	{
	    idxs += idx;
	    placeidxs += tifs_->getPlacementIdx( idx );
	}
    }

    sort_coupled( placeidxs.arr(), idxs.arr(), idxs.size() );

    for ( int idx=0; idx<idxs.size(); idx++ )
    {
	const int fidx = idxs[idx];
	scn.itemmanager_->addChild( tifs_->getFactory(fidx)->create(), true );
    }

    uiODSceneTreeItem* sceneitm =
	new uiODSceneTreeItem( scn.wsgrp_->getTitle(), scn.sovwr_->sceneID() );
    scn.itemmanager_->addChild( sceneitm, false );
    scn.lv_->display();
    appl_.addDockWindow( *dw, uiMainWin::Left );
    scn.treeWin()->display();
}


void uiODSceneMgr::updateTrees()
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	Scene& scene = *scenes_[idx];
	scene.itemmanager_->updateColumnText( cNameColumn() );
	scene.itemmanager_->updateColumnText( cColorColumn() );
	scene.itemmanager_->updateCheckStatus();
    }
}


void uiODSceneMgr::rebuildTrees()
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	Scene& scene = *scenes_[idx];
	const int sceneid = scene.sovwr_->sceneID();
	TypeSet<int> visids; visServ().getChildIds( sceneid, visids );

	for ( int idy=0; idy<visids.size(); idy++ )
	    uiODDisplayTreeItem::create( scene.itemmanager_, &applMgr(), 
		    			 visids[idy] );
    }
    updateSelectedTreeItem();
}


void uiODSceneMgr::setItemInfo( int id )
{
    mDoAllScenes(itemmanager_,updateColumnText,cColorColumn());
    appl_.statusBar()->message( "", mPosField );
    appl_.statusBar()->message( "", mValueField );
    appl_.statusBar()->message( visServ().getInteractionMsg(id), mNameField );
    appl_.statusBar()->message( "", mStatusField );
    appl_.statusBar()->setBGColor( mStatusField,
	    			   appl_.statusBar()->getBGColor(mPosField) );
}


void uiODSceneMgr::updateSelectedTreeItem()
{
    const int id = visServ().getSelObjectId();

    if ( id != -1 )
    {
	setItemInfo( id );
	//applMgr().modifyColorTable( id );
	if ( !visServ().isOn(id) ) visServ().turnOn(id, true, true);
	else if ( scenes_.size() != 1 && visServ().isSoloMode() )
	    visServ().updateDisplay( true, id );
    }

    if ( !applMgr().attrServer() )
	return;

    bool found = applMgr().attrServer()->attrSetEditorActive();
    bool gotoact = false;
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	Scene& scene = *scenes_[idx];
	if ( !found )
	{
	    mDynamicCastGet( const uiODDisplayTreeItem*, treeitem,
		    	     scene.itemmanager_->findChild(id) );
	    if ( treeitem )
	    {
		gotoact = treeitem->actModeWhenSelected();
		found = true;
	    }
	}

	scene.itemmanager_->updateSelection( id );
	scene.itemmanager_->updateColumnText( cNameColumn() );
	scene.itemmanager_->updateColumnText( cColorColumn() );
    }

    if ( gotoact && !applMgr().attrServer()->attrSetEditorActive() )
	actMode( 0 );
}


int uiODSceneMgr::getIDFromName( const char* str ) const
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	const uiTreeItem* itm = scenes_[idx]->itemmanager_->findChild( str );
	if ( itm ) return itm->selectionKey();
    }

    return -1;
}


void uiODSceneMgr::disabRightClick( bool yn )
{
    mDoAllScenes(itemmanager_,disabRightClick,yn);
}


void uiODSceneMgr::disabTrees( bool yn )
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
	scenes_[idx]->lv_->setSensitive( !yn );
}


int uiODSceneMgr::addEMItem( const EM::ObjectID& emid, int sceneid )
{
    const char* type = applMgr().EMServer()->getType(emid);
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	Scene& scene = *scenes_[idx];
	if ( sceneid>=0 && sceneid!=scene.sovwr_->sceneID() ) continue;

	uiODDisplayTreeItem* itm;
	if ( !strcmp( type, "Horizon" ) ) 
	    itm = new uiODHorizonTreeItem(emid);
	else if ( !strcmp( type, "2D Horizon" ) ) 
	    itm = new uiODHorizon2DTreeItem(emid);
	else if ( !strcmp(type,"Fault" ) ) 
	    itm = new uiODFaultTreeItem(emid);

	scene.itemmanager_->addChild( itm, false );
	return itm->displayID();
    }

    return -1;
}


int uiODSceneMgr::addRandomLineItem( int visid, int sceneid )
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	Scene& scene = *scenes_[idx];
	if ( sceneid>=0 && sceneid!=scene.sovwr_->sceneID() ) continue;

	uiODRandomLineTreeItem* itm = new uiODRandomLineTreeItem( visid );
	scene.itemmanager_->addChild( itm, false );
	return itm->displayID();
    }
    return -1;
}


void uiODSceneMgr::removeTreeItem( int displayid )
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	Scene& scene = *scenes_[idx];
	uiTreeItem* itm = scene.itemmanager_->findChild( displayid );
	if ( itm ) scene.itemmanager_->removeChild( itm );
    }
}


uiTreeItem* uiODSceneMgr::findItem( int displayid )
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	Scene& scene = *scenes_[idx];
	uiTreeItem* itm = scene.itemmanager_->findChild( displayid );
	if ( itm ) return itm;
    }

    return 0;
}


void uiODSceneMgr::findItems( const char* nm, ObjectSet<uiTreeItem>& items )
{
    deepErase( items );
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	Scene& scene = *scenes_[idx];
	scene.itemmanager_->findChildren( nm, items );
    }
}


void uiODSceneMgr::displayIn2DViewer( int visid, int attribid, bool dowva )
{
    Viewer2D* curvwr = 0;
    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	if ( viewers2d_[idx]->visid_ != visid )
	    continue;
	curvwr = viewers2d_[idx];
	break;
    }

    if ( !curvwr )
	curvwr = &addViewer2D( visid );

    const bool isvert = visServ().isVerticalDisp( visid );
    curvwr->setUpView( visServ().getDataPackID(visid,attribid), dowva, isvert );
    appl_.applMgr().visServer()->fillDispPars( visid, attribid,
	    curvwr->viewwin_->viewer().appearance().ddpars_ );
    curvwr->viewwin_->viewer().handleChange( FlatView::Viewer::All );
}


void uiODSceneMgr::remove2DViewer( int visid )
{
    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	if ( viewers2d_[idx]->visid_ != visid )
	    continue;

	delete viewers2d_.remove( idx );
	return;
    }
}


uiODSceneMgr::Viewer2D& uiODSceneMgr::addViewer2D( int visid )
{
    Viewer2D* vwr = new Viewer2D( appl_, visid );
    viewers2d_ += vwr;
    return *vwr;
}


uiODSceneMgr::Scene::Scene( uiWorkSpace* wsp )
        : lv_(0)
	, wsgrp_(0)
        , sovwr_(0)
    	, itemmanager_(0)
{
    if ( !wsp ) return;

    wsgrp_ = new uiWorkSpaceGroup();
    wsgrp_->setIcon( scene_xpm_data );
    sovwr_ = new uiSoViewer( wsgrp_ );
    sovwr_->setPrefWidth( 400 );
    sovwr_->setPrefHeight( 400 );
    wsp->addGroup( wsgrp_ );
}


uiODSceneMgr::Scene::~Scene()
{
    delete sovwr_;
    delete itemmanager_;
    delete treeWin();
}


uiDockWin* uiODSceneMgr::Scene::treeWin()
{
    return lv_ ? (uiDockWin*)lv_->parent() : 0;
}



uiODSceneMgr::Viewer2D::Viewer2D( uiODMain& appl, int visid )
    : appl_(appl)
    , visid_(visid)
    , viewwin_(0)
{
    basetxt_ = "2D Viewer - ";
    BufferString info;
    appl.applMgr().visServer()->getObjectInfo( visid, info );
    basetxt_ += info;
}


uiODSceneMgr::Viewer2D::~Viewer2D()
{
    mDynamicCastGet(uiFlatViewDockWin*,fvdw,viewwin_)
    if ( fvdw )
	appl_.removeDockWindow( fvdw );

    delete viewwin_;
}


void uiODSceneMgr::Viewer2D::setUpView( DataPack::ID packid,
					bool wva, bool isvert )
{
    const bool isnew = !viewwin_;
    if ( isnew )
	createViewWin( isvert );

    viewwin_->viewer().setPack( wva, packid, true, !isnew );
    FlatView::DataDispPars& ddp = viewwin_->viewer().appearance().ddpars_;
    (wva ? ddp.wva_.show_ : ddp.vd_.show_) = true;
    viewwin_->start();
}


void uiODSceneMgr::Viewer2D::createViewWin( bool isvert )
{    
    bool wantdock = false;
    Settings::common().getYN( "FlatView.Use Dockwin", wantdock );
    uiParent* controlparent = 0;
    if ( !wantdock )
	viewwin_ = new uiFlatViewMainWin( 0,
				   uiFlatViewMainWin::Setup(basetxt_) );
    else
    {
	uiFlatViewDockWin* dwin = new uiFlatViewDockWin( &appl_,
				   uiFlatViewDockWin::Setup(basetxt_) );
	appl_.addDockWindow( *dwin, uiMainWin::Top );
	dwin->setFloating( true );
	viewwin_ = dwin;
	controlparent = &appl_;
    }
    uiFlatViewer& vwr = viewwin_->viewer();
    vwr.setInitialSize( uiSize(600,400) );
    vwr.appearance().setDarkBG( wantdock );
    vwr.appearance().setGeoDefaults(isvert);
    vwr.appearance().annot_.setAxesAnnot(true);
    viewwin_->addControl( new uiFlatViewStdControl( vwr,
		uiFlatViewStdControl::Setup(controlparent).helpid("51.0.0") ) );
}
