/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiodmenumgr.h"

#include "ui3dviewer.h"
#include "uicrdevenv.h"
#include "uifiledlg.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodfaulttoolman.h"
#include "uiodhelpmenumgr.h"
#include "uiodscenemgr.h"
#include "uiodstdmenu.h"
#include "uiproxydlg.h"
#include "uisettings.h"
#include "uiseispartserv.h"
#include "uistrings.h"
#include "uitextfile.h"
#include "uitextedit.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"
#include "uivispartserv.h"
#include "uivolprocchain.h"
#include "visemobjdisplay.h"

#include "dirlist.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "keystrs.h"
#include "measuretoolman.h"
#include "oddirs.h"
#include "odinst.h"
#include "odsysmem.h"
#include "odver.h"
#include "settings.h"
#include "od_ostream.h"
#include "survinfo.h"
#include "texttranslator.h"
#include "thread.h"


static const char* sKeyIconSetNm = "Icon set name";


uiODMenuMgr::uiODMenuMgr( uiODMain* a )
    : appl_(*a)
    , dTectTBChanged(this)
    , dTectMnuChanged(this)
    , helpmgr_(0)
    , measuretoolman_(0)
    , inviewmode_(false)
    , langmnu_(0)
{
    surveymnu_ = appl_.menuBar()->addMenu( new uiMenu(uiStrings::sSurvey()) );
    analmnu_ = appl_.menuBar()->addMenu( new uiMenu(uiStrings::sAnalysis()) );
    procmnu_ = appl_.menuBar()->addMenu( new uiMenu(uiStrings::sProcessing()) );
    scenemnu_ = appl_.menuBar()->addMenu( new uiMenu(uiStrings::sScenes()) );
    viewmnu_ = appl_.menuBar()->addMenu( new uiMenu(uiStrings::sView()) );
    utilmnu_ = appl_.menuBar()->addMenu( new uiMenu(uiStrings::sUtilities()) );
    helpmnu_ = appl_.menuBar()->addMenu( new uiMenu(uiStrings::sHelp()) );

    dtecttb_ = new uiToolBar( &appl_, "OpendTect Tools", uiToolBar::Top );
    cointb_ = new uiToolBar( &appl_, "Graphical Tools", uiToolBar::Left );
    mantb_ = new uiToolBar( &appl_, "Manage Data", uiToolBar::Right );

    faulttoolman_ = new uiODFaultToolMan( appl_ );

    uiVisPartServer* visserv = appl_.applMgr().visServer();
    visserv->createToolBars();

    IOM().surveyChanged.notify( mCB(this,uiODMenuMgr,updateDTectToolBar) );
    IOM().surveyChanged.notify( mCB(this,uiODMenuMgr,updateDTectMnus) );
    visserv->selectionmodechange.notify(
				mCB(this,uiODMenuMgr,selectionMode) );
}


uiODMenuMgr::~uiODMenuMgr()
{
    delete appl_.removeToolBar( dtecttb_ );
    delete appl_.removeToolBar( cointb_ );
    delete appl_.removeToolBar( mantb_ );
    delete helpmgr_;
    delete faulttoolman_;
    delete measuretoolman_;
}


void uiODMenuMgr::initSceneMgrDepObjs( uiODApplMgr* appman,
				       uiODSceneMgr* sceneman )
{
    uiMenuBar* menubar = appl_.menuBar();
    fillSurveyMenu();
    fillAnalMenu();
    fillProcMenu();
    fillSceneMenu();
    fillViewMenu();
    fillUtilMenu();
    menubar->insertSeparator();
    helpmgr_ = new uiODHelpMenuMgr( this );

    fillDtectTB( appman );
    fillCoinTB( sceneman );
    fillManTB();

    measuretoolman_ = new MeasureToolMan( appl_ );
}


uiMenu* uiODMenuMgr::docMnu()
{ return helpmgr_->getDocMenu(); }


uiMenu* uiODMenuMgr::getBaseMnu( uiODApplMgr::ActType at )
{
    return at == uiODApplMgr::Imp ? impmnu_ :
	  (at == uiODApplMgr::Exp ? expmnu_ : manmnu_);
}


uiMenu* uiODMenuMgr::getMnu( bool imp, uiODApplMgr::ObjType ot )
{
    return imp ? impmnus_[(int)ot] : expmnus_[(int)ot];
}


void uiODMenuMgr::updateStereoMenu()
{
    ui3DViewer::StereoType type =
			(ui3DViewer::StereoType)sceneMgr().getStereoType();
    stereooffitm_->setChecked( type == ui3DViewer::None );
    stereoredcyanitm_->setChecked( type == ui3DViewer::RedCyan );
    stereoquadbufitm_->setChecked( type == ui3DViewer::QuadBuffer );
    stereooffsetitm_->setEnabled( type != ui3DViewer::None );
}


void uiODMenuMgr::updateViewMode( bool isview )
{
    if ( inviewmode_ == isview )
	return;
    toggViewMode( 0 );
}


void uiODMenuMgr::updateAxisMode( bool shwaxis )
{ cointb_->turnOn( axisid_, shwaxis ); }

bool uiODMenuMgr::isSoloModeOn() const
{ return cointb_->isOn( soloid_ ); }

void uiODMenuMgr::enableMenuBar( bool yn )
{ appl_.menuBar()->setSensitive( yn ); }

void uiODMenuMgr::enableActButton( bool yn )
{
    if ( yn )
	{ cointb_->setSensitive( actviewid_, true ); return; }

    if ( !inviewmode_ )
	toggViewMode(0);
    cointb_->setSensitive( actviewid_, false );
}


#define mInsertItem(menu,txt,id) \
    menu->insertItem( \
	new uiAction(txt,mCB(this,uiODMenuMgr,handleClick)), id )

#define mInsertPixmapItem(menu,txt,id,pmfnm) { \
    menu->insertItem( \
	new uiAction(txt,mCB(this,uiODMenuMgr,handleClick), \
			pmfnm), id ); }

#define mCleanUpImpExpSets(set) \
{ \
    while ( !set.isEmpty() ) \
    { \
	uiMenu* pmnu = set.removeSingle(0); \
	if ( pmnu ) delete pmnu; \
    } \
}

void uiODMenuMgr::fillSurveyMenu()
{
    mInsertPixmapItem( surveymnu_, tr("Select/Setup ..."), mManSurveyMnuItm,
		       "survey" )

    uiMenu* sessionitm = new uiMenu( &appl_, tr("Session") ) ;
    mInsertItem( sessionitm, uiStrings::sSave(false), mSessSaveMnuItm );
    mInsertItem( sessionitm, tr("Restore ..."), mSessRestMnuItm );
    mInsertItem( sessionitm, tr("Auto-load ..."), mSessAutoMnuItm );
    surveymnu_->insertItem( sessionitm );
    surveymnu_->insertSeparator();

    impmnu_ = new uiMenu( &appl_, uiStrings::sImport() );
    fillImportMenu();
    surveymnu_->insertItem( impmnu_ );

    expmnu_ = new uiMenu( &appl_, uiStrings::sExport() );
    fillExportMenu();
    surveymnu_->insertItem( expmnu_ );

    manmnu_ = new uiMenu( &appl_, tr("Manage") );
    fillManMenu();
    surveymnu_->insertItem( manmnu_ );

    preloadmnu_ = new uiMenu( &appl_, tr("Pre-load") );
    mInsertItem( preloadmnu_, uiStrings::sSeismics(false), mPreLoadSeisMnuItm );
    mInsertItem( preloadmnu_, uiStrings::sHorizons(false), mPreLoadHorMnuItm );
    surveymnu_->insertItem( preloadmnu_ );

    surveymnu_->insertSeparator();
    mInsertItem( surveymnu_, tr("Exit"), mExitMnuItm );
}



