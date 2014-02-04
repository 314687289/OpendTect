/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "wellmarker.h"
#include "welltrack.h"
#include "iopar.h"
#include "stratlevel.h"
#include "bufstringset.h"
#include "idxable.h"
#include "keystrs.h"

const char* Well::Marker::sKeyDah()	{ return "Depth along hole"; }



Well::Marker& Well::Marker::operator =( const Well::Marker& mrk )
{
    if ( this != &mrk )
    {
	setName( mrk.name() );
	dah_ = mrk.dah();
	levelid_ = mrk.levelID();
	color_ = mrk.color();
    }
    return *this;
}


Color Well::Marker::color() const
{
    if ( levelid_ >= 0 )
    {
	const Strat::Level* lvl = Strat::LVLS().get( levelid_ );
	if ( lvl )
	    return lvl->color();
    }
    return color_;
}


ObjectSet<Well::Marker>& Well::MarkerSet::operator += ( Well::Marker* mrk )
{
    if ( mrk && !isPresent( mrk->name().buf() ) )
	ObjectSet<Well::Marker>::operator += ( mrk );

    return *this;
}


Well::Marker* Well::MarkerSet::gtByName( const char* mname ) const
{
    const int idx = indexOf( mname );
    return  idx < 0 ? 0 : const_cast<Well::Marker*>((*this)[idx]);
}


int Well::MarkerSet::getIdxAbove( float reqz, const Well::Track* trck ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const Marker& mrk = *(*this)[idx];
	float mrkz = mrk.dah();
	if ( trck )
	    mrkz = mCast(float,trck->getPos(mrkz).z);
	if ( mrkz > reqz )
	    return idx-1;
    }
    return size() - 1;
}


int Well::MarkerSet::indexOf( const char* mname ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( (*this)[idx]->name()==mname )
	    return idx;
    }
    return -1;
}


bool Well::MarkerSet::insertNew( Well::Marker* newmrk )
{
    if ( !newmrk || isPresent(newmrk->name().buf()) )
	{ delete newmrk; return false; }

    int newidx = 0;
    for ( int imrk=0; imrk<size(); imrk++ )
    {
	Well::Marker& mrk = *(*this)[imrk];
	if ( newmrk->dah() < mrk.dah() )
	    break;
	newidx++;
    }
    insertAt( newmrk, newidx );
    return true;
}


void Well::MarkerSet::addCopy( const ObjectSet<Well::Marker>& ms,
				int idx, float dah )
{
    Well::Marker* newwm = new Marker( *ms[idx] );
    newwm->setDah( dah );
    insertNew( newwm );
}


void Well::MarkerSet::addSameWell( const ObjectSet<Well::Marker>& ms )
{
    const int mssz = ms.size();
    for ( int idx=0; idx<mssz; idx++ )
    {
	if ( !isPresent(ms[idx]->name()) )
	    insertNew( new Well::Marker( *ms[idx] ) );
    }
}


void Well::MarkerSet::mergeOtherWell( const ObjectSet<Well::Marker>& ms )
{
    if ( ms.isEmpty() )
	return;

	// Any new (i.e. not present in this set) markers there?
    TypeSet<int> myidxs;
    const int mssz = ms.size();
    bool havenew = false;
    for ( int msidx=0; msidx<mssz; msidx++ )
    {
	const int myidx = indexOf( ms[msidx]->name() );
	myidxs += myidx;
	if ( myidx < 0 )
	    havenew = true;
    }
    if ( !havenew )
	return; // no? then we're cool already. Nothing to do.

	// First first and last common markers.
    int msidxfirstmatch = -1; int msidxlastmatch = -1;
    for ( int msidx=0; msidx<myidxs.size(); msidx++ )
    {
	if ( myidxs[msidx] >= 0 )
	{
	    msidxlastmatch = msidx;
	    if ( msidxfirstmatch < 0 )
		msidxfirstmatch = msidx;
	}
    }
    if ( msidxfirstmatch < 0 ) // what can we do? let's hope dahs are compatible
	{ addSameWell( ms ); return; }

	// Add the markers above and below
    float edgediff = ms[msidxfirstmatch]->dah()
			- (*this)[ myidxs[msidxfirstmatch] ]->dah();
    for ( int msidx=0; msidx<msidxfirstmatch; msidx++ )
	addCopy( ms, msidx, ms[msidx]->dah() - edgediff );

    edgediff = ms[msidxlastmatch]->dah()
			- (*this)[ myidxs[msidxlastmatch] ]->dah();
    for ( int msidx=msidxlastmatch+1; msidx<mssz; msidx++ )
	addCopy( ms, msidx, ms[msidx]->dah() - edgediff );

    if ( msidxfirstmatch == msidxlastmatch )
	return;

	// There are new markers in the middle. Set up positioning framework.
    TypeSet<float> xvals, yvals;
    for ( int msidx=msidxfirstmatch; msidx<=msidxlastmatch; msidx++ )
    {
	const int myidx = myidxs[msidx];
	if ( myidx >= 0 )
	{
	    xvals += (*this)[myidx]->dah();
	    yvals += ms[msidx]->dah();
	}
    }

	// Now add the new markers at a good place.
    const int nrpts = xvals.size();
    for ( int msidx=msidxfirstmatch+1; msidx<msidxlastmatch; msidx++ )
    {
	if ( myidxs[msidx] >= 0 )
	    continue;

	int loidx;
	const float msdah = ms[msidx]->dah();
	if ( IdxAble::findFPPos(yvals,nrpts,msdah,msidxfirstmatch,loidx) )
	    continue; // Two markers in ms at same pos. Ignore this one.

	const float relpos = (msdah - yvals[loidx])
			   / (yvals[loidx+1]-yvals[loidx]);
	addCopy( ms, msidx, relpos*xvals[loidx+1] + (1-relpos)*xvals[loidx] );
    }
}


