/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uiodmain.cc,v 1.82 2007-09-19 16:30:50 cvsdgb Exp $
________________________________________________________________________

-*/

#include "uiodmain.h"

#include "oddatadirmanip.h"
#include "uiattribpartserv.h"
#include "uicmain.h"
#include "uicursor.h"
#include "uidockwin.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uinlapartserv.h"
#include "uiodapplmgr.h"
#include "uiodmenumgr.h"
#include "uiodscenemgr.h"
#include "uipluginsel.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"
#include "uimpepartserv.h"
#include "uipluginsel.h"
#include "uisetdatadir.h"
#include "uisurvey.h"
#include "uitoolbar.h"
#include "uisurvinfoed.h"
#include "ui2dsip.h"

#include "ctxtioobj.h"
#include "envvars.h"
#include "filegen.h"
#include "ioman.h"
#include "ioobj.h"
#include "odsessionfact.h"
#include "oddirs.h"
#include "pixmap.h"
#include "plugins.h"
#include "ptrman.h"
#include "settings.h"
#include "survinfo.h"
#include "timer.h"

#ifndef USEQT3
# include "uisplashscreen.h"
#endif

static const int cCTHeight = 200;


static uiODMain* manODMainWin( uiODMain* i )
{
    static uiODMain* theinst = 0;
    if ( i ) theinst = i;
    return theinst;
}


uiODMain* ODMainWin()
{
    return manODMainWin(0);
}


int ODMain( int argc, char** argv )
{
    PIM().setArgs( argc, argv );
    PIM().loadAuto( false );
    uiODMain* odmain = new uiODMain( *new uicMain(argc,argv) );

#ifndef USEQT3
    ioPixmap pm( mGetSetupFileName("splash.png") );
    uiSplashScreen splash( pm );
    splash.show();
    splash.showMessage( "Loading plugins ..." );
#endif

    manODMainWin( odmain );
    bool dodlg = true;
    Settings::common().getYN( uiPluginSel::sKeyDoAtStartup, dodlg );
    ObjectSet<PluginManager::Data>& pimdata = PIM().getData();
    if ( dodlg && pimdata.size() )
    {
	uiPluginSel dlg( odmain );
	if ( dlg.nrPlugins() )
	    dlg.go();
    }
    PIM().loadAuto( true );
    if ( !odmain->ensureGoodSurveySetup() )
	return 1;

#ifdef USEQT3
    odmain->initScene();
#else
    splash.showMessage( "Initializing Scene ..." );
    odmain->initScene();
    splash.finish( odmain );
#endif
    odmain->go();
    delete odmain;
    return 0;
}



uiODMain::uiODMain( uicMain& a )
    : uiMainWin(0,"OpendTect Main Window",4,true)
    , uiapp_(a)
    , failed_(true)
    , applmgr_(0)
    , menumgr_(0)
    , scenemgr_(0)
    , ctabed_(0)
    , ctabwin_(0)
    , timer_(*new Timer("Session restore timer"))
    , lastsession_(*new ODSession)
    , cursession_(0)
    , sessionSave(this)
    , sessionRestore(this)
    , applicationClosing(this)
{
    uiMSG().setMainWin( this );
    uiapp_.setTopLevel( this );
    uiSurveyInfoEditor::addInfoProvider( new ui2DSurvInfoProvider );

    if ( !ensureGoodDataDir()
      || (IOM().bad() && !ensureGoodSurveySetup()) )
	::exit( 0 );

    applmgr_ = new uiODApplMgr( *this );
    if ( buildUI() )
	failed_ = false;

    IOM().afterSurveyChange.notify( mCB(this,uiODMain,handleStartupSession) );
    timer_.tick.notify( mCB(this,uiODMain,timerCB) );
}


uiODMain::~uiODMain()
{
    if ( ODMainWin()==this )
	manODMainWin( 0 );

    delete ctabed_;
    delete ctabwin_;
    delete &lastsession_;
    delete &timer_;
}


bool uiODMain::ensureGoodDataDir()
{
    if ( !OD_isValidRootDataDir(GetBaseDataDir()) )
    {
	uiSetDataDir dlg( this );
	return dlg.go();
    }

    return true;
}


bool uiODMain::ensureGoodSurveySetup()
{
    BufferString errmsg;
    if ( !IOMan::validSurveySetup(errmsg) )
    {
	std::cerr << errmsg << std::endl;
	uiMSG().error( errmsg );
	return false;
    }
    else if ( !IOM().isReady() )
    {
	while ( !applmgr_->manageSurvey() )
	{
	    if ( uiMSG().askGoOn( "No survey selected. Do you wish to quit?" ) )
		return false;
	}
    }

    return true;
}

