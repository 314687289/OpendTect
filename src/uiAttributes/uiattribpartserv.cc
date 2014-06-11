/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiattribpartserv.h"

#include "attribdatacubes.h"
#include "attribdataholder.h"
#include "attribdatapack.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetman.h"
#include "attribdescsetsholder.h"
#include "attribdescsettr.h"
#include "attribengman.h"
#include "attribfactory.h"
#include "attribposvecoutput.h"
#include "attribprocessor.h"
#include "attribsel.h"
#include "uiattrsetman.h"
#include "attribsetcreator.h"
#include "attribstorprovider.h"

#include "arraynd.h"
#include "binidvalset.h"
#include "coltabmapper.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "datapointset.h"
#include "executor.h"
#include "iodir.h"
#include "iopar.h"
#include "ioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "nlacrdesc.h"
#include "nlamodel.h"
#include "posvecdataset.h"
#include "datapointset.h"
#include "ptrman.h"
#include "randcolor.h"
#include "seisbuf.h"
#include "seisinfo.h"
#include "survinfo.h"
#include "settings.h"
#include "zdomain.h"

#include "uiattrdesced.h"
#include "uiattrdescseted.h"
#include "uiattrgetfile.h"
#include "uiattrsel.h"
#include "uiattr2dsel.h"
#include "uiattrvolout.h"
#include "uiattribcrossplot.h"
#include "uievaluatedlg.h"
#include "uigeninputdlg.h"
#include "uiioobjsel.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uimultcomputils.h"
#include "uimultoutsel.h"
#include "uiseisioobjinfo.h"
#include "uisetpickdirs.h"
#include "uitaskrunner.h"
#include "uicrossattrevaluatedlg.h"

using namespace Attrib;

int uiAttribPartServer::evDirectShowAttr()	    { return 0; }
int uiAttribPartServer::evNewAttrSet()		    { return 1; }
int uiAttribPartServer::evAttrSetDlgClosed()	    { return 2; }
int uiAttribPartServer::evEvalAttrInit()	    { return 3; }
int uiAttribPartServer::evEvalCalcAttr()	    { return 4; }
int uiAttribPartServer::evEvalShowSlice()	    { return 5; }
int uiAttribPartServer::evEvalStoreSlices()	    { return 6; }
int uiAttribPartServer::evEvalRestore()	    { return 7; }
int uiAttribPartServer::objNLAModel2D()		    { return 100; }
int uiAttribPartServer::objNLAModel3D()		    { return 101; }

const char* uiAttribPartServer::attridstr()	    { return "Attrib ID"; }

static const int cMaxNrClasses = 100;
static const int cMaxMenuSize = 150;


static const char* getMenuText( bool is2d, bool issteering, bool endmenu )
{
    mDeclStaticString(menutext);
    if ( is2d )
	menutext = issteering ? "Stee&ring 2D Data" : "&Stored 2D Data";
    else
	menutext = issteering ? "Steer&ing Cubes" : "Stored &Cubes";

    if ( endmenu ) menutext.add( " ..." );

    return menutext;
}


uiAttribPartServer::uiAttribPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , dirshwattrdesc_(0)
    , attrsetdlg_(0)
    , is2devsent_(false)
    , attrsetclosetim_("Attrset dialog close")
    , multcomp3d_(uiStrings::s3D())
    , multcomp2d_(uiStrings::s2D())
    , dpsdispmgr_( 0 )
    , evalmapperbackup_( 0 )
    , attrsneedupdt_(true)
{
    attrsetclosetim_.tick.notify(
			mCB(this,uiAttribPartServer,attrsetDlgCloseTimTick) );

    stored2dmnuitem_.checkable = true;
    stored3dmnuitem_.checkable = true;
    calc2dmnuitem_.checkable = true;
    calc3dmnuitem_.checkable = true;
    steering2dmnuitem_.checkable = true;
    steering3dmnuitem_.checkable = true;
    multcomp3d_.checkable = true;
    multcomp2d_.checkable = true;

    handleAutoSet();
}


uiAttribPartServer::~uiAttribPartServer()
{
    delete attrsetdlg_;

    deepErase( attrxplotset_ );
    delete &eDSHolder();
    deepErase( attrxplotset_ );
}


void uiAttribPartServer::handleAutoSet()
{
    bool douse = false;
    Settings::common().getYN( uiAttribDescSetEd::sKeyUseAutoAttrSet, douse );
    if ( douse )
    {
	if ( SI().has2D() ) useAutoSet( true );
	if ( SI().has3D() ) useAutoSet( false );
    }
}


void uiAttribPartServer::useAutoSet( bool is2d )
{
    MultiID id;
    const char* idkey = is2d ? uiAttribDescSetEd::sKeyAuto2DAttrSetID
			     : uiAttribDescSetEd::sKeyAuto3DAttrSetID;
    Attrib::DescSetMan* setmgr = eDSHolder().getDescSetMan(is2d);
    if ( SI().pars().get(idkey,id) )
    {
	PtrMan<IOObj> ioobj = IOM().get( id );
	BufferString bs;
	DescSet* attrset = new DescSet( is2d );
	if ( !ioobj || !AttribDescSetTranslator::retrieve(*attrset,ioobj,bs)
		|| attrset->is2D()!=is2d )
	    delete attrset;
	else
	{
	    setmgr->setDescSet( attrset );
	    setmgr->attrsetid_ = id;
	}
    }
}


bool uiAttribPartServer::replaceSet( const IOPar& iopar, bool is2d )
{
    DescSet* ads = new DescSet(is2d);
    TypeSet<uiString> errmsgs;
    if ( !ads->usePar(iopar,&errmsgs) )
    {
	delete ads;
	uiMSG().errorWithDetails( errmsgs );
	return false;
    }
    eDSHolder().replaceAttribSet( ads );

    DescSetMan* adsman = eDSHolder().getDescSetMan( is2d );
    adsman->attrsetid_ = "";
    if ( attrsetdlg_ && attrsetdlg_->is2D()==is2d )
	attrsetdlg_->setDescSetMan( adsman );
    set2DEvent( is2d );
    sendEvent( evNewAttrSet() );
    return true;
}


bool uiAttribPartServer::addToDescSet( const char* key, bool is2d )
{
    //TODO: think of it: stored data can  be at 2 places: also in attrib set...
    return eDSHolder().getDescSet(is2d,true)->getStoredID( key ).isValid();
}


const DescSet* uiAttribPartServer::curDescSet( bool is2d ) const
{
    return DSHolder().getDescSetMan( is2d )->descSet();
}


void uiAttribPartServer::getDirectShowAttrSpec( SelSpec& as ) const
{
   if ( !dirshwattrdesc_ )
       as.set( 0, SelSpec::cNoAttrib(), false, 0 );
   else
       as.set( *dirshwattrdesc_ );
}


void uiAttribPartServer::manageAttribSets()
{
    uiAttrSetMan dlg( parent() );
    dlg.go();
}


