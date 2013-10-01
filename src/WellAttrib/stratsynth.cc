/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          July 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "stratsynth.h"
#include "syntheticdataimpl.h"
#include "stratsynthlevel.h"

#include "array1dinterpol.h"
#include "angles.h"
#include "arrayndimpl.h"
#include "attribsel.h"
#include "attribengman.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribparam.h"
#include "attribprocessor.h"
#include "attribfactory.h"
#include "attribsel.h"
#include "attribstorprovider.h"
#include "binidvalset.h"
#include "datapackbase.h"
#include "elasticpropsel.h"
#include "fourier.h"
#include "fftfilter.h"
#include "flatposdata.h"
#include "ioman.h"
#include "mathfunc.h"
#include "prestackattrib.h"
#include "prestackgather.h"
#include "prestackanglecomputer.h"
#include "propertyref.h"
#include "raytracerrunner.h"
#include "separstr.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "survinfo.h"
#include "statruncalc.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "synthseis.h"
#include "timeser.h"
#include "wavelet.h"

static const char* sKeyIsPreStack()		{ return "Is Pre Stack"; }
static const char* sKeySynthType()		{ return "Synthetic Type"; }
static const char* sKeyWaveLetName()		{ return "Wavelet Name"; }
static const char* sKeyRayPar() 		{ return "Ray Parameter"; } 
static const char* sKeyDispPar() 		{ return "Display Parameter"; } 
static const char* sKeyColTab() 		{ return "Color Table"; } 
static const char* sKeyInput()	 		{ return "Input Synthetic"; } 
static const char* sKeyAngleRange()		{ return "Angle Range"; } 
#define sDefaultAngleRange Interval<float>( 0.0f, 30.0f )


DefineEnumNames(SynthGenParams,SynthType,0,"Synthetic Type")
{ "Pre Stack", "Zero Offset Stack", "Angle Stack", "AVO Gradient", 0 };

SynthGenParams::SynthGenParams()
{
    synthtype_ = PreStack;	//init to avoid nasty crash in generateSD!
    anglerg_ = sDefaultAngleRange;
    const BufferStringSet& facnms = RayTracer1D::factory().getNames( false );
    if ( !facnms.isEmpty() )
	raypars_.set( sKey::Type(), facnms.get(0) );

    RayTracer1D::setIOParsToZeroOffset( raypars_ );
}


bool SynthGenParams::hasOffsets() const
{
    TypeSet<float> offsets;
    raypars_.get( RayTracer1D::sKeyOffset(), offsets );
    return offsets.size()>1;
}


void SynthGenParams::fillPar( IOPar& par ) const
{
    par.set( sKey::Name(), name_ );
    par.set( sKeySynthType(), SynthGenParams::toString(synthtype_) );
    if ( synthtype_ == SynthGenParams::AngleStack ||
	 synthtype_ == SynthGenParams::AVOGradient )
    {
	par.set( sKeyInput(), inpsynthnm_ );
	par.set( sKeyAngleRange(), anglerg_ );
    }
    par.set( sKeyWaveLetName(), wvltnm_ );
    IOPar raypar;
    raypar.mergeComp( raypars_, sKeyRayPar() );
    par.merge( raypar );
}


void SynthGenParams::usePar( const IOPar& par ) 
{
    par.get( sKey::Name(), name_ );
    par.get( sKeyWaveLetName(), wvltnm_ );
    PtrMan<IOPar> raypar = par.subselect( sKeyRayPar() );
    raypars_ = *raypar;
    if ( par.hasKey( sKeyIsPreStack()) )
    {
	bool isps = false;
	par.getYN( sKeyIsPreStack(), isps );
	if ( !isps && hasOffsets() )
	    synthtype_ = SynthGenParams::AngleStack;
	else if ( !isps )
	    synthtype_ = SynthGenParams::ZeroOffset;
	else
	    synthtype_ = SynthGenParams::PreStack;
    }
    else
    {
	BufferString typestr;
	parseEnum( par, sKeySynthType(), synthtype_ );
	if ( synthtype_ == SynthGenParams::AngleStack ||
	     synthtype_ == SynthGenParams::AVOGradient )
	{
	    par.get( sKeyInput(), inpsynthnm_ );
	    par.get( sKeyAngleRange(), anglerg_ );
	}
    }
}


void SynthGenParams::createName( BufferString& nm ) const
{
    nm = wvltnm_;
    TypeSet<float> offset; 
    raypars_.get( RayTracer1D::sKeyOffset(), offset );
    const int offsz = offset.size();
    if ( offsz )
    {
	nm += " ";
	nm += "Offset ";
	nm += ::toString( offset[0] );
	if ( offsz > 1 )
	    nm += "-"; nm += offset[offsz-1];
    }
}



void SynthDispParams::fillPar( IOPar& par ) const
{
    IOPar disppar;
    disppar.set( sKey::Range(), mapperrange_ );
    disppar.set( sKeyColTab(), coltab_ );
    par.mergeComp( disppar, sKeyDispPar() );
}


void SynthDispParams::usePar( const IOPar& par ) 
{
    PtrMan<IOPar> disppar = par.subselect( sKeyDispPar() );
    if ( !disppar ) 
	return;

    disppar->get( sKey::Range(), mapperrange_ );
    disppar->get( sKeyColTab(), coltab_ );
}



StratSynth::StratSynth( const Strat::LayerModelProvider& lmp, bool useed )
    : lmp_(lmp)
    , useed_(useed)
    , level_(0)  
    , tr_(0)
    , wvlt_(0)
    , lastsyntheticid_(0)
{
}


StratSynth::~StratSynth()
{
    deepErase( synthetics_ );
    setLevel( 0 );
}


const Strat::LayerModel& StratSynth::layMod() const
{
    return lmp_.getEdited( useed_ );
}


void StratSynth::setWavelet( const Wavelet* wvlt )
{
    if ( !wvlt ) 
	return;

    delete wvlt_; 
    wvlt_ = wvlt;
    genparams_.wvltnm_ = wvlt->name();
} 


void StratSynth::clearSynthetics()
{
    deepErase( synthetics_ );
}


#define mErrRet( msg, act )\
{\
    errmsg_ = "Can not generate synthetics ";\
    errmsg_ += synthgenpar.name_;\
    errmsg_ += " :\n";\
    errmsg_ += msg;\
    act;\
}

bool StratSynth::removeSynthetic( const char* nm )
{
    for ( int idx=0; idx<synthetics_.size(); idx++ )
    {
	if ( synthetics_[idx]->name() == nm )
	{
	    delete synthetics_.removeSingle( idx );
	    return true;
	}
    }

    return false;
}


