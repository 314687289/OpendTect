/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiodviewer2dmgr.h"

#include "uiattribpartserv.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uiflatviewslicepos.h"
#include "uiflatviewstdcontrol.h"
#include "uigraphicsview.h"
#include "uimenu.h"
#include "uiodviewer2d.h"
#include "uiodscenemgr.h"
#include "uiodvw2dfaulttreeitem.h"
#include "uiodvw2dfaultss2dtreeitem.h"
#include "uiodvw2dfaultsstreeitem.h"
#include "uiodvw2dhor2dtreeitem.h"
#include "uiodvw2dhor3dtreeitem.h"
#include "uiodvw2dpicksettreeitem.h"
#include "uiodvw2dvariabledensity.h"
#include "uiodvw2dwigglevararea.h"
#include "uitreeitemmanager.h"
#include "uivispartserv.h"
#include "zaxistransform.h"

#include "attribsel.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "emobject.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emfault3d.h"
#include "emfaultstickset.h"
#include "mouseevent.h"
#include "mousecursor.h"
#include "posinfo2d.h"
#include "randomlinegeom.h"
#include "seisioobjinfo.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "view2ddata.h"
#include "view2ddataman.h"


uiODViewer2DMgr::uiODViewer2DMgr( uiODMain* a )
    : appl_(*a)
    , tifs2d_(new uiTreeFactorySet)
    , tifs3d_(new uiTreeFactorySet)
    , l2dintersections_(0)
{
    // for relevant 2D datapack
    tifs2d_->addFactory( new uiODVW2DWiggleVarAreaTreeItemFactory, 1000 );
    tifs2d_->addFactory( new uiODVW2DVariableDensityTreeItemFactory, 2000 );
    tifs2d_->addFactory( new uiODVw2DHor2DTreeItemFactory, 3000 );
    tifs2d_->addFactory( new uiODVw2DFaultSS2DTreeItemFactory, 4000 );

    // for relevant 3D datapack
    tifs3d_->addFactory( new uiODVW2DWiggleVarAreaTreeItemFactory, 1500 );
    tifs3d_->addFactory( new uiODVW2DVariableDensityTreeItemFactory, 2500 );
    tifs3d_->addFactory( new uiODVw2DHor3DTreeItemFactory, 3500 );
    tifs3d_->addFactory( new uiODVw2DFaultTreeItemFactory, 4500 );
    tifs3d_->addFactory( new uiODVw2DFaultSSTreeItemFactory, 5500 );
    tifs3d_->addFactory( new uiODVw2DPickSetTreeItemFactory, 6500 );
}


uiODViewer2DMgr::~uiODViewer2DMgr()
{
    if ( l2dintersections_ )
	deepErase( *l2dintersections_ );
    delete l2dintersections_;
    l2dintersections_ = 0;
    delete tifs2d_; delete tifs3d_;
    deepErase( viewers2d_ );
}

void uiODViewer2DMgr::setupHorizon3Ds( uiODViewer2D* vwr2d )
{
    TypeSet<EM::ObjectID> emids;
    getLoadedHorizon3Ds( emids );
    appl_.sceneMgr().getLoadedEMIDs(emids,EM::Horizon3D::typeStr());
    vwr2d->addHorizon3Ds( emids );
}


void uiODViewer2DMgr::setupHorizon2Ds( uiODViewer2D* vwr2d )
{
    if ( SI().has2D() )
    {
	TypeSet<EM::ObjectID> emids;
	getLoadedHorizon2Ds( emids );
	appl_.sceneMgr().getLoadedEMIDs(emids,EM::Horizon2D::typeStr());
	vwr2d->addHorizon2Ds( emids );
    }
}


void uiODViewer2DMgr::setupFaults( uiODViewer2D* vwr2d )
{
    TypeSet<EM::ObjectID> emids;
    getLoadedFaults( emids );
    appl_.sceneMgr().getLoadedEMIDs(emids,EM::Fault3D::typeStr());
    vwr2d->addFaults( emids );
}


void uiODViewer2DMgr::setupFaultSSs( uiODViewer2D* vwr2d )
{
    TypeSet<EM::ObjectID> emids;
    getLoadedFaultSSs( emids );
    appl_.sceneMgr().getLoadedEMIDs(emids,EM::FaultStickSet::typeStr());
    vwr2d->addFaultSSs( emids );
}


void uiODViewer2DMgr::setupPickSets( uiODViewer2D* vwr2d )
{
    TypeSet<MultiID> pickmids;
    getLoadedPickSets( pickmids );
    appl_.sceneMgr().getLoadedPickSetIDs( pickmids, false );
    vwr2d->addPickSets( pickmids );
}


int uiODViewer2DMgr::displayIn2DViewer( Viewer2DPosDataSel& posdatasel,
					bool wva,
					float initialx1pospercm,
					float initialx2pospercm )
{
    DataPack::ID dpid = DataPack::cNoID();
    uiAttribPartServer* attrserv = appl_.applMgr().attrServer();
    attrserv->setTargetSelSpec( posdatasel.selspec_ );
    const bool isrl = !posdatasel.rdmlineid_.isUdf();
    if ( isrl )
    {
	TypeSet<BinID> knots, path;
	Geometry::RandomLineSet::getGeometry(
		posdatasel.rdmlineid_, knots, &posdatasel.tkzs_.zsamp_ );
	Geometry::RandomLine::getPathBids( knots, path );
	dpid = attrserv->createRdmTrcsOutput(
				posdatasel.tkzs_.zsamp_, &path, &knots );
    }
    else
	dpid = attrserv->createOutput( posdatasel.tkzs_, DataPack::cNoID() );
    
    return displayIn2DViewer( dpid, posdatasel.selspec_, wva,
	    		      initialx1pospercm, initialx2pospercm );
}


