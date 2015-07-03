/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiodviewer2d.h"

#include "uiattribpartserv.h"
#include "uiflatauxdataeditor.h"
#include "uiflatviewdockwin.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uiflatviewslicepos.h"
#include "uiflatviewstdcontrol.h"
#include "uimenu.h"
#include "uiodmain.h"
#include "uiodviewer2dmgr.h"
#include "uiodvw2dtreeitem.h"
#include "uiodvw2dhor3dtreeitem.h"
#include "uiodvw2dhor2dtreeitem.h"
#include "uipixmap.h"
#include "uistrings.h"
#include "uitoolbar.h"
#include "uitreeview.h"
#include "uivispartserv.h"

#include "filepath.h"
#include "flatposdata.h"
#include "ioobj.h"
#include "mouseevent.h"
#include "seisdatapack.h"
#include "seisdatapackzaxistransformer.h"
#include "settings.h"
#include "sorting.h"
#include "survinfo.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "posvecdataset.h"

#include "zaxistransform.h"
#include "zaxistransformutils.h"
#include "view2ddataman.h"
#include "view2ddata.h"
#include "od_helpids.h"

static void initSelSpec( Attrib::SelSpec& as )
{ as.set( 0, Attrib::SelSpec::cNoAttrib(), false, 0 ); }

mDefineInstanceCreatedNotifierAccess( uiODViewer2D )

uiODViewer2D::uiODViewer2D( uiODMain& appl, int visid )
    : appl_(appl)
    , visid_(visid)
    , vdselspec_(*new Attrib::SelSpec)
    , wvaselspec_(*new Attrib::SelSpec)
    , viewwin_(0)
    , slicepos_(0)
    , viewstdcontrol_(0)
    , datamgr_(new Vw2DDataManager)
    , tifs_(0)
    , treetp_(0)
    , polyseltbid_(-1)
    , basetxt_("2D Viewer - ")
    , initialcentre_(uiWorldPoint::udf())
    , initialx1pospercm_(mUdf(float))
    , initialx2pospercm_(mUdf(float))
    , isvertical_(true)
    , ispolyselect_(true)
    , viewWinAvailable(this)
    , viewWinClosed(this)
    , dataChanged(this)
    , mousecursorexchange_(0)
    , marker_(0)
    , datatransform_(0)
{
    mDefineStaticLocalObject( Threads::Atomic<int>, vwrid, (0) );
    id_ = vwrid++;

    setWinTitle();

    initSelSpec( vdselspec_ );
    initSelSpec( wvaselspec_ );

    mTriggerInstanceCreatedNotifier();
}


uiODViewer2D::~uiODViewer2D()
{
    mDynamicCastGet(uiFlatViewDockWin*,fvdw,viewwin())
    if ( fvdw )
	appl_.removeDockWindow( fvdw );

    delete treetp_;
    delete datamgr_;

    deepErase( auxdataeditors_ );
    detachAllNotifiers();

    if ( datatransform_ )
	datatransform_->unRef();

    if ( viewwin() )
    {
	removeAvailablePacks();
	viewwin()->viewer(0).removeAuxData( marker_ );
    }
    delete marker_;
    delete viewwin();
}


Pos::GeomID uiODViewer2D::geomID() const
{
    if ( tkzs_.hsamp_.survid_ == Survey::GM().get2DSurvID() )
	return tkzs_.hsamp_.trcKeyAt(0).geomID();

    return Survey::GM().cUndefGeomID();
}


const ZDomain::Def& uiODViewer2D::zDomain() const
{
    return datatransform_ ? datatransform_->toZDomainInfo().def_
			  : SI().zDomain();
}


uiParent* uiODViewer2D::viewerParent()
{ return viewwin_->viewerParent(); }