SyntheticData* StratSynth::addSynthetic()
{
    SyntheticData* sd = generateSD();
    if ( sd )
	synthetics_ += sd;
    return sd;
}


SyntheticData* StratSynth::addSynthetic( const SynthGenParams& synthgen )
{
    SyntheticData* sd = generateSD( synthgen );
    if ( sd )
	synthetics_ += sd;
    return sd;
}



SyntheticData* StratSynth::replaceSynthetic( int id )
{
    SyntheticData* sd = getSynthetic( id );
    if ( !sd ) return 0;

    const int sdidx = synthetics_.indexOf( sd );
    sd = generateSD();
    if ( sd )
    {
	sd->setName( synthetics_[sdidx]->name() );
	delete synthetics_.replace( sdidx, sd );
    }

    return sd;
}


SyntheticData* StratSynth::addDefaultSynthetic()
{
    genparams_.synthtype_ = SynthGenParams::ZeroOffset;
    genparams_.createName( genparams_.name_ );
    SyntheticData* sd = addSynthetic();
    return sd;
}


int StratSynth::syntheticIdx( const char* nm ) const
{
    for ( int idx=0; idx<synthetics().size(); idx ++ )
    {
	if( synthetics_[idx]->name() == nm )
	    return idx;
    }
    return 0;
}


SyntheticData* StratSynth::getSynthetic( const char* nm ) 
{
    for ( int idx=0; idx<synthetics().size(); idx ++ )
    {
	if ( !strcmp( synthetics_[idx]->name(), nm ) )
	    return synthetics_[idx]; 
    }
    return 0;
}


SyntheticData* StratSynth::getSynthetic( int id ) 
{
    for ( int idx=0; idx<synthetics().size(); idx ++ )
    {
	if ( synthetics_[idx]->id_ == id )
	    return synthetics_[ idx ];
    }
    return 0;
}


SyntheticData* StratSynth::getSyntheticByIdx( int idx ) 
{
    return synthetics_.validIdx( idx ) ?  synthetics_[idx] : 0;
}


const SyntheticData* StratSynth::getSyntheticByIdx( int idx ) const
{
    return synthetics_.validIdx( idx ) ?  synthetics_[idx] : 0;
}


int StratSynth::syntheticIdx( const PropertyRef& pr ) const
{
    for ( int idx=0; idx<synthetics_.size(); idx++ )
    {
	mDynamicCastGet(const StratPropSyntheticData*,pssd,synthetics_[idx]);
	if ( !pssd ) continue;
	if ( pr == pssd->propRef() )
	    return idx;
    }
    return 0;
}


SyntheticData* StratSynth::getSynthetic( const  PropertyRef& pr )
{
    for ( int idx=0; idx<synthetics_.size(); idx++ )
    {
	mDynamicCastGet(StratPropSyntheticData*,pssd,synthetics_[idx]);
	if ( !pssd ) continue;
	if ( pr == pssd->propRef() )
	    return pssd;
    }
    return 0;
}


int StratSynth::nrSynthetics() const 
{
    return synthetics_.size();
}


SyntheticData* StratSynth::generateSD()
{ return generateSD( genparams_ ); }

#define mSetBool( str, newval ) \
{ \
    mDynamicCastGet(Attrib::BoolParam*,param,psdesc->getValParam(str)) \
    param->setValue( newval ); \
}


#define mSetEnum( str, newval ) \
{ \
    mDynamicCastGet(Attrib::EnumParam*,param,psdesc->getValParam(str)) \
    param->setValue( newval ); \
}

#define mSetFloat( str, newval ) \
{ \
    Attrib::ValParam* param  = psdesc->getValParam( str ); \
    param->setValue( newval ); \
}


#define mSetString( str, newval ) \
{ \
    Attrib::ValParam* param = psdesc->getValParam( str ); \
    param->setValue( newval ); \
}


#define mCreateDesc() \
mDynamicCastGet(PreStackSyntheticData*,presd,sd); \
if ( !presd ) return 0; \
BufferString dpidstr( "#" ); \
SeparString fullidstr( toString(DataPackMgr::CubeID()), '.' ); \
const PreStack::GatherSetDataPack& gdp = presd->preStackPack(); \
fullidstr.add( toString(gdp.id()) ); \
dpidstr.add( fullidstr.buf() ); \
Attrib::Desc* psdesc = \
    Attrib::PF().createDescCopy(Attrib::PSAttrib::attribName()); \
mSetString(Attrib::StorageProvider::keyStr(),dpidstr.buf()); \
mSetFloat( Attrib::PSAttrib::offStartStr(), \
	   presd->offsetRange().start ); \
mSetFloat( Attrib::PSAttrib::offStopStr(), \
	   presd->offsetRange().stop ); 


#define mSetProc() \
mSetBool(Attrib::PSAttrib::useangleStr(), true ); \
mSetFloat(Attrib::PSAttrib::angleStartStr(), synthgenpar.anglerg_.start ); \
mSetFloat(Attrib::PSAttrib::angleStopStr(), synthgenpar.anglerg_.stop ); \
psdesc->setUserRef( synthgenpar.name_ ); \
psdesc->updateParams(); \
PtrMan<Attrib::DescSet> descset = new Attrib::DescSet( false ); \
if ( !descset ) return 0; \
Attrib::DescID attribid = descset->addDesc( psdesc ); \
PtrMan<Attrib::EngineMan> aem = new Attrib::EngineMan; \
TypeSet<Attrib::SelSpec> attribspecs; \
Attrib::SelSpec sp( 0, attribid ); \
sp.set( *psdesc ); \
attribspecs += sp; \
aem->setAttribSet( descset ); \
aem->setAttribSpecs( attribspecs ); \
aem->setCubeSampling( cs ); \
BinIDValueSet bidvals( 0, false ); \
const ObjectSet<PreStack::Gather>& gathers = gdp.getGathers(); \
for ( int idx=0; idx<gathers.size(); idx++ ) \
    bidvals.add( gathers[idx]->getBinID() ); \
SeisTrcBuf* dptrcbufs = new SeisTrcBuf( true ); \
Interval<float> zrg( cs.zrg ); \
PtrMan<Attrib::Processor> proc = \
    aem->createTrcSelOutput( errmsg_, bidvals, *dptrcbufs, 0, &zrg); \
if ( !proc || !proc->getProvider() ) \
    mErrRet( errmsg_, delete sd; return 0 ) ; \
proc->getProvider()->setDesiredVolume( cs ); \
proc->getProvider()->setPossibleVolume( cs );