#define mCBarKey	"dTect.ColorBar"
#define mHVKey		"show vertical"
#define mTopKey		"show on top"

bool uiODMain::buildUI()
{
    scenemgr_ = new uiODSceneMgr( this );
    menumgr_ = new uiODMenuMgr( this );
    menumgr_->initSceneMgrDepObjs( applmgr_, scenemgr_ );

    const char* s = GetEnvVar( "DTECT_CBAR_POS" );
    bool isvert = s && (*s == 'v' || *s == 'V');
    bool isontop = s && *s
		&& (*s == 't' || *s == 'T' || *(s+1) == 't' || *(s+1) == 'T');
    if ( !s )
    {
	PtrMan<IOPar> iopar = Settings::common().subselect( mCBarKey );
	if ( !iopar ) iopar = new IOPar;

	bool insettings = false;
	insettings |= iopar->getYN( mHVKey, isvert );
	insettings |= iopar->getYN( mTopKey, isontop );

	if ( !insettings )
	{
	    iopar->setYN( mHVKey, isvert );
	    iopar->setYN( mTopKey, isontop );

	    Settings::common().mergeComp( *iopar, mCBarKey );
	    Settings::common().write();
	}
    }

    if ( isvert )
    {
	ctabwin_ = new uiDockWin( this, "Color Table" );
	ctabed_ = new uiVisColTabEd( ctabwin_, true );
	ctabed_->coltabChange.notify( mCB(applmgr_,uiODApplMgr,coltabChg) );
	ctabed_->setPrefHeight( cCTHeight );
	ctabed_->colTabGrp()->attach( hCentered );

	addDockWindow( *ctabwin_, isontop ? uiMainWin::TornOff
					 : uiMainWin::Left );
	ctabwin_->setResizeEnabled( true );
    }
    else
    {
	uiToolBar* tb = new uiToolBar( this, "Color Table" );
	ctabed_ = new uiVisColTabEd( ctabwin_, false );
	ctabed_->coltabChange.notify( mCB(applmgr_,uiODApplMgr,coltabChg) );
	tb->addObject( ctabed_->colTabGrp()->attachObj() );
    }

    return true;
}


void uiODMain::initScene()
{
    scenemgr_->initMenuMgrDepObjs();
    applMgr().visServer()->showMPEToolbar( false );
}


IOPar& uiODMain::sessionPars()
{
    return cursession_->pluginpars();
}


CtxtIOObj* uiODMain::getUserSessionIOData( bool restore )
{
    CtxtIOObj* ctio = mMkCtxtIOObj(ODSession);
    ctio->ctxt.forread = restore;
    ctio->setObj( cursessid_ );
    uiIOObjSelDlg dlg( this, *ctio );
    if ( !dlg.go() )
	{ delete ctio->ioobj; delete ctio; ctio = 0; }
    else
    { 
	delete ctio->ioobj; ctio->ioobj = dlg.ioObj()->clone(); 
        const MultiID id( ctio->ioobj ? ctio->ioobj->key() : MultiID("") );
	cursessid_ = id;
    }

    return ctio;
}


static bool hasSceneItems( uiVisPartServer* visserv )
{
    TypeSet<int> sceneids;
    visserv->getChildIds( -1, sceneids );
    if ( sceneids.isEmpty() ) return false;

    int nrchildren = 0;
    TypeSet<int> visids;
    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	visserv->getChildIds( sceneids[0], visids );
	nrchildren += visids.size();
    }

    return nrchildren>sceneids.size()*3;
}


bool uiODMain::hasSessionChanged()
{
    if ( !hasSceneItems(applMgr().visServer()) )
	return false;

    ODSession sess;
    cursession_ = &sess;
    updateSession();
    cursession_ = &lastsession_;
    return !( sess == lastsession_ );
}


#define mDelCtioRet()	{ delete ctio->ioobj; delete ctio; return; }

void uiODMain::saveSession()
{
    CtxtIOObj* ctio = getUserSessionIOData( false );
    if ( !ctio ) { delete ctio; return; }
    ODSession sess; cursession_ = &sess;
    if ( !updateSession() ) mDelCtioRet()
    BufferString bs;
    if ( !ODSessionTranslator::store(sess,ctio->ioobj,bs) )
	{ uiMSG().error( bs ); mDelCtioRet() }

    lastsession_ = sess; cursession_ = &lastsession_;
    mDelCtioRet()
}


