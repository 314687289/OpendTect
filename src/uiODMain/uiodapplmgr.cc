/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodapplmgr.cc,v 1.298 2009-02-11 12:19:55 cvsnanne Exp $";

#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiodmenumgr.h"
#include "uiodtreeitem.h"

#include "uiconvpos.h"
#include "uicolortable.h"
#include "mousecursor.h"
#include "uipickpartserv.h"
#include "uivispartserv.h"
#include "uimpepartserv.h"
#include "uiattribpartserv.h"
#include "uiemattribpartserv.h"
#include "uiempartserv.h"
#include "uiimphorizon2d.h"
#include "uinlapartserv.h"
#include "uiprestackimpmute.h"
#include "uiprestackexpmute.h"
#include "uiseispartserv.h"
#include "uitaskrunner.h"
#include "uivelocityfunctionimp.h"
#include "uiwellpartserv.h"
#include "uiwellattribpartserv.h"
#include "vishorizondisplay.h"
#include "vispicksetdisplay.h"
#include "vispolylinedisplay.h"
#include "visrandomtrackdisplay.h"
#include "visseis2ddisplay.h"

#include "attribdescset.h"
#include "attribdatacubes.h"
#include "attribsel.h"
#include "bidvsetarrayadapter.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "emhorizon2d.h"
#include "emseedpicker.h"
#include "emsurfacetr.h"
#include "emtracker.h"
#include "errh.h"
#include "externalattrib.h"
#include "filepath.h"
#include "flatposdata.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "linekey.h"
#include "mpeengine.h"
#include "oddirs.h"
#include "odsession.h"
#include "pickset.h"
#include "posinfo.h"
#include "posvecdataset.h"
#include "ptrman.h"
#include "seisbuf.h"
#include "survinfo.h"
#include "timedepthconv.h"

#include "uiattribcrossplot.h"
#include "uibatchlaunch.h"
#include "uibatchprogs.h"
#include "uifiledlg.h"
#include "uifontsel.h"
#include "uimsg.h"
#include "uipluginman.h"
#include "uishortcuts.h"
#include "uistereodlg.h"
#include "uistrattreewin.h"
#include "uisurvey.h"
#include "uitoolbar.h"
#include "uiviscoltabed.h"
#include "uiveldesc.h"
#include "veldesc.h"

#include <iostream>

static BufferString retstr;

class uiODApplService : public uiApplService
{
public:
			uiODApplService( uiParent* p, uiODApplMgr& am )
			: par_(p), applman_(am)	{}
    uiParent*		parent() const		{ return par_; }
    bool		eventOccurred( const uiApplPartServer* ps, int evid )
			{ return applman_.handleEvent( ps, evid ); }
    void*		getObject( const uiApplPartServer* ps, int evid )
			{ return applman_.deliverObject( ps, evid ); }

    uiODApplMgr&	applman_;
    uiParent*		par_;

};


uiODApplMgr::uiODApplMgr( uiODMain& a )
	: appl_(a)
	, applservice_(*new uiODApplService(&a,*this))
	, nlaserv_(0)
	, getOtherFormatData(this)
	, otherformatvisid_(-1)
	, otherformatattrib_(-1)
	, convertposdlg_( 0 )				
{
    pickserv_ = new uiPickPartServer( applservice_ );
    visserv_ = new uiVisPartServer( applservice_ );
    attrserv_ = new uiAttribPartServer( applservice_ );
    seisserv_ = new uiSeisPartServer( applservice_ );
    emserv_ = new uiEMPartServer( applservice_ );
    emattrserv_ = new uiEMAttribPartServer( applservice_ );
    wellserv_ = new uiWellPartServer( applservice_ );
    wellattrserv_ = new uiWellAttribPartServer( applservice_ );
    mpeserv_ = new uiMPEPartServer( applservice_ );

    IOM().surveyToBeChanged.notify( mCB(this,uiODApplMgr,surveyToBeChanged) );
    IOM().surveyChanged.notify( mCB(this,uiODApplMgr,surveyChanged) );
}


uiODApplMgr::~uiODApplMgr()
{
    IOM().surveyToBeChanged.remove( mCB(this,uiODApplMgr,surveyToBeChanged) );
    delete mpeserv_;
    delete pickserv_;
    delete nlaserv_;
    delete attrserv_;
    delete seisserv_;
    delete visserv_;

    delete convertposdlg_;
    delete emserv_;
    delete emattrserv_;
    delete wellserv_;
    delete wellattrserv_;
    delete &applservice_;
}


void uiODApplMgr::resetServers()
{
    if ( nlaserv_ ) nlaserv_->reset();
    delete attrserv_; delete mpeserv_;
    attrserv_ = new uiAttribPartServer( applservice_ );
    mpeserv_ = new uiMPEPartServer( applservice_ );
    visserv_->deleteAllObjects();
    emserv_->removeUndo();
}


int uiODApplMgr::manageSurvey()
{
    BufferString prevnm = GetDataDir();
    uiSurvey dlg( &appl_ );
    if ( !dlg.go() )
	return 0;
    else if ( prevnm == GetDataDir() )
	return 1;
    else
	return 2;
}


void uiODApplMgr::surveyToBeChanged( CallBacker* )
{
    if ( convertposdlg_ )
    {
	delete convertposdlg_;
	convertposdlg_ = 0;
    }

    bool anythingasked = false;
    appl_.askStore( anythingasked );

    if ( nlaserv_ ) nlaserv_->reset();
    delete attrserv_; attrserv_ = 0;
    delete mpeserv_; mpeserv_ = 0;
    if ( appl_.sceneMgrAvailable() )
	sceneMgr().cleanUp( false );
}


void uiODApplMgr::surveyChanged( CallBacker* )
{
    bool douse = false; MultiID id;
    ODSession::getStartupData( douse, id );
    if ( !douse || id == "" )
	sceneMgr().addScene( true );

    attrserv_ = new uiAttribPartServer( applservice_ );
    mpeserv_ = new uiMPEPartServer( applservice_ );
    MPE::engine().init();
}


bool uiODApplMgr::editNLA( bool is2d )
{
    if ( !nlaserv_ ) return false;

    nlaserv_->set2DEvent( is2d );
    bool res = nlaserv_->go();
    if ( !res ) attrserv_->setNLAName( nlaserv_->modelName() );
    return res;
}