#define mCreateSeisBuf() \
if ( !TaskRunner::execute(tr_,*proc) ) \
    mErrRet( proc->message(), delete sd; return 0 ) ; \
const int crlstep = SI().crlStep(); \
const BinID bid0( SI().inlRange(false).stop + SI().inlStep(), \
		  SI().crlRange(false).stop + crlstep ); \
for ( int trcidx=0; trcidx<dptrcbufs->size(); trcidx++ ) \
{ \
    const BinID bid = dptrcbufs->get( trcidx )->info().binid; \
    dptrcbufs->get( trcidx )->info().nr =(bid.crl-bid0.crl)/crlstep; \
} \
SeisTrcBufDataPack* angledp = \
    new SeisTrcBufDataPack( dptrcbufs, Seis::Line, \
			    SeisTrcInfo::TrcNr, synthgenpar.name_ ); \
delete sd;

SyntheticData* StratSynth::createAVOGradient( SyntheticData* sd,
					     const CubeSampling& cs,
       					     const SynthGenParams& synthgenpar,
					     const Seis::RaySynthGenerator& sg )
{
    mCreateDesc()
    mSetEnum(Attrib::PSAttrib::calctypeStr(),PreStack::PropCalc::LLSQ);
    mSetEnum(Attrib::PSAttrib::offsaxisStr(),PreStack::PropCalc::Sinsq);
    mSetEnum(Attrib::PSAttrib::lsqtypeStr(), PreStack::PropCalc::Coeff );

    mSetProc();
    mDynamicCastGet(Attrib::PSAttrib*,psattr,proc->getProvider());
    if ( !psattr )
	mErrRet( proc->message(), delete sd; return 0 ) ;
    PreStack::ModelBasedAngleComputer* anglecomp =
	new PreStack::ModelBasedAngleComputer;
    anglecomp->setFFTSmoother( 10.f, 15.f );
    const ObjectSet<RayTracer1D>& rts = sg.rayTracers();
    for ( int idx=0; idx<rts.size(); idx++ )
    {
	const PreStack::Gather* gather = gathers[idx];
	const TraceID trcid( gather->getBinID() );
	anglecomp->setRayTracer( rts[idx], trcid );
    }

    psattr->setAngleComp( anglecomp );
    
    mCreateSeisBuf();
    return new AVOGradSyntheticData( synthgenpar, *angledp );
}


SyntheticData* StratSynth::createAngleStack( SyntheticData* sd,
					     const CubeSampling& cs,
       					     const SynthGenParams& synthgenpar )
{
    mCreateDesc();
    mSetEnum(Attrib::PSAttrib::calctypeStr(),PreStack::PropCalc::Stats);
    mSetEnum(Attrib::PSAttrib::stattypeStr(), Stats::Average );

    mSetProc();
    mCreateSeisBuf();
    return new AngleStackSyntheticData( synthgenpar, *angledp );
}


class ElasticModelCreator : public ParallelTask
{
public:
ElasticModelCreator( const Strat::LayerModel& lm, TypeSet<ElasticModel>& ems )
    : ParallelTask( "Generating Elastic Models" )
    , lm_(lm)
    , aimodels_(ems)
{
    aimodels_.setSize( lm_.size(), ElasticModel() );
}


const char* message() const	{ return errmsg_.isEmpty() ? 0 : errmsg_; }
const char* nrDoneText() const	{ return "Models done"; }

protected :

bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    for ( int idm=(int) start; idm<=stop; idm++ )
    {
	addToNrDone(1);
	ElasticModel& curem = aimodels_[idm];
	const Strat::LayerSequence& seq = lm_.sequence( idm ); 
	if ( seq.isEmpty() )
	    continue;

	if ( !fillElasticModel(seq,curem) )
	    return false; 
    }

    return true;
}

bool fillElasticModel( const Strat::LayerSequence& seq, ElasticModel& aimodel )
{
    const ElasticPropSelection& eps = lm_.elasticPropSel();
    const PropertyRefSelection& props = lm_.propertyRefs();

    aimodel.erase();
    BufferString errmsg;
    if ( !eps.isValidInput(&errmsg) )
    {
	mutex_.lock();
	errmsg_ = errmsg;
	mutex_.unLock();
	return false;
    }

    ElasticPropGen elpgen( eps, props );
    const float srddepth = -1*mCast(float,SI().seismicReferenceDatum() );
    int firstidx = 0;
    if ( seq.startDepth() < srddepth )
	firstidx = seq.nearestLayerIdxAtZ( srddepth );

    for ( int idx=firstidx; idx<seq.size(); idx++ )
    {
	const Strat::Layer* lay = seq.layers()[idx];
	float thickness = lay->thickness();
	if ( idx == firstidx )
	    thickness -= srddepth - lay->zTop();
	if ( thickness < 1e-4 )
	    continue;

	float dval =mUdf(float), pval = mUdf(float), sval = mUdf(float);
	elpgen.getVals( dval, pval, sval, lay->values(), props.size() );

	// Detect water - reset Vs
	if ( pval < cMaximumVpWaterVel() )
	    sval = 0;

	aimodel += ElasticLayer( thickness, pval, sval, dval );
    }

    return true;
}

static float cMaximumVpWaterVel()
{ return 1510.f; }

od_int64 nrIterations() const
{ return lm_.size(); }

const Strat::LayerModel&	lm_;
TypeSet<ElasticModel>&		aimodels_;
Threads::Mutex			mutex_;
BufferString			errmsg_;

};


bool StratSynth::createElasticModels()
{
    clearElasticModels();
    
    if ( layMod().isEmpty() )
    	return false;
    
    ElasticModelCreator emcr( layMod(), aimodels_ );
    if ( !TaskRunner::execute(tr_,emcr) )
	return false;
    bool modelsvalid = false;
    for ( int idx=0; idx<aimodels_.size(); idx++ )
    {
	if ( !aimodels_[idx].isEmpty() )
	{
	    modelsvalid = true;
	    break;
	}
    }

    errmsg_.setEmpty();
    if ( !modelsvalid )
    	return false;
    
    return adjustElasticModel( layMod(), aimodels_ );
}


