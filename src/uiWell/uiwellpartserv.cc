/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2003
 RCS:           $Id: uiwellpartserv.cc,v 1.26 2007-05-14 12:11:00 cvsraman Exp $
________________________________________________________________________

-*/


#include "uiwellpartserv.h"
#include "uiwellimpasc.h"
#include "uiwellman.h"
#include "welltransl.h"
#include "wellman.h"
#include "welldata.h"
#include "welllog.h"
#include "welltrack.h"
#include "welllogset.h"
#include "uiwellrdmlinedlg.h"
#include "multiid.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiwelldlgs.h"
#include "uilogseldlg.h"
#include "ptrman.h"
#include "color.h"


const int uiWellPartServer::evPreviewRdmLine			=0;
const int uiWellPartServer::evCreateRdmLine			=1;
const int uiWellPartServer::evCleanPreview			=2;


uiWellPartServer::uiWellPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , rdmlinedlg_(0)
{
}


uiWellPartServer::~uiWellPartServer()
{
    if ( rdmlinedlg_ )
	delete rdmlinedlg_;
}


bool uiWellPartServer::importTrack()
{
    uiWellImportAsc dlg( appserv().parent() );
    return dlg.go();
}


bool uiWellPartServer::importLogs()
{
    manageWells(); return true;
}


bool uiWellPartServer::importMarkers()
{
    manageWells(); return true;
}


bool uiWellPartServer::selectWells( ObjectSet<MultiID>& wellids )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(Well);
    ctio->ctxt.forread = true;
    uiIOObjSelDlg dlg( appserv().parent(), *ctio, 0, true );
    if ( !dlg.go() ) return false;

    deepErase( wellids );
    const int nrsel = dlg.nrSel();
    for ( int idx=0; idx<nrsel; idx++ )
	wellids += new MultiID( dlg.selected(idx) );

    return wellids.size();
}


bool uiWellPartServer::selectLogs( const MultiID& wellid, 
					Well::LogDisplayParSet*& logparset ) 
{
    Well::Data* wd = Well::MGR().get( wellid );
    if(!wd)
	return false;
    uiLogSelDlg dlg( appserv().parent(), wd->logs(), logparset );
    if( !dlg.go() )
	return false;
    logparset = dlg.getParSet();
    return true;
}


bool uiWellPartServer::hasLogs( const MultiID& wellid ) const
{
    const Well::Data* wd = Well::MGR().get( wellid );
    return wd && wd->logs().size();
}


void uiWellPartServer::manageWells()
{
    uiWellMan dlg( appserv().parent() );
    dlg.go();
}


void uiWellPartServer::selectWellCoordsForRdmLine()
{
    rdmlinedlg_ = new uiWell2RandomLineDlg( appserv().parent(), this );
    rdmlinedlg_->windowClosed.notify(mCB(this,uiWellPartServer,rdmlnDlgClosed));
    rdmlinedlg_->go();
}


void uiWellPartServer::rdmlnDlgClosed( CallBacker* )
{
    if ( rdmlinedlg_->uiResult() == 0 )
	sendEvent( evCleanPreview );
    else
	sendEvent( evCreateRdmLine );
}


void uiWellPartServer::sendPreviewEvent()
{
    sendEvent( evPreviewRdmLine );
}


void uiWellPartServer::getRdmLineCoordinates( TypeSet<Coord>& coords )
{
    rdmlinedlg_->getCoordinates( coords );
}


bool uiWellPartServer::setupNewWell( BufferString& wellname, Color& wellcolor )
{
    uiNewWellDlg dlg( appserv().parent() );
    dlg.go();
    wellname = dlg.getName();
    wellcolor = dlg.getWellColor();
    return ( dlg.uiResult() == 1 );
}


bool uiWellPartServer::storeWell( const TypeSet<Coord3>& newcoords, 
				  const BufferString& wellname, 
				  const char* errmsg )
{
    uiStoreWellDlg dlg( appserv().parent(), wellname );
    dlg.setWellCoords( newcoords );
    dlg.go();

    return true;
}
