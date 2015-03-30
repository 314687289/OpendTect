/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiseispartserv.h"

#include "arrayndimpl.h"
#include "ctxtioobj.h"
#include "iodir.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "posinfo2d.h"
#include "posinfo2dsurv.h"
#include "ptrman.h"
#include "seisselection.h"
#include "seistrctr.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "seis2ddata.h"
#include "seisbuf.h"
#include "seisbufadapters.h"
#include "seispreload.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "seisioobjinfo.h"
#include "strmprov.h"
#include "survinfo.h"

#include "uibatchtime2depthsetup.h"
#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"
#include "uiflatviewmainwin.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimenu.h"
#include "uimergeseis.h"
#include "uimsg.h"
#include "uisegyexp.h"
#include "uisegyread.h"
#include "uisegyresortdlg.h"
#include "uisegysip.h"
#include "uiseisimportcbvs.h"
#include "uiseiscbvsimpfromothersurv.h"
#include "uiseisfileman.h"
#include "uiseisioobjinfo.h"
#include "uiseisiosimple.h"
#include "uiseismulticubeps.h"
#include "uiseispsman.h"
#include "uiseisrandto2dline.h"
#include "uiseissel.h"
#include "uiseislinesel.h"
#include "uiseiswvltimpexp.h"
#include "uiseiswvltman.h"
#include "uiseispreloadmgr.h"
#include "uiselsimple.h"
#include "uisurvey.h"
#include "uisurvinfoed.h"
#include "uitaskrunner.h"
#include "uivelocityvolumeconversion.h"
#include "od_helpids.h"


static const char* sKeyPreLoad()	{ return "PreLoad"; }

uiSeisPartServer::uiSeisPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , man2dseisdlg_(0)
    , man3dseisdlg_(0)
    , man2dprestkdlg_(0)
    , man3dprestkdlg_(0)
    , manwvltdlg_(0)
{
    uiSEGYSurvInfoProvider* sip = new uiSEGYSurvInfoProvider();
    uiSurveyInfoEditor::addInfoProvider( sip );
    SeisIOObjInfo::initDefault( sKey::Steering() );
    IOM().surveyChanged.notify( mCB(this,uiSeisPartServer,survChangedCB) );
}


uiSeisPartServer::~uiSeisPartServer()
{
    delete man2dseisdlg_;
    delete man3dseisdlg_;
    delete man2dprestkdlg_;
    delete man3dprestkdlg_;
    delete manwvltdlg_;
}


void uiSeisPartServer::survChangedCB( CallBacker* )
{
    delete man2dseisdlg_; man2dseisdlg_ = 0;
    delete man3dseisdlg_; man3dseisdlg_ = 0;
    delete man2dprestkdlg_; man2dprestkdlg_ = 0;
    delete man3dprestkdlg_; man3dprestkdlg_ = 0;
    delete manwvltdlg_; manwvltdlg_ = 0;
}


bool uiSeisPartServer::ioSeis( int opt, bool forread )
{
    PtrMan<uiDialog> dlg = 0;
    if ( opt == 0 )
	dlg = new uiSeisImportCBVS( parent() );
    else if ( opt == 9 )
	dlg = new uiSeisImpCBVSFromOtherSurveyDlg( parent() );
    else if ( opt < 5 )
    {
	if ( !forread )
	    dlg = new uiSEGYExp( parent(),
				 Seis::geomTypeOf( !(opt%2), opt > 2 ) );
	else
	{
	    const bool isdirect = !(opt % 2);
	    uiSEGYRead::Setup su( isdirect ? uiSEGYRead::DirectDef
					   : uiSEGYRead::Import );
	    if ( isdirect )
		su.geoms_ -= Seis::Line;
	    new uiSEGYRead( parent(), su );
	}
    }
    else
    {
	const Seis::GeomType gt( Seis::geomTypeOf( !(opt%2), opt > 6 ) );
	if ( !uiSurvey::survTypeOKForUser(Seis::is2D(gt)) ) return true;
	dlg = new uiSeisIOSimple( parent(), gt, forread );
    }


    return dlg ? dlg->go() : true;
}