SyntheticData* StratSynth::generateSD( const SynthGenParams& synthgenpar )
{
    errmsg_.setEmpty(); 

    if ( layMod().isEmpty() ) 
    {
	errmsg_ = "Empty layer model.";
	return 0;
    }

    Seis::RaySynthGenerator synthgen( aimodels_ );
    BufferString capt( "Generating ", synthgenpar.name_ ); 
    synthgen.setName( capt.buf() );
    synthgen.setWavelet( wvlt_, OD::UsePtr );
    const IOPar& raypars = synthgenpar.raypars_;
    synthgen.usePar( raypars );

    int maxsz = 0;
    for ( int idx=0; idx<aimodels_.size(); idx++ )
	maxsz = mMAX( aimodels_[idx].size(), maxsz );

    if ( maxsz == 0 )
	return 0;

    if ( maxsz == 1 )
	mErrRet( "Model has only one layer, please add another layer.", 
		return 0; );

    if ( !TaskRunner::execute( tr_, synthgen) )
	return 0;

    const int crlstep = SI().crlStep();
    const BinID bid0( SI().inlRange(false).stop + SI().inlStep(),
	    	      SI().crlRange(false).stop + crlstep );

    ObjectSet<SeisTrcBuf> tbufs;
    CubeSampling cs( false );
    for ( int imdl=0; imdl<layMod().size(); imdl++ )
    {
	Seis::RaySynthGenerator::RayModel& rm = synthgen.result( imdl );
	ObjectSet<SeisTrc> trcs; rm.getTraces( trcs, true );
	SeisTrcBuf* tbuf = new SeisTrcBuf( true );
	for ( int idx=0; idx<trcs.size(); idx++ )
	{
	    SeisTrc* trc = trcs[idx];
	    trc->info().binid = BinID( bid0.inl, bid0.crl + imdl * crlstep );
	    trc->info().nr = imdl+1;
	    cs.hrg.include( trc->info().binid );
	    if ( !trc->isEmpty() )
	    {
		SamplingData<float> sd = trc->info().sampling;
		StepInterval<float> zrg( sd.start,
					 sd.start+(sd.step*trc->size()),
					 sd.step );
		cs.zrg.include( zrg, false );
	    }

	    trc->info().coord = SI().transform( trc->info().binid );
	    tbuf->add( trc );
	}
	tbufs += tbuf;
    }

    SyntheticData* sd = 0;
    if ( synthgenpar.synthtype_ == SynthGenParams::PreStack ||
	 synthgenpar.synthtype_ == SynthGenParams::AngleStack ||
	 synthgenpar.synthtype_ == SynthGenParams::AVOGradient )
    {
	ObjectSet<PreStack::Gather> gatherset;
	while ( tbufs.size() )
	{
	    SeisTrcBuf* tbuf = tbufs.removeSingle( 0 );
	    PreStack::Gather* gather = new PreStack::Gather();
	    if ( !gather->setFromTrcBuf( *tbuf, 0 ) )
		{ delete gather; continue; }

	    gatherset += gather;
	}

	PreStack::GatherSetDataPack* dp = 
	    new PreStack::GatherSetDataPack( synthgenpar.name_, gatherset );
	sd = new PreStackSyntheticData( synthgenpar, *dp );

	if ( synthgenpar.synthtype_ == SynthGenParams::AngleStack )
	    sd = createAngleStack( sd, cs, synthgenpar );
	else if ( synthgenpar.synthtype_ == SynthGenParams::AVOGradient )
	    sd = createAVOGradient( sd, cs, synthgenpar, synthgen );
	else
	{
	    mDynamicCastGet(PreStackSyntheticData*,presd,sd);
	    presd->createAngleData( synthgen.rayTracers(),
		    		    aimodels_ );
	}
    }
    else if ( synthgenpar.synthtype_ == SynthGenParams::ZeroOffset )
    {
	SeisTrcBuf* dptrcbuf = new SeisTrcBuf( true );
	while ( tbufs.size() )
	{
	    SeisTrcBuf* tbuf = tbufs.removeSingle( 0 );
	    SeisTrcPropChg stpc( *tbuf->get( 0 ) );
	    while ( tbuf->size() > 1 )
	    {
		SeisTrc* trc = tbuf->remove( tbuf->size()-1 );
		stpc.stack( *trc );
		delete trc;
	    }
	    dptrcbuf->add( *tbuf );
	}
	SeisTrcBufDataPack* dp = new SeisTrcBufDataPack( *dptrcbuf, Seis::Line,
				   SeisTrcInfo::TrcNr, synthgenpar.name_ );	
	sd = new PostStackSyntheticData( synthgenpar, *dp );
    }

    if ( !sd )
	return 0;

    sd->id_ = ++lastsyntheticid_;

    ObjectSet<TimeDepthModel> tmpd2ts;
    for ( int imdl=0; imdl<aimodels_.size(); imdl++ )
    {
	Seis::RaySynthGenerator::RayModel& rm = synthgen.result( imdl );
	rm.getD2T( tmpd2ts, true );
	if ( tmpd2ts.isEmpty() )
	    continue;

	while ( tmpd2ts.size() )
	    sd->d2tmodels_ += tmpd2ts.removeSingle(0);
    }

    return sd;
}


void StratSynth::generateOtherQuantities()
{
    if ( synthetics_.isEmpty() ) return;

    for ( int idx=0; idx<synthetics_.size(); idx++ )
    {
	const SyntheticData* sd = synthetics_[idx];
	mDynamicCastGet(const PostStackSyntheticData*,pssd,sd);
	mDynamicCastGet(const StratPropSyntheticData*,prsd,sd);
	if ( !pssd || prsd ) continue;
	return generateOtherQuantities( *pssd, layMod() );
    }
}


