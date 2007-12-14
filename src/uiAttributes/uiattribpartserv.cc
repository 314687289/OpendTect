/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiattribpartserv.cc,v 1.81 2007-12-14 05:15:23 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiattribpartserv.h"

#include "attribdatacubes.h"
#include "attribdataholder.h"
#include "attribdatapack.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetman.h"
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
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "executor.h"
#include "iodir.h"
#include "iopar.h"
#include "ioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "nlacrdesc.h"
#include "nlamodel.h"
#include "posvecdataset.h"
#include "ptrman.h"
#include "seisbuf.h"
#include "seisinfo.h"
#include "survinfo.h"
#include "settings.h"

#include "uiattrdesced.h"
#include "uiattrdescseted.h"
#include "uiattrsel.h"
#include "uiattrvolout.h"
#include "uievaluatedlg.h"
#include "uiexecutor.h"
#include "uiioobjsel.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiseisioobjinfo.h"
#include "uisetpickdirs.h"

#include <math.h>

using namespace Attrib;

const int uiAttribPartServer::evDirectShowAttr	 = 0;
const int uiAttribPartServer::evNewAttrSet	 = 1;
const int uiAttribPartServer::evAttrSetDlgClosed = 2;
const int uiAttribPartServer::evEvalAttrInit 	 = 3;
const int uiAttribPartServer::evEvalCalcAttr	 = 4;
const int uiAttribPartServer::evEvalShowSlice	 = 5;
const int uiAttribPartServer::evEvalStoreSlices	 = 6;
const int uiAttribPartServer::objNLAModel2D	 = 100;
const int uiAttribPartServer::objNLAModel3D	 = 101;

const char* uiAttribPartServer::attridstr_ = "Attrib ID";


uiAttribPartServer::uiAttribPartServer( uiApplService& a )
	: uiApplPartServer(a)
    	, adsman2d_(new DescSetMan(true))
    	, adsman3d_(new DescSetMan(false))
	, dirshwattrdesc_(0)
        , attrsetdlg_(0)
    	, attrsetclosetim_("Attrset dialog close")
	, stored2dmnuitem_("&Stored 2D Data")
	, stored3dmnuitem_("Stored &Cubes")
{
    attrsetclosetim_.tick.notify( 
			mCB(this,uiAttribPartServer,attrsetDlgCloseTimTick) );

    stored2dmnuitem_.checkable = true;
    stored3dmnuitem_.checkable = true;
    calc2dmnuitem_.checkable = true;
    calc3dmnuitem_.checkable = true;

    handleAutoSet();
}


uiAttribPartServer::~uiAttribPartServer()
{
    delete adsman2d_;
    delete adsman3d_;
    delete attrsetdlg_;
    deepErase( linesets2dmnuitem_ );
}


#define mUseAutoAttrSet( typ ) \
    MultiID id = \
	SI().pars().find( uiAttribDescSetEd::sKeyAuto##typ##DAttrSetID ); \
    if ( id ) \
    { \
	const bool is2d = typ==2; \
	PtrMan<IOObj> ioobj = IOM().get( id ); \
	BufferString bs; \
	DescSet* attrset = new DescSet( is2d, true ); \
	AttribDescSetTranslator::retrieve( *attrset, ioobj, bs ); \
	adsman##typ##d_->setDescSet( attrset ); \
	adsman##typ##d_->attrsetid_ = id; \
    }


void uiAttribPartServer::handleAutoSet()
{
    bool douse = false;
    Settings::common().getYN( uiAttribDescSetEd::sKeyUseAutoAttrSet, douse );
    if ( douse )
    {
	if ( SI().has2D() )
	{ mUseAutoAttrSet( 2 ); }
	if ( SI().has3D() )
	{ mUseAutoAttrSet( 3 ); }
    }
}


bool uiAttribPartServer::replaceSet( const IOPar& iopar, bool is2d )
{
    DescSet* ads = new DescSet(is2d);
    if ( !ads->usePar(iopar) )
    {
	delete ads;
	return false;
    }

    if ( is2d )
    {
	delete adsman2d_;
	adsman2d_ = new DescSetMan( is2d, ads, true );
    }
    else
    {
	delete adsman3d_;
	adsman3d_ = new DescSetMan( is2d, ads, true );
    }

    DescSetMan* adsman = getAdsMan( is2d );
    adsman->attrsetid_ = "";
    set2DEvent( is2d );
    sendEvent( evNewAttrSet );
    return true;
}


bool uiAttribPartServer::addToDescSet( const char* key, bool is2d )
{
    DescID id = getAdsMan( is2d )->descSet()->getStoredID( key );
    return id < 0 ? false : true;
}


const DescSet* uiAttribPartServer::curDescSet( bool is2d ) const
{
    return getAdsMan( is2d )->descSet();
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
    DescSetMan* adsman = getAdsMan( is2d );
    IOPar iop;
    if ( adsman->descSet() ) adsman->descSet()->fillPar( iop );

    DescSet* oldset = adsman->descSet();
    delete attrsetdlg_;
    attrsetdlg_ = new uiAttribDescSetEd( parent(), adsman );
    attrsetdlg_->dirshowcb.notify( mCB(this,uiAttribPartServer,directShowAttr));
    attrsetdlg_->evalattrcb.notify( mCB(this,uiAttribPartServer,showEvalDlg) );
    attrsetdlg_->windowClosed.notify( 
	    			mCB(this,uiAttribPartServer,attrsetDlgClosed) );
    return attrsetdlg_->go();
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
	DescSetMan* adsman = getAdsMan( is2d );
	adsman->setDescSet( attrsetdlg_->getSet()->clone() );
	adsman->attrsetid_ = attrsetdlg_->curSetID();
	set2DEvent( is2d );
	sendEvent( evNewAttrSet );
    }

    delete attrsetdlg_;
    attrsetdlg_ = 0;
    sendEvent( evAttrSetDlgClosed );
}


