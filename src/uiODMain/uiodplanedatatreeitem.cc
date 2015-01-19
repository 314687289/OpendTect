/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jul 2003
___________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiodplanedatatreeitem.h"

#include "seistrctr.h"
#include "uiattribpartserv.h"
#include "uigridlinesdlg.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiseispartserv.h"
#include "uishortcutsmgr.h"
#include "uislicesel.h"
#include "uistrings.h"
#include "uivispartserv.h"
#include "uivisslicepos3d.h"
#include "uiwellpartserv.h"
#include "visplanedatadisplay.h"
#include "visrgbatexturechannel2rgba.h"
#include "vissurvscene.h"

#include "attribdescsetsholder.h"
#include "attribsel.h"
#include "iodir.h"
#include "ioman.h"
#include "keystrs.h"
#include "linekey.h"
#include "seisioobjinfo.h"
#include "settings.h"
#include "survinfo.h"
#include "welldata.h"
#include "wellman.h"
#include "zaxistransform.h"


static const int cPositionIdx = 990;
static const int cGridLinesIdx = 980;

static uiODPlaneDataTreeItem::Type getType( int mnuid )
{
    switch ( mnuid )
    {
	case 0: return uiODPlaneDataTreeItem::Empty; break;
	case 1: return uiODPlaneDataTreeItem::Default; break;
	case 2: return uiODPlaneDataTreeItem::RGBA; break;
	default: return uiODPlaneDataTreeItem::Empty;
    }
}


uiString uiODPlaneDataTreeItem::sAddDefaultData()
{ return tr("Add default data"); }


uiString uiODPlaneDataTreeItem::sAddColorBlended()
{ return uiStrings::sAddColBlend(); }


uiString uiODPlaneDataTreeItem::sAddAtWellLocation()
{ return tr("Add at Well location ..."); }


#define mParentShowSubMenu( treeitm, fromwell ) \
    uiMenu mnu( getUiParent(), uiStrings::sAction() ); \
    mnu.insertItem( new uiAction(uiStrings::sAdd(true)), 0 ); \
    mnu.insertItem( \
	new uiAction(uiODPlaneDataTreeItem::sAddDefaultData()), 1 ); \
    mnu.insertItem( \
	new uiAction(uiODPlaneDataTreeItem::sAddColorBlended()), 2 ); \
    if ( fromwell ) \
	mnu.insertItem( \
	new uiAction(uiODPlaneDataTreeItem::sAddAtWellLocation()), 3 ); \
    addStandardItems( mnu ); \
    const int mnuid = mnu.exec(); \
    if ( mnuid==0 || mnuid==1 || mnuid==2 ) \
        addChild( new treeitm(-1,getType(mnuid)), false ); \
    else if ( mnuid==3 ) \
    { \
	TypeSet<MultiID> wellids; \
	if ( !applMgr()->wellServer()->selectWells(wellids) ) \
	    return true; \
	for ( int idx=0; idx<wellids.size(); idx++ ) \
	{ \
	    Well::Data* wd = Well::MGR().get( wellids[idx] ); \
	    if ( !wd ) continue; \
	    treeitm* itm = new treeitm( -1, getType(0) ); \
	    addChild( itm, false ); \
	    itm->setAtWellLocation( *wd ); \
	    itm->displayDefaultData(); \
	} \
    } \
    handleStandardItems( mnuid ); \
    return true


uiODPlaneDataTreeItem::uiODPlaneDataTreeItem( int did, OD::SliceType o, Type t )
    : orient_(o)
    , type_(t)
    , positiondlg_(0)
    , positionmnuitem_(tr("Position ..."),cPositionIdx)
    , gridlinesmnuitem_(tr("Gridlines ..."),cGridLinesIdx)
{
    displayid_ = did;
    positionmnuitem_.iconfnm = "orientation64";
    gridlinesmnuitem_.iconfnm = "gridlines";
}


uiODPlaneDataTreeItem::~uiODPlaneDataTreeItem()
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_));
    if ( pdd )
    {
	pdd->selection()->remove( mCB(this,uiODPlaneDataTreeItem,selChg) );
	pdd->deSelection()->remove( mCB(this,uiODPlaneDataTreeItem,selChg) );
	pdd->unRef();
    }

//  getItem()->keyPressed.remove( mCB(this,uiODPlaneDataTreeItem,keyPressCB) );
    visserv_->getUiSlicePos()->positionChg.remove(
			mCB(this,uiODPlaneDataTreeItem,posChange) );

    visserv_->getUiSlicePos()->setDisplay( -1 );
    delete positiondlg_;
}


