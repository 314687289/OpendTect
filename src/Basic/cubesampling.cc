/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : somewhere around 1999
-*/
 
static const char* rcsID = "$Id: cubesampling.cc,v 1.28 2007-03-20 14:57:48 cvskris Exp $";

#include "cubesampling.h"
#include "survinfo.h"
#include "binidselimpl.h"
#include "keystrs.h"
#include "iopar.h"
#include <math.h>


void HorSampling::init( bool tosi )
{
    if ( tosi )
    {
	start = SI().sampling(false).hrg.start;
	stop = SI().sampling(false).hrg.stop;
	step = SI().sampling(false).hrg.step;
    }
    else
    {
	start.inl = start.crl = stop.inl = stop.crl = mUdf(int);
	step.inl = step.crl = 1;
    }
}


void HorSampling::set2DDef()
{
    start.inl = start.crl = 0;
    stop.inl = stop.crl = mUdf(int);
    step.inl = step.crl = 1;
}


void CubeSampling::set2DDef()
{
    hrg.set2DDef();
    zrg = SI().zRange(false);
}


void CubeSampling::init( bool tosi )
{
    hrg.init( tosi );
    if ( tosi )
	zrg = SI().zRange(false);
    else
	{ zrg.start = zrg.stop = 0; zrg.step = 1; }
}


HorSampling& HorSampling::set( const Interval<int>& inlrg,
				const Interval<int>& crlrg )
{
    start.inl = inlrg.start; stop.inl = inlrg.stop;
    mDynamicCastGet(const StepInterval<int>*,inlsrg,&inlrg)
    if ( inlsrg )
	step.inl = inlsrg->step;
    start.crl = crlrg.start; stop.crl = crlrg.stop;
    mDynamicCastGet(const StepInterval<int>*,crlsrg,&crlrg)
    if ( crlsrg )
	step.crl = crlsrg->step;
    return *this;
}


StepInterval<int> HorSampling::inlRange() const
{ return StepInterval<int>( start.inl, stop.inl, step.inl ); }


StepInterval<int> HorSampling::crlRange() const
{ return StepInterval<int>( start.crl, stop.crl, step.crl ); }


void HorSampling::includeInl( int inl )
{
    if ( mIsUdf(start.inl) || mIsUdf(stop.inl) || nrInl()<1 )
	start.inl = stop.inl = inl;
    else
    {
	start.inl = mMIN( start.inl, inl );
	stop.inl = mMAX( stop.inl, inl );
    }
}


void HorSampling::includeCrl( int crl )
{
    if ( mIsUdf(start.crl) || mIsUdf(stop.crl) || nrCrl()<1 )
	start.crl = stop.crl = crl;
    else
    {
	start.crl = mMIN( start.crl, crl );
	stop.crl = mMAX( stop.crl, crl );
    }
}


void HorSampling::get( Interval<int>& inlrg, Interval<int>& crlrg ) const
{
    inlrg.start = start.inl; inlrg.stop = stop.inl;
    mDynamicCastGet(StepInterval<int>*,inlsrg,&inlrg)
    if ( inlsrg )
	inlsrg->step = step.inl;
    crlrg.start = start.crl; crlrg.stop = stop.crl;
    mDynamicCastGet(StepInterval<int>*,crlsrg,&crlrg)
    if ( crlsrg )
	crlsrg->step = step.crl;
}


bool HorSampling::isDefined() const
{
    return !mIsUdf(start.inl) && !mIsUdf(start.crl) &&
	   !mIsUdf(stop.inl) && !mIsUdf(stop.crl) &&
	   !mIsUdf(step.inl) && !mIsUdf(step.crl);
}


void HorSampling::limitTo( const HorSampling& h )
{
    HorSampling hs( h ); hs.normalise();
    normalise();

    if ( hs.start.inl > stop.inl || hs.stop.inl < start.inl ||
	 hs.start.crl > stop.crl || hs.stop.crl < start.crl )
    {
	init( false );
	return;
    }
    
    if ( hs.start.inl > start.inl ) start.inl = hs.start.inl;
    if ( hs.start.crl > start.crl ) start.crl = hs.start.crl;
    if ( hs.stop.crl < stop.crl ) stop.crl = hs.stop.crl;
    if ( hs.stop.inl < stop.inl ) stop.inl = hs.stop.inl;
    if ( hs.step.inl > step.inl ) step.inl = hs.step.inl;
    if ( hs.step.crl > step.crl ) step.crl = hs.step.crl;
}


bool HorSampling::usePar( const IOPar& pars )
{
    bool ret = pars.get( sKey::FirstInl, start.inl );
    ret = pars.get( sKey::FirstCrl, start.crl ) || ret;
    ret = pars.get( sKey::LastInl, stop.inl ) || ret;
    ret = pars.get( sKey::LastCrl, stop.crl ) || ret;
    pars.get( sKey::StepInl, step.inl );
    pars.get( sKey::StepCrl, step.crl );
    return ret;
}