const NLAModel* uiAttribPartServer::getNLAModel( bool is2d ) const
{
    return (NLAModel*)getObject( is2d ? objNLAModel2D : objNLAModel3D );
}


bool uiAttribPartServer::selectAttrib( SelSpec& selspec, const char* depthkey,
       				       bool is2d )
{
    DescSetMan* adsman = getAdsMan( is2d );
    uiAttrSelData attrdata( adsman->descSet() );
    attrdata.attribid = selspec.isNLA() ? SelSpec::cNoAttrib() : selspec.id();
    attrdata.outputnr = selspec.isNLA() ? selspec.id().asInt() : -1;
    attrdata.nlamodel = getNLAModel(is2d);
    attrdata.depthdomainkey = depthkey;
    uiAttrSelDlg dlg( parent(), "View Data", attrdata, false );
    if ( !dlg.go() )
	return false;

    attrdata.attribid = dlg.attribID();
    attrdata.outputnr = dlg.outputNr();
    const bool isnla = attrdata.attribid < 0 && attrdata.outputnr >= 0;
    IOObj* ioobj = IOM().get( adsman->attrsetid_ );
    BufferString attrsetnm = ioobj ? ioobj->name() : "";
    selspec.set( 0, isnla ? DescID(attrdata.outputnr,true) : attrdata.attribid,
	         isnla, isnla ? (const char*)nlaname_ : (const char*)attrsetnm);
    if ( isnla && attrdata.nlamodel )
	selspec.setRefFromID( *attrdata.nlamodel );
    else if ( !isnla )
	selspec.setRefFromID( *adsman->descSet() );
    selspec.setDepthDomainKey( dlg.depthDomainKey() );

    return true;
}

#define mAssignAdsMan( cond2d, newman ) \
    if ( cond2d ) \
	adsman2d_ = newman; \
    else \
	adsman3d_ = newman;

void uiAttribPartServer::directShowAttr( CallBacker* cb )
{
    mDynamicCastGet(uiAttribDescSetEd*,ed,cb);
    if ( !ed ) { pErrMsg("cb is not uiAttribDescSetEd*"); return; }

    dirshwattrdesc_ = ed->curDesc();
    DescSetMan* kpman = ed->is2D() ? adsman2d_ : adsman3d_;
    DescSet* edads = const_cast<DescSet*>(dirshwattrdesc_->descSet());
    PtrMan<DescSetMan> tmpadsman = new DescSetMan( ed->is2D(), edads, false );
    mAssignAdsMan(ed->is2D(),tmpadsman);
    sendEvent( evDirectShowAttr );
    mAssignAdsMan(ed->is2D(),kpman);
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
	const DescSet& ads = *adsman3d_->descSet();
	ss.setIDFromRef( ads );
	IOObj* ioobj = IOM().get( adsman3d_->attrsetid_ );
	if ( ioobj ) ss.setObjectRef( ioobj->name() );
    }
}


