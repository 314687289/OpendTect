/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
-*/

static const char* rcsID = "$Id: seisbuf.cc,v 1.23 2005-04-26 18:57:15 cvskris Exp $";

#include "seisbuf.h"
#include "seisinfo.h"
#include "seistrcsel.h"
#include "seiswrite.h"
#include "xfuncimpl.h"
#include "ptrman.h"
#include "sorting.h"


void SeisTrcBuf::insert( SeisTrc* t, int insidx )
{
    for ( int idx=insidx; idx<trcs.size(); idx++ )
	t = trcs.replace( t, idx );
    trcs += t;
}


void SeisTrcBuf::copyInto( SeisTrcBuf& buf ) const
{
    for ( int idx=0; idx<trcs.size(); idx++ )
    {
	SeisTrc* trc = trcs[idx];
	buf.add( buf.owner_ ? new SeisTrc(*trc) : trc );
    }
}


void SeisTrcBuf::fill( SeisPacketInfo& spi ) const
{
    const int sz = size();
    if ( sz < 1 ) return;
    const SeisTrc* trc = get( 0 );
    BinID bid = trc->info().binid;
    spi.inlrg.start = bid.inl; spi.inlrg.stop = bid.inl;
    spi.crlrg.start = bid.crl; spi.crlrg.stop = bid.crl;
    spi.zrg.start = trc->info().sampling.start;
    spi.zrg.step = trc->info().sampling.step;
    spi.zrg.stop = spi.zrg.start + spi.zrg.step * (trc->size() - 1);
    if ( sz < 2 ) return;

    bool doneinl = false, donecrl = false;
    const BinID pbid = bid;
    for ( int idx=1; idx<sz; idx++ )
    {
	trc = get( idx ); bid = trc->info().binid;
	spi.inlrg.include( bid.inl ); spi.crlrg.include( bid.crl );
	if ( !doneinl && bid.inl != pbid.inl )
	    { spi.inlrg.step = bid.inl - pbid.inl; doneinl = true; }
	if ( !donecrl && bid.crl != pbid.crl )
	    { spi.crlrg.step = bid.crl - pbid.crl; donecrl = true; }
    }
    if ( spi.inlrg.step < 0 ) spi.inlrg.step = -spi.inlrg.step;
    if ( spi.crlrg.step < 0 ) spi.crlrg.step = -spi.crlrg.step;
}


void SeisTrcBuf::add( SeisTrcBuf& tb )
{
    for ( int idx=0; idx<tb.size(); idx++ )
    {
	SeisTrc* trc = tb.get( idx );
	add( owner_ && trc ? new SeisTrc(*trc) : trc );
    }
}


void SeisTrcBuf::stealTracesFrom( SeisTrcBuf& tb )
{
    for ( int idx=0; idx<tb.size(); idx++ )
    {
	SeisTrc* trc = tb.get( idx );
	add( trc );
    }
    tb.trcs.erase();
}


bool SeisTrcBuf::isSorted( bool ascending, int seisinf_attrnr ) const
{
    const int sz = size();
    if ( sz < 2 ) return true;

    float prevval = get(0)->info().getAttr(seisinf_attrnr);
    for ( int idx=1; idx<sz; idx++ )
    {
	float val = get(idx)->info().getAttr(seisinf_attrnr);
	float diff = val - prevval;
	if ( !mIsZero(diff,mDefEps) )
	{
	    if ( (ascending && diff < 0) || (!ascending && diff > 0) )
		return false;
	}
	prevval = val;
    }

    return true;
}


void SeisTrcBuf::sort( bool ascending, int seisinf_attrnr )
{
    const int sz = size();
    if ( sz < 2 || isSorted(ascending,seisinf_attrnr) )
	return;

    ArrPtrMan<int> idxs = new int [sz];
    ArrPtrMan<float> vals = new float [sz];
    for ( int idx=0; idx<sz; idx++ )
    {
	idxs[idx] = idx;
	vals[idx] = get(idx)->info().getAttr(seisinf_attrnr);
    }
    sort_coupled( (float*)vals, (int*)idxs, sz );
    ObjectSet<SeisTrc> tmp;
    for ( int idx=0; idx<sz; idx++ )
	tmp += get( idxs[idx] );

    erase();
    for ( int idx=0; idx<sz; idx++ )
	add( tmp[ascending ? idx : sz - idx - 1] );
}


