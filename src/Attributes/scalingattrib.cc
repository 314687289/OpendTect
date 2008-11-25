/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          December 2004
________________________________________________________________________

-*/
static const char* rcsID = "$Id: scalingattrib.cc,v 1.26 2008-11-25 15:35:22 cvsbert Exp $";

#include "scalingattrib.h"

#include "agc.h"
#include "arrayndimpl.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "statruncalc.h"

#define mStatsTypeRMS	0
#define mStatsTypeMean	1
#define mStatsTypeMax 	2
#define mStatsTypeUser 	3

#define mScalingTypeZPower   0
#define mScalingTypeWindow   1
#define mScalingTypeAGC	     2			

static inline float interpolator( float fact1, float fact2, 
				  float t1, float t2, float curt )
{
    if ( !mIsZero((t2-t1+2),mDefEps) )
    {
	const float increment = (fact2-fact1) / (t2-t1+2);
	return fact1 + increment * (curt-t1+1);
    }

    return fact1;
}

namespace Attrib
{

mAttrDefCreateInstance(Scaling)    
    
void Scaling::initClass()
{
    mAttrStartInitClassWithUpdate

    EnumParam* scalingtype = new EnumParam( scalingTypeStr() );
    scalingtype->addEnum( scalingTypeNamesStr(mScalingTypeZPower) );
    scalingtype->addEnum( scalingTypeNamesStr(mScalingTypeWindow) );
    scalingtype->addEnum( scalingTypeNamesStr(mScalingTypeAGC) );
    desc->addParam( scalingtype );

    FloatParam* powerval = new FloatParam( powervalStr() );
    desc->setParamEnabled( powervalStr(), false );
    desc->addParam( powerval );
    
    ZGateParam gate( gateStr() );
    gate.setLimits( Interval<float>(-mLargestZGate,mLargestZGate) );

    ParamGroup<ZGateParam>* gateset
		= new ParamGroup<ZGateParam>( 0, gateStr(), gate );
    desc->addParam( gateset );

    FloatParam factor( factorStr() );
    factor.setLimits( Interval<float>(0,mUdf(float)) );

    ParamGroup<ValParam>* factorset
		= new ParamGroup<ValParam>( 0, factorStr(), factor );
    desc->addParam( factorset );

    EnumParam* statstype = new EnumParam( statsTypeStr() );
    statstype->addEnum( statsTypeNamesStr(mStatsTypeRMS) );
    statstype->addEnum( statsTypeNamesStr(mStatsTypeMean) );
    statstype->addEnum( statsTypeNamesStr(mStatsTypeMax) );
    statstype->addEnum( statsTypeNamesStr(mStatsTypeUser) );
    statstype->setDefaultValue( mStatsTypeRMS );
    desc->addParam( statstype );

    FloatParam* widthval = new FloatParam( widthStr() );
    widthval->setLimits( Interval<float>(0,mUdf(float)) );
    desc->setParamEnabled( widthStr(), false );
    desc->addParam( widthval );
    widthval->setDefaultValue(200 );

    FloatParam* mutefractionval = new FloatParam( mutefractionStr() );
    mutefractionval->setLimits( Interval<float>(0,mUdf(float)) );
    desc->setParamEnabled( mutefractionStr(), false );
    desc->addParam( mutefractionval );
    mutefractionval->setDefaultValue( 0 );

    desc->addOutputDataType( Seis::UnknowData );
    desc->addInput( InputSpec("Input data",true) );

    mAttrEndInitClass
}

    
void Scaling::updateDesc( Desc& desc )
{
    BufferString type = desc.getValParam( scalingTypeStr() )->getStringValue();
    
    if ( type == scalingTypeNamesStr(mScalingTypeZPower) )
    {
	desc.setParamEnabled( powervalStr(), true );
	desc.setParamEnabled( statsTypeStr(), false );
	desc.setParamEnabled( gateStr(), false );
	desc.setParamEnabled( widthStr(), false );
	desc.setParamEnabled( mutefractionStr(), false );
    }
    else if ( type == scalingTypeNamesStr(mScalingTypeWindow) )
    {
	desc.setParamEnabled( powervalStr(), false );
	desc.setParamEnabled( statsTypeStr(), true );
	desc.setParamEnabled( gateStr(), true );
	desc.setParamEnabled( widthStr(), false );
	desc.setParamEnabled( mutefractionStr(), false );
    }
    else if ( type == scalingTypeNamesStr(mScalingTypeAGC) )
    {
	desc.setParamEnabled( powervalStr(), false );
	desc.setParamEnabled( statsTypeStr(), false );
	desc.setParamEnabled( gateStr(), false );
	desc.setParamEnabled( widthStr(), true );
	desc.setParamEnabled( mutefractionStr(), true );
    }

    const int statstype = desc.getValParam(statsTypeStr())->getIntValue();
    desc.setParamEnabled( factorStr(), statstype==mStatsTypeUser );
}


const char* Scaling::statsTypeNamesStr( int type )
{
    if ( type==mStatsTypeRMS ) return "RMS";
    if ( type==mStatsTypeMean ) return "Mean";
    if ( type==mStatsTypeMax ) return "Max";
    return "User-defined";
}


const char* Scaling::scalingTypeNamesStr( int type )
{
    //still T in parfile for backward compatibility
    if ( type==mScalingTypeZPower ) return "T^n";
    if ( type==mScalingTypeAGC ) return "AGC";
    return "Window";
}


Scaling::Scaling( Desc& desc_ )
    : Provider( desc_ )
{
    if ( !isOK() ) return;

    mGetEnum( scalingtype_, scalingTypeStr() );
    mGetFloat( powerval_, powervalStr() );
    mGetFloat( width_, widthStr() );
    mGetFloat( mutefraction_, mutefractionStr() );

    mDescGetParamGroup(ZGateParam,gateset,desc,gateStr())
    for ( int idx=0; idx<gateset->size(); idx++ )
    {
	const ValParam& param = (ValParam&)(*gateset)[idx];
	Interval<float> interval( param.getfValue(0), param.getfValue(1) );
	interval.sort(); interval.scale( 1./zFactor() );
	gates_ += interval;
    }
    
    mGetEnum( statstype_, statsTypeStr() );
    if ( statstype_ == mStatsTypeUser )
    {
	mDescGetParamGroup(ValParam,factorset,desc,factorStr())
	for ( int idx=0; idx<factorset->size(); idx++ )
	    factors_ += ((ValParam&)((*factorset)[idx])).getfValue( 0 );
    }
    
    desgate_ = Interval<int>( -(1024-1), 1024-1 );
    window_ = Interval<float>( -width_/2, width_/2 );
}


bool Scaling::allowParallelComputation() const
{
    return scalingtype_ != mScalingTypeAGC;
}


bool Scaling::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Scaling::getInputData( const BinID& relpos, int zintv )
{
    inputdata_ = inputs[0]->getData( relpos, zintv );
    dataidx_ = getDataIndex( 0 );
    return inputdata_;
}


void Scaling::getScaleFactorsFromStats( const TypeSet<Interval<int> >& sgates,
					TypeSet<float>& scalefactors ) const
{
    Stats::Type statstype = Stats::Max;
    if ( statstype_ == mStatsTypeRMS )
	statstype = Stats::RMS;
    else if ( statstype_ == mStatsTypeMean )
	statstype = Stats::Average;

    Stats::RunCalc<float> stats( Stats::RunCalcSetup().require(statstype) );

    for ( int sgidx=0; sgidx<gates_.size(); sgidx++ )
    {
	const Interval<int>& sg = sgates[sgidx];
	if ( !sg.start && !sg.stop )
	{
	    scalefactors += 1;
	    continue;
	}

	for ( int idx=sg.start; idx<=sg.stop; idx++ )
	{
	    const ValueSeries<float>* series = inputdata_->series(dataidx_);
	    stats += fabs( series->value(idx-inputdata_->z0_) );
	}

	float val = (float)stats.getValue( statstype );
	scalefactors += !mIsZero(val,mDefEps) ? 1/val : 1;
	stats.clear();
    }
}
    

bool Scaling::computeData( const DataHolder& output, const BinID& relpos,
			   int z0, int nrsamples, int threadid ) const
{
    if ( scalingtype_ == mScalingTypeZPower )
    {
	scaleZN( output, z0, nrsamples );
	return true;
    }

    if ( scalingtype_ == mScalingTypeAGC )
    {
	scaleAGC( output, z0, nrsamples );
	return true;
    }

    TypeSet< Interval<int> > samplegates;
    getSampleGates( gates_, samplegates, z0, nrsamples );

    TypeSet<float> scalefactors;
    if ( statstype_ != mStatsTypeUser )
	getScaleFactorsFromStats( samplegates, scalefactors );
    else
	scalefactors = factors_;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const int csamp = z0 + idx;
	const float trcval = getInputValue( *inputdata_, dataidx_, idx, z0 );
	float scalefactor = 1;
	bool found = false;
	for ( int sgidx=0; sgidx<samplegates.size(); sgidx++ )
	{
	    if ( !found && samplegates[sgidx].includes(csamp) )
	    {
		if ( sgidx+1 < samplegates.size() && 
		     samplegates[sgidx+1].includes(csamp) )
		{
		    scalefactor = interpolator( scalefactors[sgidx], 
			    			scalefactors[sgidx+1],
						samplegates[sgidx+1].start,
						samplegates[sgidx].stop,
						csamp );
		}
		else
		    scalefactor = scalefactors[sgidx];

		found = true;
	    }
	}

	setOutputValue( output, 0, idx, z0, trcval*scalefactor );
    }