void uiAttribPartServer::getPossibleOutputs( bool is2d, 
					     BufferStringSet& nms ) const
{
    nms.erase();
    SelInfo attrinf( curDescSet( is2d ), 0, is2d );
    for ( int idx=0; idx<attrinf.attrnms.size(); idx++ )
	nms.add( attrinf.attrnms.get(idx) );

    BufferStringSet storedoutputs;
    for ( int idx=0; idx<attrinf.ioobjids.size(); idx++ )
    {
	const char* ioobjid = attrinf.ioobjids.get( idx );
	uiSeisIOObjInfo* sii = new uiSeisIOObjInfo( MultiID(ioobjid), false );
	sii->getDefKeys( storedoutputs, true );
	delete sii;
    }

    for ( int idx=0; idx<storedoutputs.size(); idx++ )
	nms.add( storedoutputs.get(idx) );
}


bool uiAttribPartServer::setSaved( bool is2d ) const
{
    return getAdsMan( is2d )->isSaved();
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
	    uiMSG().error("Cannot find attribute set in data base");
	else if ( !AttribDescSetTranslator::store(*getAdsMan(is2d)->descSet(),
						  ctio->ioobj,bs) )
	    uiMSG().error(bs);
    }
    ctio->setObj( 0 );
}


void uiAttribPartServer::outputVol( MultiID& nlaid, bool is2d )
{
    DescSet* dset = getAdsMan( is2d )->descSet();
    if ( !dset ) { pErrMsg("No attr set"); return; }

    uiAttrVolOut dlg( parent(), *dset, getNLAModel(is2d), nlaid );
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
    if ( targetspecs_.isEmpty() || targetspecs_[0].id() == SelSpec::cNoAttrib())
	{ pErrMsg("Nothing to do"); return false; }
    
    const bool is2d = targetspecs_[0].is2D();
    DescSetMan* adsman = getAdsMan( is2d );
    if ( !adsman->descSet() )
	{ pErrMsg("No attr set"); return false; }

    EngineMan* aem = new EngineMan;
    aem->setAttribSet( adsman->descSet() );
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
    const bool isflat = cs.isFlat();
    DataPackMgr& dpman = DPM( isflat ? DataPackMgr::FlatID
	    			     : DataPackMgr::CubeID );
    const DataPack* datapack = dpman.obtain( cacheid, true );
    const Attrib::DataCubes* cache = 0;
    mDynamicCastGet(const Attrib::CubeDataPack*,cdp,datapack);
    if ( cdp ) cache = &cdp->cube();
    mDynamicCastGet(const Attrib::Flat3DDataPack*,fdp,datapack);
    if ( fdp ) cache = &fdp->cube();
    const DataCubes* output = createOutput( cs, cache );
    if ( !output || !output->nrCubes() ) return -1;

    DataPack* newpack;
    if ( isflat )
	newpack = new Attrib::Flat3DDataPack( targetID(false), *output, 0 );
    else
	newpack = new Attrib::CubeDataPack( targetID(false), *output, 0 );
    newpack->setName( targetspecs_[0].userRef() );
    dpman.add( newpack );
    return newpack->id();
}


const Attrib::DataCubes* uiAttribPartServer::createOutput(
				const CubeSampling& cs, const DataCubes* cache )
{
    PtrMan<EngineMan> aem = createEngMan( &cs, 0 );
    if ( !aem ) return 0;

    BufferString defstr;
    const DescSet* attrds = adsman3d_->descSet();
    if ( attrds && attrds->nrDescs() && attrds->getDesc(targetspecs_[0].id()) )
    {
	attrds->getDesc(targetspecs_[0].id())->getDefStr(defstr);
	if ( strcmp (defstr, targetspecs_[0].defString()) )
	    cache = 0;
    }

    BufferString errmsg;
    Processor* process = aem->createDataCubesOutput( errmsg, cache );
    if ( !process )
	{ uiMSG().error(errmsg); return 0; }

    bool success = true;
    if ( aem->getNrOutputsToBeProcessed(*process) != 0 )
    {
	uiExecutor dlg( parent(), *process );
	success = dlg.go();
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
	if ( !uiMSG().askGoOn("Attribute loading/calculation aborted.\n"
	    "Do you want to use the partially loaded/computed data?", true ) )
	{
	    output->unRef();
	    output = 0;
	}
    }

    if ( output )
	output->unRefNoDelete();

    return output;
}


bool uiAttribPartServer::createOutput( ObjectSet<BinIDValueSet>& values )
{
    PtrMan<EngineMan> aem = createEngMan();
    if ( !aem ) return false;

    BufferString errmsg;
    PtrMan<Processor> process = aem->createLocationOutput( errmsg, values );
    if ( !process )
	{ uiMSG().error(errmsg); return false; }

    uiExecutor dlg( parent(), *process );
    if ( !dlg.go() ) return false;

    return true;
}


