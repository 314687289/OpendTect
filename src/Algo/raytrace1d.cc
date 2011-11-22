/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Kris / Bruno
Date:          2011
________________________________________________________________________

-*/


static const char* rcsID = "$Id: raytrace1d.cc,v 1.35 2011-11-22 10:28:08 cvsbruno Exp $";


#include "raytrace1d.h"

#include "arrayndimpl.h"
#include "iopar.h"
#include "sorting.h"
#include "velocitycalc.h"
#include "zoeppritzcoeff.h"

mImplFactory(RayTracer1D,RayTracer1D::factory)

bool RayTracer1D::Setup::usePar( const IOPar& par )
{
    par.getYN( sKeyPWave(), pdown_, pup_);
    par.get( sKeySRDepth(), sourcedepth_, receiverdepth_);
    return true;
}


void RayTracer1D::Setup::fillPar( IOPar& par ) const 
{
    par.setYN( sKeyPWave(), pdown_, pup_ );
    par.set( sKeySRDepth(), sourcedepth_, receiverdepth_ );
}


RayTracer1D::RayTracer1D()
    : receiverlayer_( 0 )
    , sourcelayer_( 0 )
    , sini_( 0 )
    , twt_(0)
    , reflectivity_( 0 )
{}					       


RayTracer1D::~RayTracer1D()
{ delete sini_; delete twt_; delete reflectivity_; }


RayTracer1D* RayTracer1D::createInstance( const IOPar& par, BufferString& errm )
{
    BufferString type;
    par.get( sKey::Type, type );

    RayTracer1D* raytracer = factory().create( type );
    if ( !raytracer )
    {
	errm = "Raytracer not found. Perhaps all plugins are not loaded";
	return 0;
    }

    if ( !raytracer->usePar( par ) )
    {
	errm = raytracer->errMsg();
	delete raytracer;
	return 0;
    }

    return raytracer;
}


bool RayTracer1D::usePar( const IOPar& par )
{
    par.get( sKeyOffset(), offsets_ );
    return setup().usePar( par );
}


void RayTracer1D::fillPar( IOPar& par ) const 
{
    par.set( sKey::Type, factoryKeyword() );
    par.set( sKeyOffset(), offsets_ );
    setup().fillPar( par );
}


void RayTracer1D::setModel( const ElasticModel& lys )
{ 
    model_ = lys; 
    for ( int idx=model_.size()-1; idx>=0; idx-- )
    {
	ElasticLayer& lay = model_[idx];
	if ( mIsUdf( lay.vel_ ) && mIsUdf( lay.svel_ ) || mIsUdf( lay.den_ ) )
	    model_.remove( idx );
    }
}


void RayTracer1D::setOffsets( const TypeSet<float>& offsets )
{ offsets_ = offsets; }


od_int64 RayTracer1D::nrIterations() const
{ return model_.size() -1; }


#define mStdAVelReplacementFactor 0.348
#define mStdBVelReplacementFactor -0.959
#define mVelMin 100
bool RayTracer1D::doPrepare( int nrthreads )
{
    const int sz = model_.size();

    //See if we can find zero-offset
    bool found = false;
    for ( int idx=0; idx<offsets_.size(); idx++ )
    {
	if ( mIsZero(offsets_[idx],1e-3) )
	{
	    found = true;
	    break;
	}
    }
    if ( !found )
	offsets_ += 0;

    if ( sz < 1 )
    {
	errmsg_ = sz ? "Model is only one layer, please specify a valid model"
		     : "Model is empty, please specify a valid model";
	return false;
    }

    const float p2safac = mStdAVelReplacementFactor;
    const float p2sbfac = mStdBVelReplacementFactor;
    for ( int idx=0; idx<sz; idx++ )
    {
	ElasticLayer& layer = model_[idx];
	float& pvel = layer.vel_;
	float& svel = layer.svel_;

	if ( mIsUdf( pvel ) && !mIsUdf( svel ) )
	{
	    pvel = p2safac ? sqrt( (svel*svel-p2sbfac)/p2safac ) : mUdf(float);
	}
	else if ( mIsUdf( svel ) && !mIsUdf( pvel ) )
	{
	    svel = sqrt( p2safac*pvel*pvel + p2sbfac );
	}
	if ( pvel < mVelMin )
	    pvel = mVelMin;
    }
    const int layersize = nrIterations();

    for ( int idx=0; idx<layersize+1; idx++ )
	depths_ += idx ? depths_[idx-1] + model_[idx].thickness_ 
	               : setup().sourcedepth_ + model_[idx].thickness_;

    sourcelayer_ = 0;
    float targetdepth = setup().receiverdepth_; 
    for ( int idx=0; idx<model_.size(); idx++ )
    {
	receiverlayer_ = idx;
	const float depth = depths_[idx];
	if ( targetdepth <= depth  )
	  break;
    }

    if ( sourcelayer_>=layersize || receiverlayer_>=layersize )
    {
	errmsg_ = "Source or receiver is below lowest layer";
	return false;
    }

    const int offsetsz = offsets_.size();
    TypeSet<int> offsetidx( offsetsz, 0 );
    for ( int idx=0; idx<offsetsz; idx++ )
	offsetidx[idx] = idx;

    sort_coupled( offsets_.arr(), offsetidx.arr(), offsetsz );
    offsetpermutation_.erase();
    for ( int idx=0; idx<offsetsz; idx++ )
	offsetpermutation_ += offsetidx.indexOf( idx );

    firstlayer_ = mMIN( receiverlayer_, sourcelayer_ );

    if ( !sini_ )
	sini_ = new Array2DImpl<float>( layersize, offsetsz );
    else
	sini_->setSize( layersize, offsetsz );
    sini_->setAll( 0 );

    if ( !twt_ )
	twt_ = new Array2DImpl<float>( layersize, offsetsz );
    else
	twt_->setSize( layersize, offsetsz );
    twt_->setAll( mUdf(float) );

    if ( !reflectivity_ )
	reflectivity_ = new Array2DImpl<float_complex>( layersize, offsetsz );
    else
	reflectivity_->setSize( layersize, offsetsz );

    reflectivity_->setAll( mUdf( float_complex ) );

    return true;
}