void uiODApplMgr::doOperation( ObjType ot, ActType at, int opt )
{
    switch ( ot )
    {
    case Seis:
	switch ( at )
	{
	case Imp:	seisserv_->importSeis( opt );	break;
	case Exp:	seisserv_->exportSeis( opt );	break;
	case Man:	seisserv_->manageSeismics();	break;
	}
    break;
    case Hor:
	switch ( at )
	{
	case Imp:	
	    if ( opt == 0 )
		emserv_->import3DHorizon( true );
	    else if ( opt == 1 )
		emserv_->import3DHorizon( false );
	    else if ( opt == 2 )
		emattrserv_->import2DHorizon();
	    break;
	case Exp:
	    if ( opt == 0 )
		emserv_->export3DHorizon();
	    else if ( opt == 1 )
		emserv_->export2DHorizon();
	    break;
	case Man:
	    if ( opt == 0 )
		emserv_->manageSurfaces(
				 EMAnyHorizonTranslatorGroup::keyword() );
	    else if ( opt == 1 )
		emserv_->manageSurfaces( EMHorizon2DTranslatorGroup::keyword());
	    else if ( opt == 2 )
		emserv_->manageSurfaces( EMHorizon3DTranslatorGroup::keyword());
	    break;
	}
    break;
    case Flt:
	switch( at )
	{
	case Imp:	emserv_->importFault();	break;
	case Exp:
	    if ( opt == 0 )
		emserv_->exportFault( EMFault3DTranslatorGroup::keyword() );
	    else if ( opt == 1 )
		emserv_->exportFault(EMFaultStickSetTranslatorGroup::keyword());
	    break;
	case Man:
	    if ( opt == 0 ) opt = SI().has3D() ? 2 : 1;
	    if ( opt == 1 )
		emserv_->manageSurfaces(
				    EMFaultStickSetTranslatorGroup::keyword() );
	    else if ( opt == 2 )
		emserv_->manageSurfaces( EMFault3DTranslatorGroup::keyword() );
	    break;
	}
    break;
    case Wll:
	switch ( at )
	{
	case Imp:
	    if ( opt == 0 )
		wellserv_->importTrack();
	    else if ( opt == 1 )
		wellserv_->importLogs();
	    else if ( opt == 2 )
		wellserv_->importMarkers();
	    else if ( opt == 3 )
		wellattrserv_->importSEGYVSP();

	break;
	case Man:	wellserv_->manageWells();	break;
	}
    break;
    case Attr:
	if ( at == Man ) attrserv_->manageAttribSets();
    break;
    case Pick:
	switch ( at )
	{
	case Imp:	pickserv_->impexpSet( true );	break;
	case Exp:	pickserv_->impexpSet( false );	break;
	case Man:	pickserv_->managePickSets();	break;
	}
    break;
    case Wvlt:
	switch ( at )
	{
	case Imp:	seisserv_->importWavelets();	break;
	default:	seisserv_->manageWavelets();	break;
	}
    break;
    case MDef:
        if ( at == Imp )
	{
	    PreStack:: uiImportMute dlgimp( &appl_ );
	    dlgimp.go();
	}

	if ( at == Exp )
	{
	    PreStack::uiExportMute dlgexp( &appl_ );
	    dlgexp.go();
	}
    break;
    case Vel:
        if ( at == Imp)
	{
	    Vel::uiImportVelFunc dlgvimp( &appl_ );
	    dlgvimp.go();
	}
    break;
    case Strat:
	switch ( at )
	{
	default:	manStrat();	break;
	}
    break;
    }
}


void uiODApplMgr::manStrat()
{
    StratTWin().popUp();
}


void uiODApplMgr::manPreLoad( ObjType ot )
{
// TODO: implement (Bert?)
}


void uiODApplMgr::enableMenusAndToolBars( bool yn )
{
    sceneMgr().disabRightClick( !yn );
    visServer()->disabMenus( !yn );
    visServer()->disabToolBars( !yn );
    menuMgr().dtectTB()->setSensitive( yn );
    menuMgr().manTB()->setSensitive( yn );
    menuMgr().enableMenuBar( yn );
}


void uiODApplMgr::enableTree( bool yn )
{
    sceneMgr().disabTrees( !yn );
    visServer()->blockMouseSelection( !yn );
}


void uiODApplMgr::enableSceneManipulation( bool yn )
{
    if ( !yn ) sceneMgr().setToViewMode();
    menuMgr().enableActButton( yn );
}


void uiODApplMgr::editAttribSet()
{
    editAttribSet( SI().has2D() );
}


void uiODApplMgr::editAttribSet( bool is2d )
{
    enableMenusAndToolBars( false );
    enableSceneManipulation( false );

    attrserv_->editSet( is2d ); 
}


void uiODApplMgr::createHorOutput( int tp, bool is2d )
{
    emattrserv_->setDescSet( attrserv_->curDescSet(is2d) );
    MultiID nlaid; const NLAModel* nlamdl = 0;
    if ( nlaserv_ )
    {
	nlaserv_->set2DEvent( is2d );
	nlaid = nlaserv_->modelId();
	nlamdl = &nlaserv_->getModel();
    }
    emattrserv_->setNLA( nlamdl, nlaid );

    uiEMAttribPartServer::HorOutType type =
	  tp==0 ? uiEMAttribPartServer::OnHor :
	( tp==1 ? uiEMAttribPartServer::AroundHor : 
		  uiEMAttribPartServer::BetweenHors );
    emattrserv_->createHorizonOutput( type );
}


void uiODApplMgr::createVol( bool is2d )
{
    MultiID nlaid;
    if ( nlaserv_ )
    {
	nlaserv_->set2DEvent( is2d );
	nlaid = nlaserv_->modelId();
    }
    attrserv_->outputVol( nlaid, is2d );
}


void uiODApplMgr::doXPlot()
{
    Attrib::DescSet* ads = attrserv_->getUserPrefDescSet();
    if ( !ads ) return;

    wellattrserv_->setAttribSet( *ads );
    wellattrserv_->doXPlot();
}


void uiODApplMgr::crossPlot()
{
    Attrib::DescSet* ads = attrserv_->getUserPrefDescSet();
    if ( !ads ) return;

    attrserv_->set2DEvent( ads->is2D() );
    attrserv_->showXPlot(0);
}


void uiODApplMgr::reStartProc() {uiRestartBatchDialog dlg( &appl_ ); dlg.go();}
void uiODApplMgr::batchProgs() { uiBatchProgLaunch dlg( &appl_ ); dlg.go(); }
void uiODApplMgr::pluginMan() { uiPluginMan dlg( &appl_ ); dlg.go(); }

void uiODApplMgr::posConversion()
{
   if ( !convertposdlg_ ) 
       convertposdlg_ = new uiConvertPos( &appl_, SI(), false );

   convertposdlg_->go(); 
}

void uiODApplMgr::manageShortcuts()
{ uiShortcutsDlg dlg(&appl_,"ODScene"); dlg.go(); }


void uiODApplMgr::setFonts()
{
    uiSetFonts dlg( &appl_, "Set font types" );
    dlg.go();
}


void uiODApplMgr::setStereoOffset()
{
    ObjectSet<uiSoViewer> vwrs;
    sceneMgr().getSoViewers( vwrs );
    uiStereoDlg dlg( &appl_, vwrs );
    dlg.go();
}


