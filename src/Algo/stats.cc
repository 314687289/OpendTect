/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Sep 2006
-*/

static const char* rcsID mUsedVar = "$Id$";


#define _CRT_RAND_S

#include "statruncalc.h"
#include "statrand.h"
#include "errh.h"
#include "timefun.h"
#include "envvars.h"
#include "settings.h"
#include "threadlock.h"

DefineNameSpaceEnumNames(Stats,Type,3,"Statistic type")
{
	"Count",
	"Average", "Median", "RMS",
	"StdDev", "Variance", "NormVariance",
	"Min", "Max", "Extreme",
	"Sum", "SquareSum",
	"MostFrequent",
	0
};

DefineNameSpaceEnumNames(Stats,UpscaleType,0,"Upscale type")
{
	"Take Nearest Sample",
	"Use Average", "Use Median", "Use RMS", "Use Most Frequent",
	0
};


#include <math.h>
#include <stdlib.h>


Stats::RandGen::RandGen()
    :seed_(0)		{}


Stats::RandGen Stats::randGen()
{
    static Stats::RandGen* rgptr = 0;
    if ( !rgptr )
	rgptr = new Stats::RandGen();

    return *rgptr;
}


Stats::CalcSetup& Stats::CalcSetup::require( Stats::Type t )
{
    if ( t == Stats::Median )
	{ needmed_ = true; return *this; }
    else if ( t == Stats::MostFreq )
	{ needmostfreq_ = true; return *this; }
    else if ( t >= Stats::Min && t <= Stats::Extreme )
	{ needextreme_ = true; return *this; }
    else if ( t == Stats::Variance || t == Stats::StdDev
	    || t == Stats::NormVariance )
	{ needvariance_ = true; needsums_ = true; return *this; }

    needsums_ = true;
    return *this;
}


static int medianhandling = -2;
static Threads::Lock medianhandlinglock;


int Stats::CalcSetup::medianEvenHandling()
{
    Threads::Locker locker( medianhandlinglock );
    if ( medianhandling != -2 ) return medianhandling;

    if ( GetEnvVarYN("OD_EVEN_MEDIAN_AVERAGE") )
	medianhandling = 0;
    else if ( GetEnvVarYN("OD_EVEN_MEDIAN_LOWMID") )
	medianhandling = -1;
    else
    {
	bool yn = false;
	Settings::common().getYN( "dTect.Even Median.Average", yn );
	if ( yn )
	    medianhandling = 0;
	else
	{
	    Settings::common().getYN( "dTect.Even Median.LowMid", yn );
	    medianhandling = yn ? -1 : 1;
	}
    }

    return medianhandling;
}


double Stats::RandGen::get()
{
#ifdef __win__
    unsigned int rand = 0;
    rand_s( &rand );
    double ret = rand;
    ret /= UINT_MAX;
    return ret;
#else
    return drand48();
#endif
}


int Stats::RandGen::getInt()
{
#ifdef __win__
    unsigned int rand = 0;
    rand_s( &rand );
    return *((int*)(&rand));
#else
    return (int) lrand48();
#endif
}



void Stats::RandGen::init( int seed )
{
    if ( seed == 0 )
    {
	if ( seed_ != 0 )
	    return;

	seed = (int)Time::getMilliSeconds();
    }

    seed_ = seed;

#ifndef __win__
    srand48( (long)seed_ );
#endif
}


double Stats::RandGen::getNormal( double ex, double sd )
{
    static int iset = 0;
    static double gset = 0;
    double fac, r, v1, v2;
    double arg;

    double retval = gset;
    if ( !iset )
    {
        do
	{
            v1 = 2.0*get() - 1.0;
            v2 = 2.0*get() - 1.0;
            r=v1*v1+v2*v2;
        } while (r >= 1.0 || r <= 0.0);

	arg = (double)(-2.0*log(r)/r);
        fac = Math::Sqrt( arg );
        gset = v1 * fac;
        retval = v2 * fac;
    }

    iset = !iset;
    return retval*sd + ex;
}


int Stats::RandGen::getIndex( int sz )
{
    if ( sz < 2 ) return 0;

    int idx = (int)(sz * get());
    if ( idx < 0 ) idx = 0;
    if ( idx >= sz ) idx = sz-1;

    return idx;
}


int Stats::RandGen::getIndexFast( int sz, int seed )
{
    if ( sz < 2 ) return 0;

    int randidx = 1664525u * seed + 1013904223u;
    if ( randidx < 0 ) randidx = -randidx;
    return randidx % sz;
}