DataPack::ID uiAttribPartServer::createRdmTrcsOutput(
				const Interval<float>& zrg,
				TypeSet<BinID>* path )
{
    BinIDValueSet bidset( 2, false );
    for ( int idx=0; idx<path->size(); idx++ )
	bidset.add( (*path)[idx], zrg.start, zrg.stop );

    SeisTrcBuf output( true );
    if ( !createOutput(bidset,output) )
	return -1;

    DataPackMgr& dpman = DPM( DataPackMgr::FlatID );
    DataPack* newpack =
		new Attrib::FlatRdmTrcsDataPack( targetID(false), output, path);
    newpack->setName( targetspecs_[0].userRef() );
    dpman.add( newpack );
    return newpack->id();
}


bool uiAttribPartServer::createOutput( const BinIDValueSet& bidvalset,
				       SeisTrcBuf& output )
{
    PtrMan<EngineMan> aem = createEngMan();
    if ( !aem ) return 0;

    BufferString errmsg;
    PtrMan<Processor> process = aem->createTrcSelOutput( errmsg, bidvalset, 
	    						 output );
    if ( !process )
	{ uiMSG().error(errmsg); return false; }

    uiExecutor dlg( parent(), *process );
    if ( !dlg.go() ) return false;

    return true;
}


DataPack::ID uiAttribPartServer::create2DOutput( const CubeSampling& cs,
						 const LineKey& linekey )
{
    PtrMan<EngineMan> aem = createEngMan( &cs, linekey );
    if ( !aem ) return -1;

    BufferString errmsg;
    Data2DHolder* data2d = new Data2DHolder;
    PtrMan<Processor> process = aem->createScreenOutput2D( errmsg, *data2d );
    if ( !process )
	{ uiMSG().error(errmsg); return -1; }

    uiExecutor dlg( parent(), *process );
    if ( !dlg.go() )
	return -1;

    DataPackMgr& dpman = DPM( DataPackMgr::FlatID );
    Flat2DDHDataPack* newpack = new Attrib::Flat2DDHDataPack( targetID(true),
	    						      *data2d);
    newpack->setName( linekey.attrName() );
    dpman.add( newpack );
    return newpack->id();
}


bool uiAttribPartServer::isDataAngles( bool is2ddesc ) const
{
    DescSetMan* adsman = getAdsMan( is2ddesc );
    if ( !adsman->descSet() || targetspecs_.isEmpty() )
	return false;
	    
    const Desc* desc = adsman->descSet()->getDesc(targetspecs_[0].id());
    if ( !desc )
	return false;

    return Seis::isAngle( desc->dataType() );
}


static const int sMaxNrClasses = 100;
//static const int sMaxNrVals = 100;

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
		const int ival = mNINT(val);
		if ( !mIsEqual(val,ival,mDefEps) || abs(ival)>sMaxNrClasses )
		    return false;
//		nrint++;
//		if ( nrint > sMaxNrVals )
//		    break;
	    }

    return true;
}


bool uiAttribPartServer::extractData( const NLACreationDesc& desc,
				      const ObjectSet<BinIDValueSet>& bivsets,
				      ObjectSet<PosVecDataSet>& outvds, 
				      bool is2d )
{
    DescSetMan* adsman = getAdsMan( is2d );
    if ( !adsman->descSet() ) { pErrMsg("No attr set"); return 0; }

    if ( desc.doextraction )
    {
	PosVecOutputGen pvog( *adsman->descSet(), desc.design.inputs,
				     bivsets, outvds );
	uiExecutor dlg( parent(), pvog );
	if ( !dlg.go() )
	    return false;
	else if ( outvds.size() != bivsets.size() )
	{
	    // Earlier error
	    ErrMsg( "Error during data extraction (not all data extracted)." );
	    return false;
	}
    }
    else
    {
	PtrMan<IOObj> ioobj = IOM().get( desc.vdsid );
	PosVecDataSet* vds = new PosVecDataSet;
	BufferString errmsg;
	if ( !vds->getFrom(ioobj->fullUserExpr(true),errmsg) )
	    { uiMSG().error( errmsg ); delete vds; return false; }
	if ( vds->pars().isEmpty() || vds->data().isEmpty() )
	    { uiMSG().error( "Invalid Training set specified" );
		delete vds; return false; }
	outvds += vds;
    }
    return true;
}


