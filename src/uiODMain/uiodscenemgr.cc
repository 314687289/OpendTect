/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodscenemgr.cc,v 1.47 2005-10-19 21:48:00 cvskris Exp $
________________________________________________________________________

-*/

#include "uiodscenemgr.h"
#include "uiodapplmgr.h"
#include "uiodmenumgr.h"
#include "uiodtreeitemimpl.h"
#include "uiempartserv.h"
#include "uivispartserv.h"
#include "uiattribpartserv.h"

#include "uilabel.h"
#include "uislider.h"
#include "uidockwin.h"
#include "uisoviewer.h"
#include "uilistview.h"
#include "uiworkspace.h"
#include "uistatusbar.h"
#include "uithumbwheel.h"
#include "uigeninputdlg.h"
#include "uitreeitemmanager.h"
#include "uimsg.h"

#include "ptrman.h"
#include "survinfo.h"
#include "scene.xpm"

static const int cWSWidth = 600;
static const int cWSHeight = 300;
static const int cMinZoom = 1;
static const int cMaxZoom = 150;
static const char* scenestr = "Scene ";

#define mWSMCB(fn) mCB(this,uiODSceneMgr,fn)
#define mDoAllScenes(memb,fn,arg) \
    for ( int idx=0; idx<scenes.size(); idx++ ) \
	scenes[idx]->memb->fn( arg )



uiODSceneMgr::uiODSceneMgr( uiODMain* a )
    	: appl(*a)
    	, wsp(new uiWorkSpace(a,"OpendTect work space"))
	, vwridx(0)
    	, lasthrot(0), lastvrot(0), lastdval(0)
    	, tifs(new uiTreeFactorySet)
{

    tifs->addFactory( new uiODInlineTreeItemFactory, 1000 );
    tifs->addFactory( new uiODCrosslineTreeItemFactory, 2000 );
    tifs->addFactory( new uiODTimesliceTreeItemFactory, 3000 );
    tifs->addFactory( new uiODRandomLineTreeItemFactory, 4000 );
    tifs->addFactory( new uiODPickSetTreeItemFactory, 5000 );
#ifdef __debug__
    tifs->addFactory( new uiODBodyTreeItemFactory, 5500 );
#endif
    tifs->addFactory( new uiODHorizonTreeItemFactory, 6000);
    tifs->addFactory( new uiODFaultTreeItemFactory, 7000 );
    tifs->addFactory( new uiODWellTreeItemFactory, 8000 );

    wsp->setPrefWidth( cWSWidth );
    wsp->setPrefHeight( cWSHeight );

    uiGroup* leftgrp = new uiGroup( &appl, "Left group" );

    uiThumbWheel* dollywheel = new uiThumbWheel( leftgrp, "Dolly", false );
    dollywheel->wheelMoved.notify( mWSMCB(dWheelMoved) );
    dollywheel->wheelPressed.notify( mWSMCB(anyWheelStart) );
    dollywheel->wheelReleased.notify( mWSMCB(anyWheelStop) );
    uiLabel* dollylbl = new uiLabel( leftgrp, "Mov" );
    dollylbl->attach( centeredBelow, dollywheel );

    uiLabel* dummylbl = new uiLabel( leftgrp, "" );
    dummylbl->attach( centeredBelow, dollylbl );
    dummylbl->setStretch( 0, 2 );
    uiThumbWheel* vwheel = new uiThumbWheel( leftgrp, "vRotate", false );
    vwheel->wheelMoved.notify( mWSMCB(vWheelMoved) );
    vwheel->wheelPressed.notify( mWSMCB(anyWheelStart) );
    vwheel->wheelReleased.notify( mWSMCB(anyWheelStop) );
    vwheel->attach( centeredBelow, dummylbl );

    uiLabel* rotlbl = new uiLabel( &appl, "Rot" );
    rotlbl->attach( centeredBelow, leftgrp );

    uiThumbWheel* hwheel = new uiThumbWheel( &appl, "hRotate", true );
    hwheel->wheelMoved.notify( mWSMCB(hWheelMoved) );
    hwheel->wheelPressed.notify( mWSMCB(anyWheelStart) );
    hwheel->wheelReleased.notify( mWSMCB(anyWheelStop) );
    hwheel->attach( leftAlignedBelow, wsp );

    zoomslider = new uiSliderExtra( &appl, "Zoom" );
    zoomslider->sldr()->valueChanged.notify( mWSMCB(zoomChanged) );
    zoomslider->sldr()->setMinValue( cMinZoom );
    zoomslider->sldr()->setMaxValue( cMaxZoom );
    zoomslider->setStretch( 0, 0 );
    zoomslider->attach( rightAlignedBelow, wsp );

    leftgrp->attach( leftOf, wsp );
}