void uiODViewer2D::setUpAux()
{
    const bool is2d = geomID() != Survey::GM().cUndefGeomID();
    FlatView::Annotation& vwrannot = viewwin()->viewer().appearance().annot_;
    if ( is2d || tkzs_.isFlat() )
    {
	vwrannot.x1_.showauxpos_ = true;
	vwrannot.x1_.showauxlines_ = true;
	if ( !is2d )
	{
	    vwrannot.x2_.showauxpos_ = true;
	    vwrannot.x2_.showauxlines_ = true;
	    uiString& x1auxnm = vwrannot.x1_.auxlabel_;
	    uiString& x2auxnm = vwrannot.x2_.auxlabel_;
	    if ( tkzs_.defaultDir()==TrcKeyZSampling::Inl )
	    {
		x1auxnm = tr( "Crossline intersections" );
		x2auxnm = tr( "ZSlice intersections" );
	    }
	    else if ( tkzs_.defaultDir()==TrcKeyZSampling::Crl )
	    {
		x1auxnm = tr( "Inline intersections" );
		x2auxnm = tr( "ZSlice intersections" );
	    }
	    else
	    {
		x1auxnm = tr( "Inline intersections" );
		x2auxnm = tr( "Crossline intersections" );
	    }
	}
	else
	{
	    vwrannot.x2_.showauxpos_ = false;
	    vwrannot.x2_.showauxlines_ = false;
	    vwrannot.x1_.auxlabel_ = tr( "2D Line intersections" );
	}
    }
    else
    {
	vwrannot.x1_.showauxpos_ = false;
	vwrannot.x1_.showauxlines_ = false;
	vwrannot.x2_.showauxpos_ = false;
	vwrannot.x2_.showauxlines_ = false;
    }
}


void uiODViewer2D::setUpView( DataPack::ID packid, bool wva )
{
    DataPackMgr& dpm = DPM(DataPackMgr::FlatID());
    ConstDataPackRef<FlatDataPack> fdp = dpm.obtain( packid );
    mDynamicCastGet(const SeisFlatDataPack*,seisfdp,fdp.ptr());
    mDynamicCastGet(const RegularFlatDataPack*,regfdp,fdp.ptr());
    mDynamicCastGet(const MapDataPack*,mapdp,fdp.ptr());

    const bool isnew = !viewwin();
    if ( isnew )
    {
	if ( regfdp && regfdp->is2D() )
	    tifs_ = ODMainWin()->viewer2DMgr().treeItemFactorySet2D();
	else if ( !mapdp )
	    tifs_ = ODMainWin()->viewer2DMgr().treeItemFactorySet3D();

	isvertical_ = seisfdp && seisfdp->isVertical();
	createViewWin( isvertical_, regfdp && !regfdp->is2D() );
    }

    if ( regfdp )
    {
	const TrcKeyZSampling& cs = regfdp->sampling();
	if ( tkzs_ != cs ) { removeAvailablePacks(); setTrcKeyZSampling( cs ); }
    }

    setDataPack( packid, wva, isnew ); adjustOthrDisp( wva, isnew );

    //updating stuff
    if ( treetp_ )
    {
	treetp_->updSelSpec( &wvaselspec_, true );
	treetp_->updSelSpec( &vdselspec_, false );
    }

    viewwin()->start();
}


void uiODViewer2D::adjustOthrDisp( bool wva, bool isnew )
{
    if ( !slicepos_ ) return;
    const TrcKeyZSampling& cs = slicepos_->getTrcKeyZSampling();
    const bool newcs = ( cs != tkzs_ );
    const DataPack::ID othrdpid = newcs ? createDataPack(!wva)
					: getDataPackID(!wva);
    if ( newcs && (othrdpid != DataPack::cNoID()) )
    { removeAvailablePacks(); setTrcKeyZSampling( cs ); }
    setDataPack( othrdpid, !wva, isnew );
}


void uiODViewer2D::setDataPack( DataPack::ID packid, bool wva, bool isnew )
{
    if ( packid == DataPack::cNoID() ) return;

    DataPackMgr& dpm = DPM(DataPackMgr::FlatID());
    for ( int ivwr=0; ivwr<viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewwin()->viewer(ivwr);
	const TypeSet<DataPack::ID> ids = vwr.availablePacks();
	if ( ids.isPresent(packid) )
	{ vwr.usePack( wva, packid, isnew ); continue; }

	const FixedString newpackname = dpm.nameOf(packid);
	bool setforotherdisp = false;
	for ( int idx=0; idx<ids.size(); idx++ )
	{
	    const FixedString packname = dpm.nameOf(ids[idx]);
	    if ( packname == newpackname )
	    {
		if ( ids[idx] == vwr.packID(!wva) )
		    setforotherdisp = true;
		vwr.removePack( ids[idx] );
		break;
	    }
	}

	vwr.setPack( wva, packid, isnew );
	if ( setforotherdisp || (isnew && wvaselspec_==vdselspec_) )
	    vwr.usePack( !wva, packid, isnew );
    }

    dataChanged.trigger( this );
}


