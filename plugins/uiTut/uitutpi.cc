
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : NOv 2003
-*/


#include "uitutorialattrib.h"
#include "uituthortools.h"
#include "uitutseistools.h"
#include "uitutvolproc.h"
#include "uitutwelltools.h"

#include "uihelpview.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uiodscenemgr.h"
#include "uivismenuitemhandler.h"
#include "uivispartserv.h"
#include "viswelldisplay.h"

#include "filepath.h"
#include "dbman.h"
#include "ioobj.h"
#include "oddirs.h"
#include "ptrman.h"
#include "seistype.h"
#include "survinfo.h"

#include "odplugin.h"

#include "vistutorialdisplay.h"

static const int cTutIdx = -1100;


mDefODPluginInfo(uiTut)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Tutorial plugin",
	mODPluginTutorialsPackage,
	mODPluginCreator, mODPluginVersion,
	"Shows some simple plugin development basics."
	    "\nCan be loaded into od_main only.") );
    retpi.useronoffselectable_ = true;
    return &retpi;
}


class uiTutMgr :  public CallBacker
{ mODTextTranslationClass(uiTutMgr);
public:
			uiTutMgr(uiODMain*);
			~uiTutMgr();

    uiODMain*		appl_;
    uiMenu*		mnuhor_;
    uiMenu*		mnuseis_;
    uiVisMenuItemHandler wellmnuitmhandler_;

    void		updateMenu(CallBacker*);
    void		do2DSeis(CallBacker*);
    void		do3DSeis(CallBacker*);
    void		launchDialog(Seis::GeomType);
    void		doHor(CallBacker*);
    void		doWells(CallBacker*);
    void		doVisTutorial(CallBacker*);
};


uiTutMgr::uiTutMgr( uiODMain* a )
	: appl_(a)
	, wellmnuitmhandler_(visSurvey::WellDisplay::sFactoryKeyword(),
			 *a->applMgr().visServer(),m3Dots(tr("Tut Well Tools")),
			 mCB(this,uiTutMgr,doWells),0,cTutIdx)
{
    mAttachCB( DBM().surveyChanged, uiTutMgr::updateMenu );
    updateMenu( 0 );
}


uiTutMgr::~uiTutMgr()
{
    detachAllNotifiers();
}


void uiTutMgr::updateMenu( CallBacker* )
{
    uiODMenuMgr& mnumgr = appl_->menuMgr();
    uiMenu* mnu = mnumgr.addSubMenu( mnumgr.procMnu(), tr("Tutorial Tools"),
					"tutorial" );
    mnumgr.add2D3DActions( mnu, tr("Seismic (Direct)"), "seis",
				mCB(this,uiTutMgr,do2DSeis),
				mCB(this,uiTutMgr,do3DSeis) );
    mnumgr.addAction( mnu, uiStrings::sHorizon(), "tree-horizon3d",
		      mCB(this,uiTutMgr,doHor) );

     mnumgr.addAction( mnu, tr("Visualization"), "tree-horizon3d",
		      mCB(this,uiTutMgr,doVisTutorial) );
}

void uiTutMgr::doVisTutorial( CallBacker* )
{
    uiODSceneMgr& scenemgr = appl_->sceneMgr();
    visSurvey::TutorialDisplay* tutdisplay = new visSurvey::TutorialDisplay();
    appl_->applMgr().visServer()->addObject( tutdisplay,
					   scenemgr.getActiveSceneID(), true );

     Coord xypos =
	SI().transform( SI().sampling(false).hsamp_.center().binID() );
    Coord3 xyzpos( xypos, SI().zRange(false).center() );

    tutdisplay->displayAllWells();
    tutdisplay->displayText( tr("Tutorial: Some Text"), xyzpos);
}


void uiTutMgr::do3DSeis( CallBacker* )
{
    launchDialog( Seis::Vol );
}


void uiTutMgr::do2DSeis( CallBacker* )
{
    launchDialog( Seis::Line );
}


void uiTutMgr::launchDialog( Seis::GeomType tp )
{
    uiTutSeisTools dlg( appl_, tp );
    dlg.go();
}


void uiTutMgr::doHor( CallBacker* )
{
    uiTutHorTools dlg( appl_ );
    dlg.go();
}


void uiTutMgr::doWells( CallBacker* )
{
    const int displayid = wellmnuitmhandler_.getDisplayID();
    mDynamicCastGet(visSurvey::WellDisplay*,wd,
			appl_->applMgr().visServer()->getObject(displayid))
    if ( !wd )
	return;

    const DBKey wellid = wd->getDBKey();
    PtrMan<IOObj> ioobj = DBM().get( wellid );
    if ( !ioobj )
    {
	uiMSG().error( tr("Cannot find well in database.\n"
		          "Perhaps it's not stored yet?") );
	return;
    }

    uiTutWellTools dlg( appl_, wellid );
    dlg.go();
}


class TutHelpProvider : public SimpleHelpProvider
{
public:
TutHelpProvider( const char* baseurl, const char* linkfnm )
    : SimpleHelpProvider(baseurl,linkfnm)
{
}

static void initClass()
{
    HelpProvider::factory().addCreator( TutHelpProvider::createInstance, "tut");
}

static HelpProvider* createInstance()
{
    File::Path fp( GetDocFileDir(""), "User", "tut" );
    BufferString baseurl( "file:///" );
    baseurl.add( fp.fullPath() ).add( "/" );
    fp.add( "KeyLinkTable.txt" );
    BufferString tablefnm = fp.fullPath();
    return new TutHelpProvider( baseurl.buf(), tablefnm.buf() );
}

};

mDefODInitPlugin(uiTut)
{
    mDefineStaticLocalObject( PtrMan<uiTutMgr>, theinst_, = 0 );
    if ( theinst_ ) return 0;

    theinst_ = new uiTutMgr( ODMainWin() );
    if ( !theinst_ )
	return "Cannot instantiate Tutorial plugin";

    uiTutorialAttrib::initClass();
    TutHelpProvider::initClass();
    VolProc::uiTutOpCalculator::initClass();

    return 0;
}
