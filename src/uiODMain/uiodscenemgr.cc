/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "od_helpids.h"
#include "uiodscenemgr.h"
#include "scene.xpm"

#include "uiattribpartserv.h"
#include "uiempartserv.h"
#include "uiodapplmgr.h"
#include "uipickpartserv.h"
#include "uivispartserv.h"
#include "uiwellattribpartserv.h"

#include "uibuttongroup.h"
#include "uidockwin.h"
#include "uigeninputdlg.h"
#include "uimdiarea.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uiodviewer2dmgr.h"
#include "uiprintscenedlg.h"
#include "ui3dviewer.h"
#include "uiscenepropdlg.h"
#include "uistatusbar.h"
#include "uistrings.h"
#include "uitoolbar.h"
#include "uitreeitemmanager.h"
#include "uitreeview.h"
#include "uiviscoltabed.h"
#include "uiwindowgrabber.h"

#include "emfaultstickset.h"
#include "emfault3d.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emmarchingcubessurface.h"
#include "emrandomposbody.h"
#include "ioman.h"
#include "ioobj.h"
#include "pickset.h"
#include "ptrman.h"
#include "randomlinegeom.h"
#include "sorting.h"
#include "settings.h"
#include "vissurvscene.h"
#include "vissurvobj.h"
#include "welltransl.h"

// For factories
#include "uiodannottreeitem.h"
#include "uiodbodydisplaytreeitem.h"
#include "uioddatatreeitem.h"
#include "uiodemsurftreeitem.h"
#include "uiodfaulttreeitem.h"
#include "uiodhortreeitem.h"
#include "uiodpicksettreeitem.h"
#include "uiodplanedatatreeitem.h"
#include "uiodpseventstreeitem.h"
#include "uiodrandlinetreeitem.h"
#include "uiodseis2dtreeitem.h"
#include "uiodscenetreeitem.h"
#include "uiodvolrentreeitem.h"
#include "uiodwelltreeitem.h"

#define mPosField	0
#define mValueField	1
#define mNameField	2
#define mStatusField	3

static const int cWSWidth = 600;
static const int cWSHeight = 500;
static const char* scenestr = "Scene ";
static const char* sKeyWarnStereo = "Warning.Stereo Viewing";

#define mWSMCB(fn) mCB(this,uiODSceneMgr,fn)
#define mDoAllScenes(memb,fn,arg) \
    for ( int idx=0; idx<scenes_.size(); idx++ ) \
	scenes_[idx]->memb->fn( arg )

uiODSceneMgr::uiODSceneMgr( uiODMain* a )
    : appl_(*a)
    , mdiarea_(new uiMdiArea(a,"OpendTect work space"))
    , vwridx_(0)
    , tifs_(new uiTreeFactorySet)
    , wingrabber_(new uiWindowGrabber(a))
    , activeSceneChanged(this)
    , sceneClosed(this)
    , treeToBeAdded(this)
    , viewModeChanged(this)
    , tiletimer_(new Timer)
{
    tifs_->addFactory( new uiODInlineTreeItemFactory, 1000,
		       SurveyInfo::No2D );
    tifs_->addFactory( new uiODCrosslineTreeItemFactory, 1100,
		       SurveyInfo::No2D );
    tifs_->addFactory( new uiODZsliceTreeItemFactory, 1200,
		       SurveyInfo::No2D );
    tifs_->addFactory( new uiODVolrenTreeItemFactory, 1500, SurveyInfo::No2D );
    tifs_->addFactory( new uiODRandomLineTreeItemFactory, 2000,
		       SurveyInfo::No2D );
    tifs_->addFactory( new Line2DTreeItemFactory, 3000, SurveyInfo::Only2D );
    tifs_->addFactory( new uiODHorizonTreeItemFactory, 4000,
		       SurveyInfo::Both2DAnd3D );
    tifs_->addFactory( new uiODHorizon2DTreeItemFactory, 4500,
		       SurveyInfo::Only2D );
    tifs_->addFactory( new uiODFaultTreeItemFactory, 5000 );
    tifs_->addFactory( new uiODFaultStickSetTreeItemFactory, 5500,
		       SurveyInfo::Both2DAnd3D );
    tifs_->addFactory( new uiODBodyDisplayTreeItemFactory, 6000,
		       SurveyInfo::No2D );
    tifs_->addFactory( new uiODWellTreeItemFactory, 7000,
		       SurveyInfo::Both2DAnd3D );
    tifs_->addFactory( new uiODPickSetTreeItemFactory, 8000,
		       SurveyInfo::Both2DAnd3D );
    tifs_->addFactory( new uiODPolygonTreeItemFactory, 8500,
		       SurveyInfo::Both2DAnd3D );
    tifs_->addFactory( new uiODPSEventsTreeItemFactory, 9000,
		       SurveyInfo::Both2DAnd3D );
    tifs_->addFactory( new uiODAnnotTreeItemFactory, 10000,
		       SurveyInfo::Both2DAnd3D );

    mdiarea_->setPrefWidth( cWSWidth );
    mdiarea_->setPrefHeight( cWSHeight );

    mAttachCB( mdiarea_->windowActivated, uiODSceneMgr::mdiAreaChanged );
    mAttachCB( tiletimer_->tick, uiODSceneMgr::tileTimerCB );
}


