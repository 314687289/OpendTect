/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-6-1996
-*/

static const char* rcsID = "$Id: binidvalset.cc,v 1.26 2008-11-24 15:01:13 cvskris Exp $";

#include "binidvalset.h"
#include "iopar.h"
#include "separstr.h"
#include "idxable.h"
#include "sorting.h"
#include "strmoper.h"
#include "survinfo.h"
#include <iostream>


static inline void setToUdf( float* arr, int nvals )
{
    for ( int idx=0; idx<nvals; idx++ )
	Values::setUdf( arr[idx] );
}


static int findIndexFor( const TypeSet<int>& nrs, int nr, bool* found = 0 )
{
    int ret;
    bool fnd = IdxAble::findPos( nrs.arr(), nrs.size(), nr, -1, ret );
    if ( found ) *found = fnd;
    return ret;
}


BinIDValueSet::BinIDValueSet( int nv, bool ad )
	: nrvals_(nv)
	, allowdup_(ad)
{
}


BinIDValueSet::BinIDValueSet( const BinIDValueSet& s )
    	: nrvals_(0)
{
    *this = s;
}


BinIDValueSet::~BinIDValueSet()
{
    empty();
}


BinIDValueSet& BinIDValueSet::operator =( const BinIDValueSet& bvs )
{
    if ( &bvs == this ) return *this;

    copyStructureFrom( bvs );

    for ( int iinl=0; iinl<bvs.inls_.size(); iinl++ )
    {
	inls_ += bvs.inls_[iinl];
	crlsets_ += new TypeSet<int>( *bvs.crlsets_[iinl] );
	valsets_ += new TypeSet<float>( *bvs.valsets_[iinl] );
    }

    return *this;
}


void BinIDValueSet::empty()
{
    inls_.erase();
    deepErase( crlsets_ );
    deepErase( valsets_ );
}


void BinIDValueSet::append( const BinIDValueSet& bvs )
{
    Pos pos; BinID bid;
    if ( nrvals_ <= bvs.nrvals_ )
    {
	while ( bvs.next(pos,!bvs.allowdup_) )
	{
	    bvs.get( pos, bid );
	    add( bid, nrvals_ ? bvs.getVals( pos ) : 0 );
	}
    }
    else
    {
	float* insvals = new float [nrvals_];
	setToUdf(insvals,nrvals_);
	while ( bvs.next(pos,!allowdup_) )
	{
	    bvs.get( pos, bid );
	    memcpy( insvals, bvs.getVals( pos ), bvs.nrvals_ * sizeof(float) );
	    add( bid, insvals );
	}
	delete [] insvals;
    }
}


void BinIDValueSet::remove( const BinIDValueSet& removebids ) 
{ 
    BinIDValueSet::Pos pos; 
	 
    while ( removebids.next(pos, true ) ) 
    { 
	//TODO: This loop can be optimized somewhat by reusing old pos,
	//but let's play it safe for now.
	const BinID bid = removebids.getBinID( pos ); 
	BinIDValueSet::Pos removepos = findFirst( bid ); 
					 
	while ( removepos.j>=0 ) 
	{ 
	    remove( removepos ); 
	    removepos = findFirst( bid ); 
	} 
    } 
} 


bool BinIDValueSet::getFrom( std::istream& strm )
{
    empty();
    setNrVals( 0, false );

    char linebuf[4096], valbuf[1024];
    Coord crd; BinID bid;
    bool first_time = true;
    while ( strm.getline( linebuf, 4096 ) )
    {
	char* firstchar = linebuf;
	mSkipBlanks( firstchar );
	if ( *firstchar == '"' )
	{
	    char* ptr = strchr( firstchar+1, '"' );
	    if ( !ptr ) continue;
	    firstchar = ptr+1;
	}
	mSkipBlanks( firstchar );
	if ( !*firstchar || (*firstchar != '-' && !isdigit(*firstchar) ) )
	    continue;

	const char* nextword = getNextWord( firstchar, valbuf );
	crd.x = atof( valbuf );
	mSkipBlanks( nextword ); if ( !*nextword ) continue;
	nextword = getNextWord( nextword, valbuf );
	crd.y = atof( valbuf );

	bid = SI().transform( crd );
	if ( !SI().isReasonable(bid) )
	{
	    bid.inl = (int)crd.x; bid.crl = (int)crd.y;
	    if ( !SI().isReasonable(bid) )
		continue;
	}

	if ( first_time )
	{
	    first_time = false;
	    const char* firstval = nextword;
	    int nrvalsfound = 0;
	    while ( true )
	    {
		mSkipBlanks( nextword ); if ( !*nextword ) break;
		nrvalsfound++;
		nextword = getNextWord( nextword, valbuf );
	    }
	    setNrVals( nrvalsfound, false );
	    nextword = firstval;
	}

	float* vals = getVals( add(bid) );
	if ( !vals ) continue;
	for ( int idx=0; idx<nrVals(); idx++ )
	{
	    mSkipBlanks( nextword ); if ( !*nextword ) break;
	    nextword = getNextWord( nextword, valbuf );
	    if ( !valbuf[0] ) break;
	    vals[idx] = (float)atof(valbuf);
	}
    }

    return !isEmpty();
}