int uiODViewer2DMgr::displayIn2DViewer( DataPack::ID dpid,
			const Attrib::SelSpec& as, bool dowva,
			float initialx1pospercm, float initialx2pospercm )
{
    uiODViewer2D* vwr2d = &addViewer2D( -1 );
    const DataPack::ID vwdpid = vwr2d->createFlatDataPack( dpid, 0 );
    vwr2d->setSelSpec( &as, dowva ); vwr2d->setSelSpec( &as, !dowva );
    vwr2d->setInitialX1PosPerCM( initialx1pospercm );
    vwr2d->setInitialX2PosPerCM( initialx2pospercm );
    vwr2d->setUpView( vwdpid, dowva );
    vwr2d->useStoredDispPars( dowva );
    vwr2d->useStoredDispPars( !dowva );

    uiFlatViewer& vwr = vwr2d->viewwin()->viewer();
    FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
    (!dowva ? ddp.wva_.show_ : ddp.vd_.show_) = false;
    vwr.handleChange( FlatView::Viewer::DisplayPars );
    attachNotifiersAndSetAuxData( vwr2d );
    return vwr2d->id_;
}


void uiODViewer2DMgr::displayIn2DViewer( int visid, int attribid, bool dowva )
{
    const DataPack::ID id = visServ().getDisplayedDataPackID( visid, attribid );
    if ( id < 0 ) return;

    uiODViewer2D* vwr2d = find2DViewer( visid, true );
    const bool isnewvwr = !vwr2d;
    if ( !vwr2d )
    {
	vwr2d = &addViewer2D( visid );
	ConstRefMan<ZAxisTransform> zat =
		visServ().getZAxisTransform( visServ().getSceneID(visid) );
	vwr2d->setZAxisTransform( const_cast<ZAxisTransform*>(zat.ptr()) );
    }
    else
	visServ().fillDispPars( visid, attribid,
		vwr2d->viewwin()->viewer().appearance().ddpars_, dowva );
    //<-- So that new display parameters are read before the new data is set.
    //<-- This will avoid time lag between updating data and display parameters.

    const Attrib::SelSpec* as = visServ().getSelSpec(visid,attribid);
    vwr2d->setSelSpec( as, dowva );
    if ( isnewvwr ) vwr2d->setSelSpec( as, !dowva );

    const int version = visServ().currentVersion( visid, attribid );
    const DataPack::ID dpid = vwr2d->createFlatDataPack( id, version );
    vwr2d->setUpView( dpid, dowva );
    vwr2d->setWinTitle();

    uiFlatViewer& vwr = vwr2d->viewwin()->viewer();
    if ( isnewvwr )
    {
	FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
	visServ().fillDispPars( visid, attribid, ddp, dowva );
	visServ().fillDispPars( visid, attribid, ddp, !dowva );
	(!dowva ? ddp.wva_.show_ : ddp.vd_.show_) = false;
	attachNotifiersAndSetAuxData( vwr2d );
    }

    vwr.handleChange( FlatView::Viewer::DisplayPars );
}


#define mGetAuxAnnotIdx \
    uiODViewer2D* curvwr2d = find2DViewer( *meh ); \
    if ( !curvwr2d ) return; \
    uiFlatViewer& curvwr = curvwr2d->viewwin()->viewer( 0 ); \
    const uiWorldPoint wp = curvwr.getWorld2Ui().transform(meh->event().pos());\
    const Coord3 coord = curvwr.getCoord( wp );\
    if ( coord.isUdf() ) return;\
    const float x1eps  = ((float)curvwr.posRange(true).step)*2.f; \
    const int x1auxposidx = \
	curvwr.appearance().annot_.x1_.auxPosIdx( mCast(float,wp.x), x1eps ); \
    const float x2eps  = ((float)curvwr.posRange(false).step)*2.f; \
    const int x2auxposidx = \
	curvwr.appearance().annot_.x2_.auxPosIdx( mCast(float,wp.y), x2eps );

