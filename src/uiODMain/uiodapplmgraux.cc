/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodapplmgraux.cc,v 1.22 2010-08-20 11:23:26 cvsnageswara Exp $";

#include "uiodapplmgraux.h"
#include "uiodapplmgr.h"

#include "attribdescset.h"
#include "bidvsetarrayadapter.h"
#include "ctxtioobj.h"
#include "datapointset.h"
#include "datapackbase.h"
#include "emsurfacetr.h"
#include "ioobj.h"
#include "posvecdataset.h"
#include "separstr.h"
#include "survinfo.h"
#include "timedepthconv.h"
#include "veldesc.h"

#include "uimsg.h"
#include "uiconvpos.h"
#include "uidatapointset.h"
#include "uiveldesc.h"
#include "uifontsel.h"
#include "uipluginman.h"
#include "uishortcuts.h"
#include "uibatchprogs.h"
#include "uibatchlaunch.h"
#include "uistrattreewin.h"
#include "uiprestackimpmute.h"
#include "uiprestackexpmute.h"
#include "uibatchprestackproc.h"
#include "uivelocityfunctionimp.h"
#include "uivisdatapointsetdisplaymgr.h"
#include "uiprobdenfuncman.h"
#include "uiimpexppdf.h"
#include "uiimppvds.h"
#include "uiseisbayesclass.h"

#include "uiempartserv.h"
#include "uinlapartserv.h"
#include "uiseispartserv.h"
#include "uiwellpartserv.h"
#include "uipickpartserv.h"
#include "uiattribpartserv.h"
#include "uiemattribpartserv.h"
#include "uiwellattribpartserv.h"


bool uiODApplService::eventOccurred( const uiApplPartServer* ps, int evid )
{
    return applman_.handleEvent( ps, evid );
}


void* uiODApplService::getObject( const uiApplPartServer* ps, int evid )
{
    return applman_.deliverObject( ps, evid );
}


uiParent* uiODApplService::parent() const
{
    uiParent* res = uiMainWin::activeWindow();
    if ( !res )
	res = par_;

    return res;
}


void uiODApplMgrDispatcher::survChg( bool before )
{
    if ( before && convposdlg_ )
	{ delete convposdlg_; convposdlg_ = 0; }
}


#define mCase(val) case uiODApplMgr::val

void uiODApplMgrDispatcher::doOperation( int iot, int iat, int opt )
{
    const uiODApplMgr::ObjType ot = (uiODApplMgr::ObjType)iot;
    const uiODApplMgr::ActType at = (uiODApplMgr::ActType)iat;

    switch ( ot )
    {
    mCase(Seis):
	switch ( at )
	{
	mCase(Imp):	am_.seisserv_->importSeis( opt );	break;
	mCase(Exp):	am_.seisserv_->exportSeis( opt );	break;
	mCase(Man):	am_.seisserv_->manageSeismics(opt==1);	break;
	}
    break;
    mCase(Hor):
	switch ( at )
	{
	mCase(Imp):	
	    if ( opt == 0 )
		am_.emserv_->import3DHorizon( true );
	    else if ( opt == 1 )
		am_.emserv_->import3DHorizon( false );
	    else if ( opt == 2 )
		am_.emattrserv_->import2DHorizon();
	    break;
	mCase(Exp):
	    if ( opt == 0 )
		am_.emserv_->export3DHorizon();
	    else if ( opt == 1 )
		am_.emserv_->export2DHorizon();
	    break;
	mCase(Man):
	    if ( opt == 0 )
		am_.emserv_->manageSurfaces(
				 EMAnyHorizonTranslatorGroup::keyword() );
	    else if ( opt == 1 )
		am_.emserv_->manageSurfaces(
				EMHorizon2DTranslatorGroup::keyword());
	    else if ( opt == 2 )
		am_.emserv_->manageSurfaces(
				EMHorizon3DTranslatorGroup::keyword());
	    break;
	}
    break;
    mCase(Flt):
	switch( at )
	{
	mCase(Imp):
	    if ( opt == 0 )
		am_.emserv_->importFault(
				EMFault3DTranslatorGroup::keyword() );
	    else if ( opt == 1 )
		am_.emserv_->importFault(
				EMFaultStickSetTranslatorGroup::keyword() );
	    else if ( opt == 2 )
		am_.emattrserv_->import2DFaultStickset(
				EMFaultStickSetTranslatorGroup::keyword() );
	    break;
	mCase(Exp):
	    if ( opt == 0 )
		am_.emserv_->exportFault(
				EMFault3DTranslatorGroup::keyword() );
	    else if ( opt == 1 )
		am_.emserv_->exportFault(
				EMFaultStickSetTranslatorGroup::keyword());
	    break;
	mCase(Man):
	    if ( opt == 0 ) opt = SI().has3D() ? 2 : 1;
	    if ( opt == 1 )
		am_.emserv_->manageSurfaces(
				EMFaultStickSetTranslatorGroup::keyword() );
	    else if ( opt == 2 )
		am_.emserv_->manageSurfaces(
				EMFault3DTranslatorGroup::keyword() );
	    break;
	}
    break;
    mCase(Wll):
	switch ( at )
	{
	mCase(Imp):
	    if ( opt == 0 )
		am_.wellserv_->importTrack();
	    else if ( opt == 1 )
		am_.wellserv_->importLogs();
	    else if ( opt == 2 )
		am_.wellserv_->importMarkers();
	    else if ( opt == 3 )
		am_.wellattrserv_->importSEGYVSP();

	break;
	mCase(Man):	am_.wellserv_->manageWells();	break;
	}
    break;
    mCase(Attr):
	if ( at == uiODApplMgr::Man )
	    am_.attrserv_->manageAttribSets();
    break;
    mCase(Pick):
	switch ( at )
	{
	mCase(Imp):	am_.pickserv_->impexpSet( true );	break;
	mCase(Exp):	am_.pickserv_->impexpSet( false );	break;
	mCase(Man):	am_.pickserv_->managePickSets();	break;
	}
    break;
    mCase(Wvlt):
	switch ( at )
	{
	mCase(Imp):	am_.seisserv_->importWavelets();	break;
	mCase(Exp):	am_.seisserv_->exportWavelets();	break;
	default:	am_.seisserv_->manageWavelets();	break;
	}
    break;
    mCase(MDef):
        if ( at == uiODApplMgr::Imp )
	{
	    PreStack:: uiImportMute dlgimp( par_ );
	    dlgimp.go();
	}
	else if ( at == uiODApplMgr::Exp )
	{
	    PreStack::uiExportMute dlgexp( par_ );
	    dlgexp.go();
	}
    break;
    mCase(Vel):
        if ( at == uiODApplMgr::Imp)
	{
	    Vel::uiImportVelFunc dlgvimp( par_ );
	    dlgvimp.go();
	}
    break;
    mCase(Strat):
	switch ( at )
	{
	default:	StratTWin().popUp();	break;
	}
    break;
    mCase(PDF):
        if ( at == uiODApplMgr::Imp )
	{
	    uiImpRokDocPDF dlg( par_ );
	    dlg.go();
	}
	else if ( at == uiODApplMgr::Exp )
	{
	    uiExpRokDocPDF dlg( par_ );
	    dlg.go();
	}
	else if ( at == uiODApplMgr::Man )
	{
	    uiProbDenFuncMan dlg( par_ );
	    dlg.go();
	}
    break;
    mCase(PVDS):
        if ( at == uiODApplMgr::Imp )
	{
	    uiImpPVDS dlg( par_ );
	    dlg.go();
	}
    break;
    }
}