bool BinIDValueSet::putTo( std::ostream& strm ) const
{
    Pos pos;
    while ( next(pos) )
    {
	const BinID bid( getInl(pos), getCrl(pos) );
	const float* vals = getVals(pos);
	strm << bid.inl << '\t' << bid.crl;
	for ( int idx=0; idx<nrvals_; idx++ )
	    strm << '\t' << getStringFromFloat(0,vals[idx]);
	strm << '\n';
    }
    strm.flush();
    return strm.good();
}


int BinIDValueSet::nrCrls( int inl ) const
{
    const int inlidx = inls_.indexOf( inl );
    return inlidx<0 ? 0 : getCrlSet(inlidx).size();
}


Interval<int> BinIDValueSet::inlRange() const
{
    Interval<int> ret( mUdf(int), mUdf(int) );

    bool first = true;
    for ( int iinl=0; iinl<inls_.size(); iinl++ )
    {
	if ( first )
	    { ret.start = ret.stop = inls_[iinl]; first = false; }
	else
	    ret.include( inls_[iinl], false );
    }
    return ret;
}


Interval<int> BinIDValueSet::crlRange( int inl ) const
{
    Interval<int> ret( mUdf(int), mUdf(int) );
    if ( inls_.isEmpty() ) return ret;

    const int inlidx = inls_.indexOf( inl );
    if ( inlidx >= 0 )
    {
	const TypeSet<int>& crlset = getCrlSet(inlidx);
	for ( int idx=0; idx<crlset.size(); idx++ )
	    ret.include( crlset[idx], false );
	return ret;
    }
    
    Pos pos; BinID bid;
    bool first = true;
    while ( next(pos) )
    {
	get( pos, bid );
	if ( first )
	    { ret.start = ret.stop = bid.crl; first = false; }
	else
	    ret.include( bid.crl, false );
    }

    return ret;
}


Interval<float> BinIDValueSet::valRange( int valnr ) const
{
    Interval<float> ret( mUdf(float), mUdf(float) );
    if ( valnr >= nrvals_ || valnr < 0 || isEmpty() )
	return ret;

    Pos pos;
    while ( next(pos) )
    {
	const float val = getVals(pos)[valnr];
	if ( !mIsUdf(val) )
	    { ret.start = ret.stop = val; break; }
    }
    while ( next(pos) )
    {
	const float val = getVals(pos)[valnr];
	if ( !mIsUdf(val) )
	    ret.include( val, false );
    }

    return ret;
}


void BinIDValueSet::copyStructureFrom( const BinIDValueSet& bvs )
{
    empty();
    const_cast<int&>(nrvals_) = bvs.nrvals_;
    allowdup_ = bvs.allowdup_;
}


BinIDValueSet::Pos BinIDValueSet::findFirst( const BinID& bid ) const
{
    bool found; int idx = findIndexFor(inls_,bid.inl,&found);
    Pos pos( found ? idx : -1, -1 );
    if ( pos.i >= 0 )
    {
	const TypeSet<int>& crls = getCrlSet(pos);
	idx = findIndexFor(crls,bid.crl,&found);
	pos.j = found ? idx : -1;
	if ( found )
	{
	    pos.j = idx;
	    while ( pos.j && crls[pos.j-1] == bid.crl )
		pos.j--;
	}
    }

    return pos;
}