void uiODViewer2DMgr::mouseMoveCB( CallBacker* cb )
{
    mDynamicCastGet(const MouseEventHandler*,meh,cb);
    if ( !meh || !meh->hasEvent() )
	return;

    mGetAuxAnnotIdx
    if ( !selauxannot_.isselected_ )
    {
	if ( curvwr.appearance().annot_.editable_ ) return;

	if ( x1auxposidx<0 && x2auxposidx<0 && selauxannot_.auxposidx_<0 )
	    return;

	if ( selauxannot_.auxposidx_<0 )
	{
	    curvwr.rgbCanvas().setDragMode( uiGraphicsViewBase::NoDrag );
	    MouseCursorManager::mgr()->setOverride(
		    x1auxposidx>=0 ? MouseCursor::SplitH
				   : MouseCursor::SplitV );
	}

	if ( x1auxposidx>=0 )
	    selauxannot_ = SelectedAuxAnnot( x1auxposidx, true, false );
	else if ( x2auxposidx>=0 )
	    selauxannot_ = SelectedAuxAnnot( x2auxposidx, false, false );
	else if ( selauxannot_.auxposidx_>=0 )
	{
	    reSetPrevDragMode( curvwr2d );
	    selauxannot_.auxposidx_ = -1;
	}
    }
    else if ( selauxannot_.isValid() && selauxannot_.isselected_ )
    {
	TypeSet<PlotAnnotation>& xauxannot =
	    selauxannot_.isx1_ ? curvwr.appearance().annot_.x1_.auxannot_
			     : curvwr.appearance().annot_.x2_.auxannot_;
	if ( !xauxannot.validIdx(selauxannot_.auxposidx_) )
	    return;

	PlotAnnotation& selauxannot = xauxannot[selauxannot_.auxposidx_];
	if ( selauxannot.isNormal() )
	    return;

	const StepInterval<double> xrg =
	    curvwr2d->viewwin()->viewer().posRange( selauxannot_.isx1_ );
	const int newposidx = xrg.nearestIndex( selauxannot_.isx1_
						? wp.x : wp.y);
	const float newpos = mCast(float, xrg.atIndex(newposidx) );
	selauxannot.pos_ = newpos;
	TrcKeyZSampling::Dir vwr2ddir =
	    curvwr2d->getTrcKeyZSampling().defaultDir();
	if ( (vwr2ddir==TrcKeyZSampling::Inl && selauxannot_.isx1_) ||
	     (vwr2ddir==TrcKeyZSampling::Z && !selauxannot_.isx1_) )
	    selauxannot.txt_ = tr( "CRL %1" ).arg( toString(mNINT32(newpos)) );
	else if ( (vwr2ddir==TrcKeyZSampling::Crl && selauxannot_.isx1_) ||
		  (vwr2ddir==TrcKeyZSampling::Z && selauxannot_.isx1_) )
	    selauxannot.txt_ = tr( "INL %1" ).arg( toString(mNINT32(newpos)) );
	else if ( (vwr2ddir==TrcKeyZSampling::Inl && !selauxannot_.isx1_) ||
		  (vwr2ddir==TrcKeyZSampling::Crl && !selauxannot_.isx1_) )
	    selauxannot.txt_ = tr( "ZSlice %1" ).arg( toString(newpos) );
    }

    setAuxAnnotLineStyles( curvwr, true );
    setAuxAnnotLineStyles( curvwr, false );
    curvwr.handleChange( FlatView::Viewer::Annot );
}


void uiODViewer2DMgr::setAuxAnnotLineStyles( uiFlatViewer& vwr, bool forx1 )
{
    FlatView::Annotation& annot = vwr.appearance().annot_;
    TypeSet<PlotAnnotation>& auxannot = forx1 ? annot.x1_.auxannot_
					      : annot.x2_.auxannot_;
    for ( int idx=0; idx<auxannot.size(); idx++ )
	if ( !auxannot[idx].isNormal() )
	    auxannot[idx].linetype_ = PlotAnnotation::Bold;

    const int selannotidx = selauxannot_.auxposidx_;
    if ( !(forx1^selauxannot_.isx1_) && auxannot.validIdx(selannotidx) )
	if ( !auxannot[selannotidx].isNormal() )
	    auxannot[selannotidx].linetype_ = PlotAnnotation::HighLighted;
}


void uiODViewer2DMgr::reSetPrevDragMode( uiODViewer2D* curvwr2d )
{
    uiGraphicsViewBase::ODDragMode prevdragmode =
    uiGraphicsViewBase::ScrollHandDrag;
    if ( curvwr2d->viewControl()->isEditModeOn() )
	prevdragmode = uiGraphicsViewBase::NoDrag;
    else if ( curvwr2d->viewControl()->isRubberBandOn() )
	prevdragmode = uiGraphicsViewBase::RubberBandDrag;
    curvwr2d->viewwin()->viewer(0).rgbCanvas().setDragMode( prevdragmode );
    MouseCursorManager::mgr()->restoreOverride();
}


