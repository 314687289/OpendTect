/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : 9-3-1999
-*/

static const char* rcsID = "$Id: arrayndinfo.cc,v 1.9 2007-04-23 10:48:48 cvshelene Exp $";

#include <arrayndinfo.h>


bool ArrayNDInfo::setSize(int dim, int sz)
{ return false; }


uint64 ArrayNDInfo::getTotalSz() const
{ return calcTotalSz(); }


uint64 ArrayNDInfo::getOffset( const int* pos ) const
{
    const int ndim = getNDim();
    uint64 unitsize = 1;
    uint64 res = 0;

    for ( int idx=ndim-1; idx>=0; idx-- )
    {
	res += unitsize*pos[idx];
	unitsize *= getSize(idx);
    }

    return res;
}


bool ArrayNDInfo::validPos( const int* pos ) const
{
    const int ndim = getNDim();
    for ( int idx=ndim-1; idx>=0; idx-- )
    {
	if ( pos[idx]<0 || pos[idx]>=getSize(idx) )
	    return false;
    }

    return true;
}


void ArrayNDInfo::getArrayPos( uint64 mempos, int* pos ) const
{
    const int ndim = getNDim();
    for ( int idx=0; idx<ndim; idx++ )
    {
	int tsz = 1;
	for ( int idy=idx+1; idy<ndim; idy++ )
	    tsz *= getSize(idy);

	pos[idx] = mempos/tsz;
	mempos = mempos%tsz;
    }
}
	

uint64 ArrayNDInfo::calcTotalSz() const
{
    const int ndim = getNDim();
    uint64 res = 1;

    for ( int idx=0; idx<ndim; idx++ )
	res *= getSize( idx );

    return res;
}


uint64 Array1DInfo::getOffset( int pos ) const
{ return pos; }


bool Array1DInfo::validPos( int pos ) const
{ return ArrayNDInfo::validPos( &pos ); }


uint64 Array2DInfo::getOffset( int p0, int p1 ) const
{
    const int pos[2] = { p0, p1 };
    return ArrayNDInfo::getOffset( pos );
}


bool Array2DInfo::validPos( int p0, int p1 ) const
{ 
    const int pos[2] = { p0, p1 };
    return ArrayNDInfo::validPos( pos );
}


uint64 Array3DInfo::getOffset( int p0, int p1, int p2 ) const
{
    const int pos[3] = { p0, p1, p2 };
    return ArrayNDInfo::getOffset( pos );
}


bool Array3DInfo::validPos( int p0, int p1, int p2 ) const
{
    const int pos[3] = { p0, p1, p2 };
    return ArrayNDInfo::validPos( pos );
}


Array1DInfoImpl::Array1DInfoImpl( int nsz ) 
	: sz_( nsz ) 
{ }


Array1DInfoImpl::Array1DInfoImpl( const Array1DInfo& nsz)
	: sz_( nsz.getSize(0) )
{ } 


bool Array1DInfoImpl::setSize( int dim, int nsz )
{
    if( dim != 0 ) return false;
    sz_ = nsz;
    return true;
}


Array2DInfoImpl::Array2DInfoImpl( int sz0, int sz1 )
{
    sz0_ = sz0; sz1_ = sz1;
    cachedtotalsz_ = calcTotalSz();
}


Array2DInfoImpl::Array2DInfoImpl( const Array2DInfo& nsz)
{
    sz0_ = nsz.getSize(0);
    sz1_ = nsz.getSize(1);

    cachedtotalsz_ = calcTotalSz(); 
}


bool Array2DInfoImpl::setSize( int dim, int nsz )
{
    if( dim > 1 || dim < 0 ) return false;
    if ( dim == 0 ) sz0_ = nsz;
    else sz1_ = nsz;
    cachedtotalsz_ = calcTotalSz();	
    return true;
}


Array3DInfoImpl::Array3DInfoImpl( int sz0, int sz1, int sz2) 
{ 
    sz0_ = sz0; sz1_ = sz1; sz2_ = sz2;
    cachedtotalsz_ = calcTotalSz();
}


