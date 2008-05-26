/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: well.cc,v 1.45 2008-05-26 12:05:55 cvsbert Exp $";

#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "idxable.h"
#include "iopar.h"

const char* Well::Info::sKeyuwid	= "Unique Well ID";
const char* Well::Info::sKeyoper	= "Operator";
const char* Well::Info::sKeystate	= "State";
const char* Well::Info::sKeycounty	= "County";
const char* Well::Info::sKeycoord	= "Surface coordinate";
const char* Well::Info::sKeyelev	= "Surface elevation";
const char* Well::D2TModel::sKeyTimeWell = "=Time";
const char* Well::D2TModel::sKeyDataSrc	= "Data source";
const char* Well::Marker::sKeyDah	= "Depth along hole";
const char* Well::Log::sKeyUnitLbl	= "Unit of Measure";


float Well::DahObj::dahStep( bool ismin ) const
{
    const int sz = dah_.size();
    if ( sz < 2 ) return mUdf(float);

    float res = dah_[1] - dah_[0];
    int nrvals = 1;
    for ( int idx=2; idx<sz; idx++ )
    {
	float val = dah_[idx] - dah_[idx-1];
	if ( mIsZero(val,mDefEps) )
	    continue;

	if ( !ismin )
	    res += val;
	else
	{
	    if ( val < res )
		res = val;
	}
	nrvals++;
    }

    if ( !ismin ) res /= nrvals; // average
    return mIsZero(res,mDefEps) ? mUdf(float) : res;
}


void Well::DahObj::deInterpolate()
{
    TypeSet<int> bpidxs;
    TypeSet<float> dahsc( dah_ );
    for ( int idx=0; idx<dahsc.size(); idx++ )
	dahsc[idx] *= 0.001; // gives them about same dimension

    IdxAble::getBendPoints( dahsc, *this, dahsc.size(), 1e-5, bpidxs );
    if ( bpidxs.size() < 1 ) return;

    int bpidx = 0;
    TypeSet<int> torem;
    for ( int idx=0; idx<dahsc.size(); idx++ )
    {
	if ( idx != bpidxs[bpidx] )
	    torem += idx;
	else
	    bpidx++;
    }

    for ( int idx=torem.size()-1; idx>-1; idx-- )
	remove( torem[idx] );
}


void Well::DahObj::addToDahFrom( int fromidx, float extradah )
{
    for ( int idx=fromidx; idx<dah_.size(); idx++ )
	dah_[idx] += extradah;
}


void Well::DahObj::removeFromDahFrom( int fromidx, float extradah )
{
    for ( int idx=fromidx; idx<dah_.size(); idx++ )
	dah_[idx] -= extradah;
}


Well::Data::Data( const char* nm )
    : info_(nm)
    , track_(*new Well::Track)
    , logs_(*new Well::LogSet)
    , d2tmodel_(0)
    , markerschanged(this)
    , d2tchanged(this)
{
}


Well::Data::~Data()
{
    delete &track_;
    delete &logs_;
    delete d2tmodel_;
}


void Well::Data::setD2TModel( D2TModel* d )
{
    delete d2tmodel_;
    d2tmodel_ = d;
}


Well::LogSet::~LogSet()
{
    deepErase( logs );
}


void Well::LogSet::add( Well::Log* l )
{
    if ( !l ) return;

    logs += l;
    updateDahIntv( *l );;
}


void Well::LogSet::updateDahIntv( const Well::Log& wl )
{
    if ( wl.isEmpty() ) return;

    if ( mIsUdf(dahintv.start) )
	{ dahintv.start = wl.dah(0); dahintv.stop = wl.dah(wl.size()-1); }
    else
    {
	if ( dahintv.start > wl.dah(0) )
	    dahintv.start = wl.dah(0);
	if ( dahintv.stop < wl.dah(wl.size()-1) )
	    dahintv.stop = wl.dah(wl.size()-1);
    }
}


void Well::LogSet::updateDahIntvs()
{
    for ( int idx=0; idx<logs.size(); idx++ )
    {
	logs[idx]->ensureAscZ();
	updateDahIntv( *logs[idx] );
    }
}


int Well::LogSet::indexOf( const char* nm ) const
{
    for ( int idx=0; idx<logs.size(); idx++ )
    {
	const Log& l = *logs[idx];
	if ( l.name() == nm )
	    return idx;
    }
    return -1;
}