uiODSceneMgr::~uiODSceneMgr()
{
    cleanUp( false );
    delete tifs;
}


void uiODSceneMgr::initMenuMgrDepObjs()
{
    if ( !scenes.size() )
	addScene();
}


void uiODSceneMgr::cleanUp( bool startnew )
{
    while ( scenes.size() )
	scenes[0]->vwrGroup()->mainObject()->close();
    	// close() cascades callbacks which remove the scene from set

    visServ().deleteAllObjects();
    vwridx = 0;
    if ( startnew ) addScene();
}


uiODSceneMgr::Scene& uiODSceneMgr::mkNewScene()
{
    uiODSceneMgr::Scene& scn = *new uiODSceneMgr::Scene( wsp );
    scn.vwrGroup()->mainObject()->closed.notify( mWSMCB(removeScene) );
    scenes += &scn;
    vwridx++;
    return scn;
}


int uiODSceneMgr::addScene()
{
    Scene& scn = mkNewScene();
    int sceneid = visServ().addScene();
    scn.sovwr->setSceneGraph( sceneid );
    BufferString title( scenestr );
    title += vwridx;
    scn.sovwr->setTitle( title );
    visServ().setObjectName( sceneid, title );
    scn.sovwr->display();
    scn.sovwr->viewAll();
    scn.sovwr->saveHomePos();
    scn.sovwr->viewmodechanged.notify( mWSMCB(viewModeChg) );
    scn.sovwr->pageupdown.notify( mCB(this,uiODSceneMgr,pageUpDownPressed) );
    scn.vwrGroup()->display( true, false, true );
    actMode(0);
    setZoomValue( scn.sovwr->getCameraZoom() );
    initTree( scn, vwridx );

    if ( scenes.size() > 1 && scenes[0] )
    {
	scn.sovwr->setStereoType( scenes[0]->sovwr->getStereoType() );
	scn.sovwr->setStereoOffset(
		scenes[0]->sovwr->getStereoOffset() );
    }

    return sceneid;
}


void uiODSceneMgr::removeScene( CallBacker* cb )
{
    mDynamicCastGet(uiGroupObj*,grp,cb)
    if ( !grp ) return;
    int idxnr = -1;
    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	if ( grp == scenes[idx]->vwrGroup()->mainObject() )
	{
	    idxnr = idx;
	    break;
	}
    }
    if ( idxnr < 0 ) return;

    uiODSceneMgr::Scene* scene = scenes[idxnr];
    scene->itemmanager->prepareForShutdown();
    visServ().removeScene( scene->itemmanager->sceneID() );
    appl.removeDockWindow( scene->treeWin() );

    scene->vwrGroup()->mainObject()->closed.remove(mWSMCB(removeScene));
    scenes -= scene;
    delete scene;
}


void uiODSceneMgr::storePositions()
{
    //TODO remember the scene's positions
    // mDoAllScenes(sovwr,storePosition,);
}


