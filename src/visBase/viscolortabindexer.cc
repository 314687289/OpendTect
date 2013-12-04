/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2005
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "viscolortabindexer.h"

#include "thread.h"
#include "valseries.h"
#include "varlenarray.h"
#include "viscolortab.h"


#define mUndefColIdx	255

namespace visBase
{

ColorTabIndexer::ColorTabIndexer( const ValueSeries<float>& inp,
				  unsigned char* outp, int nsz,
				  const VisColorTab* ct )
    : colortab_( ct )
    , indexcache_( outp )
    , sz_( nsz )
    , datacache_( &inp )
    , datacacheptr_( inp.arr() )
    , histogrammutex_( *new Threads::Mutex )
    , globalhistogram_( 0 )
    , nrhistogramsteps_( ct->nrSteps() )
{
    histogrammutex_.lock();

    globalhistogram_ = new unsigned int[nrhistogramsteps_];

    OD::memZero( globalhistogram_, sizeof(int)*nrhistogramsteps_ );
    histogrammutex_.unLock();
}


ColorTabIndexer::ColorTabIndexer( const float* inp,
				  unsigned char* outp, int nsz,
				  const VisColorTab* ct )
    : colortab_( ct )
    , indexcache_( outp )
    , sz_( nsz )
    , datacache_( 0 )
    , datacacheptr_( inp )
    , histogrammutex_( *new Threads::Mutex )
    , globalhistogram_( 0 )
    , nrhistogramsteps_( ct->nrSteps() )
{
    histogrammutex_.lock();

    globalhistogram_ = new unsigned int[nrhistogramsteps_];

    OD::memZero( globalhistogram_, sizeof(int)*nrhistogramsteps_ );
    histogrammutex_.unLock();
}


ColorTabIndexer::~ColorTabIndexer()
{
    delete &histogrammutex_;
    delete [] globalhistogram_;
}


const unsigned int* ColorTabIndexer::getHistogram() const
{ return globalhistogram_; }


int ColorTabIndexer::nrHistogramSteps() const
{ return nrhistogramsteps_; }


od_int64 ColorTabIndexer::nrIterations() const
{ return sz_; }



bool ColorTabIndexer::doWork( od_int64 start, od_int64 stop, int threadid )
{
    mAllocVarLenArr( unsigned int, histogram, nrhistogramsteps_ );
    OD::memZero( histogram, sizeof(int)*nrhistogramsteps_ );

    if ( datacacheptr_ )
    {
	for ( od_int64 idx=start; idx<=stop; idx++, addToNrDone(1) )
	{
	    int colorindex = colortab_->colIndex( datacacheptr_[idx] );
	    if ( colorindex < 0 ) colorindex = 0;
	    indexcache_[idx] = (unsigned char) colorindex;
	    if ( colorindex<nrhistogramsteps_ )
		histogram[colorindex]++;
	}
    }
    else
    {
	for ( od_int64 idx=start; idx<=stop; idx++ )
	{
	    int colorindex = colortab_->colIndex(datacache_->value(idx));
	    if ( colorindex < 0 ) colorindex = 0;
	    indexcache_[idx] = (unsigned char) colorindex;
	    if ( colorindex<nrhistogramsteps_ )
	        histogram[colorindex]++;
	}
    }

    histogrammutex_.lock();
    for ( int idx=nrhistogramsteps_-1; idx>=0; idx-- )
	globalhistogram_[idx] += histogram[idx];
    histogrammutex_.unLock();

    return true;
}



}; // namespace visBase