Attrib::DescID uiAttribPartServer::getStoredID( const LineKey& lkey, bool is2d )
{
    DescSet* ds = is2d ? adsman2d_->descSet() : adsman3d_->descSet();
    return ds ? ds->getStoredID( lkey ) : DescID::undef();
}


bool uiAttribPartServer::createAttributeSet( const BufferStringSet& inps,
					     DescSet* attrset )
{
    AttributeSetCreator attrsetcr( parent(), inps, attrset );
    return attrsetcr.create();
}


//TODO check what is the exact role of the descset
bool uiAttribPartServer::setPickSetDirs( Pick::Set& ps, const NLAModel* nlamod )
{
//    uiSetPickDirs dlg( parent(), ps, curDescSet(), nlamod );
//	return dlg.go();
    return true;
}


//TODO may require a linekey in the as ( for docheck )
void uiAttribPartServer::insert2DStoredItems( const BufferStringSet& bfset, 
					      int start, int stop, 
					      bool correcttype, MenuItem* mnu,
       					      const SelSpec& as, bool usesubmnu) 
{
    mnu->checkable = true;
    mnu->enabled = bfset.size();
    if ( usesubmnu )
    {
	while ( linesets2dmnuitem_.size() )
	{
	    MenuItem* olditm = linesets2dmnuitem_.remove(0);
	    delete olditm;
	}
    }	    
    for ( int idx=start; idx<stop; idx++ )
    {
	BufferString key = bfset.get(idx);
	MenuItem* itm = new MenuItem( key );
	itm->checkable = true;
	const bool docheck =  correcttype && key == as.userRef();
	mAddManagedMenuItem( mnu, itm, true, docheck );
	if ( docheck ) mnu->checked = true;
	if ( usesubmnu )
	{
	    linesets2dmnuitem_ += itm;
	    SelInfo attrinf(adsman2d_->descSet(), 0, true, DescID::undef());
	    const MultiID mid( attrinf.ioobjids.get(idx) );
	    BufferStringSet nmsset = get2DStoredItems(mid);
	    insert2DStoredItems( nmsset, 0, nmsset.size(), correcttype,
				 itm, as, false );
	}
    }
}


BufferStringSet uiAttribPartServer::get2DStoredItems( const MultiID& mid) const
{
    BufferStringSet nms;
    SelInfo::getAttrNames( mid, nms );
    return nms;
}


BufferStringSet uiAttribPartServer::get2DStoredLSets( const SelInfo& sinf) const
{
    BufferStringSet linesets;
    for ( int idlset=0; idlset<sinf.ioobjids.size(); idlset++ )
    {
	const char* lsetid = sinf.ioobjids.get(idlset);
	const MultiID mid( lsetid );
	const BufferString& lsetnm = IOM().get(mid)->name();
	linesets.add( lsetnm );
    }

    return linesets;
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
    mAddManagedMenuItem( mnu, itm, true, docheck );\
    if ( docheck ) (mnu)->checked = true; \
}


static int cMaxMenuSize = 150;

MenuItem* uiAttribPartServer::storedAttribMenuItem( const SelSpec& as, 
						    bool is2d )
{
    const DescSet* ds = is2d ? adsman2d_->descSet() : adsman3d_->descSet();
    SelInfo attrinf( ds, 0, is2d, DescID::undef() );

    const bool isstored = ds && ds->getDesc( as.id() ) 
	? ds->getDesc( as.id() )->isStored() : false;
    const bool isnla = as.isNLA();
    const bool hasid = as.id() >= 0;
    const BufferStringSet bfset = is2d ? get2DStoredLSets( attrinf )
				       : attrinf.ioobjnms;
    int nritems = bfset.size();
    if ( nritems <= cMaxMenuSize )
    {
	if ( is2d )
	{
	    if ( nritems == 1 )
	    {
		const MultiID mid( attrinf.ioobjids.get(0) );
		BufferStringSet nmsset = get2DStoredItems(mid);
		insert2DStoredItems( nmsset, 0, nmsset.size(), isstored,
				     &stored2dmnuitem_, as, false );
	    }
	    else
		insert2DStoredItems( bfset, 0, nritems, isstored,
				     &stored2dmnuitem_, as, true );
	}
	else
	{
	    const int start = 0; const int stop = nritems;
	    mInsertItems(ioobjnms,&stored3dmnuitem_,isstored);
	}
    }
    else
	insertNumerousItems( bfset, as, isstored, is2d );

    MenuItem* storedmnuitem = is2d ? &stored2dmnuitem_ : &stored3dmnuitem_;
    storedmnuitem->enabled = storedmnuitem->nrItems();
    return storedmnuitem;
}