uiODSceneMgr::~uiODSceneMgr()
{
    detachAllNotifiers();
    cleanUp( false );
    delete tifs_;
    delete mdiarea_;
    delete wingrabber_;
    delete tiletimer_;
}


void uiODSceneMgr::initMenuMgrDepObjs()
{
    if ( scenes_.isEmpty() )
	addScene(true);
}


void uiODSceneMgr::cleanUp( bool startnew )
{
    mdiarea_->closeAll();
    // closeAll() cascades callbacks which remove the scene from set

    visServ().deleteAllObjects();
    vwridx_ = 0;
    if ( startnew ) addScene(true);
}


uiODSceneMgr::Scene& uiODSceneMgr::mkNewScene()
{
    uiODSceneMgr::Scene& scn = *new uiODSceneMgr::Scene( mdiarea_ );
    scn.mdiwin_->closed().notify( mWSMCB(removeSceneCB) );
    scenes_ += &scn;
    vwridx_++;
    BufferString vwrnm( "Viewer Scene ", vwridx_ );
    scn.vwr3d_->setName( vwrnm );
    return scn;
}


int uiODSceneMgr::addScene( bool maximized, ZAxisTransform* zt,
			    const char* name )
{
    Scene& scn = mkNewScene();
    const int sceneid = visServ().addScene();
    mDynamicCastGet(visSurvey::Scene*,visscene,visServ().getObject(sceneid));
    if ( visscene && scn.vwr3d_->getPolygonSelector() )
	visscene->setPolygonSelector( scn.vwr3d_->getPolygonSelector() );
    if ( visscene && scn.vwr3d_->getSceneColTab() )
	visscene->setSceneColTab( scn.vwr3d_->getSceneColTab() );
    if ( visscene )
	mAttachCB(
	visscene->sceneboundingboxupdated,uiODSceneMgr::newSceneUpdated );

    scn.vwr3d_->setSceneID( sceneid );
    BufferString title( scenestr );
    title += vwridx_;
    scn.mdiwin_->setTitle( title );
    visServ().setObjectName( sceneid, title );
    scn.vwr3d_->display( true );
    scn.vwr3d_->setAnnotationFont( visscene ? visscene->getAnnotFont()
					    : FontData() );
    scn.vwr3d_->viewmodechanged.notify( mWSMCB(viewModeChg) );
    scn.vwr3d_->pageupdown.notify( mCB(this,uiODSceneMgr,pageUpDownPressed) );
    scn.mdiwin_->display( true, false, maximized );
    actMode(0);
    treeToBeAdded.trigger( sceneid );
    initTree( scn, vwridx_ );

    if ( scenes_.size()>1 && scenes_[0] )
    {
	scn.vwr3d_->setStereoType( scenes_[0]->vwr3d_->getStereoType() );
	scn.vwr3d_->setStereoOffset(
		scenes_[0]->vwr3d_->getStereoOffset() );
	scn.vwr3d_->showRotAxis( scenes_[0]->vwr3d_->rotAxisShown() );
	if ( !scenes_[0]->vwr3d_->isCameraPerspective() )
	    scn.vwr3d_->toggleCameraType();
	visServ().displaySceneColorbar( visServ().sceneColorbarDisplayed() );
    }
    else if ( scenes_[0] )
    {
	const bool isperspective = scenes_[0]->vwr3d_->isCameraPerspective();
	if ( appl_.menuMgrAvailable() )
	{
	    appl_.menuMgr().setCameraPixmap( isperspective );
	    appl_.menuMgr().updateAxisMode( true );
	}
	scn.vwr3d_->showRotAxis( true );
    }

    if ( name ) setSceneName( sceneid, name );

    visServ().setZAxisTransform( sceneid, zt, 0 );

    visServ().turnSelectionModeOn( visServ().isSelectionModeOn() );
    return sceneid;
}


void uiODSceneMgr::newSceneUpdated( CallBacker* cb )
{
    if ( scenes_.size() >0 && scenes_.last()->vwr3d_ )
    {
	scenes_.last()->vwr3d_->viewAll( false );
	tiletimer_->start( 10,true );

	visBase::DataObject* obj =
	    visBase::DM().getObject( scenes_.last()->vwr3d_->sceneID() );

	mDynamicCastGet( visSurvey::Scene*,visscene,obj );

	mDetachCB(
	    visscene->sceneboundingboxupdated,uiODSceneMgr::newSceneUpdated );
    }
}


void uiODSceneMgr::tileTimerCB( CallBacker* )
{
    if ( scenes_.size() > 1 )
	tile();
}


