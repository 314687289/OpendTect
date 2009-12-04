/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 28-2-1996
 * FUNCTION : Seismic trace functions
-*/

static const char* rcsID = "$Id: seistrc.cc,v 1.45 2009-12-04 20:11:09 cvskris Exp $";

#include "seistrc.h"
#include "simpnumer.h"
#include "socket.h"
#include "iopar.h"
#include "errh.h"
#include "valseriesinterpol.h"
#include <math.h>
#include <float.h>

const float SeisTrc::snapdist = 1e-3;


SeisTrc::~SeisTrc()
{
    delete intpol_;
}


SeisTrc& SeisTrc::operator =( const SeisTrc& trc )
{
    if ( &trc == this ) return *this;

    while ( nrComponents() > trc.nrComponents() )
	data_.delComponent( 0 );

    info_ = trc.info_;
    copyDataFrom( trc, -1, false );

    delete intpol_; intpol_ = 0;
    if ( trc.intpol_ )
	intpol_ = new ValueSeriesInterpolator<float>( *trc.intpol_ );

    return *this;
}


bool SeisTrc::reSize( int sz, bool copydata )
{
    for ( int idx=0; idx<nrComponents(); idx++ )
	data_.reSize( sz, idx, copydata );
    return data_.allOk();
}


const ValueSeriesInterpolator<float>& SeisTrc::interpolator() const
{
    static ValueSeriesInterpolator<float>* defintpol = 0;
    if ( !defintpol )
    {
	defintpol = new ValueSeriesInterpolator<float>();
	defintpol->snapdist_ = snapdist;
	defintpol->smooth_ = true;
	defintpol->extrapol_ = false;
	defintpol->udfval_ = 0;
    }
    ValueSeriesInterpolator<float>& ret
	= const_cast<ValueSeriesInterpolator<float>&>(
	    				intpol_ ? *intpol_ : *defintpol );
    ret.maxidx_ = size() - 1;
    return ret;
}


void SeisTrc::setInterpolator( ValueSeriesInterpolator<float>* intpol )
{
    delete intpol_; intpol_ = intpol;
}


bool SeisTrc::isNull( int icomp ) const
{
    if ( icomp >= 0 )
	return data_.isZero(icomp);

    for ( icomp=0; icomp<nrComponents(); icomp++ )
    {
	if ( !data_.isZero(icomp) )
	    return false;
    }
    return true;
}


float SeisTrc::getValue( float t, int icomp ) const
{
    const int sz = size();
    int sampidx = nearestSample( t );
    if ( sampidx < 0 || sampidx >= sz )
	return interpolator().udfval_;

    const float pos = ( t - startPos() ) / info_.sampling.step;
    if ( sampidx-pos > -snapdist && sampidx-pos < snapdist )
	return get( sampidx, icomp );

    return interpolator().value( SeisTrcValueSeries(*this,icomp), pos );
}


SampleGate SeisTrc::sampleGate( const Interval<float>& tg, bool check ) const
{
    SampleGate sg( info_.sampleGate(tg) );
    if ( !check ) return sg;

    const int maxsz = size() - 1;
    if ( sg.start > maxsz ) sg.start = maxsz;
    if ( sg.stop > maxsz ) sg.stop = maxsz;
    return sg;
}


SeisTrc* SeisTrc::getRelTrc( const ZGate& zgate, float sr ) const
{
    const float pick = info_.pick;
    if ( mIsUdf(pick) )
	const_cast<float&>(pick) = info_.refpos;
    if ( mIsUdf(pick) )
	return 0;

    ZGate zg( zgate ); zg.sort();
    SeisTrc* ret = new SeisTrc;
    ret->info_ = info_;
    ret->info_.sampling.start = zg.start;
    if ( mIsUdf(sr) ) sr = info_.sampling.step;
    ret->info_.sampling.step = sr;
    ret->info_.pick = 0;

    const int nrsamps = (int)( (zg.stop - zg.start) / sr + 1.5);
    ret->reSize( nrsamps, false );

    while ( ret->nrComponents() < nrComponents() )
	ret->data_.addComponent( ret->size(),
		data_.getInterpreter( ret->nrComponents() )->dataChar() );

    for ( int idx=0; idx<nrsamps; idx++ )
    {
	const float curt = pick + zg.start + sr * idx;
	for ( int icomp=0; icomp<nrComponents(); icomp++ )
	    ret->set( idx, getValue(curt,icomp), icomp );
    }

    return ret;
}