Well::Log* Well::LogSet::remove( int logidx )
{
    Log* log = logs[logidx]; logs -= log;
    ObjectSet<Well::Log> tmp( logs );
    logs.erase(); init();
    for ( int idx=0; idx<tmp.size(); idx++ )
	add( tmp[idx] );
    return log;
}


Well::Log& Well::Log::operator =( const Well::Log& l )
{
    if ( &l != this )
    {
	setName( l.name() );
	dah_ = l.dah_; val_ = l.val_;
	range_ = l.range_; selrange_ = l.selrange_;
	displogrthm_ = l.displogrthm_;
    }
    return *this;
}


float Well::Log::getValue( float dh ) const
{
    int idx1;
    if ( IdxAble::findFPPos(dah_,dah_.size(),dh,-1,idx1) )
	return val_[idx1];
    else if ( idx1 < 0 || idx1 == dah_.size()-1 )
	return mUdf(float);

    const int idx2 = idx1 + 1;
    const float v1 = val_[idx1];
    const float v2 = val_[idx2];
    const float d1 = dh - dah_[idx1];
    const float d2 = dah_[idx2] - dh;
    if ( mIsUdf(v1) )
	return d1 > d2 ? v2 : mUdf(float);
    if ( mIsUdf(v2) )
	return d2 > d1 ? v1 : mUdf(float);

    return ( d1*val_[idx2] + d2*val_[idx1] ) / (d1 + d2);
}


void Well::Log::addValue( float z, float val )
{
    if ( !mIsUdf(val) ) 
    {
	if ( val < range_.start ) range_.start = val;
	if ( val > range_.stop ) range_.stop = val;
	selrange_.setFrom( range_ );
    }

    dah_ += z; 
    val_ += val;
}


void Well::Log::ensureAscZ()
{
    if ( dah_.size() < 2 ) return;
    const int sz = dah_.size();
    if ( dah_[0] < dah_[sz-1] ) return;
    const int hsz = sz / 2;
    for ( int idx=0; idx<hsz; idx++ )
    {
	Swap( dah_[idx], dah_[sz-idx-1] );
	Swap( val_[idx], val_[sz-idx-1] );
    }
}


void Well::Log::setSelValueRange( const Interval<float>& newrg )
{
    selrange_.setFrom( newrg );
}


Well::Track& Well::Track::operator =( const Track& t )
{
    if ( &t != this )
    {
	setName( t.name() );
	dah_ = t.dah_;
	pos_ = t.pos_;
	zistime_ = t.zistime_;
    }
    return *this;
}


void Well::Track::addPoint( const Coord& c, float z, float dahval )
{
    pos_ += Coord3(c,z);
    if ( mIsUdf(dahval) )
    {
	const int previdx = dah_.size() - 1;
	dahval = previdx < 0 ? 0
	    : dah_[previdx] + pos_[previdx].distTo( pos_[previdx+1] );
    }
    dah_ += dahval;
}


void Well::Track::insertAfterIdx( int aftidx, const Coord3& c )
{
    const int oldsz = pos_.size();
    if ( aftidx > oldsz-2 )
	{ addPoint( c, c.z ); return; }

    float extradah, owndah = 0;
    if ( aftidx == -1 )
	extradah = c.distTo( pos_[0] );
    else
    {
	float dist0 = c.distTo( pos_[aftidx] );
	float dist1 = c.distTo( pos_[aftidx+1] );
	owndah = dah_[aftidx] + dist0;
	extradah = dist0 + dist1 - pos_[aftidx].distTo( pos_[aftidx+1] );
    }

    pos_.insert( aftidx+1, c );
    dah_.insert( aftidx+1, owndah );
    addToDahFrom( aftidx+2, extradah );
}