void uiODMain::restoreSession()
{
    CtxtIOObj* ctio = getUserSessionIOData( true );
    if ( !ctio ) { delete ctio; return; }
    restoreSession( ctio->ioobj );
    mDelCtioRet()
}


class uiODMainAutoSessionDlg : public uiDialog
{
public:

uiODMainAutoSessionDlg( uiODMain* p )
    : uiDialog(p,uiDialog::Setup("Auto-load session"
				,"Set auto-load session","50.3.1"))
    , ctio_(*mMkCtxtIOObj(ODSession))
{
    bool douse = false; MultiID id;
    ODSession::getStartupData( douse, id );

    usefld_ = new uiGenInput( this, "Auto-load sessions",
	    		      BoolInpSpec(douse,"Enabled","Disabled") );
    usefld_->valuechanged.notify( mCB(this,uiODMainAutoSessionDlg,useChg) );
    doselfld_ = new uiGenInput( this, "Use one for this survey",
	    		      BoolInpSpec(id != "") );
    doselfld_->valuechanged.notify( mCB(this,uiODMainAutoSessionDlg,useChg) );
    doselfld_->attach( alignedBelow, usefld_ );

    ctio_.setObj( id ); ctio_.ctxt.forread = true;
    selgrp_ = new uiIOObjSelGrp( this, ctio_ );
    selgrp_->attach( alignedBelow, doselfld_ );
    lbl_ = new uiLabel( this, "Session to use" );
    lbl_->attach( centeredLeftOf, selgrp_ );

    loadnowfld_ = new uiGenInput( this, "Load selected session now",
	    			  BoolInpSpec(true) );
    loadnowfld_->attach( alignedBelow, selgrp_ );

    finaliseDone.notify( mCB(this,uiODMainAutoSessionDlg,useChg) );
}

~uiODMainAutoSessionDlg()
{
    delete ctio_.ioobj; delete &ctio_;
}

void useChg( CallBacker* )
{
    const bool douse = usefld_->getBoolValue();
    const bool dosel = douse ? doselfld_->getBoolValue() : false;
    doselfld_->display( douse );
    selgrp_->display( dosel );
    loadnowfld_->display( dosel );
    lbl_->display( dosel );
}


bool acceptOK( CallBacker* )
{
    const bool douse = usefld_->getBoolValue();
    const bool dosel = douse ? doselfld_->getBoolValue() : false;
    if ( !dosel )
	ctio_.setObj( 0 );
    else
    {
	selgrp_->processInput();
	if ( selgrp_->nrSel() < 1 )
	    { uiMSG().error("Please select a session"); return false; }
	if ( selgrp_->nrSel() > 0 )
	    ctio_.setObj( selgrp_->selected(0) );
    }

    const MultiID id( ctio_.ioobj ? ctio_.ioobj->key() : MultiID("") );
    ODSession::setStartupData( douse, id );

    return true;
}

    CtxtIOObj&		ctio_;

    uiGenInput*		usefld_;
    uiGenInput*		doselfld_;
    uiGenInput*		loadnowfld_;
    uiIOObjSelGrp*	selgrp_;
    uiLabel*		lbl_;

};


void uiODMain::autoSession()
{
    uiODMainAutoSessionDlg dlg( this );
    if ( dlg.go() )
    {
	if ( dlg.loadnowfld_->getBoolValue() && dlg.ctio_.ioobj )
	    handleStartupSession(0);
    }
}


void uiODMain::restoreSession( const IOObj* ioobj )
{
    ODSession sess; BufferString bs;
    if ( !ODSessionTranslator::retrieve(sess,ioobj,bs) )
	{ uiMSG().error( bs ); return; }

    cursession_ = &sess;
    doRestoreSession();
    cursession_ = &lastsession_; lastsession_.clear();
    ctabed_->updateColTabList();
    timer_.start( 200, true );
}


bool uiODMain::updateSession()
{
    cursession_->clear();
    applMgr().visServer()->fillPar( cursession_->vispars() );
    applMgr().attrServer()->fillPar( cursession_->attrpars(true), true );
    applMgr().attrServer()->fillPar( cursession_->attrpars(false), false );
    sceneMgr().getScenePars( cursession_->scenepars() );
    if ( applMgr().nlaServer()
      && !applMgr().nlaServer()->fillPar( cursession_->nlapars() ) ) 
	return false;
    applMgr().mpeServer()->fillPar( cursession_->mpepars() );

    sessionSave.trigger();
    return true;
}


