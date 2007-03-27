/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attribprocessor.cc,v 1.48 2007-03-27 16:30:40 cvshelene Exp $";

#include "attribprocessor.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attriboutput.h"
#include "attribprovider.h"
#include "cubesampling.h"
#include "linekey.h"
#include "seisinfo.h"
#include "seistrcsel.h"


namespace Attrib
{

Processor::Processor( Desc& desc , const char* lk, BufferString& err )
    : Executor("Attribute Processor")
    , desc_(desc)
    , provider_(Provider::create(desc,err))
    , nriter_(0)
    , nrdone_(0)
    , isinited_(false)
    , useshortcuts_(false)
    , moveonly(this)
    , prevbid_(BinID(-1,-1))
    , sd_(0)
{
    if ( !provider_ ) return;
    provider_->ref();
    desc_.ref();

    is2d_ = desc_.descSet()->is2D();
    if ( is2d_ )
	provider_->setCurLineKey( lk );
}


Processor::~Processor()
{
    if ( provider_ )  { provider_->unRef(); desc_.unRef(); }
    deepUnRef( outputs_ );

    if (sd_) delete sd_;
}


bool Processor::isOK() const { return provider_; }


void Processor::addOutput( Output* output )
{
    if ( !output ) return;
    output->ref();
    outputs_ += output;
}


int Processor::nextStep()
{
    if ( !provider_ || outputs_.isEmpty() ) return ErrorOccurred;

    if ( !isinited_ )
	init();

    if ( useshortcuts_ ) 
	provider_->setUseSC();
    
    int res;
    res = provider_->moveToNextTrace();
    if ( res < 0 || !nriter_ )
    {
	errmsg_ = provider_->errMsg().buf();
	if ( errmsg_.size() )
	    return ErrorOccurred;
    }
    useshortcuts_ ? useSCProcess( res ) : useFullProcess( res );
    if ( errmsg_.size() )
	return ErrorOccurred;

    provider_->resetMoved();
    provider_->resetZIntervals();
    nriter_++;
    return res;
}
    

void Processor::useFullProcess( int& res )
{
    if ( res < 0 )
    {
	BinID firstpos;

	if ( sd_ && sd_->type_ == Seis::Table )
	    firstpos = sd_->table_.firstPos();
	else
	{
	    const BinID step = provider_->getStepoutStep();
	    firstpos.inl = step.inl/abs(step.inl)>0 ? 
			   provider_->getDesiredVolume()->hrg.start.inl : 
			   provider_->getDesiredVolume()->hrg.stop.inl;
	    firstpos.crl = step.crl/abs(step.crl)>0 ?
			   provider_->getDesiredVolume()->hrg.start.crl :
			   provider_->getDesiredVolume()->hrg.stop.crl;
	}
	provider_->resetMoved();
	res = provider_->moveToNextTrace( firstpos, true );
	
	if ( res < 0 )
	{
	    errmsg_ = "Error during data read";
	    return;
	}
    }

    provider_->updateCurrentInfo();
    const SeisTrcInfo* curtrcinfo = provider_->getCurrentTrcInfo();
    const bool needsinput = !provider_->getDesc().isStored() && 
			    provider_->getDesc().nrInputs();
    if ( !curtrcinfo && needsinput )
    {
	errmsg_ = "No trace info available";
	return;
    }

    if ( res == 0 && !nrdone_ )
    {
	errmsg_ = "No position to process.\n";
	errmsg_ += "You may not be in the possible volume,\n";
	errmsg_ += "mind the stepout...";
	return;
    }
    else if ( res != 0 )
	fullProcess( curtrcinfo );
}


void Processor::fullProcess( const SeisTrcInfo* curtrcinfo )
{
    BinID curbid = provider_->getCurrentPosition();
    if ( is2d_ && curtrcinfo )
    {
	mDynamicCastGet( LocationOutput*, locoutp, outputs_[0] );
	if ( locoutp ) 
	    curbid = curtrcinfo->binid;
	else
	{
	    curbid.inl = 0;
	    curbid.crl = curtrcinfo->nr;
	}
    }

    SeisTrcInfo mytrcinfo;
    if ( !curtrcinfo )
    {
	mytrcinfo.binid = curbid;
	if ( is2d_ ) mytrcinfo.nr = curbid.crl;

	curtrcinfo = &mytrcinfo;
    }

    TypeSet< Interval<int> > localintervals;
    bool isset = setZIntervals( localintervals, curbid );

    for ( int idi=0; idi<localintervals.size(); idi++ )
    {
	const DataHolder* data = isset ? 
				provider_->getData( BinID(0,0), idi ) : 0;
	if ( data )
	{
	    for ( int idx=0; idx<outputs_.size(); idx++ )
		outputs_[idx]->collectData( *data, provider_->getRefStep(),
					    *curtrcinfo );
	    if ( isset )
		nrdone_++;
	}
    }

    prevbid_ = curbid;
}


void Processor::useSCProcess( int& res )
{
    if ( res < 0 )
    {
	errmsg_ = "Error during data read";
	return;
    }
    if ( res == 0 && !nrdone_ )
    {
	errmsg_ = "This stored cube contains no data in selected area.\n";
	return;
    }

    if ( res == 0 ) return;

    for ( int idx=0; idx<outputs_.size(); idx++ )
	provider_->fillDataCubesWithTrc( 
			outputs_[idx]->getDataCubes(provider_->getRefStep() ) );

    nrdone_++;
}


void Processor::init()
{
    TypeSet<int> globaloutputinterest;
    CubeSampling globalcs;
    defineGlobalOutputSpecs( globaloutputinterest, globalcs );
    if ( is2d_ ) provider_->adjust2DLineStoredVolume();
    computeAndSetRefZStep();

    prepareForTableOutput();

    for ( int idx=0; idx<globaloutputinterest.size(); idx++ )
	provider_->enableOutput(globaloutputinterest[idx], true );

    computeAndSetPosAndDesVol( globalcs );
    mDynamicCastGet( DataCubesOutput*, dcoutp, outputs_[0] );
    if ( dcoutp && provider_->getDesc().isStored() )
    	useshortcuts_ = true;
    else
    {
	for ( int idx=0; idx<outputs_.size(); idx++ )
	    outputs_[idx]->adjustInlCrlStep( *provider_->getPossibleVolume() );

	provider_->prepareForComputeData();
    }
    isinited_ = true;
}


void Processor::defineGlobalOutputSpecs( TypeSet<int>& globaloutputinterest,
				      	 CubeSampling& globalcs )
{
    for ( int idx=0; idx<outputs_.size(); idx++ )
    {
	CubeSampling cs;
	if ( !outputs_[idx]->getDesiredVolume(cs) )
	{
	    outputs_[idx]->unRef();
	    outputs_.remove(idx);
	    idx--;
	    continue;
	}

	if ( !idx )
	    globalcs = cs;
	else
	{
	    globalcs.hrg.include(cs.hrg.start);
	    globalcs.hrg.include(cs.hrg.stop);
	    globalcs.zrg.include(cs.zrg);
	}

	for ( int idy=0; idy<outpinterest_.size(); idy++ )
	{
	    if ( globaloutputinterest.indexOf(outpinterest_[idy])==-1 )
		globaloutputinterest += outpinterest_[idy];
	}
	outputs_[idx]->setDesiredOutputs( outpinterest_ );

	mDynamicCastGet( SeisTrcStorOutput*, storoutp, outputs_[0] );
	if ( storoutp )
	{
	    TypeSet<Seis::DataType> outptypes;
	    for ( int ido=0; ido<outpinterest_.size(); ido++ )
		outptypes += provider_->getDesc().dataType(outpinterest_[ido]);

	    for ( int idoutp=0; idoutp<outputs_.size(); idoutp++ )
		((SeisTrcStorOutput*)outputs_[idoutp])->setOutpTypes(outptypes);
	}
    }
}


void Processor::computeAndSetRefZStep()
{
    provider_->computeRefStep();
    const float zstep = provider_->getRefStep();
    provider_->setRefStep( zstep );
}


void Processor::prepareForTableOutput()
{
    if ( outputs_.size() && outputs_[0]->getSelData().type_==Seis::Table )
    {
	for ( int idx=0; idx<outputs_.size(); idx++ )
	{
	    if ( !idx ) sd_ = new SeisSelData(outputs_[0]->getSelData());
	    else sd_->include( outputs_[idx]->getSelData() );
	}
    }

    if ( sd_ && sd_->type_ == Seis::Table )
    {
	provider_->setSelData( sd_ );
	mDynamicCastGet( LocationOutput*, locoutp, outputs_[0] );
	if ( locoutp )
	{
	    Interval<float> extraz( -2*provider_->getRefStep(), 
		    		    2*provider_->getRefStep() );
	    provider_->setExtraZ( extraz );
	    provider_->setNeedInterpol(true);
	}
    }
}


void Processor::computeAndSetPosAndDesVol( CubeSampling& globalcs )
{
    if ( provider_->getInputs().isEmpty() && !provider_->getDesc().isStored() )
    {
	provider_->setDesiredVolume( globalcs );
	provider_->setPossibleVolume( globalcs );
    }
    else
    {
	CubeSampling possvol;
	if ( !possvol.includes(globalcs) )
	    possvol = globalcs;
	provider_->setDesiredVolume( possvol );
	provider_->getPossibleVolume( -1, possvol );
	provider_->resetDesiredVolume();

#       define mAdjustIf(v1,op,v2) \
	if ( !mIsUdf(v1) && !mIsUdf(v2) && v1 op v2 ) v1 = v2;
	
	mAdjustIf(globalcs.hrg.start.inl,<,possvol.hrg.start.inl);
	mAdjustIf(globalcs.hrg.start.crl,<,possvol.hrg.start.crl);
	mAdjustIf(globalcs.zrg.start,<,possvol.zrg.start);
	mAdjustIf(globalcs.hrg.stop.inl,>,possvol.hrg.stop.inl);
	mAdjustIf(globalcs.hrg.stop.crl,>,possvol.hrg.stop.crl);
	mAdjustIf(globalcs.zrg.stop,>,possvol.zrg.stop);
	
	provider_->setDesiredVolume( globalcs );
    }
}


bool Processor::setZIntervals( TypeSet< Interval<int> >& localintervals, 
			       BinID curbid )
{
    //TODO: Smarter way if output's intervals don't intersect
    bool isset = false;
    TypeSet<float> exactz;
    for ( int idx=0; idx<outputs_.size(); idx++ )
    {
	if ( !outputs_[idx]->wantsOutput(curbid) || curbid == prevbid_ ) 
	    continue;

	if ( isset )
	    localintervals.append ( outputs_[idx]->
		getLocalZRanges( curbid, provider_->getRefStep(), exactz ) );
	else
	{
	    localintervals = outputs_[idx]->
		getLocalZRanges( curbid, provider_->getRefStep(), exactz );
	    isset = true;
	}
    }

    if ( isset ) 
    {
	provider_->addLocalCompZIntervals( localintervals );
	if ( !exactz.isEmpty() )
	    provider_->setExactZ( exactz );
    }

    return isset;
}


int Processor::totalNr() const
{
    return provider_ ? provider_->getTotalNrPos(is2d_) : 0;
}


const char* Processor::getAttribName()
{
    return desc_.attribName();
}


}; // namespace Attrib