bool BinIDValueSet::next( BinIDValueSet::Pos& pos, bool skip_dup ) const
{
    if ( pos.i < 0 )
    {
	if ( inls_.size() < 1 ) return false;
	pos.i = pos.j = 0;
	return true;
    }
    else if ( pos.i >= inls_.size() )
	{ pos.i = pos.j = -1; return false; }
    else if ( pos.j < 0 )
    	{ pos.j = 0; return true; }

    const TypeSet<int>& crls = getCrlSet(pos);
    if ( pos.j > crls.size()-2 )
    {
	pos.j = 0;
	pos.i++;
	if ( pos.i >= inls_.size() )
	    pos.i = pos.j = -1;
	return pos.i >= 0;
    }

    pos.j++;
    if ( skip_dup && crls[pos.j] == crls[pos.j-1] )
	return next( pos, true );

    return true;
}


bool BinIDValueSet::prev( BinIDValueSet::Pos& pos, bool skip_dup ) const
{
    if ( !pos.valid() )
	return false;
    if ( pos.i == 0 && pos.j == 0)
	{ pos.i = pos.j = -1; return false; }

    int curcrl = getCrl(pos);
    if ( pos.j )
	pos.j--;
    else
    {
	pos.i--;
	pos.j = getCrlSet(pos).size() - 1;
    }

    if ( !skip_dup ) return true;

    while ( getCrl(pos) == curcrl )
	return prev( pos, true );

    return true;
}


bool BinIDValueSet::valid( const BinID& bid ) const
{
    Pos pos = findFirst( bid );
    return pos.valid()
	&& inls_.indexOf(bid.inl) >= 0
	&& getCrlSet(pos).size() > pos.j;
}


void BinIDValueSet::get( const Pos& pos, BinID& bid, float* vs,
       			 int maxnrvals ) const
{
    if ( maxnrvals < 0 || maxnrvals > nrvals_ ) maxnrvals = nrvals_;

    if ( !pos.valid() )
	{ bid.inl = bid.crl = 0; }
    else
    {
	bid.inl = getInl(pos); bid.crl = getCrl(pos);
	if ( vs && maxnrvals )
	{
	    memcpy( vs, getVals(pos), maxnrvals * sizeof(float) );
	    return;
	}
    }

    if ( vs ) 
	setToUdf(vs,maxnrvals);
}


BinID BinIDValueSet::getBinID( const Pos& pos ) const
{
    return pos.valid() ? BinID(getInl(pos),getCrl(pos)) : BinID(0,0);
}


BinIDValueSet::Pos BinIDValueSet::getPos( int glidx ) const
{
    int firstidx = 0; Pos pos;
    for ( pos.i=0; pos.i<inls_.size(); pos.i++ )
    {
	const TypeSet<int>& crls = getCrlSet(pos);
	if ( firstidx + crls.size() > glidx )
	{
	    pos.j = glidx - firstidx;
	    return pos;
	}
	firstidx += crls.size();
    }

    return Pos(-1,-1);
}


BinIDValueSet::Pos BinIDValueSet::add( const BinID& bid, const float* arr )
{
    Pos pos( findFirst(bid) );
    if ( pos.i < 0 )
    {
	pos.i = findIndexFor(inls_,bid.inl) + 1;
	if ( pos.i > inls_.size()-1 )
	{
	    inls_ += bid.inl;
	    crlsets_ += new TypeSet<int>;
	    valsets_ += new TypeSet<float>;
	    pos.i = inls_.size() - 1;
	}
	else
	{
	    inls_.insert( pos.i, bid.inl );
	    crlsets_.insertAt( new TypeSet<int>, pos.i );
	    valsets_.insertAt( new TypeSet<float>, pos.i );
	}
    }

    if ( pos.j < 0 || allowdup_ )
	addNew( pos, bid.crl, arr );

    return pos;
}


void BinIDValueSet::addNew( BinIDValueSet::Pos& pos, int crl, const float* arr )
{
    TypeSet<int>& crls = getCrlSet(pos);

    if ( pos.j < 0 )
	pos.j = findIndexFor(crls,crl) + 1;
    else
    {
	pos.j++;
	while ( pos.j < crls.size() && crls[pos.j] == crl )
	    pos.j++;
    }


    TypeSet<float>& vals = getValSet(pos);
    if ( pos.j > crls.size() - 1 )
    {
	crls += crl;
	for ( int idx=0; idx<nrvals_; idx++ )
	    vals += arr ? arr[idx] : mUdf(float);
    }
    else
    {
	crls.insert( pos.j, crl );
	//TOOPTIM: with memcpy's. This will be slow for high nrvals_
	for ( int idx=nrvals_-1; idx>=0; idx-- )
	    vals.insert( pos.j*nrvals_, arr ? arr[idx] : mUdf(float) );
    }
}