void uiODSceneMgr::getScenePars( IOPar& iopar )
{
    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	IOPar iop;
	scenes[idx]->sovwr->fillPar( iop );
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
  	if ( !scn.sovwr->usePar( *scenepar ) )
	{
	    removeScene( scn.vwrGroup() );
	    continue;
	}
    
	int sceneid = scn.sovwr->sceneId();
	BufferString title( scenestr );
	title += vwridx;
  	scn.sovwr->setTitle( title );
	scn.sovwr->display();
	scn.sovwr->viewmodechanged.notify( mWSMCB(viewModeChg) );
	scn.sovwr->pageupdown.notify( mCB(this,uiODSceneMgr,pageUpDownPressed));
	scn.vwrGroup()->display( true, false );
	setZoomValue( scn.sovwr->getCameraZoom() );
	initTree( scn, vwridx );
    }

    rebuildTrees();
}


void uiODSceneMgr::viewModeChg( CallBacker* cb )
{
    if ( !scenes.size() ) return;

    mDynamicCastGet(uiSoViewer*,sovwr,cb)
    if ( sovwr ) setToViewMode( sovwr->isViewing() );
}


void uiODSceneMgr::setToViewMode( bool yn )
{
    mDoAllScenes(sovwr,setViewing,yn);
    visServ().setViewMode( yn );
    menuMgr().updateViewMode( yn );
}


void uiODSceneMgr::actMode( CallBacker* )
{
    setToViewMode( false );
}


void uiODSceneMgr::viewMode( CallBacker* )
{
    setToViewMode( true );
    appl.statusBar()->message( "", 0 );
    appl.statusBar()->message( "", 1 );
    appl.statusBar()->message( "", 2 );
}


void uiODSceneMgr::pageUpDownPressed( CallBacker* cb )
{
    mCBCapsuleUnpack(bool,up,cb);
    applMgr().pageUpDownPressed( up );
}


void uiODSceneMgr::setMousePos()
{
    Coord3 xytpos = visServ().getMousePos(true);
    BufferString msg;
    if ( !mIsUndefined( xytpos.x ) )
    {
	BinID bid( SI().transform( Coord(xytpos.x,xytpos.y) ) );
	msg = bid.inl; msg += "/"; msg += bid.crl;
	msg += "   (";
	msg += mNINT(xytpos.x); msg += ",";
	msg += mNINT(xytpos.y); msg += ",";
	msg += SI().zIsTime() ? mNINT(xytpos.z * 1000) : xytpos.z; msg += ")";
    }

    appl.statusBar()->message( msg, 0 );
    msg = "Value = "; msg += visServ().getMousePosVal();
    appl.statusBar()->message( msg, 1 );
    msg = visServ().getMousePosString();
    appl.statusBar()->message( msg, 2 );
}


void uiODSceneMgr::setKeyBindings()
{
    if ( !scenes.size() ) return;

    BufferStringSet keyset;
    scenes[0]->sovwr->getAllKeyBindings( keyset );

    StringListInpSpec* inpspec = new StringListInpSpec( keyset );
    inpspec->setText( scenes[0]->sovwr->getCurrentKeyBindings(), 0 );
    uiGenInputDlg dlg( &appl, "Select Mouse Controls", "Select", inpspec );
    if ( dlg.go() )
	mDoAllScenes(sovwr,setKeyBindings,dlg.text());
}


int uiODSceneMgr::getStereoType() const
{
    return scenes.size() ? (int)scenes[0]->sovwr->getStereoType() : 0;
}


void uiODSceneMgr::setStereoType( int type )
{
    if ( !scenes.size() ) return;

    uiSoViewer::StereoType stereotype = (uiSoViewer::StereoType)type;
    const float stereooffset = scenes[0]->sovwr->getStereoOffset();
    for ( int ids=0; ids<scenes.size(); ids++ )
    {
	uiSoViewer& sovwr = *scenes[ids]->sovwr;
	if ( !sovwr.setStereoType(stereotype) )
	{
	    uiMSG().error( "No support for this type of stereo rendering" );
	    return;
	}
	if ( type )
	    sovwr.setStereoOffset( stereooffset );
    }
}


