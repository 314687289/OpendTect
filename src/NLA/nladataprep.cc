/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : May 2005
-*/
 
static const char* rcsID = "$Id: nladataprep.cc,v 1.6 2006-12-28 21:10:33 cvsnanne Exp $";

#include "nladataprep.h"
#include "binidvalset.h"
#include "statrand.h"


void NLADataPreparer::limitRange( const Interval<float>& r )
{
    Interval<float> rg( r ); rg.sort(true);
    const float ext = rg.width() * mDefEps;
    rg.widen( ext, false );
    bvs_.removeRange( targetcol_, rg, false );
}


void NLADataPreparer::removeUndefs( bool targetonly )
{
    TypeSet<BinIDValueSet::Pos> poss;
    BinIDValueSet::Pos pos;
    while ( bvs_.next(pos) )
    {
	float* vals = bvs_.getVals( pos );
	if ( targetonly && mIsUdf(vals[targetcol_]) )
	    poss += pos;
	else
	{
	    for ( int idx=0; idx<bvs_.nrVals(); idx++ )
	    {
		if ( mIsUdf(vals[idx]) )
		    { poss += pos; break; }
	    }
	}
    }
    bvs_.remove( poss );
}


void NLADataPreparer::balance( const NLADataPreparer::BalanceSetup& setup )
{
    ObjectSet<BinIDValueSet> bvss;
    for ( int idx=0; idx<setup.nrclasses; idx++ )
    {
	BinIDValueSet* newbvs = new BinIDValueSet( 0, true );
	newbvs->copyStructureFrom( bvs_ );
	bvss += newbvs;
    }

    const int nrvals = bvs_.nrVals();
    ArrPtrMan< Interval<float> > rgs = new Interval<float> [nrvals];
    BinIDValueSet::Pos pos; BinID bid;
    bool first_pos = true;
    while ( bvs_.next(pos) )
    {
	const float* vals = bvs_.getVals( pos );
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    if ( first_pos )
		rgs[idx].start = rgs[idx].stop = vals[idx];
	    else
		rgs[idx].include( vals[idx] );
	}
	first_pos = false;
    }

    const Interval<float> targetrg( rgs[targetcol_] );
    pos = BinIDValueSet::Pos();
    const float targetrgwdth = targetrg.width();
    while ( bvs_.next(pos) )
    {
	const float* vals = bvs_.getVals( pos );
	float val = vals[targetcol_];
	if ( mIsUdf(val) ) continue;

	float relpos = (val-targetrg.start) / targetrgwdth;
	relpos *= setup.nrclasses; relpos -= 0.5;
	int clss = mNINT( relpos );
	if ( clss < 0 )			clss = 0;
	if ( clss >= setup.nrclasses )	clss = setup.nrclasses-1;
	bvs_.get( pos, bid );
	bvss[clss]->add( bid, vals );
    }

    Stats::RandGen::init();
    bvs_.empty();
    for ( int idx=0; idx<setup.nrclasses; idx++ )
    {
	BinIDValueSet& bvs = *bvss[idx];
	const int totsz = bvs.totalSize();
	if ( totsz < setup.nrptsperclss )
	    addVecs( bvs, setup.nrptsperclss - totsz, setup.noiselvl, rgs );
	else
	    removeVecs( bvs, totsz - setup.nrptsperclss );
	bvs_.append( bvs );
    }
    deepErase( bvss );
}


void NLADataPreparer::addVecs( BinIDValueSet& bvs, int nr, float noiselvl,
				const Interval<float>* rgs )
{
    const int orgsz = bvs.totalSize();
    if ( orgsz == 0 ) return;

    bvs.allowDuplicateBids( true );
    BinIDValueSet bvsnew( bvs.nrVals(), true );
    BinID bid;
    const int nrvals = bvs.nrVals();
    const bool nonoise = noiselvl < 1e-6 || noiselvl > 1 + 1e-6;
    for ( int idx=0; idx<nr; idx++ )
    {
	const int dupidx = Stats::RandGen::getIndex( orgsz );
	BinIDValueSet::Pos pos = bvs.getPos( dupidx );
	const float* vals = bvs.getVals( pos );
	bvs.get( pos, bid );
	if ( nonoise )
	    bvsnew.add( bid, vals );
	else
	{
	    ArrPtrMan<float> newvals = new float [nrvals];
	    for ( int validx=0; validx<nrvals; validx++ )
	    {
		float wdth = rgs[validx].stop - rgs[validx].start;
		wdth *= noiselvl;
		newvals[validx] = vals[validx] +
		    		  ((Stats::RandGen::get()-.5) * wdth);
	    }
	    bvsnew.add( bid, newvals );
	}
    }
    bvs.append( bvsnew );
}


void NLADataPreparer::removeVecs( BinIDValueSet& bvs, int nr )
{
    if ( nr == 0 ) return;

    TypeSet<BinIDValueSet::Pos> poss;
    const int orgsz = bvs.totalSize();
    BinID bid;

    TypeSet<int> rmidxs;
    while ( rmidxs.size() < nr )
    {
	int rmidx = Stats::RandGen::getIndex( orgsz );
	if ( rmidxs.indexOf(rmidx) < 0 )
	{
	    rmidxs += rmidx;
	    poss += bvs.getPos( rmidx );
	}
    }
    bvs.remove( poss );
}
