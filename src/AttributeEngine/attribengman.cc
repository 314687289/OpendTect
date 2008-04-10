/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H.Payraudeau
 Date:          04/2005
 RCS:           $Id: attribengman.cc,v 1.79 2008-04-10 14:08:18 cvsbert Exp $
________________________________________________________________________

-*/

#include "attribengman.h"

#include "arrayndimpl.h"
#include "attribdatacubes.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attriboutput.h"
#include "attribparam.h"
#include "attribprocessor.h"
#include "attribprovider.h"
#include "attribstorprovider.h"

#include "binidvalset.h"
#include "cubesampling.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "linekey.h"
#include "linesetposinfo.h"
#include "nladesign.h"
#include "nlamodel.h"
#include "posvecdataset.h"
#include "ptrman.h"
#include "seis2dline.h"
#include "separstr.h"
#include "survinfo.h"



namespace Attrib
{
    
EngineMan::EngineMan()
	: inpattrset(0)
	, procattrset(0)
	, nlamodel(0)
	, cs_(*new CubeSampling)
	, cache(0)
	, udfval( mUdf(float) )
	, curattridx(0)
{
}


EngineMan::~EngineMan()
{
    delete procattrset;
    delete inpattrset;
    delete nlamodel;
    delete &cs_;
    if ( cache ) cache->unRef();
}


void EngineMan::getPossibleVolume( DescSet& attribset, CubeSampling& cs,
				   const char* linename, const DescID& outid )
{
    TypeSet<DescID> desiredids(1,outid);
    BufferString errmsg;
    DescID evalid = createEvaluateADS( attribset, desiredids, errmsg );
    PtrMan<Processor> proc = 
			createProcessor( attribset, linename, evalid, errmsg );
    if ( !proc ) return;

    proc->computeAndSetRefZStep();
    proc->getProvider()->setDesiredVolume( cs );
    proc->getProvider()->getPossibleVolume( -1, cs );
}


Processor* EngineMan::usePar( const IOPar& iopar, DescSet& attribset, 
	      		      const char* linename, BufferString& errmsg )
{
    int outputidx = 0;
    TypeSet<DescID> ids;
    while ( true )
    {    
	BufferString outpstr = IOPar::compKey( "Output", outputidx );
	PtrMan<IOPar> outputpar = iopar.subselect( outpstr );
	if ( !outputpar )
	{
	    if ( !outputidx )
	    { outputidx++; continue; }
	    else 
		break;
	}

	int attribidx = 0;
	while ( true )
	{
	    BufferString attribidstr = 
			IOPar::compKey( "Attributes", attribidx );
	    int attribid;
	    if ( !outputpar->get(attribidstr,attribid) )
		break;

	    ids += DescID(attribid,true);
	    attribidx++;
	}

	outputidx++;
    }

    DescID evalid = createEvaluateADS( attribset, ids, errmsg );
    Processor* proc = createProcessor( attribset, linename, evalid, errmsg );
    if ( !proc ) return 0;

    for ( int idx=1; idx<ids.size(); idx++ )
	proc->addOutputInterest(idx);
    
    BufferString basekey = IOPar::compKey( "Output",1 );
    PtrMan<IOPar> outpar = iopar.subselect( IOPar::compKey(basekey,"Sub") );
    if ( !outpar || !cs_.usePar( *outpar ) )
    {
	if ( attribset.is2D() )
	    cs_.set2DDef();
	else
	    cs_.init();
    }

    const Attrib::Desc* curdesc = attribset.getDesc( ids[0] );
    BufferString attribname = curdesc->isStored() ? "" : curdesc->userRef();
    LineKey lkey( linename, attribname );
    SeisTrcStorOutput* storeoutp = createOutput( iopar, lkey );
    
    proc->addOutput( storeoutp );
    return proc;
}


Processor* EngineMan::createProcessor( const DescSet& attribset,
				       const char* linename,const DescID& outid,
				       BufferString& errmsg )
{
    Desc* targetdesc = const_cast<Desc*>(attribset.getDesc(outid));
    if ( !targetdesc ) return 0;
    
    Processor* processor = new Processor( *targetdesc, linename, errmsg );
    if ( !processor->isOK() )
    {
	delete processor;
	return 0;
    }

    processor->addOutputInterest( targetdesc->selectedOutput() );
    return processor;
}


void EngineMan::setExecutorName( Executor* ex )
{
    if ( !ex ) return;

    BufferString usernm( getCurUserRef() );
    if ( usernm.isEmpty() || !inpattrset ) return;

    if ( curattridx < 0 || curattridx >= attrspecs_.size() )
	ex->setName( "Processing attribute" );

    SelSpec& ss = attrspecs_[curattridx];
    BufferString nm( "Calculating " );
    if ( ss.isNLA() && nlamodel )
    {
	nm = "Applying ";
	nm += nlamodel->nlaType(true);
	nm += ": calculating";
	if ( IOObj::isKey(usernm) )
	    usernm = IOM().nameOf( usernm, false );
    }
    else
    {
	const Desc* desc = inpattrset->getDesc( ss.id() );
	if ( desc && desc->isStored() )
	    nm = "Reading from";
    }

    nm += " \"";
    nm += usernm;
    nm += "\"";

    ex->setName( nm );
}


SeisTrcStorOutput* EngineMan::createOutput( const IOPar& pars, 
					    const LineKey& lkey )
{
    const char* typestr = pars.find("Output.1.Type");

    if ( !strcmp(typestr,"Cube") )
    {
	SeisTrcStorOutput* outp = new SeisTrcStorOutput( cs_, lkey );
	outp->setGeometry(cs_);
	outp->doUsePar( pars );
	return outp;
    }

    return 0;
}


void EngineMan::setNLAModel( const NLAModel* m )
{
    delete nlamodel;
    nlamodel = m ? m->clone() : 0;
}


void EngineMan::setAttribSet( const DescSet* ads )
{
    delete inpattrset;
    inpattrset = ads ? ads->clone() : 0;
}


const char* EngineMan::getCurUserRef() const
{
    const int idx = curattridx;
    if ( attrspecs_.isEmpty() || attrspecs_[idx].id() < 0 ) return "";

    SelSpec& ss = const_cast<EngineMan*>(this)->attrspecs_[idx];
    if ( attrspecs_[idx].isNLA() )
    {
	if ( !nlamodel ) return "";
	ss.setRefFromID( *nlamodel );
    }
    else
    {
	if ( !inpattrset ) return "";
	ss.setRefFromID( *inpattrset );
    }
    return attrspecs_[idx].userRef();
}


const DataCubes* EngineMan::getDataCubesOutput( const Processor& proc )
{
    if ( proc.outputs_.size()==1 && !cache )
	return proc.outputs_[0]->getDataCubes();

    ObjectSet<const DataCubes> cubeset;
    for ( int idx=0; idx<proc.outputs_.size(); idx++ )
    {
	if ( !proc.outputs_[idx] || !proc.outputs_[idx]->getDataCubes() )
	    continue;

	const DataCubes* dc = proc.outputs_[idx]->getDataCubes();
	dc->ref();
	if ( cubeset.size() && cubeset[0]->nrCubes()!=dc->nrCubes() )
	{
	    dc->unRef();
	    continue;
	}

	cubeset += dc;
    }

    if ( cache )
    {
	cubeset += cache;
	cache->ref();
    }

    if ( cubeset.isEmpty() )
	return 0;

    DataCubes* output = new DataCubes;
    output->ref();
    if ( cache && cache->cubeSampling().zrg.step != cs_.zrg.step )
    {
	CubeSampling cswithcachestep = cs_;
	cswithcachestep.zrg.step = cache->cubeSampling().zrg.step;
	output->setSizeAndPos(cswithcachestep);
    }
    else
	output->setSizeAndPos(cs_);

    for ( int idx=0; idx<cubeset[0]->nrCubes(); idx++ )
	output->addCube(mUdf(float));

    for ( int iset=0; iset<cubeset.size(); iset++ )
    {
	const DataCubes& cubedata = *cubeset[iset];
	for ( int sidx=cubedata.getInlSz()-1; sidx>=0; sidx-- )
	{
	    const int inl = cubedata.inlsampling.atIndex(sidx);
	    const int tidx = output->inlsampling.nearestIndex(inl);
	    if ( tidx<0 || tidx>=output->getInlSz() )
		continue;

	    for ( int scdx=cubedata.getCrlSz()-1; scdx>=0; scdx-- )
	    {
		const int crl = cubedata.crlsampling.atIndex(scdx);
		const int tcdx = output->crlsampling.nearestIndex(crl);
		if ( tcdx<0 || tcdx>=output->getCrlSz() )
		    continue;

		for ( int szdx=cubedata.getZSz()-1; szdx>=0; szdx-- )
		{
		    const int z = cubedata.z0+szdx;
		    const int tzdx = z-output->z0;

		    if ( tzdx<0 || tzdx>=output->getZSz() )
			continue;

		    for ( int cubeidx=output->nrCubes()-1;cubeidx>=0;cubeidx--)
		    {
			const float val =
			    cubedata.getCube(cubeidx).get( sidx, scdx, szdx );

			if ( Values::isUdf( val ) )
			    continue;

			output->setValue( cubeidx, tidx, tcdx, tzdx, val );
		    }
		}
	    }
	}
    }

    deepUnRef( cubeset );
    output->unRefNoDelete();
    return output;
}


void EngineMan::setAttribSpecs( const TypeSet<SelSpec>& specs )
{ attrspecs_ = specs; }


void EngineMan::setAttribSpec( const SelSpec& spec )
{
    attrspecs_.erase();
    attrspecs_ += spec;
}


void EngineMan::setCubeSampling( const CubeSampling& newcs )
{
    cs_ = newcs;
    cs_.normalise();
}


DescSet* EngineMan::createNLAADS( DescID& nladescid, BufferString& errmsg,
       				  const DescSet* addtoset )
{
    if ( attrspecs_.isEmpty() ) return 0;
    DescSet* descset = addtoset ? addtoset->clone() 
				: new DescSet(attrspecs_[0].is2D());
    if ( !addtoset && !descset->usePar(nlamodel->pars()) )
    {
	errmsg = descset->errMsg();
	delete descset;
	return 0;
    }

    BufferString s;
    nlamodel->dump(s);
    BufferString defstr( nlamodel->nlaType(true) );
    defstr += " specification=\""; defstr += s; defstr += "\"";

    addNLADesc( defstr, nladescid, *descset, attrspecs_[0].id().asInt(), 
		nlamodel, errmsg );

    DescSet* cleanset = descset->optimizeClone( nladescid );
    delete descset;
    return cleanset;
}


void EngineMan::addNLADesc( const char* specstr, DescID& nladescid,
			    DescSet& descset, int outputnr,
			    const NLAModel* nlamdl, BufferString& errmsg )
{
    RefMan<Desc> desc = PF().createDescCopy( "NN" );
    desc->setDescSet( &descset );

    if ( !desc->parseDefStr(specstr) )
    { 
	errmsg = "Invalid definition string for NLA model:\n";
	errmsg += specstr;
	return;
    }
    desc->setHidden( true );

    // Need to make a Provider because the inputs and outputs may
    // not be known otherwise
    const char* emsg = Provider::prepare( *desc );
    if ( emsg )
	{ errmsg = emsg; return; }

    const int nrinputs = desc->nrInputs();
    for ( int idx=0; idx<nrinputs; idx++ )
    {
	const char* inpname = desc->inputSpec(idx).getDesc();
	DescID descid = descset.getID( inpname, true );
	if ( descid < 0 && IOObj::isKey(inpname) )
	{
	    descid = descset.getID( inpname, false );
	    if ( descid < 0 )
	    {
		// It could be 'storage', but it's not yet in the set ...
		PtrMan<IOObj> ioobj = IOM().get( MultiID(inpname) );
		if ( ioobj )
		{
		    Desc* stordesc = 
			PF().createDescCopy( StorageProvider::attribName() );
		    stordesc->setDescSet( &descset );
		    ValParam* idpar = 
			stordesc->getValParam( StorageProvider::keyStr() );
		    idpar->setValue( inpname );
		    stordesc->setUserRef( ioobj->name() );
		    descid = descset.addDesc( stordesc );
		    if ( descid < 0 )
		    {
			errmsg = "NLA input '";
			errmsg += inpname;
			errmsg += "' cannot be found in the provided set.";
			return;
		    }
		}
	    }
	}

	desc->setInput( idx, descset.getDesc(descid) );
    }

    if ( outputnr > desc->nrOutputs() )
    {
	errmsg = "Output "; errmsg += outputnr; 
	errmsg += " not present.";
	return;
    }
    
    const NLADesign& nlades = nlamdl->design();
    desc->setUserRef( *nlades.outputs[outputnr] );
    desc->selectOutput( outputnr );

    nladescid = descset.addDesc( desc );
    if ( nladescid == DescID::undef() )
	errmsg = descset.errMsg();
}


DescID EngineMan::createEvaluateADS( DescSet& descset, 
				     const TypeSet<DescID>& outids,
				     BufferString& errmsg )
{
    if ( outids.isEmpty() ) return DescID::undef();
    if ( outids.size() == 1 ) return outids[0];

    Desc* desc = PF().createDescCopy( "Evaluate" );
    desc->setDescSet( &descset );
    desc->setNrOutputs( Seis::UnknowData, outids.size() );

    desc->setHidden( true );

    const int nrinputs = outids.size();
    for ( int idx=0; idx<nrinputs; idx++ )
    {
	desc->addInput( InputSpec("Data",true) );
	Desc* inpdesc = descset.getDesc( outids[idx] );
	if ( !inpdesc ) continue;
	
	desc->setInput( idx, inpdesc );
    }

    desc->setUserRef( "evaluate attributes" );
    desc->selectOutput(0);

    DescID evaldescid = descset.addDesc( desc );
    if ( evaldescid == DescID::undef() )
    {
	errmsg = descset.errMsg();
	desc->unRef();
    }

    return evaldescid;
}


#undef mErrRet
#define mErrRet(s) { errmsg = s; return 0; }

#define mStepEps 1e-3


Processor* EngineMan::createScreenOutput2D( BufferString& errmsg,
					    Data2DHolder& output )
{
    Processor* proc = getProcessor( errmsg );
    if ( !proc ) 
	return 0; 
    
    LineKey lkey = linekey.buf();
    const Provider* prov = proc->getProvider();
    if ( prov && !prov->getDesc().isStored() )
	lkey.setAttrName( proc->getAttribName() );
    
    Interval<int> trcrg( cs_.hrg.start.crl, cs_.hrg.stop.crl );
    Interval<float> zrg( cs_.zrg.start, cs_.zrg.stop );
    TwoDOutput* attrout = new TwoDOutput( trcrg, zrg, lkey );
    attrout->setOutput( output );
    proc->addOutput( attrout ); 

    return proc;
}

#define mRg(dir) (cachecs.dir##rg)

Processor* EngineMan::createDataCubesOutput( BufferString& errmsg,
					    const DataCubes* prev )
{
    if ( cache )
    {
	cache->unRef();
	cache = 0;
    }


    if ( cs_.isEmpty() )
	prev = 0;
    else if ( prev )
    {
	cache = prev;
	cache->ref();
	const CubeSampling cachecs = cache->cubeSampling();
	if ( mRg(h).step != cs_.hrg.step
	  || (mRg(h).start.inl - cs_.hrg.start.inl) % cs_.hrg.step.inl
	  || (mRg(h).start.crl - cs_.hrg.start.crl) % cs_.hrg.step.crl 
	  || mRg(h).start.inl > cs_.hrg.stop.inl
	  || mRg(h).stop.inl < cs_.hrg.start.inl
	  || mRg(h).start.crl > cs_.hrg.stop.crl
	  || mRg(h).stop.crl < cs_.hrg.start.crl
	  || mRg(z).start > cs_.zrg.stop + mStepEps*cs_.zrg.step
	  || mRg(z).stop < cs_.zrg.start - mStepEps*cs_.zrg.step )
	    // No overlap, gotta crunch all the numbers ...
	{
	    cache->unRef();
	    cache = 0;
	}
    }

#define mAddAttrOut(todocs) \
{ \
    DataCubesOutput* attrout = new DataCubesOutput(todocs); \
    attrout->setGeometry( todocs ); \
    attrout->setUndefValue( udfval ); \
    proc->addOutput( attrout ); \
}

    Processor* proc = getProcessor(errmsg);
    if ( !proc ) 
	return 0; 

    if ( !cache )
	mAddAttrOut( cs_ )
    else
    {
	const CubeSampling cachecs = cache->cubeSampling();
	CubeSampling todocs( cs_ );
	if ( mRg(h).start.inl > cs_.hrg.start.inl )
	{
	    todocs.hrg.stop.inl = mRg(h).start.inl - cs_.hrg.step.inl;
	    mAddAttrOut( todocs )
	}

	if ( mRg(h).stop.inl < cs_.hrg.stop.inl )
	{
	    todocs = cs_;
	    todocs.hrg.start.inl = mRg(h).stop.inl + cs_.hrg.step.inl;
	    mAddAttrOut( todocs )
	}

	const int startinl = mMAX(cs_.hrg.start.inl, mRg(h).start.inl );
	const int stopinl = mMIN( cs_.hrg.stop.inl, mRg(h).stop.inl );

	if ( mRg(h).start.crl > cs_.hrg.start.crl )
	{
	    todocs = cs_;
	    todocs.hrg.start.inl = startinl; todocs.hrg.stop.inl = stopinl;
	    todocs.hrg.stop.crl = mRg(h).start.crl - cs_.hrg.step.crl;
	    mAddAttrOut( todocs )
	}
	
	if ( mRg(h).stop.crl < cs_.hrg.stop.crl )
	{
	    todocs = cs_;
	    todocs.hrg.start.inl = startinl; todocs.hrg.stop.inl = stopinl;
	    todocs.hrg.start.crl = mRg(h).stop.crl + cs_.hrg.step.crl;
	    mAddAttrOut( todocs )
	}

	todocs = cs_;
	todocs.hrg.start.inl = startinl; todocs.hrg.stop.inl = stopinl;
	todocs.hrg.start.crl = mMAX(cs_.hrg.start.crl, mRg(h).start.crl );
	todocs.hrg.stop.crl = mMIN(cs_.hrg.stop.crl, mRg(h).stop.crl );

	if ( mRg(z).start > cs_.zrg.start + mStepEps*cs_.zrg.step )
	{
	    todocs.zrg.stop = mMAX(mRg(z).start-cs_.zrg.step, todocs.zrg.start);
	    mAddAttrOut( todocs )
	}
	    
	if ( mRg(z).stop < cs_.zrg.stop - mStepEps*cs_.zrg.step )
	{
	    todocs.zrg = cs_.zrg;
	    todocs.zrg.start = mMIN(mRg(z).stop+cs_.zrg.step, todocs.zrg.stop);
	    mAddAttrOut( todocs )
	}
    }

    return proc;
}


class AEMFeatureExtracter : public Executor
{
public:
AEMFeatureExtracter( EngineMan& aem, const BufferStringSet& inputs,
		     const ObjectSet<BinIDValueSet>& bivsets )
    : Executor("Extracting attributes")
{
    const int nrinps = inputs.size();
    const DescSet* attrset = aem.procattrset ? aem.procattrset : aem.inpattrset;
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	const DescID id = attrset->getID( inputs.get(idx), true );
	if ( id == DescID::undef() )
	    continue;
	SelSpec ss( 0, id );
	ss.setRefFromID( *attrset );
	aem.attrspecs_ += ss;
    }