bool uiAttribPartServer::editSet( bool is2d )
{
    const DescSetMan* adsman = DSHolder().getDescSetMan( is2d );
    IOPar iop;
    if ( adsman->descSet() ) adsman->descSet()->fillPar( iop );

    delete attrsetdlg_;
    attrsetdlg_ = new uiAttribDescSetEd( parent(),
					 const_cast<DescSetMan*>(adsman), 0 );
    attrsetdlg_->applycb.notify(
		mCB(this,uiAttribPartServer,attrsetDlgApply) );
    attrsetdlg_->dirshowcb.notify(
		mCB(this,uiAttribPartServer,directShowAttr) );
    attrsetdlg_->evalattrcb.notify(
		mCB(this,uiAttribPartServer,showEvalDlg) );
    attrsetdlg_->crossevalattrcb.notify(
		mCB(this,uiAttribPartServer,showCrossEvalDlg) );
    attrsetdlg_->xplotcb.notify(
		mCB(this,uiAttribPartServer,showXPlot) );
    if ( attrsneedupdt_ )
    {
	attrsetdlg_->updtAllEntries();
	attrsneedupdt_ = false;
    }

    attrsetdlg_->windowClosed.notify(
		mCB(this,uiAttribPartServer,attrsetDlgClosed) );
    return attrsetdlg_->go();
}


void uiAttribPartServer::showXPlot( CallBacker* cb )
{
    bool is2d = false;
    if ( !cb )
	is2d = is2DEvent();
    else if ( attrsetdlg_ )
	is2d = attrsetdlg_->getSet()->is2D();
    uiAttribCrossPlot* uiattrxplot = new uiAttribCrossPlot( 0,
		 *(attrsetdlg_ ? attrsetdlg_->getSet() : curDescSet(is2d)) );
    uiattrxplot->windowClosed.notify(
	    mCB(this,uiAttribPartServer,xplotClosedCB) );
    uiattrxplot->setDisplayMgr( dpsdispmgr_ );
    uiattrxplot->setDeleteOnClose( false );
    uiattrxplot->show();
    attrxplotset_ += uiattrxplot;
}


void uiAttribPartServer::xplotClosedCB( CallBacker* cb )
{
    mDynamicCastGet(uiAttribCrossPlot*,crossplot,cb);
    if ( !crossplot ) return;
    if ( attrxplotset_.isPresent(crossplot) )
    {
	uiAttribCrossPlot* xplot =
	    attrxplotset_.removeSingle( attrxplotset_.indexOf(crossplot) );
	xplot->windowClosed.remove(
		mCB(this,uiAttribPartServer,xplotClosedCB) );
	xplot->setDeleteOnClose( true );
    }
}


void uiAttribPartServer::attrsetDlgApply( CallBacker* )
{
    const bool is2d = attrsetdlg_->getSet()->is2D();
    DescSetMan* adsman = eDSHolder().getDescSetMan( is2d );
    adsman->setDescSet( new Attrib::DescSet( *attrsetdlg_->getSet() ) );
    adsman->attrsetid_ = attrsetdlg_->curSetID();
    set2DEvent( is2d );
    sendEvent( evNewAttrSet() );
}


void uiAttribPartServer::attrsetDlgClosed( CallBacker* )
{
    attrsetclosetim_.start( 10, true );
}


void uiAttribPartServer::attrsetDlgCloseTimTick( CallBacker* )
{
    if ( attrsetdlg_->uiResult() )
    {
	bool is2d = attrsetdlg_->getSet()->is2D();
	DescSetMan* adsman = eDSHolder().getDescSetMan( is2d );
	adsman->setDescSet( new Attrib::DescSet( *attrsetdlg_->getSet() ) );
	adsman->attrsetid_ = attrsetdlg_->curSetID();
	set2DEvent( is2d );
	sendEvent( evNewAttrSet() );
    }

    delete attrsetdlg_;
    attrsetdlg_ = 0;
    sendEvent( evAttrSetDlgClosed() );
}


const NLAModel* uiAttribPartServer::getNLAModel( bool is2d ) const
{
    return (NLAModel*)getObject( is2d ? objNLAModel2D() : objNLAModel3D() );
}


bool uiAttribPartServer::selectAttrib( SelSpec& selspec,
				       const ZDomain::Info* zdominfo,
				       Pos::GeomID geomid,
				       const char* seltxt )
{
    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid );
    const bool is2d = geom && geom->is2D();
    DescSetMan* adsman = eDSHolder().getDescSetMan( is2d );
    if ( !adsman->descSet() )
	return false;

    uiAttrSelData attrdata( *adsman->descSet() );
    attrdata.attribid_ = selspec.isNLA() ? SelSpec::cNoAttrib() : selspec.id();
    attrdata.outputnr_ = selspec.isNLA() ? selspec.id().asInt() : -1;
    attrdata.nlamodel_ = getNLAModel(is2d);
    attrdata.zdomaininfo_ = zdominfo;

    if ( is2d )
    {
	TypeSet<Pos::GeomID> geomids; geomids += geomid;
	uiAttr2DSelDlg dlg( parent(), adsman->descSet(), geomids,
			    attrdata.nlamodel_ );
	if ( !dlg.go() )
	    return false;

	if ( dlg.getSelType()==0 || dlg.getSelType()==1 )
	{
	    SeisIOObjInfo info( dlg.getStoredAttrName() );
	    attrdata.attribid_ = adsman->descSet()->getStoredID(
					info.ioObj() ? info.ioObj()->key() : 0,
					dlg.getComponent(), true );
	}
	else
	    attrdata.attribid_.asInt() = dlg.getSelDescID().asInt();

    }
    else
    {
	uiAttrSelDlg::Setup setup( seltxt );
	setup.showsteeringdata(true);
	uiAttrSelDlg dlg( parent(), attrdata, setup );
	if ( !dlg.go() )
	    return false;

	attrdata.attribid_.asInt() = dlg.attribID().asInt();
	attrdata.outputnr_ = dlg.outputNr();
	attrdata.setAttrSet( &dlg.getAttrSet() );
    }

    const bool isnla = !attrdata.attribid_.isValid() && attrdata.outputnr_ >= 0;
    const Desc* desc = attrdata.attrSet().getDesc( attrdata.attribid_ );
    const bool isstored = desc && desc->isStored();
    BufferString objref;
    if ( isnla )
	objref = nlaname_;
    else if ( !isstored )
    {
	PtrMan<IOObj> ioobj = IOM().get( adsman->attrsetid_ );
	objref = ioobj ? ioobj->name().buf() : "";
	attrdata.attribid_.setStored( false );
    }

    selspec.set( 0, isnla ? DescID(attrdata.outputnr_,isstored)
			  : attrdata.attribid_, isnla, objref );
    if ( isnla && attrdata.nlamodel_ )
	selspec.setRefFromID( *attrdata.nlamodel_ );
    else if ( !isnla )
	selspec.setRefFromID( attrdata.attrSet() );
    //selspec.setZDomainKey( dlg.zDomainKey() );

    return true;
}


void uiAttribPartServer::directShowAttr( CallBacker* cb )
{
    mDynamicCastGet(uiAttribDescSetEd*,ed,cb);
    if ( !ed ) { pErrMsg("cb is not uiAttribDescSetEd*"); return; }

    dirshwattrdesc_ = ed->curDesc();
    DescSetMan* kpman = eDSHolder().getDescSetMan( ed->is2D() );
    DescSet* edads = const_cast<DescSet*>(dirshwattrdesc_->descSet());
    PtrMan<DescSetMan> tmpadsman = new DescSetMan( ed->is2D(), edads, false );
    eDSHolder().replaceADSMan(tmpadsman);
    sendEvent( evDirectShowAttr() );
    eDSHolder().replaceADSMan(kpman);
}