void HorSampling::fillPar( IOPar& pars ) const
{
    pars.set( sKey::FirstInl, start.inl );
    pars.set( sKey::FirstCrl, start.crl );
    pars.set( sKey::LastInl, stop.inl );
    pars.set( sKey::LastCrl, stop.crl );
    pars.set( sKey::StepInl, step.inl );
    pars.set( sKey::StepCrl, step.crl );
}


void HorSampling::removeInfo( IOPar& par )
{
    par.removeWithKey( sKey::FirstInl );
    par.removeWithKey( sKey::FirstCrl );
    par.removeWithKey( sKey::LastInl );
    par.removeWithKey( sKey::LastCrl );
    par.removeWithKey( sKey::StepInl );
    par.removeWithKey( sKey::StepCrl );
}


int HorSampling::nrInl() const
{
    if ( !step.inl || (mIsUdf(start.inl) && mIsUdf(stop.inl)) )
	return 0;

    int ret = inlIdx( stop.inl );
    return ret < 0 ? 1 - ret : ret + 1;
}


int HorSampling::nrCrl() const
{
    if ( !step.crl || (mIsUdf(start.crl) && mIsUdf(stop.crl)) )
	return 0;

    int ret = crlIdx( stop.crl );
    return ret < 0 ? 1 - ret : ret + 1;
}


static bool intersect(	int start1, int stop1, int step1,
			int start2, int stop2, int step2,
			int& outstart, int& outstop, int& outstep )
{
    if ( stop1 < start2 || start1 > stop2 )
	return false;

    // Determine step. Only accept reasonable step differences
    outstep = step2 > step1 ? step2 : step1;
    int lostep = step2 > step1 ? step1 : step2;
    if ( !lostep || outstep%lostep ) return false;

    // Snap start
    outstart = start1 < start2 ? start2 : start1;
    while ( (outstart-start1) % step1 )
	outstart += lostep;
    while ( (outstart-start2) % step2 )
	outstart += lostep;

    // Snap stop
    outstop = stop1 > stop2 ? stop2 : stop1;
    int nrsteps = (outstop - outstart) / outstep;
    outstop = outstart + nrsteps * outstep;
    return outstop >= outstart;
}

#define Eps 2e-5

inline bool IsZero( float f )
{
    return f > -Eps && f < Eps;
}

static inline bool inSeries( float v, float start, float step )
{
    float fdiff = (start - v) / step;
    int idiff = mNINT( fdiff );
    fdiff -= idiff;
    return IsZero( fdiff );
}


static bool intersectF(	float start1, float stop1, float step1,
			float start2, float stop2, float step2,
			float& outstart, float& outstop, float& outstep )
{
    if ( stop1-start2 < mDefEps || start1-stop2 > mDefEps )
	return false;

    outstep = step2 > step1 ? step2 : step1;
    float lostep = step2 > step1 ? step1 : step2;
    if ( IsZero(lostep) ) return false;

    // See if starts are compatible
    if ( !inSeries(start1,start2,lostep) )
	return false;

    // Only accept reasonable step differences
    int ifac = 1;
    for ( ; ifac<2001; ifac++ )
    {
	float stp = ifac * lostep;
	if ( IsZero(stp-outstep) ) break;
	else if ( ifac == 2000 ) return false;
    }

    outstart = start1 < start2 ? start2 : start1;
    while ( !inSeries(outstart,start1,step1)
	 || !inSeries(outstart,start2,step2) )
	outstart += lostep;

    // Snap stop
    outstop = stop1 > stop2 ? stop2 : stop1;
    int nrsteps = (int)( (outstop - outstart + Eps) / outstep );
    outstop = outstart + nrsteps * outstep;
    return (outstop-outstart) > Eps;
}


bool HorSampling::getInterSection( const HorSampling& hs,
				   HorSampling& out ) const
{
    HorSampling hs1( hs );	hs1.normalise();
    HorSampling hs2( *this );	hs2.normalise();

    return intersect( hs1.start.inl, hs1.stop.inl, hs1.step.inl,
		      hs2.start.inl, hs2.stop.inl, hs2.step.inl,
		      out.start.inl, out.stop.inl, out.step.inl )
	&& intersect( hs1.start.crl, hs1.stop.crl, hs1.step.crl,
		      hs2.start.crl, hs2.stop.crl, hs2.step.crl,
		      out.start.crl, out.stop.crl, out.step.crl );
}


void HorSampling::snapToSurvey()
{
    SI().snap( start, BinID(-1,-1) );
    SI().snap( stop, BinID(1,1) );
}


static void normaliseZ( StepInterval<float>& zrg )
{
    if ( zrg.start > zrg.stop )	Swap(zrg.start,zrg.stop);
    if ( zrg.step < 0 )		zrg.step = -zrg.step;
    else if ( !zrg.step )	zrg.step = SI().zStep();
}