    ObjectSet<BinIDValueSet>& bvs = 
	const_cast<ObjectSet<BinIDValueSet>&>(bivsets);

    proc = aem.createLocationOutput( errmsg, bvs );
}

~AEMFeatureExtracter()		{ delete proc; }

int totalNr() const		{ return proc ? proc->totalNr() : -1; }
int nrDone() const		{ return proc ? proc->nrDone() : 0; }
const char* nrDoneText() const	{ return proc ? proc->nrDoneText() : ""; }

const char* message() const
{
    return *(const char*)errmsg ? errmsg.buf() 
	: (proc ? proc->message() : "Cannot create output");
}

int haveError( const char* msg )
{
    if ( msg ) errmsg = msg;
    return -1;
}

int nextStep()
{
    if ( !proc ) return haveError( 0 );

    int rv = proc->doStep();
    if ( rv >= 0 ) return rv;
    return haveError( proc->message() );
}

    BufferString		errmsg;
    Processor*			proc;
    TypeSet<DescID>		outattribs;
};


Executor* EngineMan::createFeatureOutput( const BufferStringSet& inputs,
				    const ObjectSet<BinIDValueSet>& bivsets )
{
    return new AEMFeatureExtracter( *this, inputs, bivsets );
}


void EngineMan::computeIntersect2D( ObjectSet<BinIDValueSet>& bivsets ) const
{
    if ( !procattrset || !attrspecs_.size() )
	return;

    if ( !procattrset->is2D() )
	return;

    Desc* storeddesc = 0;
    for ( int idx=0; idx<attrspecs_.size(); idx++ )
    {
	const Desc* desc = procattrset->getDesc( attrspecs_[idx].id() );
	if ( !desc ) continue;
	if ( desc->isStored() )
	{
	    storeddesc = const_cast<Desc*>(desc);
	    break;
	}
	else
	{
	    Desc* candidate = desc->getStoredInput();
	    if ( candidate )
	    {
		storeddesc = candidate;
		break;
	    }
	}
    }

    if ( !storeddesc )
	return;

    const LineKey lk( storeddesc->getValParam(
			StorageProvider::keyStr())->getStringValue(0) );

    const MultiID key( lk.lineName() );
    PtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj ) return;
    const Seis2DLineSet lset(ioobj->fullUserExpr(true));

    PosInfo::LineSet2DData linesetgeom;
    if ( !lset.getGeometry( linesetgeom ) ) return;

    ObjectSet<BinIDValueSet> newbivsets;
    for ( int idx=0; idx<bivsets.size(); idx++ )
    {
	BinIDValueSet* newset = new BinIDValueSet(bivsets[idx]->nrVals(), true);
	ObjectSet<PosInfo::LineSet2DData::IR> resultset;
	linesetgeom.intersect( *bivsets[idx], resultset );

	for ( int idy=0; idy<resultset.size(); idy++)
	    newset->append(*resultset[idy]->posns_);
	
	newbivsets += newset;
    }
    bivsets = newbivsets;
}