bool uiODPlaneDataTreeItem::init()
{
    if ( displayid_==-1 )
    {
	RefMan<visSurvey::PlaneDataDisplay> pdd =
	    new visSurvey::PlaneDataDisplay;
	displayid_ = pdd->id();
	if ( type_ == RGBA )
	{
	    pdd->setChannels2RGBA( visBase::RGBATextureChannel2RGBA::create() );
	    pdd->addAttrib();
	    pdd->addAttrib();
	    pdd->addAttrib();
	}

	pdd->setOrientation( orient_);
	visserv_->addObject( pdd, sceneID(), true );

	BufferString res;
	Settings::common().get( "dTect.Texture2D Resolution", res );
	for ( int idx=0; idx<pdd->nrResolutions(); idx++ )
	{
	    if ( res == pdd->getResolutionName(idx) )
		pdd->setResolution( idx, 0 );
	}

	if ( type_ == Default )
	    displayDefaultData();
    }

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_));
    if ( !pdd ) return false;

    pdd->ref();
    pdd->selection()->notify( mCB(this,uiODPlaneDataTreeItem,selChg) );
    pdd->deSelection()->notify( mCB(this,uiODPlaneDataTreeItem,selChg) );

//  getItem()->keyPressed.notify( mCB(this,uiODPlaneDataTreeItem,keyPressCB) );
    visserv_->getUiSlicePos()->positionChg.notify(
			mCB(this,uiODPlaneDataTreeItem,posChange) );

    return uiODDisplayTreeItem::init();
}


void uiODPlaneDataTreeItem::setAtWellLocation( const Well::Data& wd )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_));
    if ( !pdd ) return;

    const Coord surfacecoord = wd.info().surfacecoord;
    const BinID bid = SI().transform( surfacecoord );
    TrcKeyZSampling cs = pdd->getTrcKeyZSampling();
    if ( orient_ == OD::InlineSlice )
	cs.hrg.setInlRange( Interval<int>(bid.inl(),bid.inl()) );
    else
	cs.hrg.setCrlRange( Interval<int>(bid.crl(),bid.crl()) );

    pdd->setTrcKeyZSampling( cs );
    select();
}


bool uiODPlaneDataTreeItem::getDefaultDescID( Attrib::DescID& descid )
{
    BufferString key( IOPar::compKey(sKey::Default(),
		      SeisTrcTranslatorGroup::sKeyDefault3D()) );
    BufferString midstr( SI().pars().find(key) );
    if ( midstr.isEmpty() )
    {
	const IOObjContext ctxt( SeisTrcTranslatorGroup::ioContext() );
	const IODir iodir ( ctxt.getSelKey() );
	const ObjectSet<IOObj>& ioobjs = iodir.getObjs();
	int nrod3d = 0;
	int def3didx = 0;
	for ( int idx=0; idx<ioobjs.size(); idx++ )
	{
	    SeisIOObjInfo seisinfo( ioobjs[idx] );
	    if ( seisinfo.isOK() && !seisinfo.is2D() )
	    {
		nrod3d++;
		def3didx = idx;
	    }
	}

	if ( nrod3d == 1 )
	    midstr = ioobjs[def3didx]->key();
    }

    uiAttribPartServer* attrserv = applMgr()->attrServer();
    descid = attrserv->getStoredID( midstr.buf(),false );
    const Attrib::DescSet* ads =
	Attrib::DSHolder().getDescSet( false, true );
    if ( descid.isValid() && ads )
	return true;

    uiString msg = tr("No or no valid default volume found."
		      "You can set a default volume in the 'Manage Seismics' "
		      "window. Do you want to go there now? "
		      "On 'No' an empty plane will be added");
    const bool tomanage = uiMSG().askGoOn( msg );
    if ( tomanage )
    {
	applMgr()->seisServer()->manageSeismics( 0, true );
	return getDefaultDescID( descid );
    }

    return false;
}


bool uiODPlaneDataTreeItem::displayDefaultData()
{
    Attrib::DescID descid;
    if ( !getDefaultDescID(descid) )
	return false;

    const Attrib::DescSet* ads =
	Attrib::DSHolder().getDescSet( false, true );
    Attrib::SelSpec as( 0, descid, false, "" );
    as.setRefFromID( *ads );
    visserv_->setSelSpec( displayid_, 0, as );
    const bool res = visserv_->calculateAttrib( displayid_, 0, false );
    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
    return res;
}


void uiODPlaneDataTreeItem::posChange( CallBacker* )
{
    uiSlicePos3DDisp* slicepos = visserv_->getUiSlicePos();
    if ( slicepos->getDisplayID() != displayid_ )
	return;

    movePlaneAndCalcAttribs( slicepos->getTrcKeyZSampling() );
}


