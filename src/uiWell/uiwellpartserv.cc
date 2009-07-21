/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellpartserv.cc,v 1.46 2009-07-21 09:06:02 cvsbert Exp $";


#include "uiwellpartserv.h"
#include "uiwellimpasc.h"
#include "uiwellman.h"
#include "welltransl.h"
#include "wellman.h"
#include "welldata.h"
#include "welllog.h"
#include "welltrack.h"
#include "welld2tmodel.h"
#include "welldisp.h"
#include "welllogset.h"
#include "wellwriter.h"
#include "uiwellrdmlinedlg.h"
#include "uiwelldisppropdlg.h"
#include "multiid.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiwelldlgs.h"
#include "uilogselectdlg.h"
#include "ptrman.h"
#include "color.h"
#include "errh.h"
#include "survinfo.h"


const int uiWellPartServer::evPreviewRdmLine()	    { return 0; }
const int uiWellPartServer::evCleanPreview()	    { return 2; }


uiWellPartServer::uiWellPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , rdmlinedlg_(0)
    , uiwellpropdlg_(0)		    
    , cursceneid_(-1)
    , disponcreation_(false)
    , multiid_(0)
    , randLineDlgClosed(this)
    , uiwellpropDlgClosed(this)
    , isdisppropopened_(false)			       
{
}


uiWellPartServer::~uiWellPartServer()
{
    delete rdmlinedlg_;
    delete uiwellpropdlg_;
}


bool uiWellPartServer::importTrack()
{
    uiWellImportAsc dlg( parent() );
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
    uiIOObjSelDlg dlg( parent(), *ctio, 0, true );
    if ( !dlg.go() ) return false;

    deepErase( wellids );
    const int nrsel = dlg.nrSel();
    for ( int idx=0; idx<nrsel; idx++ )
	wellids += new MultiID( dlg.selected(idx) );

    return wellids.size();
}


bool uiWellPartServer::editDisplayProperties( const MultiID& mid )
{
    allapplied_ = false;
    Well::Data* wd = Well::MGR().get( mid );
    if ( !wd ) return false;
    
    if (isdisppropopened_ == false )
    {
	uiwellpropdlg_ = new uiWellDispPropDlg( parent(), *wd );
	isdisppropopened_ = true;
	uiwellpropdlg_->applyAllReq.notify( mCB(this,uiWellPartServer,applyAll) );
	uiwellpropdlg_->windowClosed.notify(mCB(this,uiWellPartServer,
		    				wellPropDlgClosed));

	bool rv = uiwellpropdlg_->go();    
    }
    return true;
}


void uiWellPartServer::wellPropDlgClosed( CallBacker* cb)
{
    mDynamicCastGet(uiWellDispPropDlg*,dlg,cb)
    if ( !dlg ) { pErrMsg("Huh"); return; }
    const Well::Data& edwd = dlg->wellData();
    const Well::DisplayProperties& edprops = edwd.displayProperties();

    if ( dlg->savedefault_ == true )
    {
	edprops.defaults() = edprops;
	saveWellDispProps( allapplied_ ? 0 : &edwd );
	edprops.commitDefaults();
    }

    isdisppropopened_ = false;
    sendEvent( evCleanPreview() );
    uiwellpropDlgClosed.trigger();
}


void uiWellPartServer::saveWellDispProps( const Well::Data* wd )
{
    ObjectSet<Well::Data>& wds = Well::MGR().wells();
    for ( int iwll=0; iwll<wds.size(); iwll++ )
    {
	Well::Data& curwd = *wds[iwll];
	if ( wd && &curwd != wd )
	   continue;
	saveWellDispProps( curwd, *Well::MGR().keys()[iwll] );
    }
}


void uiWellPartServer::saveWellDispProps( const Well::Data& wd, const MultiID& key )
{
    Well::Writer wr( Well::IO::getMainFileName(key), wd );
    if ( !wr.putDispProps() )
	uiMSG().error( "Could not write display properties for \n",
			wd.name() );
}