void uiODViewer2DMgr::handleLeftClick( uiODViewer2D* vwr2d )
{
    if ( !vwr2d ) return;
    uiFlatViewer& vwr = vwr2d->viewwin()->viewer( 0 );
    uiODViewer2D* clickedvwr2d = 0;
    const TrcKeyZSampling& tkzs = vwr2d->getTrcKeyZSampling();
    TypeSet<PlotAnnotation>& auxannot =
	selauxannot_.isx1_ ? vwr.appearance().annot_.x1_.auxannot_
			   : vwr.appearance().annot_.x2_.auxannot_;
    if ( TrcKey::is2D(tkzs.hsamp_.survid_) )
    {
	if ( selauxannot_.auxposidx_<0 ||
	     auxannot[selauxannot_.auxposidx_].isNormal())
	    return;

	Line2DInterSection::Point intpoint2d( Survey::GM().cUndefGeomID(),
					      mUdf(int), mUdf(int) );
	const float auxpos = auxannot[selauxannot_.auxposidx_].pos_;
	intpoint2d = intersectingLineID( vwr2d, auxpos );
	if ( intpoint2d.line==Survey::GM().cUndefGeomID() )
	   return;
	clickedvwr2d = find2DViewer( intpoint2d.line );
    }
    else
    {
	if ( !tkzs.isFlat() || selauxannot_.auxposidx_<0 )
	    return;

	TrcKeyZSampling clickedtkzs;
	TrcKeyZSampling newposkzs;
	if ( tkzs.defaultDir()==TrcKeyZSampling::Inl )
	{
	    if ( selauxannot_.isx1_ )
	    {
		const int auxpos = mNINT32( selauxannot_.oldauxpos_ );
		clickedtkzs.hsamp_.setTrcRange(
			Interval<int>(auxpos,auxpos) );
		const int newauxpos =
		    mNINT32( auxannot[selauxannot_.auxposidx_].pos_ );
		newposkzs.hsamp_.setTrcRange(
			Interval<int>(newauxpos,newauxpos) );
	    }
	    else
	    {
		const float auxpos = selauxannot_.oldauxpos_;
		clickedtkzs.zsamp_ =
		    StepInterval<float>( auxpos, auxpos,
					 clickedtkzs.zsamp_.step );
		const float newauxpos = auxannot[selauxannot_.auxposidx_].pos_;
		newposkzs.zsamp_ = StepInterval<float>( newauxpos, newauxpos,
							newposkzs.zsamp_.step );
	    }
	}
	else if ( tkzs.defaultDir()==TrcKeyZSampling::Crl )
	{
	    if ( selauxannot_.isx1_ )
	    {
		const int auxpos = mNINT32(selauxannot_.oldauxpos_);
		clickedtkzs.hsamp_.setLineRange(
			Interval<int>(auxpos,auxpos) );
		const int newauxpos =
		    mNINT32(auxannot[selauxannot_.auxposidx_].pos_);
		newposkzs.hsamp_.setLineRange(
			Interval<int>(newauxpos,newauxpos) );
	    }
	    else
	    {
		const float auxpos = selauxannot_.oldauxpos_;
		clickedtkzs.zsamp_ =
		    StepInterval<float>( auxpos, auxpos,
					 clickedtkzs.zsamp_.step );
		const float newauxpos = auxannot[selauxannot_.auxposidx_].pos_;
		newposkzs.zsamp_ = StepInterval<float>( newauxpos, newauxpos,
							newposkzs.zsamp_.step );
	    }

	}
	else if ( tkzs.defaultDir()==TrcKeyZSampling::Z )
	{
	    const int auxpos = mNINT32(selauxannot_.oldauxpos_);
	    const int newauxpos =
				mNINT32(auxannot[selauxannot_.auxposidx_].pos_);
	    if ( selauxannot_.isx1_ )
	    {
		clickedtkzs.hsamp_.setLineRange(
			Interval<int>(auxpos,auxpos) );
		newposkzs.hsamp_.setLineRange(
			Interval<int>(newauxpos,newauxpos) );
	    }
	    else
	    {
		clickedtkzs.hsamp_.setTrcRange(
			Interval<int>(auxpos,auxpos) );
		newposkzs.hsamp_.setTrcRange(
			Interval<int>(newauxpos,newauxpos) );
	    }
	}

	clickedvwr2d = find2DViewer( clickedtkzs );
	if ( clickedvwr2d )
	    clickedvwr2d->setPos( newposkzs );
	setAllIntersectionPositions();
    }

    selauxannot_.isselected_ = false;
    selauxannot_.oldauxpos_ = mUdf(float);
    reSetPrevDragMode( vwr2d );
    if ( clickedvwr2d )
	clickedvwr2d->viewwin()->dockParent()->raise();
}


void uiODViewer2DMgr::mouseClickedCB( CallBacker* cb )
{
    mDynamicCastGet(const MouseEventHandler*,meh,cb);
    if ( !meh || !meh->hasEvent() ||
	 (!meh->event().rightButton() && !meh->event().leftButton()) )
	return;

    handleLeftClick( find2DViewer(*meh) );
}