void uiODSceneMgr::tile()
{ wsp->tile(); }
void uiODSceneMgr::cascade()
{ wsp->cascade(); }


void uiODSceneMgr::layoutScenes()
{
    const int nrgrps = scenes.size();
    if ( nrgrps == 1 && scenes[0] )
	scenes[0]->vwrGroup()->display( true, false, true );
    else if ( scenes[0] )
	tile();
}


void uiODSceneMgr::toHomePos( CallBacker* )
{ mDoAllScenes(sovwr,toHomePos,); }
void uiODSceneMgr::saveHomePos( CallBacker* )
{ mDoAllScenes(sovwr,saveHomePos,); }
void uiODSceneMgr::viewAll( CallBacker* )
{ mDoAllScenes(sovwr,viewAll,); }
void uiODSceneMgr::align( CallBacker* )
{ mDoAllScenes(sovwr,align,); }

void uiODSceneMgr::viewX( CallBacker* )
{ mDoAllScenes(sovwr,viewPlane,uiSoViewer::X); }
void uiODSceneMgr::viewY( CallBacker* )
{ mDoAllScenes(sovwr,viewPlane,uiSoViewer::Y); }
void uiODSceneMgr::viewZ( CallBacker* )
{ mDoAllScenes(sovwr,viewPlane,uiSoViewer::Z); }
void uiODSceneMgr::viewInl( CallBacker* )
{ mDoAllScenes(sovwr,viewPlane,uiSoViewer::Inl); }
void uiODSceneMgr::viewCrl( CallBacker* )
{ mDoAllScenes(sovwr,viewPlane,uiSoViewer::Crl); }


void uiODSceneMgr::showRotAxis( CallBacker* )
{
    mDoAllScenes(sovwr,showRotAxis,);
    if ( scenes.size() )
	menuMgr().updateAxisMode( scenes[0]->sovwr->rotAxisShown() );
}


void uiODSceneMgr::mkSnapshot( CallBacker* )
{
    if ( scenes.size() )
	scenes[0]->sovwr->renderToFile();
}


void uiODSceneMgr::soloMode( CallBacker* )
{
    TypeSet<int> dispids;
    int selectedid;
    for ( int idx=0; idx<scenes.size(); idx++ )
	scenes[idx]->itemmanager->getDisplayIds( dispids, selectedid,  
						 !menuMgr().isSoloModeOn() );
    
    visServ().setSoloMode( menuMgr().isSoloModeOn(), dispids, selectedid );
}


void uiODSceneMgr::setZoomValue( float val )
{
    zoomslider->sldr()->setValue( val );
}


void uiODSceneMgr::zoomChanged( CallBacker* )
{
    const float zmval = zoomslider->sldr()->getValue();
    mDoAllScenes(sovwr,setCameraZoom,zmval);
}


void uiODSceneMgr::anyWheelStart( CallBacker* )
{ mDoAllScenes(sovwr,anyWheelStart,); }
void uiODSceneMgr::anyWheelStop( CallBacker* )
{ mDoAllScenes(sovwr,anyWheelStop,); }


void uiODSceneMgr::wheelMoved( CallBacker* cb, int wh, float& lastval )
{
    mDynamicCastGet(uiThumbWheel*,wheel,cb)
    if ( !wheel ) { pErrMsg("huh") ; return; }

    const float whlval = wheel->lastMoveVal();
    const float diff = lastval - whlval;

    if ( diff )
    {
	for ( int idx=0; idx<scenes.size(); idx++ )
	{
	    if ( wh == 1 )
		scenes[idx]->sovwr->rotateH( diff );
	    else if ( wh == 2 )
		scenes[idx]->sovwr->rotateV( -diff );
	    else
		scenes[idx]->sovwr->dolly( diff );
	}
    }

    lastval = whlval;
}


void uiODSceneMgr::hWheelMoved( CallBacker* cb )
{ wheelMoved(cb,1,lasthrot); }
void uiODSceneMgr::vWheelMoved( CallBacker* cb )
{ wheelMoved(cb,2,lastvrot); }
void uiODSceneMgr::dWheelMoved( CallBacker* cb )
{ wheelMoved(cb,0,lastdval); }