int Well::Track::insertPoint( const Coord& c, float z )
{
    const int oldsz = pos_.size();
    if ( oldsz < 1 )
	{ addPoint( c, z ); return oldsz; }

    Coord3 cnew( c.x, c.y, z );
    if ( oldsz < 2 )
    {
	Coord3 oth( pos_[0] );
	if ( oth.z < cnew.z )
	{
	    addPoint( c, z );
	    return oldsz;
	}
	else
	{
	    pos_.erase(); dah_.erase();
	    pos_ += cnew; pos_ += oth;
	    dah_ += 0;
	    dah_ += oth.distTo( cnew );
	    return 0;
	}
    }

    // Need to find 'best' position. This is when the angle of the triangle
    // at the new point is maximal
    // This boils down to min(sum of sq distances / product of distances)

    float minval = 1e30; int minidx = -1;
    float mindist = pos_[0].distTo(cnew); int mindistidx = 0;
    for ( int idx=1; idx<oldsz; idx++ )
    {
	const Coord3& c0 = pos_[idx-1];
	const Coord3& c1 = pos_[idx];
	const float d = c0.distTo( c1 );
	const float d0 = c0.distTo( cnew );
	const float d1 = c1.distTo( cnew );
	if ( mIsZero(d0,1e-4) || mIsZero(d1,1e-4) )
	    return -1; // point already present
	float val = ( d0 * d0 + d1 * d1 - ( d * d ) ) / (2 * d0 * d1);
	if ( val < minval )
	    { minidx = idx-1; minval = val; }
	if ( d1 < mindist )
	    { mindist = d1; mindistidx = idx; }
	if ( idx == oldsz-1 && minval > 0.85 )
	{
	    if ( mindistidx == oldsz-1)
	    {
		addPoint( c, z );
		return oldsz;
	    }
	    else
	    {
		float prevdist = pos_[mindistidx-1].distTo(cnew);
		float nextdist = pos_[mindistidx+1].distTo(cnew);
		minidx = prevdist > nextdist ? mindistidx : mindistidx -1;
	    }
	}
    }

    if ( minidx == 0 )
    {
	// The point may be before the first
	const Coord3& c0 = pos_[0];
	const Coord3& c1 = pos_[1];
	const float d01sq = c0.sqDistTo( c1 );
	const float d0nsq = c0.sqDistTo( cnew );
	const float d1nsq = c1.sqDistTo( cnew );
	if ( d01sq + d0nsq < d1nsq )
	    minidx = -1;
    }
    if ( minidx == oldsz-2 )
    {
	// Hmmm. The point may be beyond the last
	const Coord3& c0 = pos_[oldsz-2];
	const Coord3& c1 = pos_[oldsz-1];
	const float d01sq = c0.sqDistTo( c1 );
	const float d0nsq = c0.sqDistTo( cnew );
	const float d1nsq = c1.sqDistTo( cnew );
	if ( d01sq + d1nsq < d0nsq )
	    minidx = oldsz-1;
    }

    insertAfterIdx( minidx, cnew );
    return minidx+1;
}


void Well::Track::setPoint( int idx, const Coord& c, float z )
{
    const int nrpts = pos_.size();
    if ( idx<0 || idx>=nrpts ) return;

    Coord3 oldpt( pos_[idx] );
    Coord3 newpt( c.x, c.y, z );
    float olddist0 = idx > 0 ? oldpt.distTo(pos_[idx-1]) : 0;
    float newdist0 = idx > 0 ? newpt.distTo(pos_[idx-1]) : 0;
    float olddist1 = 0, newdist1 = 0;
    if ( idx < nrpts-1 )
    {
	olddist1 = oldpt.distTo(pos_[idx+1]);
	newdist1 = newpt.distTo(pos_[idx+1]);
    }

    pos_[idx] = newpt;
    dah_[idx] += newdist0 - olddist0;
    addToDahFrom( idx+1, newdist0 - olddist0 + newdist1 - olddist1 );
}


void Well::Track::removePoint( int idx )
{
    float olddist = dah_[idx+1] - dah_[idx-1];
    float newdist = pos_[idx+1].distTo( pos_[idx-1] );
    float extradah = olddist - newdist;
    removeFromDahFrom( idx+1, extradah );
    remove( idx );
}


Coord3 Well::Track::getPos( float dh ) const
{
    int idx1;
    if ( IdxAble::findFPPos(dah_,dah_.size(),dh,-1,idx1) )
	return pos_[idx1];
    else if ( idx1 < 0 || idx1 == dah_.size()-1 )
	return Coord3(0,0,0);

    return coordAfterIdx( dh, idx1 );
}