void uiODApplMgr::addTimeDepthScene()
{
    CtxtIOObj ctxt( uiVelSel::ioContext() );
    ctxt.ctxt.forread = true;

    const char* txt = "Select velocity model";
    uiDialog::Setup dlgsetup( txt, txt, "base: 0.4.7" );
    uiDialog dlg( &appl_, dlgsetup );
    
    uiSeisSel::Setup selsetup( false, false );
    selsetup.seltxt("Velocity model");
    uiVelSel* velsel = new uiVelSel( &dlg, ctxt, selsetup );

    while ( dlg.go() )
    {
	if ( !velsel->ctxtIOObj().ioobj )
	{
	    uiMSG().error("No model selected");
	    continue;
	}

	VelocityDesc desc;

	if ( !desc.usePar( velsel->ctxtIOObj().ioobj->pars() ) )
	{
	    uiMSG().error("Cannot read velocity information for "
		    	  "selected model");
	    continue;
	}

	RefMan<VelocityStretcher> trans = 0;
	float zscale;
	if ( SI().zIsTime() ) 
	    // TODO: Should really depend on z-domain of model, not
	    // the survey.
	{
	    if ( desc.type_!=VelocityDesc::Interval &&
		 desc.type_!=VelocityDesc::RMS )
	    {
		uiMSG().error("Only RMS and Interval velocity are allowed for "
			      "time based models");
		continue;
	    }

	    trans = new Time2DepthStretcher();
	    zscale = SurveyInfo::defaultXYtoZScale( SurveyInfo::Meter,
		    				    SI().xyUnit() );
	}
	else
	{
	    if ( desc.type_!=VelocityDesc::Interval )
	    {
		uiMSG().error("Only Interval velocity are allowed for "
			      "depth based models");
		continue;
	    }

	    trans = new Depth2TimeStretcher();
	    zscale = SurveyInfo::defaultXYtoZScale( SurveyInfo::Second,
		    				    SI().xyUnit() );
	}

	if ( !trans->setVelData( velsel->ctxtIOObj().ioobj->key() ) || 
	     !trans->isOK() )
	{
	    uiMSG().error("Internal: Could not initialize transform");
	    continue;
	}

	BufferString scenename = "Converted with ";
	scenename += velsel->ctxtIOObj().ioobj->name();

	const int sceneid = sceneMgr().addScene(false,trans,scenename.buf());
	if ( sceneid!=-1 )
	{
	    mDynamicCastGet( visSurvey::Scene*, scene,
		    	     visserv_->getObject(sceneid) );
	    scene->setZScale( zscale );
	    sceneMgr().viewAll( 0 );
	    sceneMgr().tile();
	}

	return;
    }
}


void uiODApplMgr::setWorkingArea()
{
    if ( visserv_->setWorkingArea() )
	sceneMgr().viewAll(0);
}


void uiODApplMgr::setZStretch()
{
    visserv_->setZStretch();
}


bool uiODApplMgr::selectAttrib( int id, int attrib )
{
    if ( appl_.isRestoringSession() ) return false;

    if ( id < 0 ) return false;
    const Attrib::SelSpec* as = visserv_->getSelSpec( id, attrib );
    if ( !as ) return false;

    const char* key = visserv_->getZDomainKey( visserv_->getSceneID(id) );
    Attrib::SelSpec myas( *as );
    const bool selok = attrserv_->selectAttrib( myas, key, myas.is2D() );
    if ( selok )
	visserv_->setSelSpec( id, attrib, myas );

    return selok;
}


void uiODApplMgr::selectWells( ObjectSet<MultiID>& wellids )
{ wellserv_->selectWells( wellids ); }

bool uiODApplMgr::storePickSets()
{ return pickserv_->storeSets(); }

bool uiODApplMgr::storePickSet( const Pick::Set& ps )
{ return pickserv_->storeSet( ps ); }

bool uiODApplMgr::storePickSetAs( const Pick::Set& ps )
{ return pickserv_->storeSetAs( ps ); }

bool uiODApplMgr::setPickSetDirs( Pick::Set& ps )
{ return attrserv_->setPickSetDirs(ps, nlaserv_ ? &nlaserv_->getModel():0);}

bool uiODApplMgr::pickSetsStored() const
{ return pickserv_->pickSetsStored(); }


// TODO: Remove in v3.5
static bool useOldDefColTab( const IOPar& par, ColTab::MapperSetup& ms,
			     ColTab::Sequence& seq )
{
    BufferString seqnm;
    bool autoscale = false;
    float symmidval = mUdf(float);
    float cliprate = mUdf(float);
    Interval<float> coltabrange;
    if ( !par.get("ColorSeq Name",seqnm) || seqnm.isEmpty() )
	return false;
    seq = ColTab::Sequence( seqnm.buf() );
    par.getYN( "Auto scale", autoscale );
    ms.type_ = autoscale ? ColTab::MapperSetup::Auto
			 : ColTab::MapperSetup::Fixed;
    if ( autoscale )
    {
	if ( par.get("Cliprate",cliprate) )
	    ms.cliprate_ = cliprate;
	if ( par.get("Symmetry Midvalue",symmidval) )
	    ms.symmidval_ = symmidval;
    }
    else if ( par.get("Scale Factor",coltabrange) )
    {
	ms.start_ = coltabrange.start;
	ms.width_ = coltabrange.width();
    }

    return true;
}


void uiODApplMgr::useDefColTab( int visid, int attrib )
{
    if ( appl_.isRestoringSession() ) return;

    const Attrib::SelSpec* as = visserv_->getSelSpec( visid, attrib );
    if ( !as ) return;

    PtrMan<IOObj> ioobj = attrserv_->getIOObj( *as );
    if ( !ioobj ) return;

    ColTab::MapperSetup mapper;
    ColTab::Sequence seq( 0 );
    FilePath fp( ioobj->fullUserExpr(true) );
    fp.setExtension( "par" );
    BufferString fnm = fp.fullPath();
    IOPar iop;
    if ( iop.read(fnm,sKey::Pars) )
    {
	if ( !useOldDefColTab(iop,mapper,seq) )
	{
	    const char* ctname = iop.find( sKey::Name );
	    seq = ColTab::Sequence( ctname );
	    mapper.usePar( iop );
	}
    }

    visserv_->setColTabMapperSetup( visid, attrib, mapper );
    visserv_->setColTabSequence( visid, attrib, seq );
    appl_.colTabEd().colTab()->setMapperSetup( &mapper );
    appl_.colTabEd().colTab()->setSequence( &seq, true );
    modifyColorTable( visid, attrib );
}


void uiODApplMgr::saveDefColTab( int visid, int attrib )
{
    const Attrib::SelSpec* as = visserv_->getSelSpec(visid,attrib);
    IOObj* ioobj = attrserv_->getIOObj( *as );
    if ( !ioobj ) return;

    FilePath fp( ioobj->fullUserExpr(true) );
    fp.setExtension( "par" );
    BufferString fnm = fp.fullPath();
    IOPar iop;
    const ColTab::Sequence& ctseq = *visserv_->getColTabSequence(
	    visid, attrib );
    const ColTab::MapperSetup& mapper = *visserv_->getColTabMapperSetup(
	    visid, attrib );
    iop.set( sKey::Name, ctseq.name() );
    mapper.fillPar( iop );
    iop.write( fnm, sKey::Pars );
    delete ioobj;
}


