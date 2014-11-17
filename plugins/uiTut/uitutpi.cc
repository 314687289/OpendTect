
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : NOv 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uitutorialattrib.h"
#include "uituthortools.h"
#include "uitutseistools.h"
#include "uitutwelltools.h"

#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uivismenuitemhandler.h"
#include "uivispartserv.h"
#include "viswelldisplay.h"

#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "seistype.h"
#include "survinfo.h"

#include "odplugin.h"

static const int cTutIdx = -1100;


mDefODPluginInfo(uiTut)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Tutorial plugin",
	"OpendTect",
	"dGB (Raman/Bert)",
	"3.2",
    	"Shows some simple plugin development basics."
	    "\nCan be loaded into od_main only.") );
    return &retpi;
}


class uiTutMgr :  public CallBacker
{ mODTextTranslationClass(uiTutMgr);
public:
			uiTutMgr(uiODMain*);

    uiODMain*		appl_;
    uiMenu*		mnuhor_;
    uiMenu*		mnuseis_;
    uiVisMenuItemHandler wellmnuitmhandler_;

    void		doSeis(CallBacker*);
    void		do2DSeis(CallBacker*);
    void		do3DSeis(CallBacker*);
    void		launchDialog(Seis::GeomType);
    void		doHor(CallBacker*);
    void		doWells(CallBacker*);
};


uiTutMgr::uiTutMgr( uiODMain* a )
	: appl_(a)
	, wellmnuitmhandler_(visSurvey::WellDisplay::sFactoryKeyword(),
		  	     *a->applMgr().visServer(),tr("Tut Well Tools ..."),
			     mCB(this,uiTutMgr,doWells),0,cTutIdx)
{
    uiMenu* mnu = new uiMenu( appl_, tr("Tut Tools") );
    if ( SI().has2D() && SI().has3D() ) 
    {
	mnu->insertItem( new uiAction(tr("Seismic 2D (Direct) ..."),
					mCB(this,uiTutMgr,do2DSeis)) );
	mnu->insertItem( new uiAction(tr("Seismic 3D (Direct) ..."),
					mCB(this,uiTutMgr,do3DSeis)) );
    }	
    else
	mnu->insertItem( new uiAction(tr("Seismic (Direct) ..."),
					mCB(this,uiTutMgr,doSeis)) );

    mnu->insertItem( new uiAction(uiStrings::sHorizon(false),
				    mCB(this,uiTutMgr,doHor)) );

    appl_->menuMgr().toolsMnu()->insertItem( mnu );
}


void uiTutMgr::do3DSeis( CallBacker* )
{ launchDialog( Seis::Vol ); }


void uiTutMgr::do2DSeis( CallBacker* )
{ launchDialog( Seis::Line ); }

void uiTutMgr::doSeis( CallBacker* )
{ launchDialog( SI().has2D() ? Seis::Line : Seis::Vol ); }


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

    const MultiID wellid = wd->getMultiID();
    PtrMan<IOObj> ioobj = IOM().get( wellid );
    if ( !ioobj )
    {
	uiMSG().error( tr("Cannot find well in database.\n"
		          "Perhaps it's not stored yet?") );
	return;
    }

    uiTutWellTools dlg( appl_, wellid );
    dlg.go();
}


mDefODInitPlugin(uiTut)
{
    mDefineStaticLocalObject( uiTutMgr*, mgr, = 0 );
    if ( mgr ) return 0;
    mgr = new uiTutMgr( ODMainWin() );

    uiTutorialAttrib::initClass();
    return 0;
}