Processor* EngineMan::createLocationOutput( BufferString& errmsg,
					    ObjectSet<BinIDValueSet>& bidzvset )
{
    if ( bidzvset.size() == 0 ) mErrRet("No locations to extract data on")

    Processor* proc = getProcessor(errmsg);
    if ( !proc )
	return 0; 

    computeIntersect2D(bidzvset);
    ObjectSet<LocationOutput> outputs;
    for ( int idx=0; idx<bidzvset.size(); idx++ )
    {
	BinIDValueSet& bidzvs = *bidzvset[idx];
	LocationOutput* attrout = new LocationOutput( bidzvs );
	outputs += attrout;
    }
    
    if ( !outputs.size() )
        return 0;

    for ( int idx=0; idx<outputs.size(); idx++ )
	proc->addOutput( outputs[idx] );

    return proc;
}


//TODO TableOutput should later on replace at least Location- and TrcSel-Output
//AEMFeatureExtractor will also be replaced by AEMTableExtractor
class AEMTableExtractor : public Executor
{
public:
AEMTableExtractor( EngineMan& aem, DataPointSet& datapointset,
		   const Attrib::DescSet& descset )
    : Executor("Extracting attributes")
{
    const int nrinps = datapointset.nrCols();
    for ( int idx=0; idx<nrinps; idx++ )
    {
	BufferString tmpstr( datapointset.colDef(idx).ref_ );
	char* ptrtosep = strchr( tmpstr.buf(), '`' );
	if ( ptrtosep )	*ptrtosep = '\0';
	const DescID did( atoi(tmpstr.buf()), true );
	if ( did == DescID::undef() )
	    continue;
	SelSpec ss( 0, did );
	ss.setRefFromID( descset );
	aem.attrspecs_.addIfNew( ss );
    }

    proc = aem.getTableOutExecutor( datapointset, errmsg );
}

~AEMTableExtractor()		{ delete proc; }

int totalNr() const		{ return proc ? proc->totalNr() : -1; }
int nrDone() const		{ return proc ? proc->nrDone() : 0; }
const char* nrDoneText() const	{ return proc ? proc->nrDoneText() : ""; }

const char* message() const
{
    return *(const char*)errmsg ? errmsg.buf() 
	: (proc ? proc->message() : "Cannot create output");
}

int haveError( const char* msg )
{
    if ( msg ) errmsg = msg;
    return -1;
}

int nextStep()
{
    if ( !proc ) return haveError( 0 );

    int rv = proc->doStep();
    if ( rv >= 0 ) return rv;
    return haveError( proc->message() );
}