void uiODMenuMgr::fillImportMenu()
{
    impmnu_->clear();

    uiMenu* impattr = new uiMenu( &appl_, uiStrings::sAttribute() );
    uiMenu* impseis = new uiMenu( &appl_, uiStrings::sSeismics(true) );
    uiMenu* imphor = new uiMenu( &appl_, uiStrings::sHorizon(true) );
    uiMenu* impfault = new uiMenu( &appl_, uiStrings::sFaults(true) );
    uiMenu* impfaultstick = new uiMenu( &appl_, tr("FaultStickSets") );
    uiMenu* impwell = new uiMenu( &appl_, uiStrings::sWells(true) );
    uiMenu* imppick = new uiMenu( &appl_, tr("PickSets/Polygons") );
    uiMenu* impwvlt = new uiMenu( &appl_, tr("Wavelets") );
    uiMenu* impmute = new uiMenu( &appl_, tr("Mute Functions") );
    uiMenu* impcpd = new uiMenu( &appl_, tr("Cross-plot Data") );
    uiMenu* impvelfn = new uiMenu( &appl_, tr("Velocity Functions") );
    uiMenu* imppdf = new uiMenu( &appl_, tr("Probability Density Functions") );

    impmnu_->insertItem( impattr );
    mInsertItem( impmnu_, uiStrings::sColorTable(false), mImpColTabMnuItm );
    impmnu_->insertItem( impcpd );
    impmnu_->insertItem( impfault );
    impmnu_->insertItem( impfaultstick );
    impmnu_->insertItem( imphor );
    impmnu_->insertItem( impmute );
    impmnu_->insertItem( imppick );
    impmnu_->insertItem( imppdf );
    impmnu_->insertItem( impseis );
    impmnu_->insertItem( impvelfn );
    impmnu_->insertItem( impwvlt );
    impmnu_->insertItem( impwell );
    impmnu_->insertSeparator();

    const uiString ascii = uiStrings::sASCII(false);
    const char* ascic = "ascii";

    mInsertPixmapItem( impattr, ascii, mImpAttrMnuItm, ascic );
    mInsertPixmapItem( imppick, ascii, mImpPickAsciiMnuItm, ascic );
    mInsertPixmapItem( impwvlt, ascii, mImpWvltAsciiMnuItm, ascic );
    mInsertPixmapItem( impmute, ascii, mImpMuteDefAsciiMnuItm, ascic );
    mInsertPixmapItem( impcpd, ascii, mImpPVDSAsciiMnuItm, ascic );
    mInsertPixmapItem( impvelfn, ascii, mImpVelocityAsciiMnuItm, ascic );
    mInsertPixmapItem( imppdf, tr("RokDoc ASCII ..."),
					mImpPDFAsciiMnuItm, ascic );

    uiMenu* impseissimple = new uiMenu( &appl_, tr("Simple File"), ascic );
    mInsertItem( impseissimple, uiStrings::s2D(false), mImpSeisSimple2DMnuItm );
    mInsertItem( impseissimple, uiStrings::s3D(false), mImpSeisSimple3DMnuItm );
    mInsertItem( impseissimple, "Prestack 2D ...", mImpSeisSimplePS2DMnuItm );
    mInsertItem( impseissimple, "Prestack 3D ...", mImpSeisSimplePS3DMnuItm );
    impseis->insertItem( impseissimple );
    uiMenu* impcbvsseis = new uiMenu( &appl_, tr("CBVS"), "od" );
    mInsertItem( impcbvsseis, tr("From File ..."), mImpSeisCBVSMnuItm );
    mInsertItem( impcbvsseis, tr("From Other Survey ..."),
		 mImpSeisCBVSOtherSurvMnuItm );
    impseis->insertItem( impcbvsseis );

    uiMenu* imphorasc = new uiMenu( &appl_, uiStrings::sASCII(true) );
    mInsertItem( imphorasc, tr("Geometry 2D ..."),
                 mImpHor2DAsciiMnuItm );
    mInsertItem( imphorasc, tr("Geometry 3D ..."), mImpHorAsciiMnuItm );
    mInsertItem( imphorasc, tr("Attributes 3D ..."),
                mImpHorAsciiAttribMnuItm );
    mInsertItem( imphorasc, tr("Bulk 3D ..."), mImpBulkHorAsciiMnuIm );
    imphor->insertItem( imphorasc );

    mInsertPixmapItem( impfault, ascii, mImpFaultMnuItm, ascic );
    mInsertItem( impfaultstick, tr("ASCII 2D ..."),
                mImpFaultSSAscii2DMnuItm );
    mInsertItem( impfaultstick, tr("ASCII 3D ..."),
                mImpFaultSSAscii3DMnuItm );

    uiMenu* impwellasc = new uiMenu( &appl_, uiStrings::sASCII(true) );
    mInsertItem( impwellasc, uiStrings::sTrack(false),
                 mImpWellAsciiTrackMnuItm );
    mInsertItem( impwellasc, uiStrings::sLogs(false), mImpWellAsciiLogsMnuItm );
    mInsertItem( impwellasc, uiStrings::sMarkers(false),
                 mImpWellAsciiMarkersMnuItm );
    impwell->insertItem( impwellasc );
    mInsertItem( impwell, tr("Simple Multi-Well ..."), mImpWellSimpleMnuItm );

    uiMenu* impwellbulk = new uiMenu( &appl_, tr("Bulk") );
    mInsertItem( impwellbulk, uiStrings::sTrack(false), mImpBulkWellTrackItm );
    mInsertItem( impwellbulk, uiStrings::sLogs(false), mImpBulkWellLogsItm );
    mInsertItem( impwellbulk, uiStrings::sMarkers(false),
                 mImpBulkWellMarkersItm );
    mInsertItem( impwellbulk, tr("Depth/Time Model ..."), mImpBulkWellD2TItm );
    impwell->insertItem( impwellbulk );

// Fill impmenus_
    impmnus_.erase();
    impmnus_.allowNull();
    for ( int idx=0; idx<uiODApplMgr::NrObjTypes; idx++ )
	impmnus_ += 0;

#define mAddImpMnu(tp,mnu) impmnus_.replace( (int)uiODApplMgr::tp, mnu )
    mAddImpMnu( Seis, impseis );
    mAddImpMnu( Hor, imphor );
    mAddImpMnu( Flt, impfault );
    mAddImpMnu( Fltss, impfaultstick );
    mAddImpMnu( Wll, impwell );
    mAddImpMnu( Attr, impattr );
    mAddImpMnu( Pick, imppick );
    mAddImpMnu( Wvlt, impwvlt );
    mAddImpMnu( MDef, impmute );
    mAddImpMnu( Vel, impvelfn );
    mAddImpMnu( PDF, imppdf );
}


void uiODMenuMgr::fillExportMenu()
{
    expmnu_->clear();
    uiMenu* expseis = new uiMenu( &appl_, uiStrings::sSeismics(true) );
    uiMenu* exphor = new uiMenu( &appl_, uiStrings::sHorizons(true) );
    uiMenu* expflt = new uiMenu( &appl_, uiStrings::sFaults(true) );
    uiMenu* expfltss = new uiMenu( &appl_, tr("FaultStickSets") );
    uiMenu* expgeom2d = new uiMenu( &appl_, tr("Geometry 2D") );
    uiMenu* exppick = new uiMenu( &appl_, tr("PickSets/Polygons") );
    uiMenu* expwvlt = new uiMenu( &appl_, tr("Wavelets") );
    uiMenu* expmute = new uiMenu( &appl_, tr("Mute Functions") );
    uiMenu* exppdf =
	new uiMenu( &appl_, tr("Probability Density Functions") );

    expmnu_->insertItem( expflt );
    expmnu_->insertItem( expfltss );
    expmnu_->insertItem( expgeom2d );
    expmnu_->insertItem( exphor );
    expmnu_->insertItem( expmute );
    expmnu_->insertItem( exppick );
    expmnu_->insertItem( exppdf );
    expmnu_->insertItem( expseis );
    expmnu_->insertItem( expwvlt );
    expmnu_->insertSeparator();

    uiMenu* expseissimple = new uiMenu( &appl_, tr("Simple File") );
    mInsertItem( expseissimple, uiStrings::s2D(false), mExpSeisSimple2DMnuItm );
    mInsertItem( expseissimple, uiStrings::s3D(false), mExpSeisSimple3DMnuItm );
    mInsertItem( expseissimple, tr("Prestack 3D ..."),
                 mExpSeisSimplePS3DMnuItm );
    expseis->insertItem( expseissimple );

    const uiString ascii = uiStrings::sASCII(false);

    mInsertItem( exphor, tr("ASCII 2D ..."), mExpHorAscii2DMnuItm );
    mInsertItem( exphor, tr("ASCII 3D ..."), mExpHorAscii3DMnuItm );
    mInsertItem( expflt, ascii, mExpFltAsciiMnuItm );
    mInsertItem( expfltss, ascii, mExpFltSSAsciiMnuItm );
    mInsertItem( expgeom2d, ascii, mExpGeom2DMnuItm );
    mInsertItem( exppick, ascii, mExpPickAsciiMnuItm );
    mInsertItem( expwvlt, ascii, mExpWvltAsciiMnuItm );
    mInsertItem( expmute, ascii, mExpMuteDefAsciiMnuItm );
    mInsertItem( exppdf, tr("RokDoc ASCII..."), mExpPDFAsciiMnuItm );

// Fill expmenus_
    expmnus_.erase();
    expmnus_.allowNull();
    for ( int idx=0; idx<uiODApplMgr::NrObjTypes; idx++ )
	expmnus_ += 0;

#define mAddExpMnu(tp,mnu) expmnus_.replace( (int)uiODApplMgr::tp, mnu )
    mAddExpMnu( Seis, expseis );
    mAddExpMnu( Hor, exphor );
    mAddExpMnu( Flt, expflt );
    mAddExpMnu( Fltss, expfltss );
    mAddExpMnu( Pick, exppick );
    mAddExpMnu( Wvlt, expwvlt );
    mAddExpMnu( MDef, expmute );
    mAddExpMnu( PDF, exppdf );
    mAddExpMnu( Geom, expgeom2d );
}


