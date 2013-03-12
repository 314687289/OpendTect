/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2008
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "prestackprop.h"

#include "idxable.h"
#include "flatposdata.h"
#include "prestackgather.h"
#include "statruncalc.h"
#include "linear.h"
#include "math2.h"

namespace PreStack
{ 
DefineEnumNames(PropCalc,CalcType,1,"Calculation type")
{
	"Statistics",
	"Least-Square",
	0
};

DefineEnumNames(PropCalc,AxisType,2,"Axis transformation")
{
	"None",
	"Logarithmic",
	"Exponential",
	"Quadratic",
	"Square root",
	"Absolute value",
	"Sine-square",
	0
};

DefineEnumNames(PropCalc,LSQType,0,"Axis type")
{
	"Intercept",
	"Gradient",
	"Angle Intercept",
	"Angle Gradient",
	"StdDev of Intercept",
	"StdDev of Gradient",
	"Correlation coefficient",
	0
};


PropCalc::PropCalc( const Setup& s )
    : setup_(s)
    , gather_( 0 )
    , innermutes_( 0 )
    , outermutes_( 0 )
    , angledata_( 0 )
{}


PropCalc::~PropCalc()
{
    removeGather();
}


void PropCalc::removeGather()
{
    if ( gather_ )
    {
	DPM(DataPackMgr::FlatID()).release( gather_->id() );
	gather_ = 0;
    }

    if ( angledata_ )
    {
	DPM(DataPackMgr::FlatID()).release( angledata_->id() );
	angledata_ = 0;
    }

    delete [] innermutes_;
    innermutes_ = outermutes_ = 0;
}


void PropCalc::setGather( DataPack::ID id )
{
    removeGather();

    DataPack* dp = DPM(DataPackMgr::FlatID()).obtain( id );
    mDynamicCastGet( Gather*, g, dp );
    if ( g )
    {
	gather_ = g;
	const int nroffsets = gather_->size( !gather_->offsetDim() );
	mTryAlloc( innermutes_, int[nroffsets*2] );
	if ( innermutes_ )
	{
	    outermutes_ = innermutes_ + nroffsets;

	    gather_->detectOuterMutes( outermutes_, 0 );
	    gather_->detectInnerMutes( innermutes_, 0 );
	}
    }
    else if ( dp )
	DPM(DataPackMgr::FlatID()).release( id );
}


void PropCalc::setAngleData( DataPack::ID id )
{
    DataPack* dp = DPM(DataPackMgr::FlatID()).obtain( id );
    mDynamicCastGet( Gather*, angledata, dp );

    if ( angledata )
	angledata_ = angledata;
    else
	DPM(DataPackMgr::FlatID()).release( id );
}


float PropCalc::getVal( int sampnr ) const
{
    if ( !gather_  || gather_->isOffsetAngle() )
	return mUdf(float);

    const float eps = 1e-3;
    Interval<float> offsrg( setup_.offsrg_ );
    offsrg.start -= eps; offsrg.stop += eps;
    offsrg.sort();

    const int nroffsets = gather_->size( !gather_->offsetDim() );
    const int nrz = gather_->size( gather_->offsetDim() );

    TypeSet<float> axisvals, vals;
    for ( int ishft=-setup_.aperture_; ishft<=setup_.aperture_; ishft++ )
    {
	for ( int itrc=0; itrc<nroffsets; itrc++ )
	{
	    const float offset = gather_->getOffset(itrc);
	    if ( !offsrg.includes( offset, true ) )
		continue;

	    const int cursamp = sampnr + ishft;
	    if ( cursamp>=innermutes_[itrc] || cursamp<=outermutes_[itrc] )
		continue;

	    if ( cursamp<0 || cursamp>=nrz )
		continue;

	    const float val = gather_->data().get( itrc, cursamp );

	    vals += val;
	    if ( setup_.calctype_ != Stats )
	    {
		if ( (setup_.lsqtype_==AngleA0||setup_.lsqtype_==AngleCoeff) 
		      && !setup_.useazim_ && angledata_ )
		    axisvals += angledata_->data().get( itrc, (int)cursamp );
		else
		    axisvals += setup_.useazim_ ? gather_->getAzimuth(itrc)
						: offset;
	    }
	}
    }

    return getVal( setup_, vals, axisvals );
}


float PropCalc::getVal( float z ) const
{
    if ( !gather_  || gather_->isOffsetAngle() )
	return mUdf(float);

    const float eps = 1e-3;
    Interval<float> offsrg( setup_.offsrg_ );
    offsrg.start -= eps; offsrg.stop += eps;
    offsrg.sort();

    const int nroffsets = gather_->size( !gather_->offsetDim() );
    const int nrz = gather_->size( !gather_->zDim() );

    TypeSet<float> axisvals, vals;
    vals.setCapacity( nroffsets );
    if ( setup_.calctype_ != Stats )
	axisvals.setCapacity( nroffsets );

    const StepInterval<double> si = gather_->posData().range(!gather_->zDim());
    for ( int itrc=0; itrc<nroffsets; itrc++ )
    {
	const float offset = gather_->getOffset(itrc);
	if ( !offsrg.includes( offset, true ) )
	    continue;

	const float* seisdata = gather_->data().getData() +
	    gather_->data().info().getOffset(itrc,0);

	for ( int ishft=-setup_.aperture_; ishft<=setup_.aperture_; ishft++ )
	{
	    const float cursamp = ishft+si.getfIndex( z );
	    if ( cursamp>=innermutes_[itrc] || cursamp<=outermutes_[itrc] )
		continue;

	    if ( cursamp<0 || cursamp>=nrz )
		continue;

	    const float val =
		IdxAble::interpolateReg( seisdata, nrz,cursamp, false );

	    vals += val;
	    if ( setup_.calctype_ != Stats )
	    {
		if ( (setup_.lsqtype_==AngleA0||setup_.lsqtype_==AngleCoeff) 
		      && !setup_.useazim_ && angledata_ )
		    axisvals += angledata_->data().get( itrc, (int)cursamp );
		else
		    axisvals += setup_.useazim_ ? gather_->getAzimuth(itrc)
						: offset;
	    }
	}
    }

    return getVal( setup_, vals, axisvals );
}


static void transformAxis( TypeSet<float>& vals, PropCalc::AxisType at )
{
    if ( at == PropCalc::Norm ) return;

    for ( int idx=0; idx<vals.size(); idx++ )
    {
	const float val = vals[idx];
	switch ( at )
	{
	case PropCalc::Sqr:	vals[idx] = val * val;		break;
	case PropCalc::Log:	vals[idx] = Math::Log( val );	break;
	case PropCalc::Exp:	vals[idx] = Math::Exp( val );	break;
	case PropCalc::Sqrt:	vals[idx] = Math::Sqrt( val );	break;
	case PropCalc::Abs:	vals[idx] = fabs( val );	break;
	case PropCalc::Sinsq:	vals[idx] = sin( val );
				vals[idx] *= vals[idx];		break;
	default:						break;
	}
    }
}


float PropCalc::getVal( const PropCalc::Setup& su,
			      TypeSet<float>& vals, TypeSet<float>& axisvals )
{
    transformAxis( vals, su.valaxis_ );

    if ( su.calctype_ == Stats && !vals.size() )
	return 0;

    Stats::CalcSetup rcs;
    if ( su.calctype_ == Stats )
	rcs.require( su.stattype_ );
    else
	rcs.require( Stats::StdDev );

    Stats::RunCalc<float> rc( rcs );

    if ( su.calctype_ == Stats )
    {
	rc.addValues( vals.size(), vals.arr() );
	return (float) rc.getValue( su.stattype_ );
    }

    rc.addValues( axisvals.size(), axisvals.arr() );

    if ( vals.size()>0 && mIsZero( rc.getValue( Stats::StdDev ), 1e-3 ) )
    {
	Stats::CalcSetup rcsvals;
	rcsvals.require( Stats::StdDev );
	Stats::RunCalc<float> rcvals( rcsvals );
	rcvals.addValues( vals.size(), vals.arr() );
	if ( mIsZero( rcvals.getValue( Stats::StdDev ), 1e-9 ) )
	{
	    switch ( su.lsqtype_ )
	    {
		case A0:		return vals[0];
		case Coeff:		return 0;
		case AngleA0:		return vals[0];
		case AngleCoeff:	return 0;
		case StdDevA0:		return 0;
		case StdDevCoeff:	return 0;
		default:		return 1;
	    }
	}
    }

    if ( (su.lsqtype_==AngleA0||su.lsqtype_==AngleCoeff) && !su.useazim_ )
	transformAxis( axisvals, PropCalc::Sinsq );
    else
	transformAxis( axisvals, su.offsaxis_ );

    LinStats2D ls2d;
    ls2d.use( axisvals.arr(), vals.arr(), vals.size() );
    switch ( su.lsqtype_ )
    {
    case A0:		return ls2d.lp.a0;
    case Coeff:		return ls2d.lp.ax;
    case AngleA0:	return ls2d.lp.a0;
    case AngleCoeff:	return ls2d.lp.ax;
    case StdDevA0:	return ls2d.sd.a0;
    case StdDevCoeff:	return ls2d.sd.ax;
    default:		return ls2d.corrcoeff;
    }
}

}