bool RayTracer1D::compute( int layer, int offsetidx, float rayparam )
{
    const ElasticLayer& ellayer = model_[layer];
    const float downvel = setup().pdown_ ? ellayer.vel_ : ellayer.svel_;
    const float upvel = setup().pup_ ? ellayer.vel_ : ellayer.svel_;

    const float sini = downvel * rayparam;
    sini_->set( layer-1, offsetidx, sini );

    const float off = offsets_[offsetidx];

    float_complex reflectivity = 0;

    if ( !mIsZero(off,mDefEps) ) 
    {
	mAllocVarLenArr( ZoeppritzCoeff, coefs, layer );
	for ( int idx=firstlayer_; idx<layer; idx++ )
	    coefs[idx].setInterface( rayparam, model_[idx], model_[idx+1] );
	int lidx = sourcelayer_;
	reflectivity = coefs[lidx].getCoeff( true, 
				lidx!=layer-1 , setup().pdown_,
				lidx==layer-1 ? setup().pup_ : setup().pdown_ ); 
	lidx++;

	while ( lidx < layer )
	{
	    reflectivity *= coefs[lidx].getCoeff( true, lidx!=layer-1,
		    setup().pdown_, lidx==layer-1? setup().pup_ : setup().pdown_ );
	    lidx++;
	}

	for ( lidx=layer-1; lidx>=receiverlayer_; lidx--)
	{
	    if ( lidx>receiverlayer_  )
		reflectivity *= coefs[lidx-1].getCoeff( 
				    false,false,setup().pup_,setup().pup_);
	}
    }
    else
    {
	const ElasticLayer& ail0 = model_[ layer-1 ];
	const ElasticLayer& ail1 = model_[ layer ];
	const float ai0 = ail0.vel_ * ail0.den_;
	const float ai1 = ail1.vel_ * ail1.den_;
        reflectivity = float_complex( (ai1-ai0)/(ai1+ai0), 0 );
    }

    reflectivity_->set( layer-1, offsetidx, reflectivity );

    return true;
}







float RayTracer1D::getSinAngle( int layer, int offset ) const
{
    if ( !offsetpermutation_.validIdx( offset ) )
	return false;

    const int offsetidx = offsetpermutation_[offset];

    if ( !sini_ || layer<0 || layer>=sini_->info().getSize(0) || 
	 offsetidx<0 || offsetidx>=sini_->info().getSize(1) )
	return mUdf(float);
    
    return sini_->get( layer, offsetidx );
}