    BufferString		errmsg;
    Processor*			proc;
    TypeSet<DescID>		outattribs;
};


Executor* EngineMan::getTableExtractor( DataPointSet& datapointset,
       					const Attrib::DescSet& descset,
       					BufferString& errmsg )
{
    if ( !ensureDPSAndADSPrepared( datapointset, descset, errmsg ) )
	return 0;
    
    setAttribSet( &descset );
    AEMTableExtractor* tabex = new AEMTableExtractor( *this, datapointset,
	    					      descset );
    if ( tabex && !tabex->errmsg.isEmpty() )
	errmsg = tabex->errmsg;
    return tabex;
}


Processor* EngineMan::getTableOutExecutor( DataPointSet& datapointset,
					   BufferString& errmsg )
{
    if ( datapointset.size() == 0 ) mErrRet("No locations to extract data on");

    Processor* proc = getProcessor(errmsg);
    if ( !proc )
	return 0; 

    ObjectSet<BinIDValueSet> bidsets;
    bidsets += &datapointset.bivSet();
    computeIntersect2D( bidsets );
    TableOutput* tableout = new TableOutput( datapointset );
    if ( !tableout ) return 0;

    proc->addOutput( tableout );

    return proc;
}


#undef mErrRet
#define mErrRet(s) { errmsg = s; return false; }