Coord3 Well::Track::coordAfterIdx( float dh, int idx1 ) const
{
    const int idx2 = idx1 + 1;
    const float d1 = dh - dah_[idx1];
    const float d2 = dah_[idx2] - dh;
    const Coord3& c1 = pos_[idx1];
    const Coord3& c2 = pos_[idx2];
    const float f = 1. / (d1 + d2);
    return Coord3( f * (d1 * c2.x + d2 * c1.x), f * (d1 * c2.y + d2 * c1.y),
		   f * (d1 * c2.z + d2 * c1.z) );
}


float Well::Track::getDahForTVD( float z, float prevdah ) const
{
    if ( zistime_ )
	{ pErrMsg("Cannot compare TVD to time"); return dah_[0]; }

    bool haveprevdah = !mIsUdf(prevdah);
    int foundidx = -1;
    for ( int idx=0; idx<pos_.size(); idx++ )
    {
	const Coord3& c = pos_[idx];
	float cz = pos_[idx].z;
	if ( haveprevdah && prevdah-1e-4 > dah_[idx] )
	    continue;
	if ( pos_[idx].z + 1e-4 > z )
	    { foundidx = idx; break; }
    }
    if ( foundidx < 1 )
	return foundidx ? mUdf(float) : dah_[0];

    const int idx1 = foundidx - 1;
    const int idx2 = foundidx;
    float z1 = pos_[idx1].z;
    float z2 = pos_[idx2].z;
    float dah1 = dah_[idx1];
    float dah2 = dah_[idx2];
    return ((z-z1) * dah2 + (z2-z) * dah1) / (z2-z1);
}


float Well::Track::nearestDah( const Coord3& posin ) const
{
    if ( dah_.isEmpty() ) return 0;

    if ( dah_.size() < 2 ) return dah_[1];

    const float zfac = zistime_ ? 2000 : 1;
    Coord3 pos( posin ); pos.z *= zfac;
    Coord3 curpos( getPos( dah_[0] ) ); curpos.z *= zfac;
    float sqneardist = curpos.sqDistTo( pos );
    float sqsecdist = sqneardist;
    int nearidx = 0; int secondidx = 0;
    curpos = getPos( dah_[1] ); curpos.z *= zfac;
    float sqdist = curpos.sqDistTo( pos );
    if ( sqdist < sqneardist )
	{ nearidx = 1; sqneardist = sqdist; }
    else
	{ secondidx = 1; sqsecdist = sqdist; }
	
    for ( int idah=2; idah<dah_.size(); idah++ )
    {
	curpos = getPos( dah_[idah] ); curpos.z *= zfac;
	sqdist = curpos.sqDistTo( pos );
	if ( sqdist < 0.1 ) return dah_[idah];

	if ( sqdist < sqneardist )
	{
	    secondidx = nearidx; sqsecdist = sqneardist;
	    nearidx = idah; sqneardist = sqdist;
	}
	else if ( sqdist < sqsecdist )
	    { secondidx = idah; sqsecdist = sqdist; }

	if ( sqdist > 2 * sqneardist ) // safe for 'reasonable wells'
	    break;
    }

    const float neardist = sqrt( sqneardist );
    const float secdist = sqrt( sqsecdist );
    return (neardist*dah_[secondidx] + secdist * dah_[nearidx])
         / (neardist + secdist);
}


bool Well::Track::alwaysDownward() const
{
    if ( pos_.size() < 2 )
	return pos_.size();

    float prevz = pos_[0].z;
    for ( int idx=1; idx<pos_.size(); idx++ )
    {
	float curz = pos_[idx].z;
	if ( curz <= prevz )
	    return false;
	prevz = curz;
    }

    return true;
}


void Well::Track::toTime( const D2TModel& d2t )
{
    TypeSet<float> newdah;
    TypeSet<Coord3> newpos;

    // We need to collect control points from both the track and the d2t model
    // because both will be 'bend' points in time

    // First, get the dahs + positions from both - in depth.
    // We'll start with the first track point
    int d2tidx = 0;
    float curdah = dah_[0];
    while ( d2tidx < d2t.size() && d2t.dah(d2tidx) < curdah + mDefEps )
	d2tidx++; // don't need those points: before well track
    newdah += curdah;
    newpos += pos_[0];

    // Now collect the rest of the track points and the d2t control points
    // Make sure no phony double points: allow a tolerance
    const float tol = 0.001;
    for ( int trckidx=1; trckidx<dah_.size(); trckidx++ )
    {
	curdah = dah_[trckidx];
	while ( d2tidx < d2t.size() )
	{
	    const float d2tdah = d2t.dah( d2tidx );
	    const float diff = d2tdah - curdah;
	    if ( diff > tol ) // d2t dah is further down track; handle later
		break;
	    else if ( diff <= -tol ) // therfore a dah not on the track
	    {
		newdah += d2tdah;
		newpos += getPos( d2tdah );
	    }
	    d2tidx++;
	}
	newdah += curdah;
	newpos += pos_[trckidx];
    }

    // Copy the extended set into the new track definition
    dah_ = newdah;
    pos_ = newpos;

    // Now, convert to time
    for ( int idx=0; idx<dah_.size(); idx++ )
    {
	Coord3& pt = pos_[idx];
	pt.z = d2t.getTime( dah_[idx] );
    }

    zistime_ = true;
}


