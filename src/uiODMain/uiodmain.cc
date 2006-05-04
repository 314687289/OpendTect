/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uiodmain.cc,v 1.42 2006-05-04 20:21:02 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiodmain.h"
#include "uicmain.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiodmenumgr.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"
#include "uinlapartserv.h"
#include "uiattribpartserv.h"
#include "uimpepartserv.h"
#include "uidockwin.h"
#include "uisurvey.h"
#include "uisurvinfoed.h"
#include "ui2dsip.h"
#include "uicursor.h"
#include "uiioobjsel.h"
#include "uisetdatadir.h"
#include "uipluginsel.h"
#include "uimsg.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "ctxtioobj.h"
#include "filegen.h"
#include "settings.h"
#include "plugins.h"
#include "envvars.h"
#include "odsessionfact.h"

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
    manODMainWin( odmain );
    bool dodlg = true;
    Settings::common().getYN( uiPluginSel::sKeyDoAtStartup, dodlg );
    ObjectSet<PluginManager::Data>& pimdata = PIM().getData();
    if ( dodlg && pimdata.size() )
    {
	uiPluginSel dlg( odmain );
	dlg.go();
    }
    PIM().loadAuto( true );
    if ( !odmain->ensureGoodSurveySetup() )
	return 1;

    odmain->initScene();
    odmain->go();
    delete odmain;
    return 0;
}



uiODMain::uiODMain( uicMain& a )
	: uiMainWin(0,"OpendTect Main Window",4,true)
    	, uiapp(a)
	, failed(true)
    	, menumgr(0)
    	, scenemgr(0)
    	, ctabed(0)
    	, ctabwin(0)
    	, lastsession(*new ODSession)
    	, cursession(0)
    	, sessionSave(this)
    	, sessionRestore(this)
{
    uiMSG().setMainWin( this );
    uiapp.setTopLevel( this );

    if ( !ensureGoodDataDir() )
	::exit( 0 );

    uiSurveyInfoEditor::addInfoProvider( new ui2DSurvInfoProvider );

    applmgr = new uiODApplMgr( *this );

    if ( buildUI() )
	failed = false;
}


uiODMain::~uiODMain()
{
    delete ctabed;
    delete ctabwin;
    delete &lastsession;
}


bool uiODMain::ensureGoodDataDir()
{
    if ( !uiSetDataDir::isOK() )
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
	while ( !applmgr->manageSurvey() )
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
    scenemgr = new uiODSceneMgr( this );
    menumgr = new uiODMenuMgr( this );
    menumgr->initSceneMgrDepObjs();

    const char* s = GetEnvVar( "DTECT_CBAR_POS" );
    bool isvert = !s || *s == 'v' || *s == 'V';
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

    ctabwin = new uiDockWin( this, "Color Table" );
    if ( isvert )
    {
	ctabwin->setResizeEnabled( true );
    }
    else
    {
	ctabwin->setVerStretchable( false );
	ctabwin->setHorStretchable( true );
    }

    ctabed = new uiVisColTabEd( ctabwin, isvert );
    if ( isvert )
    {
	ctabed->setPrefHeight( cCTHeight );
	ctabed->attach(hCentered);
    }

    moveDockWindow( *ctabwin, isontop ? uiMainWin::TornOff
		    	    : (isvert ? uiMainWin::Left : uiMainWin::Top), 0);

    return true;
}


void uiODMain::initScene()
{
    scenemgr->initMenuMgrDepObjs();
}


IOPar& uiODMain::sessionPars()
{
    return cursession->pluginpars();
}


CtxtIOObj* uiODMain::getUserSessionIOData( bool restore )
{
    CtxtIOObj* ctio = mMkCtxtIOObj(ODSession);
    ctio->ctxt.forread = restore;
    uiIOObjSelDlg dlg( this, *ctio );
    if ( !dlg.go() )
	{ delete ctio->ioobj; delete ctio; ctio = 0; }
    else
	{ delete ctio->ioobj; ctio->ioobj = dlg.ioObj()->clone(); }

    return ctio;
}