Processor* EngineMan::getProcessor( BufferString& errmsg )
{
    if ( procattrset )
	{ delete procattrset; procattrset = 0; }

    if ( !inpattrset || !attrspecs_.size() )
	mErrRet( "No attribute set or input specs" )

    TypeSet<DescID> outattribs;
    for ( int idx=0; idx<attrspecs_.size(); idx++ )
	outattribs += attrspecs_[idx].id();

    DescID outid = outattribs[0];

    errmsg = "";
    bool doeval = false;
    if ( !attrspecs_[0].isNLA() )
    {
	procattrset = inpattrset->optimizeClone( outattribs );
	if ( !procattrset ) mErrRet("Attribute set not valid");

	if ( outattribs.size() > 1 )
	{
	    doeval = true;
	    outid = createEvaluateADS( *procattrset, outattribs, errmsg);
	}
    }
    else
    {
	DescID nlaid( SelSpec::cNoAttrib() );
	procattrset = createNLAADS( nlaid, errmsg );
	if ( *(const char*)errmsg )
	    mErrRet(errmsg)
	outid = nlaid;
    }

    Processor* proc = 
		createProcessor( *procattrset, lineKey().buf(), outid, errmsg );
    setExecutorName( proc );
    if ( !proc )
	mErrRet( errmsg )
	    
    if ( doeval )
    {
	for ( int idx=1; idx<attrspecs_.size(); idx++ )
	    proc->addOutputInterest(idx);
    }
    
    return proc;
}