void uiODMain::doRestoreSession()
{
    uiCursor::setOverride( uiCursor(uiCursor::Wait) );
    sceneMgr().cleanUp( false );
    applMgr().resetServers();

    if ( applMgr().nlaServer() )
	applMgr().nlaServer()->usePar( cursession_->nlapars() );
    if ( SI().has2D() )
	applMgr().attrServer()->usePar( cursession_->attrpars(true), true );
    if ( SI().has3D() )
	applMgr().attrServer()->usePar( cursession_->attrpars(false), false );
    applMgr().mpeServer()->usePar( cursession_->mpepars() );
    const bool visok = applMgr().visServer()->usePar( cursession_->vispars() );
    if ( visok )
	sceneMgr().useScenePars( cursession_->scenepars() );

    sessionRestore.trigger();
    if ( visok )
	applMgr().visServer()->calculateAllAttribs();
    else
    {
	uiCursor::restoreOverride();
	uiMSG().error( "An error occurred while reading session file.\n"
		       "A new scene will be launched" );	
	uiCursor::setOverride( uiCursor(uiCursor::Wait) );
	sceneMgr().cleanUp( true );
    }

    uiCursor::restoreOverride();
}


void uiODMain::handleStartupSession( CallBacker* )
{
    bool douse = false; MultiID id;
    ODSession::getStartupData( douse, id );
    if ( !douse || id == "" ) 
	return;

    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj ) return;
    cursessid_ = id;
    restoreSession( ioobj );
}


void uiODMain::timerCB( CallBacker* )
{
    sceneMgr().layoutScenes();
}


bool uiODMain::go()
{
    if ( failed_ ) return false;

    show();
    uiSurvey::updateViewsGlobal();
    Timer tm( "Handle startup session" );
    tm.tick.notify( mCB(this,uiODMain,handleStartupSession) );
    tm.start( 200, true );
    int rv = uiapp_.exec();
    delete applmgr_; applmgr_ = 0;
    return rv ? false : true;
}


bool uiODMain::askStore( bool& askedanything )
{
    if ( !applmgr_->attrServer() ) return false;

    bool doask = false;
    Settings::common().getYN( "dTect.Ask store session", doask );
    if ( doask && hasSessionChanged() )
    {
	askedanything = true;
	int res = uiMSG().askGoOnAfter( "Do you want to save this session?" );
	if ( res == 0 )
	    saveSession();
	else if ( res == 2 )
	    return false;
    }

    doask = true;
    Settings::common().getYN( "dTect.Ask store picks", doask );
    if ( doask && !applmgr_->pickSetsStored() )
    {
	askedanything = true;
	int res = uiMSG().askGoOnAfter( "Pick sets have changed.\n"
					"Store the changes now?");
	if ( res == 0 )
	    applmgr_->storePickSets();
	else if ( res == 2 )
	    return false;
    }
    if ( SI().has2D() ) askStoreAttribs( true, askedanything );
    if ( SI().has3D() ) askStoreAttribs( false, askedanything );

    return true;
}


bool uiODMain::askStoreAttribs( bool is2d, bool& askedanything )
{
    bool doask = true;
    Settings::common().getYN( "dTect.Ask store attributeset", doask );
    if ( doask && !applmgr_->attrServer()->setSaved( is2d ) )
    {
	askedanything = true;
	int res = uiMSG().askGoOnAfter( "Current attribute set has changed.\n"
					"Store the changes now?" );
	if ( res == 0 )
	    applmgr_->attrServer()->saveSet( is2d );
	else if ( res == 2 )
	    return false;
    }

    return true;
}

bool uiODMain::closeOK()
{
    applicationClosing.trigger();

    if ( failed_ ) return true;

    menumgr_->storePositions();
    scenemgr_->storePositions();
    if ( ctabwin_ )
	ctabwin_->storePosition();

    bool askedanything = false;
    if ( !askStore(askedanything) )
	return false;

    if ( !askedanything )
    {
	bool doask = false;
	Settings::common().getYN( "dTect.Ask close", doask );
	if ( doask && !uiMSG().askGoOn( "Do you want to quit?" ) )
	    return false;
    }

    removeDockWindow( ctabwin_ );
    delete scenemgr_;
    delete menumgr_;

    return true;
}


void uiODMain::exit()
{
    if ( !closeOK() ) return;

    uiapp_.exit(0);
}