void uiODSceneMgr::switchCameraType( CallBacker* )
{
    ObjectSet<uiSoViewer> vwrs;
    getSoViewers( vwrs );
    if ( !vwrs.size() ) return;
    mDoAllScenes(sovwr,toggleCameraType,);
    const bool isperspective = vwrs[0]->isCameraPerspective();
    menuMgr().setCameraPixmap( isperspective );
    zoomslider->setSensitive( isperspective );
}


void uiODSceneMgr::getSoViewers( ObjectSet<uiSoViewer>& vwrs )
{
    vwrs.erase();
    for ( int idx=0; idx<scenes.size(); idx++ )
	vwrs += scenes[idx]->sovwr;
}


void uiODSceneMgr::initTree( Scene& scn, int vwridx )
{
    BufferString capt( "Tree scene " ); capt += vwridx;
    uiDockWin* dw = new uiDockWin( &appl, capt );
    appl.moveDockWindow( *dw, uiMainWin::Left );
    dw->setResizeEnabled( true );

    scn.lv = new uiListView( dw, "d-Tect Tree" );
    scn.lv->addColumn( "Elements" );
    scn.lv->addColumn( "Position" );
    scn.lv->addColumn( "Color" );
    scn.lv->setColumnWidthMode( 0, uiListView::Manual );
    scn.lv->setColumnWidth( 0, 90 );
    scn.lv->setColumnWidthMode( 1, uiListView::Manual );
    scn.lv->setColumnWidthMode( 2, uiListView::Manual );
    scn.lv->setColumnWidth( 2, 38 );
    scn.lv->setPrefWidth( 190 );
    scn.lv->setStretch( 2, 2 );

    scn.itemmanager = new uiODTreeTop( scn.sovwr, scn.lv, &applMgr(), tifs );

    int nradded = 0;
    int globalhighest = INT_MAX;
    while ( nradded<tifs->nrFactories() )
    {
	int highest = INT_MIN;
	for ( int idx=0; idx<tifs->nrFactories(); idx++ )
	{
	    const int placementidx = tifs->getPlacementIdx(idx);
	    if ( placementidx>highest && placementidx<globalhighest )
		highest = placementidx;
	}

	for ( int idx=0; idx<tifs->nrFactories(); idx++ )
	{
	    if ( tifs->getPlacementIdx(idx)==highest )
	    {
		scn.itemmanager->addChild( tifs->getFactory(idx)->create() );
		nradded++;
	    }
	}

	globalhighest = highest;
    }

    scn.itemmanager->addChild( new uiODSceneTreeItem(scn.sovwr->getTitle(),
						 scn.sovwr->sceneId() ) );
    scn.lv->display();
    scn.treeWin()->display();
}


void uiODSceneMgr::updateTrees()
{
    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	Scene& scene = *scenes[idx];
	scene.itemmanager->updateColumnText(0);
	scene.itemmanager->updateColumnText(1);
    }
}


void uiODSceneMgr::rebuildTrees()
{
    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	Scene& scene = *scenes[idx];
	const int sceneid = scene.sovwr->sceneId();
	TypeSet<int> visids; visServ().getChildIds( sceneid, visids );

	for ( int idy=0; idy<visids.size(); idy++ )
	    uiODDisplayTreeItem::create( scene.itemmanager, &applMgr(), 
		    			 visids[idy] );
    }
    updateSelectedTreeItem();
}


void uiODSceneMgr::setItemInfo( int id )
{
    mDoAllScenes(itemmanager,updateColumnText,1);
    mDoAllScenes(itemmanager,updateColumnText,2);
    appl.statusBar()->message( "", 0 );
    appl.statusBar()->message( "", 1 );
    appl.statusBar()->message( visServ().getInteractionMsg(id), 2 );
}