void uiODApplMgrDispatcher::manPreLoad( int iot )
{
    const uiODApplMgr::ObjType ot = (uiODApplMgr::ObjType)iot;
    switch ( ot )
    {
	case uiODApplMgr::Seis:
	    am_.seisserv_->managePreLoad();
	break;
	case uiODApplMgr::Hor:
	    am_.emserv_->managePreLoad();
	default:
	break;
    }
}


void uiODApplMgrDispatcher::posConversion()
{
    if ( !convposdlg_ )
    {
	convposdlg_ = new uiConvertPos( par_, SI(), false );
	convposdlg_->windowClosed.notify(
				mCB(this,uiODApplMgrDispatcher,posDlgClose) );
	convposdlg_->setDeleteOnClose( true );
    }
    convposdlg_->show();
}


void uiODApplMgrDispatcher::posDlgClose( CallBacker* )
{
    convposdlg_ = 0;
}


int uiODApplMgrDispatcher::createMapDataPack( const DataPointSet& data,
						int colnr )
{
    BIDValSetArrAdapter* bvsarr = new BIDValSetArrAdapter(data.bivSet(), colnr);
    MapDataPack* newpack = new MapDataPack( "Attribute", data.name(), bvsarr );
    StepInterval<double> inlrg( bvsarr->inlrg_.start, bvsarr->inlrg_.stop, 
	    			SI().inlStep() );
    StepInterval<double> crlrg( bvsarr->crlrg_.start, bvsarr->crlrg_.stop,
	    			SI().crlStep() );
    BufferStringSet dimnames;
    dimnames.add("X").add("Y").add("In-Line").add("Cross-line");
    newpack->setProps( inlrg, crlrg, true, &dimnames );
    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );
    dpman.add( newpack );
    return newpack->id();
}


void uiODApplMgrDispatcher::openXPlot()
{
    PosVecDataSet pvds;
    pvds.setEmpty();
    DataPointSet* newdps = new DataPointSet( pvds, false );
    DPM(DataPackMgr::PointID()).addAndObtain( newdps );
    uiDataPointSet* uidps =
	new uiDataPointSet( ODMainWin(), *newdps,
			    uiDataPointSet::Setup("CrossPlot from saved data"),
			    ODMainWin()->applMgr().visDPSDispMgr() );
    uidps->go();
}


void uiODApplMgrDispatcher::processPreStack()
{ PreStack::uiBatchProcSetup dlg( par_, false ); dlg.go(); }
void uiODApplMgrDispatcher::bayesClass( bool is2d )
{ new uiSeisBayesClass( ODMainWin(), is2d ); }
void uiODApplMgrDispatcher::reStartProc()
{ uiRestartBatchDialog dlg( par_ ); dlg.go(); }
void uiODApplMgrDispatcher::batchProgs()
{ uiBatchProgLaunch dlg( par_ ); dlg.go(); }
void uiODApplMgrDispatcher::pluginMan()
{ uiPluginMan dlg( par_ ); dlg.go(); }
void uiODApplMgrDispatcher::manageShortcuts()
{ uiShortcutsDlg dlg( par_, "ODScene" ); dlg.go(); }
void uiODApplMgrDispatcher::setFonts()
{ uiSetFonts dlg( par_, "Set font types" ); dlg.go(); }