void uiODMenuMgr::fillManMenu()
{
    manmnu_->clear();
    mInsertPixmapItem( manmnu_, tr("Attribute Sets ..."), mManAttrMnuItm,
		       "man_attrs" );
    mInsertPixmapItem( manmnu_, tr("Bodies ..."), mManBodyMnuItm,
		       "man_body" );
    mInsertPixmapItem( manmnu_, tr("Color Tables ..."), mManColTabMnuItm,
		       "empty" );
    mInsertPixmapItem( manmnu_, tr("Cross-plot Data ..."), mManCrossPlotItm,
			"manxplot" );
    mInsertPixmapItem( manmnu_, uiStrings::sFaults(false), mManFaultMnuItm,
		       "man_flt" )
    mInsertPixmapItem( manmnu_, tr("FaultStickSets ..."), mManFaultStickMnuItm,
			"man_fltss" );
    mInsertPixmapItem( manmnu_, tr("Geometry 2D ..."), mManGeomItm,
		       "man2dgeom" );
    if ( SI().survDataType() == SurveyInfo::No2D )
	mInsertPixmapItem( manmnu_, uiStrings::sHorizons(false),
			   mManHor3DMnuItm, "man_hor" )
    else
    {
	uiMenu* mnu = new uiMenu( &appl_, uiStrings::sHorizons(true),"man_hor");
	mInsertItem( mnu, uiStrings::s2D(false), mManHor2DMnuItm );
	mInsertItem( mnu, uiStrings::s3D(false), mManHor3DMnuItm );
	manmnu_->insertItem( mnu );
    }

    mInsertPixmapItem( manmnu_, tr("Layer Properties ..."), mManPropsMnuItm,
			"man_props" );
    mInsertPixmapItem( manmnu_, tr("PickSets/Polygons ..."), mManPickMnuItm,
			"man_picks" );
    mInsertPixmapItem( manmnu_, tr("Probability Density Functions ..."),
		 mManPDFMnuItm, "man_prdfs" );
    add2D3DMenuItem( *manmnu_, "man_seis", uiStrings::sSeismics(true),
		     mManSeis2DMnuItm, mManSeis3DMnuItm );
    add2D3DMenuItem( *manmnu_, "man_ps", tr("Seismics Prestack"),
		     mManSeisPS2DMnuItm, mManSeisPS3DMnuItm );
    mInsertPixmapItem( manmnu_, tr("Sessions ..."), mManSessMnuItm, "empty" )
    mInsertPixmapItem( manmnu_, uiStrings::sStratigraphy(false),
                       mManStratMnuItm, "man_strat" )
    mInsertPixmapItem( manmnu_, tr("Wavelets ..."),
		       mManWvltMnuItm, "man_wvlt" )
    mInsertPixmapItem( manmnu_, uiStrings::sWells(false),
		       mManWellMnuItm, "man_wll" )
    manmnu_->insertSeparator();
}


void uiODMenuMgr::fillProcMenu()
{
    procmnu_->clear();

    csoitm_ = new uiMenu( &appl_, tr("Create Seismic Output") );

// Attributes
    uiMenu* attritm = new uiMenu( uiStrings::sAttributes(true) );
    csoitm_->insertItem( attritm );

    add2D3DMenuItem( *attritm, "seisout", tr("Single Attribute"),
		     mSeisOut2DMnuItm, mSeisOut3DMnuItm );
    if ( SI().has3D() )
    {
	attritm->insertItem(
	    new uiAction( tr("Multi Attribute ..."),
			mCB(&applMgr(),uiODApplMgr,createMultiAttribVol)) );
	attritm->insertItem(
	    new uiAction( tr("MultiCube DataStore ..."),
			mCB(&applMgr(),uiODApplMgr,createMultiCubeDS)) );
    }

    add2D3DMenuItem( *attritm, "alonghor", tr("Along Horizon"),
		     mCompAlongHor2DMnuItm, mCompAlongHor3DMnuItm );
    add2D3DMenuItem( *attritm, "betweenhors", tr("Between Horizons"),
		     mCompBetweenHor2DMnuItm, mCompBetweenHor3DMnuItm );


// 2D <-> 3D
    uiMenu* itm2d3d = new uiMenu( "2D <=> 3D" );
    csoitm_->insertItem( itm2d3d );
    if ( SI().has3D() )
    {
	mInsertItem( itm2d3d, tr("Create 2D from 3D ..."),
		     mCreate2DFrom3DMnuItm );
	mInsertItem( itm2d3d, tr("Extract 2D from 3D ..."),
		     m2DFrom3DMnuItm );
    }
#ifdef __debug__
    if ( SI().has2D() )
    {
	mInsertItem( itm2d3d, tr("Create 3D from 2D ..."), m3DFrom2DMnuItm );
    }
#endif

    if ( SI().has3D() )
    {
// Other 3D items
	csoitm_->insertItem(
	    new uiAction(tr("Angle Mute Function ..."),
			mCB(&applMgr(),uiODApplMgr,genAngleMuteFunction) ));
	csoitm_->insertItem(
	    new uiAction(tr("Bayesian Classification ..."),
			mCB(&applMgr(),uiODApplMgr,bayesClass3D), "bayes"));
	csoitm_->insertItem(
	    new uiAction(tr("From Well Logs ..."),
			mCB(&applMgr(),uiODApplMgr,createCubeFromWells) ));
    }

    add2D3DMenuItem( *csoitm_, "empty", tr("Prestack Processing"),
		     mPSProc2DMnuItm, mPSProc3DMnuItm );

    if ( SI().has3D() )
    {
// Velocity
	uiMenu* velitm = new uiMenu( tr("Velocity") );
	csoitm_->insertItem( velitm );
	velitm->insertItem(
	    new uiAction(tr("Time - Depth Conversion ..."),
			 mCB(&applMgr(),uiODApplMgr,processTime2Depth)) );
	velitm->insertItem(
	    new uiAction(tr("Velocity Conversion ..."),
			 mCB(&applMgr(),uiODApplMgr,processVelConv)) );

	csoitm_->insertItem(
	    new uiAction(tr("Volume Builder ..."),
			mCB(&applMgr(),uiODApplMgr,createVolProcOutput)) );
    }

    procmnu_->insertItem( csoitm_ );

    uiMenu* grditm = new uiMenu( &appl_, tr("Create Horizon Output") );
    add2D3DMenuItem( *grditm, "ongrid", uiStrings::sAttributes(true),
		     mCreateSurf2DMnuItm, mCreateSurf3DMnuItm );
    procmnu_->insertItem( grditm );

    mInsertItem( procmnu_, tr("(Re-)Start Batch Job ..."),
		 mStartBatchJobMnuItm );
}