void uiAttribPartServer::insertNumerousItems( const BufferStringSet& bfset,
					      const SelSpec& as,
       					      bool correcttype, bool is2d )
{
    int nritems = bfset.size();
    const int nrsubmnus = (nritems-1)/cMaxMenuSize + 1;
    for ( int mnuidx=0; mnuidx<nrsubmnus; mnuidx++ )
    {
	const int start = mnuidx * cMaxMenuSize;
	int stop = (mnuidx+1) * cMaxMenuSize;
	if ( stop > nritems ) stop = nritems;
	const char* startnm = bfset.get(start);
	const char* stopnm = bfset.get(stop-1);
	BufferString str; strncat(str.buf(),startnm,3);
	str += " - "; strncat(str.buf(),stopnm,3);
	MenuItem* submnu = new MenuItem( str );
	if ( is2d )
	    insert2DStoredItems( bfset, start, stop, correcttype,
		    		 submnu, as, true );
	else
	{
	    SelInfo attrinf( adsman3d_->descSet(), 0, false, DescID::undef() );
	    mInsertItems(ioobjnms,submnu,correcttype);
	}
	
	MenuItem* storedmnuitem = is2d ? &stored2dmnuitem_ : &stored3dmnuitem_;
	mAddManagedMenuItem( storedmnuitem, submnu, true,submnu->checked);
    }
}


MenuItem* uiAttribPartServer::calcAttribMenuItem( const SelSpec& as,
						  bool is2d, bool useext )
{
    SelInfo attrinf( is2d ? adsman2d_->descSet() : adsman3d_->descSet() );
    const bool isattrib = attrinf.attrids.indexOf( as.id() ) >= 0; 

    const int start = 0; const int stop = attrinf.attrnms.size();
    MenuItem* calcmnuitem = is2d ? &calc2dmnuitem_ : &calc3dmnuitem_;
    BufferString txt = useext ? ( is2d? "Attributes &2D" : "Attributes &3D" )
			      : "&Attributes";
    calcmnuitem->text = txt;
    mInsertItems(attrnms,calcmnuitem,isattrib);

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
	DescSet* dset = is2d ? adsman2d_->descSet() : adsman3d_->descSet();
	SelInfo attrinf( dset, nlamodel );
	const bool isnla = as.isNLA();
	const bool hasid = as.id() >= 0;
	const int start = 0; const int stop = attrinf.nlaoutnms.size();
	mInsertItems(nlaoutnms,nlamnuitem,isnla);
    }

    nlamnuitem->enabled = nlamnuitem->nrItems();
    return nlamnuitem;
}


// TODO: create more general function, for now it does what we need
MenuItem* uiAttribPartServer::depthdomainAttribMenuItem( const SelSpec& as,
							 const char* key,
							 bool is2d, bool useext)
{
    MenuItem* depthdomainmnuitem = is2d ? &depthdomain2dmnuitem_ 
					: &depthdomain3dmnuitem_;
    BufferString itmtxt = key;
    itmtxt += useext ? ( is2d ? " Cubes" : " 2D Lines") : " Data";
    depthdomainmnuitem->text = itmtxt;
    depthdomainmnuitem->removeItems();
    depthdomainmnuitem->checkable = true;
    depthdomainmnuitem->checked = false;

    BufferStringSet ioobjnms;
    SelInfo::getSpecialItems( key, ioobjnms );
    for ( int idx=0; idx<ioobjnms.size(); idx++ )
    {
	const BufferString& nm = ioobjnms.get( idx );
	MenuItem* itm = new MenuItem( nm );
	const bool docheck = nm == as.userRef();
	mAddManagedMenuItem( depthdomainmnuitem, itm, true, docheck );
	if ( docheck ) depthdomainmnuitem->checked = true;
    }

    depthdomainmnuitem->enabled = depthdomainmnuitem->nrItems();
    return depthdomainmnuitem;
}