void uiODViewer2DMgr::mouseClickCB( CallBacker* cb )
{
    mDynamicCastGet(const MouseEventHandler*,meh,cb);
    if ( !meh || !meh->hasEvent() ||
	 (!meh->event().rightButton() && !meh->event().leftButton()) )
	return;

    mGetAuxAnnotIdx

    if ( meh->event().leftButton() )
    {
	if ( curvwr.appearance().annot_.editable_ ||
	     curvwr2d->geomID()!=mUdfGeomID || (x1auxposidx<0 && x2auxposidx<0))
	    return;

	const bool isx1auxannot = x1auxposidx>=0;
	const int auxannotidx = isx1auxannot ? x1auxposidx : x2auxposidx;
	const FlatView::Annotation::AxisData& axisdata =
			isx1auxannot ? curvwr.appearance().annot_.x1_
				     : curvwr.appearance().annot_.x2_;
	selauxannot_ = SelectedAuxAnnot( auxannotidx, isx1auxannot, true );
	selauxannot_.oldauxpos_ = axisdata.auxannot_[auxannotidx].pos_;
	return;
    }

    uiMenu menu( "Menu" );
    Line2DInterSection::Point intpoint2d( Survey::GM().cUndefGeomID(),
					  mUdf(int), mUdf(int) );
    const TrcKeyZSampling& tkzs = curvwr2d->getTrcKeyZSampling();
    if ( tkzs.hsamp_.survid_ == Survey::GM().get2DSurvID() )
    {
	if ( x1auxposidx>=0 &&
	     curvwr.appearance().annot_.x1_.auxannot_[x1auxposidx].isNormal() )
	{
	    intpoint2d = intersectingLineID( curvwr2d, (float) wp.x );
	    if ( intpoint2d.line==Survey::GM().cUndefGeomID() )
	       return;
	    const uiString show2dtxt = m3Dots(tr("Show Line '%1'")).arg(
					Survey::GM().getName(intpoint2d.line) );
	    menu.insertAction( new uiAction(show2dtxt), 0 );
	}
    }
    else
    {
	const BinID bid = SI().transform( coord );
	const uiString showinltxt
			= m3Dots(tr("Show In-line %1")).arg( bid.inl() );
	const uiString showcrltxt
			= m3Dots(tr("Show Cross-line %1")).arg( bid.crl());
	const uiString showztxt = m3Dots(tr("Show Z-slice %1"))
		.arg( mNINT32(coord.z*curvwr2d->zDomain().userFactor()) );

	const bool isflat = tkzs.isFlat();
	const TrcKeyZSampling::Dir dir = tkzs.defaultDir();
	if ( !isflat || dir!=TrcKeyZSampling::Inl )
	    menu.insertAction( new uiAction(showinltxt), 1 );
	if ( !isflat || dir!=TrcKeyZSampling::Crl )
	    menu.insertAction( new uiAction(showcrltxt), 2 );
	if ( !isflat || dir!=TrcKeyZSampling::Z )
	    menu.insertAction( new uiAction(showztxt), 3 );
    }

    menu.insertAction( new uiAction("Properties..."), 4 );

    const int menuid = menu.exec();
    if ( menuid>=0 && menuid<4 )
    {
	const BinID bid = SI().transform( coord );
	uiWorldPoint initialcentre( uiWorldPoint::udf() );
	TrcKeyZSampling newtkzs = SI().sampling(true);
	newtkzs.hsamp_.survid_ = tkzs.hsamp_.survid_;
	if ( menuid==0 )
	{
	    const PosInfo::Line2DData& l2ddata =
		Survey::GM().getGeometry( intpoint2d.line )->as2D()->data();
	    const StepInterval<int> trcnrrg = l2ddata.trcNrRange();
	    const float trcdist =
		l2ddata.distBetween( trcnrrg.start, intpoint2d.linetrcnr );
	    if ( mIsUdf(trcdist) )
		return;
	    initialcentre = uiWorldPoint( mCast(double,trcdist), coord.z );
	    newtkzs.hsamp_.init( intpoint2d.line );
	    newtkzs.hsamp_.setLineRange(
		    Interval<int>(intpoint2d.line,intpoint2d.line) );
	}
	else if ( menuid == 1 )
	{
	    newtkzs.hsamp_.setLineRange( Interval<int>(bid.inl(),bid.inl()) );
	    initialcentre = uiWorldPoint( mCast(double,bid.crl()), coord.z );
	}
	else if ( menuid == 2 )
	{
	    newtkzs.hsamp_.setTrcRange( Interval<int>(bid.crl(),bid.crl()) );
	    initialcentre = uiWorldPoint( mCast(double,bid.inl()), coord.z );
	}
	else if ( menuid == 3 )
	{
	    newtkzs.zsamp_ = Interval<float>( mCast(float,coord.z),
					      mCast(float,coord.z) );
	    initialcentre = uiWorldPoint( mCast(double,bid.inl()),
					  mCast(double,bid.crl()) );
	}

	create2DViewer( *curvwr2d, newtkzs, initialcentre );
    }
    else if ( menuid == 4 )
	curvwr2d->viewControl()->doPropertiesDialog( 0 );
}


void uiODViewer2DMgr::create2DViewer( const uiODViewer2D& curvwr2d,
				      const TrcKeyZSampling& newsampling,
				      const uiWorldPoint& initialcentre )
{
    uiODViewer2D* vwr2d = &addViewer2D( -1 );
    vwr2d->setSelSpec( &curvwr2d.selSpec(true), true );
    vwr2d->setSelSpec( &curvwr2d.selSpec(false), false );
    vwr2d->setTrcKeyZSampling( newsampling );
    vwr2d->setZAxisTransform( curvwr2d.getZAxisTransform() );

    const uiFlatViewStdControl* control = curvwr2d.viewControl();
    vwr2d->setInitialCentre( initialcentre );
    vwr2d->setInitialX1PosPerCM( control->getCurrentPosPerCM(true) );
    if ( newsampling.defaultDir()!=TrcKeyZSampling::Z && curvwr2d.isVertical() )
	vwr2d->setInitialX2PosPerCM( control->getCurrentPosPerCM(false) );

    const uiFlatViewer& curvwr = curvwr2d.viewwin()->viewer( 0 );
    if ( curvwr.isVisible(true) )
	vwr2d->setUpView( vwr2d->createDataPack(true), true );
    else if ( curvwr.isVisible(false) )
	vwr2d->setUpView( vwr2d->createDataPack(false), false );

    for ( int idx=0; idx<vwr2d->viewwin()->nrViewers(); idx++ )
    {
	uiFlatViewer& vwr = vwr2d->viewwin()->viewer( idx );
	vwr.appearance().ddpars_ = curvwr.appearance().ddpars_;
	vwr.appearance().ddpars_.wva_.allowuserchange_ = vwr2d->isVertical();
	vwr.handleChange( FlatView::Viewer::DisplayPars );
    }

    attachNotifiersAndSetAuxData( vwr2d );
}


void uiODViewer2DMgr::attachNotifiersAndSetAuxData( uiODViewer2D* vwr2d )
{
    mAttachCB( vwr2d->viewWinClosed, uiODViewer2DMgr::viewWinClosedCB );
    if ( vwr2d->slicePos() )
	mAttachCB( vwr2d->slicePos()->positionChg,
		   uiODViewer2DMgr::vw2DPosChangedCB );
    for ( int idx=0; idx<vwr2d->viewwin()->nrViewers(); idx++ )
    {
	uiFlatViewer& vwr = vwr2d->viewwin()->viewer( idx );
	mAttachCB( vwr.rgbCanvas().getMouseEventHandler().buttonPressed,
		   uiODViewer2DMgr::mouseClickCB );
	mAttachCB( vwr.rgbCanvas().getMouseEventHandler().movement,
		   uiODViewer2DMgr::mouseMoveCB );
	mAttachCB( vwr.rgbCanvas().getMouseEventHandler().buttonReleased,
		   uiODViewer2DMgr::mouseClickedCB );
    }

    vwr2d->setUpAux();
    setAllIntersectionPositions();
    setupHorizon3Ds( vwr2d );
    setupHorizon2Ds( vwr2d );
    setupFaults( vwr2d );
    setupFaultSSs( vwr2d );
    setupPickSets( vwr2d );
}