void uiODMenuMgr::fillAnalMenu()
{
    analmnu_->clear();
    SurveyInfo::Pol2D survtype = SI().survDataType();
    const char* attrpm = "attributes";
    if ( survtype == SurveyInfo::Both2DAnd3D )
    {
	uiMenu* aitm = new uiMenu( &appl_, uiStrings::sAttributes(true),
				   attrpm );
	mInsertPixmapItem( aitm, uiStrings::s2D(false), mEdit2DAttrMnuItm,
			   "attributes_2d" );
	mInsertPixmapItem( aitm, uiStrings::s3D(false), mEdit3DAttrMnuItm,
			   "attributes_3d" );

	analmnu_->insertItem( aitm );
	analmnu_->insertSeparator();
    }
    else
    {
	mInsertPixmapItem( analmnu_, uiStrings::sAttributes(false),
			   mEditAttrMnuItm, attrpm)
	analmnu_->insertSeparator();
    }

    if ( survtype!=SurveyInfo::Only2D )
    {
	analmnu_->insertItem( new uiAction( tr("Volume Builder ..."),
				mCB(&applMgr(),uiODApplMgr,doVolProcCB),
				VolProc::uiChain::pixmapFileName() ) );
    }

    uiMenu* xplotmnu = new uiMenu( &appl_, tr("Cross-plot Data"),
				   "xplot");
    mInsertPixmapItem( xplotmnu, tr("Well logs vs Attributes ..."),
		       mXplotMnuItm, "xplot_wells" );
    mInsertPixmapItem( xplotmnu, tr("Attributes vs Attributes ..."),
		       mAXplotMnuItm, "xplot_attribs" );
    mInsertItem( xplotmnu, tr("Open Cross-plot ..."), mOpenXplotMnuItm );
    analmnu_->insertItem( xplotmnu );

    analwellmnu_ = new uiMenu( &appl_, uiStrings::sWells(true) );
    analwellmnu_->insertItem( new uiAction( tr("Edit Logs ..."),
	mCB(&applMgr(),uiODApplMgr,doWellLogTools), "well_props" ) );
    if (  SI().zIsTime() )
	analwellmnu_->insertItem( new uiAction( tr("Tie Well to Seismic ..."),
	mCB(&applMgr(),uiODApplMgr,tieWellToSeismic), "well_tie" ) );
    analwellmnu_->insertItem( new uiAction( tr("Rock Physics ..."),
		mCB(&applMgr(),uiODApplMgr,launchRockPhysics), "rockphys"));
    analmnu_->insertItem( analwellmnu_ );

    layermodelmnu_ = new uiMenu( &appl_, tr("Layer Modeling"),
				 "stratlayermodeling" );
    layermodelmnu_->insertItem( new uiAction( tr("Basic ..."),
	mCB(&applMgr(),uiODApplMgr,doLayerModeling), "empty") );
    analmnu_->insertItem( layermodelmnu_ );
    analmnu_->insertSeparator();
}


void uiODMenuMgr::fillSceneMenu()
{
    scenemnu_->clear();
    mInsertItem( scenemnu_, uiStrings::sNew(true), mAddSceneMnuItm );
    mInsertItem( scenemnu_, tr("New Map View"), mAddMapSceneMnuItm );

    addtimedepthsceneitm_ = new uiAction( "Dummy",
					  mCB(this,uiODMenuMgr,handleClick) );
    scenemnu_->insertItem( addtimedepthsceneitm_, mAddTmeDepthMnuItm );

    add2D3DMenuItem( *scenemnu_, "empty", tr("New [Horizon Flattened]"),
		     mAddHorFlat2DMnuItm, mAddHorFlat3DMnuItm );
    lastsceneitm_ = scenemnu_->insertSeparator();

    mInsertItem( scenemnu_, tr("Cascade"), mCascadeMnuItm );
    uiMenu* tileitm = new uiMenu( &appl_, uiStrings::sTile() );
    scenemnu_->insertItem( tileitm );

    mInsertItem( tileitm, tr("Auto"), mTileAutoMnuItm );
    mInsertItem( tileitm, uiStrings::sHorizontal(), mTileHorMnuItm );
    mInsertItem( tileitm, uiStrings::sVertical(), mTileVerMnuItm );

    mInsertItem( scenemnu_, uiStrings::sProperties(false), mScenePropMnuItm );
    scenemnu_->insertSeparator();

    updateSceneMenu();
}


void uiODMenuMgr::insertNewSceneItem( uiAction* action, int id )
{
    scenemnu_->insertAction( action, id, lastsceneitm_ );
    lastsceneitm_ = action;
}


void uiODMenuMgr::updateSceneMenu()
{
    BufferStringSet scenenms;
    int activescene = 0;
    sceneMgr().getSceneNames( scenenms, activescene );

    for ( int idx=0; idx<=scenenms.size(); idx++ )
    {
	const int id = mSceneSelMnuItm + idx;
	uiAction* itm = scenemnu_->findAction( id );

	if ( idx >= scenenms.size() )
	{
	    if ( itm )
		scenemnu_->removeItem( id );
	    continue;
	}

	if ( !itm )
	{
	    itm = new uiAction( uiStrings::sEmptyString(),
				mCB(this,uiODMenuMgr,handleClick) );
	    scenemnu_->insertItem( itm, id );
	    itm->setCheckable( true );
	}

	itm->setText( scenenms.get(idx) );
	itm->setChecked( idx==activescene );
    }

    BufferString itmtxt( "New [" );
    itmtxt += SI().zIsTime() ? "Depth]" : "Time]";
    addtimedepthsceneitm_->setText( itmtxt );
}


void uiODMenuMgr::fillViewMenu()
{
    viewmnu_->clear();
    mInsertItem( viewmnu_, tr("Work Area ..."), mWorkAreaMnuItm );
    mInsertItem( viewmnu_, tr("Z-Scale ..."), mZScaleMnuItm );
    mInsertItem( viewmnu_, tr("Viewer 2D ..."), m2DViewMnuItm );
    uiMenu* stereoitm = new uiMenu( &appl_, tr("Stereo Viewing") );
    viewmnu_->insertItem( stereoitm );

#define mInsertStereoItem(itm,txt,docheck,id) \
    itm = new uiAction( txt, mCB(this,uiODMenuMgr,handleClick) ); \
    stereoitm->insertItem( itm, id ); \
    itm->setCheckable( true ); \
    itm->setChecked( docheck );

    mInsertStereoItem( stereooffitm_, tr("Off"), true, mStereoOffMnuItm)
    mInsertStereoItem( stereoredcyanitm_, tr("Red/Cyan"), false,
			mStereoRCMnuItm )
    mInsertStereoItem( stereoquadbufitm_, tr("Quad Buffer"), false,
			mStereoQuadMnuItm )

    stereooffsetitm_ = new uiAction( tr("Stereo Offset ..."),
				mCB(this,uiODMenuMgr,handleClick) );
    stereoitm->insertItem( stereooffsetitm_, mStereoOffsetMnuItm );
    stereooffsetitm_->setEnabled( false );

    mkViewIconsMnu();
    viewmnu_->insertSeparator();

    uiMenu& toolbarsmnu = appl_.getToolbarsMenu();
    toolbarsmnu.setName("Toolbars");
    viewmnu_->insertItem( &toolbarsmnu );
}


void uiODMenuMgr::addIconMnuItems( const DirList& dl, uiMenu* iconsmnu,
				   BufferStringSet& nms )
{
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const BufferString nm( dl.get( idx ).buf() + 6 );
	if ( nm.isEmpty() || nms.isPresent(nm) )
	    continue;

	BufferString mnunm( "&" ); mnunm += nm;
	mInsertItem( iconsmnu, mnunm, mViewIconsMnuItm+nms.size() );
	nms.add( nm );
    }
}


void uiODMenuMgr::mkViewIconsMnu()
{
    DirList dlsett( GetSettingsDir(), DirList::DirsOnly, "icons.*" );
    DirList dlsite( mGetApplSetupDataDir(), DirList::DirsOnly, "icons.*" );
    DirList dlrel( mGetSWDirDataDir(), DirList::DirsOnly, "icons.*" );
    if ( dlsett.size() + dlsite.size() + dlrel.size() < 2 )
	return;

    uiMenu* iconsmnu = new uiMenu( &appl_, tr("Icons") );
    viewmnu_->insertItem( iconsmnu );
    mInsertItem( iconsmnu, tr("Default"), mViewIconsMnuItm+0 );
    BufferStringSet nms; nms.add( "Default" );
    addIconMnuItems( dlsett, iconsmnu, nms );
    addIconMnuItems( dlsite, iconsmnu, nms );
    addIconMnuItems( dlrel, iconsmnu, nms );
}


static void updateTranslateMenu( uiMenu& mnu )
{
    for ( int idx=0; idx<mnu.actions().size(); idx++ )
    {
	uiAction* itm = const_cast<uiAction*>(mnu.actions()[idx]);
	itm->setChecked( idx==TrMgr().currentLanguage() );
    }
}


