/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "velocityfunction.h"

#include "attribdataholder.h"
#include "binidvalset.h"
#include "trckeyzsampling.h"
#include "interpol1d.h"
#include "dbman.h"
#include "ioobj.h"
#include "survinfo.h"

namespace Vel
{

Function::Function( FunctionSource& vfs )
    : source_(vfs)
    , cache_(0)
    , desiredrg_(SI().zRange(true).start,SI().zRange(true).stop,
	         SI().zRange(true).step)
    , bid_(mUdf(int),mUdf(int))
{
    source_.ref();
}


Function::~Function()
{
    removeCache();
    source_.removeFunction( this );
    source_.unRef();
}


const VelocityDesc& Function::getDesc() const
{
    return source_.getDesc();
}


const StepInterval<float>& Function::getDesiredZ() const
{ return desiredrg_; }


void Function::setDesiredZRange( const StepInterval<float>& n )
{ desiredrg_ = n; }


float Function::getVelocity( float z ) const
{
    Threads::Locker cachelckr( cachelock_ );
    if ( !cache_ )
    {
	const StepInterval<float> sampling = getDesiredZ();
	cachesd_ = sampling;
	const int zstart = mNINT32(sampling.start/sampling.step);
	const int zstop = mNINT32(sampling.stop/sampling.step);
	mTryAlloc( cache_, TypeSet<float>( zstop-zstart+1, mUdf(float) ) );
	if ( !cache_ ) return mUdf(float);

	if ( !computeVelocity( (float) cachesd_.start, (float) cachesd_.step,
		    	       cache_->size(), cache_->arr() ) )
	{
	    delete cache_;
	    cache_ = 0;
	    return mUdf( float );
	}
    }
    cachelckr.unlockNow();

    const int sz = cache_->size();
    const float fsample = cachesd_.getfIndex( z );
    const int isample = (int) fsample;
    if ( sz < 1 )
	return mUdf(float);
    else if ( isample<0 )
	return (*cache_)[0];
    else if ( isample>=sz-1 )
	return (*cache_)[sz-1];

    return Interpolate::linearReg1DWithUdf( (*cache_)[isample],
					    (*cache_)[isample+1],
					    fsample-isample );
}


const BinID& Function::getBinID() const
{ return bid_; }


bool Function::moveTo( const BinID& bid )
{
    bid_ = bid;
    removeCache();

    return true;
}


void Function::removeCache()
{
    Threads::Locker lckr( cachelock_ );
    delete cache_; cache_ = 0;
}


mImplFactory1Param( FunctionSource, const DBKey&, FunctionSource::factory );


BufferString FunctionSource::userName() const
{
    PtrMan<IOObj> ioobj = DBM().get( mid_ );
    if ( ioobj )
	return ioobj->name();

    return BufferString( factoryKeyword() );
}


void FunctionSource::removeFunction( const Function* func )
{
    Threads::Locker lckr( lock_ );

    int idx = functions_.indexOf( func );

    if ( idx!=-1 )
    {
	functions_.removeSingle( idx );
    }
}


const uiString FunctionSource::errMsg() const
{ return errmsg_; }


void FunctionSource::getSurroundingPositions( const BinID& bid,
					      BinIDValueSet& bids) const
{
    BinIDValueSet mybids( 0, false );
    getAvailablePositions( mybids );
    if ( !mybids.isEmpty() )
	bids.append( mybids ); //Filter?
}


int FunctionSource::findFunction( const BinID& bid ) const
{
    for ( int idx=functions_.size()-1; idx>=0; idx-- )
    {
	if ( functions_[idx]->getBinID()==bid )
	    return idx;
    }

    return -1;
}


ConstRefMan<Function> FunctionSource::getFunction( const BinID& bid )
{
    if ( mIsUdf(bid.inl()) || mIsUdf(bid.crl()) )
	return 0;

    Threads::Locker lckr( lock_ );
    RefMan<Function> tmpfunc = 0;
    int idx = findFunction( bid );
    if ( idx==-1 )
    {
 	tmpfunc = createFunction( bid );
	if ( !tmpfunc )
	    return 0;

 	functions_ += tmpfunc.ptr();
    }
    else
	tmpfunc = functions_[idx];

    lckr.unlockNow();

    return ConstRefMan<Function>( tmpfunc.ptr() );

}

} // namespace Vel