Well::Marker* Well::MarkerSet::gtByLvlID( int lvlid ) const
{
    if ( lvlid<=0 ) return 0;
    for ( int idmrk=0; idmrk<size(); idmrk++ )
    {
	Well::Marker* mrk = const_cast<Well::Marker*>((*this)[idmrk]);
	if ( mrk && mrk->levelID() == lvlid )
	    return mrk;
    }
    return 0;
}


void Well::MarkerSet::getNames( BufferStringSet& nms ) const
{
    for ( int idx=0; idx<size(); idx++ )
	nms.add( (*this)[idx]->name() );
}


void Well::MarkerSet::getColors( TypeSet<Color>& cols ) const
{
    for ( int idx=0; idx<size(); idx++ )
	cols += (*this)[idx]->color();
}


void Well::MarkerSet::fillPar( IOPar& iop ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	IOPar mpar;
	const Marker& mrk = *(*this)[ idx ];
	mpar.set( sKey::Name(), mrk.name() );
	mpar.set( sKey::Color(), mrk.color() );
	mpar.set( sKey::Depth(), mrk.dah() );
	mpar.set( sKey::Level(), mrk.levelID() );
	iop.mergeComp( mpar, ::toString(idx+1) );
    }
}


void Well::MarkerSet::usePar( const IOPar& iop )
{
    setEmpty();

    for ( int imrk=1; ; imrk++ )
    {
	PtrMan<IOPar> mpar = iop.subselect( imrk );
	if ( !mpar || mpar->isEmpty() )
	    break;

	BufferString nm; mpar->get( sKey::Name(), nm );
	if ( nm.isEmpty() || isPresent(nm) )
	    continue;

	float dpt = 0; mpar->get( sKey::Depth(), dpt );
	Color col(0,0,0); mpar->get( sKey::Color(), col );
	int lvlid = -1; mpar->get( sKey::Level(), lvlid );

	Marker* mrk = new Marker( nm, dpt );
	mrk->setColor( col ); mrk->setLevelID( lvlid );
	insertNew( mrk );
    }
}


Well::MarkerRange::MarkerRange( const Well::MarkerSet& ms, Interval<int> rg )
    : markers_(ms)
    , rg_(rg)
{
    const int inpsz = ms.size();
    if ( inpsz < 1 )
	rg_.start = rg_.stop = -1;
    else
    {
	if ( rg_.start < 0 ) rg_.start = 0;
	if ( rg_.stop < 0 ) rg_.stop = inpsz - 1;
	rg_.sort();
	if ( rg_.start >= inpsz ) rg_.start = inpsz - 1;
	if ( rg_.stop >= inpsz ) rg_.stop = inpsz - 1;
    }
}


bool Well::MarkerRange::isValid() const
{
    const int inpsz = markers_.size();
    return inpsz > 0
	&& rg_.start >= 0 && rg_.stop >= 0
	&& rg_.start < inpsz && rg_.stop < inpsz
	&& rg_.start <= rg_.stop;
}


bool Well::MarkerRange::isIncluded( const char* nm ) const
{
    if ( !isValid() ) return false;

    for ( int idx=rg_.start; idx<=rg_.stop; idx++ )
	if ( markers_[idx]->name() == nm )
	    return true;
    return false;
}


bool Well::MarkerRange::isIncluded( float z ) const
{
    if ( !isValid() ) return false;

    return z >= markers_[rg_.start]->dah()
	&& z <= markers_[rg_.stop]->dah();
}


void Well::MarkerRange::getNames( BufferStringSet& nms ) const
{
    if ( !isValid() ) return;

    for ( int idx=rg_.start; idx<=rg_.stop; idx++ )
	nms.add( markers_[idx]->name() );
}


Well::MarkerSet* Well::MarkerRange::getResultSet() const
{
    Well::MarkerSet* ret = new Well::MarkerSet;
    if ( !isValid() ) return ret;

    for ( int idx=rg_.start; idx<=rg_.stop; idx++ )
	*ret += new Well::Marker( *markers_[idx] );
    return ret;
}