bool uiAttribPartServer::handleAttribSubMenu( int mnuid, SelSpec& as ) const
{
    bool needext = SI().getSurvDataType()==SurveyInfo::Both2DAnd3D;
    bool is2d = stored2dmnuitem_.findItem(mnuid) ||
		calc2dmnuitem_.findItem(mnuid) ||
		nla2dmnuitem_.findItem(mnuid) ||
		depthdomain2dmnuitem_.findItem(mnuid);

    DescSetMan* adsman = getAdsMan( is2d );
    uiAttrSelData attrdata( adsman->descSet() );
    attrdata.nlamodel = getNLAModel(is2d);
    SelInfo attrinf( attrdata.attrset, attrdata.nlamodel, is2d );
    const MenuItem* calcmnuitem = is2d ? &calc2dmnuitem_ : &calc3dmnuitem_;
    const MenuItem* nlamnuitem = is2d ? &nla2dmnuitem_ : &nla3dmnuitem_;
    const MenuItem* depthdomainmnuitem = is2d ? &depthdomain2dmnuitem_
					      : &depthdomain3dmnuitem_;

    DescID attribid = SelSpec::cAttribNotSel();
    int outputnr = -1;
    bool isnla = false;

    if ( stored3dmnuitem_.findItem(mnuid) )
    {
	const MenuItem* item = stored3dmnuitem_.findItem(mnuid);
	int idx = attrinf.ioobjnms.indexOf(item->text);
	attribid = adsman->descSet()->getStoredID( attrinf.ioobjids.get(idx) );
    }
    else if ( stored2dmnuitem_.findItem(mnuid) )
    {
	if ( linesets2dmnuitem_.isEmpty() )
	{
	    const MenuItem* item = stored2dmnuitem_.findItem(mnuid);
	    const MultiID mid( attrinf.ioobjids.get(0) );
	    LineKey idlkey( mid, item->text );
	    attribid = adsman->descSet()->getStoredID( idlkey );
	}
	else
	{
	    for ( int idx=0; idx<linesets2dmnuitem_.size(); idx++ )
	    {
		if ( linesets2dmnuitem_[idx]->findItem(mnuid) )
		{
		    const MenuItem* item =
				    linesets2dmnuitem_[idx]->findItem(mnuid);
		    const MultiID mid( attrinf.ioobjids.get(idx) );
		    LineKey idlkey( mid, item->text );
		    attribid = adsman->descSet()->getStoredID( idlkey );
		}
	    }
	}
    }
    else if ( calcmnuitem->findItem(mnuid) )
    {
	const MenuItem* item = calcmnuitem->findItem(mnuid);
	int idx = attrinf.attrnms.indexOf(item->text);
	attribid = attrinf.attrids[idx];
    }
    else if ( nlamnuitem->findItem(mnuid) )
    {
	outputnr = nlamnuitem->itemIndex(nlamnuitem->findItem(mnuid));
	isnla = true;
    }
    else if ( depthdomainmnuitem->findItem(mnuid) )
    {
	const MenuItem* item = depthdomainmnuitem->findItem( mnuid );
	IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id));
	PtrMan<IOObj> ioobj = IOM().getLocal( item->text );
	if ( ioobj )
	    attribid = adsman->descSet()->getStoredID( ioobj->key() );
    }
    else
	return false;

    IOObj* ioobj = IOM().get( adsman->attrsetid_ );
    BufferString attrsetnm = ioobj ? ioobj->name() : "";
    as.set( 0, isnla ? DescID(outputnr,true) : attribid, isnla,
	    isnla ? (const char*)nlaname_ : (const char*)attrsetnm );
    
    BufferString bfs;
    if ( attribid != SelSpec::cAttribNotSel() )
    {
	adsman->descSet()->getDesc(attribid)->getDefStr(bfs);
	as.setDefString(bfs.buf());
    }

    if ( isnla )
	as.setRefFromID( *attrdata.nlamodel );
    else
	as.setRefFromID( *adsman->descSet() );
    
    as.set2DFlag( is2d );

    return true;
}


IOObj* uiAttribPartServer::getIOObj( const Attrib::SelSpec& as ) const
{
    const Attrib::DescSet* attrset = curDescSet(false);
    if ( !attrset ) return 0;

    const Attrib::Desc* desc = attrset->getDesc( as.id() );
    if ( !desc ) return 0;

    MultiID id;
    if ( !desc->isStored() || !desc->getMultiID(id) ) return 0;

    return IOM().get( id );
}


#define mErrRet(msg) { uiMSG().error(msg); return; }

void uiAttribPartServer::showEvalDlg( CallBacker* )
{
    if ( !attrsetdlg_ ) return;
    const Desc* curdesc = attrsetdlg_->curDesc();
    if ( !curdesc )
	mErrRet( "Please add this attribute first" )

    uiAttrDescEd* ade = attrsetdlg_->curDescEd();
    if ( !ade ) return;

    sendEvent( evEvalAttrInit );
    if ( !alloweval_ ) mErrRet( "Evaluation of attributes only possible on\n"
			       "Inlines, Crosslines, Timeslices and Surfaces.");

    uiEvaluateDlg* evaldlg = new uiEvaluateDlg( attrsetdlg_, *ade,
	    					allowevalstor_ );
    if ( !evaldlg->evaluationPossible() )
	mErrRet( "This attribute has no parameters to evaluate" )

    evaldlg->calccb.notify( mCB(this,uiAttribPartServer,calcEvalAttrs) );
    evaldlg->showslicecb.notify( mCB(this,uiAttribPartServer,showSliceCB) );
    evaldlg->windowClosed.notify( mCB(this,uiAttribPartServer,evalDlgClosed) );
    evaldlg->go();
    attrsetdlg_->setSensitive( false );
}