void uiAttribPartServer::updateSelSpec( SelSpec& ss ) const
{
    bool is2d = ss.is2D();
    if ( ss.isNLA() )
    {
	const NLAModel* nlamod = getNLAModel( is2d );
	if ( nlamod )
	{
	    ss.setIDFromRef( *nlamod );
	    ss.setObjectRef( nlaname_ );
	}
	else
	    ss.set( ss.userRef(), SelSpec::cNoAttrib(), true, 0 );
    }
    else
    {
	if ( is2d ) return;
	bool isstored = ss.isStored();
	const bool isother = ss.id().asInt() == SelSpec::cOtherAttrib().asInt();
	const DescSet* ads = DSHolder().getDescSet( false, isstored );
	ss.setIDFromRef( *ads );

	const bool notfound = ss.id() == DescID( -1, false );
	if ( isother && notfound )	//Could be multi-components stored cube
	{
	    ss.setIDFromRef( *DSHolder().getDescSet( false, true ) );
	    isstored = ss.isStored();
	}

	if ( !isstored )
	{
	    IOObj* ioobj = IOM().get(
				DSHolder().getDescSetMan(false)->attrsetid_ );
	    if ( ioobj ) ss.setObjectRef( ioobj->name() );
	}
    }
}


void uiAttribPartServer::getPossibleOutputs( bool is2d,
					     BufferStringSet& nms ) const
{
    nms.erase();
    SelInfo attrinf( curDescSet( is2d ), 0, is2d );
    for ( int idx=0; idx<attrinf.attrnms_.size(); idx++ )
	nms.add( attrinf.attrnms_.get(idx) );

    BufferStringSet storedoutputs;
    for ( int idx=0; idx<attrinf.ioobjids_.size(); idx++ )
    {
	const char* ioobjid = attrinf.ioobjids_.get( idx );
	uiSeisIOObjInfo* sii = new uiSeisIOObjInfo( MultiID(ioobjid), false );
	sii->getDefKeys( storedoutputs, true );
	delete sii;
    }

    for ( int idx=0; idx<storedoutputs.size(); idx++ )
	nms.add( storedoutputs.get(idx) );
}


bool uiAttribPartServer::setSaved( bool is2d ) const
{
    return DSHolder().getDescSetMan( is2d )->isSaved();
}


int uiAttribPartServer::use3DMode() const
{
    const Attrib::DescSet* ads = getUserPrefDescSet();
    if ( !ads ) return -1;
    return DSHolder().getDescSetMan(true)
		&& ads==DSHolder().getDescSet(true,false) ? 0 : 1;
}


const Attrib::DescSet* uiAttribPartServer::getUserPrefDescSet() const
{
    const Attrib::DescSet* ds3d = DSHolder().getDescSet( false, false );
    const Attrib::DescSet* ds2d = DSHolder().getDescSet( true, false );
    if ( !ds3d && !ds2d ) return 0;
    if ( !(ds3d && ds2d) ) return ds3d ? ds3d : ds2d;
    if ( !SI().has3D() ) return ds2d;
    if ( !SI().has2D() ) return ds3d;

    const int nr3d = ds3d->nrDescs( false, true );
    const int nr2d = ds2d->nrDescs( false, true );
    if ( (nr3d>0) != (nr2d>0) ) return nr2d > 0 ? ds2d : ds3d;

    int res = uiMSG().askGoOnAfter( tr("Which attributes do you want to use?"),
				    0, uiStrings::s2D(), uiStrings::s3D() );
    if ( res == 2 ) return 0;
    return res == 0 ? ds2d : ds3d;
}


void uiAttribPartServer::saveSet( bool is2d )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(AttribDescSet);
    ctio->ctxt.forread = false;
    uiIOObjSelDlg dlg( parent(), *ctio );
    if ( dlg.go() && dlg.ioObj() )
    {
	ctio->ioobj = 0;
	ctio->setObj( dlg.ioObj()->clone() );
	BufferString bs;
	if ( !ctio->ioobj )
	    uiMSG().error(tr("Cannot find attribute set in data base"));
	else if (
	    !AttribDescSetTranslator::store(*DSHolder().getDescSet(is2d,false),
					      ctio->ioobj,bs) )
	    uiMSG().error(bs);
    }
    ctio->setObj( 0 );
}


void uiAttribPartServer::outputVol( const MultiID& nlaid, bool is2d,
				    bool multiattrib )
{
    const DescSet* dset = DSHolder().getDescSet( is2d, false );
    if ( !dset ) { pErrMsg("No attr set"); return; }

    uiAttrVolOut dlg( parent(), *dset, multiattrib, getNLAModel(is2d), nlaid );
    dlg.go();
}


void uiAttribPartServer::setTargetSelSpec( const SelSpec& selspec )
{
    targetspecs_.erase();
    targetspecs_ += selspec;
}


Attrib::DescID uiAttribPartServer::targetID( bool for2d, int nr ) const
{
    return targetspecs_.size() <= nr ? Attrib::DescID::undef()
				     : targetspecs_[nr].id();
}


EngineMan* uiAttribPartServer::createEngMan( const CubeSampling* cs,
					     const char* linekey )
{
    if ( targetspecs_.isEmpty() ||
	 targetspecs_[0].id().asInt() == SelSpec::cNoAttrib().asInt())
	{ pErrMsg("Nothing to do"); return 0; }

    const bool istargetstored = targetspecs_[0].isStored();
    const bool is2d = targetspecs_[0].is2D();
    Attrib::DescSet* curdescset = eDSHolder().getDescSet(is2d,istargetstored);
    if ( !curdescset )
	{ pErrMsg("No attr set"); return 0; }

    if ( !istargetstored )
    {
	DescID attribid = targetspecs_[0].id();
	Desc* seldesc = curdescset->getDesc( attribid );
	if ( seldesc )
	{
	    DescID multoiid = seldesc->getMultiOutputInputID();
	    if ( multoiid != DescID::undef() )
	    {
		const DescSetMan* adsman = DSHolder().getDescSetMan( is2d );
		uiAttrSelData attrdata( *adsman->descSet() );
		SelInfo attrinf( &attrdata.attrSet(), attrdata.nlamodel_, is2d,
				 DescID::undef(), false, false );
		if ( !uiMultOutSel::handleMultiCompChain( attribid, multoiid,
			    is2d, attrinf, curdescset, parent(), targetspecs_ ))
		    return 0;
	    }
	}
    }

    EngineMan* aem = new EngineMan;
    aem->setAttribSet( curdescset );
    aem->setNLAModel( getNLAModel(is2d) );
    aem->setAttribSpecs( targetspecs_ );
    if ( cs )
	aem->setCubeSampling( *cs );
    if ( linekey )
	aem->setLineKey( linekey );

    return aem;
}