void uiODSceneMgr::removeScene( uiODSceneMgr::Scene& scene )
{
    appl_.colTabEd().setColTab( 0, mUdf(int), mUdf(int) );
    appl_.removeDockWindow( scene.dw_ );

    if ( scene.itemmanager_ )
    {
	visBase::DataObject* obj =
	    visBase::DM().getObject( scene.itemmanager_->sceneID() );
	mDynamicCastGet( visSurvey::Scene*,visscene,obj );
	mDetachCB(
	    visscene->sceneboundingboxupdated,uiODSceneMgr::newSceneUpdated );
	scene.itemmanager_->askContinueAndSaveIfNeeded( false );
	scene.itemmanager_->prepareForShutdown();
	visServ().removeScene( scene.itemmanager_->sceneID() );
	sceneClosed.trigger( scene.itemmanager_->sceneID() );
    }

    scene.mdiwin_->closed().remove( mWSMCB(removeSceneCB) );
    scenes_ -= &scene;
    delete &scene;
}


void uiODSceneMgr::removeSceneCB( CallBacker* cb )
{
    mDynamicCastGet(uiGroupObj*,grp,cb)
    if ( !grp ) return;
    int idxnr = -1;
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	if ( grp == scenes_[idx]->mdiwin_->mainObject() )
	{
	    idxnr = idx;
	    break;
	}
    }
    if ( idxnr < 0 ) return;

    uiODSceneMgr::Scene* scene = scenes_[idxnr];
    removeScene( *scene );
}


void uiODSceneMgr::setSceneName( int sceneid, const char* nm )
{
    visServ().setObjectName( sceneid, nm );
    Scene* scene = getScene( sceneid );
    if ( !scene ) return;

    scene->mdiwin_->setTitle( nm );
    scene->dw_->setDockName( nm );
    uiTreeItem* itm = scene->itemmanager_->findChild( sceneid );
    if ( itm )
	itm->updateColumnText( uiODSceneMgr::cNameColumn() );
}


const char* uiODSceneMgr::getSceneName( int sceneid ) const
{ return const_cast<uiODSceneMgr*>(this)->visServ().getObjectName( sceneid ); }


void uiODSceneMgr::getScenePars( IOPar& iopar )
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	IOPar iop;
	scenes_[idx]->vwr3d_->fillPar( iop );
	iopar.mergeComp( iop, toString(idx) );
    }
}


void uiODSceneMgr::useScenePars( const IOPar& sessionpar )
{
    for ( int idx=0; ; idx++ )
    {
	PtrMan<IOPar> scenepar = sessionpar.subselect( toString(idx) );
	if ( !scenepar || !scenepar->size() )
	{
	    if ( !idx ) continue;
	    break;
	}

	Scene& scn = mkNewScene();
	if ( !scn.vwr3d_->usePar(*scenepar) )
	{
	    removeScene( scn );
	    continue;
	}

	visBase::DataObject* obj =
	    visBase::DM().getObject( scn.vwr3d_->sceneID() );
	mDynamicCastGet( visSurvey::Scene*,visscene,obj );

	if ( visscene )
	{
	    if ( scn.vwr3d_->getPolygonSelector() )
		visscene->setPolygonSelector(scn.vwr3d_->getPolygonSelector());
	    if ( scn.vwr3d_->getSceneColTab() )
		visscene->setSceneColTab( scn.vwr3d_->getSceneColTab() );
	}

	visServ().displaySceneColorbar( visServ().sceneColorbarDisplayed() );
	visServ().turnSelectionModeOn( false );

	BufferString title( scenestr );
	title += vwridx_;
	scn.mdiwin_->setTitle( title );
	visServ().setObjectName( scn.vwr3d_->sceneID(), title );

	scn.vwr3d_->display( true );
	scn.vwr3d_->showRotAxis( true );
	scn.vwr3d_->viewmodechanged.notify( mWSMCB(viewModeChg) );
	scn.vwr3d_->pageupdown.notify(mCB(this,uiODSceneMgr,pageUpDownPressed));
	scn.mdiwin_->display( true, false );

	treeToBeAdded.trigger( scn.vwr3d_->sceneID() );
	initTree( scn, vwridx_ );
    }

    ObjectSet<ui3DViewer> vwrs;
    get3DViewers( vwrs );
    if ( appl_.menuMgrAvailable() && !vwrs.isEmpty() && vwrs[0] )
    {
	const bool isperspective = vwrs[0]->isCameraPerspective();
	appl_.menuMgr().setCameraPixmap( isperspective );
	appl_.menuMgr().updateAxisMode( true );
    }

    rebuildTrees();

}


void uiODSceneMgr::setSceneProperties()
{
    ObjectSet<ui3DViewer> vwrs;
    get3DViewers( vwrs );
    if ( vwrs.isEmpty() )
    {
	uiMSG().error( tr("No scenes available") );
	return;
    }

    int curvwridx = 0;
    if ( vwrs.size() > 1 )
    {
	const int sceneid = askSelectScene();
	const ui3DViewer* vwr = get3DViewer( sceneid );
	if ( !vwr ) return;

	curvwridx = vwrs.indexOf( vwr );
    }

    uiScenePropertyDlg dlg( &appl_, vwrs, curvwridx );
    dlg.go();
}


void uiODSceneMgr::viewModeChg( CallBacker* cb )
{
    if ( scenes_.isEmpty() ) return;

    mDynamicCastGet(ui3DViewer*,vwr3d_,cb)
    if ( vwr3d_ ) setToViewMode( vwr3d_->isViewMode() );
}