Well::D2TModel& Well::D2TModel::operator =( const Well::D2TModel& d2t )
{
    if ( &d2t != this )
    {
	setName( d2t.name() );
	desc = d2t.desc; datasource = d2t.datasource;
	dah_ = d2t.dah_; t_ = d2t.t_;
    }
    return *this;
}


float Well::D2TModel::getTime( float dh ) const
{
    int idx1;
    if ( IdxAble::findFPPos(dah_,dah_.size(),dh,-1,idx1) )
	return t_[idx1];
    else if ( dah_.size() < 2 )
	return mUdf(float);
    else if ( idx1 < 0 || idx1 == dah_.size()-1 )
    {
	// Extrapolate. Is this correct?
	int idx0 = idx1 < 0 ? 1 : idx1;
	const float v = (dah_[idx0] - dah_[idx0-1]) / (t_[idx0] - t_[idx0-1]);
	idx0 = idx1 < 0 ? 0 : idx1;
	return t_[idx0] + ( dh - dah_[idx0] ) / v;
    }

    const int idx2 = idx1 + 1;
    const float d1 = dh - dah_[idx1];
    const float d2 = dah_[idx2] - dh;
    return (d1 * t_[idx2] + d2 * t_[idx1]) / (d1 + d2);
}


float Well::D2TModel::getDepth( float time ) const
{
    int idx1;
    if ( IdxAble::findFPPos(t_,t_.size(),time,-1,idx1) )
	return dah_[idx1];
    else if ( t_.size() < 2 )
	return mUdf(float);
    else if ( idx1 < 0 || idx1 == t_.size()-1 )
    {
	// Extrapolate. Is this correct?
	int idx0 = idx1 < 0 ? 1 : idx1;
	const float v = (dah_[idx0] - dah_[idx0-1]) / (t_[idx0] - t_[idx0-1]);
	idx0 = idx1 < 0 ? 0 : idx1;
	return dah_[idx0] + ( time - t_[idx0] ) * v;
    }

    const int idx2 = idx1 + 1;
    const float t1 = time - t_[idx1];
    const float t2 = t_[idx2] - time;
    return (t1 * dah_[idx2] + t2 * dah_[idx1]) / (t1 + t2);
}


float Well::D2TModel::getVelocity( float dh ) const
{
    if ( dah_.size() < 2 ) return mUdf(float);

    int idx1;
    IdxAble::findFPPos( dah_, dah_.size(), dh, -1, idx1 );
    if ( idx1 < 1 )
	idx1 = 1;
    else if ( idx1 > dah_.size()-1 )
	idx1 = dah_.size() - 1;

    int idx0 = idx1 - 1;
    return (dah_[idx1] - dah_[idx0]) / (t_[idx1] - t_[idx0]);
}


#define mName "Well name"

void Well::Info::fillPar(IOPar& par) const
{
    par.set( mName, name() );
    par.set( sKeyuwid, uwid );
    par.set( sKeyoper, oper );
    par.set( sKeystate, state );
    par.set( sKeycounty, county );

    BufferString coord;
    surfacecoord.fill( coord.buf() );
    par.set( sKeycoord, coord );

    par.set( sKeyelev, surfaceelev );
}

void Well::Info::usePar(const IOPar& par)
{
    setName( par[mName] );
    par.get( sKeyuwid, uwid );
    par.get( sKeyoper, oper );
    par.get( sKeystate, state );
    par.get( sKeycounty, county );

    BufferString coord;
    par.get( sKeycoord, coord );
    surfacecoord.use( coord );

    par.get( sKeyelev, surfaceelev );
}

