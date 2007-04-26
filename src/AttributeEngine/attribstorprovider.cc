/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attribstorprovider.cc,v 1.64 2007-04-26 13:45:25 cvshelene Exp $";

#include "attribstorprovider.h"

#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attriblinebuffer.h"
#include "attribdataholder.h"
#include "attribdatacubes.h"
#include "datainpspec.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "linekey.h"
#include "multiid.h"
#include "ptrman.h"
#include "seis2dline.h"
#include "seisread.h"
#include "seismscprov.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "seistrctr.h"
#include "simpnumer.h"
#include "survinfo.h"
#include "threadwork.h"
#include "basictask.h"

namespace Attrib
{


void StorageProvider::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    desc->addParam( new SeisStorageRefParam(keyStr()) );
    desc->addOutputDataType( Seis::UnknowData );

    PF().addDesc( desc, createFunc );
    desc->unRef();
}


Provider* StorageProvider::createFunc( Desc& desc )
{
    StorageProvider* res = new StorageProvider( desc );
    res->ref();

    if ( !res->isOK() ) { res->unRef(); return 0; }

    res->unRefNoDelete();
    return res; 
}


void StorageProvider::updateDesc( Desc& desc )
{
    const LineKey lk( desc.getValParam(keyStr())->getStringValue(0) );

    const MultiID key( lk.lineName() );
    const BufferString attrnm = lk.attrName();
    
    PtrMan<IOObj> ioobj = IOM().get( key );
    SeisTrcReader rdr( ioobj );
    if ( !rdr.ioObj() || !rdr.prepareWork(Seis::PreScan) || rdr.psIOProv() )
    {
//	desc.setErrMsg( rdr.errMsg() );
	return;
    }

    if ( rdr.is2D() )
    {
	if ( !rdr.lineSet() )
	{
	    BufferString errmsg = "No line set available for '";
	    errmsg += ioobj->name(); errmsg += "'";
//	    desc.setErrMsg( errmsg );
	    return;
	}
	const bool issteering = attrnm == sKey::Steering;
	if ( !issteering )
	{
	    SeisTrcTranslator* transl = rdr.seisTranslator();
	    if ( !transl || transl->componentInfo().isEmpty() )
		desc.setNrOutputs( Seis::UnknowData, 1 );//why only 1 ?
	    else
		desc.setNrOutputs( (Seis::DataType)
				   transl->componentInfo()[0]->datatype, 1 );
	}
	else
	    desc.setNrOutputs( Seis::Dip, 2 );
    }
    else
    {
	SeisTrcTranslator* transl = rdr.seisTranslator();
	if ( !transl )
	{
	    BufferString errmsg = "No data interpreter available for '";
	    errmsg += ioobj->name(); errmsg += "'";
//	    desc.setErrMsg ( errmsg );
	    return;
	}

	BufferString type;
	ioobj->pars().get( sKey::Type, type );

	const int nrattribs = transl->componentInfo().size();
	if ( type == sKey::Steering )
	    desc.setNrOutputs( Seis::Dip, nrattribs );
	else
	{
	    for ( int idx=1; idx<=nrattribs; idx++ )
		if ( desc.nrOutputs() < idx )
		    desc.addOutputDataType( (Seis::DataType)
				    transl->componentInfo()[idx-1]->datatype);
	}
    }
}


StorageProvider::StorageProvider( Desc& desc_ )
    : Provider( desc_ )
    , mscprov_(0)
    , status( Nada )
{
}


StorageProvider::~StorageProvider()
{
    delete mscprov_;
}


#undef mErrRet
#define mErrRet(s) { errmsg = s; delete mscprov_; mscprov_= 0; return false; }