bool uiODViewer2D::setZAxisTransform( ZAxisTransform* zat )
{
    if ( datatransform_ )
	datatransform_->unRef();

    datatransform_ = zat;
    if ( datatransform_ )
	datatransform_->ref();

    return true;
}


void uiODViewer2D::setTrcKeyZSampling( const TrcKeyZSampling& tkzs )
{
    tkzs_ = tkzs;
    if ( slicepos_ )
    {
	slicepos_->setTrcKeyZSampling( tkzs );
	slicepos_->getToolBar()->display( tkzs.isFlat() );

	if ( datatransform_ )
	{
	    TrcKeyZSampling limitcs;
	    limitcs.zsamp_.setFrom( datatransform_->getZInterval(false) );
	    limitcs.zsamp_.step = datatransform_->getGoodZStep();
	    slicepos_->setLimitSampling( limitcs );
	}
    }

    if ( tkzs.isFlat() ) setWinTitle( true );

    if ( treetp_ ) treetp_->updSampling( tkzs, true );
}


void uiODViewer2D::createViewWin( bool isvert, bool needslicepos )
{
    bool wantdock = false;
    Settings::common().getYN( "FlatView.Use Dockwin", wantdock );
    uiParent* controlparent = 0;
    if ( !wantdock )
    {
	uiFlatViewMainWin* fvmw = new uiFlatViewMainWin( &appl_,
		uiFlatViewMainWin::Setup(basetxt_).deleteonclose(true) );
	mAttachCB( fvmw->windowClosed, uiODViewer2D::winCloseCB );

	if ( needslicepos )
	{
	    slicepos_ = new uiSlicePos2DView( fvmw, ZDomain::Info(zDomain()) );
	    mAttachCB( slicepos_->positionChg, uiODViewer2D::posChg );
	}

	viewwin_ = fvmw;
	createTree( fvmw );
    }
    else
    {
	uiFlatViewDockWin* dwin = new uiFlatViewDockWin( &appl_,
				   uiFlatViewDockWin::Setup(basetxt_) );
	appl_.addDockWindow( *dwin, uiMainWin::Top );
	dwin->setFloating( true );
	viewwin_ = dwin;
	controlparent = &appl_;
    }

    viewwin_->setInitialSize( 700, 400 );
    if ( tkzs_.isFlat() ) setTrcKeyZSampling( tkzs_ );

    for ( int ivwr=0; ivwr<viewwin_->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewwin()->viewer( ivwr);
	vwr.setZAxisTransform( datatransform_ );
	vwr.appearance().setDarkBG( wantdock );
	vwr.appearance().setGeoDefaults(isvert);
	vwr.appearance().annot_.setAxesAnnot(true);
    }

    const float initialx2pospercm = isvert ? initialx2pospercm_
					   : initialx1pospercm_;
    uiFlatViewer& mainvwr = viewwin()->viewer();
    viewstdcontrol_ = new uiFlatViewStdControl( mainvwr,
	    uiFlatViewStdControl::Setup(controlparent).helpkey(
					mODHelpKey(mODViewer2DHelpID) )
					.withedit(tifs_).isvertical(isvert)
					.withfixedaspectratio(true)
					.withhomebutton(true)
					.initialx1pospercm(initialx1pospercm_)
					.initialx2pospercm(initialx2pospercm)
					.initialcentre(initialcentre_)
					.managecoltab(!tifs_) );
    mAttachCB( viewstdcontrol_->infoChanged, uiODViewer2D::mouseMoveCB );
    if ( tifs_ )
    {
	createPolygonSelBut( viewstdcontrol_->toolBar() );
	createViewWinEditors();
    }

    viewwin_->addControl( viewstdcontrol_ );
    viewWinAvailable.trigger( this );
}