BinIDValueSet::Pos BinIDValueSet::add( const BinIDValues& bivs )
{
    if ( bivs.size() >= nrvals_ )
	return add( bivs.binid, bivs.values() );

    BinIDValues locbivs( 0, 0, nrvals_ );
    for ( int idx=0; idx<bivs.size(); idx++ )
	locbivs.value(idx) = bivs.value(idx);
    for ( int idx=bivs.size(); idx<nrvals_; idx++ )
	Values::setUdf( locbivs.value(idx) );
    return add( bivs.binid, locbivs.values() );
}


void BinIDValueSet::set( BinIDValueSet::Pos pos, const float* vals )
{
    if ( !pos.valid() || !nrvals_ ) return;

    if ( vals )
	memcpy( getVals(pos), vals, nrvals_*sizeof(float) );
    else
	setToUdf( getVals(pos), nrvals_ );
}


int BinIDValueSet::nrPos( int inlidx ) const
{
    return inlidx < 0 || inlidx >= inls_.size() ? 0 : getCrlSet(inlidx).size();
}


int BinIDValueSet::totalSize() const
{
    int nr = 0;
    for ( int idx=0; idx<inls_.size(); idx++ )
	nr += getCrlSet(idx).size();
    return nr;
}


bool BinIDValueSet::hasInl( int inl ) const
{
    return inls_.indexOf(inl) >= 0;
}


bool BinIDValueSet::hasCrl( int crl ) const
{
    for ( int iinl=0; iinl<inls_.size(); iinl++ )
    {
	const TypeSet<int>& crls = getCrlSet(iinl);
	for ( int icrl=0; icrl<crls.size(); icrl++ )
	    if ( crls[icrl] == crl ) return true;
    }
    return false;
}


BinID BinIDValueSet::firstPos() const
{
    Pos pos; next(pos);
    BinID bid; get(pos,bid);
    return bid;
}


void BinIDValueSet::remove( const Pos& pos )
{
    if ( pos.i < 0 || pos.i >= inls_.size() )
	return;

    TypeSet<int>& crls = getCrlSet(pos);
    if ( pos.j < 0 || pos.j >= crls.size() )
	return;

    crls.remove( pos.j );
    if ( crls.size() )
    {
	if ( nrvals_ )
	    getValSet(pos).remove( pos.j*nrvals_, (pos.j+1)*nrvals_ - 1 );
    }
    else
    {
	inls_.remove( pos.i );
	delete crlsets_[pos.i];
	crlsets_.remove( pos.i );
	delete valsets_[pos.i];
	valsets_.remove( pos.i );
    }
}


void BinIDValueSet::remove( const TypeSet<BinIDValueSet::Pos>& poss )
{
    if ( poss.size() < 1 )
	return;
    else if ( poss.size() == 1 )
	{ remove( poss[0] ); return; }

    BinIDValueSet bvs( *this );
    empty();
    Pos pos; BinID bid;
    while ( bvs.next(pos) )
    {
	if ( poss.indexOf(pos) < 0 )
	{
	    bvs.get( pos, bid );
	    add( bid, bvs.getVals(pos) );
	}
    }
}


void BinIDValueSet::removeDuplicateBids()
{
    if ( isEmpty() ) return;

    Pos pos; next(pos,false);
    BinID prevbid; get( pos, prevbid );
    TypeSet<BinIDValueSet::Pos> poss;
    BinID cur;
    while ( next(pos,false) )
    {
	get( pos, cur );
	if ( prevbid == cur )
	    poss += pos;
	else
	    prevbid = cur;
    }
    remove( poss );
}