bool uiODApplMgr::getNewData( int visid, int attrib )
{
    if ( visid<0 ) return false;

    const Attrib::SelSpec* as = visserv_->getSelSpec( visid, attrib );
    if ( !as )
    {
	uiMSG().error( "Cannot calculate attribute on this object" );
	return false;
    }

    Attrib::SelSpec myas( *as );
    if ( myas.id()!=Attrib::DescID::undef() )
	attrserv_->updateSelSpec( myas );
    if ( myas.id()<-1 )
    {
	uiMSG().error( "Cannot find selected attribute" );
	return false;
    } 

    bool res = false;
    switch ( visserv_->getAttributeFormat(visid) )
    {
	case uiVisPartServer::Cube :
	{
	    const DataPack::ID cacheid =
				visserv_->getDataPackID( visid, attrib );
	    if ( cacheid == DataPack::cNoID() )
		useDefColTab( visid, attrib );

	    CubeSampling cs = visserv_->getCubeSampling( visid, attrib );
	    if ( !cs.isDefined() )
		return false;

	    if ( myas.id()==Attrib::SelSpec::cOtherAttrib() )
	    {
		MouseCursorChanger cursorchgr( MouseCursor::Wait );
		PtrMan<Attrib::ExtAttribCalc> calc = 
			    Attrib::ExtAttrFact().create( 0, myas, false );

		if ( !calc )
		{
		    BufferString errstr = "Selected attribute '";
		    errstr += myas.userRef();
		    errstr += "' is not present in the set\n";
		    errstr += "and cannot be created";
		    uiMSG().error( errstr );
		    return false;
		}

		uiTaskRunner progm( &appl_ );
		const DataPack::ID dpid =
		    calc->createAttrib( cs, cacheid, &progm );
		if ( dpid==DataPack::cNoID() && !calc->errmsg_.isEmpty() )
		{
		    uiMSG().error( calc->errmsg_ );
		    return false;
		}

		res = dpid != DataPack::cNoID();
		visserv_->setDataPackID( visid, attrib, dpid );
		break;
	    }

	    attrserv_->setTargetSelSpec( myas );
	    const DataPack::ID newid = attrserv_->createOutput( cs, cacheid );
	    if ( newid == DataPack::cNoID() )
	    {
		visserv_->setCubeData( visid, attrib, 0 );
		return false;
	    }

	    visserv_->setDataPackID( visid, attrib, newid );

	    res = true;
	    break;
	}
	case uiVisPartServer::Traces :
	{
	    const Interval<float> zrg = visserv_->getDataTraceRange( visid );
	    TypeSet<BinID> bids;
	    visserv_->getDataTraceBids( visid, bids );
	    attrserv_->setTargetSelSpec( myas );
	    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rdmtdisp,
		    	    visserv_->getObject(visid) );
	    DataPack::ID cacheid = rdmtdisp->getDataPackID( attrib );
	    if ( cacheid==-1 )
		useDefColTab( visid, attrib );
	    TypeSet<BinID>* trcspath = rdmtdisp ? rdmtdisp->getPath() : 0;
	    const DataPack::ID newid = 
		    attrserv_->createRdmTrcsOutput( zrg, trcspath );
	    res = true;
	    if ( newid == -1 )
		res = false;
	    visserv_->setDataPackID( visid, attrib, newid );
	    return res;;
	}
	case uiVisPartServer::RandomPos :
	{
	    if ( myas.id()==Attrib::SelSpec::cOtherAttrib() )
	    {
		const MultiID surfmid = visserv_->getMultiID(visid);
		const EM::ObjectID emid = emserv_->getObjectID(surfmid);
		const int auxdatanr = emserv_->loadAuxData(emid,myas.userRef());
		bool allok = true;
		if ( auxdatanr<0 )
		    uiMSG().error( "Cannot find stored data" );
		else
		{
		    TypeSet<DataPointSet::DataRow> pts;
		    BufferStringSet nms;
		    DataPointSet vals( pts, nms, false, true );
		    emserv_->getAuxData( emid, auxdatanr, vals );
		    createAndSetMapDataPack( visid, attrib, vals, 1 );
		}

		return  auxdatanr>=0;
	    }

	    TypeSet<DataPointSet::DataRow> pts;
	    BufferStringSet nms;
	    DataPointSet data( pts, nms, false, true );
	    visserv_->getRandomPos( visid, data );
	    const int firstcol = data.nrCols();
	    data.dataSet().add( new DataColDef(myas.userRef()) );
	    attrserv_->setTargetSelSpec( myas );
	    if ( !attrserv_->createOutput(data,firstcol) )
		return false;

	    //Use the first value stored in the set, what else? (0 stands for Z)
	    createAndSetMapDataPack( visid, attrib, data, 1 );
	    return true;
	}
	case uiVisPartServer::OtherFormat :
	{
	    otherformatvisid_ = visid;
	    otherformatattrib_ = attrib;
	    getOtherFormatData.trigger();
	    otherformatvisid_ = -1;
	    otherformatattrib_ = -1;
	    return true;
	}
	default :
	{
	    pErrMsg("Invalid format");
	    return false;
	}
    }

    modifyColorTable( visid, attrib );
    return res;
}


bool uiODApplMgr::evaluateAttribute( int visid, int attrib )
{
    /* Perhaps better to merge this with uiODApplMgr::getNewData(), 
       for now it works */
    uiVisPartServer::AttribFormat format = visserv_->getAttributeFormat( visid);
    if ( format == uiVisPartServer::Cube )
    {
	const CubeSampling cs = visserv_->getCubeSampling( visid );
	DataPack::ID packid  = attrserv_->createOutput( cs, DataPack::cNoID() );
	visserv_->setDataPackID( visid, attrib, packid );
    }
    else if ( format==uiVisPartServer::Traces )
    {
	const Interval<float> zrg = visserv_->getDataTraceRange( visid );
	TypeSet<BinID> bids;
	visserv_->getDataTraceBids( visid, bids );
	mDynamicCastGet(visSurvey::RandomTrackDisplay*,rdmtdisp,
			visserv_->getObject(visid) );
	TypeSet<BinID>* trcspath = rdmtdisp ? rdmtdisp->getPath() : 0;
	const DataPack::ID dpid =
	    	attrserv_->createRdmTrcsOutput( zrg, trcspath );
	visserv_->setDataPackID( visid, attrib, dpid );
    }
    else if ( format==uiVisPartServer::RandomPos )
    {
	TypeSet<DataPointSet::DataRow> pts;
	BufferStringSet nms;
	DataPointSet data( pts, nms, false, true );
	visserv_->getRandomPos( visid, data );
	attrserv_->createOutput( data );
	visserv_->setRandomPosData( visid, attrib, &data );
    }
    else
    {
	uiMSG().error( "Cannot evaluate attributes on this object" );
	return false;
    }

    return true;
}


bool uiODApplMgr::evaluate2DAttribute( int visid, int attrib )
{
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
	    	    visserv_->getObject(visid))
    if ( !s2d ) return false;

    CubeSampling cs;
    cs.hrg.start.inl = cs.hrg.stop.inl = 0;
    cs.hrg.start.crl = s2d->getTraceNrRange().start;
    cs.hrg.stop.crl = s2d->getTraceNrRange().stop;
    cs.zrg.setFrom( s2d->getZRange(false) );

    LineKey lk( s2d->name() );
    DataPack::ID dpid = attrserv_->create2DOutput( cs, lk );
    if ( dpid < 0 )
	return false;

    s2d->setDataPackID( attrib, dpid );
    return true;
}


bool uiODApplMgr::handleEvent( const uiApplPartServer* aps, int evid )
{
    if ( !aps ) return true;

    if ( aps == pickserv_ )
	return handlePickServEv(evid);
    else if ( aps == visserv_ )
	return handleVisServEv(evid);
    else if ( aps == nlaserv_ )
	return handleNLAServEv(evid);
    else if ( aps == attrserv_ )
	return handleAttribServEv(evid);
    else if ( aps == emserv_ )
	return handleEMServEv(evid);
    else if ( aps == wellserv_ )
	return handleWellServEv(evid);
    else if ( aps == wellattrserv_ )
	return handleWellAttribServEv(evid);
    else if ( aps == mpeserv_ )
	return handleMPEServEv(evid);

    return false;
}


void* uiODApplMgr::deliverObject( const uiApplPartServer* aps, int id )
{
    if ( aps == attrserv_ )
    {
	bool isnlamod2d = id == uiAttribPartServer::objNLAModel2D();
	bool isnlamod3d = id == uiAttribPartServer::objNLAModel3D();
	if ( isnlamod2d || isnlamod3d  )
	{
	    if ( nlaserv_ )
		nlaserv_->set2DEvent( isnlamod2d );
	    return nlaserv_ ? (void*)(&nlaserv_->getModel()) : 0;
	}
    }
    else
	pErrMsg("deliverObject for unsupported part server");

    return 0;
}