void uiODViewer2D::createTree( uiMainWin* mw )
{
    if ( !mw || !tifs_ ) return;

    uiDockWin* treedoc = new uiDockWin( mw, "Tree items" );
    treedoc->setMinimumWidth( 200 );
    uiTreeView* lv = new uiTreeView( treedoc, "Tree items" );
    treedoc->setObject( lv );
    BufferStringSet labels;
    labels.add( "Elements" );
    labels.add( "Color" );
    lv->addColumns( labels );
    lv->setFixedColumnWidth( uiODViewer2DMgr::cColorColumn(), 40 );

    treetp_ = new uiODVw2DTreeTop( lv, &appl_.applMgr(), this, tifs_ );

    TypeSet<int> idxs;
    TypeSet<int> placeidxs;

    for ( int idx=0; idx < tifs_->nrFactories(); idx++ )
    {
	SurveyInfo::Pol2D pol2d = (SurveyInfo::Pol2D)tifs_->getPol2D( idx );
	if ( SI().survDataType() == SurveyInfo::Both2DAnd3D
	     || pol2d == SurveyInfo::Both2DAnd3D
	     || pol2d == SI().survDataType() )
	{
	    idxs += idx;
	    placeidxs += tifs_->getPlacementIdx(idx);
	}
    }

    sort_coupled( placeidxs.arr(), idxs.arr(), idxs.size() );

    for ( int iidx=0; iidx<idxs.size(); iidx++ )
    {
	const int fidx = idxs[iidx];
	treetp_->addChild( tifs_->getFactory(fidx)->create(), true );
    }

    treetp_->setZAxisTransform( datatransform_ );
    lv->display( true );
    mw->addDockWindow( *treedoc, uiMainWin::Left );
    treedoc->display( true );
}


void uiODViewer2D::createPolygonSelBut( uiToolBar* tb )
{
    if ( !tb ) return;

    polyseltbid_ = tb->addButton( "polygonselect", tr("Polygon Selection mode"),
				  mCB(this,uiODViewer2D,selectionMode), true );
    uiMenu* polymnu = new uiMenu( tb, tr("PoluMenu") );

    uiAction* polyitm = new uiAction( uiStrings::sPolygon(),
				      mCB(this,uiODViewer2D,handleToolClick) );
    polymnu->insertItem( polyitm, 0 );
    polyitm->setIcon( "polygonselect" );

    uiAction* rectitm = new uiAction( uiStrings::sRectangle(),
				      mCB(this,uiODViewer2D,handleToolClick) );
    polymnu->insertItem( rectitm, 1 );
    rectitm->setIcon( "rectangleselect" );

    tb->setButtonMenu( polyseltbid_, polymnu );

    tb->addButton( "trashcan", tr("Remove PolySelection"),
			mCB(this,uiODViewer2D,removeSelected), false );
}


void uiODViewer2D::createViewWinEditors()
{
    for ( int ivwr=0; ivwr<viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewwin()->viewer( ivwr);
	uiFlatViewAuxDataEditor* adeditor = new uiFlatViewAuxDataEditor( vwr );
	adeditor->setSelActive( false );
	auxdataeditors_ += adeditor;
    }
}


void uiODViewer2D::winCloseCB( CallBacker* cb )
{
    delete treetp_; treetp_ = 0;
    datamgr_->removeAll();

    deepErase( auxdataeditors_ );
    removeAvailablePacks();

    viewWinClosed.trigger();
}


void uiODViewer2D::removeAvailablePacks()
{
    if ( !viewwin() ) { pErrMsg("No main window"); return; }

    for ( int ivwr=0; ivwr<viewwin()->nrViewers(); ivwr++ )
	viewwin()->viewer(ivwr).clearAllPacks();
}


void uiODViewer2D::setSelSpec( const Attrib::SelSpec* as, bool wva )
{
    if ( as )
	(wva ? wvaselspec_ : vdselspec_) = *as;
    else
	initSelSpec( wva ? wvaselspec_ : vdselspec_ );
}


void uiODViewer2D::posChg( CallBacker* )
{
    setNewPosition( slicepos_->getTrcKeyZSampling() );
}