void uiODMenuMgr::fillUtilMenu()
{
    settmnu_ = new uiMenu( &appl_, uiStrings::sSettings(true) );
    utilmnu_->insertItem( settmnu_ );
    if ( TrMgr().nrSupportedLanguages() > 1 )
    {
	langmnu_ = new uiMenu( &appl_, tr("Language") );
	settmnu_->insertItem( langmnu_ );
	for ( int idx=0; idx<TrMgr().nrSupportedLanguages(); idx++ )
	{
	    uiAction* itm = new uiAction( TrMgr().getLanguageUserName(idx),
					  mCB(this,uiODMenuMgr,handleClick) );
	    itm->setCheckable( true );
	    langmnu_->insertItem( itm, mLanguageMnu+idx );
	}

	updateTranslateMenu( *langmnu_ );
    }

    mInsertItem( settmnu_, tr("Look and Feel ..."), mSettLkNFlMnuItm );
    mInsertItem( settmnu_, tr("Mouse Controls ..."), mSettMouseMnuItm );
    mInsertItem( settmnu_, tr("Keyboard Shortcuts ..."), mSettShortcutsMnuItm);
    uiMenu* advmnu = new uiMenu( &appl_, uiStrings::sAdvanced() );
    mInsertItem( advmnu, tr("Personal Settings ..."), mSettGeneral );
    mInsertItem( advmnu, tr("Survey Defaults ..."), mSettSurvey );
    settmnu_->insertItem( advmnu );

    toolsmnu_ = new uiMenu( &appl_, uiStrings::sTools() );
    utilmnu_->insertItem( toolsmnu_ );

    mInsertItem( toolsmnu_, tr("Batch Programs ..."), mBatchProgMnuItm );
    mInsertItem( toolsmnu_, tr("Position Conversion ..."), mPosconvMnuItm );
    BufferString develverstr;
    GetSpecificODVersion( "devel", develverstr );
    if ( !develverstr.isEmpty() )
	mInsertItem( toolsmnu_, tr("Create Plugin Devel. Env. ..."),
		     mCrDevEnvMnuItm );

    installmnu_ = new uiMenu( &appl_, tr("Installation") );
    utilmnu_->insertItem( installmnu_ );
    FilePath installerdir( ODInst::GetInstallerDir() );
    const bool hasinstaller = File::isDirectory( installerdir.fullPath() );
    if ( hasinstaller && !__ismac__ )
    {
	const ODInst::AutoInstType ait = ODInst::getAutoInstType();
	const bool aitfixed = ODInst::autoInstTypeIsFixed();
	if ( !aitfixed || ait == ODInst::UseManager || ait == ODInst::FullAuto )
	    mInsertItem( installmnu_, tr("Update ..."),
			 mInstMgrMnuItem );
	if ( !aitfixed )
	    mInsertItem( installmnu_, tr("Auto-update Policy ..."),
			 mInstAutoUpdPolMnuItm );
	installmnu_->insertSeparator();
    }

    mInsertItem( installmnu_, tr("Connection Settings ..."),
		 mInstConnSettsMnuItm );
    mInsertItem( installmnu_, tr("Plugins ..."), mPluginsMnuItm );
    mInsertItem( installmnu_, tr("Setup Batch Processing ..."), mSetupBatchItm);

    const char* lmfnm = od_ostream::logStream().fileName();
    if ( lmfnm && *lmfnm )
	mInsertItem( utilmnu_, tr("Show Log File ..."), mShwLogFileMnuItm );
#ifdef __debug__
    const bool enabdpdump = true;
#else
    const bool enabdpdump = GetEnvVarYN( "OD_ENABLE_DATAPACK_DUMP" );
#endif
    if ( enabdpdump )
    {
	mInsertItem( toolsmnu_, tr("DataPack Dump ..."), mDumpDataPacksMnuItm);
	mInsertItem( toolsmnu_, tr("Display Memory Info ..."),
		     mDisplayMemoryMnuItm);
    }
}


void uiODMenuMgr::add2D3DToolButton( uiToolBar& tb, const char* iconnm,
				     const uiString& tt,
				     const CallBack& cb2d, const CallBack& cb3d,
				     int itmid2d, int itmid3d )
{
    if ( !SI().has2D() )
	tb.addButton( iconnm, tt, cb3d, false, itmid3d );
    else if ( !SI().has3D() )
	tb.addButton( iconnm, tt, cb2d, false, itmid2d );
    else
    {
	const int butid = tb.addButton( iconnm, tt );
	uiMenu* popmnu = new uiMenu( tb.parent(), uiStrings::sAction() );
	popmnu->insertItem( new uiAction(uiStrings::s2D(false),cb2d), itmid2d );
	popmnu->insertItem( new uiAction(uiStrings::s3D(false),cb3d), itmid3d );
	tb.setButtonMenu( butid, popmnu, uiToolButton::InstantPopup );
    }
}


void uiODMenuMgr::add2D3DMenuItem( uiMenu& menu, const char* iconnm,
				   const uiString& itmtxt,
				   const CallBack& cb2d, const CallBack& cb3d,
				   int itmid2d, int itmid3d )
{
    if ( SI().has2D() && SI().has3D() )
    {
	uiMenu* mnu = new uiMenu( itmtxt, iconnm );
	mnu->insertAction( new uiAction(uiStrings::s2D(false),cb2d), itmid2d );
	mnu->insertAction( new uiAction(uiStrings::s3D(false),cb3d), itmid3d );
	menu.addMenu( mnu );
    }
    else
    {
	uiString titledots( itmtxt );
	titledots.append( " ..." );
	if ( SI().has2D() )
	    menu.insertAction( new uiAction(titledots,cb2d,iconnm), itmid2d );
	else if ( SI().has3D() )
	    menu.insertAction( new uiAction(titledots,cb3d,iconnm), itmid3d );
    }
}


void uiODMenuMgr::add2D3DMenuItem( uiMenu& menu, const char* iconnm,
				   const uiString& itmtxt,
				   int itmid2d, int itmid3d )
{
    const CallBack cb = mCB(this,uiODMenuMgr,handleClick);
    add2D3DMenuItem( menu, iconnm, itmtxt, cb, cb, itmid2d, itmid3d );
}



#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( fnm, txt, mCB(appman,uiODApplMgr,fn), togg )

void uiODMenuMgr::fillDtectTB( uiODApplMgr* appman )
{
    mAddTB(dtecttb_,"survey",tr("Survey Setup"),false,manSurvCB);
    add2D3DToolButton( *dtecttb_, "attributes", tr("Edit Attributes"),
		       mCB(appman,uiODApplMgr,editAttr2DCB),
		       mCB(appman,uiODApplMgr,editAttr3DCB) );
    add2D3DToolButton( *dtecttb_, "seisout", tr("Create Seismic Output"),
		       mCB(appman,uiODApplMgr,seisOut2DCB),
		       mCB(appman,uiODApplMgr,seisOut3DCB) );

    if ( SI().has3D() )
    {
	mAddTB( dtecttb_,VolProc::uiChain::pixmapFileName(),
		tr("Volume Builder"),false,doVolProcCB);
    }

    mAddTB(dtecttb_,"xplot_wells",tr("Cross-plot Attribute vs Well Data"),
	   false,doWellXPlot);
    mAddTB(dtecttb_,"xplot_attribs",
	   tr("Cross-plot Attribute vs Attribute Data"),
	   false,doAttribXPlot);

    mAddTB(dtecttb_,"rockphys",tr("Create New Well Logs Using Rock Physics"),
			false,launchRockPhysics);

    dTectTBChanged.trigger();
}


#undef mAddTB
#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( fnm, txt, mCB(this,uiODMenuMgr,fn), togg )

#define mAddPopUp( nm, txt1, txt2, itm1, itm2, mnuid ) { \
    uiMenu* popmnu = new uiMenu( &appl_, nm ); \
    popmnu->insertItem( new uiAction(txt1, \
		       mCB(this,uiODMenuMgr,handleClick)), itm1 ); \
    popmnu->insertItem( new uiAction(txt2, \
		       mCB(this,uiODMenuMgr,handleClick)), itm2 ); \
    mantb_ ->setButtonMenu( mnuid, popmnu, uiToolButton::InstantPopup ); }

#define mAddPopupMnu( mnu, txt, itm ) \
    mnu->insertItem( new uiAction(txt,mCB(this,uiODMenuMgr,handleClick)), itm );