void uiODSceneMgr::updateSelectedTreeItem()
{
    const int id = visServ().getSelObjectId();

    if ( id != -1 )
    {
	setItemInfo( id );
	applMgr().modifyColorTable( id );
	if ( !visServ().isOn(id) ) visServ().turnOn(id, true, true);
    }

    bool found = applMgr().attrServer()->attrSetEditorActive();
    bool gotoact = false;
    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	Scene& scene = *scenes[idx];
	if ( !found )
	{
	    mDynamicCastGet( const uiODDisplayTreeItem*, treeitem,
		    	     scene.itemmanager->findChild(id) );
	    if ( treeitem )
	    {
		gotoact = treeitem->actModeWhenSelected();
		found = true;
	    }
	}

	scene.itemmanager->updateSelection( id );
	scene.itemmanager->updateColumnText(0);
	scene.itemmanager->updateColumnText(1);
    }

    if ( gotoact && !applMgr().attrServer()->attrSetEditorActive() )
	actMode( 0 );
}


int uiODSceneMgr::getIDFromName( const char* str ) const
{
    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	const uiTreeItem* itm = scenes[idx]->itemmanager->findChild( str );
	if ( itm ) return itm->selectionKey();
    }

    return -1;
}


void uiODSceneMgr::disabRightClick( bool yn )
{
    mDoAllScenes(itemmanager,disabRightClick,yn);
}


void uiODSceneMgr::disabTrees( bool yn )
{
    for ( int idx=0; idx<scenes.size(); idx++ )
	scenes[idx]->lv->setSensitive( !yn );
}


int uiODSceneMgr::addPickSetItem( const PickSet& ps, int sceneid )
{
    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	Scene& scene = *scenes[idx];
	if ( sceneid >= 0 && sceneid != scene.sovwr->sceneId() ) continue;

	uiODPickSetTreeItem* itm = new uiODPickSetTreeItem( ps );
	scene.itemmanager->addChild( itm );
	return itm->displayID();
    }

    return -1;
}


int uiODSceneMgr::addEMItem( const EM::ObjectID& emid, int sceneid )
{
    const char* type = applMgr().EMServer()->getType(emid);
    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	Scene& scene = *scenes[idx];
	if ( sceneid >= 0 && sceneid != scene.sovwr->sceneId() ) continue;

	uiODDisplayTreeItem* itm;
	if ( !strcmp( type, "Horizon" ) ) itm = new uiODHorizonTreeItem(emid);
	else if ( !strcmp(type,"Fault" ) ) itm = new uiODFaultTreeItem(emid);
	else if ( !strcmp(type,"Horizontal Tube") )
	    itm = new uiODBodyTreeItem(emid);

	scene.itemmanager->addChild( itm );
	return itm->displayID();
    }

    return -1;
}


void uiODSceneMgr::removeTreeItem( int displayid )
{
    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	Scene& scene = *scenes[idx];
	uiTreeItem* itm = scene.itemmanager->findChild( displayid );
	if ( itm ) scene.itemmanager->removeChild( itm );
    }
}


uiODSceneMgr::Scene::Scene( uiWorkSpace* wsp )
        : lv(0)
        , sovwr(0)
    	, itemmanager( 0 )
{
    if ( !wsp ) return;

    uiGroup* grp = new uiGroup( wsp );
    grp->setPrefWidth( 200 );
    grp->setPrefHeight( 200 );
    sovwr = new uiSoViewer( grp );
    sovwr->setPrefWidth( 200 );
    sovwr->setPrefHeight( 200 );
    sovwr->setIcon( scene_xpm_data );
}


uiODSceneMgr::Scene::~Scene()
{
    delete sovwr;
    delete itemmanager;
    delete treeWin();
}


uiGroup* uiODSceneMgr::Scene::vwrGroup()
{
    return sovwr ? (uiGroup*)sovwr->parent() : 0;
}


uiDockWin* uiODSceneMgr::Scene::treeWin()
{
    return lv ? (uiDockWin*)lv->parent() : 0;
}