void uiODViewer2DMgr::reCalc2DIntersetionIfNeeded( Pos::GeomID geomid )
{
    if ( intersection2DIdx(geomid) < 0 )
    {
	if ( l2dintersections_ )
	    deepErase( *l2dintersections_ );
	delete l2dintersections_;
	l2dintersections_ = new Line2DInterSectionSet;
	BufferStringSet lnms;
	TypeSet<Pos::GeomID> geomids;
	SeisIOObjInfo::getLinesWithData( lnms, geomids );
	BendPointFinder2DGeomSet bpfinder( geomids );
	bpfinder.execute();
	Line2DInterSectionFinder intfinder( bpfinder.bendPoints(),
					    *l2dintersections_ );
	intfinder.execute();
    }
}


uiODViewer2D& uiODViewer2DMgr::addViewer2D( int visid )
{
    uiODViewer2D* vwr = new uiODViewer2D( appl_, visid );
    vwr->setMouseCursorExchange( &appl_.applMgr().mouseCursorExchange() );
    viewers2d_ += vwr;
    return *vwr;
}


uiODViewer2D* uiODViewer2DMgr::find2DViewer( int id, bool byvisid )
{
    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	const int vwrid = byvisid ? viewers2d_[idx]->visid_
				  : viewers2d_[idx]->id_;
	if ( vwrid == id )
	    return viewers2d_[idx];
    }

    return 0;
}


uiODViewer2D* uiODViewer2DMgr::find2DViewer( const MouseEventHandler& meh )
{
    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	if ( viewers2d_[idx]->viewControl()->getViewerIdx(&meh,true) != -1 )
	    return viewers2d_[idx];
    }

    return 0;
}


uiODViewer2D* uiODViewer2DMgr::find2DViewer( const Pos::GeomID& geomid )
{
    if ( geomid == Survey::GM().cUndefGeomID() )
	return 0;

    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	if ( viewers2d_[idx]->geomID() == geomid )
	    return viewers2d_[idx];
    }

    return 0;
}


uiODViewer2D* uiODViewer2DMgr::find2DViewer( const TrcKeyZSampling& tkzs )
{
    if ( !tkzs.isFlat() )
	return 0;

    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	if ( viewers2d_[idx]->getTrcKeyZSampling() == tkzs )
	    return viewers2d_[idx];
    }

    return 0;
}