void uiODMenuMgr::fillManTB()
{
    const int seisid =
	mAddTB(mantb_,"man_seis",tr("Manage Seismic data"),false,manSeis);
    const int horid =
	mAddTB(mantb_,"man_hor",tr("Manage Horizons"),false,manHor);
    const int fltid =
	mAddTB(mantb_,"man_flt",tr("Manage Faults"),false,manFlt);
    mAddTB(mantb_,"man_wll",tr("Manage Well data"),false,manWll);
    mAddTB(mantb_,"man_picks",tr("Manage PickSets/Polygons"),false,manPick);
    mAddTB(mantb_,"man_body",tr("Manage Bodies/Regions"),false,manBody);
    mAddTB(mantb_,"man_wvlt",uiStrings::sManWav(),false,manWvlt);
    mAddTB(mantb_,"man_strat",tr("Manage Stratigraphy"),false,manStrat);

    uiMenu* seispopmnu = new uiMenu( &appl_, tr("Seismics Menu") );
    if ( SI().has2D() )
    {
	mAddPopupMnu( seispopmnu, tr("2D Seismics"), mManSeis2DMnuItm )
	mAddPopupMnu( seispopmnu, tr("2D Prestack Seismics"),
		      mManSeisPS2DMnuItm )
    }
    if ( SI().has3D() )
    {
        mAddPopupMnu( seispopmnu, tr("3D Seismics"), mManSeis3DMnuItm )
	mAddPopupMnu( seispopmnu, tr("3D Prestack Seismics"),
		      mManSeisPS3DMnuItm )
    }
    mantb_->setButtonMenu( seisid, seispopmnu, uiToolButton::InstantPopup );

    if ( SI().survDataType() != SurveyInfo::No2D )
	mAddPopUp( tr("Horizon Menu"), tr("2D Horizons"),
		   tr("3D Horizons"),
		   mManHor2DMnuItm, mManHor3DMnuItm, horid );

    mAddPopUp( tr("Fault Menu"), uiStrings::sFaults(true),tr("FaultStickSets"),
	       mManFaultMnuItm, mManFaultStickMnuItm, fltid );
}


static bool sIsPolySelect = true;

#undef mAddTB
#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( fnm, txt, mCB(scenemgr,uiODSceneMgr,fn), togg )

#define mAddMnuItm(mnu,txt,fn,fnm,idx) {\
    uiAction* itm = new uiAction( txt, mCB(this,uiODMenuMgr,fn), fnm ); \
    mnu->insertItem( itm, idx ); }

void uiODMenuMgr::fillCoinTB( uiODSceneMgr* scenemgr )
{
    actviewid_ = cointb_->addButton( "altpick", tr("Switch to View Mode"),
			mCB(this,uiODMenuMgr,toggViewMode), false );
    mAddTB(cointb_,"home",tr("To home position"),false,toHomePos);
    mAddTB(cointb_,"set_home",tr("Save Home Position"),false,saveHomePos);
    mAddTB(cointb_,"view_all",tr("View All"),false,viewAll);
    cameraid_ = mAddTB(cointb_,"perspective",
		       tr("Switch to Orthographic Camera"),
		       false,switchCameraType);

    curviewmode_ = ui3DViewer::Inl;
    bool separateviewbuttons = false;
    Settings::common().getYN( "dTect.SeparateViewButtons", separateviewbuttons);
    if ( !separateviewbuttons )
    {
	viewselectid_ = cointb_->addButton( "cube_inl",tr("View In-line"),
				mCB(this,uiODMenuMgr,handleViewClick), false );

	uiMenu* vwmnu = new uiMenu( &appl_, tr("View Menu") );
	mAddMnuItm( vwmnu, tr("View In-line"),
		    handleViewClick, "cube_inl", 0 );
	mAddMnuItm( vwmnu, tr("View Cross-line"), handleViewClick,
		    "cube_crl", 1 );
	mAddMnuItm( vwmnu, tr("View Z"), handleViewClick, "cube_z", 2 );
	mAddMnuItm( vwmnu, tr("View North"), handleViewClick, "view_N", 3 );
	mAddMnuItm( vwmnu, tr("View North Z"), handleViewClick, "view_NZ", 4);
	cointb_->setButtonMenu( viewselectid_, vwmnu );
	viewinlid_ = viewcrlid_ = viewzid_ = viewnid_ = viewnzid_ = -1;
    }
    else
    {
#define mAddVB(img,txt) cointb_->addButton( img, txt, \
			mCB(this,uiODMenuMgr,handleViewClick), false );
	viewinlid_ = mAddVB( "cube_inl", tr("View In-line") );
	viewcrlid_ = mAddVB( "cube_crl", tr("View Cross-line") );
	viewzid_ = mAddVB( "cube_z", tr("View Z") );
	viewnid_ = mAddVB( "view_N", tr("View North") );
	viewnzid_ = mAddVB( "view_NZ", tr("View North Z") );
	viewselectid_ = -1;
    }

    mAddTB( cointb_, "dir-light", tr("Set light options"), false,
	    doDirectionalLight);

    axisid_ = mAddTB(cointb_,"axis",tr("Display Orientation Axis"),
		     true,showRotAxis);
    coltabid_ = cointb_->addButton( "colorbar", tr("Display Color Bar"),
			    mCB(this,uiODMenuMgr,dispColorBar), true );
    uiMenu* colbarmnu = new uiMenu( &appl_, tr("ColorBar Menu") );
    mAddMnuItm( colbarmnu, uiStrings::sSettings(false), dispColorBar,
		"disppars", 0 );
    cointb_->setButtonMenu( coltabid_, colbarmnu );

    mAddTB(cointb_,"snapshot",uiStrings::sTakeSnapshot(),false,mkSnapshot);
    polyselectid_ = cointb_->addButton( "polygonselect",
		tr("Polygon Selection mode"),
		mCB(this,uiODMenuMgr,selectionMode), true );
    uiMenu* mnu = new uiMenu( &appl_, tr("Menu") );
    mAddMnuItm( mnu, uiStrings::sPolygon(),
		handleToolClick, "polygonselect", 0 );
    mAddMnuItm( mnu, uiStrings::sRectangle(),
		handleToolClick, "rectangleselect", 1 );
    cointb_->setButtonMenu( polyselectid_, mnu );

    removeselectionid_ = cointb_->addButton( "trashcan", tr("Remove selection"),
		    mCB(this,uiODMenuMgr,removeSelection), false );

    soloid_ = mAddTB(cointb_,"solo",tr("Display current element only"),
		     true,soloMode);
}


void uiODMenuMgr::handleViewClick( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,itm,cb)
    if ( !itm ) return;

    const int itmid = itm->getID();
    if ( itmid==viewselectid_ )
    {
	sceneMgr().setViewSelectMode( curviewmode_ );
	return;
    }

    BufferString pm( "cube_inl" );
    BufferString tt( "View In-line" );
    curviewmode_ = ui3DViewer::Inl;
    switch( itmid )
    {
	case 1: pm = "cube_crl"; tt = "View Cross-line";
		curviewmode_ = ui3DViewer::Crl; break;
	case 2: pm = "cube_z"; tt = "View Z";
		curviewmode_ = ui3DViewer::Z; break;
	case 3: pm = "view_N"; tt = "View North";
		curviewmode_ = ui3DViewer::Y; break;
	case 4: pm = "view_NZ"; tt = "View North Z";
		curviewmode_ = ui3DViewer::YZ; break;
    }

    cointb_->setIcon( viewselectid_, pm );
    cointb_->setToolTip( viewselectid_, tt );
    sceneMgr().setViewSelectMode( curviewmode_ );
}


void uiODMenuMgr::handleToolClick( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,itm,cb)
    if ( !itm ) return;

    sIsPolySelect = itm->getID()==0;
    selectionMode( 0 );
}


void uiODMenuMgr::selectionMode( CallBacker* cb )
{
    uiVisPartServer* visserv = appl_.applMgr().visServer();
    if ( cb == visserv )
    {
	const bool ison = visserv->isSelectionModeOn();
	cointb_->turnOn( polyselectid_, ison );
	cointb_->setSensitive( removeselectionid_, ison );
	sIsPolySelect = visserv->getSelectionMode()==uiVisPartServer::Polygon;
    }
    else
    {
	uiVisPartServer::SelectionMode mode = sIsPolySelect ?
			 uiVisPartServer::Polygon : uiVisPartServer::Rectangle;
	visserv->turnSelectionModeOn(
	    !inviewmode_  && cointb_->isOn(polyselectid_) );
	visserv->setSelectionMode( mode );
    }

    cointb_->setIcon( polyselectid_, sIsPolySelect ?
			"polygonselect" : "rectangleselect" );
    cointb_->setToolTip( polyselectid_,
			 sIsPolySelect ? tr("Polygon Selection Mode")
				       : tr("Rectangle Selection Mode") );
}


void uiODMenuMgr::removeSelection( CallBacker* )
{
    uiVisPartServer& visserv = *appl_.applMgr().visServer();
    visserv.removeSelection();
}


