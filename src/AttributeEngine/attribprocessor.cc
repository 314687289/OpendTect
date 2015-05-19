/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID mUsedVar = "$Id$";

#include "attribprocessor.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attriboutput.h"
#include "attribprovider.h"
#include "cubesampling.h"
#include "linekey.h"
#include "seisinfo.h"
#include "seisselectionimpl.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "binidvalset.h"

#include <limits.h>


namespace Attrib
{

Processor::Processor( Desc& desc , const char* lk, uiString& err )
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

    is2d_ = desc_.is2D();
    if ( is2d_ )
	provider_->setCurLineName( lk );
}


Processor::~Processor()
{
    if ( provider_ )  { provider_->unRef(); desc_.unRef(); }
    deepUnRef( outputs_ );

    if (sd_) delete sd_;
}


bool Processor::isOK() const { return provider_ && provider_->isOK(); }


void Processor::addOutput( Output* output )
{
    if ( !output ) return;
    output->ref();
    outputs_ += output;
}


void Processor::setLineName( const char* lnm )
{
    if ( provider_ && is2d_ )
	provider_->setCurLineName( lnm );
}


int Processor::nextStep()
{
    if ( !provider_ || outputs_.isEmpty() ) return ErrorOccurred();

    if ( !isinited_ )
	init();

    if ( !errmsg_.isEmpty() )
	return ErrorOccurred();

    if ( !provider_->errMsg().isEmpty() )
    {
	errmsg_ = provider_->errMsg();
	return ErrorOccurred();
    }

    if ( useshortcuts_ ) 
	provider_->setUseSC();
    
    int res;
    res = provider_->moveToNextTrace();
    if ( res < 0 || !nriter_ )
    {
	errmsg_ = provider_->errMsg();
	if ( !errmsg_.isEmpty() )
	    return ErrorOccurred();
    }
    useshortcuts_ ? useSCProcess( res ) : useFullProcess( res );
    if ( !errmsg_.isEmpty() )
	return ErrorOccurred();

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

	if ( sd_ && sd_->type() == Seis::Table )
	{
	    mDynamicCastGet(Seis::TableSelData*,tsd,sd_)
	    firstpos = tsd->binidValueSet().firstBinID();
	}
	else
	{
	    const BinID step = provider_->getStepoutStep();
	    firstpos.inl() = step.inl()/abs(step.inl())>0 ? 
			   provider_->getDesiredVolume()->hrg.start.inl() : 
			   provider_->getDesiredVolume()->hrg.stop.inl();
	    firstpos.crl() = step.crl()/abs(step.crl())>0 ?
			   provider_->getDesiredVolume()->hrg.start.crl() :
			   provider_->getDesiredVolume()->hrg.stop.crl();
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
    if ( !curtrcinfo && provider_->needStoredInput() )
    {
	errmsg_ = "No trace info available";
	return;
    }

    if ( res == 0 && !nrdone_ )
    {
	errmsg_ = "No positions processed.\n"
	"Most probably, your input volume(s) are not available in the \n"
	"selected region or the required stepout traces are not available";
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
	mDynamicCastGet( TableOutput*, taboutp, outputs_[0] );
	if ( locoutp || taboutp ) 
	    curbid = curtrcinfo->binid;
	else
	{
	    curbid.inl() = 0;
	    curbid.crl() = curtrcinfo->nr;
	}
    }

    SeisTrcInfo mytrcinfo;
    if ( !curtrcinfo )
    {
	mytrcinfo.binid = curbid;
	if ( is2d_ )
	{
	    mytrcinfo.nr = curbid.crl();
	    const Pos::GeomID geomid = provider_->getGeomID();
	    mDynamicCastGet( const Survey::Geometry2D*, geom2d,
		    	     Survey::GM().getGeometry(geomid) );
	    PosInfo::Line2DPos pos2d;
	    if ( geom2d && geom2d->data().getPos(curbid.crl(),pos2d) )
		mytrcinfo.coord = pos2d.coord_;
	    else
		mytrcinfo.coord = SI().transform( mytrcinfo.binid );
	}

	curtrcinfo = &mytrcinfo;
    }

    TypeSet< Interval<int> > localintervals;
    bool isset = setZIntervals( localintervals, curbid, curtrcinfo->coord );

    for ( int idi=0; idi<localintervals.size(); idi++ )
    {
	const DataHolder* data = isset ? 
				provider_->getData( BinID(0,0), idi ) : 0;
	if ( data )
	{
	    for ( int idx=0; idx<outputs_.size(); idx++ )
		outputs_[idx]->collectData( *data, provider_->getRefStep(),
					    *curtrcinfo );
	}
	
	if ( isset )
	    nrdone_++;
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
    if ( is2d_ )
    {
	provider_->adjust2DLineStoredVolume();
	provider_->compDistBetwTrcsStats();
	mDynamicCastGet( Trc2DVarZStorOutput*, trcvarzoutp, outputs_[0] );
	mDynamicCastGet( TableOutput*, taboutp, outputs_[0] );
	if ( trcvarzoutp || taboutp )
	{
	    float maxdist = provider_->getDistBetwTrcs(true);
	    if ( trcvarzoutp ) trcvarzoutp->setMaxDistBetwTrcs( maxdist );
	    if ( taboutp ) taboutp->setMaxDistBetwTrcs( maxdist );
	}

    }
    computeAndSetRefZStepAndZ0();
    provider_->prepPriorToBoundsCalc();    

    prepareForTableOutput();

    for ( int idx=0; idx<globaloutputinterest.size(); idx++ )
	provider_->enableOutput(globaloutputinterest[idx], true );

    //Special case for attributes (like PreStack) which inputs are not treated
    //as normal input cubes and thus not delivering adequate cs automaticly
    provider_->updateCSIfNeeded(globalcs);

    computeAndSetPosAndDesVol( globalcs );
    for ( int idx=0; idx<outputs_.size(); idx++ )
	outputs_[idx]->adjustInlCrlStep( *provider_->getPossibleVolume() );

    mDynamicCastGet( DataCubesOutput*, dcoutp, outputs_[0] );
    if ( dcoutp && provider_->getDesc().isStored() )
    	useshortcuts_ = true;
    else
	provider_->prepareForComputeData();

    isinited_ = true;
}


void Processor::defineGlobalOutputSpecs( TypeSet<int>& globaloutputinterest,
				      	 CubeSampling& globalcs )
{
    bool ovruleoutp = provider_->prepPriorToOutputSetup();
    if ( ovruleoutp )
    {
	for ( int idx=0; idx<provider_->nrOutputs(); idx++ )
	    if ( provider_->isOutputEnabled(idx) )
		outpinterest_.addIfNew( idx );
    }
    
    for ( int idx=0; idx<outputs_.size(); idx++ )
    {
	CubeSampling cs;
	cs.zrg.start = 0;    //cover up for synthetics
	if ( !outputs_[idx]->getDesiredVolume(cs) )
	{
	    outputs_[idx]->unRef();
	    outputs_.removeSingle(idx);
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
	    if ( !globaloutputinterest.isPresent(outpinterest_[idy]) )
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


void Processor::computeAndSetRefZStepAndZ0()
{
    provider_->computeRefStep();
    const float zstep = provider_->getRefStep();
    provider_->setRefStep( zstep );

    provider_->computeRefZ0();
    const float z0 = provider_->getRefZ0();
    provider_->setRefZ0( z0, true );
}


void Processor::prepareForTableOutput()
{
    if ( outputs_.size() && outputs_[0]->getSelData().type() == Seis::Table )
    {
	for ( int idx=0; idx<outputs_.size(); idx++ )
	{
	    if ( !idx )
		sd_ = outputs_[0]->getSelData().clone();
	    else
		sd_->include( outputs_[idx]->getSelData() );
	}
    }

    if ( sd_ && sd_->type() == Seis::Table )
    {
	provider_->setSelData( sd_ );
	mDynamicCastGet( LocationOutput*, locoutp, outputs_[0] );
	mDynamicCastGet( TableOutput*, taboutp, outputs_[0] );
	if ( locoutp || taboutp )
	{
	    Interval<float> extraz( -2*provider_->getRefStep(), 
		    		    2*provider_->getRefStep() );
	    provider_->setExtraZ( extraz );
	    provider_->setNeedInterpol(true);
	}
    }

    if ( outputs_.size() >1 )
    {
	for ( int idx=0; idx<outputs_.size(); idx++ )
	{
	    mDynamicCastGet( LocationOutput*, locoutp, outputs_[0] );
	    if ( locoutp )
		locoutp->setPossibleBinIDDuplic();
	    else
	    {
		mDynamicCastGet( TableOutput*, taboutp, outputs_[0] );
		if ( taboutp )
		    taboutp->setPossibleBinIDDuplic();
	    }
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
	else if ( is2d_ && possvol == globalcs )
	{
	    possvol.hrg.stop.inl() = possvol.hrg.start.inl() = 0;
	    possvol.hrg.start.crl() = 0;
	    possvol.hrg.stop.crl() = INT_MAX/3*2;//unlikely;still, a limitation.
	    globalcs = possvol;
	}

	provider_->setDesiredVolume( possvol );
	if ( !provider_->getPossibleVolume( -1, possvol ) )
	{
	    errmsg_="Not possible to output required attribute in this area.\n"
		    "Please confront stepouts/timegates with available data";
	    return;
	}

	provider_->resetDesiredVolume();
	globalcs.limitToWithUdf( possvol );
	
	provider_->setDesiredVolume( globalcs );
    }
}


bool Processor::setZIntervals( TypeSet< Interval<int> >& localintervals, 
			       const BinID& curbid, const Coord& curcoords )
{
    //TODO: Smarter way if output's intervals don't intersect
    bool isset = false;
    TypeSet<float> exactz;
    mDynamicCastGet( TableOutput*, taboutp, outputs_[0] );
    mDynamicCastGet( Trc2DVarZStorOutput*, trc2dvarzoutp, outputs_[0] );
    for ( int idx=0; idx<outputs_.size(); idx++ )
    {
	bool wantout = outputs_[idx]->useCoords()
	    			? outputs_[idx]->wantsOutput(curcoords)
				: outputs_[idx]->wantsOutput(curbid);

	if ( ( taboutp || trc2dvarzoutp ) && is2d_ )		//tmp patch
	    wantout = true;

	if ( !wantout || (curbid == prevbid_ && !is2d_) ) //!is2d = tmp patch 
	    continue;

	const float refzstep = provider_->getRefStep();
	TypeSet< Interval<int> > localzrange = outputs_[idx]->useCoords()
	    	? outputs_[idx]->getLocalZRanges( curcoords, refzstep, exactz )
		: outputs_[idx]->getLocalZRanges( curbid, refzstep, exactz );
	if ( isset )
	    localintervals.append ( localzrange );
	else
	{
	    localintervals = localzrange;
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


od_int64 Processor::totalNr() const
{
    return provider_ ? provider_->getTotalNrPos(is2d_) : 0;
}


od_int64 Processor::nrDone() const
{ return nrdone_; }


uiString Processor::uiMessage() const
{ return errmsg_.isEmpty() ? "Processing" : errmsg_; }


void Processor::addOutputInterest( int sel )
{ outpinterest_.addIfNew( sel ); }


const char* Processor::getAttribName() const
{
    return desc_.attribName();
}


const char* Processor::getAttribUserRef() const
{
    return desc_.userRef();
}


void Processor::setRdmPaths( TypeSet<BinID>* truepath,
			     TypeSet<BinID>* snappedpath )
{
    if ( !provider_ ) return;

    provider_->setRdmPaths( truepath, snappedpath );
}

}; // namespace Attrib