void uiODSceneMgr::setToViewMode( bool yn )
{
    mDoAllScenes(vwr3d_,setViewMode,yn);
    visServ().setViewMode( yn , false );
    if ( appl_.menuMgrAvailable() )
	appl_.menuMgr().updateViewMode( yn );

    updateStatusBar();
    viewModeChanged.trigger();
}


bool uiODSceneMgr::inViewMode() const
{ return scenes_.isEmpty() ? false : scenes_[0]->vwr3d_->isViewMode(); }


void uiODSceneMgr::setToWorkMode( uiVisPartServer::WorkMode wm )
{
    bool yn = ( wm == uiVisPartServer::View ) ? true : false;

    mDoAllScenes(vwr3d_,setViewMode,yn);
    if ( appl_.menuMgrAvailable() )
	appl_.menuMgr().updateViewMode( yn );

    visServ().setWorkMode( wm , false );
    updateStatusBar();
}


void uiODSceneMgr::actMode( CallBacker* )
{
    setToWorkMode( uiVisPartServer::Interactive );
}


void uiODSceneMgr::viewMode( CallBacker* )
{
    setToWorkMode( uiVisPartServer::View );
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
	appl_.statusBar()->message( uiStrings::sEmptyString(), mPosField );
	appl_.statusBar()->message( uiStrings::sEmptyString(), mValueField );
	appl_.statusBar()->message( uiStrings::sEmptyString(), mNameField );
	appl_.statusBar()->message( uiStrings::sEmptyString(), mStatusField );
	appl_.statusBar()->setBGColor( mStatusField,
				   appl_.statusBar()->getBGColor(mPosField) );
    }

    const Coord3 xytpos = visServ().getMousePos( true );
    const bool haspos = xytpos.isDefined();

    BufferString msg;
    if ( haspos  )
    {
	const BinID bid( SI().transform( Coord(xytpos.x,xytpos.y) ) );
	msg = bid.toString();
	msg += "   (";
	msg += mNINT32(xytpos.x); msg += ", ";
	msg += mNINT32(xytpos.y); msg += ", ";

	const float zfact = mCast(float,visServ().zFactor());
	float zval = (float) (zfact * xytpos.z);
	if ( zfact>100 || zval>10 ) zval = mCast( float, mNINT32(zval) );
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
    scenes_[0]->vwr3d_->getAllKeyBindings( keyset );

    StringListInpSpec* inpspec = new StringListInpSpec( keyset );
    inpspec->setText( scenes_[0]->vwr3d_->getCurrentKeyBindings(), 0 );
    uiGenInputDlg dlg( &appl_, tr("Select Mouse Controls"),
		       uiStrings::sSelect(true), inpspec );
    dlg.setHelpKey(mODHelpKey(mODSceneMgrsetKeyBindingsHelpID) );
    if ( dlg.go() )
	mDoAllScenes(vwr3d_,setKeyBindings,dlg.text());
}


int uiODSceneMgr::getStereoType() const
{
    return scenes_.size() ? (int)scenes_[0]->vwr3d_->getStereoType() : 0;
}


void uiODSceneMgr::setStereoType( int type )
{
    if ( scenes_.isEmpty() ) return;

    if ( type )
    {
	if ( !Settings::common().isFalse(sKeyWarnStereo) )
	{
	    bool wantmsg = uiMSG().showMsgNextTime(
		tr("Stereo viewing is not officially supported."
		"\nIt may not work well for your particular graphics setup.") );
	    if ( !wantmsg )
	    {
		Settings::common().setYN( sKeyWarnStereo, false );
		Settings::common().write();
	    }
	}
    }

    ui3DViewer::StereoType stereotype = (ui3DViewer::StereoType)type;
    const float stereooffset = scenes_[0]->vwr3d_->getStereoOffset();
    for ( int ids=0; ids<scenes_.size(); ids++ )
    {
	ui3DViewer& vwr3d_ = *scenes_[ids]->vwr3d_;
	if ( !vwr3d_.setStereoType(stereotype) )
	{
	    uiMSG().error( tr("No support for this type of stereo rendering") );
	    return;
	}
	if ( type )
	    vwr3d_.setStereoOffset( stereooffset );
    }

    if ( type>0 )
	applMgr().setStereoOffset();
}


void uiODSceneMgr::tile()		{ mdiarea_->tile(); }
void uiODSceneMgr::tileHorizontal()	{ mdiarea_->tileHorizontal(); }
void uiODSceneMgr::tileVertical()	{ mdiarea_->tileVertical(); }
void uiODSceneMgr::cascade()		{ mdiarea_->cascade(); }


void uiODSceneMgr::layoutScenes()
{
    const int nrgrps = scenes_.size();
    if ( nrgrps == 1 && scenes_[0] )
	scenes_[0]->mdiwin_->display( true, false, true );
    else if ( nrgrps>1 && scenes_[0] )
	tile();
}