od_int64 Stats::RandGen::getIndex( od_int64 sz )
{
    if ( sz < 2 ) return 0;

    od_int64 idx = (od_int64)(sz * get());
    if ( idx < 0 ) idx = 0;
    if ( idx >= sz ) idx = sz-1;

    return idx;
}


od_int64 Stats::RandGen::getIndexFast( od_int64 sz, od_int64 seed )
{
    if ( sz < 2 ) return 0;

    const int randidx1 = mCast( int, 1664525u * seed + 1013904223u );
    const int randidx2 = mCast(int, 1664525u * (seed+0x12341234) +
				    1013904223u);
    od_int64 randidx = (((od_int64)randidx1)<<32)|((od_int64)randidx2);
    if ( randidx < 0 ) randidx = -randidx;
    return randidx % sz;
}



void initSeed( int seed )
{
    mDefineStaticLocalObject( int, seed_, = 0 );

    if ( seed == 0 )
    {
	if ( seed_ != 0 )
	    return;

	seed = (int)Time::getMilliSeconds();
    }

    seed_ = seed;

#ifndef __win__
    srand48( (long)seed_ );
#endif
}


void Stats::RandomGenerator::init( int seed )
{
    initSeed( seed );
}


Stats::UniformRandGen::UniformRandGen()
    : seed_(0)
{
    initSeed( 0 );
}


Stats::UniformRandGen Stats::uniformRandGen()
{
    mDefineStaticLocalObject( PtrMan<Stats::UniformRandGen>, rgptr,
			      = new Stats::UniformRandGen() );
    return *rgptr;
}


double Stats::UniformRandGen::get() const
{
#ifdef __win__
    unsigned int rand = 0;
    rand_s( &rand );
    double ret = rand;
    ret /= UINT_MAX;
    return ret;
#else
    return drand48();
#endif
}


int Stats::UniformRandGen::getInt() const
{
#ifdef __win__
    unsigned int rand = 0;
    rand_s( &rand );
    return *((int*)(&rand));
#else
    return (int) lrand48();
#endif
}


int Stats::UniformRandGen::getIndex( int sz ) const
{
    if ( sz < 2 ) return 0;

    int idx = (int)(sz * get());
    if ( idx < 0 ) idx = 0;
    if ( idx >= sz ) idx = sz-1;

    return idx;
}


int Stats::UniformRandGen::getIndexFast( int sz, int seed ) const
{
    if ( sz < 2 ) return 0;

    int randidx = 1664525u * seed + 1013904223u;
    if ( randidx < 0 ) randidx = -randidx;
    return randidx % sz;
}


od_int64 Stats::UniformRandGen::getIndex( od_int64 sz ) const
{
    if ( sz < 2 ) return 0;

    od_int64 idx = (od_int64)(sz * get());
    if ( idx < 0 ) idx = 0;
    if ( idx >= sz ) idx = sz-1;

    return idx;
}


od_int64 Stats::UniformRandGen::getIndexFast( od_int64 sz,
					      od_int64 seed ) const
{
    if ( sz < 2 ) return 0;

    const int randidx1 = mCast( int, 1664525u * seed + 1013904223u );
    const int randidx2 = mCast(int, 1664525u * (seed + 0x12341234)
				    + 1013904223u);
    od_int64 randidx = (((od_int64)randidx1)<<32)|((od_int64)randidx2);
    if ( randidx < 0 ) randidx = -randidx;
    return randidx % sz;
}



Stats::NormalRandGen::NormalRandGen()
    : useothdrawn_(false)
      , othdrawn_(0)
{
    initSeed( 0 );
}


double Stats::NormalRandGen::get() const
{
    if ( useothdrawn_ )
    {
	useothdrawn_ = false;
	return othdrawn_;
    }

    double v1, v2, r;
    do
    {
	v1 = 2.0 * uniformRandGen().get() - 1.0;
	v2 = 2.0 * uniformRandGen().get() - 1.0;
	r  = v1*v1 + v2*v2;
    } while (r < mDefEps || r > 1.0-mDefEps);

    const double arg = -2.0 * log(r) / r;
    const double fac = Math::Sqrt( arg );
    othdrawn_ = v2 * fac;
    useothdrawn_ = true;

    return v1 * fac;
}


float Stats::NormalRandGen::get( float e, float s ) const
{
    return (float)(e + get() * s);
}


double Stats::NormalRandGen::get( double e, double s ) const
{
    return e + get() * s;
}