bool StorageProvider::init()
{
    if ( status!=Nada ) return false;

    const LineKey lk( desc.getValParam(keyStr())->getStringValue(0) );
    const MultiID mid( lk.lineName() );
    mscprov_ = new SeisMSCProvider( mid );

    if ( !initMSCProvider() )
	mErrRet( mscprov_->errMsg() )

    const bool is2d = mscprov_->is2D();
    desc.set2D( is2d );
    if ( !is2d )
	SeisTrcTranslator::getRanges( mid, storedvolume, lk );
    else
    {
	const Seis2DLineSet* lset = mscprov_->reader().lineSet();
	if ( !lset )
	    mErrRet( "2D seismic data/No line set found" );

	const int lineidx = lset->indexOf( lk.buf() );
	if ( lineidx == -1 )
	{
	    storedvolume.hrg.start.inl = storedvolume.hrg.start.crl = 0;
	    storedvolume.hrg.stop.inl = storedvolume.hrg.step.inl = 1;
	    storedvolume.hrg.stop.crl = SI().maxNrTraces(true);
	    storedvolume.hrg.step.crl = 1; // what else?
	    storedvolume.zrg = SI().sampling(true).zrg;
	}
	else
	{
	    storedvolume.hrg.start.inl = storedvolume.hrg.stop.inl = lineidx;
	    StepInterval<int> trcrg; StepInterval<float> zrg;
	    if ( !lset->getRanges( lineidx, trcrg, zrg ) )
		mErrRet("Cannot get needed trace range from 2D line set")
	    else
	    {
		storedvolume.hrg.start.crl = trcrg.start;
		storedvolume.hrg.stop.crl = trcrg.stop;
		storedvolume.zrg = zrg;
	    }
	}
    }

    status = StorageOpened;
    return true;
}


int StorageProvider::moveToNextTrace( BinID startpos, bool firstcheck )
{
    if ( alreadymoved )
	return 1;

    if ( status==Nada )
	return -1;

    if ( status==StorageOpened )
    {
	if ( !setMSCProvSelData() )
	    return -1;

	status = Ready;
    }

    if ( !useshortcuts_ )
    {
	if ( getDesc().is2D() )
	    prevtrcnr = currentbid.crl;

	bool validstartpos = startpos != BinID(-1,-1);
	if ( validstartpos && curtrcinfo_ && curtrcinfo_->binid == startpos )
	{
	    alreadymoved = true;
	    return 1;
	}
    }
    
    bool advancefurther = true;
    while ( advancefurther )
    {
	SeisMSCProvider::AdvanceState res = mscprov_->advance();
	switch ( res )
	{
	    case SeisMSCProvider::Error:	return -1;
	    case SeisMSCProvider::EndReached:	return 0;
	    case SeisMSCProvider::Buffering:	continue;
						//TODO return 'no new position'

	    case SeisMSCProvider::NewPosition:
	    {
		if ( useshortcuts_ )
		    { advancefurther = false; continue; }

		SeisTrc* trc = mscprov_->get( 0, 0 );
		if ( !trc ) continue; // should not happen

		for ( int idx=0; idx<trc->nrComponents(); idx++ )
		{
		    if ( datachar_.size()<=idx )
			datachar_ +=
			    trc->data().getInterpreter()->dataChar();
		    else
			datachar_[idx] =
			    trc->data().getInterpreter()->dataChar();
		}

		curtrcinfo_ = 0;
		const SeisTrcInfo& newti = trc->info();
		currentbid = desc.is2D()? BinID( 0, newti.nr ) 
					: newti.binid;
		trcinfobid = newti.binid;
		if ( firstcheck || startpos == BinID(-1,-1)
		  || currentbid == startpos || newti.binid == startpos )
		{
		    advancefurther = false;
		    curtrcinfo_ = &trc->info();
		}
	    }
	}
    }

    alreadymoved = true;
    return 1;
}


bool StorageProvider::getPossibleVolume( int, CubeSampling& res )
{
    if ( !possiblevolume ) 
	possiblevolume = new CubeSampling;
    
    *possiblevolume = storedvolume;
    res.limitToWithUdf( *possiblevolume );
    if ( res.hrg.inlRange().width(false)<0 || res.hrg.crlRange().width(false)<0)
	return false;

    return true;
}


bool StorageProvider::initMSCProvider()
{
    if ( !mscprov_ || !mscprov_->prepareWork() )
	return false;

    updateStorageReqs();
    return true;
}