void SeisTrcBuf::enforceNrTrcs( int nrrequired, int seisinf_attrnr )
{
    SeisTrc* prevtrc = get(0);
    if ( !prevtrc ) return;

    float prevval = prevtrc->info().getAttr(seisinf_attrnr);
    int nrwithprevval = 1;
    for ( int idx=1; idx<=size(); idx++ )
    {
	SeisTrc* trc = idx==size() ? 0 : get(idx);
	float val = trc ? trc->info().getAttr(seisinf_attrnr) : 0;

	if ( trc && mIsEqual(prevval,val,mDefEps) )
	{
	    nrwithprevval++;
	    if ( nrwithprevval > nrrequired )
	    {
		remove(trc); idx--; delete trc;
		nrwithprevval = nrrequired;
	    }
	}
	else
	{
	    for ( int inew=nrwithprevval; inew<nrrequired; inew++ )
	    {
		SeisTrc* newtrc = new SeisTrc(*prevtrc);
		newtrc->data().zero();
		add( newtrc );
		idx++;
	    }
	    nrwithprevval = 1;
	}

	prevval = val;
    }
}


void SeisTrcBuf::transferData( FloatList& fl, int takeeach, int icomp ) const
{
    for ( int idx=0; idx<size(); idx+=takeeach )
    {
	const SeisTrc& trc = *get( idx );
	const int trcsz = trc.size();
	for ( int isamp=0; isamp<trcsz; isamp++ )
	    fl += trc.get( isamp, icomp );
    }
}


void SeisTrcBuf::revert()
{
    int sz = trcs.size();
    int hsz = sz / 2;
    for ( int idx=0; idx<hsz; idx++ )
	trcs.replace( trcs.replace(trcs[sz-idx-1],idx), sz-idx-1 );
}


int SeisTrcBuf::find( const BinID& binid, bool is2d ) const
{
    int sz = size();
    int startidx = probableIdx( binid, is2d );
    int idx = startidx, pos = 0;
    while ( idx<sz && idx>=0 )
    {
	if ( !is2d && ((SeisTrcBuf*)this)->get(idx)->info().binid == binid )
	    return idx;
	else if ( is2d && ((SeisTrcBuf*)this)->get(idx)->info().nr == binid.crl)
	    return idx;
	if ( pos < 0 ) pos = -pos;
	else	       pos = -pos-1;
	idx = startidx + pos;
	if ( idx < 0 ) { pos = -pos; idx = startidx + pos; }
	else if ( idx >= sz ) { pos = -pos-1; idx = startidx + pos; }
    }
    return -1;
}


int SeisTrcBuf::find( const SeisTrc* trc, bool is2d ) const
{
    if ( !trc ) return -1;

    int tryidx = probableIdx( trc->info().binid, is2d );
    if ( trcs[tryidx] == trc ) return tryidx;

    // Bugger. brute force then
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( ((SeisTrcBuf*)this)->get(idx) == trc )
	    return idx;
    }

    return -1;
}


int SeisTrcBuf::probableIdx( const BinID& bid, bool is2d ) const
{
    int sz = size(); if ( sz < 2 ) return 0;
    BinID start = trcs[0]->info().binid;
    BinID stop = trcs[sz-1]->info().binid;
    if ( is2d )
    {
	start.inl = stop.inl = 0;
	start.crl = trcs[0]->info().nr;
	stop.crl = trcs[sz-1]->info().nr;
    }

    BinID dist( start.inl - stop.inl, start.crl - stop.crl );
    if ( !dist.inl && !dist.crl )
	return 0;

    int n1  = dist.inl ? start.inl : start.crl;
    int n2  = dist.inl ? stop.inl  : stop.crl;
    int pos = dist.inl ? bid.inl   : bid.crl;
 
    float fidx = ((sz-1.) * (pos - n1)) / (n2-n1);
    int idx = mNINT(fidx);
    if ( idx < 0 ) idx = 0;
    if ( idx >= sz ) idx = sz-1;
    return idx;
}


SeisTrcBufWriter::SeisTrcBufWriter( const SeisTrcBuf& b, SeisTrcWriter& w )
	: Executor("Trace storage")
	, trcbuf(b)
	, writer(w)
	, nrdone(0)
{
    if ( trcbuf.size() )
	writer.prepareWork( *trcbuf.get(0) );
}


const char* SeisTrcBufWriter::message() const
{
    return "Writing traces";
}
const char* SeisTrcBufWriter::nrDoneText() const
{
    return "Traces written";
}
int SeisTrcBufWriter::totalNr() const
{
    return trcbuf.size();
}
int SeisTrcBufWriter::nrDone() const
{
    return nrdone;
}


int SeisTrcBufWriter::nextStep()
{
    Interval<int> nrs( nrdone, nrdone+9 );
    if ( nrs.start >= trcbuf.size() ) return 0;
    if ( nrs.stop >= trcbuf.size() ) nrs.stop = trcbuf.size() - 1;
    nrdone = nrs.stop + 1;
    
    for ( int idx=nrs.start; idx<=nrs.stop; idx++ )
	writer.put( *trcbuf.get(idx) );

    return nrdone >= trcbuf.size() ? 0 : 1;
}