void uiODSceneMgr::toHomePos( CallBacker* )
{ mDoAllScenes(vwr3d_,toHomePos,); }
void uiODSceneMgr::saveHomePos( CallBacker* )
{ mDoAllScenes(vwr3d_,saveHomePos,); }
void uiODSceneMgr::viewAll( CallBacker* )
{ mDoAllScenes(vwr3d_,viewAll,); }
void uiODSceneMgr::align( CallBacker* )
{ mDoAllScenes(vwr3d_,align,); }

void uiODSceneMgr::viewX( CallBacker* )
{ mDoAllScenes(vwr3d_,viewPlane,ui3DViewer::X); }
void uiODSceneMgr::viewY( CallBacker* )
{ mDoAllScenes(vwr3d_,viewPlane,ui3DViewer::Y); }
void uiODSceneMgr::viewZ( CallBacker* )
{ mDoAllScenes(vwr3d_,viewPlane,ui3DViewer::Z); }
void uiODSceneMgr::viewInl( CallBacker* )
{ mDoAllScenes(vwr3d_,viewPlane,ui3DViewer::Inl); }
void uiODSceneMgr::viewCrl( CallBacker* )
{ mDoAllScenes(vwr3d_,viewPlane,ui3DViewer::Crl); }

void uiODSceneMgr::setViewSelectMode( int md )
{
    mDoAllScenes(vwr3d_,viewPlane,(ui3DViewer::PlaneType)md);
}


void uiODSceneMgr::showRotAxis( CallBacker* cb )
{
    mDynamicCastGet(const uiAction*,act,cb)
    mDoAllScenes(vwr3d_,showRotAxis,act?act->isChecked():false);
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	const Color& col = applMgr().visServer()->getSceneAnnotCol( idx );
	scenes_[idx]->vwr3d_->setAnnotationColor( col );
    }
}


class uiSnapshotDlg : public uiDialog
{
public:
			uiSnapshotDlg(uiParent*);

    enum		SnapshotType { Scene=0, Window, Desktop };
    SnapshotType	getSnapshotType() const;

protected:
    uiButtonGroup*	butgrp_;
};


uiSnapshotDlg::uiSnapshotDlg( uiParent* p )
    : uiDialog( p, uiDialog::Setup("Specify snapshot",
				   "Select area to take snapshot",
                                   mODHelpKey(mSnapshotDlgHelpID) ) )
{
    butgrp_ = new uiButtonGroup( this, "Area type", OD::Vertical );
    butgrp_->setExclusive( true );
    new uiRadioButton( butgrp_, "Scene" );
    new uiRadioButton( butgrp_, "Window" );
    new uiRadioButton( butgrp_, "Desktop" );
    butgrp_->selectButton( 0 );
}


uiSnapshotDlg::SnapshotType uiSnapshotDlg::getSnapshotType() const
{ return (uiSnapshotDlg::SnapshotType) butgrp_->selectedId(); }


void uiODSceneMgr::mkSnapshot( CallBacker* )
{
    uiSnapshotDlg snapdlg( &appl_ );
    if ( !snapdlg.go() )
	return;

    if ( snapdlg.getSnapshotType() == uiSnapshotDlg::Scene )
    {

	ObjectSet<ui3DViewer> viewers;
	get3DViewers( viewers );
	if ( viewers.size() == 0 ) return;

	uiPrintSceneDlg printdlg( &appl_, viewers );
	printdlg.go();
    }
    else
    {
	const bool desktop = snapdlg.getSnapshotType()==uiSnapshotDlg::Desktop;
	wingrabber_->grabDesktop( desktop );
	wingrabber_->go();
    }
}


void uiODSceneMgr::soloMode( CallBacker* )
{
    if ( !appl_.menuMgrAvailable() )
	return;

    TypeSet< TypeSet<int> > dispids;
    int selectedid = -1;

    const bool issolomodeon = appl_.menuMgr().isSoloModeOn();
    for ( int idx=0; idx<scenes_.size(); idx++ )
	dispids += scenes_[idx]->itemmanager_->getDisplayIds( selectedid,
							      !issolomodeon );

    visServ().setSoloMode( issolomodeon, dispids, selectedid );
    updateSelectedTreeItem();
}


void uiODSceneMgr::switchCameraType( CallBacker* )
{
    ObjectSet<ui3DViewer> vwrs;
    get3DViewers( vwrs );
    if ( vwrs.isEmpty() ) return;
    mDoAllScenes(vwr3d_,toggleCameraType,);
    const bool isperspective = vwrs[0]->isCameraPerspective();
    if ( appl_.menuMgrAvailable() )
	appl_.menuMgr().setCameraPixmap( isperspective );
}


int uiODSceneMgr::askSelectScene() const
{
    BufferStringSet scenenms; TypeSet<int> sceneids;
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	int sceneid = scenes_[idx]->itemmanager_->sceneID();
	sceneids += sceneid;
	scenenms.add( getSceneName(sceneid) );
    }

    if ( sceneids.size() < 2 )
	return sceneids.isEmpty() ? -1 : sceneids[0];

    StringListInpSpec* inpspec = new StringListInpSpec( scenenms );
    uiGenInputDlg dlg( &appl_, tr("Choose scene"), "", inpspec );
    const int selidx = dlg.go() ? dlg.getIntValue() : -1;
    return sceneids.validIdx(selidx) ? sceneids[selidx] : -1;
}