#define setBufStepout( prefix ) \
{ \
    if ( ns.inl <= prefix##bufferstepout.inl \
	    && ns.crl <= prefix##bufferstepout.crl ) \
	return; \
\
    if ( ns.inl > prefix##bufferstepout.inl ) \
	prefix##bufferstepout.inl = ns.inl; \
    if ( ns.crl > prefix##bufferstepout.crl ) \
	prefix##bufferstepout.crl = ns.crl;\
}


void StorageProvider::setReqBufStepout( const BinID& ns, bool wait )
{
    setBufStepout(req);
    if ( !wait )
	updateStorageReqs();
}


void StorageProvider::setDesBufStepout( const BinID& ns, bool wait )
{
    setBufStepout(des);
    if ( !wait )
	updateStorageReqs();
}


void StorageProvider::updateStorageReqs( bool )
{
    if ( !mscprov_ ) return;

    mscprov_->setStepout( desbufferstepout.inl, desbufferstepout.crl, false );
    mscprov_->setStepout( reqbufferstepout.inl, reqbufferstepout.crl, true );
}


bool StorageProvider::setMSCProvSelData()
{
    SeisTrcReader& reader = mscprov_->reader();
    if ( reader.psIOProv() ) return false;

    const bool is2d = reader.is2D();
    const bool haveseldata = seldata_ && !seldata_->all_;

    if ( haveseldata && seldata_->type_ == Seis::Table )
	return setTableSelData();

    if ( is2d )
	return set2DRangeSelData(); 

    if ( !desiredvolume )
    {
	for ( int idp=0; idp<parents.size(); idp++ )
	{
	    if ( !parents[idp] ) continue;
	    desiredvolume = parents[idp]->getDesiredVolume();
	    if ( desiredvolume ) break;
	}
	if ( !desiredvolume )
	    return true;
    }

    if ( !checkDesiredVolumeOK() )
	return false;

    CubeSampling cs;
    cs.hrg.start.inl = 
	    desiredvolume->hrg.start.inl < storedvolume.hrg.start.inl ?
	    storedvolume.hrg.start.inl : desiredvolume->hrg.start.inl;
    cs.hrg.stop.inl = 
	    desiredvolume->hrg.stop.inl > storedvolume.hrg.stop.inl ?
	    storedvolume.hrg.stop.inl : desiredvolume->hrg.stop.inl;
    cs.hrg.stop.crl = 
	    desiredvolume->hrg.stop.crl > storedvolume.hrg.stop.crl ?
	    storedvolume.hrg.stop.crl : desiredvolume->hrg.stop.crl;
    cs.hrg.start.crl = 
	    desiredvolume->hrg.start.crl < storedvolume.hrg.start.crl ?
	    storedvolume.hrg.start.crl : desiredvolume->hrg.start.crl;
    cs.zrg.start = desiredvolume->zrg.start < storedvolume.zrg.start ?
		    storedvolume.zrg.start : desiredvolume->zrg.start;
    cs.zrg.stop = desiredvolume->zrg.stop > storedvolume.zrg.stop ?
		     storedvolume.zrg.stop : desiredvolume->zrg.stop;

    SeisSelData* seldata = seldata_ ? new SeisSelData( *seldata_ )
				    : new SeisSelData( true );
    seldata->inlrg_.start = cs.hrg.start.inl;
    seldata->inlrg_.stop = cs.hrg.stop.inl;
    seldata->crlrg_.start = cs.hrg.start.crl;
    seldata->crlrg_.stop = cs.hrg.stop.crl;
    seldata->zrg_.start = cs.zrg.start;
    seldata->zrg_.stop = cs.zrg.stop;
    reader.setSelData( seldata );

    SeisTrcTranslator* transl = reader.seisTranslator();
    for ( int idx=0; idx<outputinterest.size(); idx++ )
    {
	if ( !outputinterest[idx] ) 
	    transl->componentInfo()[idx]->destidx = -1;
    }

    return true;
}


bool StorageProvider::setTableSelData()
{
    SeisSelData* seldata = new SeisSelData( *seldata_ );
    seldata->extraz_ = extraz_;
    SeisTrcReader& reader = mscprov_->reader();
    if ( reader.is2D() )
    {
	const LineKey lk( desc.getValParam(keyStr())->getStringValue(0) );
	seldata->linekey_.setAttrName( lk.attrName() );
    }
    reader.setSelData( seldata );
    return true;
}


bool StorageProvider::set2DRangeSelData()
{
    SeisSelData* seldata = seldata_ ? new SeisSelData( *seldata_ )
				    : new SeisSelData( true );
    SeisTrcReader& reader = mscprov_->reader();
    Seis2DLineSet* lset = reader.lineSet();
    if ( !lset )
	return false;

    seldata->linekey_.setAttrName( curlinekey_.attrName() );
    StepInterval<int> trcrg;
    StepInterval<float> zrg;
    if ( !curlinekey_.lineName().isEmpty() )
    {
	seldata->linekey_.setLineName( curlinekey_.lineName() );
	int idx = lset->indexOf( curlinekey_ );
	if ( idx >= 0 && lset->getRanges(idx,trcrg,zrg) )
	{
	    if ( !checkDesiredTrcRgOK(trcrg,zrg) )
		return false;
	    seldata->crlrg_.start = 
			desiredvolume->hrg.start.crl < trcrg.start?
			trcrg.start : desiredvolume->hrg.start.crl;
	    seldata->crlrg_.stop = 
			desiredvolume->hrg.stop.crl > trcrg.stop ?
			trcrg.stop : desiredvolume->hrg.stop.crl;
	    seldata->zrg_.start = 
			desiredvolume->zrg.start < zrg.start ?
			zrg.start : desiredvolume->zrg.start;
	    seldata->zrg_.stop = 
			desiredvolume->zrg.stop > zrg.stop ?
			zrg.stop : desiredvolume->zrg.stop;
	    seldata->inlrg_.stop = seldata->inlrg_.start = 0;
	}
	reader.setSelData( seldata );
    }

    return true;
}


#define mInitErrMsg() \
    errmsg = "'"; errmsg += desc.userRef(); errmsg += "'"; \
    errmsg += " contains no data in selected area:\n"

#define mAdd2ErrMsg(varwrong,s,start,stop) \
    if ( varwrong ) \
    { \
	errmsg += s; errmsg += " range is: "; \
	errmsg += start; errmsg += "-"; errmsg += stop; \
	errmsg += "\n"; \
    }

bool StorageProvider::checkDesiredVolumeOK()
{
    if ( !desiredvolume )
	return true;

    const bool inlwrong =
	desiredvolume->hrg.start.inl > storedvolume.hrg.stop.inl
     || desiredvolume->hrg.stop.inl < storedvolume.hrg.start.inl;
    const bool crlwrong =
	desiredvolume->hrg.start.crl > storedvolume.hrg.stop.crl
     || desiredvolume->hrg.stop.crl < storedvolume.hrg.start.crl;
    const bool zwrong =
	desiredvolume->zrg.start > storedvolume.zrg.stop
     || desiredvolume->zrg.stop < storedvolume.zrg.start;

    if ( !inlwrong && !crlwrong && !zwrong )
	return true;

    mInitErrMsg();
    mAdd2ErrMsg(inlwrong,"Inline",
	    	storedvolume.hrg.start.inl,storedvolume.hrg.stop.inl)
    mAdd2ErrMsg(crlwrong,"Crossline",
	    	storedvolume.hrg.start.crl,storedvolume.hrg.stop.crl)
    mAdd2ErrMsg(zwrong,"Z",storedvolume.zrg.start,storedvolume.zrg.stop)
    return false;
}


bool StorageProvider::checkDesiredTrcRgOK( StepInterval<int> trcrg, 
				   StepInterval<float>zrg )
{
    const bool trcrgwrong =
	desiredvolume->hrg.start.crl > trcrg.start
     || desiredvolume->hrg.stop.crl < trcrg.stop;
    const bool zwrong =
	desiredvolume->zrg.start > zrg.stop
     || desiredvolume->zrg.stop < zrg.start;

    if ( !trcrgwrong && !zwrong )
	return true;

    mInitErrMsg();
    mAdd2ErrMsg(trcrgwrong,"Trace range",trcrg.start,trcrg.stop)
    mAdd2ErrMsg(zwrong,"Z",zrg.start,zrg.stop)
    return false;
}


bool StorageProvider::computeData( const DataHolder& output,
				   const BinID& relpos,
				   int z0, int nrsamples, int threadid ) const
{
    const BinID bidstep = getStepoutStep();
    const SeisTrc* trc = mscprov_->get( relpos.inl/bidstep.inl, 
	    				relpos.crl/bidstep.crl );
    if ( !trc )
	return false;

    return fillDataHolderWithTrc( trc, output );
}


bool StorageProvider::fillDataHolderWithTrc( const SeisTrc* trc, 
					     const DataHolder& data ) const
{
    const int z0 = data.z0_;
    int offset = 0;
    float exacttime = 0;
    BoolTypeSet isclass( outputinterest.size(), true );
    if ( needinterp )
    {
	int intvidx = localcomputezintervals.indexOf( 
				    Interval<int>( z0, z0+data.nrsamples_-1) );
	exacttime = exactz_[intvidx];
	offset = mNINT( z0 - exacttime/refstep + 0.5 );
	checkClassType( trc, isclass );
    }
    
    Interval<float> trcrange = trc->info().sampling.interval(trc->size());
    trcrange.widen( 0.001 * trc->info().sampling.step );
    for ( int idx=0; idx<data.nrsamples_; idx++ )
    {
	const float curt = needinterp ? exacttime + (offset+idx)*refstep 
				      : (z0+idx)*refstep;
	int compidx = -1;
	for ( int idy=0; idy<outputinterest.size(); idy++ )
	{
	    if ( outputinterest[idy] )
	    {
		compidx++;
		const float val = trcrange.includes(curt) ? 
		   ( isclass[idy] ? trc->get(trc->nearestSample(curt), compidx)
				  : trc->getValue(curt, compidx) )
		   : mUdf(float);

		const_cast<DataHolder&>(data).series(idy)->setValue(idx,val);
	    }
	}
    }

    return true;
}


BinDataDesc StorageProvider::getOutputFormat( int output ) const
{
    if ( output>=datachar_.size() )
	return Provider::getOutputFormat( output );

    return datachar_[output];
}


BinID StorageProvider::getStepoutStep() const
{
    if ( !mscprov_ )
	return BinID(0,0);

    BinID ret; mscprov_->reader().getSteps( ret.inl, ret.crl );
    return ret;
}


void StorageProvider::adjust2DLineStoredVolume()
{
    const SeisTrcReader& reader = mscprov_->reader();
    if ( !reader.is2D() ) return;

    const Seis2DLineSet* lset = reader.lineSet();
    const int idx = lset->indexOf( curlinekey_ );
    StepInterval<int> trcrg;
    StepInterval<float> zrg;
    if ( idx >= 0 && lset->getRanges(idx,trcrg,zrg) )
    {
	storedvolume.hrg.start.crl = trcrg.start;
	storedvolume.hrg.stop.crl = trcrg.stop;
	storedvolume.zrg.start = zrg.start;
	storedvolume.zrg.stop = zrg.stop;
	storedvolume.zrg.step = zrg.step;
    }
}


void StorageProvider::fillDataCubesWithTrc( DataCubes* dc ) const
{
    const SeisTrc* trc = mscprov_->get(0,0);
    if ( !trc ) return;

    Interval<float> trcrange = trc->info().sampling.interval(trc->size());
    trcrange.widen( 0.001 * trc->info().sampling.step );
    const BinID bid = trc->info().binid;
    if ( !dc->includes(bid) )
	return;

    const int inlidx = dc->inlsampling.nearestIndex( bid.inl );
    const int crlidx = dc->crlsampling.nearestIndex( bid.crl );
    for ( int idz=0; idz<dc->getZSz(); idz++ )
    {
	const float curt = (dc->z0+idz) * dc->zstep;
	int cubeidx = -1;
	for ( int idx=0; idx<outputinterest.size(); idx++ )
	{
	    if ( !outputinterest[idx] )
		continue;

	    cubeidx++;
	    if ( cubeidx >= dc->nrCubes() )
		dc->addCube(mUdf(float));

	    if ( !trcrange.includes(curt) )
		continue;

	    const float val = trc->getValue( curt, idx );
	    dc->setValue( cubeidx, inlidx, crlidx, idz, val );
	}
    }
} 


void StorageProvider::checkClassType( const SeisTrc* trc,
				      BoolTypeSet& isclass ) const
{
    int idx = 0;
    bool foundneed = true;
    while ( idx<trc->size() && foundneed )
    {
	foundneed = false;
	for ( int ido=0; ido<outputinterest.size(); ido++ )
	{
	    if ( outputinterest[ido] && isclass[ido] )
	    {
		foundneed = true;
		const float val  = trc->get( idx, ido );
		if ( !holdsClassValue( val) )
		    isclass[ido] = false;
	    }
	}
	idx++;
    }
}

}; // namespace Attrib