void uiODViewer2DMgr::setVWR2DIntersectionPositions( uiODViewer2D* vwr2d )
{
    TrcKeyZSampling::Dir vwr2ddir = vwr2d->getTrcKeyZSampling().defaultDir();
    TypeSet<PlotAnnotation>& x1intannots =
	vwr2d->viewwin()->viewer().appearance().annot_.x1_.auxannot_;
    TypeSet<PlotAnnotation>& x2intannots =
	vwr2d->viewwin()->viewer().appearance().annot_.x2_.auxannot_;
    x1intannots.erase(); x2intannots.erase();
    const PlotAnnotation::LineType boldltype = PlotAnnotation::Bold;

    if ( vwr2d->geomID()!=Survey::GM().cUndefGeomID() )
    {
	reCalc2DIntersetionIfNeeded( vwr2d->geomID() );
	const int intscidx = intersection2DIdx( vwr2d->geomID() );
	if ( intscidx<0 )
	    return;
	const Line2DInterSection* intsect = (*l2dintersections_)[intscidx];
	if ( !intsect )
	    return;
	Attrib::DescSet* ads2d = Attrib::eDSHolder().getDescSet( true, false );
	Attrib::DescSet* ads2dns = Attrib::eDSHolder().getDescSet( true, true );
	const Attrib::Desc* wvadesc =
	    ads2d->getDesc( vwr2d->selSpec(true).id() );
	if ( !wvadesc )
	    wvadesc = ads2dns->getDesc( vwr2d->selSpec(true).id() );
	const Attrib::Desc* vddesc =
	    ads2d->getDesc( vwr2d->selSpec(false).id() );
	if ( !vddesc )
	    vddesc = ads2dns->getDesc( vwr2d->selSpec(false).id() );

	if ( !wvadesc && !vddesc )
	    return;

	const SeisIOObjInfo wvasi( wvadesc ? wvadesc->getStoredID(true)
					   : vddesc->getStoredID(true) );
	const SeisIOObjInfo vdsi( vddesc ? vddesc->getStoredID(true)
					 : wvadesc->getStoredID(true));
	BufferStringSet wvalnms, vdlnms;
	wvasi.getLineNames( wvalnms );
	vdsi.getLineNames( vdlnms );
	TypeSet<Pos::GeomID> commongids;

	for ( int lidx=0; lidx<wvalnms.size(); lidx++ )
	{
	    const char* wvalnm = wvalnms.get(lidx).buf();
	    if ( vdlnms.isPresent(wvalnm) )
		commongids += Survey::GM().getGeomID( wvalnm );
	}

	const StepInterval<double> x1rg =
	    vwr2d->viewwin()->viewer().posRange( true );
	const StepInterval<int> trcrg =
	    vwr2d->getTrcKeyZSampling().hsamp_.trcRange();
	for ( int intposidx=0; intposidx<intsect->size(); intposidx++ )
	{
	    const Line2DInterSection::Point& intpos =
		intsect->getPoint( intposidx );
	    if ( !commongids.isPresent(intpos.line) )
		continue;
	    PlotAnnotation newannot;
	    if ( find2DViewer(intpos.line) )
		newannot.linetype_ = boldltype;

	    const int posidx = trcrg.getIndex( intpos.mytrcnr );
	    newannot.pos_ = mCast(float,x1rg.atIndex(posidx));
	    newannot.txt_ = Survey::GM().getName( intpos.line );
	    x1intannots += newannot;
	}
    }
    else
    {
	for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	{
	    const uiODViewer2D* idxvwr = viewers2d_[vwridx];
	    const TrcKeyZSampling& idxvwrtkzs = idxvwr->getTrcKeyZSampling();
	    TrcKeyZSampling::Dir idxvwrdir = idxvwrtkzs.defaultDir();
	    if ( vwr2d == idxvwr || vwr2ddir==idxvwrdir )
		continue;

	    PlotAnnotation newannot;
	    newannot.linetype_ = boldltype;

	    if ( vwr2ddir==TrcKeyZSampling::Inl )
	    {
		if ( idxvwrdir==TrcKeyZSampling::Crl )
		{
		    newannot.pos_ = (float) idxvwrtkzs.hsamp_.crlRange().start;
		    newannot.txt_ = tr( "CRL %1" ).arg( newannot.pos_ );
		    x1intannots += newannot;
		}
		else
		{
		    newannot.pos_ = idxvwrtkzs.zsamp_.start;
		    newannot.txt_ = tr( "ZSlice %1" ).arg(newannot.pos_);
		    x2intannots += newannot;
		}
	    }
	    else if ( vwr2ddir==TrcKeyZSampling::Crl )
	    {
		if ( idxvwrdir==TrcKeyZSampling::Inl )
		{
		    newannot.pos_ = (float) idxvwrtkzs.hsamp_.inlRange().start;
		    newannot.txt_ = tr( "INL %1" ).arg( newannot.pos_ );
		    x1intannots += newannot;
		}
		else
		{
		    newannot.pos_ = idxvwrtkzs.zsamp_.start;
		    newannot.txt_ = tr( "ZSlice %1" ).arg(newannot.pos_);
		    x2intannots += newannot;
		}
	    }
	    else
	    {
		if ( idxvwrdir==TrcKeyZSampling::Inl )
		{
		    newannot.pos_ = (float) idxvwrtkzs.hsamp_.inlRange().start;
		    newannot.txt_ = tr( "INL %1" ).arg( newannot.pos_ );
		    x1intannots += newannot;
		}
		else
		{
		    newannot.pos_ = (float) idxvwrtkzs.hsamp_.crlRange().start;
		    newannot.txt_ = tr( "CRL %1" ).arg( newannot.pos_ );
		    x2intannots += newannot;
		}
	    }
	}
    }

    vwr2d->viewwin()->viewer().handleChange( FlatView::Viewer::Annot );
}


void uiODViewer2DMgr::setAllIntersectionPositions()
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
    {
	uiODViewer2D* vwr2d = viewers2d_[vwridx];
	setVWR2DIntersectionPositions( vwr2d );
    }
}


int uiODViewer2DMgr::intersection2DIdx( Pos::GeomID newgeomid ) const
{
    if ( !l2dintersections_ )
	return -1;
    for ( int lidx=0; lidx<l2dintersections_->size(); lidx++ )
    {
	if ( (*l2dintersections_)[lidx] &&
	     (*l2dintersections_)[lidx]->geomID()==newgeomid )
	    return lidx;
    }

    return -1;
}


Line2DInterSection::Point uiODViewer2DMgr::intersectingLineID(
	const uiODViewer2D* vwr2d, float intpos ) const
{
    Line2DInterSection::Point udfintpoint( Survey::GM().cUndefGeomID(),
					   mUdf(int), mUdf(int) );
    const int intsecidx = intersection2DIdx( vwr2d->geomID() );
    if ( intsecidx<0 )
	return udfintpoint;

    const Line2DInterSection* int2d = (*l2dintersections_)[intsecidx];
    if ( !int2d ) return udfintpoint;

    const StepInterval<double> vwrxrg =
	vwr2d->viewwin()->viewer().posRange( true );
    const int intidx = vwrxrg.getIndex( intpos );
    if ( intidx<0 )
	return udfintpoint;
    StepInterval<int> vwrtrcrg = vwr2d->getTrcKeyZSampling().hsamp_.trcRange();
    const int inttrcnr = vwrtrcrg.atIndex( intidx );
    for ( int idx=0; idx<int2d->size(); idx++ )
    {
	const Line2DInterSection::Point& intpoint = int2d->getPoint( idx );
	if ( mIsEqual(intpoint.mytrcnr,inttrcnr,2.0) )
	    return intpoint;
    }

    return udfintpoint;
}


void uiODViewer2DMgr::vw2DPosChangedCB( CallBacker* )
{
    setAllIntersectionPositions();
}


void uiODViewer2DMgr::viewWinClosedCB( CallBacker* cb )
{
    mDynamicCastGet( uiODViewer2D*, vwr2d, cb );
    if ( vwr2d )
	remove2DViewer( vwr2d->id_, false );
}


void uiODViewer2DMgr::remove2DViewer( int id, bool byvisid )
{
    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	const int vwrid = byvisid ? viewers2d_[idx]->visid_
				  : viewers2d_[idx]->id_;
	if ( vwrid != id )
	    continue;

	delete viewers2d_.removeSingle( idx );
	setAllIntersectionPositions();
	return;
    }
}