bool uiODApplMgr::handleMPEServEv( int evid )
{
    if ( evid == uiMPEPartServer::evAddTreeObject() )
    {
	const int trackerid = mpeserv_->activeTrackerID();
	const EM::ObjectID emid = mpeserv_->getEMObjectID(trackerid);
	const int sceneid = mpeserv_->getCurSceneID();
	const int sdid = sceneMgr().addEMItem( emid, sceneid );
	if ( sdid==-1 )
	    return false;

	sceneMgr().updateTrees();
	return true;
    }
    else if ( evid == uiMPEPartServer::evRemoveTreeObject() )
    {
	const int trackerid = mpeserv_->activeTrackerID();
	const EM::ObjectID emid = mpeserv_->getEMObjectID(trackerid);
	const MultiID mid = emserv_->getStorageID(emid);

	TypeSet<int> sceneids;
	visserv_->getChildIds( -1, sceneids );

	TypeSet<int> ids;
	visserv_->findObject( mid, ids );

	for ( int idx=0; idx<ids.size(); idx++ )
	{
	    for ( int idy=0; idy<sceneids.size(); idy++ )
		visserv_->removeObject( ids[idx], sceneids[idy] );
	    sceneMgr().removeTreeItem(ids[idx] );
	}


	sceneMgr().updateTrees();
	return true;
    }
    else if ( evid == uiMPEPartServer::evStartSeedPick() )
    {
	//Turn off everything

	visserv_->turnSeedPickingOn( true );
	sceneMgr().setToViewMode( false );
    }
    else if ( evid==uiMPEPartServer::evEndSeedPick() )
    {
	visserv_->turnSeedPickingOn( false );
    }
    else if ( evid==uiMPEPartServer::evWizardClosed() )
    {
	enableMenusAndToolBars( true );
	enableTree( true );
	visserv_->reportMPEWizardActive( false );
    }
    else if ( evid==uiMPEPartServer::evGetAttribData() )
    {
	const Attrib::SelSpec* as = mpeserv_->getAttribSelSpec();
	if ( !as ) return false;
	const CubeSampling cs = mpeserv_->getAttribVolume(*as);
	const Attrib::DataCubes* cache = mpeserv_->getAttribCache(*as);
	attrserv_->setTargetSelSpec( *as );
	RefMan<const Attrib::DataCubes> newdata =
	    				attrserv_->createOutput( cs, cache );
	mpeserv_->setAttribData(*as, newdata );
    }
    else if ( evid==uiMPEPartServer::evCreate2DSelSpec() )
    {
	LineKey lk( mpeserv_->get2DLineSet(), mpeserv_->get2DAttribName() );
	const Attrib::DescID attribid = attrServer()->getStoredID( lk, true );
	if ( attribid<0 ) return false;

	const Attrib::SelSpec as( mpeserv_->get2DAttribName(), attribid );
	mpeserv_->set2DSelSpec( as );
    }
    else if ( evid==uiMPEPartServer::evShowToolbar() )
	visserv_->showMPEToolbar();
    else if ( evid==uiMPEPartServer::evMPEDispIntro() )
	visserv_->introduceMPEDisplay();
    else if ( evid==uiMPEPartServer::evInitFromSession() )
	visserv_->initMPEStuff();
    else if ( evid==uiMPEPartServer::evUpdateTrees() )
	sceneMgr().updateTrees();
    else if ( evid==uiMPEPartServer::evUpdateSeedConMode() )
	visserv_->updateSeedConnectMode();
    else
	pErrMsg("Unknown event from mpeserv");

    return true;
}


bool uiODApplMgr::handleWellServEv( int evid )
{
    if ( evid == uiWellPartServer::evPreviewRdmLine() )
    {
	TypeSet<Coord> coords;
	wellserv_->getRdmLineCoordinates( coords );
	setupRdmLinePreview( coords );
	//enableTree( false );
	//enableMenusAndToolBars( false );
    }
    if ( evid == uiWellPartServer::evCleanPreview() )
    {
	cleanPreview();
	enableTree( true );
	enableMenusAndToolBars( true );
    }
    
    return true;
}


bool uiODApplMgr::handleWellAttribServEv( int evid )
{
    if ( evid == uiWellAttribPartServer::evShowPickSet() )
	pickserv_->setPickSet( wellattrserv_->getSelPickSet() );
    return true;
}


bool uiODApplMgr::handleEMServEv( int evid )
{
    if ( evid == uiEMPartServer::evDisplayHorizon() )
    {
	TypeSet<int> sceneids;
	visserv_->getChildIds( -1, sceneids );
	if ( sceneids.isEmpty() ) return false;

	const EM::ObjectID emid = emserv_->selEMID();
	sceneMgr().addEMItem( emid, sceneids[0] );
	sceneMgr().updateTrees();
	return true;
    }
    else if ( evid == uiEMPartServer::evRemoveTreeObject() )
    {
	const EM::ObjectID emid = emserv_->selEMID();
	const MultiID mid = emserv_->getStorageID(emid);

	TypeSet<int> sceneids;
	visserv_->getChildIds( -1, sceneids );

	TypeSet<int> ids;
	visserv_->findObject( mid, ids );

	for ( int idx=0; idx<ids.size(); idx++ )
	{
	    for ( int idy=0; idy<sceneids.size(); idy++ )
		visserv_->removeObject( ids[idx], sceneids[idy] );
	    sceneMgr().removeTreeItem(ids[idx] );
	}


	sceneMgr().updateTrees();
	return true;
    }
    else if ( evid == uiEMPartServer::evSyncGeometry() )
    {
	mDynamicCastGet( EM::Horizon2D*, h2d, emserv_->selEMObject() );
	for ( int lidx=0; h2d && lidx<h2d->geometry().nrLines(); lidx++ )
	{
	    const int lineid = h2d->geometry().lineID( lidx );
	    if ( h2d->geometry().syncBlocked(lineid) )
		continue;
	    const MultiID& lset = h2d->geometry().lineSet( lineid );
	    const char* lnm = h2d->geometry().lineName( lineid );
	    PosInfo::Line2DData ldat;
	    seisserv_->get2DLineGeometry( lset, lnm, ldat );
	    h2d->geometry().syncLine( lset, lnm, ldat );
	}
    }
    else if ( evid == uiEMPartServer::evCalcShiftAttribute() )
    {
	ObjectSet<DataPointSet> dpsset;
	EMServer()->fillHorShiftDPS( dpsset );
	const Attrib::SelSpec* as = visserv_->getSelSpec(
		visserv_->getEventObjId(), emserv_->attribIdx() );
	attrserv_->setTargetSelSpec( *as );
	attrserv_->createOutput( dpsset );

	TypeSet<DataPointSet::DataRow> drset;
	BufferStringSet nmset;
	DataPointSet* dps = new DataPointSet( drset, nmset, false, true );
	dps->bivSet().setNrVals( dpsset.size()+1 );

	BinIDValueSet::Pos bvspos;
	while ( dpsset[0]->bivSet().next(bvspos) )
	{
	    TypeSet<float> attribvals;
	    attribvals += 0.0;
	    BinID binid;
	    for ( int idx=0; idx<dpsset.size(); idx++ )
	    {
		float zval=0;
		float attribval=0;
		dpsset[idx]->bivSet().get( bvspos, binid, zval, attribval );
		attribvals += attribval;
	    }
	    dps->bivSet().add( binid, attribvals.arr() );
	}
	dps->dataChanged();
	visServer()->setRandomPosData( visServer()->getEventObjId(),
				       EMServer()->attribIdx(), dps );
	visServer()->selectTexture( visServer()->getEventObjId(),
				    EMServer()->attribIdx(),
				    EMServer()->textureIdx() );
    }
    else if ( evid == uiEMPartServer::evHorizonShift() )
    {
	visServer()->setTranslation( visServer()->getEventObjId(),
				     Coord3(0,0,EMServer()->getShift()) );
	if ( !mIsUdf(EMServer()->attribIdx()) )
	    visServer()->selectTexture( visServer()->getEventObjId(),
					EMServer()->attribIdx(),
					EMServer()->textureIdx() );
    }
    else if ( evid == uiEMPartServer::evStoreShiftHorizons() )
    {
	const int visid = visserv_->getEventObjId();
	const uiVisPartServer::AttribFormat format = 
	    				visserv_->getAttributeFormat( visid );
	if ( format!=uiVisPartServer::RandomPos ) return false;

	TypeSet<DataPointSet::DataRow> pts;
	BufferStringSet nms;
	DataPointSet data( pts, nms, false, true );
	visserv_->getRandomPosCache( visid, emserv_->attribIdx(), data );
	if ( data.isEmpty() ) return false;

	const MultiID mid = visserv_->getMultiID( visid );
	const EM::ObjectID emid = emserv_->getObjectID( mid );
	const int nrvals = data.bivSet().nrVals()-1;
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    BufferString auxdatanm( emserv_->getAttribBaseNm() );
	    auxdatanm += "_";
	    auxdatanm += emserv_->shiftRange().atIndex( idx );
	    emserv_->setAuxData( emid, data, auxdatanm, idx+1,
		   		 emserv_->shiftRange().atIndex(idx) );
	    BufferString dummy;
	    emserv_->storeAuxData( emid, dummy, false );
	}
    }
    else if ( evid == uiEMPartServer::evShiftDlgOpened() )
	enableMenusAndToolBars( false );
    else if ( evid == uiEMPartServer::evShiftDlgClosed() )
	enableMenusAndToolBars( true );
    else
	pErrMsg("Unknown event from emserv");

    return true;
}