bool uiODMain::hasSessionChanged()
{
    ODSession sess;
    cursession = &sess;
    updateSession();
    cursession = &lastsession;
    return !( sess == lastsession );
}


#define mDelCtioRet()	{ delete ctio->ioobj; delete ctio; return; }

void uiODMain::saveSession()
{
    CtxtIOObj* ctio = getUserSessionIOData( false );
    if ( !ctio ) { delete ctio; return; }
    ODSession sess; cursession = &sess;
    if ( !updateSession() ) mDelCtioRet()
    BufferString bs;
    if ( !ODSessionTranslator::store(sess,ctio->ioobj,bs) )
	{ uiMSG().error( bs ); mDelCtioRet() }

    lastsession = sess; cursession = &lastsession;
    mDelCtioRet()
}


void uiODMain::restoreSession()
{
    CtxtIOObj* ctio = getUserSessionIOData( true );
    if ( !ctio ) { delete ctio; return; }
    ODSession sess; BufferString bs;
    if ( !ODSessionTranslator::retrieve(sess,ctio->ioobj,bs) )
	{ uiMSG().error( bs ); mDelCtioRet(); }

    cursession = &sess;
    doRestoreSession();
    cursession = &lastsession; lastsession.clear();
    mDelCtioRet()
}


bool uiODMain::updateSession()
{
    cursession->clear();
    applMgr().visServer()->fillPar( cursession->vispars() );
    applMgr().attrServer()->fillPar( cursession->attrpars() );
    sceneMgr().getScenePars( cursession->scenepars() );
    if ( applMgr().nlaServer()
      && !applMgr().nlaServer()->fillPar( cursession->nlapars() ) ) 
	return false;
    applMgr().mpeServer()->fillPar( cursession->mpepars() );

    sessionSave.trigger();
    return true;
}


void uiODMain::doRestoreSession()
{
    uiCursor::setOverride( uiCursor::Wait );
    sceneMgr().cleanUp( false );
    applMgr().resetServers();

    if ( applMgr().nlaServer() )
	applMgr().nlaServer()->usePar( cursession->nlapars() );
    applMgr().attrServer()->usePar( cursession->attrpars() );
    applMgr().mpeServer()->usePar( cursession->mpepars() );
    bool visok = applMgr().visServer()->usePar( cursession->vispars() );

    if ( visok )
    {
	sceneMgr().useScenePars( cursession->scenepars() );
	applMgr().visServer()->calculateAllAttribs();
    }
    else
    {
	uiCursor::restoreOverride();
	uiMSG().error( "An error occurred while reading session file.\n"
		       "A new scene will be launched" );	
	uiCursor::setOverride( uiCursor::Wait );
	sceneMgr().cleanUp( true );
    }
    sessionRestore.trigger();
    uiCursor::restoreOverride();
}


bool uiODMain::go()
{
    if ( failed ) return false;

    show();
    uiSurvey::updateViewsGlobal();
    int rv = uiapp.exec();
    delete applmgr; applmgr = 0;
    return rv ? false : true;
}


bool uiODMain::closeOK()
{
    if ( failed ) return true;

    menumgr->storePositions();
    scenemgr->storePositions();
    ctabwin->storePosition();

    int res = hasSessionChanged()
	    ? uiMSG().askGoOnAfter( "Do you want to save this session?" )
            : (int)!uiMSG().askGoOn( "Do you want to quit?" ) + 1;

    if ( res == 0 )
	saveSession();
    else if ( res == 2 )
	return false;

    removeDockWindow( ctabwin );
    delete scenemgr;
    delete menumgr;

    return true;
}


void uiODMain::exit()
{
    if ( !closeOK() ) return;

    uiapp.exit(0);
}