void uiODSceneMgr::get3DViewers( ObjectSet<ui3DViewer>& vwrs )
{
    vwrs.erase();
    for ( int idx=0; idx<scenes_.size(); idx++ )
	vwrs += scenes_[idx]->vwr3d_;
}


const ui3DViewer* uiODSceneMgr::get3DViewer( int sceneid ) const
{
    const Scene* scene = getScene( sceneid );
    return scene ? scene->vwr3d_ : 0;
}


ui3DViewer* uiODSceneMgr::get3DViewer( int sceneid )
{
    const Scene* scene = getScene( sceneid );
    return scene ? scene->vwr3d_ : 0;
}


uiODTreeTop* uiODSceneMgr::getTreeItemMgr( const uiTreeView* lv ) const
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	if ( scenes_[idx]->lv_ == lv )
	    return scenes_[idx]->itemmanager_;
    }
    return 0;
}


void uiODSceneMgr::getSceneNames( BufferStringSet& nms, int& active ) const
{
    uiStringSet windownames;
    mdiarea_->getWindowNames( windownames );

    nms.setEmpty();
    for ( int idx=0; idx<windownames.size(); idx++ )
    {
	nms.add( windownames[idx].getFullString() );
    }

    const char* activenm = mdiarea_->getActiveWin();
    active = nms.indexOf( activenm );
}


void uiODSceneMgr::getActiveSceneName( BufferString& nm ) const
{ nm = mdiarea_->getActiveWin(); }


int uiODSceneMgr::getActiveSceneID() const
{
    const BufferString scenenm = mdiarea_->getActiveWin();
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	if ( !scenes_[idx] || !scenes_[idx]->itemmanager_ )
	    continue;

	if ( scenenm == getSceneName(scenes_[idx]->itemmanager_->sceneID()) )
	    return scenes_[idx]->itemmanager_->sceneID();
    }

    return -1;
}


void uiODSceneMgr::mdiAreaChanged( CallBacker* )
{
//    const bool wasparalysed = mdiarea_->paralyse( true );
    if ( appl_.menuMgrAvailable() )
	appl_.menuMgr().updateSceneMenu();
//    mdiarea_->paralyse( wasparalysed );
    activeSceneChanged.trigger();
}


void uiODSceneMgr::setActiveScene( const char* scenenm )
{
    mdiarea_->setActiveWin( scenenm );
    activeSceneChanged.trigger();
}


void uiODSceneMgr::initTree( Scene& scn, int vwridx )
{
    BufferString capt( "Tree scene " ); capt += vwridx;
    scn.dw_ = new uiDockWin( &appl_, capt );
    scn.dw_->setMinimumWidth( 200 );
    scn.lv_ = new uiTreeView( scn.dw_, capt );
    scn.dw_->setObject( scn.lv_ );
    BufferStringSet labels;
    labels.add( "Elements" );
    labels.add( "Color" );
    scn.lv_->addColumns( labels );
    scn.lv_->setFixedColumnWidth( cColorColumn(), 40 );

    scn.itemmanager_ = new uiODTreeTop( scn.vwr3d_, scn.lv_, &applMgr(), tifs_);
    uiODSceneTreeItem* sceneitm =
	new uiODSceneTreeItem( scn.mdiwin_->getTitle().getFullString(),
			       scn.vwr3d_->sceneID() );
    scn.itemmanager_->addChild( sceneitm, false );

    TypeSet<int> idxs;
    TypeSet<int> placeidxs;

    for ( int idx=0; idx<tifs_->nrFactories(); idx++ )
    {
	SurveyInfo::Pol2D pol2d = (SurveyInfo::Pol2D)tifs_->getPol2D( idx );
	if ( SI().survDataType() == SurveyInfo::Both2DAnd3D ||
	     pol2d == SurveyInfo::Both2DAnd3D ||
	     pol2d == SI().survDataType() )
	{
	    idxs += idx;
	    placeidxs += tifs_->getPlacementIdx( idx );
	}
    }

    sort_coupled( placeidxs.arr(), idxs.arr(), idxs.size() );

    for ( int idx=0; idx<idxs.size(); idx++ )
    {
	const int fidx = idxs[idx];
	scn.itemmanager_->addChild(
		tifs_->getFactory(fidx)->create(), true );
    }

    scn.lv_->display( true );
    appl_.addDockWindow( *scn.dw_, uiMainWin::Left );
    scn.dw_->display( true );
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
	const int sceneid = scene.vwr3d_->sceneID();
	TypeSet<int> visids; visServ().getChildIds( sceneid, visids );

	for ( int idy=0; idy<visids.size(); idy++ )
	{
	    mDynamicCastGet( const visSurvey::SurveyObject*, surobj,
		visServ().getObject(visids[idy]) );

	    if ( surobj && surobj->getSaveInSessionsFlag() == false )
		continue;

	    uiODDisplayTreeItem::create( scene.itemmanager_, &applMgr(),
					 visids[idy] );
	}
    }
    updateSelectedTreeItem();
}