bool uiODApplMgr::handlePickServEv( int evid )
{
    if ( evid == uiPickPartServer::evGetHorInfo3D() )
    {
	emserv_->getAllSurfaceInfo( pickserv_->horInfos(), false );
    }
    else if ( evid == uiPickPartServer::evGetHorInfo2D() )
    {
	emserv_->getAllSurfaceInfo( pickserv_->horInfos(), true );
    }
    else if ( evid == uiPickPartServer::evGetHorDef3D() )
    {
	TypeSet<EM::ObjectID> horids;
	const ObjectSet<MultiID>& storids = pickserv_->selHorIDs();
	for ( int idx=0; idx<storids.size(); idx++ )
	{
	    const MultiID horid = *storids[idx];
	    const EM::ObjectID id = emserv_->getObjectID(horid);
	    if ( id<0 || !emserv_->isFullyLoaded(id) )
		emserv_->loadSurface( horid );

	    horids += emserv_->getObjectID(horid);
	}
	
	emserv_->getSurfaceDef3D( horids, pickserv_->genDef(),
			       pickserv_->selHorSampling() );
    }
    else if ( evid == uiPickPartServer::evGetHorDef2D() )
	emserv_->getSurfaceDef2D( pickserv_->selHorIDs(),
				  pickserv_->lineGeoms(),
				  pickserv_->selectLines(),
				  pickserv_->getPos2D(),
				  pickserv_->getHor2DZRgs() );
    else if ( evid == uiPickPartServer::evFillPickSet() )
	emserv_->fillPickSet( *pickserv_->pickSet(), pickserv_->horID() );
    else if ( evid == uiPickPartServer::evGet2DLineInfo() )
	seisserv_->get2DLineInfo( pickserv_->lineSets(),
				  pickserv_->lineSetIds(),
				  pickserv_->lineNames());
    else if ( evid == uiPickPartServer::evGet2DLineDef() )
    {
	BufferStringSet& lnms = pickserv_->selectLines();
	for ( int idx=0; idx<lnms.size(); idx++ )
	{
	    PosInfo::Line2DData geom;
	    seisserv_->get2DLineGeometry( pickserv_->lineSetID(), *lnms[idx],
		   			  geom );
	    pickserv_->lineGeoms() += new PosInfo::Line2DData( geom );
	}
    }
    else
	pErrMsg("Unknown event from pickserv");

    return true;
}


#define mGetSelTracker( tracker ) \
    const int selobjvisid = visserv_->getSelObjectId(); \
    const MultiID selobjmid = visserv_->getMultiID(selobjvisid); \
    const EM::ObjectID& emid = emserv_->getObjectID(selobjmid); \
    const int trackerid = mpeserv_->getTrackerID(emid); \
    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid ); 

bool uiODApplMgr::handleVisServEv( int evid )
{
    int visid = visserv_->getEventObjId();
    visserv_->unlockEvent();

    if ( evid == uiVisPartServer::evUpdateTree() )
	sceneMgr().updateTrees();
    else if ( evid == uiVisPartServer::evDeSelection()
	   || evid == uiVisPartServer::evSelection() )
	sceneMgr().updateSelectedTreeItem();
    else if ( evid == uiVisPartServer::evGetNewData() )
	return getNewData( visid, visserv_->getEventAttrib() );
    else if ( evid == uiVisPartServer::evInteraction() )
	sceneMgr().setItemInfo( visid );
    else if ( evid == uiVisPartServer::evMouseMove() ||
	      evid==uiVisPartServer::evPickingStatusChange() )
	sceneMgr().updateStatusBar();
    else if ( evid == uiVisPartServer::evViewModeChange() )
	sceneMgr().setToViewMode( visserv_->isViewMode() );
    else if ( evid == uiVisPartServer::evSelectAttrib() )
	return selectAttrib( visid, visserv_->getEventAttrib() );
    else if ( evid == uiVisPartServer::evViewAll() )
	sceneMgr().viewAll(0);
    else if ( evid == uiVisPartServer::evToHomePos() )
	sceneMgr().toHomePos(0);
    else if ( evid == uiVisPartServer::evShowSetupDlg() )
    {
	mGetSelTracker( tracker );
	const MPE::EMSeedPicker* seedpicker = tracker ? 
					      tracker->getSeedPicker(false) : 0;
	const EM::SectionID sid = seedpicker ? seedpicker->getSectionID() : -1;
	mpeserv_->showSetupDlg( emid, sid, true );
	visserv_->updateMPEToolbar();
    }
    else if ( evid == uiVisPartServer::evLoadPostponedData() )
	mpeserv_->loadPostponedVolume();
    else if ( evid == uiVisPartServer::evToggleBlockDataLoad() )
	mpeserv_->blockDataLoading( !mpeserv_->isDataLoadingBlocked() );
    else if ( evid == uiVisPartServer::evDisableSelTracker() )
    {
	mGetSelTracker( tracker );
	if ( tracker )
	    tracker->enable( false );
    }
    else if ( evid == uiVisPartServer::evColorTableChange() )
	modifyColorTable( visserv_->getEventObjId(), 
			  visserv_->getEventAttrib() );
    else
	pErrMsg("Unknown event from visserv");

    return true;
}