void uiODViewer2D::setNewPosition( const TrcKeyZSampling& tkzs )
{
    if ( tkzs_ == tkzs ) return;
    setTrcKeyZSampling( tkzs );
    const uiFlatViewer& vwr = viewwin()->viewer(0);
    if ( vwr.isVisible(false) && vdselspec_.id().isValid() )
	setUpView( createDataPack(false), false );
    else if ( vwr.isVisible(true) && wvaselspec_.id().isValid() )
	setUpView( createDataPack(true), true );
}


void uiODViewer2D::setPos( const TrcKeyZSampling& tkzs )
{
    if ( tkzs == tkzs_ ) return;
    const uiFlatViewer& vwr = viewwin()->viewer(0);
    if ( vwr.isVisible(false) && vdselspec_.id().isValid() )
	setUpView( createDataPack(false), false );
    else if ( vwr.isVisible(true) && wvaselspec_.id().isValid() )
	setUpView( createDataPack(true), true );
}


DataPack::ID uiODViewer2D::getDataPackID( bool wva ) const
{
    const uiFlatViewer& vwr = viewwin()->viewer(0);
    if ( vwr.hasPack(wva) )
	return vwr.packID(wva);
    else if ( wvaselspec_ == vdselspec_ )
    {
	const DataPack::ID dpid = vwr.packID(!wva);
	if ( dpid != DataPack::cNoID() ) return dpid;
    }
    return createDataPack( wva );
}


DataPack::ID uiODViewer2D::createDataPack( const Attrib::SelSpec& selspec )const
{
    TrcKeyZSampling tkzs = slicepos_ ? slicepos_->getTrcKeyZSampling() : tkzs_;
    if ( !tkzs.isFlat() ) return DataPack::cNoID();

    RefMan<ZAxisTransform> zat = getZAxisTransform();
    if ( zat && !selspec.isZTransformed() )
    {
	if ( tkzs.nrZ() == 1 )
	    return createDataPackForTransformedZSlice( selspec );
	tkzs.zsamp_.setFrom( zat->getZInterval(true) );
	tkzs.zsamp_.step = SI().zStep();
    }

    uiAttribPartServer* attrserv = appl_.applMgr().attrServer();
    attrserv->setTargetSelSpec( selspec );
    const DataPack::ID dpid = attrserv->createOutput( tkzs, DataPack::cNoID() );
    return createFlatDataPack( dpid, 0 );
}


DataPack::ID uiODViewer2D::createFlatDataPack(
				DataPack::ID dpid, int comp ) const
{
    DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    ConstDataPackRef<SeisDataPack> seisdp = dpm.obtain( dpid );
    if ( !seisdp ) return dpid;

    const FixedString zdomainkey( seisdp->zDomain().key() );
    const bool alreadytransformed =
	!zdomainkey.isEmpty() && zdomainkey!=ZDomain::SI().key();
    if ( datatransform_ && !alreadytransformed )
    {
	DataPack::ID outputid = DataPack::cNoID();
	SeisDataPackZAxisTransformer transformer( *datatransform_ );
	transformer.setInput( seisdp.ptr() );
	transformer.setOutput( outputid );
	transformer.setInterpolate( true );
	transformer.execute();
	if ( outputid != DataPack::cNoID() )
	    seisdp = dpm.obtain( outputid );
    }

    mDynamicCastGet(const RegularSeisDataPack*,regsdp,seisdp.ptr());
    mDynamicCastGet(const RandomSeisDataPack*,randsdp,seisdp.ptr());
    if ( regsdp )
    {
	RegularFlatDataPack* regfdp = new RegularFlatDataPack( *regsdp, comp );
	DPM(DataPackMgr::FlatID()).add( regfdp );
	return regfdp->id();
    }
    else if ( randsdp )
    {
	RandomFlatDataPack* randfdp = new RandomFlatDataPack( *randsdp, comp );
	DPM(DataPackMgr::FlatID()).add( randfdp );
	return randfdp->id();
    }

    return DataPack::cNoID();
}