void uiAttribPartServer::evalDlgClosed( CallBacker* cb )
{
    mDynamicCastGet(uiEvaluateDlg*,evaldlg,cb);
    if ( !evaldlg ) { pErrMsg("cb is not uiEvaluateDlg*"); return; }

    if ( evaldlg->storeSlices() )
	sendEvent( evEvalStoreSlices );
    
    Desc* curdesc = attrsetdlg_->curDesc();
    BufferString curusrref = curdesc->userRef();
    uiAttrDescEd* ade = attrsetdlg_->curDescEd();

    DescSet* curattrset = attrsetdlg_->getSet();
    const Desc* evad = evaldlg->getAttribDesc();
    if ( evad )
    {
	BufferString defstr;
	evad->getDefStr( defstr );
	curdesc->parseDefStr( defstr );
	attrsetdlg_->updateCurDescEd();
    }

    attrsetdlg_->setSensitive( true );
}


void uiAttribPartServer::calcEvalAttrs( CallBacker* cb )
{
    mDynamicCastGet(uiEvaluateDlg*,evaldlg,cb);
    if ( !evaldlg ) { pErrMsg("cb is not uiEvaluateDlg*"); return; }

    const bool is2d = attrsetdlg_->is2D();
    DescSetMan* kpman = is2d ? adsman2d_ : adsman3d_;
    DescSet* ads = evaldlg->getEvalSet();
    evaldlg->getEvalSpecs( targetspecs_ );
    PtrMan<DescSetMan> tmpadsman = new DescSetMan( is2d, ads, false );
    mAssignAdsMan(is2d,tmpadsman);
    set2DEvent( is2d );
    sendEvent( evEvalCalcAttr );
    mAssignAdsMan(is2d,kpman);
}


void uiAttribPartServer::showSliceCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sel,cb);
    if ( sel < 0 ) return;

    sliceidx_ = sel;
    sendEvent( evEvalShowSlice );
}


DescSetMan* uiAttribPartServer::getAdsMan( bool is2d )
{
    return is2d ? adsman2d_ : adsman3d_;
}


DescSetMan* uiAttribPartServer::getAdsMan( bool is2d ) const 
{
    return is2d ? adsman2d_ : adsman3d_;
}


#define mCleanMenuItems(startstr,mnuitem_)\
{\
    startstr##mnuitem_.removeItems();\
    startstr##mnuitem_.checked = false;\
}

void uiAttribPartServer::resetMenuItems()
{
    mCleanMenuItems(stored2d,mnuitem_)
    mCleanMenuItems(calc2d,mnuitem_)
    mCleanMenuItems(nla2d,mnuitem_)
    mCleanMenuItems(stored3d,mnuitem_);
    mCleanMenuItems(calc3d,mnuitem_);
    mCleanMenuItems(nla3d,mnuitem_);
}


void uiAttribPartServer::fillPar( IOPar& iopar, bool is2d ) const
{
    DescSetMan* adsman = getAdsMan( is2d );
    if ( adsman->descSet() && adsman->descSet()->nrDescs() )
	adsman->descSet()->fillPar( iopar );
}


void uiAttribPartServer::usePar( const IOPar& iopar, bool is2d )
{
    DescSetMan* adsman = getAdsMan( is2d );
    if ( adsman->descSet() )
    {
	BufferStringSet errmsgs;
	adsman->descSet()->usePar( iopar, &errmsgs );
	BufferString errmsg;
	for ( int idx=0; idx<errmsgs.size(); idx++ )
	{
	    if ( !idx )
	    {
		errmsg = "Error during restore of ";
		errmsg += is2d ? "2D " : "3D "; errmsg += "Attribute Set:";
	    }
	    errmsg += "\n";
	    errmsg += errmsgs.get( idx );
	}
	if ( !errmsg.isEmpty() )
	    uiMSG().error( errmsg );

	set2DEvent( is2d );
	sendEvent( evNewAttrSet );
    }
}