mClass(WellAttrib) StratPropSyntheticDataCreator : public ParallelTask
{

public:
StratPropSyntheticDataCreator( ObjectSet<SyntheticData>& synths,
		    const PostStackSyntheticData& sd,
		    const Strat::LayerModel& lm,
       		    int& lastsynthid )
    : ParallelTask( "Creating Synthetics for Properties" )
    , synthetics_(synths)
    , sd_(sd)
    , lm_(lm)
    , lastsyntheticid_(lastsynthid)
    , isprepared_(false)
{
}

od_int64 nrIterations() const
{ return lm_.size(); }


const char* message() const
{
    return !isprepared_ ? "Preparing Models" : "Calculating";
}

const char* nrDoneText() const
{
    return "Models done";
}


protected:

bool doPrepare( int nrthreads )
{
    const StepInterval<double>& zrg =
	sd_.postStackPack().posData().range( false );

    layermodels_.setEmpty();
    ManagedObjectSet<Strat::LayerModel> layermodels;
    const int sz = zrg.nrSteps() + 1;
    for ( int idz=0; idz<sz; idz++ )
	layermodels_ += new Strat::LayerModel();
    const PropertyRefSelection& props = lm_.propertyRefs();
    for ( int iprop=1; iprop<props.size(); iprop++ )
    {
	SeisTrcBufDataPack* seisbuf =
	    new SeisTrcBufDataPack( sd_.postStackPack() );
	SeisTrcBuf* trcbuf = new SeisTrcBuf( seisbuf->trcBuf() );
	seisbuf->setBuffer( trcbuf, Seis::Line, SeisTrcInfo::TrcNr );
	seisbufdps_ += seisbuf;
    }

    for ( int iseq=0; iseq<lm_.size(); iseq++ )
    {
	addToNrDone( 1 );
	const Strat::LayerSequence& seq = lm_.sequence( iseq ); 
	const TimeDepthModel& t2d = *sd_.d2tmodels_[iseq];
	const Interval<float> seqdepthrg = seq.zRange();
	const float seqstarttime = t2d.getTime( seqdepthrg.start );
	const float seqstoptime = t2d.getTime( seqdepthrg.stop );
	Interval<float> seqtimerg( seqstarttime, seqstoptime );
	for ( int idz=0; idz<sz; idz++ )
	{
	    Strat::LayerModel* lmsamp = layermodels_[idz];
	    if ( !lmsamp )
		continue;

	    lmsamp->addSequence();
	    Strat::LayerSequence& curseq = lmsamp->sequence( iseq );
	    const float time = mCast( float, zrg.atIndex(idz) );
	    if ( !seqtimerg.includes(time,false) )
		continue;

	    const float dptstart = t2d.getDepth( time - (float)zrg.step );
	    const float dptstop = t2d.getDepth( time + (float)zrg.step );
	    Interval<float> depthrg( dptstart, dptstop );
	    seq.getSequencePart( depthrg, true, curseq );
	}
    }

    isprepared_ = true;
    resetNrDone();
    return true;
}


bool doFinish( bool success )
{
    const PropertyRefSelection& props = lm_.propertyRefs();
    SynthGenParams sgp;
    sd_.fillGenParams( sgp );
    for ( int idx=0; idx<seisbufdps_.size(); idx++ )
    {
	SeisTrcBufDataPack* dp = seisbufdps_[idx];
	BufferString nm( "[", props[idx+1]->name(), "]" );
	dp->setName( nm );
	StratPropSyntheticData* prsd = 
	    	 new StratPropSyntheticData( sgp, *dp, *props[idx+1] );
	prsd->id_ = ++lastsyntheticid_;
	prsd->setName( nm );

	deepCopy( prsd->d2tmodels_, sd_.d2tmodels_ );
	synthetics_ += prsd;
    }

    return true;
}


bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    const StepInterval<double>& zrg =
	sd_.postStackPack().posData().range( false );
    const int sz = layermodels_.size();
    const PropertyRefSelection& props = lm_.propertyRefs();
    for ( int iseq=mCast(int,start); iseq<=mCast(int,stop); iseq++ )
    {
	addToNrDone( 1 );
	const Strat::LayerSequence& seq = lm_.sequence( iseq ); 
	const TimeDepthModel& t2d = *sd_.d2tmodels_[iseq];
	Interval<float> seqtimerg(  t2d.getTime(seq.zRange().start),
				    t2d.getTime(seq.zRange().stop) );

	for ( int iprop=1; iprop<props.size(); iprop++ )
	{
	    const bool propisvel = props[iprop]->stdType() == PropertyRef::Vel;
	    SeisTrcBufDataPack* dp = seisbufdps_[iprop-1];
	    SeisTrcBuf& trcbuf = dp->trcBuf();
	    const int bufsz = trcbuf.size();
	    SeisTrc* rawtrc = iseq < bufsz ? trcbuf.get( iseq ) : 0;
	    if ( !rawtrc )
		continue;

	    PointBasedMathFunction propvals( PointBasedMathFunction::Linear,
					     PointBasedMathFunction::EndVal );
	    for ( int idz=0; idz<sz; idz++ )
	    {
		const float time = mCast( float, zrg.atIndex(idz) );
		if ( !seqtimerg.includes(time,false) )
		    continue;

		if ( !layermodels_.validIdx(idz) )
		    continue;

		const Strat::LayerSequence& curseq =
		    layermodels_[idz]->sequence(iseq);
		if ( curseq.isEmpty() )
		    continue;

		Stats::CalcSetup laypropcalc( true );
		laypropcalc.require( Stats::Average );
		Stats::RunCalc<double> propval( laypropcalc );
		for ( int ilay=0; ilay<curseq.size(); ilay++ )
		{
		    const Strat::Layer* lay = curseq.layers()[ilay];
		    if ( !lay ) continue;
		    const float val = lay->value(iprop);
		    if ( mIsUdf(val) || ( propisvel && val < 1e-5f ) )
			continue;

		    propval.addValue( propisvel ? 1.f / val : val,
				      lay->thickness() );
		}
		const float val = mCast( float, propval.average() );
		if ( mIsUdf(val) || ( propisvel && val < 1e-5f ) )
		    continue;

		propvals.add( time, propisvel ? 1.f / val : val );
	    }

	    Array1DImpl<float> proptr( sz );
	    for ( int idz=0; idz<sz; idz++ )
	    {
		const float time = mCast( float, zrg.atIndex(idz) );
		proptr.set( idz, propvals.getValue( time ) );
	    }

	    const float step = mCast( float, zrg.step );
	    ::FFTFilter filter( sz, step );
	    const float f4 = 1.f / (2.f * step );
	    filter.setLowPass( f4 );
	    if ( !filter.apply(proptr) )
		continue;

	    for ( int idz=0; idz<sz; idz++ )
		rawtrc->set( idz, proptr.get( idz ), 0 );
	}
    }

    return true;
}


    const PostStackSyntheticData&	sd_;
    const Strat::LayerModel&		lm_;
    ManagedObjectSet<Strat::LayerModel> layermodels_;
    ObjectSet<SyntheticData>&		synthetics_;
    ObjectSet<SeisTrcBufDataPack>	seisbufdps_;
    int&				lastsyntheticid_;
    bool				isprepared_;

};


void StratSynth::generateOtherQuantities( const PostStackSyntheticData& sd, 
					  const Strat::LayerModel& lm ) 
{
    StratPropSyntheticDataCreator propcreator( synthetics_, sd, lm,
	    				       lastsyntheticid_ );
    TaskRunner::execute( tr_, propcreator );
}


const char* StratSynth::errMsg() const
{
    return errmsg_.isEmpty() ? 0 : errmsg_.buf();
}