DataPack::ID uiAttribPartServer::createOutput( const CubeSampling& cs,
					       DataPack::ID cacheid )
{
    DataPackRef<DataPack> datapack =
	DPM( DataPackMgr::FlatID() ).obtain( cacheid );
    if ( !datapack )
	datapack = DPM( DataPackMgr::CubeID() ).obtain( cacheid );

    const Attrib::DataCubes* cache = 0;
    mDynamicCastGet(const Attrib::CubeDataPack*,cdp,datapack.ptr());
    if ( cdp ) cache = &cdp->cube();
    mDynamicCastGet(const Attrib::Flat3DDataPack*,fdp,datapack.ptr());
    if ( fdp ) cache = &fdp->cube();
    const DataCubes* output = createOutput( cs, cache );
    if ( !output || !output->nrCubes() )  return DataPack::cNoID();

    const bool isflat = cs.isFlat();
    DataPack* newpack;
    if ( isflat )
	newpack = new Attrib::Flat3DDataPack( targetID(false), *output, 0 );
    else
	newpack = new Attrib::CubeDataPack( targetID(false), *output, 0 );
    newpack->setName( targetspecs_[0].userRef() );

    DataPackMgr& dpman = DPM( isflat ? DataPackMgr::FlatID()
				     : DataPackMgr::CubeID() );
    dpman.add( newpack );
    return newpack->id();
}


const Attrib::DataCubes* uiAttribPartServer::createOutput(
				const CubeSampling& cs, const DataCubes* cache )
{
    PtrMan<EngineMan> aem = createEngMan( &cs, 0 );
    if ( !aem ) return 0;

    BufferString defstr;
    const bool isstortarget = targetspecs_.size() && targetspecs_[0].isStored();
    const DescSet* attrds = DSHolder().getDescSet( false, isstortarget );
    const Desc* targetdesc = !attrds || attrds->isEmpty() ? 0
			   : attrds->getDesc( targetspecs_[0].id() );
    if ( targetdesc )
    {
	attrds->getDesc(targetspecs_[0].id())->getDefStr(defstr);
	if ( defstr != targetspecs_[0].defString() )
	    cache = 0;
    }

    uiString errmsg;
    Processor* process = aem->createDataCubesOutput( errmsg, cache );
    if ( !process )
	{ uiMSG().error(errmsg); return 0; }
    bool showinlprogress = true;
    bool showcrlprogress = true;
    Settings::common().getYN( "dTect.Show inl progress", showinlprogress );
    Settings::common().getYN( "dTect.Show crl progress", showcrlprogress );

    const bool isstored =
	targetdesc && targetdesc->isStored() && !targetspecs_[0].isNLA();
    const bool isinl = cs.isFlat() && cs.defaultDir() == CubeSampling::Inl;
    const bool iscrl = cs.isFlat() && cs.defaultDir() == CubeSampling::Crl;
    const bool hideprogress = isstored &&
	( (isinl&&!showinlprogress) || (iscrl&&!showcrlprogress) );

    bool success = true;
    if ( aem->getNrOutputsToBeProcessed(*process) != 0 )
    {
	if ( !hideprogress )
	{
	    uiTaskRunner taskrunner( parent() );
	    success = TaskRunner::execute( &taskrunner, *process );
	}
	else
	{
	    MouseCursorChanger cursorchgr( MouseCursor::Wait );
	    if ( !process->execute() )
	    {
		const uiString msg( process->uiMessage() );
		if ( !msg.isEmpty() )
		    uiMSG().error( msg );
		delete process;
		return 0;
	    }
	}
    }

    const DataCubes* output = aem->getDataCubesOutput( *process );
    if ( !output )
    {
	delete process;
	return 0;
    }
    output->ref();
    delete process;

    if ( !success )
    {
	if ( !uiMSG().askGoOn(tr("Attribute loading/calculation aborted.\n"
	    "Do you want to use the partially loaded/computed data?"), true ) )
	{
	    output->unRef();
	    output = 0;
	}
    }

    if ( output )
	output->unRefNoDelete();

    return output;
}


bool uiAttribPartServer::createOutput( DataPointSet& posvals, int firstcol )
{
    PtrMan<EngineMan> aem = createEngMan();
    if ( !aem ) return false;

    uiString errmsg;
    PtrMan<Processor> process =
			aem->getTableOutExecutor( posvals, errmsg, firstcol );
    if ( !process )
	{ uiMSG().error(errmsg); return false; }

    uiTaskRunner taskrunner( parent() );
    if ( !TaskRunner::execute( &taskrunner, *process ) ) return false;

    posvals.setName( targetspecs_[0].userRef() );
    return true;
}


bool uiAttribPartServer::createOutput( ObjectSet<DataPointSet>& dpss,
				       int firstcol )
{
    ExecutorGroup execgrp( "Calculating Attribute", true );
    uiString errmsg;

    ObjectSet<EngineMan> aems;
    for ( int idx=0; idx<dpss.size(); idx++ )
    {
	EngineMan* aem = createEngMan();
	if ( !aem ) continue;

	execgrp.add( aem->getTableOutExecutor(*dpss[idx],errmsg,firstcol) );
	aems += aem;
    }

    bool res = true;
    uiTaskRunner taskrunner( parent() );
    res = TaskRunner::execute( &taskrunner, execgrp );

    deepErase( aems );
    return res;
}


DataPack::ID uiAttribPartServer::createRdmTrcsOutput(
				const Interval<float>& zrg,
				TypeSet<BinID>* path,
				TypeSet<BinID>* trueknotspos )
{
    BinIDValueSet bidset( 2, false );
    for ( int idx=0; idx<path->size(); idx++ )
	bidset.add( (*path)[idx], zrg.start, zrg.stop );

    SeisTrcBuf output( true );
    if ( !createOutput( bidset, output, trueknotspos, path ) )
	return -1;

    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );
    DataPack* newpack = new Attrib::FlatRdmTrcsDataPack( targetID(false),
							 output, path );
    newpack->setName( targetspecs_[0].userRef() );
    dpman.add( newpack );
    return newpack->id();
}


bool uiAttribPartServer::createOutput( const BinIDValueSet& bidset,
				       SeisTrcBuf& output,
				       TypeSet<BinID>* trueknotspos,
				       TypeSet<BinID>* snappedpos )
{
    PtrMan<EngineMan> aem = createEngMan();
    if ( !aem ) return 0;

    uiString errmsg;
    PtrMan<Processor> process = aem->createTrcSelOutput( errmsg, bidset,
							 output, mUdf(float), 0,
							 trueknotspos,
							 snappedpos );
    if ( !process )
	{ uiMSG().error(errmsg); return false; }

    uiTaskRunner taskrunner( parent() );
    if ( !TaskRunner::execute( &taskrunner, *process ) ) return false;

    return true;
}


DataPack::ID uiAttribPartServer::create2DOutput( const CubeSampling& cs,
						 const LineKey& linekey,
						 TaskRunner& taskrunner )
{
    PtrMan<EngineMan> aem = createEngMan( &cs, linekey );
    if ( !aem ) return -1;

    uiString errmsg;
    RefMan<Data2DHolder> data2d = new Data2DHolder;
    PtrMan<Processor> process = aem->createScreenOutput2D( errmsg, *data2d );
    if ( !process )
	{ uiMSG().error(errmsg); return -1; }

    if ( !TaskRunner::execute( &taskrunner, *process ) )
	return -1;

    int component = 0;
    const bool isstored = targetspecs_[0].isStored();
    Attrib::DescID adid = targetID(true);
    const Attrib::DescSet* curds =
			DSHolder().getDescSet( true, isstored );
    if ( curds )
    {
	const Attrib::Desc* targetdesc = curds->getDesc( adid );
	if ( targetdesc )
	    component = targetdesc->selectedOutput();
    }

    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );
    mDeclareAndTryAlloc( Flat2DDHDataPack*, newpack,
	Attrib::Flat2DDHDataPack( adid, *data2d, false, component ) );
    if ( !newpack || !newpack->isOK() )
	return DataPack::cNoID();

    newpack->setName( linekey.attrName() );
    newpack->setLineName( linekey.lineName() );
    dpman.add( newpack );
    return newpack->id();
}