void uiODPlaneDataTreeItem::selChg( CallBacker* )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
    visserv_->getObject(displayid_))

    if ( pdd->isSelected() )
	visserv_->getUiSlicePos()->setDisplay( displayid_ );
}


BufferString uiODPlaneDataTreeItem::createDisplayName() const
{
    BufferString res;
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_))
    const TrcKeyZSampling cs = pdd->getTrcKeyZSampling(true,true);
    const OD::SliceType orientation = pdd->getOrientation();

    if ( orientation==OD::InlineSlice )
	res = cs.hrg.start.inl();
    else if ( orientation==OD::CrosslineSlice )
	res = cs.hrg.start.crl();
    else
    {
	mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()))
	if ( !scene )
	    res = cs.zsamp_.start;
	else
	{
	    const ZDomain::Def& zdef = scene->zDomainInfo().def_;
	    const float zval = cs.zsamp_.start * zdef.userFactor();
	    res = toString( zdef.isTime() || zdef.userFactor()==1000
		    ? (float)(mNINT32(zval)) : zval );
	}
    }

    return res;
}


void uiODPlaneDataTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || menu->menuID() != displayID() )
	return;

    const bool islocked = visserv_->isLocked( displayid_ );
    mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &positionmnuitem_,
		      !islocked, false );
    mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &gridlinesmnuitem_,
		      true, false );
}


void uiODPlaneDataTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() || mnuid==-1 )
	return;

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_))

    if ( mnuid==positionmnuitem_.id )
    {
	menu->setIsHandled(true);
	if ( !pdd ) return;
	delete positiondlg_;
	TrcKeyZSampling maxcs = SI().sampling(true);
	mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()))
	if ( scene && scene->getZAxisTransform() )
	    maxcs = scene->getTrcKeyZSampling();

	positiondlg_ = new uiSliceSelDlg( getUiParent(),
				pdd->getTrcKeyZSampling(true,true), maxcs,
				mCB(this,uiODPlaneDataTreeItem,updatePlanePos),
				(uiSliceSel::Type)orient_,scene->zDomainInfo());
	positiondlg_->windowClosed.notify(
		mCB(this,uiODPlaneDataTreeItem,posDlgClosed) );
	positiondlg_->go();
	pdd->getMovementNotifier()->notify(
		mCB(this,uiODPlaneDataTreeItem,updatePositionDlg) );
	applMgr()->enableMenusAndToolBars( false );
	applMgr()->visServer()->disabToolBars( false );
    }
    else if ( mnuid == gridlinesmnuitem_.id )
    {
	menu->setIsHandled(true);
	if ( !pdd ) return;

	uiGridLinesDlg gldlg( getUiParent(), pdd );
	gldlg.go();
    }
}


void uiODPlaneDataTreeItem::updatePositionDlg( CallBacker* )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_))
    const TrcKeyZSampling newcs = pdd->getTrcKeyZSampling( true, true );
    positiondlg_->setTrcKeyZSampling( newcs );
}


void uiODPlaneDataTreeItem::posDlgClosed( CallBacker* )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_))
    TrcKeyZSampling newcs = positiondlg_->getTrcKeyZSampling();
    bool samepos = newcs == pdd->getTrcKeyZSampling();
    if ( positiondlg_->uiResult() && !samepos )
	movePlaneAndCalcAttribs( newcs );

    applMgr()->enableMenusAndToolBars( true );
    applMgr()->enableSceneManipulation( true );
    pdd->getMovementNotifier()->remove(
		mCB(this,uiODPlaneDataTreeItem,updatePositionDlg) );
}


void uiODPlaneDataTreeItem::updatePlanePos( CallBacker* cb )
{
    mDynamicCastGet(uiSliceSel*,slicesel,cb)
    if ( !slicesel ) return;

    movePlaneAndCalcAttribs( slicesel->getTrcKeyZSampling() );
}


void uiODPlaneDataTreeItem::movePlaneAndCalcAttribs( const TrcKeyZSampling& cs )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_))

    pdd->annotateNextUpdateStage( true );
    pdd->setTrcKeyZSampling( cs );
    pdd->resetManipulation();
    pdd->annotateNextUpdateStage( true );
    for ( int attrib=0; attrib<visserv_->getNrAttribs(displayid_); attrib++ )
	visserv_->calculateAttrib( displayid_, attrib, false );
    pdd->annotateNextUpdateStage( false );

    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODPlaneDataTreeItem::keyPressCB( CallBacker* cb )
{
    mCBCapsuleGet(uiKeyDesc,caps,cb)
    if ( !caps ) return;
    const uiShortcutsList& scl = SCMgr().getList( "ODScene" );
    BufferString act( scl.nameOf(caps->data) );
    const bool fwd = act == "Move slice forward";
    const bool bwd = fwd ? false : act == "Move slice backward";
    if ( !fwd && !bwd ) return;

    int step = scl.valueOf(caps->data);
    caps->data.setKey( 0 );
    movePlane( fwd, step );
}


