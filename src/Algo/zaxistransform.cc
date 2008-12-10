/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 2005
-*/

static const char* rcsID = "$Id: zaxistransform.cc,v 1.14 2008-12-10 17:43:32 cvskris Exp $";

#include "zaxistransform.h"

#include "survinfo.h"


mImplFactory( ZAxisTransform, ZATF );

ZAxisTransform::ZAxisTransform()
{}


ZAxisTransform::~ZAxisTransform()
{ }


int ZAxisTransform::addVolumeOfInterest( const CubeSampling&, bool )
{ return -1; }


void ZAxisTransform::setVolumeOfInterest( int, const CubeSampling&, bool )
{}


void ZAxisTransform::removeVolumeOfInterest( int ) {}


bool ZAxisTransform::loadDataIfMissing(int,TaskRunner*)
{ return !needsVolumeOfInterest(); }


float ZAxisTransform::transform( const Coord3& pos ) const
{ return transform( BinIDValue( SI().transform(pos), pos.z ) ); }


float ZAxisTransform::transform( const BinIDValue& pos ) const
{
    float res = mUdf(float);
    transform( pos.binid, SamplingData<float>(pos.value,1), 1, &res );
    return res;
}


float ZAxisTransform::transformBack( const Coord3& pos ) const
{ return transformBack( BinIDValue( SI().transform(pos), pos.z ) ); }


float ZAxisTransform::transformBack( const BinIDValue& pos ) const
{
    float res = mUdf(float);
    transformBack( pos.binid, SamplingData<float>(pos.value,1), 1, &res );
    return res;
}


float ZAxisTransform::getZIntervalCenter( bool from ) const
{
    const Interval<float> rg = getZInterval( from );
    if ( mIsUdf(rg.start) || mIsUdf(rg.stop) )
	return mUdf(float);

    return rg.center();
}


float ZAxisTransform::getGoodZStep() const
{
    return SI().zRange(true).step;
}


const char* ZAxisTransform::getZDomainString() const
{ return SI().zDomainString(); }


ZAxisTransformSampler::ZAxisTransformSampler( const ZAxisTransform& trans,
					      bool b, const BinID& nbid,
					      const SamplingData<double>& nsd )
    : transform_( trans )
    , back_( b )
    , bid_( nbid )
    , sd_( nsd )
{ transform_.ref(); }


ZAxisTransformSampler::~ZAxisTransformSampler()
{ transform_.unRef(); }


float ZAxisTransformSampler::operator[](int idx) const
{
    const int cachesz = cache_.size();
    if ( cachesz )
    {
	const int cacheidx = idx-firstcachesample_;
	if ( cacheidx>=0 && cacheidx<cachesz )
	    return cache_[cacheidx];
    }

    const BinIDValue bidval( BinIDValue(bid_,sd_.atIndex(idx)) );
    return back_ ? transform_.transformBack( bidval )
	         : transform_.transform( bidval );
}


void ZAxisTransformSampler::computeCache( const Interval<int>& range )
{
    const int sz = range.width()+1;
    cache_.setSize( sz );
    const SamplingData<float> cachesd( sd_.atIndex(range.start), sd_.step );
    if ( back_ ) transform_.transformBack( bid_, cachesd, sz, cache_.arr() );
    else transform_.transform( bid_, cachesd, sz, cache_.arr() );
    firstcachesample_ = range.start;
}