bool uiAttribPartServer::isDataClassified( const Array3D<float>& array ) const
{
    const int sz0 = array.info().getSize( 0 );
    const int sz1 = array.info().getSize( 1 );
    const int sz2 = array.info().getSize( 2 );
//    int nrint = 0;
    for ( int x0=0; x0<sz0; x0++ )
	for ( int x1=0; x1<sz1; x1++ )
	    for ( int x2=0; x2<sz2; x2++ )
	    {
		const float val = array.get( x0, x1, x2 );
		if ( mIsUdf(val) ) continue;
		const int ival = mNINT32(val);
		if ( !mIsEqual(val,ival,mDefEps) || abs(ival)>cMaxNrClasses )
		    return false;
//		nrint++;
//		if ( nrint > sMaxNrVals )
//		    break;
	    }

    return true;
}


bool uiAttribPartServer::extractData( ObjectSet<DataPointSet>& dpss )
{
    if ( dpss.isEmpty() ) { pErrMsg("No inp data"); return 0; }
    const DescSet* ads = DSHolder().getDescSet( dpss[0]->is2D(), false );
    if ( !ads ) { pErrMsg("No attr set"); return 0; }

    Attrib::EngineMan aem;
    uiTaskRunner taskrunner( parent() );
    bool somesuccess = false;
    bool somefail = false;

    for ( int idx=0; idx<dpss.size(); idx++ )
    {
	uiString err;
	DataPointSet& dps = *dpss[idx];
	Executor* tabextr = aem.getTableExtractor( dps, *ads, err );
	if ( !tabextr ) { pErrMsg(err.getFullString()); return 0; }

	if ( TaskRunner::execute( &taskrunner, *tabextr ) )
	    somesuccess = true;
	else
	    somefail = true;

	delete tabextr;
    }

    if ( somefail )
    {
	return somesuccess &&
	     uiMSG().askGoOn(
	      tr("Some data extraction failed. Do you want to continue and use"
	      " the (partially) extracted data?"), true);
    }

    return true;
}


Attrib::DescID uiAttribPartServer::getStoredID( const LineKey& lkey, bool is2d,
						int selout )
{
    DescSet* ds = eDSHolder().getDescSet( is2d, true );
    return ds ? ds->getStoredID( lkey, selout ) : DescID::undef();
}


bool uiAttribPartServer::createAttributeSet( const BufferStringSet& inps,
					     DescSet* attrset )
{
    AttributeSetCreator attrsetcr( parent(), inps, attrset );
    return attrsetcr.create();
}


void uiAttribPartServer::importAttrSetFromFile()
{
    uiImpAttrSet dlg( parent() );
    dlg.go();
}


void uiAttribPartServer::importAttrSetFromOtherSurvey()
{
    uiMSG().message( tr("Not implemented yet") );
}


bool uiAttribPartServer::setPickSetDirs( Pick::Set& ps, const NLAModel* nlamod,
					 float velocity )
{
    //TODO: force 3D to avoid crash for 2D, need workaround for 2D later
    const Attrib::DescSet* ds = DSHolder().getDescSet( false, false );
    if ( !ds )
	return false;

    uiSetPickDirs dlg( parent(), ps, ds, nlamod, velocity );
    return dlg.go();
}


#define mInsertItems(list,mnu,correcttype) \
(mnu)->removeItems(); \
(mnu)->enabled = attrinf.list.size(); \
for ( int idx=start; idx<stop; idx++ ) \
{ \
    const BufferString& nm = attrinf.list.get(idx); \
    MenuItem* itm = new MenuItem( nm ); \
    itm->checkable = true; \
    const bool docheck = correcttype && nm == as.userRef(); \
    mAddMenuItem( mnu, itm, true, docheck );\
    if ( docheck ) (mnu)->checked = true; \
}


MenuItem* uiAttribPartServer::storedAttribMenuItem( const SelSpec& as,
						    bool is2d, bool issteer )
{
    MenuItem* storedmnuitem = is2d ? issteer ? &steering2dmnuitem_
					     : &stored2dmnuitem_
				   : issteer ? &steering3dmnuitem_
					     : &stored3dmnuitem_;
    fillInStoredAttribMenuItem( storedmnuitem, is2d, issteer, as, false );

    return storedmnuitem;
}


void uiAttribPartServer::fillInStoredAttribMenuItem(
					MenuItem* menu, bool is2d, bool issteer,
					const SelSpec& as, bool multcomp,
					bool needext )
{
    const DescSet* ds = DSHolder().getDescSet( is2d, true );
    SelInfo attrinf( ds, 0, is2d, DescID::undef(), issteer, issteer, multcomp );

    const bool isstored = ds && ds->getDesc( as.id() )
	? ds->getDesc( as.id() )->isStored() : false;
    BufferStringSet bfset = issteer ? attrinf.steerids_ : attrinf.ioobjids_;

    MenuItem* mnu = menu;
    if ( multcomp && needext )
    {
	MenuItem* submnu = is2d ? &multcomp2d_ : &multcomp3d_;
	mAddManagedMenuItem( menu, submnu, true, submnu->checked );
	mnu = submnu;
    }

    int nritems = bfset.size();
    if ( nritems <= cMaxMenuSize )
    {
	    const int start = 0; const int stop = nritems;
	    if ( issteer )
	    { mInsertItems(steernms_,mnu,isstored); }
	    else
	    { mInsertItems(ioobjnms_,mnu,isstored); }
	}

    menu->text = getMenuText( is2d, issteer, nritems>cMaxMenuSize );
}



void uiAttribPartServer::insertNumerousItems( const BufferStringSet& bfset,
					      const SelSpec& as,
					      bool correcttype, bool is2d,
					      bool issteer )
{
    int nritems = bfset.size();
    const int nrsubmnus = (nritems-1)/cMaxMenuSize + 1;
    for ( int mnuidx=0; mnuidx<nrsubmnus; mnuidx++ )
    {
	const int start = mnuidx * cMaxMenuSize;
	int stop = (mnuidx+1) * cMaxMenuSize;
	if ( stop > nritems ) stop = nritems;
	BufferString startnm = bfset.get(start);
	if ( startnm.size() > 3 ) startnm[3] = '\0';
	BufferString stopnm = bfset.get(stop-1);
	if ( stopnm.size() > 3 ) stopnm[3] = '\0';
	MenuItem* submnu = new MenuItem( BufferString(startnm," - ",stopnm) );

	    SelInfo attrinf( DSHolder().getDescSet(false,true), 0, false,
			     DescID::undef() );
	    if ( issteer )
	    { mInsertItems(steernms_,submnu,correcttype); }
	    else
	    { mInsertItems(ioobjnms_,submnu,correcttype); }

	MenuItem* storedmnuitem = is2d ? issteer ? &steering2dmnuitem_
						 : &stored2dmnuitem_
				       : issteer ? &steering3dmnuitem_
						 : &stored3dmnuitem_;
	mAddManagedMenuItem( storedmnuitem, submnu, true,submnu->checked);
    }
}