void uiODMenuMgr::dispColorBar( CallBacker* cb )
{
    uiVisPartServer& visserv = *appl_.applMgr().visServer();

    mDynamicCastGet(uiAction*,itm,cb)
    if ( itm && itm->getID()==0 )
    {
	visserv.manageSceneColorbar( sceneMgr().askSelectScene() );
	return;
    }

    visserv.displaySceneColorbar( cointb_->isOn(coltabid_) );
}


void uiODMenuMgr::setCameraPixmap( bool perspective )
{
    cointb_->setToolTip( cameraid_,
			 perspective ? tr("Switch to Orthographic Camera")
				     : tr("Switch to Perspective Camera"));
    cointb_->setIcon( cameraid_, perspective ? "perspective"
					     : "orthographic" );
}


#define mDoOp(ot,at,op) \
	applMgr().doOperation(uiODApplMgr::at,uiODApplMgr::ot,op)

void uiODMenuMgr::handleClick( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,itm,cb)
    if ( !itm ) return; // Huh?

    const int id = itm->getID();
    switch( id )
    {
    case mManSurveyMnuItm:		applMgr().manageSurvey(); break;
    case mSessSaveMnuItm:		appl_.saveSession(); break;
    case mSessRestMnuItm:		appl_.restoreSession(); break;
    case mSessAutoMnuItm:		appl_.autoSession(); break;
    case mImpAttrMnuItm:		mDoOp(Imp,Attr,0); break;
    case mImpAttrOthSurvMnuItm:		mDoOp(Imp,Attr,1); break;
    case mImpColTabMnuItm:		mDoOp(Imp,ColTab,0); break;
    case mImpSeisCBVSMnuItm:		mDoOp(Imp,Seis,0); break;
    case mImpSeisSimple3DMnuItm:	mDoOp(Imp,Seis,5); break;
    case mImpSeisSimple2DMnuItm:	mDoOp(Imp,Seis,6); break;
    case mImpSeisSimplePS3DMnuItm:	mDoOp(Imp,Seis,7); break;
    case mImpSeisSimplePS2DMnuItm:	mDoOp(Imp,Seis,8); break;
    case mImpSeisCBVSOtherSurvMnuItm:	mDoOp(Imp,Seis,9); break;
    case mExpSeisSimple3DMnuItm:	mDoOp(Exp,Seis,5); break;
    case mExpSeisSimple2DMnuItm:	mDoOp(Exp,Seis,6); break;
    case mExpSeisSimplePS3DMnuItm:	mDoOp(Exp,Seis,7); break;
    case mExpSeisSimplePS2DMnuItm:	mDoOp(Exp,Seis,8); break;
    case mImpHorAsciiMnuItm:		mDoOp(Imp,Hor,0); break;
    case mImpHorAsciiAttribMnuItm:	mDoOp(Imp,Hor,1); break;
    case mImpHor2DAsciiMnuItm:		mDoOp(Imp,Hor,2); break;
    case mImpBulkHorAsciiMnuIm:		mDoOp(Imp,Hor,3); break;
    case mExpHorAscii3DMnuItm:		mDoOp(Exp,Hor,0); break;
    case mExpHorAscii2DMnuItm:		mDoOp(Exp,Hor,1); break;
    case mExpFltAsciiMnuItm:		mDoOp(Exp,Flt,0); break;
    case mExpFltSSAsciiMnuItm:		mDoOp(Exp,Fltss,0); break;
    case mImpWellAsciiTrackMnuItm:	mDoOp(Imp,Wll,0); break;
    case mImpWellAsciiLogsMnuItm:	mDoOp(Imp,Wll,1); break;
    case mImpWellAsciiMarkersMnuItm:	mDoOp(Imp,Wll,2); break;
    case mImpWellSimpleMnuItm:		mDoOp(Imp,Wll,4); break;
    case mImpBulkWellTrackItm:		mDoOp(Imp,Wll,5); break;
    case mImpBulkWellLogsItm:		mDoOp(Imp,Wll,6); break;
    case mImpBulkWellMarkersItm:	mDoOp(Imp,Wll,7); break;
    case mImpBulkWellD2TItm:		mDoOp(Imp,Wll,8); break;
    case mImpPickAsciiMnuItm:		mDoOp(Imp,Pick,0); break;
    case mExpPickAsciiMnuItm:		mDoOp(Exp,Pick,0); break;
    case mExpWvltAsciiMnuItm:		mDoOp(Exp,Wvlt,0); break;
    case mImpWvltAsciiMnuItm:		mDoOp(Imp,Wvlt,0); break;
    case mImpFaultMnuItm:		mDoOp(Imp,Flt,0); break;
    case mImpFaultSSAscii3DMnuItm:	mDoOp(Imp,Fltss,0); break;
    case mImpFaultSSAscii2DMnuItm:	mDoOp(Imp,Fltss,1); break;
    case mImpMuteDefAsciiMnuItm:	mDoOp(Imp,MDef,0); break;
    case mExpMuteDefAsciiMnuItm:	mDoOp(Exp,MDef,0); break;
    case mImpPVDSAsciiMnuItm:		mDoOp(Imp,PVDS,0); break;
    case mImpVelocityAsciiMnuItm:	mDoOp(Imp,Vel,0); break;
    case mImpPDFAsciiMnuItm:		mDoOp(Imp,PDF,0); break;
    case mExpPDFAsciiMnuItm:		mDoOp(Exp,PDF,0); break;
    case mExpGeom2DMnuItm:		mDoOp(Exp,Geom,0); break;
    case mManColTabMnuItm:		mDoOp(Man,ColTab,0); break;
    case mManSeis3DMnuItm:		mDoOp(Man,Seis,0); break;
    case mManSeis2DMnuItm:		mDoOp(Man,Seis,1); break;
    case mManSeisPS3DMnuItm:		mDoOp(Man,Seis,2); break;
    case mManSeisPS2DMnuItm:		mDoOp(Man,Seis,3); break;
    case mManHor3DMnuItm:		mDoOp(Man,Hor,2); break;
    case mManHor2DMnuItm:		mDoOp(Man,Hor,1); break;
    case mManFaultStickMnuItm:		mDoOp(Man,Flt,1); break;
    case mManFaultMnuItm:		mDoOp(Man,Flt,2); break;
    case mManBodyMnuItm:		mDoOp(Man,Body,0); break;
    case mManPropsMnuItm:		mDoOp(Man,Props,0); break;
    case mManWellMnuItm:		mDoOp(Man,Wll,0); break;
    case mManPickMnuItm:		mDoOp(Man,Pick,0); break;
    case mManWvltMnuItm:		mDoOp(Man,Wvlt,0); break;
    case mManAttrMnuItm:		mDoOp(Man,Attr,0); break;
    case mManNLAMnuItm:			mDoOp(Man,NLA,0); break;
    case mManSessMnuItm:		mDoOp(Man,Sess,0); break;
    case mManStratMnuItm:		mDoOp(Man,Strat,0); break;
    case mManPDFMnuItm:			mDoOp(Man,PDF,0); break;
    case mManGeomItm:			mDoOp(Man,Geom,0); break;
    case mManCrossPlotItm:		mDoOp(Man,PVDS,0); break;

    case mPreLoadSeisMnuItm:	applMgr().manPreLoad(uiODApplMgr::Seis); break;
    case mPreLoadHorMnuItm:	applMgr().manPreLoad(uiODApplMgr::Hor); break;
    case mExitMnuItm:		appl_.exit(); break;
    case mEditAttrMnuItm:	applMgr().editAttribSet(); break;
    case mEdit2DAttrMnuItm:	applMgr().editAttribSet(true); break;
    case mEdit3DAttrMnuItm:	applMgr().editAttribSet(false); break;
    case mSeisOutMnuItm:	applMgr().createVol(SI().has2D(),false); break;
    case mSeisOut2DMnuItm:	applMgr().createVol(true,false); break;
    case mSeisOut3DMnuItm:	applMgr().createVol(false,false); break;
    case mPSProc2DMnuItm:	applMgr().processPreStack(true); break;
    case mPSProc3DMnuItm:	applMgr().processPreStack(false); break;
    case mCreateSurf2DMnuItm:	applMgr().createHorOutput(0,true); break;
    case mCreateSurf3DMnuItm:	applMgr().createHorOutput(0,false); break;
    case mCompAlongHor2DMnuItm:	applMgr().createHorOutput(1,true); break;
    case mCompAlongHor3DMnuItm:	applMgr().createHorOutput(1,false); break;
    case mCompBetweenHor2DMnuItm: applMgr().createHorOutput(2,true); break;
    case mCompBetweenHor3DMnuItm: applMgr().createHorOutput(2,false); break;
    case mCreate2DFrom3DMnuItm:	applMgr().create2DGrid(); break;
    case m2DFrom3DMnuItm:	applMgr().create2DFrom3D(); break;
    case m3DFrom2DMnuItm:	applMgr().create3DFrom2D(); break;
    case mStartBatchJobMnuItm:	applMgr().startBatchJob(); break;
    case mXplotMnuItm:		applMgr().doWellXPlot(); break;
    case mAXplotMnuItm:		applMgr().doAttribXPlot(); break;
    case mOpenXplotMnuItm:	applMgr().openCrossPlot(); break;
    case mAddSceneMnuItm:	sceneMgr().tile(); // leave this, or --> crash!
				sceneMgr().addScene(true); break;
    case mAddTmeDepthMnuItm:	applMgr().addTimeDepthScene(); break;
    case mAddHorFlat2DMnuItm:	applMgr().addHorFlatScene(true); break;
    case mAddHorFlat3DMnuItm:	applMgr().addHorFlatScene(false); break;
    case mCascadeMnuItm:	sceneMgr().cascade(); break;
    case mTileAutoMnuItm:	sceneMgr().tile(); break;
    case mTileHorMnuItm:	sceneMgr().tileHorizontal(); break;
    case mTileVerMnuItm:	sceneMgr().tileVertical(); break;
    case mScenePropMnuItm:	sceneMgr().setSceneProperties(); break;
    case mWorkAreaMnuItm:	applMgr().setWorkingArea(); break;
    case mZScaleMnuItm:		applMgr().setZStretch(); break;
    case m2DViewMnuItm:		applMgr().show2DViewer(); break;
    case mBatchProgMnuItm:	applMgr().batchProgs(); break;
    case mPluginsMnuItm:	applMgr().pluginMan(); break;
    case mSetupBatchItm:	applMgr().setupBatchHosts(); break;
    case mPosconvMnuItm:	applMgr().posConversion(); break;
    case mInstMgrMnuItem:	applMgr().startInstMgr(); break;
    case mInstAutoUpdPolMnuItm:	applMgr().setAutoUpdatePol(); break;
    case mCrDevEnvMnuItm:	uiCrDevEnv::crDevEnv(&appl_); break;
    case mShwLogFileMnuItm:	showLogFile(); break;
    case mSettMouseMnuItm:	sceneMgr().setKeyBindings(); break;

    case mAddMapSceneMnuItm: {
	sceneMgr().tile();
	const int sceneid = sceneMgr().addScene( true, 0, "Map View" );
	ui3DViewer* vwr = sceneMgr().get3DViewer( sceneid );
	if ( vwr ) vwr->setMapView( true );
    } break;
    case mInstConnSettsMnuItm: {
	uiProxyDlg dlg( &appl_ ); dlg.go(); } break;

    case mSettLkNFlMnuItm: {
	uiSettingsDlg dlg( &appl_ );
	if ( !dlg.go() ) return;

	if ( dlg.needsRestart() )
	    uiMSG().message(tr("Your new settings will become active\n"
			       "the next time OpendTect is started."));
	else if ( dlg.needsRenewal() )
	    uiMSG().message(tr("Your new settings will become active\n"
			       "only for newly launched objects."));
    } break;

    case mDumpDataPacksMnuItm: {
	uiFileDialog dlg( &appl_, false, "/tmp/dpacks.txt",
			  "*.txt", tr("Data pack dump") );
	if ( dlg.go() )
	{
	    od_ostream strm( dlg.fileName() );
	    if ( strm.isOK() )
		DataPackMgr::dumpDPMs( strm );
	}
    } break;
    case mDisplayMemoryMnuItm: {
	IOPar iopar;
	OD::dumpMemInfo( iopar );
	BufferString text;
	iopar.dumpPretty( text );
	uiDialog dlg( &appl_,
	     uiDialog::Setup(tr("Memory Information"),mNoDlgTitle,mNoHelpKey) );
	uiTextBrowser* browser = new uiTextBrowser( &dlg );
	browser->setText( text.buf() );
	dlg.setCancelText( 0 );
	dlg.go();
    } break;

    case mSettGeneral: {
	uiSettings dlg( &appl_, "Set Personal Settings" );
	dlg.go();
    } break;
    case mSettSurvey: {
	uiSettings dlg( &appl_, "Set Survey Default Settings",
				uiSettings::sKeySurveyDefs() );
	dlg.go();
    } break;

    case mSettShortcutsMnuItm:	applMgr().manageShortcuts(); break;

    case mStereoOffsetMnuItm:	applMgr().setStereoOffset(); break;
    case mStereoOffMnuItm:
    case mStereoRCMnuItm :
    case mStereoQuadMnuItm :
    {
	const int type = id == mStereoRCMnuItm ? 1
					: (id == mStereoQuadMnuItm ? 2 : 0 );
	sceneMgr().setStereoType( type );
	updateStereoMenu();
    } break;

    default:
    {
	if ( id>=mSceneSelMnuItm && id<=mSceneSelMnuItm +100 )
	{
	    const char* scenenm = itm->text().getFullString();
	    sceneMgr().setActiveScene( scenenm );
	    itm->setChecked( true );
	}

	if ( id>=mLanguageMnu && id<mLanguageMnu+499 )
	{
	    const int langidx = id - mLanguageMnu;
	    uiString errmsg;
	    if ( !TrMgr().setLanguage(langidx,errmsg) )
		uiMSG().error( errmsg );

	    updateTranslateMenu( *langmnu_ );
	}

	if ( id >= mViewIconsMnuItm && id < mViewIconsMnuItm+100 )
	{
	    Settings::common().set( sKeyIconSetNm,
				    itm->text().getFullString() + 1 );
	    for ( int idx=0; idx<uiToolBar::toolBars().size(); idx++ )
		uiToolBar::toolBars()[idx]->reloadIcons();
	    Settings::common().write();
	}
	if ( id > mHelpMnu )
	    helpmgr_->handle( id );

    } break;

    }
}