void BinIDValueSet::removeVal( int validx )
{
    if ( validx < 0 || validx >= nrvals_ )
	return;

    if ( nrvals_ == 1 )
    {
	setNrVals( 0, false );
	return;
    }

    for ( int iinl=0; iinl<inls_.size(); iinl++ )
    {
	TypeSet<float>& vals = getValSet(iinl);
	TypeSet<int>& crls = getCrlSet(iinl);
	for ( int icrl=crls.size()-1; icrl>=0; icrl-- )
	    vals.remove( nrvals_*icrl+validx );
    }
    const_cast<int&>(nrvals_)--;
}


void BinIDValueSet::setNrVals( int newnrvals, bool keepdata )
{
    if ( newnrvals == nrvals_ )
	return;

    const int oldnrvals = nrvals_;
    const_cast<int&>( nrvals_ ) = newnrvals;

    for ( int iinl=0; iinl<inls_.size(); iinl++ )
    {
	const int nrcrl = getCrlSet(iinl).size();
	if ( nrvals_ == 0 )
	    getValSet(iinl).erase();
	else if ( oldnrvals == 0 )
	    getValSet(iinl).setSize( nrcrl * nrvals_, mUdf(float) );
	else
	{
	    TypeSet<float>* oldvals = valsets_[iinl];
	    TypeSet<float>* newvals = new TypeSet<float>( nrcrl*nrvals_,
		    					  mUdf(float) );
	    valsets_.replace( iinl, newvals );
	    if ( keepdata )
	    {
		float* oldarr = oldvals->arr();
		float* newarr = newvals->arr();
		const int cpsz = (oldnrvals > nrvals_ ? nrvals_ : oldnrvals)
		    		   * sizeof( float );
		for ( int icrl=0; icrl<nrcrl; icrl++ )
		    memcpy( newarr+icrl*nrvals_, oldarr+icrl*oldnrvals, cpsz );
	    }
	    delete oldvals;
	}
    }
}


void BinIDValueSet::sortDuplicateBids( int valnr, bool asc )
{
    if ( valnr >= nrvals_ || !allowdup_ ) return;

    for ( int iinl=0; iinl<inls_.size(); iinl++ )
    {
	TypeSet<int>& crls = getCrlSet(iinl);
	TypeSet<float>& vals = getValSet(iinl);
	for ( int icrl=1; icrl<crls.size(); icrl++ )
	{
	    int curcrl = crls[icrl];
	    int firstdup = icrl - 1;
	    if ( crls[icrl-1] == curcrl )
	    {
		for ( int idup=icrl+1; idup<crls.size(); idup++ )
		{
		    if ( crls[idup] == curcrl )
			icrl = idup;
		    else
			break;
		}
		sortPart( crls, vals, valnr, firstdup, icrl, asc );
	    }
	}
    }
}


void BinIDValueSet::sortPart( TypeSet<int>& crls, TypeSet<float>& vals,
			      int valnr, int firstidx, int lastidx, bool asc )
{
    int nridxs = lastidx - firstidx + 1;
    float* vs = new float [ nridxs ];
    int* idxs = new int [ nridxs ];
    for ( int idx=firstidx; idx<=lastidx; idx++ )
    {
	idxs[idx] = idx;
	vs[idx-firstidx] = vals[nrvals_*idx+valnr];
    }
    sort_coupled( vs, idxs, nridxs );

    for ( int idx=0; idx<nridxs; idx++ )
    {
	if ( idxs[idx] != idx )
	{
	    Swap( crls[idx], crls[ idxs[idx] ] );
	    for ( int iv=0; iv<nrvals_; iv++ )
		Swap( vals[ idx*nrvals_ + iv], vals[ idxs[idx]*nrvals_ + iv ] );
	}
    }

    delete [] vs; delete [] idxs;
}


void BinIDValueSet::extend( const BinID& so, const BinID& sos )
{
    if ( (!so.inl && !so.crl) || (!sos.inl && !sos.crl) ) return;

    BinIDValueSet bvs( *this );

    const bool kpdup = allowdup_;
    allowdup_ = false;

    Pos pos; BinID bid;
    float* vals = nrvals_ ? new float [nrvals_] : 0;
    while ( bvs.next(pos) )
    {
	bvs.get( pos, bid, vals );
	const BinID centralbid( bid );
	for ( int iinl=-so.inl; iinl<=so.inl; iinl++ )
	{
	    bid.inl = centralbid.inl + iinl * sos.inl;
	    for ( int icrl=-so.crl; icrl<=so.crl; icrl++ )
	    {
		if ( !iinl && !icrl )
		    continue;
		bid.crl = centralbid.crl + icrl * sos.crl;
		add( bid, vals );
	    }
	}
    }

    delete [] vals;
    allowdup_ = kpdup;
}