const char* StratSynth::infoMsg() const
{
    return infomsg_.isEmpty() ? 0 : infomsg_.buf();
}


#define mValidDensityRange( val ) \
(mIsUdf(val) || (val>100 && val<10000))
#define mValidWaveRange( val ) \
(mIsUdf(val) || (val>10 && val<10000))

#define mAddValToMsg( var, isdens ) \
infomsg_ += "( sample value "; \
infomsg_ += var; \
infomsg_ += isdens ? "kg/m3" : "m/s"; \
infomsg_ += ") \n";

bool StratSynth::adjustElasticModel( const Strat::LayerModel& lm,
				     TypeSet<ElasticModel>& aimodels )
{
    infomsg_.setEmpty();
    for ( int midx=0; midx<aimodels.size(); midx++ )
    {
	const Strat::LayerSequence& seq = lm.sequence( midx );
	ElasticModel& aimodel = aimodels[midx];
	if ( aimodel.isEmpty() ) continue;

	Array1DImpl<float> densvals( aimodel.size() );
	Array1DImpl<float> velvals( aimodel.size() );
	Array1DImpl<float> svelvals( aimodel.size() );
	densvals.setAll( mUdf(float) );
	velvals.setAll( mUdf(float) );
	svelvals.setAll( mUdf(float) );
	int invaliddenscount = 0;
	int invalidvelcount = 0;
	int invalidsvelcount = 0;
	for ( int idx=0; idx<aimodel.size(); idx++ )
	{
	    const ElasticLayer& layer = aimodel[idx];
	    densvals.set( idx, layer.den_ );
	    velvals.set( idx, layer.vel_ );
	    svelvals.set( idx, layer.svel_ );
	    if ( !mValidDensityRange(layer.den_) ||
		 !mValidWaveRange(layer.vel_) ||
		 !mValidWaveRange(layer.svel_) )
	    {
		if ( !mValidDensityRange(layer.den_) )
		{
		    invaliddenscount++;
		    densvals.set( idx, mUdf(float) );
		}
		if ( !mValidWaveRange(layer.vel_) )
		{
		    invalidvelcount++;
		    velvals.set( idx, mUdf(float) );
		}
		if ( !mValidWaveRange(layer.svel_) )
		{
		    invalidsvelcount++;
		    svelvals.set( idx, mUdf(float) );
		}
		
		if ( infomsg_.isEmpty() )
		{
		    infomsg_ += "Layer model contains invalid values of "
				"following properties :\n";
		    if ( !mValidDensityRange(layer.den_) )
		    {
			infomsg_ += "'Density'";
			mAddValToMsg( layer.den_, true );
		    }
		    if ( !mValidWaveRange(layer.vel_) )
		    {
			infomsg_ += "'P-Wave'";
			mAddValToMsg( layer.vel_, false );
		    }
		    if ( !mValidWaveRange(layer.svel_) )
		    {
			infomsg_ += "'S-Wave'";
			mAddValToMsg( layer.svel_, false );
		    }

		    infomsg_ += "First occurence found in layer '";
		    infomsg_ += seq.layers()[idx]->name();
		    infomsg_ += "' of pseudo well number ";
		    infomsg_ += midx+1;
		    infomsg_ += ". Invalid values will be interpolated";
		}

	    }
	}

	if ( invalidsvelcount>=aimodel.size() ||
	     invalidvelcount>=aimodel.size() ||
	     invaliddenscount>=aimodel.size() )
	{
	    errmsg_.setEmpty();
	    errmsg_ += "Cannot generate elastic model as all the values "
		       "of the properties ";
	    const ElasticLayer& layer = aimodel[0];
	    if ( invaliddenscount>=aimodel.size() )
	    {
		errmsg_ += "'Density' ";
		mAddValToMsg( layer.den_, true );
	    }
	    if ( invalidvelcount>=aimodel.size() )
	    {
		errmsg_ += "'Pwave Velocity' ";
		mAddValToMsg( layer.vel_, false );
	    }
	    if ( invalidsvelcount>=aimodel.size() )
	    {
		errmsg_ += "'Swave Velocity' ";
		mAddValToMsg( layer.svel_, false );
	    }

	    errmsg_ += "are invalid. Probably units are not set correctly";
	    return false;
	}

	LinearArray1DInterpol interpol;
	interpol.setExtrapol( true );
	interpol.setFillWithExtremes( true );
	interpol.setArray( densvals );
	interpol.execute();
	interpol.setArray( velvals );
	interpol.execute();
	interpol.setArray( svelvals );
	interpol.execute();

	for ( int idx=0; idx<aimodel.size(); idx++ )
	{
	    ElasticLayer& layer = aimodel[idx];
	    layer.den_ = densvals.get( idx );
	    layer.vel_ = velvals.get( idx );
	    layer.svel_ = svelvals.get( idx );
	}
	aimodel.mergeSameLayers();
	aimodel.upscale( 5.0f );
    }

    return true;
}


void StratSynth::getLevelDepths( const Strat::Level& lvl,
				 TypeSet<float>& zvals ) const
{
    zvals.setEmpty();
    for ( int iseq=0; iseq<layMod().size(); iseq++ )
	zvals += layMod().sequence(iseq).depthPositionOf( lvl );
}


static void convD2T( TypeSet<float>& zvals,
		     const ObjectSet<const TimeDepthModel>& d2ts )
{
    for ( int imdl=0; imdl<zvals.size(); imdl++ )
	zvals[imdl] = d2ts.validIdx(imdl) && !mIsUdf(zvals[imdl]) ? 
	    	d2ts[imdl]->getTime( zvals[imdl] ) : mUdf(float);
}


void StratSynth::getLevelTimes( const Strat::Level& lvl,
				const ObjectSet<const TimeDepthModel>& d2ts,
				TypeSet<float>& zvals ) const
{
    getLevelDepths( lvl, zvals );
    convD2T( zvals, d2ts );
}