uiTreeView* uiODSceneMgr::getTree( int sceneid )
{
    Scene* scene = getScene( sceneid );
    return scene ? scene->lv_ : 0;
}


void uiODSceneMgr::setItemInfo( int id )
{
    mDoAllScenes(itemmanager_,updateColumnText,cNameColumn());
    mDoAllScenes(itemmanager_,updateColumnText,cColorColumn());
    appl_.statusBar()->message( uiStrings::sEmptyString(), mPosField );
    appl_.statusBar()->message( uiStrings::sEmptyString(), mValueField );
    appl_.statusBar()->message( visServ().getInteractionMsg(id), mNameField );
    appl_.statusBar()->message( uiStrings::sEmptyString(), mStatusField );
    appl_.statusBar()->setBGColor( mStatusField,
				   appl_.statusBar()->getBGColor(mPosField) );
}


void uiODSceneMgr::updateItemToolbar( int id )
{
    visServ().getToolBarHandler()->setMenuID( id );
    visServ().getToolBarHandler()->executeMenu(); // addButtons
}


void uiODSceneMgr::updateSelectedTreeItem()
{
    const int id = visServ().getSelObjectId();
    updateItemToolbar( id );

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
    const bool wasparalysed = mdiarea_->paralyse( true );

    for ( int idx=0; idx<scenes_.size(); idx++ )
	scenes_[idx]->lv_->setSensitive( !yn );

    mdiarea_->paralyse( wasparalysed );
}


#define mGetOrAskForScene \
    Scene* scene = getScene( sceneid ); \
    if ( !scene ) \
    { \
	sceneid = askSelectScene(); \
	scene = getScene( sceneid ); \
    } \
    if ( !scene ) return -1;

int uiODSceneMgr::addWellItem( const MultiID& mid, int sceneid )
{
    mGetOrAskForScene

    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !scene || !ioobj ) return -1;

    if ( ioobj->group() != mTranslGroupName(Well) )
	return -1;

    uiODDisplayTreeItem* itm = new uiODWellTreeItem( mid );
    scene->itemmanager_->addChild( itm, false );
    return itm->displayID();
}


void uiODSceneMgr::getLoadedEMIDs( TypeSet<int>& emids, const char* type,
				   int sceneid ) const
{
    if ( sceneid>=0 )
    {
	const Scene* scene = getScene( sceneid );
	if ( !scene ) return;
	gtLoadedEMIDs( scene, emids, type );
	return;
    }

    for ( int idx=0; idx<scenes_.size(); idx++ )
	gtLoadedEMIDs( scenes_[idx], emids, type );
}


void uiODSceneMgr::gtLoadedEMIDs( const uiTreeItem* topitm, TypeSet<int>& emids,
				  const char* type ) const
{
    for ( int chidx=0; chidx<topitm->nrChildren(); chidx++ )
    {
	const uiTreeItem* chlditm = topitm->getChild( chidx );
	mDynamicCastGet(const uiODEarthModelSurfaceTreeItem*,emtreeitem,chlditm)
	if ( !emtreeitem )
	    continue;

	EM::ObjectID emid = emtreeitem->emObjectID();
	if ( !type )
	    emids.addIfNew( emid );
	else if ( EM::Horizon3D::typeStr()==type )
	{
	    mDynamicCastGet(const uiODHorizonTreeItem*,hor3dtreeitm,chlditm)
	    if ( hor3dtreeitm )
		emids.addIfNew( emid );
	}
	else if ( EM::Horizon2D::typeStr()==type )
	{
	    mDynamicCastGet(const uiODHorizon2DTreeItem*,hor2dtreeitm,chlditm)
	    if ( hor2dtreeitm )
		emids.addIfNew( emid );
	}
	else if ( EM::Fault3D::typeStr()==type )
	{
	    mDynamicCastGet(const uiODFaultTreeItem*,faulttreeitm,chlditm)
	    if ( faulttreeitm )
		emids.addIfNew( emid );
	}
	else if ( EM::FaultStickSet::typeStr()==type )
	{
	    mDynamicCastGet(const uiODFaultStickSetTreeItem*,fltstickreeitm,
		    	    chlditm)
	    if ( fltstickreeitm )
		emids.addIfNew( emid );
	}
    }
}


void uiODSceneMgr::gtLoadedEMIDs( const Scene* scene, TypeSet<int>& emids,
				  const char* type ) const
{
    for ( int chidx=0; chidx<scene->itemmanager_->nrChildren(); chidx++ )
    {
	const uiTreeItem* chlditm = scene->itemmanager_->getChild( chidx );
	gtLoadedEMIDs( chlditm, emids, type );
    }
}