DataPack::ID uiODViewer2D::createDataPackForTransformedZSlice(
					const Attrib::SelSpec& selspec ) const
{
    if ( !hasZAxisTransform() || selspec.isZTransformed() )
	return DataPack::cNoID();

    const TrcKeyZSampling& tkzs = slicepos_ ? slicepos_->getTrcKeyZSampling()
					    : tkzs_;
    if ( tkzs.nrZ() != 1 ) return DataPack::cNoID();

    uiAttribPartServer* attrserv = appl_.applMgr().attrServer();
    attrserv->setTargetSelSpec( selspec );

    DataPackRef<DataPointSet> data =
	DPM(DataPackMgr::PointID()).addAndObtain(new DataPointSet(false,true));

    ZAxisTransformPointGenerator generator( *datatransform_ );
    generator.setInput( tkzs );
    generator.setOutputDPS( *data );
    generator.execute();

    const int firstcol = data->nrCols();
    BufferStringSet userrefs; userrefs.add( selspec.userRef() );
    data->dataSet().add( new DataColDef(userrefs.get(0)) );
    if ( !attrserv->createOutput(*data,firstcol) )
	return DataPack::cNoID();

    const DataPack::ID dpid = RegularSeisDataPack::createDataPackForZSlice(
	    &data->bivSet(), tkzs, datatransform_->toZDomainInfo(), userrefs );
    return createFlatDataPack( dpid, 0 );
}


bool uiODViewer2D::useStoredDispPars( bool wva )
{
    PtrMan<IOObj> ioobj = appl_.applMgr().attrServer()->getIOObj(selSpec(wva));
    if ( !ioobj ) return false;

    FilePath fp( ioobj->fullUserExpr(true) );
    fp.setExtension( "par" );
    IOPar iop;
    if ( !iop.read(fp.fullPath(),sKey::Pars()) || iop.isEmpty() )
	return false;

    ColTab::MapperSetup mapper;
    if ( !mapper.usePar(iop) )
	return false;

    for ( int ivwr=0; ivwr<viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewwin()->viewer( ivwr );
	FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
	wva ? ddp.wva_.mappersetup_ : ddp.vd_.mappersetup_ = mapper;
	if ( !wva ) ddp.vd_.ctab_ = iop.find( sKey::Name() );
    }

    return true;
}


void uiODViewer2D::selectionMode( CallBacker* cb )
{
    if ( !viewstdcontrol_ || !viewstdcontrol_->toolBar() )
	return;

    viewstdcontrol_->toolBar()->setIcon( polyseltbid_, ispolyselect_ ?
				"polygonselect" : "rectangleselect" );
    viewstdcontrol_->toolBar()->setToolTip( polyseltbid_,
                                ispolyselect_ ? tr("Polygon Selection mode")
                                              : tr("Rectangle Selection mode"));
    const bool ispolyseltbon = viewstdcontrol_->toolBar()->isOn( polyseltbid_ );
    if ( ispolyseltbon )
	viewstdcontrol_->setEditMode( true );

    for ( int edidx=0; edidx<auxdataeditors_.size(); edidx++ )
    {
	auxdataeditors_[edidx]->setSelectionPolygonRectangle( !ispolyselect_ );
	auxdataeditors_[edidx]->setSelActive( ispolyseltbon );
    }
}


void uiODViewer2D::handleToolClick( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,itm,cb)
    if ( !itm ) return;

    ispolyselect_ = itm->getID()==0;
    selectionMode( cb );
}


void uiODViewer2D::removeSelected( CallBacker* cb )
{
    if ( !viewstdcontrol_->toolBar()->isOn(polyseltbid_) )
	return;

    for ( int edidx=0; edidx<auxdataeditors_.size(); edidx++ )
	auxdataeditors_[edidx]->removePolygonSelected( -1 );
}


void uiODViewer2D::setWinTitle( bool fromcs )
{
    BufferString info;
    if ( !fromcs )
    {
	appl_.applMgr().visServer()->getObjectInfo( visid_, info );
	if ( info.isEmpty() )
	    info = appl_.applMgr().visServer()->getObjectName( visid_ );
    }
    else
    {
	if ( tkzs_.hsamp_.survid_ == Survey::GM().get2DSurvID() )
	    { info = "Line: "; info += Survey::GM().getName( geomID() ); }
	else if ( tkzs_.defaultDir() == TrcKeyZSampling::Inl )
	    { info = "In-line: "; info += tkzs_.hsamp_.start_.inl(); }
	else if ( tkzs_.defaultDir() == TrcKeyZSampling::Crl )
	    { info = "Cross-line: "; info += tkzs_.hsamp_.start_.crl(); }
	else
	{
	    info = zDomain().userName(); info += ": ";
	    info += mNINT32(tkzs_.zsamp_.start * zDomain().userFactor());
	}
    }

    info.insertAt( 0, basetxt_ );
    if ( viewwin() )
	viewwin()->setWinTitle( info );
}