void BinIDValueSet::getColumn( int valnr, TypeSet<float>& vals,
				bool incudf ) const
{
    if ( valnr < 0 || valnr >= nrVals() ) return;
    Pos pos;
    while ( next(pos) )
    {
	const float* v = getVals( pos );
	if ( incudf || !mIsUdf(v[valnr]) )
	    vals += v[ valnr ];
    }
}


void BinIDValueSet::removeRange( int valnr, const Interval<float>& rg,
				 bool inside )
{
    if ( valnr < 0 || valnr >= nrVals() ) return;
    TypeSet<Pos> poss; Pos pos;
    while ( next(pos) )
    {
	const float* v = getVals( pos );
	if ( inside == rg.includes(v[valnr]) )
	    poss += pos;
    }
    remove( poss );
}


BinIDValueSet::Pos BinIDValueSet::add( const BinIDValue& biv )
{
    return nrvals_ < 2 ? add(biv.binid,&biv.value)  : add(BinIDValues(biv));
}


BinIDValueSet::Pos BinIDValueSet::add( const BinID& bid, float v )
{
    if ( nrvals_ < 2 )
	return add( bid, nrvals_ == 1 ? &v : 0 );

    BinIDValues bvs( bid, 1 );
    bvs.value(0) = v;
    return add( bvs );
}


BinIDValueSet::Pos BinIDValueSet::add( const BinID& bid, float v1, float v2 )
{
    if ( nrvals_ == 0 )
	return add( bid );
    else if ( nrvals_ < 3 )
    {
	float v[2]; v[0] = v1; v[1] = v2;
	return add( bid, v );
    }

    BinIDValues bvs( bid, 2 );
    bvs.value(0) = v1; bvs.value(1) = v2;
    return add( bvs );
}


BinIDValueSet::Pos BinIDValueSet::add( const BinID& bid,
				       const TypeSet<float>& vals )
{
    if ( vals.isEmpty() )
	return add( bid );
    else if ( vals.size() >= nrvals_ )
	return add( bid, vals.arr() );

    BinIDValues bvs( bid, vals.size() );
    bvs.setVals( vals.arr() );
    return add( bvs );
}


void BinIDValueSet::get( const Pos& pos, BinIDValues& bivs ) const
{
    bivs.setSize( nrvals_ );
    get( pos, bivs.binid, bivs.values() );
}


void BinIDValueSet::get( const Pos& pos, BinIDValue& biv ) const
{
    if ( nrvals_ < 2 )
	get( pos, biv.binid, &biv.value );
    else
    {
	BinIDValues bvs; get( pos, bvs ); biv.binid = bvs.binid;
	biv.value = bvs.value(0);
    }
}


void BinIDValueSet::get( const BinIDValueSet::Pos& pos,
			 BinID& bid, float& v ) const
{
    if ( nrvals_ < 2 )
	get( pos, bid, &v );
    else
    {
	BinIDValues bvs; get( pos, bvs ); bid = bvs.binid;
	v = bvs.value(0);
    }
}


void BinIDValueSet::get( const BinIDValueSet::Pos& pos, BinID& bid,
			 float& v1, float& v2 ) const
{
    if ( nrvals_ < 3 )
    {
	float v[2]; get( pos, bid, v );
	v1 = v[0]; v2 = v[1];
    }
    else
    {
	BinIDValues bvs; get( pos, bvs ); bid = bvs.binid;
	v1 = bvs.value(0); v2 = bvs.value(1);
    }
}


void BinIDValueSet::get( const BinIDValueSet::Pos& pos, BinID& bid,
			 TypeSet<float>& vals, int maxnrvals ) const
{
    if ( maxnrvals < 0 || maxnrvals > nrvals_ ) maxnrvals = nrvals_;

    if ( vals.size() != maxnrvals )
    {
	vals.erase();
	for ( int idx=0; idx<maxnrvals; idx++ )
	    vals += mUdf(float);
    }
    get( pos, bid, vals.arr(), maxnrvals );
}