bool RayTracer1D::getReflectivity( int offset, ReflectivityModel& model ) const
{
    if ( !offsetpermutation_.validIdx( offset ) )
	return false;

    const int offsetidx = offsetpermutation_[offset];

    if ( offsetidx<0 || offsetidx>=reflectivity_->info().getSize(1) )
	return false;

    const int nrlayers = reflectivity_->info().getSize(0);

    model.erase();
    model.setCapacity( nrlayers );
    ReflectivitySpike spike;

    for ( int idx=0; idx<nrlayers; idx++ )
    {
	spike.reflectivity_ = reflectivity_->get( idx, offsetidx );
	spike.depth_ = depths_[idx]; 
	spike.time_ = twt_->get( idx, offsetidx );
	spike.correctedtime_ = twt_->get( idx, 0 );
	model += spike;
    }
    return true;
}


bool RayTracer1D::getTWT( int offset, TimeDepthModel& d2tm ) const
{
    if ( !offsetpermutation_.validIdx( offset ) )
	return false;

    const int offsetidx = offsetpermutation_[offset];

    if ( !twt_ || offsetidx<0 || offsetidx>=twt_->info().getSize(1) )
	return false;

    const int nrtimes = twt_->info().getSize(0);
    const int nrlayers = model_.size();

    TypeSet<float> times, depths;
    times += 0; depths += setup().sourcedepth_;
    for ( int idx=0; idx<nrlayers; idx++ )
    {
	depths += depths_[idx];
	times += idx < nrtimes ? twt_->get( idx, offsetidx ) : 
	    times[times.size()-1] + 2*model_[idx].thickness_/model_[idx].vel_;
    }
    sort_array( times.arr(), nrlayers+1 );
    return d2tm.setModel( depths.arr(), times.arr(), nrlayers+1 ); 
}




bool VrmsRayTracer1D::doPrepare( int nrthreads )
{
    if ( !RayTracer1D::doPrepare( nrthreads ) )
	return false;

    const int layersize = nrIterations();
    const bool iszerooff = offsets_.size() == 1 && mIsZero(offsets_[0],1e-3);

    TypeSet<float> dnmotimes, dvrmssum, unmotimes, uvrmssum; 
    velmax_ +=0;
    for ( int idx=firstlayer_; idx<layersize; idx++ )
    {
	const ElasticLayer& layer = model_[idx];
	dnmotimes += 0; dvrmssum += 0;
	unmotimes += 0; uvrmssum += 0;
	const float dvel = setup_.pdown_ ? layer.vel_ : layer.svel_;
	const float uvel = setup_.pup_ ? layer.vel_ : layer.svel_;
	const float dz = layer.thickness_;
	if ( idx >= sourcelayer_)
	{
	    dnmotimes[idx] += dz / dvel;
	    dvrmssum[idx] += dz * dvel; 
	}

	if ( idx >= receiverlayer_ )
	{
	    unmotimes[idx] += dz / uvel;
	    uvrmssum[idx] += dz * uvel; 
	}
	if ( idx ) 
	{
	    dvrmssum[idx] += dvrmssum[idx-1];
	    uvrmssum[idx] += uvrmssum[idx-1];
	    dnmotimes[idx] += dnmotimes[idx-1];
	    unmotimes[idx] += unmotimes[idx-1];
	}
	const float vrmssum = dvrmssum[idx] + uvrmssum[idx];
	const float twt = unmotimes[idx] + dnmotimes[idx];
	velmax_ += sqrt( vrmssum / twt );
	twt_->set( idx, 0, twt );
    }
    return true;
}


bool VrmsRayTracer1D::doWork( od_int64 start, od_int64 stop, int nrthreads )
{
    const int offsz = offsets_.size();

    for ( int layer=start; layer<=stop; layer++ )
    {
	const ElasticLayer& ellayer = model_[layer];
	const float depth = depths_[layer];
	const float vel = setup_.pdown_ ? ellayer.vel_ : ellayer.svel_;
	for ( int osidx=0; osidx<offsz; osidx++ )
	{
	    const float offset = offsets_[osidx];
	    const float angle = depth ? atan( offset / depth ) : 0;
	    const float rayparam = sin(angle) / vel;

	    if ( !compute( layer+1, osidx, rayparam ) )
	    { 
		BufferString msg( "Can not compute reflectivity " );
		msg += toString( layer+1 ); 
		msg += "\n most probably the velocity is negative or null";
		errmsg_ = msg; return false; 
	    }
	}
    }
    return true;
}


bool VrmsRayTracer1D::compute( int layer, int offsetidx, float rayparam )
{
    const float tnmo = twt_->get( layer-1, 0 );
    const float vrms = velmax_[layer];
    const float off = offsets_[offsetidx];
    const float twt = sqrt( off*off/( vrms*vrms ) + tnmo*tnmo );

    twt_->set( layer-1, offsetidx, twt );

    return RayTracer1D::compute( layer, offsetidx, rayparam );
}


