
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: uimadpi.cc,v 1.19 2010-03-25 03:58:45 cvsranojay Exp $";

#include "uimadagascarmain.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uitoolbar.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "maddefs.h"
#include "madio.h"
#include "pixmap.h"
#include "plugins.h"
#include "separstr.h"
#include "odusgclient.h"

mExternC int GetuiMadagascarPluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC PluginInfo* GetuiMadagascarPluginInfo()
{
    static PluginInfo retpi = {
	"Madagascar link",
	"dGB (Bert)",
	"3.0",
    	"Enables the Madagascar link." };
    return &retpi;
}


bool checkEnvVars( BufferString& msg )
{
    BufferString rsfdir = GetEnvVar( "RSFROOT" );
    if ( rsfdir.isEmpty() || !File::isDirectory(rsfdir.buf()) )
    {
	msg = "RSFROOT is either not set or invalid";
	return false;
    }
    
    return true;
}


class uiMadagascarLink	: public CallBacker
			, public Usage::Client
{
public:
			uiMadagascarLink(uiODMain&);
			~uiMadagascarLink();

    uiODMenuMgr&	mnumgr;
    uiMadagascarMain*	madwin_;
    bool		ishidden_;

    void		doMain(CallBacker*);
    void		updateToolBar(CallBacker*);
    void		updateMenu(CallBacker*);
    void		survChg(CallBacker*);
    void		winHide(CallBacker*);

};


uiMadagascarLink::uiMadagascarLink( uiODMain& a )
    	: Usage::Client("Madagascar")
    	, mnumgr(a.menuMgr())
        , madwin_(0)
        , ishidden_(false)
{
    mnumgr.dTectTBChanged.notify( mCB(this,uiMadagascarLink,updateToolBar) );
    mnumgr.dTectMnuChanged.notify( mCB(this,uiMadagascarLink,updateMenu) );
    IOM().surveyToBeChanged.notify( mCB(this,uiMadagascarLink,survChg) );
    updateToolBar(0);
    updateMenu(0);
}


uiMadagascarLink::~uiMadagascarLink()
{
    delete madwin_;
}


void uiMadagascarLink::updateToolBar( CallBacker* )
{
    mnumgr.dtectTB()->addButton( "madagascar.png",
	    			 mCB(this,uiMadagascarLink,doMain),
				 "Madagascar link" );
}


void uiMadagascarLink::updateMenu( CallBacker* )
{
    delete madwin_; madwin_ = 0; ishidden_ = false;
    const ioPixmap madpm( "madagascar.png" );
    uiMenuItem* newitem = new uiMenuItem( "&Madagascar ...",
	    				  mCB(this,uiMadagascarLink,doMain),
	   				  &madpm );
    mnumgr.procMnu()->insertItem( newitem );
}


void uiMadagascarLink::survChg( CallBacker* )
{
    if ( !madwin_ ) return;

    madwin_->askSave(false);
}


void uiMadagascarLink::winHide( CallBacker* )
{
    prepUsgEnd( "Process" ); sendUsgInfo();
    ishidden_ = true;
}


void uiMadagascarLink::doMain( CallBacker* )
{
    BufferString errmsg;
    if ( !checkEnvVars(errmsg) )
    {
	uiMSG().error( errmsg );
	return;
    }

    bool needreportstart = !madwin_ || ishidden_;
    if ( !madwin_ )
    {
	madwin_ = new uiMadagascarMain( 0 );
	madwin_->windowHide.notify( mCB(this,uiMadagascarLink,winHide) );
    }

    if ( needreportstart )
	{ prepUsgStart( "Process" ); sendUsgInfo(); }

    ishidden_ = false;
    madwin_->show();
}


mExternC const char* InituiMadagascarPlugin( int, char** )
{
    static uiMadagascarLink* lnk = 0;
    if ( lnk ) return 0;

    IOMan::CustomDirData cdd( ODMad::sKeyMadSelKey(), ODMad::sKeyMadagascar(),
	    		      "Madagascar data" );
    MultiID id = IOMan::addCustomDataDir( cdd );
    if ( id != ODMad::sKeyMadSelKey() )
	return "Cannot create 'Madagascar' directory in survey";

#ifdef MAD_UIMSG_IF_FAIL
    if ( !ODMad::PI().errMsg().isEmpty() )
	uiMSG().error( ODMad::PI().errMsg() );
#endif

    lnk = new uiMadagascarLink( *ODMainWin() );
    return lnk ? 0 : ODMad::PI().errMsg().buf();
}