void uiODViewer2D::usePar( const IOPar& iop )
{
    if ( !viewwin() ) return;

    IOPar* vdselspecpar = iop.subselect( sKeyVDSelSpec() );
    if ( vdselspecpar ) vdselspec_.usePar( *vdselspecpar );
    IOPar* wvaselspecpar = iop.subselect( sKeyWVASelSpec() );
    if ( wvaselspecpar ) wvaselspec_.usePar( *wvaselspecpar );
    delete vdselspecpar; delete wvaselspecpar;
    IOPar* tkzspar = iop.subselect( sKeyPos() );
    TrcKeyZSampling tkzs; if ( tkzspar ) tkzs.usePar( *tkzspar );
    if ( viewwin()->nrViewers() > 0 )
    {
	const uiFlatViewer& vwr = viewwin()->viewer(0);
	const bool iswva = wvaselspec_.id().isValid();
	ConstDataPackRef<RegularSeisDataPack> regsdp = vwr.obtainPack( iswva );
	if ( regsdp ) setPos( tkzs );
    }

    datamgr_->usePar( iop, viewwin(), dataEditor() );
    rebuildTree();
}


void uiODViewer2D::fillPar( IOPar& iop ) const
{
    IOPar vdselspecpar, wvaselspecpar;
    vdselspec_.fillPar( vdselspecpar );
    wvaselspec_.fillPar( wvaselspecpar );
    iop.mergeComp( vdselspecpar, sKeyVDSelSpec() );
    iop.mergeComp( wvaselspecpar, sKeyWVASelSpec() );
    IOPar pospar; tkzs_.fillPar( pospar );
    iop.mergeComp( pospar, sKeyPos() );

    datamgr_->fillPar( iop );
}


void uiODViewer2D::rebuildTree()
{
    ObjectSet<Vw2DDataObject> objs;
    dataMgr()->getObjects( objs );
    for ( int iobj=0; iobj<objs.size(); iobj++ )
	uiODVw2DTreeItem::create( treeTop(), *this, objs[iobj]->id() );
}


void uiODViewer2D::setMouseCursorExchange( MouseCursorExchange* mce )
{
    if ( mousecursorexchange_ )
	mDetachCB( mousecursorexchange_->notifier,uiODViewer2D::mouseCursorCB );

    mousecursorexchange_ = mce;
    if ( mousecursorexchange_ )
	mAttachCB( mousecursorexchange_->notifier,uiODViewer2D::mouseCursorCB );
}


void uiODViewer2D::mouseCursorCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(const MouseCursorExchange::Info&,info,
			       caller,cb);
    if ( caller==this )
	return;

    uiFlatViewer& vwr = viewwin()->viewer(0);
    if ( !marker_ )
    {
	marker_ = vwr.createAuxData( "XYZ Marker" );
	vwr.addAuxData( marker_ );
	marker_->poly_ += FlatView::Point(0,0);
	marker_->markerstyles_ += MarkerStyle2D();
    }

    ConstDataPackRef<FlatDataPack> fdp = vwr.obtainPack( false, true );
    mDynamicCastGet(const SeisFlatDataPack*,seisfdp,fdp.ptr());
    mDynamicCastGet(const MapDataPack*,mapdp,fdp.ptr());
    if ( !seisfdp && !mapdp ) return;

    const Coord3& coord = info.surveypos_;
    FlatView::Point& pt = marker_->poly_[0];
    if ( seisfdp )
    {
	const Survey::Geometry* geometry = Survey::GM().getGeometry(
			seisfdp->is2D() ? geomID() : tkzs_.hsamp_.survid_ );
	const TrcKey trckey = geometry ? geometry->nearestTrace(coord)
				       : TrcKey::udf();
	const int gidx = seisfdp->getSourceDataPack().getGlobalIdx( trckey );
	if ( seisfdp->isVertical() )
	{
	    pt.x = fdp->posData().range(true).atIndex( gidx );
	    pt.y = datatransform_ ? datatransform_->transform(coord) : coord.z;
	}
	else
	{
	    pt.x = fdp->posData().range(true).atIndex( gidx / tkzs_.nrTrcs() );
	    pt.y = fdp->posData().range(false).atIndex( gidx % tkzs_.nrTrcs() );
	}
    }
    else if ( mapdp )
	pt = FlatView::Point( coord.x, coord.y );

    vwr.handleChange( FlatView::Viewer::Auxdata );
}