MenuItem* uiAttribPartServer::calcAttribMenuItem( const SelSpec& as,
						  bool is2d, bool useext )
{
    SelInfo attrinf( DSHolder().getDescSet(is2d,false), 0, is2d );
    const bool isattrib = attrinf.attrids_.isPresent( as.id() );

    const int start = 0; const int stop = attrinf.attrnms_.size();
    MenuItem* calcmnuitem = is2d ? &calc2dmnuitem_ : &calc3dmnuitem_;
    uiString txt = useext ? ( is2d ? uiStrings::sAttributes2D()
                                   : uiStrings::sAttributes3D() )
			           : uiStrings::sAttributes();
    calcmnuitem->text = txt;
    mInsertItems(attrnms_,calcmnuitem,isattrib);

    calcmnuitem->enabled = calcmnuitem->nrItems();
    return calcmnuitem;
}


MenuItem* uiAttribPartServer::nlaAttribMenuItem( const SelSpec& as, bool is2d,
						 bool useext )
{
    const NLAModel* nlamodel = getNLAModel(is2d);
    MenuItem* nlamnuitem = is2d ? &nla2dmnuitem_ : &nla3dmnuitem_;
    if ( nlamodel )
    {
	BufferString ittxt;
	if ( !useext || is2d )
        { ittxt = "&"; ittxt += nlamodel->nlaType(false); }
	else
	    ittxt = "N&eural Network 3D";
	if ( useext && is2d ) ittxt += " 2D";

	nlamnuitem->text = ittxt;
	const DescSet* dset = DSHolder().getDescSet(is2d,false);
	SelInfo attrinf( dset, nlamodel, is2d );
	const bool isnla = as.isNLA();
	const int start = 0; const int stop = attrinf.nlaoutnms_.size();
	mInsertItems(nlaoutnms_,nlamnuitem,isnla);
    }

    nlamnuitem->enabled = nlamnuitem->nrItems();
    return nlamnuitem;
}


MenuItem* uiAttribPartServer::zDomainAttribMenuItem( const SelSpec& as,
						     const ZDomain::Info& zdinf,
						     bool is2d, bool useext)
{
    MenuItem* zdomainmnuitem = is2d ? &zdomain2dmnuitem_
				    : &zdomain3dmnuitem_;
    BufferString itmtxt = zdinf.userName();
    itmtxt += useext ? (!is2d ? " Cubes" : " 2D Lines") : " Data";
    zdomainmnuitem->text = itmtxt;
    zdomainmnuitem->removeItems();
    zdomainmnuitem->checkable = true;
    zdomainmnuitem->checked = false;

    BufferStringSet ioobjnms;
    SelInfo::getZDomainItems( zdinf, ioobjnms );
    for ( int idx=0; idx<ioobjnms.size(); idx++ )
    {
	const BufferString& nm = ioobjnms.get( idx );
	MenuItem* itm = new MenuItem( nm );
	const bool docheck = nm == as.userRef();
	mAddManagedMenuItem( zdomainmnuitem, itm, true, docheck );
	if ( docheck ) zdomainmnuitem->checked = true;
    }

    zdomainmnuitem->enabled = zdomainmnuitem->nrItems();
    return zdomainmnuitem;
}


bool uiAttribPartServer::handleAttribSubMenu( int mnuid, SelSpec& as,
					      bool& dousemulticomp )
{
    if ( stored3dmnuitem_.id == mnuid )
	return selectAttrib( as, 0, Survey::GM().cUndefGeomID(),
			     "Select Attribute" );

    const bool is3d = stored3dmnuitem_.findItem(mnuid) ||
		      calc3dmnuitem_.findItem(mnuid) ||
		      nla3dmnuitem_.findItem(mnuid) ||
		      zdomain3dmnuitem_.findItem(mnuid) ||
		      steering3dmnuitem_.findItem(mnuid);
    //look at 3D =trick: extra menus available in 2D cannot be reached from here
    const bool is2d = !is3d;

    const bool issteering = steering2dmnuitem_.findItem(mnuid) ||
			    steering3dmnuitem_.findItem(mnuid);
    const DescSetMan* adsman = DSHolder().getDescSetMan( is2d );
    uiAttrSelData attrdata( *adsman->descSet() );
    attrdata.nlamodel_ = getNLAModel(is2d);
    SelInfo attrinf( &attrdata.attrSet(), attrdata.nlamodel_, is2d,
		     DescID::undef(), issteering, issteering );
    const MenuItem* calcmnuitem = is2d ? &calc2dmnuitem_ : &calc3dmnuitem_;
    const MenuItem* nlamnuitem = is2d ? &nla2dmnuitem_ : &nla3dmnuitem_;
    const MenuItem* zdomainmnuitem = is2d ? &zdomain2dmnuitem_
					      : &zdomain3dmnuitem_;

    DescID attribid = SelSpec::cAttribNotSel();
    int outputnr = -1;
    bool isnla = false;
    bool isstored = false;
    LineKey idlkey;

    if ( stored3dmnuitem_.findItem(mnuid) )
    {
	const MenuItem* item = stored3dmnuitem_.findItem(mnuid);
	const int idx = attrinf.ioobjnms_.indexOf(item->text.getFullString());
	const char* objidstr = attrinf.ioobjids_.get(idx);
	attribid = eDSHolder().getDescSet(false,true)->getStoredID( objidstr );
	idlkey = LineKey( objidstr );
	isstored = true;
    }
    else if ( steering3dmnuitem_.findItem(mnuid) )
    {
	const MenuItem* item = steering3dmnuitem_.findItem( mnuid );
	const int idx = attrinf.steernms_.indexOf( item->text.getFullString() );
	const char* objidstr = attrinf.steerids_.get( idx );
	attribid = eDSHolder().getDescSet(false,true)->getStoredID( objidstr );
	idlkey = LineKey( objidstr );
	isstored = true;
    }
    else if ( stored2dmnuitem_.findItem(mnuid) ||
	      steering2dmnuitem_.findItem(mnuid) )
    {
	    const MenuItem* item = stored2dmnuitem_.findItem(mnuid);
	    if ( !item ) item = steering2dmnuitem_.findItem(mnuid);
	if ( !item ) return false;

	const BufferString& itmnm = item->text.getFullString();
	const int idx = issteering ? attrinf.steernms_.indexOf( itmnm )
				   : attrinf.ioobjnms_.indexOf( itmnm );
	idlkey = LineKey( issteering ? attrinf.steerids_.get(idx)
				     : attrinf.ioobjids_.get(idx) );
	const int selout = issteering ? 1 : -1;
	attribid =
	    eDSHolder().getDescSet(true,true)->getStoredID( idlkey, selout );
	    isstored = true;
	}
    else if ( calcmnuitem->findItem(mnuid) )
    {
	const MenuItem* item = calcmnuitem->findItem(mnuid);
	int idx = attrinf.attrnms_.indexOf(item->text.getFullString());
	attribid = attrinf.attrids_[idx];
    }
    else if ( nlamnuitem->findItem(mnuid) )
    {
	outputnr = nlamnuitem->itemIndex(nlamnuitem->findItem(mnuid));
	isnla = true;
    }
    else if ( zdomainmnuitem->findItem(mnuid) )
    {
	if ( is2d )
	    return false;
	const MenuItem* item = zdomainmnuitem->findItem( mnuid );
	IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id));
	PtrMan<IOObj> ioobj = IOM().getLocal( item->text.getFullString(), 0 );
	if ( ioobj )
	{
	    attribid = eDSHolder().getDescSet( false, true )
					->getStoredID( ioobj->key() );
	    idlkey = LineKey( ioobj->key() );
	    isstored = true;
	}
    }
    else
	return false;

    const bool nocompsel = is2d && issteering;
    if ( isstored && !nocompsel )
    {
	BufferStringSet complist;
	SeisIOObjInfo::getCompNames( idlkey.buf(), complist );
	if ( complist.size()>1 )
	{
	    TypeSet<int> selcomps;
	    if ( !handleMultiComp( idlkey, is2d, issteering, complist,
				   attribid, selcomps ) )
		return false;

	    dousemulticomp = selcomps.size()>1;
	    if ( dousemulticomp )
		return true;
	}
    }


    BufferString objref;
    if ( isnla )
	objref = nlaname_;
    else if ( !isstored )
    {
	PtrMan<IOObj> ioobj = IOM().get( adsman->attrsetid_ );
	objref = ioobj ? ioobj->name().buf() : "";
    }

    DescID did = isnla ? DescID(outputnr,false) : attribid;
    as.set( 0, did, isnla, objref );

    BufferString bfs;
    if ( attribid.asInt() != SelSpec::cAttribNotSel().asInt() )
    {
	const Attrib::DescSet* attrset = DSHolder().getDescSet(is2d, isstored);
	const Attrib::Desc* desc = attrset ? attrset->getDesc( attribid ) : 0;
	if ( desc  )
	{
	    desc->getDefStr( bfs );
	    as.setDefString( bfs.buf() );
	}
    }

    if ( isnla )
	as.setRefFromID( *attrdata.nlamodel_ );
    else
	as.setRefFromID( *DSHolder().getDescSet(is2d, isstored) );

    as.set2DFlag( is2d );

    return true;
}