int uiODSceneMgr::addEMItem( const EM::ObjectID& emid, int sceneid )
{
    mGetOrAskForScene

    FixedString type = applMgr().EMServer()->getType( emid );
    uiODDisplayTreeItem* itm;
    if ( type==EM::Horizon3D::typeStr() )
	itm = new uiODHorizonTreeItem(emid,false,false);
    else if ( type==EM::Horizon2D::typeStr() )
	itm = new uiODHorizon2DTreeItem(emid);
    else if ( type==EM::Fault3D::typeStr() )
	itm = new uiODFaultTreeItem(emid);
    else if ( type==EM::FaultStickSet::typeStr() )
	itm = new uiODFaultStickSetTreeItem(emid);
    else if ( type==EM::RandomPosBody::typeStr() )
	itm = new uiODBodyDisplayTreeItem(emid);
    else if ( type==EM::MarchingCubesSurface::typeStr() )
	itm = new uiODBodyDisplayTreeItem(emid);
    else
	return -1;

    scene->itemmanager_->addChild( itm, false );
    return itm->displayID();
}


int uiODSceneMgr::addPickSetItem( const MultiID& mid, int sceneid )
{
    Pick::Set* ps = applMgr().pickServer()->loadSet( mid );
    if ( !ps ) return -1;

    return addPickSetItem( *ps, sceneid );
}


int uiODSceneMgr::addPickSetItem( Pick::Set& ps, int sceneid )
{
    mGetOrAskForScene

    uiODPickSetTreeItem* itm = new uiODPickSetTreeItem( -1, ps );
    scene->itemmanager_->addChild( itm, false );
    return itm->displayID();
}


int uiODSceneMgr::addRandomLineItem( const Geometry::RandomLineSet& rl,
				     int sceneid )
{
    mGetOrAskForScene

    uiODRandomLineTreeItem* itm = new uiODRandomLineTreeItem( rl );
    scene->itemmanager_->addChild( itm, false );
    itm->displayDefaultData();
    return itm->displayID();
}


int uiODSceneMgr::add2DLineItem( Pos::GeomID geomid, int sceneid )
{
    mGetOrAskForScene

    uiOD2DLineTreeItem* itm = new uiOD2DLineTreeItem( geomid );
    scene->itemmanager_->addChild( itm, false );
    return itm->displayID();
}


int uiODSceneMgr::add2DLineItem( const MultiID& mid , int sceneid )
{
    mGetOrAskForScene
    const Survey::Geometry* geom = Survey::GM().getGeometry( mid );
    if ( !geom ) return -1;

    const Pos::GeomID geomid = geom->getID();
    uiOD2DLineTreeItem* itm = new uiOD2DLineTreeItem( geomid );
    scene->itemmanager_->addChild( itm, false );
    return itm->displayID();
}


int uiODSceneMgr::addInlCrlItem( OD::SliceType st, int nr, int sceneid )
{
    mGetOrAskForScene
    uiODPlaneDataTreeItem* itm = 0;
    TrcKeyZSampling tkzs = SI().sampling(true);
    if ( st == OD::InlineSlice )
    {
	itm = new uiODInlineTreeItem( -1, uiODPlaneDataTreeItem::Empty );
	tkzs.hsamp_.setInlRange( Interval<int>(nr,nr) );
    }
    else if ( st == OD::CrosslineSlice )
    {
	itm = new uiODCrosslineTreeItem( -1, uiODPlaneDataTreeItem::Empty );
	tkzs.hsamp_.setCrlRange( Interval<int>(nr,nr) );
    }
    else
	return -1;

    if ( !scene->itemmanager_->addChild(itm,false) )
	return -1;

    itm->setTrcKeyZSampling( tkzs );
    itm->displayDefaultData();
    return itm->displayID();
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
    appl_.viewer2DMgr().displayIn2DViewer( visid, attribid, dowva );
}


void uiODSceneMgr::remove2DViewer( int visid )
{
    appl_.viewer2DMgr().remove2DViewer( visid, true );
}


void uiODSceneMgr::doDirectionalLight(CallBacker*)
{
    visServ().setDirectionalLight();
}


uiODSceneMgr::Scene* uiODSceneMgr::getScene( int sceneid )
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	uiODSceneMgr::Scene* scn = scenes_[idx];
	if ( scn && scn->itemmanager_ &&
		scn->itemmanager_->sceneID() == sceneid )
	    return scenes_[idx];
    }

    return 0;
}


const uiODSceneMgr::Scene* uiODSceneMgr::getScene( int sceneid ) const
{ return const_cast<uiODSceneMgr*>(this)->getScene( sceneid ); }


// uiODSceneMgr::Scene
uiODSceneMgr::Scene::Scene( uiMdiArea* mdiarea )
        : lv_(0)
	, dw_(0)
	, mdiwin_(0)
        , vwr3d_(0)
	, itemmanager_(0)
{
    if ( !mdiarea ) return;

    mdiwin_ = new uiMdiAreaWindow( *mdiarea );
    mdiwin_->setIcon( scene_xpm_data );
    vwr3d_ = new ui3DViewer( mdiwin_, true );
    vwr3d_->setPrefWidth( 400 );
    vwr3d_->setPrefHeight( 400 );
    mdiarea->addWindow( mdiwin_ );
}


uiODSceneMgr::Scene::~Scene()
{
    delete vwr3d_;
    delete itemmanager_;
    delete dw_;
}