void uiODPlaneDataTreeItem::movePlane( bool forward, int step )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_))

    TrcKeyZSampling cs = pdd->getTrcKeyZSampling();
    const int dir = forward ? step : -step;

    if ( pdd->getOrientation() == OD::InlineSlice )
    {
	cs.hrg.start.inl() += cs.hrg.step.inl() * dir;
	cs.hrg.stop.inl() = cs.hrg.start.inl();
    }
    else if ( pdd->getOrientation() == OD::CrosslineSlice )
    {
	cs.hrg.start.crl() += cs.hrg.step.crl() * dir;
	cs.hrg.stop.crl() = cs.hrg.start.crl();
    }
    else if ( pdd->getOrientation() == OD::ZSlice )
    {
	cs.zsamp_.start += cs.zsamp_.step * dir;
	cs.zsamp_.stop = cs.zsamp_.start;
    }
    else
	return;

    movePlaneAndCalcAttribs( cs );
}


uiTreeItem*
    uiODInlineTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    ODMainWin()->applMgr().visServer()->getObject(visid));

    if ( !pdd || pdd->getOrientation()!=OD::InlineSlice )
	return 0;

    mDynamicCastGet( visBase::RGBATextureChannel2RGBA*, rgba,
		     pdd->getChannels2RGBA() );

    return new uiODInlineTreeItem( visid,
	rgba ? uiODPlaneDataTreeItem::RGBA : uiODPlaneDataTreeItem::Empty );
}


uiODInlineParentTreeItem::uiODInlineParentTreeItem()
    : uiODTreeItem( "In-line" )
{ }


bool uiODInlineParentTreeItem::showSubMenu()
{
    if ( !SI().crlRange(true).width() ||
	  SI().zRange(true).width() < SI().zStep() * 0.5 )
    {
	uiMSG().warning( tr("Flat survey, disabled inline display") );
	return false;
    }

    mParentShowSubMenu( uiODInlineTreeItem, true );
}


uiODInlineTreeItem::uiODInlineTreeItem( int id, Type tp )
    : uiODPlaneDataTreeItem( id, OD::InlineSlice, tp )
{}


uiTreeItem*
    uiODCrosslineTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd,
		     ODMainWin()->applMgr().visServer()->getObject(visid));

    if ( !pdd || pdd->getOrientation()!=OD::CrosslineSlice )
	return 0;

    mDynamicCastGet(visBase::RGBATextureChannel2RGBA*,rgba,
		    pdd->getChannels2RGBA());
    return new uiODCrosslineTreeItem( visid,
	rgba ? uiODPlaneDataTreeItem::RGBA : uiODPlaneDataTreeItem::Empty );
}


uiODCrosslineParentTreeItem::uiODCrosslineParentTreeItem()
    : uiODTreeItem( "Cross-line" )
{ }


bool uiODCrosslineParentTreeItem::showSubMenu()
{
    if ( !SI().inlRange(true).width() ||
	  SI().zRange(true).width() < SI().zStep() * 0.5 )
    {
	uiMSG().warning( tr("Flat survey, disabled cross-line display") );
	return false;
    }

    mParentShowSubMenu( uiODCrosslineTreeItem, true );
}


uiODCrosslineTreeItem::uiODCrosslineTreeItem( int id, Type tp )
    : uiODPlaneDataTreeItem( id, OD::CrosslineSlice, tp )
{}


uiTreeItem*
    uiODZsliceTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd,
		     ODMainWin()->applMgr().visServer()->getObject(visid));

    if ( !pdd || pdd->getOrientation()!=OD::ZSlice )
	return 0;

    mDynamicCastGet(visBase::RGBATextureChannel2RGBA*,rgba,
		    pdd->getChannels2RGBA());

    return new uiODZsliceTreeItem( visid,
	rgba ? uiODPlaneDataTreeItem::RGBA : uiODPlaneDataTreeItem::Empty );
}


uiODZsliceParentTreeItem::uiODZsliceParentTreeItem()
    : uiODTreeItem( "Z-slice" )
{}


bool uiODZsliceParentTreeItem::showSubMenu()
{
     if ( !SI().inlRange(true).width() || !SI().crlRange(true).width() )
     {
	 uiMSG().warning( tr("Flat survey, disabled z display") );
	 return false;
     }

    mParentShowSubMenu( uiODZsliceTreeItem, false );
}


uiODZsliceTreeItem::uiODZsliceTreeItem( int id, Type tp )
    : uiODPlaneDataTreeItem( id, OD::ZSlice, tp )
{
}