bool CubeSampling::getIntersection( const CubeSampling& cs,
				    CubeSampling& out ) const
{
    if ( !hrg.getInterSection(cs.hrg,out.hrg) )
	return false;

    StepInterval<float> zrg1( cs.zrg );	normaliseZ( zrg1 );
    StepInterval<float> zrg2( zrg );	normaliseZ( zrg2 );
    return intersectF( zrg1.start, zrg1.stop, zrg1.step,
		       zrg2.start, zrg2.stop, zrg2.step,
		       out.zrg.start, out.zrg.stop, out.zrg.step );
}


bool CubeSampling::isFlat() const
{
    if ( hrg.start.inl==hrg.stop.inl || hrg.start.crl==hrg.stop.crl )
	return true;

    return fabs( zrg.stop-zrg.start ) < fabs( zrg.step * 0.5 );
}


CubeSampling::Dir CubeSampling::defaultDir() const
{
    const int nrinl = nrInl();
    const int nrcrl = nrCrl();
    const int nrz = nrZ();
    if ( nrz < nrinl && nrz < nrcrl )
	return Z;

    return nrinl < nrcrl ? Inl : Crl;
}


int64 CubeSampling::totalNr() const
{ return ((int64) nrZ()) * ((int64) hrg.totalNr()); }


bool CubeSampling::includes( const CubeSampling& c ) const
{
    return
	hrg.includes( c.hrg.start ) &&
	hrg.includes( c.hrg.stop ) &&
	zrg.includes( c.zrg.start ) &&
	zrg.includes( c.zrg.stop );
}


void CubeSampling::include( const CubeSampling& c )
{
    CubeSampling cs( c ); cs.normalise();
    normalise();

    hrg.include( cs.hrg.start ); hrg.include( cs.hrg.stop );
    if ( cs.zrg.start < zrg.start ) zrg.start = cs.zrg.start;
    if ( cs.zrg.stop > zrg.stop ) zrg.stop = cs.zrg.stop;
    if ( cs.zrg.step < zrg.step ) zrg.step = cs.zrg.step;
}


bool CubeSampling::isDefined() const
{
    return hrg.isDefined() &&
	!mIsUdf(zrg.start) && !mIsUdf(zrg.stop) && !mIsUdf(zrg.step);
}



void CubeSampling::limitTo( const CubeSampling& c )
{
    CubeSampling cs( c ); cs.normalise();
    normalise();
    hrg.limitTo( cs.hrg );
    if ( hrg.isEmpty() || cs.zrg.start > zrg.stop || cs.zrg.stop < zrg.start )
    {
	init( false );
	return;
    }

    if ( zrg.start < cs.zrg.start ) zrg.start = cs.zrg.start;
    if ( zrg.stop > cs.zrg.stop ) zrg.stop = cs.zrg.stop;
    if ( zrg.step < cs.zrg.step ) zrg.step = cs.zrg.step;
}


void CubeSampling::snapToSurvey()
{
    hrg.snapToSurvey();
    SI().snapZ( zrg.start, -1 );
    SI().snapZ( zrg.stop, 1 );
}


bool CubeSampling::operator==( const CubeSampling& cs ) const
{
   if ( cs.hrg==this->hrg )
   {
       float diff = fabs(cs.zrg.start-this->zrg.start);
       const float eps = SI().zIsTime() ? 1e-6 : 1e-3;
       if ( diff>eps ) return false;

       diff = fabs(cs.zrg.stop-this->zrg.stop);
       if ( diff>eps ) return false;

       diff = fabs(cs.zrg.step-this->zrg.step);
       if ( diff>eps ) return false;

       return true;
   }
   
   return false;
}


bool CubeSampling::usePar( const IOPar& par )
{
    return hrg.usePar( par ) &&
	   par.get( sKey::ZRange, zrg.start, zrg.stop, zrg.step );
}


void CubeSampling::fillPar( IOPar& par ) const
{
    hrg.fillPar( par );
    par.set( sKey::ZRange, zrg.start, zrg.stop, zrg.step );
}


void CubeSampling::removeInfo( IOPar& par )
{
    HorSampling::removeInfo( par );
    par.removeWithKey( sKey::ZRange );
}


void HorSampling::normalise()
{
    if ( start.inl > stop.inl )	Swap(start.inl,stop.inl);
    if ( start.crl > stop.crl )	Swap(start.crl,stop.crl);
    if ( step.inl < 0 )		step.inl = -step.inl;
    else if ( !step.inl )	step.inl = SI().inlStep();
    if ( step.crl < 0 )		step.crl = -step.crl;
    else if ( !step.crl )	step.crl = SI().crlStep();
}


void CubeSampling::normalise()
{
    hrg.normalise();
    normaliseZ( zrg );
}



HorSamplingIterator::HorSamplingIterator( const HorSampling& hrg )
    : hrg_(hrg)
    , firstpos_(true)
{
}


bool HorSamplingIterator::next( BinID& bid )
{
    if ( firstpos_ )
    {
	bid = hrg_.start;
	firstpos_ = false;
	return true;
    }

    bid.crl += hrg_.step.crl;
    if ( bid.crl > hrg_.stop.crl )
    {
	bid.inl += hrg_.step.inl;
	bid.crl = hrg_.start.crl;
    }

    return bid.inl <= hrg_.stop.inl;
}