void BinIDValueSet::set( const BinIDValueSet::Pos& pos, float v )
{
    if ( nrvals_ < 1 ) return;

    if ( nrvals_ == 1 )
	set( pos, &v );
    else
    {
	TypeSet<float> vals( nrvals_, mUdf(float) );
	vals[0] = v; set( pos, vals );
    }
}


void BinIDValueSet::set( const BinIDValueSet::Pos& pos, float v1, float v2 )
{
    if ( nrvals_ < 1 ) return;

    if ( nrvals_ == 2 )
    {
	float v[2]; v[0] = v1; v[1] = v2;
	set( pos, v );
    }
    else
    {
	TypeSet<float> vals( nrvals_, mUdf(float) );
	vals[0] = v1; if ( nrvals_ > 1 ) vals[1] = v2; set( pos, vals );
    }
}


void BinIDValueSet::set( const BinIDValueSet::Pos& pos, const TypeSet<float>& v)
{
    if ( nrvals_ < 1 ) return;

    if ( nrvals_ <= v.size() )
	set( pos, v.arr() );
    else
    {
	TypeSet<float> vals( nrvals_, mUdf(float) );
	for ( int idx=0; idx<v.size(); idx++ )
	    vals[idx] = v[idx];

	set( pos, vals.arr() );
    }
}


void BinIDValueSet::fillPar( IOPar& iop, const char* ky ) const
{
    FileMultiString fms;
    fms += nrvals_; fms += allowdup_ ? "D" : "N";
    BufferString key; if ( ky && *ky ) { key = ky; key += ".Setup"; }
    iop.set( key, fms );

    for ( int iinl=0; iinl<inls_.size(); iinl++ )
    {
	fms = ""; fms += inls_[iinl];
	const TypeSet<int>& crls = getCrlSet(iinl);
	const TypeSet<float>& vals = getValSet(iinl);
	for ( int icrl=0; icrl<crls.size(); icrl++ )
	{
	    fms += crls[icrl];
	    if ( nrvals_ )
	    {
		const float* v = vals.arr() + icrl*nrvals_;
		for ( int idx=0; idx<nrvals_; idx++ )
		    fms += getStringFromFloat(0,v[idx]);
	    }
	}
	if ( ky && *ky )
	    { key = ky; key += "."; }
	else
	    key = "";
	key += iinl;
	iop.set( key, fms );
    }
}


void BinIDValueSet::usePar( const IOPar& iop, const char* ky )
{
    BinIDValues bivs( 0, 0, nrvals_ );
    FileMultiString fms;
    BufferString key; if ( ky && *ky ) { key = ky; key += ".Setup"; }
    const char* res = iop.find( key );
    if ( res && *res )
    {
	empty();
	fms = res;
	setNrVals( atoi(fms[0]), false );
	allowdup_ = *fms[1] == 'D';
    }

    for ( int iinl=0; ; iinl++ )
    {
	if ( ky && *ky )
	    { key = ky; key += "."; }
	else
	    key = "";
	key += iinl;
	res = iop.find( key );
	if ( !res ) return;
	if ( !*res ) continue;

	fms = res;
	bivs.binid.inl = atoi( fms[0] );
	int nrpos = (fms.size() - 1) / (nrvals_ + 1);
	for ( int icrl=0; icrl<nrpos; icrl++ )
	{
	    int fmsidx = 1 + icrl * (nrvals_ + 1);
	    bivs.binid.crl = atoi( fms[fmsidx] );
	    fmsidx++;
	    for ( int ival=0; ival<nrvals_; ival++ )
		bivs.value(ival) = atof( fms[fmsidx+ival] );
	    add( bivs );
	}
    }
}


bool BinIDValueSet::areBinidValuesThere( const BinIDValues& bidvals ) const
{
    Pos pos = findFirst( bidvals.binid );
    bool found = false;
    BinID tmpbid;
    while ( !found && pos.valid() )
    {
	if ( getBinID(pos) != bidvals.binid )
	    break;
	
	TypeSet<float> valofset;
	get( pos, tmpbid, valofset );
	if ( valofset.size() == bidvals.size() )
	{
	    bool diff = false;
	    for ( int idx=0; idx<valofset.size(); idx++ )
	    {
		if ( bidvals.value(idx) != valofset[idx] )
		    diff = true;
	    }
	    found = !diff;
	}
	next( pos );
    }
    
    return found;
}