void uiODViewer2DMgr::fillPar( IOPar& iop ) const
{
    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	const uiODViewer2D& vwr2d = *viewers2d_[idx];
	if ( !vwr2d.viewwin() ) continue;

	IOPar vwrpar;
	vwrpar.set( sKeyVisID(), viewers2d_[idx]->visid_ );
	bool wva = vwr2d.viewwin()->viewer().appearance().ddpars_.wva_.show_;
	vwrpar.setYN( sKeyWVA(), wva );
	vwrpar.set( sKeyAttrID(), vwr2d.selSpec(wva).id().asInt() );
	vwr2d.fillPar( vwrpar );

	iop.mergeComp( vwrpar, toString( idx ) );
    }
}


void uiODViewer2DMgr::usePar( const IOPar& iop )
{
    deepErase( viewers2d_ );

    for ( int idx=0; ; idx++ )
    {
	PtrMan<IOPar> vwrpar = iop.subselect( toString(idx) );
	if ( !vwrpar || !vwrpar->size() )
	{
	    if ( !idx ) continue;
	    break;
	}
	int visid; bool wva; int attrid;
	if ( vwrpar->get( sKeyVisID(), visid ) &&
		vwrpar->get( sKeyAttrID(), attrid ) &&
		    vwrpar->getYN( sKeyWVA(), wva ) )
	{
	    const int nrattribs = visServ().getNrAttribs( visid );
	    const int attrnr = nrattribs-1;
	    displayIn2DViewer( visid, attrnr, wva );
	    uiODViewer2D* curvwr = find2DViewer( visid, true );
	    if ( curvwr ) curvwr->usePar( *vwrpar );
	}
    }
}


void uiODViewer2DMgr::removeHorizon3D( EM::ObjectID emid )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->removeHorizon3D( emid );
}


void uiODViewer2DMgr::getLoadedHorizon3Ds( TypeSet<EM::ObjectID>& emids ) const
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->getLoadedHorizon3Ds( emids );
}


void uiODViewer2DMgr::addHorizon3Ds( const TypeSet<EM::ObjectID>& emids )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->addHorizon3Ds( emids );
}


void uiODViewer2DMgr::addNewTrackingHorizon3D( EM::ObjectID emid )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->addNewTrackingHorizon3D( emid );
    TypeSet<EM::ObjectID> emids;
    appl_.sceneMgr().getLoadedEMIDs(emids,EM::Horizon3D::typeStr());
    if ( emids.isPresent(emid) )
	return;
    appl_.sceneMgr().addEMItem( emid );
}


void uiODViewer2DMgr::removeHorizon2D( EM::ObjectID emid )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->removeHorizon2D( emid );
}


void uiODViewer2DMgr::getLoadedHorizon2Ds( TypeSet<EM::ObjectID>& emids ) const
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->getLoadedHorizon2Ds( emids );
}


void uiODViewer2DMgr::addHorizon2Ds( const TypeSet<EM::ObjectID>& emids )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->addHorizon2Ds( emids );
}


void uiODViewer2DMgr::addNewTrackingHorizon2D( EM::ObjectID emid )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->addNewTrackingHorizon2D( emid );
    TypeSet<EM::ObjectID> emids;
    appl_.sceneMgr().getLoadedEMIDs(emids,EM::Horizon2D::typeStr());
    if ( emids.isPresent(emid) )
	return;
    appl_.sceneMgr().addEMItem( emid );
}


void uiODViewer2DMgr::removeFault( EM::ObjectID emid )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->removeFault( emid );
}


void uiODViewer2DMgr::getLoadedFaults( TypeSet<EM::ObjectID>& emids ) const
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->getLoadedFaults( emids );
}


void uiODViewer2DMgr::addFaults( const TypeSet<EM::ObjectID>& emids )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->addFaults( emids );
}


void uiODViewer2DMgr::addNewTempFault( EM::ObjectID emid )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->addNewTempFault( emid );
    TypeSet<EM::ObjectID> emids;
    appl_.sceneMgr().getLoadedEMIDs(emids,EM::Fault3D::typeStr());
    if ( emids.isPresent(emid) )
	return;
    appl_.sceneMgr().addEMItem( emid );
}


void uiODViewer2DMgr::removeFaultSS( EM::ObjectID emid )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->removeFaultSS( emid );
}


void uiODViewer2DMgr::getLoadedFaultSSs( TypeSet<EM::ObjectID>& emids ) const
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->getLoadedFaultSSs( emids );
}


void uiODViewer2DMgr::addFaultSSs( const TypeSet<EM::ObjectID>& emids )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->addFaultSSs( emids );
}


void uiODViewer2DMgr::addNewTempFaultSS( EM::ObjectID emid )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->addNewTempFaultSS( emid );
    TypeSet<EM::ObjectID> emids;
    appl_.sceneMgr().getLoadedEMIDs(emids,EM::FaultStickSet::typeStr());
    if ( emids.isPresent(emid) )
	return;
    appl_.sceneMgr().addEMItem( emid );
}


void uiODViewer2DMgr::removePickSet( const MultiID& mid )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->removePickSet( mid );
}


void uiODViewer2DMgr::getLoadedPickSets( TypeSet<MultiID>& mids ) const
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->getLoadedPickSets( mids );
}


void uiODViewer2DMgr::addPickSets( const TypeSet<MultiID>& mids )
{
    for ( int vwridx=0; vwridx<viewers2d_.size(); vwridx++ )
	viewers2d_[vwridx]->addPickSets( mids );
}