void uiAttribPartServer::info2DAttribSubMenu( int mnuid, BufferString& attbnm,
					      bool& steering, bool& stored )
{
    steering = steering2dmnuitem_.findItem(mnuid);
    stored = false;

    if ( stored2dmnuitem_.findItem(mnuid) || steering2dmnuitem_.findItem(mnuid))
    {
	stored = true;
	const MenuItem* item = stored2dmnuitem_.findItem( mnuid );
	if ( !item ) item = steering2dmnuitem_.findItem( mnuid );
	attbnm = item->text.getFullString();
    }
    else if ( calc2dmnuitem_.findItem(mnuid) )
    {
	const MenuItem* item = calc2dmnuitem_.findItem( mnuid );
	attbnm = item->text.getFullString();
    }
}


#define mFakeCompName( searchfor, replaceby ) \
{ \
    LineKey lkey( desc->userRef() ); \
    if ( lkey.attrName() == searchfor ) \
	lkey.setAttrName( replaceby );\
    desc->setUserRef( lkey.buf() ); \
}

bool uiAttribPartServer::handleMultiComp( const LineKey& idlkey, bool is2d,
					  bool issteering,
					  BufferStringSet& complist,
					  DescID& attribid,
					  TypeSet<int>& selectedcomps )
{
    //Trick for old steering cubes: fake good component names
    if ( !is2d && issteering && complist.isPresent("Component 1") )
    {
	complist.erase();
	complist.add( "Inline Dip" );
	complist.add( "Crossline Dip" );
    }

    uiMultCompDlg compdlg( parent(), complist );
    if ( compdlg.go() )
    {
	selectedcomps.erase();
	compdlg.getCompNrs( selectedcomps );
	if ( !selectedcomps.size() ) return false;

	DescSet* ads = eDSHolder().getDescSet( is2d, true );
	if ( selectedcomps.size() == 1 )
	{
	    //Using const_cast for compiler but ads won't be modified anyway
	    attribid = const_cast<DescSet*>(ads)
			->getStoredID( idlkey, selectedcomps[0], false );
	    //Trick for old steering cubes: fake good component names
	    if ( !is2d && issteering )
	    {
		Attrib::Desc* desc = ads->getDesc(attribid);
		if ( !desc ) return false;
		mFakeCompName( "Component 1", "Inline Dip" );
		mFakeCompName( "Component 2", "Crossline Dip" );
	    }

	    return true;
	}
	prepMultCompSpecs( selectedcomps, idlkey, is2d, issteering );
    }
    else
	return false;

    return true;
}


bool uiAttribPartServer::prepMultCompSpecs( TypeSet<int> selectedcomps,
					    const LineKey& idlkey, bool is2d,
					    bool issteering )
{
    targetspecs_.erase();
    DescSet* ads = eDSHolder().getDescSet( is2d, true );
    for ( int idx=0; idx<selectedcomps.size(); idx++ )
    {
	DescID did = ads->getStoredID( idlkey, selectedcomps[idx], true );
	SelSpec as( 0, did );
	BufferString bfs;
	Attrib::Desc* desc = ads->getDesc(did);
	if ( !desc ) return false;

	desc->getDefStr(bfs);
	as.setDefString(bfs.buf());
	//Trick for old steering cubes: fake good component names
	if ( !is2d && issteering )
	{
	    mFakeCompName( "Component 1", "Inline Dip" );
	    mFakeCompName( "Component 2", "Crossline Dip" );
	}

	//Trick for PreStack offsets displayed on the fly
	if ( desc->isStored() && desc->userRef()[0] == '{' )
	{
	    LineKey lkey( desc->userRef() );
            BufferString newnm = "offset index "; newnm += selectedcomps[idx];
	    lkey.setAttrName( newnm );
	    desc->setUserRef( lkey.buf() );
	}

	as.setRefFromID( *ads );
	as.set2DFlag( is2d );
	targetspecs_ += as;
    }
    set2DEvent( is2d );
    return true;
}


IOObj* uiAttribPartServer::getIOObj( const Attrib::SelSpec& as ) const
{
    if ( as.isNLA() ) return 0;

    const Attrib::DescSet* attrset = DSHolder().getDescSet( as.is2D(), true );
    const Attrib::Desc* desc = attrset ? attrset->getDesc( as.id() ) : 0;
    if ( !desc )
    {
        attrset = DSHolder().getDescSet( as.is2D(), false );
        desc = attrset ? attrset->getDesc( as.id() ) : 0;
        if ( !desc )
            return 0;
    }

    BufferString storedid = desc->getStoredID();
    if ( !desc->isStored() || storedid.isEmpty() ) return 0;

    return IOM().get( MultiID(storedid.buf()) );
}


#define mErrRet(msg) { uiMSG().error(msg); return; }