SeisTrc* SeisTrc::getExtendedTo( const ZGate& zgate, bool usevals ) const
{
    const float fnrsamps = (zgate.stop-zgate.start) / info_.sampling.step + 1;
    const int outnrsamps = mNINT( fnrsamps );
    const TraceDataInterpreter* tdi = data_.getInterpreter(0);
    DataCharacteristics dc( tdi ? tdi->dataChar() : DataCharacteristics() );
    SeisTrc* newtrc = new SeisTrc( outnrsamps, dc );
    while ( newtrc->nrComponents() < nrComponents() )
	newtrc->data_.addComponent( newtrc->size(),
		data_.getInterpreter( newtrc->nrComponents() )->dataChar() );
    if ( size() < 1 )
	{ newtrc->zero(); return newtrc; }

    newtrc->info_ = info_;
    newtrc->info_.sampling.start = zgate.start;
    const float z0 = info_.sampling.start - snapdist * info_.sampling.step;
    const float z1 = samplePos( size() - 1 ) + snapdist * info_.sampling.step;

    for ( int icomp=0; icomp<nrComponents(); icomp++ )
    {
	const float preval = usevals ? get(0,icomp) : 0;
	const float postval = usevals ? get(size()-1,icomp) : 0;
	for ( int isamp=0; isamp<newtrc->size(); isamp++ )
	{
	    const float z = newtrc->samplePos( isamp );
	    const float val = (z < z0 ? preval : (z > z1 ? postval
			    : getValue( z, icomp ) ) );
	    newtrc->set( isamp, val, icomp );
	}
    }

    return newtrc;
}


bool SeisTrc::isWriteReady() const
{
    const float nsr = info_.sampling.start / info_.sampling.step;
    const float intnsr = (float)( mNINT(nsr) );
    return mIsEqual( nsr, intnsr, mDefEps );
}


void SeisTrc::getWriteReady( SeisTrc& trc ) const
{
    trc = *this;
    if ( isWriteReady() )
	return;

    const int sz = size();
    const int nrcomps = nrComponents();
    const int nsr = mNINT( info_.sampling.start / info_.sampling.step );
    trc.info_.sampling.start = nsr * info_.sampling.step;
    if ( sz < 2 )
    {
	if ( sz == 1 )
	{
	    for ( int icomp=0; icomp<nrcomps; icomp++ )
		trc.set( 0, get(0,icomp), icomp );
	    return;
	}
    }

#define mGetVal(isamp,icomp) getValue(trc.info_.sampling.atIndex(isamp),icomp)

    for ( int icomp=0; icomp<nrcomps; icomp++ )
    {
	if ( trc.info_.sampling.start < info_.sampling.start )
	{
	    trc.set( 0, get(0,icomp), icomp );
	    trc.set( sz-1, mGetVal(sz-1,icomp), icomp );
	}
	else
	{
	    trc.set( sz-1, get(sz-1,icomp), icomp );
	    trc.set( 0, mGetVal(0,icomp), icomp );
	}
    }

    for ( int icomp=0; icomp<nrcomps; icomp++ )
    {
	for ( int isamp=1; isamp<sz-1; isamp++ )
	    trc.set( isamp, mGetVal(isamp,icomp), icomp );
    }
}


void SeisTrc::copyDataFrom( const SeisTrc& trc, int tarcomp, bool forcefloats )
{
    int startcomp = tarcomp < 0 ? 0 : tarcomp;
    int stopcomp = tarcomp < 0 ? trc.nrComponents()-1 : tarcomp;
    const int sz = trc.size();
    for ( int icomp=startcomp; icomp<=stopcomp; icomp++ )
    {
	DataCharacteristics dc = forcefloats
	    			? DataCharacteristics()
				: trc.data().getInterpreter(icomp)->dataChar();

	if ( nrComponents() <= icomp )
	    data().addComponent( sz, dc );
	else
	{
	    if ( data().getInterpreter(icomp)->dataChar() != dc )
		data().setComponent(dc,icomp);
	    if ( data_.size(icomp) != sz )
		data().getComponent(icomp)->reSize( sz );
	}
	memcpy( data().getComponent(icomp)->data(),
		trc.data().getComponent(icomp)->data(),
		sz * (int)dc.nrBytes() );
    }
}