bool uiSeisPartServer::importSeis( int opt )
{ return ioSeis( opt, true ); }
bool uiSeisPartServer::exportSeis( int opt )
{ return ioSeis( opt, false ); }


#define mManageSeisDlg( dlgobj, dlgclss, modal ) \
    delete dlgobj; \
    dlgobj = new dlgclss( parent(), is2d ); \
    dlgobj->setModal( modal ); \
    dlgobj->go();

void uiSeisPartServer::manageSeismics( int opt )
{
    const bool is2d = opt == 1 || opt == 3;
    switch( opt )
    {
	case 0: mManageSeisDlg(man3dseisdlg_,uiSeisFileMan,false);
		break;
	case 1: mManageSeisDlg(man2dseisdlg_,uiSeisFileMan,false);
		break;
	case 2: mManageSeisDlg(man3dprestkdlg_,uiSeisPreStackMan,false);
		break;
	case 3: mManageSeisDlg(man2dprestkdlg_,uiSeisPreStackMan,false);
		break;
    }
}


void uiSeisPartServer::manageSeismicsModal( int opt )
{
    const bool is2d = opt == 1 || opt == 3;
    switch( opt )
    {
	case 0: mManageSeisDlg(man3dseisdlg_,uiSeisFileMan,true);
		break;
	case 1: mManageSeisDlg(man2dseisdlg_,uiSeisFileMan,true);
		break;
	case 2: mManageSeisDlg(man3dprestkdlg_,uiSeisPreStackMan,true);
		break;
	case 3: mManageSeisDlg(man2dprestkdlg_,uiSeisPreStackMan,true);
		break;
    }
}



void uiSeisPartServer::managePreLoad()
{
    uiSeisPreLoadMgr dlg( parent() );
    dlg.go();
}


void uiSeisPartServer::importWavelets()
{
    uiSeisWvltImp dlg( parent() );
    dlg.go();
}


void uiSeisPartServer::exportWavelets()
{
    uiSeisWvltExp dlg( parent() );
    dlg.go();
}


void uiSeisPartServer::manageWavelets()
{
    delete manwvltdlg_;
    manwvltdlg_ = new uiSeisWvltMan( parent() );
    manwvltdlg_->go();
}


bool uiSeisPartServer::select2DSeis( MultiID& mid )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(SeisTrc);
    uiSeisSel::Setup setup(Seis::Line);
    uiSeisSelDlg dlg( parent(), *ctio, setup );
    if ( !dlg.go() || !dlg.ioObj() ) return false;

    mid = dlg.ioObj()->key();
    return true;
}


#define mGet2DDataSet(retval) \
    PtrMan<IOObj> ioobj = IOM().get( mid ); \
    if ( !ioobj ) return retval; \
    Seis2DDataSet dataset( *ioobj );


void uiSeisPartServer::get2DDataSetName( const MultiID& mid,
					 BufferString& setname )
{
    mGet2DDataSet(;)
    setname = dataset.name();
}


bool uiSeisPartServer::select2DLines( TypeSet<Pos::GeomID>& selids,
				      int& action )
{
    selids.erase();

    uiDialog::Setup dsu( "Select 2D Lines", mNoDlgTitle,
			 mODHelpKey(mSeisPartServerselect2DLinesHelpID) );
    uiDialog dlg( parent(), dsu );
    MouseCursorChanger cursorchgr( MouseCursor::Wait );
    uiSeis2DLineChoose* lchfld =
		new uiSeis2DLineChoose( &dlg, OD::ChooseAtLeastOne );
    BufferStringSet options;
    options.add( "Display projection lines only" )
	   .add( "Load default data" )
	   .add( "Select attribute" );
    uiGenInput* optfld =
	new uiGenInput( &dlg, "On OK", StringListInpSpec(options) );
    optfld->attach( alignedBelow, lchfld );
    cursorchgr.restore();
    if ( !dlg.go() )
	return false;

    action = optfld->getIntValue();
    lchfld->getChosen( selids );
    return selids.size();
}