Processor* EngineMan::createTrcSelOutput( BufferString& errmsg,
					  const BinIDValueSet& bidvalset,
					  SeisTrcBuf& output, float outval,
       					  Interval<float>* cubezbounds )
{
    Processor* proc = getProcessor(errmsg);
    if ( !proc )
	return 0;

    TrcSelectionOutput* attrout	= new TrcSelectionOutput( bidvalset, outval );
    attrout->setOutput( &output );
    if ( cubezbounds )
	attrout->setTrcsBounds( *cubezbounds );

    proc->addOutput( attrout );

    return proc;
}


Processor* EngineMan::create2DVarZOutput( BufferString& errmsg,
					  const IOPar& pars,
					  DataPointSet* datapointset,
					  float outval,
       					  Interval<float>* cubezbounds )
{
    PtrMan<IOPar> output = pars.subselect( "Output.1" );
    const char* linename = output->find(sKey::LineKey);
    setLineKey( linename );

    Processor* proc = getProcessor( errmsg );
    if ( !proc ) return 0;

    LineKey lkey( linename, proc->getAttribUserRef() );
    Trc2DVarZStorOutput* attrout = new Trc2DVarZStorOutput( lkey, datapointset,
	    						    outval );
    attrout->doUsePar( pars );
    if ( cubezbounds )
	attrout->setTrcsBounds( *cubezbounds );

    proc->addOutput( attrout );
    return proc;
}


