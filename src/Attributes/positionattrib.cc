/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          November 2002
 RCS:           $Id: positionattrib.cc,v 1.14 2006-01-12 20:37:38 cvsnanne Exp $
________________________________________________________________________

-*/


#include "positionattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsteering.h"
#include "runstat.h"
#include "valseriesinterpol.h"


#define mPositionOperMin           0
#define mPositionOperMax           1
#define mPositionOperMedian        2


namespace Attrib
{

void Position::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    BinIDParam* stepout = new BinIDParam( stepoutStr() );
    stepout->setDefaultValue( BinID(0,0) );
    desc->addParam( stepout );

    ZGateParam* gate = new ZGateParam( gateStr() );
    gate->setLimits( Interval<float>(-mLargestZGate,mLargestZGate) );
    desc->addParam( gate );

    EnumParam* oper = new EnumParam( operStr() );
    //Note: Ordering must be the same as numbering!
    oper->addEnum( operTypeStr(mPositionOperMin) );
    oper->addEnum( operTypeStr(mPositionOperMax) );
    oper->addEnum( operTypeStr(mPositionOperMedian) );
    oper->setDefaultValue( mPositionOperMin );
    desc->addParam( oper );

    BoolParam* steering = new BoolParam( steeringStr() );
    steering->setDefaultValue( false );
    desc->addParam( steering );

    desc->addInput( InputSpec("Input attribute",true) );
    desc->addInput( InputSpec("Output attribute",true) );
    desc->addOutputDataType( Seis::UnknowData );

    InputSpec steerspec( "Steering data", false );
    steerspec.issteering = true;
    desc->addInput( steerspec );

    PF().addDesc( desc, createInstance );
    desc->unRef();
}


Provider* Position::createInstance( Desc& ds )
{
    Position* res = new Position( ds );
    res->ref();

    if ( !res->isOK() )
    {
	res->unRef();
	return 0;
    }

    res->unRefNoDelete();
    return res;
}


void Position::updateDesc( Desc& desc )
{
    desc.inputSpec(2).enabled =
		desc.getValParam( steeringStr() )->getBoolValue();
}


const char* Position::operTypeStr( int type )
{
    if ( type==mPositionOperMin ) return "Min";
    if ( type==mPositionOperMax ) return "Max";
    return "Median";
}

    
Position::Position( Desc& desc_ )
    : Provider( desc_ )
{
    if ( !isOK() ) return;

    inputdata_.allowNull(true);

    mGetBinID( stepout_, stepoutStr() );
    mGetFloatInterval( gate_, gateStr() );
    gate_.scale( 1/zFactor() );

    mGetEnum( oper_, operStr() );
    mGetBool( steering_, steeringStr() );

    BinID pos;
    for ( pos.inl=-stepout_.inl; pos.inl<=stepout_.inl; pos.inl++ )
	for ( pos.crl=-stepout_.crl; pos.crl<=stepout_.crl; pos.crl++ )
	    positions_ += pos;

    outdata_ = new Array2DImpl<const DataHolder*>( stepout_.inl*2+1,
	    					  stepout_.crl*2+1 );

    const float maxso = mMAX( stepout_.inl*inldist(), stepout_.crl*crldist() );
    const float extraz = maxso * mMAXDIP;
    desgate_ = Interval<float>( gate_.start-extraz, gate_.stop+extraz );
}


Position::~Position()
{
    delete outdata_;
}


void Position::initSteering()
{
    for( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( inputs[idx] && inputs[idx]->getDesc().isSteering() )
	    inputs[idx]->initSteering(stepout_);
    }
}


bool Position::getInputOutput( int input, TypeSet<int>& res ) const
{
    if ( !steering_ || input==0 || input==1 ) 
	return Provider::getInputOutput( input, res );

    for ( int inl=-stepout_.inl; inl<=stepout_.inl; inl++ )
    {
	for ( int crl=-stepout_.crl; crl<=stepout_.crl; crl++ )
	    res += getSteeringIndex( BinID(inl,crl) );
    }

    return true;
}


bool Position::getInputData( const BinID& relpos, int zintv )
{
    const int nrpos = positions_.size();
    const int inlsz = stepout_.inl * 2 + 1;
    const int crlsz = stepout_.crl * 2 + 1;
    BinID bidstep = inputs[0]->getStepoutStep();
    //bidstep.inl = abs(bidstep.inl); bidstep.crl = abs(bidstep.crl);

    while ( inputdata_.size()<nrpos )
	inputdata_ += 0;
    
    for ( int posidx=0; posidx<nrpos; posidx++ )
    {
	BinID truepos = relpos + positions_[posidx] * bidstep;
	const DataHolder* indata = inputs[0]->getData( truepos, zintv );

	const DataHolder* odata = inputs[1]->getData( truepos, zintv );
	if ( !indata || !odata ) return false;

	inputdata_.replace( posidx, indata );
	outdata_->set( positions_[posidx].inl + stepout_.inl,
		       positions_[posidx].crl + stepout_.crl, odata );
    }
    
    inidx_ = getDataIndex( 0 );
    outidx_ = getDataIndex( 1 );
    steerdata_ = steering_ ? inputs[2]->getData( relpos, zintv ) : 0;

    return true;
}


bool Position::computeData( const DataHolder& output, const BinID& relpos,
			    int z0, int nrsamples ) const
{
    if ( !inputdata_.size() || !outdata_ ) return false;
    
    const int nrpos = positions_.size();
    const int cposnr = (int)(nrpos/2);

    const Interval<int> samplegate( mNINT(gate_.start/refstep),
				    mNINT(gate_.stop/refstep) );
    
    RunningStatistics<float> stats;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const int cursample = z0 + idx;
	TypeSet<BinIDValue> bidv;
	stats.clear();
	for ( int idp=0; idp<nrpos; idp++ )
	{
	    const DataHolder* dh = inputdata_[idp];
	    if ( !dh || !dh->nrSeries() || !dh->series(inidx_) ) 
		continue;

	    ValueSeriesInterpolator<float> interp( dh->nrsamples_-1 );
	    int ds = samplegate.start;
	    const int steeridx = getSteeringIndex( positions_[idp] );

	    float sample = cursample;
	    if ( steering_ && steerdata_->series(steeridx) )
		sample += steerdata_->series(steeridx)->value( 
						cursample-steerdata_->z0_ );
		
	    for ( int ids=0; ids<samplegate.width()+1; ids++ )
	    {
		float place = sample + ds - dh->z0_;
		stats += interp.value( *(dh->series(inidx_)), place );
		bidv += BinIDValue( positions_[idp], sample + ds );
		ds++;
	    }
	}

	if ( !stats.size() ) return false;
	int posidx;
	float outval;
	switch ( oper_ )
	{
	    case 0: { outval = stats.min( &posidx ); } break;
	    case 1: { outval = stats.max( &posidx ); } break;
	    case 2: { outval = stats.median( &posidx ); } break;
	}

	BinID bid = bidv[posidx].binid;
	float sample = bidv[posidx].value;
	const DataHolder* odata = outdata_->get( bid.inl+stepout_.inl, 
						bid.crl+stepout_.crl );

	float val = 0;
	if ( odata && odata->nrSeries() && odata->series(outidx_) )
	{
	    ValueSeriesInterpolator<float> intp( odata->nrsamples_-1 );
	    val = intp.value( *(odata->series(outidx_)), sample-odata->z0_ );
	}

	output.series(0)->setValue( output.z0_-z0+idx, val );
    }

    return true;
}


} // namespace Attrib
