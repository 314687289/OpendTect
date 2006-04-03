/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          December 2004
 RCS:           $Id: scalingattrib.cc,v 1.17 2006-04-03 13:35:31 cvshelene Exp $
________________________________________________________________________

-*/

#include "scalingattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "runstat.h"

#define mStatsTypeRMS	0
#define mStatsTypeMean	1
#define mStatsTypeMax 	2
#define mStatsTypeUser 	3

#define mScalingTypeTPower   0
#define mScalingTypeWindow   1

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

void Scaling::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    EnumParam* scalingtype = new EnumParam( scalingTypeStr() );
    scalingtype->addEnum( scalingTypeNamesStr(mScalingTypeTPower) );
    scalingtype->addEnum( scalingTypeNamesStr(mScalingTypeWindow) );
    desc->addParam( scalingtype );

    FloatParam* powerval = new FloatParam( powervalStr() );
    powerval->setLimits( Interval<float>(0,mUdf(float)) );
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

    desc->addOutputDataType( Seis::UnknowData );
    desc->addInput( InputSpec("Input data",true) );

    PF().addDesc( desc, createInstance );
    desc->unRef();
}

    
Provider* Scaling::createInstance( Desc& desc )
{
    Scaling* res = new Scaling( desc );
    res->ref();
    if ( !res->isOK() )
    {
	res->unRef();
	return 0;
    }

    res->unRefNoDelete();
    return res;
}


void Scaling::updateDesc( Desc& desc )
{
    BufferString type = desc.getValParam( scalingTypeStr() )->getStringValue();
    const bool ispow = type == scalingTypeNamesStr(mScalingTypeTPower);
    desc.setParamEnabled( powervalStr(), ispow );
    desc.setParamEnabled( statsTypeStr(), !ispow );
    desc.setParamEnabled( gateStr(), !ispow );

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
    if ( type==mScalingTypeTPower ) return "T^n";
    return "Window";
}


Scaling::Scaling( Desc& desc_ )
    : Provider( desc_ )
    , desgate_( 0 , 0 )
{
    if ( !isOK() ) return;

    mGetEnum( scalingtype_, scalingTypeStr() );
    mGetFloat( powerval_, powervalStr() );

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
    RunningStatistics<float> stats;
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

	float val = 1;
	if ( statstype_ == mStatsTypeRMS )
	    val = stats.rms();
	else if ( statstype_ == mStatsTypeMean )
	    val = stats.mean();
	else
	    val = stats.max();

	scalefactors += !mIsZero(val,mDefEps) ? 1/val : 1;
	stats.clear();
    }
}
    

bool Scaling::computeData( const DataHolder& output, const BinID& relpos,
			   int z0, int nrsamples ) const
{
    if ( scalingtype_ == mScalingTypeTPower )
    {
	scaleTimeN( output, z0, nrsamples );
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
	const float trcval = inputdata_->series(dataidx_)->
	    				value( csamp-inputdata_->z0_ );
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

	const int outidx = z0 - output.z0_ + idx;
	output.series(0)->setValue( outidx, trcval*scalefactor );
    }

    return true;
}


void Scaling::scaleTimeN( const DataHolder& output, int z0, int nrsamples) const
{
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const int cursample = idx+z0;
	const float curt = cursample*refstep;
	const float result = pow(curt,powerval_) * 
	    inputdata_->series(dataidx_)->value( cursample-inputdata_->z0_ );
	output.series(0)->setValue( cursample-output.z0_, result );
    }
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


const Interval<float>* Scaling::desZMargin( int inp, int ) const
{
    if ( scalingtype_ == mScalingTypeTPower )
	return 0;

    const_cast<Scaling*>(this)->desgate_ =
	                Interval<float>( -(1024-1)*refstep, (1024-1)*refstep );
    return &desgate_;
}

} // namespace Attrib