void uiAttribPartServer::processEvalDlg( bool iscrossevaluate )
{
    if ( !attrsetdlg_ ) return;
    const Desc* curdesc = attrsetdlg_->curDesc();
    if ( !curdesc )
        mErrRet( tr("Please add this attribute first") );

    uiAttrDescEd* ade = attrsetdlg_->curDescEd();
    if ( !ade ) return;

    sendEvent( evEvalAttrInit() );
    //if ( !alloweval_ ) mErrRet( "Evaluation of attributes only possible on\n"
//			       "Inlines, Crosslines, Timeslices and Surfaces.");

    if ( !iscrossevaluate )
    {
	uiEvaluateDlg* evaldlg =
	    new uiEvaluateDlg( attrsetdlg_, *ade, allowevalstor_ );

	if ( !evaldlg->evaluationPossible() )
	    mErrRet( tr("This attribute has no parameters to evaluate") );

	evaldlg->calccb.notify( mCB(this,uiAttribPartServer,calcEvalAttrs) );
	evaldlg->showslicecb.notify( mCB(this,uiAttribPartServer,showSliceCB) );
	evaldlg->windowClosed.notify(
		mCB(this,uiAttribPartServer,evalDlgClosed) );
	evaldlg->go();
    }
    else
    {
	uiCrossAttrEvaluateDlg* crossevaldlg =
	    new uiCrossAttrEvaluateDlg(attrsetdlg_,*attrsetdlg_,allowevalstor_);
	crossevaldlg->calccb.notify(
		mCB(this,uiAttribPartServer,calcEvalAttrs) );
	crossevaldlg->showslicecb.notify(
		mCB(this,uiAttribPartServer,showSliceCB) );
	crossevaldlg->windowClosed.notify(
		mCB(this,uiAttribPartServer,evalDlgClosed) );
	crossevaldlg->go();
    }

    attrsetdlg_->setSensitive( false );
}


void uiAttribPartServer::showCrossEvalDlg( CallBacker* )
{ processEvalDlg( true ); }


void uiAttribPartServer::showEvalDlg( CallBacker* cb )
{ processEvalDlg( false ); }


void uiAttribPartServer::evalDlgClosed( CallBacker* cb )
{
    mDynamicCastGet(uiEvaluateDlg*,evaldlg,cb);
    mDynamicCastGet(uiCrossAttrEvaluateDlg*,crossevaldlg,cb);
    if ( !evaldlg && !crossevaldlg )
       return;

    if ( (evaldlg && evaldlg->storeSlices()) ||
	 (crossevaldlg && crossevaldlg->storeSlices()) )
	sendEvent( evEvalStoreSlices() );

    Desc* curdesc = attrsetdlg_->curDesc();
    BufferString curusrref = curdesc->userRef();

    const Desc* evad = evaldlg ? evaldlg->getAttribDesc()
			       : crossevaldlg->getAttribDesc();
    if ( evad )
    {
	BufferString defstr;
	evad->getDefStr( defstr );
	curdesc->parseDefStr( defstr );
	curdesc->setUserRef( curusrref );
	attrsetdlg_->updateCurDescEd();

	if ( crossevaldlg )
	{
	    const TypeSet<Attrib::DescID>& cids =
		crossevaldlg->evaluateChildIds();
	    BufferString ds = crossevaldlg->acceptedDefStr();
	    Attrib::DescSet* ads = attrsetdlg_->getSet();
	    for ( int idx=0; idx<cids.size(); idx++ )
	    {
		Desc* ad = ads->getDesc( cids[idx] );
		if ( ad ) ad->parseDefStr( ds );
	    }
	}
    }

    sendEvent( evEvalRestore() );
    attrsetdlg_->setSensitive( true );
}


void uiAttribPartServer::calcEvalAttrs( CallBacker* cb )
{
    mDynamicCastGet(uiEvaluateDlg*,evaldlg,cb);
    mDynamicCastGet(uiCrossAttrEvaluateDlg*,crossevaldlg,cb);
    if ( !evaldlg && !crossevaldlg )
       return;

    const bool is2d = attrsetdlg_->is2D();
    DescSetMan* kpman = eDSHolder().getDescSetMan( is2d );
    DescSet* ads = evaldlg ? evaldlg->getEvalSet() : crossevaldlg->getEvalSet();
    if ( evaldlg )
	evaldlg->getEvalSpecs( targetspecs_ );
    else
	crossevaldlg->getEvalSpecs( targetspecs_ );

    PtrMan<DescSetMan> tmpadsman = new DescSetMan( is2d, ads, false );
    eDSHolder().replaceADSMan( tmpadsman );
    set2DEvent( is2d );
    sendEvent( evEvalCalcAttr() );
    eDSHolder().replaceADSMan( kpman );
}


void uiAttribPartServer::showSliceCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sel,cb);
    if ( sel < 0 ) return;

    sliceidx_ = sel;
    sendEvent( evEvalShowSlice() );
}


#define mCleanMenuItems(startstr)\
{\
    startstr##mnuitem_.removeItems();\
    startstr##mnuitem_.checked = false;\
}

void uiAttribPartServer::resetMenuItems()
{
    mCleanMenuItems(stored2d)
    mCleanMenuItems(steering2d)
    mCleanMenuItems(calc2d)
    mCleanMenuItems(nla2d)
    mCleanMenuItems(stored3d)
    mCleanMenuItems(steering3d)
    mCleanMenuItems(calc3d)
    mCleanMenuItems(nla3d)
}


void uiAttribPartServer::fillPar( IOPar& iopar, bool is2d, bool isstored ) const
{
    const DescSet* ads = DSHolder().getDescSet( is2d, isstored );
    if ( ads && !ads->isEmpty() )
	ads->fillPar( iopar );
}


void uiAttribPartServer::usePar( const IOPar& iopar, bool is2d, bool isstored )
{
    DescSet* ads = eDSHolder().getDescSet( is2d, isstored );
    if ( ads )
    {
	TypeSet<uiString> errmsgs;
	const int odversion = iopar.odVersion();
	if ( isstored && odversion<411 )	//backward compatibility v<4.1.1
	{
	    DescSet* adsnonstored = eDSHolder().getDescSet( is2d, false );
	    if ( adsnonstored )
	    {
		TypeSet<DescID> allstoredids;
		adsnonstored->getStoredIds( allstoredids );
		ads = adsnonstored->optimizeClone( allstoredids );
		ads->setContainStoredDescOnly( true );
		eDSHolder().replaceStoredAttribSet( ads );
	    }
	}
	else
	    ads->usePar( iopar, &errmsgs );

	if ( !errmsgs.isEmpty() )
	{
	    BufferString basemsg = "Error during restore of ";
	    basemsg += is2d ? "2D " : "3D "; basemsg += "Attribute Set";
	uiMSG().errorWithDetails( errmsgs, basemsg );
	}

	set2DEvent( is2d );
	sendEvent( evNewAttrSet() );
    }
}


void uiAttribPartServer::setEvalBackupColTabMapper(
			const ColTab::MapperSetup* mp )
{
    if ( evalmapperbackup_ && mp )
	*evalmapperbackup_ = *mp;
    else if ( !mp )
    {
	delete evalmapperbackup_;
	evalmapperbackup_ = 0;
    }
    else if ( mp )
    {
	evalmapperbackup_ = new ColTab::MapperSetup( *mp );
    }
}


const ColTab::MapperSetup* uiAttribPartServer::getEvalBackupColTabMapper() const
{ return evalmapperbackup_; }