bool uiODApplMgr::handleNLAServEv( int evid )
{
    if ( evid == uiNLAPartServer::evPrepareWrite() )
    {
	// Before NLA model can be written, the AttribSet's IOPar must be
	// made available as it almost certainly needs to be stored there.
	const Attrib::DescSet* ads = attrserv_->curDescSet(nlaserv_->is2DEvent());
	if ( !ads ) return false;
	IOPar& iopar = nlaserv_->modelPars();
	iopar.clear();
	BufferStringSet inputs = nlaserv_->modelInputs();
	const Attrib::DescSet* cleanads = ads->optimizeClone( inputs );
	(cleanads ? cleanads : ads)->fillPar( iopar );
	delete cleanads;
    }
    else if ( evid == uiNLAPartServer::evPrepareRead() )
    {
	bool saved = attrserv_->setSaved(nlaserv_->is2DEvent());
        const char* msg = "Current attribute set is not saved.\nSave now?";
        if ( !saved && uiMSG().askGoOn( msg ) )
	    attrserv_->saveSet(nlaserv_->is2DEvent());
    }
    else if ( evid == uiNLAPartServer::evReadFinished() )
    {
	// New NLA Model available: replace the attribute set!
	// Create new attrib set from NLA model's IOPar

	attrserv_->replaceSet( nlaserv_->modelPars(), nlaserv_->is2DEvent() );
	wellattrserv_->setNLAModel( &nlaserv_->getModel() );
    }
    else if ( evid == uiNLAPartServer::evGetInputNames() )
    {
	// Construct the choices for input nodes.
	// Should be:
	// * All attributes (stored ones filtered out)
	// * All cubes - between []
	attrserv_->getPossibleOutputs( nlaserv_->is2DEvent(),
				      nlaserv_->inputNames() );
	if ( nlaserv_->inputNames().size() == 0 )
	    { uiMSG().error( "No usable input" ); return false; }
    }
    else if ( evid == uiNLAPartServer::evGetStoredInput() )
    {
	BufferStringSet linekeys;
	nlaserv_->getNeededStoredInputs( linekeys );
	for ( int idx=0; idx<linekeys.size(); idx++ )
            attrserv_->addToDescSet( linekeys.get(idx), nlaserv_->is2DEvent() );
    }
    else if ( evid == uiNLAPartServer::evGetData() )
    {
	// OK, the input and output nodes are known.
	// Query the server and make sure the relevant data is extracted
	// Put data in the training and test posvec data sets

	if ( !attrserv_->curDescSet(nlaserv_->is2DEvent()) ) 
	    { pErrMsg("Huh"); return false; }
	ObjectSet<DataPointSet> dpss;
	const bool dataextraction = nlaserv_->willDoExtraction();
	if ( !dataextraction )
	    dpss += new DataPointSet( nlaserv_->is2DEvent(), false );
	else
	{
	    nlaserv_->getDataPointSets( dpss );
	    if ( dpss.isEmpty() )
		{ uiMSG().error("No valid data locations found"); return false;}
	    if ( !attrserv_->extractData(dpss) )
		return true;
	    IOPar& iop = nlaserv_->storePars();
	    attrserv_->curDescSet(nlaserv_->is2DEvent())->fillPar( iop );
	    if ( iop.name().isEmpty() )
		iop.setName( "Attributes" );
	}

	const char* res = nlaserv_->prepareInputData( dpss );
	if ( res && *res && strcmp(res,uiNLAPartServer::sKeyUsrCancel()) )
	    uiMSG().warning( res );
	if ( !dataextraction ) // i.e. if we have just read a DataPointSet
	    attrserv_->replaceSet( dpss[0]->dataSet().pars(), dpss[0]->is2D() );
	deepErase(dpss);
    }
    else if ( evid == uiNLAPartServer::evSaveMisclass() )
    {
	const DataPointSet& dps = nlaserv_->dps();
	BinIDValueSet mcpicks( 1, true );
	for ( int irow=0; irow<dps.size(); irow++ )
	{
	    if ( dps.group(irow) == 3 )
		mcpicks.add( dps.binID(irow), dps.z(irow) );
	}
	pickserv_->setMisclassSet( mcpicks );
    }
    else if ( evid == uiNLAPartServer::evCreateAttrSet() )
    {
	Attrib::DescSet attrset(nlaserv_->is2DEvent());
	if ( !attrserv_->createAttributeSet(nlaserv_->modelInputs(),&attrset) )
	    return false;
	attrset.fillPar( nlaserv_->modelPars() );
	attrserv_->replaceSet( nlaserv_->modelPars(), nlaserv_->is2DEvent() );
    }
    else if ( evid == uiNLAPartServer::evShowSelPts() )
	pickserv_->setPickSet( nlaserv_->getSelectedPts() );
    else
	pErrMsg("Unknown event from nlaserv");

    return true;
}


