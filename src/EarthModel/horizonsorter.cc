/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2006
 RCS:           $Id: horizonsorter.cc,v 1.6 2006-05-10 21:26:48 cvskris Exp $
________________________________________________________________________

-*/

#include "horizonsorter.h"

#include "arrayndimpl.h"
#include "cubesampling.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "ptrman.h"
#include "survinfo.h"


HorizonSorter::HorizonSorter( const TypeSet<MultiID>& ids )
    : Executor("Sort horizons")
    , unsortedids_(ids)
    , totalnr_( ids.size() * (SI().inlRange(true).width()+1) )
    , nrdone_(0)
    , iterator_(0)
    , result_(0)
{
}


HorizonSorter::~HorizonSorter()
{
    delete result_;
    delete iterator_;
    deepUnRef( horizons_ );
}


void HorizonSorter::init()
{
    calcBoundingBox();
    totalnr_ = hrg_.nrInl();

    delete iterator_;
    iterator_ = new HorSamplingIterator( hrg_ );

    delete result_;
    result_ = new Array3DImpl<int>( horizons_.size(), horizons_.size(), 2 );
    for ( int idx=0; idx<result_->info().getTotalSz(); idx++ )
	result_->getData()[idx] = 0;
}


void HorizonSorter::calcBoundingBox()
{
    for ( int idx=0; idx<horizons_.size(); idx++ )
    {
	StepInterval<int> rrg = horizons_[idx]->geometry().rowRange();
	StepInterval<int> crg = horizons_[idx]->geometry().colRange();
	if ( !idx )
	{
	    hrg_.set( rrg, crg );
	    continue;
	}

	hrg_.include( BinID(rrg.start,crg.start) );
	hrg_.include( BinID(rrg.stop,crg.stop) );
    }
}


void HorizonSorter::sort()
{
    sortedids_ = unsortedids_;
    const int nrhors = unsortedids_.size();
    int nrswaps = 0;
    while ( true )
    {
	nrswaps = 0;
	for ( int idx=0; idx<nrhors; idx++ )
	{
	    for ( int idy=idx+1; idy<nrhors; idy++ )
	    {
		const int nrabove = result_->get( idx, idy, 0 );
		const int nrbelow = result_->get( idx, idy, 1 );
		const int idx0 = sortedids_.indexOf( unsortedids_[idx] );
		const int idx1 = sortedids_.indexOf( unsortedids_[idy] );
		if ( nrbelow > nrabove && idx0 > idx1 ) continue;
		if ( nrbelow < nrabove && idx0 < idx1 ) continue;

		if ( nrbelow > nrabove )
		{
		    MultiID mid = sortedids_[idx0];
		    sortedids_.remove( idx0 );
		    sortedids_.insert( idx1, mid );
		}
		else if ( nrbelow < nrabove )
		{
		    MultiID mid = sortedids_[idx1];
		    sortedids_.remove( idx1 );
		    sortedids_.insert( idx0, mid );
		}
		else
		    continue;

		nrswaps++;
	    }
	}

	if ( nrswaps == 0 )
	    break;
    }
}


void HorizonSorter::getSortedList( TypeSet<MultiID>& ids )
{
    ids = sortedids_;
}


int HorizonSorter::getNrCrossings( const MultiID& mid1,
				   const MultiID& mid2 ) const
{
    const int idx1 = unsortedids_.indexOf( mid1 );
    const int idx2 = unsortedids_.indexOf( mid2 );
    const int nrabove = result_->get( mMIN(idx1,idx2), mMAX(idx1,idx2), 0 );
    const int nrbelow = result_->get( mMIN(idx1,idx2), mMAX(idx1,idx2), 1 );
    return mMIN(nrabove,nrbelow);
}


const char* HorizonSorter::message() const	{ return "Sorting"; }

const char* HorizonSorter::nrDoneText() const	{ return "Positions done"; }

int HorizonSorter::nrDone() const		{ return nrdone_; }

int HorizonSorter::totalNr() const		{ return totalnr_; }

int HorizonSorter::nextStep()
{
    if ( !nrdone_ )
    {
	PtrMan<Executor> horreader = EM::EMM().objectLoader( unsortedids_ );
	horreader->execute();

	for ( int idx=0; idx<unsortedids_.size(); idx++ )
	{
	    EM::ObjectID objid = EM::EMM().getObjectID( unsortedids_[idx] );
	    EM::EMObject* emobj = EM::EMM().getObject( objid );
	    emobj->ref();
	    mDynamicCastGet(EM::Horizon*,horizon,emobj);
	    if ( !horizon )
		emobj->unRef();
	    horizons_ += horizon;
	}

	init();
    }

    if ( !iterator_ ) return Finished;

    const int previnl = binid_.inl;
    while ( binid_.inl==previnl )
    {
	if ( !iterator_->next(binid_) )
	{
	    sort();
	    return Finished;
	}

	const int nrhors = horizons_.size();
	ArrPtrMan<float> depths = new float [nrhors];
	for ( int idx=0; idx<nrhors; idx++ )
	    depths[idx] = horizons_[idx]->getPos( horizons_[idx]->sectionID(0),
						  binid_.getSerialized() ).z;

	for ( int idx=0; idx<nrhors; idx++ )
	{
	    if ( mIsUdf(depths[idx]) ) continue;
	    for ( int idy=idx+1; idy<nrhors; idy++ )
	    {
		if ( mIsUdf(depths[idy]) ) continue;

		const int resultidx = depths[idx] <= depths[idy] ? 0 : 1;
		int val = result_->get( idx, idy, resultidx ); val++;
		result_->set( idx, idy, resultidx, val );
	    }
	}
    }

    nrdone_++;
    return MoreToDo;
}