    return true;
}


void Scaling::scaleZN( const DataHolder& output, int z0, int nrsamples) const
{
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const float curt = (idx+z0)*refstep;
	const float result = pow(curt,powerval_) * 
	    		     getInputValue( *inputdata_, dataidx_, idx, z0 );
       	setOutputValue( output, 0, idx, z0, result );
    }
}


void Scaling::scaleAGC( const DataHolder& output, int z0, int nrsamples ) const
{
    Interval<int> samplewindow;
    samplewindow.start = mNINT( window_.start/refstep );
    samplewindow.stop = mNINT( window_.stop/refstep );

    if ( nrsamples <= samplewindow.width() )
    {
	for ( int idx=0; idx<inputdata_->nrsamples_; idx++ )
	{
	    const float result = getInputValue( *inputdata_, dataidx_, idx, z0);
	    setOutputValue( output, 0, idx, z0, result );
	}
	return;
    }
    
    ::AGC<float> agc;
    agc.setMuteFraction( mutefraction_ );
    agc.setSampleGate( samplewindow );
    agc.setInput( *inputdata_->series(dataidx_), inputdata_->nrsamples_ );
    Array1DImpl<float> outarr( inputdata_->nrsamples_ );
    agc.setOutput( outarr );
    agc.execute( false );

    for ( int idx=0; idx<output.nrsamples_ 
	  && idx<inputdata_->nrsamples_- (inputdata_->z0_ - output.z0_); idx++ )
	setOutputValue( output, 0, idx, z0, 
			outarr.get(idx+(inputdata_->z0_ - output.z0_)) );
}


void Scaling::getSampleGates( const TypeSet< Interval<float> >& oldtgs,
			      TypeSet<Interval<int> >& newsampgates,
			      int z0, int nrsamples ) const
{
    for( int idx=0; idx<oldtgs.size(); idx++ )
    {
	Interval<int> sg( mNINT(oldtgs[idx].start/refstep),
			  mNINT(oldtgs[idx].stop/refstep) );
	if ( sg.start>nrsamples+z0 || sg.stop<z0 )
	{
	    newsampgates += Interval<int>(0,0);
	    continue;
	}

	if ( sg.start < inputdata_->z0_ ) sg.start = inputdata_->z0_;
	if ( sg.stop >= inputdata_->z0_ + inputdata_->nrsamples_ ) 
	    sg.stop = inputdata_->z0_ + inputdata_->nrsamples_ -1;
	newsampgates += sg;
    }
}


const Interval<int>* Scaling::desZSampMargin( int inp, int ) const
{
    return scalingtype_ == mScalingTypeZPower ? 0 : &desgate_;
}

} // namespace Attrib