bool uiSeisPartServer::select2DLines( BufferStringSet& selnames,
				      TypeSet<Pos::GeomID>& selids )
{
    int action = 0;
    const bool res = select2DLines( selids, action );
    if ( !res ) return false;

    for ( int idx=0; idx<selids.size(); idx++ )
	selnames.add( Survey::GM().getName(selids[idx]) );

    return true;
}


void uiSeisPartServer::get2DLineInfo( TypeSet<Pos::GeomID>& geomids,
				      BufferStringSet& linenames )
{
    Survey::GM().getList( linenames, geomids, true );
}


void uiSeisPartServer::get2DStoredAttribs( const char* linenm,
					   BufferStringSet& datasets,
					   int steerpol )
{
    SeisIOObjInfo::Opts2D o2d; o2d.steerpol_ = steerpol;
    SeisIOObjInfo::getDataSetNamesForLine( linenm, datasets, o2d );
}


bool uiSeisPartServer::create2DOutput( const MultiID& mid, const char* linekey,
				       CubeSampling& cs, SeisTrcBuf& buf )
{
    mGet2DDataSet(false)

    const int lidx = dataset.indexOf( linekey );
    if ( lidx < 0 ) return false;

    StepInterval<int> trcrg;
    dataset.getRanges( lidx, trcrg, cs.zrg );
    cs.hrg.setCrlRange( trcrg );
    PtrMan<Executor> exec = dataset.lineFetcher( lidx, buf );
    uiTaskRunner dlg( parent() );
    return TaskRunner::execute( &dlg, *exec );
}


void uiSeisPartServer::getStoredGathersList( bool for3d,
					     BufferStringSet& nms ) const
{
    const IODir iodir(
	MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id) );
    const ObjectSet<IOObj>& ioobjs = iodir.getObjs();

    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	if ( SeisTrcTranslator::isPS(ioobj)
	  && SeisTrcTranslator::is2D(ioobj) != for3d )
	    nms.add( (const char*)ioobj.name() );
    }

    nms.sort();
}


void uiSeisPartServer::storeRlnAs2DLine( const Geometry::RandomLine& rln ) const
{
    uiSeisRandTo2DLineDlg dlg( parent(), &rln );
    dlg.go();
}


void uiSeisPartServer::resortSEGY() const
{
    uiResortSEGYDlg dlg( parent() );
    dlg.go();
}


void uiSeisPartServer::processTime2Depth() const
{
    uiBatchTime2DepthSetup dlg( parent() );
    dlg.go();
}


void uiSeisPartServer::processVelConv() const
{
    Vel::uiBatchVolumeConversion dlg( parent() );
    dlg.go();
}


void uiSeisPartServer::createMultiCubeDataStore() const
{
    uiSeisMultiCubePS dlg( parent() );
    dlg.go();
}


void uiSeisPartServer::get2DZdomainAttribs( const char* linenm,
			const char* zdomainstr, BufferStringSet& attribs )
{
    SeisIOObjInfo::Opts2D o2d;
    o2d.zdomky_ = zdomainstr;
    SeisIOObjInfo::getDataSetNamesForLine( linenm, attribs, o2d );
}


void uiSeisPartServer::fillPar( IOPar& par ) const
{
    BufferStringSet ids;
    StreamProvider::getPreLoadedIDs( ids );
    for ( int iobj=0; iobj<ids.size(); iobj++ )
    {
	const MultiID id( ids.get(iobj).buf() );
	IOPar iop;
	Seis::PreLoader spl( id ); spl.fillPar( iop );
	const BufferString parkey = IOPar::compKey( sKeyPreLoad(), iobj );
	par.mergeComp( iop, parkey );
    }
}


bool uiSeisPartServer::usePar( const IOPar& par )
{
    StreamProvider::unLoadAll();
    PtrMan<IOPar> plpar = par.subselect( sKeyPreLoad() );
    if ( !plpar ) return true;

    IOPar newpar;
    newpar.mergeComp( *plpar, "Seis" );

    uiTaskRunner uitr( parent() );
    Seis::PreLoader::load( newpar, &uitr );
    return true;
}