void uiWellPartServer::applyAll( CallBacker* cb )
{
    mDynamicCastGet(uiWellDispPropDlg*,dlg,cb)
    if ( !dlg ) { pErrMsg("Huh"); return; }
    const Well::Data& edwd = dlg->wellData();
    const Well::DisplayProperties& edprops = edwd.displayProperties();

    ObjectSet<Well::Data>& wds = Well::MGR().wells();
    for ( int iwll=0; iwll<wds.size(); iwll++ )
    {
	Well::Data& wd = *wds[iwll];
	if ( &wd != &edwd )
	{
	    wd.displayProperties() = edprops;
	    wd.dispparschanged.trigger();
	}
    }
    allapplied_ = true;
}


bool uiWellPartServer::selectLogs( const MultiID& wellid, 
					Well::LogDisplayParSet*& logparset ) 
{
    ObjectSet<Well::LogDisplayParSet> logparsets;
    logparsets += logparset;
    ObjectSet<MultiID> wellids;
    MultiID wellmultiid( wellid ); wellids += &wellmultiid;
    uiLogSelectDlg dlg( parent(), wellids, logparsets );
    return dlg.go();
}


bool uiWellPartServer::hasLogs( const MultiID& wellid ) const
{
    const Well::Data* wd = Well::MGR().get( wellid );
    return wd && wd->logs().size();
}


void uiWellPartServer::manageWells()
{
    uiWellMan dlg( parent() );
    dlg.go();
}


void uiWellPartServer::selectWellCoordsForRdmLine()
{
    delete rdmlinedlg_;
    rdmlinedlg_ = new uiWell2RandomLineDlg( parent(), this );
    rdmlinedlg_->windowClosed.notify(mCB(this,uiWellPartServer,rdmlnDlgClosed));
    rdmlinedlg_->go();
}


void uiWellPartServer::rdmlnDlgClosed( CallBacker* )
{
    multiid_ = rdmlinedlg_->getRandLineID();
    disponcreation_ = rdmlinedlg_->dispOnCreation();
    sendEvent( evCleanPreview() );
    randLineDlgClosed.trigger();
}


void uiWellPartServer::sendPreviewEvent()
{
    sendEvent( evPreviewRdmLine() );
}


void uiWellPartServer::getRdmLineCoordinates( TypeSet<Coord>& coords )
{
    rdmlinedlg_->getCoordinates( coords );
}


bool uiWellPartServer::setupNewWell( BufferString& wellname, Color& wellcolor )
{
    uiNewWellDlg dlg( parent() );
    dlg.go();
    wellname = dlg.getName();
    wellcolor = dlg.getWellColor();
    return ( dlg.uiResult() == 1 );
}

#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiWellPartServer::storeWell( const TypeSet<Coord3>& coords, 
				  const char* wellname, MultiID& mid )
{
    if ( coords.isEmpty() )
	mErrRet("Empty well track")
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(Well);
    ctio->setObj(0); ctio->setName( wellname );
    if ( !ctio->fillObj() )
	mErrRet("Cannot create an entry in the data store")
    PtrMan<Translator> tr = ctio->ioobj->getTranslator();
    mDynamicCastGet(WellTranslator*,wtr,tr.ptr())
    if ( !wtr ) mErrRet( "Please choose a different name for the well.\n"
			 "Another type object with this name already exists." );

    PtrMan<Well::Data> well = new Well::Data( wellname );
    Well::D2TModel* d2t = SI().zIsTime() ? new Well::D2TModel : 0;
    const float vel = d2t ? 3000 : 1;
    const Coord3& c0( coords[0] );
    const float minz = c0.z * vel;
    well->track().addPoint( c0, minz, minz );
    if ( d2t ) d2t->add( minz, c0.z );

    for ( int idx=1; idx<coords.size(); idx++ )
    {
	const Coord3& c( coords[idx] );
	well->track().addPoint( c, c.z*vel );
	if ( d2t ) d2t->add( well->track().dah(idx), c.z );
    }

    well->setD2TModel( d2t );
    if ( !wtr->write(*well,*ctio->ioobj) )
	mErrRet( "Cannot write well. Please check permissions." )

    mid = ctio->ioobj->key();
    return true;
}