void StratSynth::getLevelTimes( SeisTrcBuf& trcs, 
			const ObjectSet<const TimeDepthModel>& d2ts ) const
{
    if ( !level_ ) return;

    TypeSet<float> times = level_->zvals_;
    convD2T( times, d2ts );

    for ( int idx=0; idx<trcs.size(); idx++ )
    {
	const SeisTrc& trc = *trcs.get( idx );
	SeisTrcPropCalc stp( trc );
	float z = times.validIdx( idx ) ? times[idx] : mUdf( float );
	trcs.get( idx )->info().zref = z;
	if ( !mIsUdf( z ) && level_->snapev_ != VSEvent::None )
	{
	    Interval<float> tg( z, trc.startPos() );
	    mFlValSerEv ev1 = stp.find( level_->snapev_, tg, 1 );
	    tg.start = z; tg.stop = trc.endPos();
	    mFlValSerEv ev2 = stp.find( level_->snapev_, tg, 1 );
	    float tmpz = ev2.pos;
	    const bool ev1invalid = mIsUdf(ev1.pos) || ev1.pos < 0;
	    const bool ev2invalid = mIsUdf(ev2.pos) || ev2.pos < 0;
	    if ( ev1invalid && ev2invalid )
		continue;
	    else if ( ev2invalid )
		tmpz = ev1.pos;
	    else if ( fabs(z-ev1.pos) < fabs(z-ev2.pos) )
		tmpz = ev1.pos;

	    z = tmpz;
	}
	trcs.get( idx )->info().pick = z;
    }
}


void StratSynth::setLevel( const StratSynthLevel* lvl )
{ delete level_; level_ = lvl; }



void StratSynth::trimTraces( SeisTrcBuf& tbuf, float flatshift,
			     const ObjectSet<const TimeDepthModel>& d2ts,
			     float zskip ) const
{
    if ( mIsZero(zskip,mDefEps) )
	return;

    for ( int idx=0; idx<tbuf.size(); idx++ )
    {
	SeisTrc* trc = tbuf.get( idx );
	SeisTrc* newtrc = new SeisTrc( *trc );
	newtrc->info() = trc->info();
	const TimeDepthModel& d2tmodel = *d2ts[idx];
	const int startidx =
	    trc->nearestSample( d2tmodel.getTime(zskip)-flatshift );
	newtrc->reSize( trc->size()-startidx, false );
	newtrc->setStartPos( trc->samplePos(startidx) );
	for ( int sampidx=startidx; sampidx<trc->size(); sampidx++ )
	    newtrc->set( sampidx-startidx, trc->get(sampidx,0), 0 );
	delete tbuf.replace( idx, newtrc );
    }
}


void StratSynth::flattenTraces( SeisTrcBuf& tbuf ) const
{
    if ( tbuf.isEmpty() )
	return;

    float tmax = tbuf.get(0)->info().sampling.start;
    float tmin = tbuf.get(0)->info().sampling.atIndex( tbuf.get(0)->size() );
    for ( int idx=tbuf.size()-1; idx>=1; idx-- )
    {
	if ( mIsUdf(tbuf.get(idx)->info().pick) ) continue;
	tmin = mMIN(tmin,tbuf.get(idx)->info().pick);
	tmax = mMAX(tmax,tbuf.get(idx)->info().pick);
    }

    for ( int idx=tbuf.size()-1; idx>=0; idx-- )
    {
	const SeisTrc* trc = tbuf.get( idx );
	const float start = trc->info().sampling.start - tmax;
	const float stop  = trc->info().sampling.atIndex(trc->size()-1) -tmax;
	SeisTrc* newtrc = trc->getRelTrc( ZGate(start,stop) );
	if ( !newtrc )
	{
	    newtrc = new SeisTrc( *trc );
	    newtrc->zero();
	}

	delete tbuf.replace( idx, newtrc );
    }
}	


void StratSynth::decimateTraces( SeisTrcBuf& tbuf, int fac ) const
{
    for ( int idx=tbuf.size()-1; idx>=0; idx-- )
    {
	if ( idx%fac )
	    delete tbuf.remove( idx ); 
    }
}


SyntheticData::SyntheticData( const SynthGenParams& sgp, DataPack& dp )
    : NamedObject(sgp.name_)
    , datapack_(dp)
    , id_(-1) 
{
}


SyntheticData::~SyntheticData()
{
    deepErase( d2tmodels_ );
    removePack(); 
}


void SyntheticData::setName( const char* nm )
{
    NamedObject::setName( nm );
    datapack_.setName( nm );
}


void SyntheticData::removePack()
{
    const DataPack::FullID dpid = datapackid_;
    DataPackMgr::ID packmgrid = DataPackMgr::getID( dpid );
    const DataPack* dp = DPM(packmgrid).obtain( DataPack::getID(dpid) );
    if ( dp )
	DPM(packmgrid).release( dp->id() );
}


float SyntheticData::getTime( float dpt, int seqnr ) const
{
    return d2tmodels_.validIdx( seqnr ) ? d2tmodels_[seqnr]->getTime( dpt ) 
					: mUdf( float );
}


float SyntheticData::getDepth( float time, int seqnr ) const
{
    return d2tmodels_.validIdx( seqnr ) ? d2tmodels_[seqnr]->getDepth( time ) 
					: mUdf( float );
}
 

PostStackSyntheticData::PostStackSyntheticData( const SynthGenParams& sgp,
						SeisTrcBufDataPack& dp)
    : SyntheticData(sgp,dp)
{
    useGenParams( sgp );
    DataPackMgr::ID pmid = DataPackMgr::FlatID();
    DPM( pmid ).add( &dp );
    datapackid_ = DataPack::FullID( pmid, dp.id());
}


PostStackSyntheticData::~PostStackSyntheticData()
{
}


const SeisTrc* PostStackSyntheticData::getTrace( int seqnr ) const
{ return postStackPack().trcBuf().get( seqnr ); }


SeisTrcBufDataPack& PostStackSyntheticData::postStackPack()
{
    return static_cast<SeisTrcBufDataPack&>( datapack_ );
}


const SeisTrcBufDataPack& PostStackSyntheticData::postStackPack() const
{
    return static_cast<const SeisTrcBufDataPack&>( datapack_ );
}


PreStackSyntheticData::PreStackSyntheticData( const SynthGenParams& sgp,
					     PreStack::GatherSetDataPack& dp)
    : SyntheticData(sgp,dp)
    , angledp_(0)
{
    useGenParams( sgp );
    DataPackMgr::ID pmid = DataPackMgr::CubeID();
    DPM( pmid ).add( &dp );
    datapackid_ = DataPack::FullID( pmid, dp.id());
    ObjectSet<PreStack::Gather>& gathers = dp.getGathers();
    for ( int idx=0; idx<gathers.size(); idx++ )
    {
	DPM(DataPackMgr::FlatID()).obtain( gathers[idx]->id() );
	gathers[idx]->setName( name() );
    }
}