#define mErrRet(msg) \
    { \
	if ( errbuf )	{ *errbuf=msg; } \
	else		{ sock.writeErrorMsg(msg); } \
	return false; \
    } 

#define mSockErrRet( msg ) \
    { \
	if ( errbuf ) \
	{ \
	    sock.fetchMsg(*errbuf); \
	    if ( errbuf->isEmpty() ) \
	    { \
		*errbuf = msg; \
		pErrMsg("Warning: error msg empty; fallback to default msg.");\
	    } \
	} \
	return false; \
    } 

bool SeisTrc::putTo(Socket& sock, bool withinfo, BufferString* errbuf) const
{
    if ( !sock.write( withinfo ) ) mSockErrRet( "Could not write to socket." );

    if ( withinfo )
    {
	IOPar infpar;
	info().fillPar(infpar);
	if ( !sock.write( infpar ) )
	    { mSockErrRet( "Could not write to socket." ); }
    }

    int nrcomps = nrComponents();
    if ( !sock.write( nrcomps ) )
	    { mSockErrRet( "Could not write to socket." ); }

    for( int idx=0; idx< nrComponents(); idx++ )
    {
	DataCharacteristics dc = data_.getInterpreter(idx)->dataChar();

	BufferString dcrep;
	dc.toString( dcrep.buf() );

	if ( !sock.write(dcrep) )
	    { mSockErrRet( "Could not write to socket." ); }

	const DataBuffer* buf = data_.getComponent( idx );
	int nrbytes = buf->size() * buf->bytesPerSample();
	if ( !sock.write( nrbytes ) )
	    { mSockErrRet( "Could not write to socket." ); }

	const unsigned char* rawdata = buf->data();

	if ( !sock.writedata( rawdata, nrbytes ) )
	    { mSockErrRet( "Could not write data to socket." ); }
    }

    return true;
}


bool SeisTrc::getFrom( Socket& sock, BufferString* errbuf )
{
    int totalcmps = nrComponents();
    for ( int idx=0; idx < totalcmps; idx++ )
	data().delComponent(0);

    if ( nrComponents() )
	{ mErrRet( "Could not clear trace buffers" ); }

    bool withinfo;
    if ( !sock.read( withinfo ) )
	    { mSockErrRet( "Could not read from socket." ); }
    if ( withinfo )
    {
	IOPar infpar;
	if ( !sock.read( infpar ) )
	    { mSockErrRet( "Could not read from socket." ); }
	info().usePar(infpar);
    }

    int nrcomps;
    if ( !sock.read( nrcomps ) )
	{ mSockErrRet( "Could not read from socket." ); }

    for ( int idx=0; idx<nrcomps; idx++ )
    {
	BufferString dcrep;
	if ( !sock.read(dcrep) )
	    { mSockErrRet( "Could not read from socket." ); }

	DataCharacteristics dc( dcrep );

	int nrbytes;
	if ( !sock.read( nrbytes ) )
	    { mSockErrRet( "Could not read from socket." ); }

	int nrsamples = nrbytes / (int)dc.nrBytes();
	data().addComponent(nrsamples,dc);

	unsigned char* dest = data().getComponent(idx)->data();
	if ( !dest ) { mErrRet("Could not create buffer for trace data."); }

	int bytesread = sock.readdata( (void*)dest, nrbytes );
	if ( bytesread != nrbytes )
	    { mSockErrRet( "Could not read data from socket." ); }
    }

    return true;
} 


float* SeisTrcValueSeries::arr()
{
    float val;
    static DataCharacteristics dc( val );
    const TraceDataInterpreter* tdi = trc_.data().getInterpreter( icomp_ );
    if ( !tdi ) return 0;

    if ( tdi->dataChar()!=dc )
	return 0;

    return (float*) trc_.data().getComponent( icomp_ )->data();
}


const float* SeisTrcValueSeries::arr() const
{ return const_cast<SeisTrcValueSeries*>( this )->arr(); }