bool uiODApplMgr::handleAttribServEv( int evid )
{
    if ( evid==uiAttribPartServer::evDirectShowAttr() )
    {
	Attrib::SelSpec as;
	attrserv_->getDirectShowAttrSpec( as );
	const int visid = visserv_->getEventObjId();
	const int attrib = visserv_->getSelAttribNr();
	if ( attrib<0 || attrib>=visserv_->getNrAttribs(visid) )
	{
	    uiMSG().error( "Please select an attribute element in the tree" );
	    return false;
	}

	visserv_->setSelSpec( visid, attrib, as );
	getNewData( visid, attrib );
	sceneMgr().updateTrees();
    }
    else if ( evid==uiAttribPartServer::evNewAttrSet() )
	mpeserv_->setCurrentAttribDescSet(
				attrserv_->curDescSet(attrserv_->is2DEvent()) );
    else if ( evid==uiAttribPartServer::evAttrSetDlgClosed() )
    {
	enableMenusAndToolBars( true );
	enableSceneManipulation( true );
    }
    else if ( evid==uiAttribPartServer::evEvalAttrInit() )
    {
	const uiVisPartServer::AttribFormat format = 
	    	visserv_->getAttributeFormat( visserv_->getEventObjId() );
	const bool alloweval = !( format==uiVisPartServer::None );
	const bool allowstorage = format==uiVisPartServer::RandomPos;
	attrserv_->setEvaluateInfo( alloweval, allowstorage );
    }
    else if ( evid==uiAttribPartServer::evEvalCalcAttr() )
    {
	const int visid = visserv_->getEventObjId();
	Attrib::SelSpec as( "Evaluation", Attrib::SelSpec::cOtherAttrib() );

	const TypeSet<Attrib::SelSpec>& tmpset = attrserv_->getTargetSelSpecs();
	BufferString savedusrref = tmpset.size() ? tmpset[0].objectRef() : "";
	as.setObjectRef( savedusrref );
	const int attrib = visserv_->getSelAttribNr();
	if ( attrib<0 || attrib>=visserv_->getNrAttribs(visid) )
	{
	    uiMSG().error( "Please select an attribute element in the tree" );
	    return false;
	}
	as.set2DFlag( attrserv_->is2DEvent() );
	visserv_->setSelSpec( visid, attrib, as );
	const bool success = as.is2D() ? evaluate2DAttribute(visid,attrib)
	    			       : evaluateAttribute(visid,attrib);
	if ( !success )
	{
	    uiMSG().error( "Could not evaluate this attribute" );
	    return false;
	}
	sceneMgr().updateTrees();
    }
    else if ( evid==uiAttribPartServer::evEvalShowSlice() )
    {
	const int visid = visserv_->getEventObjId();
	const int sliceidx = attrserv_->getSliceIdx();
	const int attrnr =
	    visserv_->getSelAttribNr()==-1 ? 0 : visserv_->getSelAttribNr();
	visserv_->selectTexture( visid, attrnr, sliceidx );
	modifyColorTable( visid, attrnr );
	sceneMgr().updateTrees();
    }
    else if ( evid==uiAttribPartServer::evEvalStoreSlices() )
    {
	const int visid = visserv_->getEventObjId();
	const int attrnr =
	    visserv_->getSelAttribNr()==-1 ? 0 : visserv_->getSelAttribNr();
	const uiVisPartServer::AttribFormat format = 
	    				visserv_->getAttributeFormat( visid );
	if ( format!=uiVisPartServer::RandomPos ) return false;

	TypeSet<DataPointSet::DataRow> pts;
	BufferStringSet nms;
	DataPointSet data( pts, nms, false, true );
	visserv_->getRandomPosCache( visid, attrnr, data );
	if ( data.isEmpty() ) return false;

	const MultiID mid = visserv_->getMultiID( visid );
	const EM::ObjectID emid = emserv_->getObjectID( mid );
	const TypeSet<Attrib::SelSpec>& specs = attrserv_->getTargetSelSpecs();
	const int nrvals = data.bivSet().nrVals()-1;
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    emserv_->setAuxData( emid, data, specs[idx].userRef(), idx+1,
		   emserv_->shiftRange().atIndex(idx) );
	    BufferString dummy;
	    emserv_->storeAuxData( emid, dummy, false );
	}
    }
    else if ( evid == uiAttribPartServer::evShowSelPtPickSet() )
	pickserv_->setPickSet( attrserv_->getSelPickSet() );
    else if ( evid==uiAttribPartServer::evEvalUpdateName() )
    {
	const int visid = visserv_->getEventObjId();
	const int attrib = visserv_->getSelAttribNr();
	Attrib::SelSpec* as = const_cast<Attrib::SelSpec*>(
					visserv_->getSelSpec(visid,attrib) );
	//set user chosen name stocked in objectRef during evaluation process
	if ( as ) as->setUserRef( as->objectRef() );
	sceneMgr().updateTrees();
    }
    else
	pErrMsg("Unknown event from attrserv");

    return true;
}


void uiODApplMgr::pageUpDownPressed( bool pageup )
{
    const int visid = visserv_->getEventObjId();
    const int attrib = visserv_->getSelAttribNr();
    if ( attrib<0 || attrib>=visserv_->getNrAttribs(visid) )
	return;

    int texture = visserv_->selectedTexture( visid, attrib );
    if ( texture<visserv_->nrTextures(visid,attrib)-1 && !pageup )
	texture++;
    else if ( texture && pageup )
	texture--;

    visserv_->selectTexture( visid, attrib, texture );
    modifyColorTable( visid, attrib );
    sceneMgr().updateTrees();
}


void uiODApplMgr::modifyColorTable( int visid, int attrib )
{
    if ( attrib<0 || attrib>=visserv_->getNrAttribs(visid) )
	return;

    coltabvisid_ = visid;
    coltabattribnr_ = attrib;

    appl_.colTabEd().setColTab( visserv_->getColTabSequence( visid, attrib ),
	    			true,
				visserv_->getColTabMapperSetup(visid,attrib) );
    setHistogram( visid, attrib );
}


void uiODApplMgr::colSeqChg( CallBacker* )
{
    const int visid = visserv_->getSelObjectId();
    int attrib = visserv_->getSelAttribNr();
    if ( attrib == -1 ) attrib = 0;
    setHistogram( visid, attrib );

    visserv_->setColTabSequence( visid, attrib,
	    appl_.colTabEd().getColTabSequence() );

    sceneMgr().updateSelectedTreeItem();
}


void uiODApplMgr::colMapperChg( CallBacker* )
{
    const int visid = visserv_->getSelObjectId();
    int attrib = visserv_->getSelAttribNr();
    if ( attrib == -1 ) attrib = 0;

    visserv_->setColTabMapperSetup( visid, attrib,
	    appl_.colTabEd().getColTabMapperSetup() );
    setHistogram( visid, attrib );

    //Autoscale may have changed ranges, so update.
    appl_.colTabEd().setColTab( visserv_->getColTabSequence( visid, attrib ),
	    			true,
				visserv_->getColTabMapperSetup(visid,attrib) );
}


NotifierAccess* uiODApplMgr::colorTableSeqChange()
{
    return &appl_.colTabEd().seqChange();
}


void uiODApplMgr::setHistogram( int visid, int attrib )
{ appl_.colTabEd().setHistogram( visserv_->getHistogram(visid,attrib) ); }


void uiODApplMgr::setupRdmLinePreview(const TypeSet<Coord>& coords)
{
    if ( wellserv_->getPreviewIds().size()>0 )
	cleanPreview();

    TypeSet<int> plids;
    TypeSet<int> sceneids;
    visSurvey::PolyLineDisplay* pl = visSurvey::PolyLineDisplay::create();
    pl->fillPolyLine( coords );
    mDynamicCastGet(visBase::DataObject*,doobj,pl);
    visserv_->getChildIds( -1, sceneids );
    
    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	visserv_->addObject( doobj, sceneids[idx], true );
	plids.addIfNew( doobj->id() );
    }
    wellserv_->setPreviewIds(plids);
}


void uiODApplMgr::cleanPreview()
{
    TypeSet<int> sceneids;
    visserv_->getChildIds( -1, sceneids );
    TypeSet<int>& previds = wellserv_->getPreviewIds();
    if ( previds.size() == 0 ) return;
    for ( int idx=0; idx<sceneids.size(); idx++ )
	visserv_->removeObject( previds[0],sceneids[idx] );

    previds.erase();
}


void uiODApplMgr::createAndSetMapDataPack( int visid, int attrib,
					   const DataPointSet& data, int colnr )
{
    DataPack::ID cacheid = visserv_->getDataPackID( visid, attrib );
    if ( cacheid == -1 )
	useDefColTab( visid, attrib );
    const int dpid = createMapDataPack( data, colnr );
    visserv_->setDataPackID( visid, attrib, dpid );
    visserv_->setRandomPosData( visid, attrib, &data );
}


int uiODApplMgr::createMapDataPack( const DataPointSet& data, int colnr )
{
    BIDValSetArrAdapter* bvsarr = new BIDValSetArrAdapter(data.bivSet(), colnr);
    MapDataPack* newpack = new MapDataPack( "Attribute", data.name(), bvsarr );
    StepInterval<double> inlrg( bvsarr->inlrg_.start, bvsarr->inlrg_.stop, 
	    			SI().inlStep() );
    StepInterval<double> crlrg( bvsarr->crlrg_.start, bvsarr->crlrg_.stop,
	    			SI().crlStep() );
    BufferStringSet dimnames;
    dimnames.add("X").add("Y").add("In-Line").add("Cross-line");
    newpack->setPropsAndInit( inlrg, crlrg, false, &dimnames );
    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );
    dpman.add( newpack );
    return newpack->id();
}