int EngineMan::getNrOutputsToBeProcessed( const Processor& proc ) const
{
    return proc.outputs_.size();
}


bool EngineMan::ensureDPSAndADSPrepared( DataPointSet& datapointset,
					 const Attrib::DescSet& descset,
       					 BufferString& errmsg )
{
    BufferStringSet attrrefs;
    descset.fillInAttribColRefs( attrrefs );
    
    const int nrdpsfixcols = datapointset.nrFixedCols();
    for ( int idx=nrdpsfixcols; idx<datapointset.nrCols()+nrdpsfixcols; idx++ )
    {
	const char* nmstr = datapointset.dataSet().colDef(idx).name_.buf();
	if ( !strncmp( nmstr, "[", 1 ) )
	{
	    int refidx = -1;
	    for ( int ids=0; ids<attrrefs.size(); ids++ )
	    {
		BufferString tmpstr( attrrefs.get(ids) );
		char* ptrtosep = strchr( tmpstr.buf(), '`' );
		if ( ptrtosep )	*ptrtosep='\0';
		if ( !strcmp( tmpstr.buf(), nmstr ) )
		{
		    refidx = ids;
		    break;
		}
	    }
	    
	    DescID descid = DescID::undef();
	    if ( refidx > -1 )
	    {
		const char* str = strchr( attrrefs.get(refidx).buf(), '`' );
		if ( str )
		{
		    BufferString multiidstr( str + 1 );
		    descid = const_cast<DescSet&>(descset).
					getStoredID( multiidstr, 0, true );
		}
	    }
	    
	    if ( descid == DescID::undef() )
	    {
		BufferString err = "Cannot use stored data"; err += nmstr;
		mErrRet(err);
	    }

	    BufferString tmpstr;
	    const Attrib::Desc* desc = descset.getDesc( descid );
	    if ( !desc ) mErrRet("Huh?");
	    desc->getDefStr( tmpstr );
	    BufferString defstr = descid.asInt(); defstr += "`";
	    defstr += tmpstr;
	    attrrefs.get(refidx) = defstr;
	    datapointset.dataSet().colDef(idx).ref_ = defstr;
	}

	BufferString& refstr = datapointset.dataSet().colDef(idx).ref_;
	if ( refstr.isEmpty() )
	{
	    int refidx = attrrefs.indexOf( nmstr ); //case ref misplaced in name
	    if ( refidx == -1 )
	    {
		DescID did = descset.getID( nmstr, true );
		if ( did != DescID::undef() )
		{
		    for ( int idref=0; idref< attrrefs.size(); idref++ )
		    {
			BufferString tmpstr( attrrefs.get(idref) );
			char* ptrtosep = strchr( tmpstr.buf(), '`' );
			if ( ptrtosep ) *ptrtosep = '\0';
			const DescID candidatid( atoi(tmpstr.buf()), true );
			if ( did == candidatid )
			{
			    refidx = idref;
			    break;
			}
		    }
		}
	    }

	    if ( refidx > -1 )
		refstr = attrrefs.get( refidx );
	}
    }
    return true;
}

} // namespace Attrib