Array3DInfoImpl::Array3DInfoImpl( const Array3DInfo& nsz)
{
    sz0_ = nsz.getSize(0);
    sz1_ = nsz.getSize(1);
    sz2_ = nsz.getSize(2);

    cachedtotalsz_ = calcTotalSz();
}


bool Array3DInfoImpl::setSize(int dim, int nsz)
{
    if( dim > 2 || dim < 0 ) return false;
    if ( dim == 0 ) sz0_ = nsz;
    else if ( dim == 1) sz1_ = nsz;
    else sz2_ = nsz;
    cachedtotalsz_ = calcTotalSz(); 
    return true;
}


ArrayNDInfo* ArrayNDInfoImpl::clone() const
{
    if ( ndim==1 ) return new Array1DInfoImpl(sizes[0]);
    if ( ndim==2 ) return new Array2DInfoImpl(sizes[0], sizes[1]);
    if ( ndim==3 ) return new Array3DInfoImpl(sizes[0], sizes[1], sizes[2]);

    return new ArrayNDInfoImpl(*this); 
}


ArrayNDInfo* ArrayNDInfoImpl::create( int ndim )
{
    if ( ndim==1 ) return new Array1DInfoImpl;
    if ( ndim==2 ) return new Array2DInfoImpl;
    if ( ndim==3 ) return new Array3DInfoImpl;

    return new ArrayNDInfoImpl(ndim); 
}


ArrayNDInfoImpl::ArrayNDInfoImpl( int ndim_ )
	: sizes( new int[ndim_] )  
	, ndim( ndim_ )
{
    cachedtotalsz_ = 0;
    for ( int idx = 0; idx < ndim; idx++ )
	sizes[idx] = 0;
}


ArrayNDInfoImpl::ArrayNDInfoImpl( const ArrayNDInfoImpl& nsz )
	: sizes(new int[nsz.getNDim()]) 
	, ndim( nsz.getNDim() )
{
    for (int idx = 0; idx < ndim; idx++)
	setSize( idx, nsz.getSize(idx) ); 
}


ArrayNDInfoImpl::ArrayNDInfoImpl( const ArrayNDInfo& nsz )
	: sizes(new int[nsz.getNDim()]) 
	, ndim( nsz.getNDim() )
{
    for (int idx = 0; idx < ndim; idx++)
	setSize( idx, nsz.getSize(idx) ); 
}

ArrayNDInfoImpl::~ArrayNDInfoImpl()
{
    delete [] sizes;
}


int ArrayNDInfoImpl::getNDim() const
{
    return ndim;
}


bool ArrayNDInfoImpl::setSize( int dim, int newSz )
{
    if ( dim >= ndim || dim < 0 ) return false;

    sizes[dim] = newSz;
    cachedtotalsz_ = calcTotalSz();
    return true;
}


int ArrayNDInfoImpl::getSize( int dim ) const
{
    return dim >= ndim || dim < 0 ? 0 : sizes[dim];
}


ArrayNDIter::ArrayNDIter( const ArrayNDInfo& sz )
    : sz_ ( sz )
    , position_( new int[sz.getNDim()] )
{
    reset();
}


ArrayNDIter::~ArrayNDIter()
{
    delete [] position_;
}


bool ArrayNDIter::next()
{
    return inc( sz_.getNDim() - 1 );
}


void ArrayNDIter::reset()
{
    const int ndim = sz_.getNDim();

    for ( int idx=0; idx<ndim; idx++ )
	position_[idx] = 0;
}


int ArrayNDIter::operator[](int dim) const
{
    return position_[dim];
}


bool ArrayNDIter::inc( int dim )
{
    position_[dim] ++;

    if ( position_[dim] >= sz_.getSize(dim))
    {
	if ( dim )
	{
	    position_[dim] = 0;
	    return inc( dim-1 );
	}
	else
	    return false;
    }

    return true;
}