int uiODMenuMgr::ask2D3D( const char* txt, int res2d, int res3d, int rescncl )
{
    int res = rescncl;
    if ( !SI().has2D() )
	res = res3d;
    else if ( !SI().has3D() )
	res = res2d;
    else
    {
	const int msg = uiMSG().askGoOnAfter( txt, uiStrings::sCancel(),
				uiStrings::s2D(true), uiStrings::s3D(true) );
	res = msg == -1 ? rescncl : ( msg == 1 ? res2d : res3d );
    }

    return res;
}


void uiODMenuMgr::manHor( CallBacker* )
{
    const int opt = ask2D3D( "Manage 2D or 3D Horizons", 1, 2, 0 );
    if ( opt == 0 ) return;

    mDoOp(Man,Hor,opt);
}


void uiODMenuMgr::manSeis( CallBacker* )
{
    appl_.applMgr().seisServer()->manageSeismics( !SI().has2D() ? 0 : 2 );
}

#define mDefManCBFn(typ) \
    void uiODMenuMgr::man##typ( CallBacker* ) { mDoOp(Man,typ,0); }

mDefManCBFn(Flt)
mDefManCBFn(Wll)
mDefManCBFn(Pick)
mDefManCBFn(Body)
mDefManCBFn(Props)
mDefManCBFn(Wvlt)
mDefManCBFn(Strat)
mDefManCBFn(PDF)


void uiODMenuMgr::showLogFile()
{
    uiTextFileDlg* dlg = new uiTextFileDlg( &appl_,
				od_ostream::logStream().fileName(), true );
    dlg->setDeleteOnClose( true );
    dlg->go();
}


void uiODMenuMgr::updateDTectToolBar( CallBacker* )
{
    dtecttb_->clear();
    mantb_->clear();
    fillDtectTB( &applMgr() );
    fillManTB();
}


void uiODMenuMgr::toggViewMode( CallBacker* cb )
{
    inviewmode_ = !inviewmode_;
    cointb_->setIcon( actviewid_, inviewmode_ ? "altview" : "altpick" );
    cointb_->setToolTip( actviewid_,
			 inviewmode_ ? tr("Switch to Interact Mode")
				     : tr("Switch to View Mode") );
    if ( inviewmode_ )
	sceneMgr().viewMode( cb );
    else
	sceneMgr().actMode( cb );

    selectionMode( 0 );
}


void uiODMenuMgr::updateDTectMnus( CallBacker* )
{
    fillImportMenu();
    fillExportMenu();
    fillManMenu();

    fillSceneMenu();
    fillAnalMenu();
    fillProcMenu();
    dTectMnuChanged.trigger();
}