void uiODViewer2D::mouseMoveCB( CallBacker* cb )
{
    Coord3 mousepos( Coord3::udf() );
    mCBCapsuleUnpack(IOPar,pars,cb);

    FixedString valstr = pars.find( "X" );
    if ( valstr.isEmpty() ) valstr = pars.find( "X-coordinate" );
    if ( !valstr.isEmpty() ) mousepos.x = valstr.toDouble();
    valstr = pars.find( "Y" );
    if ( valstr.isEmpty() ) valstr = pars.find( "Y-coordinate" );
    if ( !valstr.isEmpty() ) mousepos.y = valstr.toDouble();
    valstr = pars.find( "Z" );
    if ( valstr.isEmpty() ) valstr = pars.find( "Z-Coord" );
    if ( !valstr.isEmpty() )
    {
	mousepos.z = valstr.toDouble() / zDomain().userFactor();
	if ( datatransform_ )
	    mousepos.z = datatransform_->transformBack( mousepos );
    }

    if ( mousecursorexchange_ && mousepos.isDefined() )
    {
	MouseCursorExchange::Info info( mousepos );
	mousecursorexchange_->notifier.trigger( info, this );
    }
}


void uiODViewer2D::removeHorizon3D( EM::ObjectID emid )
{
    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DHor3DParentTreeItem*,hor3dpitem,
			treetp_->getChild(idx))
	if ( hor3dpitem )
	    hor3dpitem->removeHorizon3D( emid );
    }
}


void uiODViewer2D::getLoadedHorizon3Ds( TypeSet<EM::ObjectID>& emids ) const
{
    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DHor3DParentTreeItem*,hor3dpitem,
			treetp_->getChild(idx))
	if ( hor3dpitem )
	    hor3dpitem->getLoadedHorizon3Ds( emids );
    }
}


void uiODViewer2D::addHorizon3Ds( const TypeSet<EM::ObjectID>& emids )
{
    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DHor3DParentTreeItem*,hor3dpitem,
			treetp_->getChild(idx))
	if ( hor3dpitem )
	    hor3dpitem->addHorizon3Ds( emids );
    }
}


void uiODViewer2D::addNewTrackingHorizon3D( EM::ObjectID emid )
{
    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DHor3DParentTreeItem*,hor3dpitem,
			treetp_->getChild(idx))
	if ( hor3dpitem )
	    hor3dpitem->addNewTrackingHorizon3D( emid );
    }
}


void uiODViewer2D::removeHorizon2D( EM::ObjectID emid )
{
    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DHor2DParentTreeItem*,hor2dpitem,
			treetp_->getChild(idx))
	if ( hor2dpitem )
	    hor2dpitem->removeHorizon2D( emid );
    }
}


void uiODViewer2D::getLoadedHorizon2Ds( TypeSet<EM::ObjectID>& emids ) const
{
    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DHor2DParentTreeItem*,hor2dpitem,
			treetp_->getChild(idx))
	if ( hor2dpitem )
	    hor2dpitem->getLoadedHorizon2Ds( emids );
    }
}


void uiODViewer2D::addHorizon2Ds( const TypeSet<EM::ObjectID>& emids )
{
    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DHor2DParentTreeItem*,hor2dpitem,
			treetp_->getChild(idx))
	if ( hor2dpitem )
	    hor2dpitem->addHorizon2Ds( emids );
    }
}


void uiODViewer2D::addNewTrackingHorizon2D( EM::ObjectID emid )
{
    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DHor2DParentTreeItem*,hor2dpitem,
			treetp_->getChild(idx))
	if ( hor2dpitem )
	    hor2dpitem->addNewTrackingHorizon2D( emid );
    }
}