PreStackSyntheticData::~PreStackSyntheticData()
{
    mDynamicCastGet(PreStack::GatherSetDataPack&,gsetdp,datapack_)
    ObjectSet<PreStack::Gather>& gathers = gsetdp.getGathers();
    for ( int idx=0; idx<gathers.size(); idx++ )
	DPM(DataPackMgr::FlatID()).release( gathers[idx] );
    if ( angledp_ )
    {
	ObjectSet<PreStack::Gather>& anglegathers = angledp_->getGathers();
	for ( int idx=0; idx<anglegathers.size(); idx++ )
	    DPM(DataPackMgr::FlatID()).release( anglegathers[idx] );
	DPM( DataPackMgr::CubeID() ).release( angledp_->id() );
    }
}


PreStack::GatherSetDataPack& PreStackSyntheticData::preStackPack()
{
    return static_cast<PreStack::GatherSetDataPack&>( datapack_ );
}


const PreStack::GatherSetDataPack& PreStackSyntheticData::preStackPack() const
{
    return static_cast<const PreStack::GatherSetDataPack&>( datapack_ );
}


void PreStackSyntheticData::convertAngleDataToDegrees(
					PreStack::Gather* ag ) const
{
    Array2D<float>& agdata = ag->data();
    const int dim0sz = agdata.info().getSize(0);
    const int dim1sz = agdata.info().getSize(1);
    for ( int idx=0; idx<dim0sz; idx++ )
    {
	for ( int idy=0; idy<dim1sz; idy++ )
	{
	    const float radval = agdata.get( idx, idy );
	    if ( mIsUdf(radval) ) continue;
	    const float dval =  Angle::rad2deg( radval );
	    agdata.set( idx, idy, dval );
	}
    }
}


void PreStackSyntheticData::createAngleData( const ObjectSet<RayTracer1D>& rts,
					     const TypeSet<ElasticModel>& ems ) 
{
    if ( angledp_ ) DPM( DataPackMgr::CubeID() ).release( angledp_->id() );
    ObjectSet<PreStack::Gather> anglegathers;
    const ObjectSet<PreStack::Gather>& gathers = preStackPack().getGathers();
    PreStack::ModelBasedAngleComputer anglecomp;
    anglecomp.setFFTSmoother( 10.f, 15.f );
    for ( int idx=0; idx<rts.size(); idx++ )
    {
	if ( !gathers.validIdx(idx) )
	    continue;
	const PreStack::Gather* gather = gathers[idx];
	anglecomp.setOutputSampling( gather->posData() );
	const TraceID trcid( gather->getBinID() );
	anglecomp.setRayTracer( rts[idx], trcid );
	anglecomp.setTraceID( trcid );
	PreStack::Gather* anglegather = anglecomp.computeAngles();
	convertAngleDataToDegrees( anglegather );
	TypeSet<float> azimuths;
	gather->getAzimuths( azimuths );
	anglegather->setAzimuths( azimuths );
	BufferString angledpnm( name(), "(Angle Data)" );
	anglegather->setName( angledpnm );
	anglegather->setBinID( gather->getBinID() );
	anglegathers += anglegather;
	DPM(DataPackMgr::FlatID()).addAndObtain( anglegather );
    }

    angledp_ = new PreStack::GatherSetDataPack( name(), anglegathers );
    DPM( DataPackMgr::CubeID() ).add( angledp_ );
}


float PreStackSyntheticData::offsetRangeStep() const
{
    float offsetstep = mUdf(float);
    const ObjectSet<PreStack::Gather>& gathers = preStackPack().getGathers();
    if ( !gathers.isEmpty() )
    {
	const PreStack::Gather& gather = *gathers[0];
	offsetstep = gather.getOffset(1)-gather.getOffset(0);
    }

    return offsetstep;
}


const Interval<float> PreStackSyntheticData::offsetRange() const
{
    Interval<float> offrg( 0, 0 );
    const ObjectSet<PreStack::Gather>& gathers = preStackPack().getGathers();
    if ( !gathers.isEmpty() ) 
    {
	const PreStack::Gather& gather = *gathers[0];
	offrg.set(gather.getOffset(0),gather.getOffset( gather.size(true)-1));
    }
    return offrg;
}


bool PreStackSyntheticData::hasOffset() const
{ return offsetRange().width() > 0; }


const SeisTrc* PreStackSyntheticData::getTrace( int seqnr, int* offset ) const
{ return preStackPack().getTrace( seqnr, offset ? *offset : 0 ); }


SeisTrcBuf* PreStackSyntheticData::getTrcBuf( float offset, 
					const Interval<float>* stackrg ) const
{
    SeisTrcBuf* tbuf = new SeisTrcBuf( true );
    Interval<float> offrg = stackrg ? *stackrg : Interval<float>(offset,offset);
    preStackPack().fill( *tbuf, offrg );
    return tbuf;
}


PSBasedPostStackSyntheticData::PSBasedPostStackSyntheticData(
	const SynthGenParams& sgp, SeisTrcBufDataPack& sdp )
    : PostStackSyntheticData(sgp,sdp)
{
    useGenParams( sgp );
}


PSBasedPostStackSyntheticData::~PSBasedPostStackSyntheticData()
{}


void PSBasedPostStackSyntheticData::fillGenParams( SynthGenParams& sgp ) const
{
    SyntheticData::fillGenParams( sgp );
    sgp.inpsynthnm_ = inpsynthnm_;
    sgp.anglerg_ = anglerg_;
}


void PSBasedPostStackSyntheticData::useGenParams( const SynthGenParams& sgp )
{
    SyntheticData::useGenParams( sgp );
    inpsynthnm_ = sgp.inpsynthnm_;
    anglerg_ = sgp.anglerg_;
}


StratPropSyntheticData::StratPropSyntheticData( const SynthGenParams& sgp,
						    SeisTrcBufDataPack& dp,
						    const PropertyRef& pr )
    : PostStackSyntheticData( sgp, dp ) 
    , prop_(pr)
{}


bool SyntheticData::isAngleStack() const
{
    TypeSet<float> offsets;
    raypars_.get( RayTracer1D::sKeyOffset(), offsets );
    return !isPS() && offsets.size()>1;
}


void SyntheticData::fillGenParams( SynthGenParams& sgp ) const
{
    sgp.raypars_ = raypars_;
    sgp.wvltnm_ = wvltnm_;
    sgp.name_ = name();
    sgp.synthtype_ = synthType();
}


void SyntheticData::useGenParams( const SynthGenParams& sgp )
{
    raypars_ = sgp.raypars_;
    wvltnm_ = sgp.wvltnm_;
    setName( sgp.name_ );
}


void SyntheticData::fillDispPar( IOPar& par ) const
{
    disppars_.fillPar( par );
}


void SyntheticData::useDispPar( const IOPar& par )
{
    disppars_.usePar( par );
}
