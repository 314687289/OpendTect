/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 23-3-1996
 * FUNCTION : Wavelet
-*/

static const char* rcsID = "$Id: wavelet.cc,v 1.28 2006-12-14 18:34:37 cvsbert Exp $";

#include "wavelet.h"
#include "seisinfo.h"
#include "wvltfact.h"
#include "ascstream.h"
#include "streamconn.h"
#include "survinfo.h"
#include "ioobj.h"
#include "keystrs.h"
#include "errh.h"
#include "ptrman.h"
#include "tabledef.h"

#include <math.h>


Wavelet::Wavelet( const char* nm, int idxfsamp, float sr )
	: NamedObject(nm)
	, iw(idxfsamp)
	, dpos(mIsUdf(sr)?SeisTrcInfo::defaultSampleInterval(true):sr)
	, sz(0)
	, samps(0)
{
}


Wavelet::Wavelet( bool isricker, float fpeak, float sr, float scale )
    	: dpos(sr)
    	, sz(0)
	, samps(0)
{
    if ( mIsUdf(dpos) )
	dpos = SeisTrcInfo::defaultSampleInterval(true);
    if ( mIsUdf(scale) )
	scale = 1;
    if ( mIsUdf(fpeak) || fpeak <= 0 )
	fpeak = 25;
    iw = (int)( -( 1 + 1. / (fpeak*dpos) ) );

    BufferString nm( isricker ? "Ricker " : "Sinc " );
    nm += " (Central freq="; nm += fpeak; nm += ")";
    setName( nm );

    int lw = 1 - 2*iw;
    reSize( lw );
    float pos = iw * dpos;
    for ( int idx=0; idx<lw; idx++ )
    {
	float x = M_PI * fpeak * pos;
	float x2 = x * x;
	if ( idx == -iw )
	    samps[idx] = scale;
	else if ( isricker )
	    samps[idx] = scale * exp(-x2) * (1-2*x2);
	else
	{
	    samps[idx] = scale * exp(-x2) * sin(x)/x;
	    if ( samps[idx] < 0 ) samps[idx] = 0;
	}
	pos += dpos;
    }
}


Wavelet::Wavelet( const Wavelet& wv )
	: sz(0)
	, samps(0)
{
    *this = wv;
}


Wavelet& Wavelet::operator =( const Wavelet& wv )
{
    iw = wv.iw;
    dpos = wv.dpos;
    reSize( wv.size() );
    if ( sz ) memcpy( samps, wv.samps, sz*sizeof(float) );
    return *this;
}


Wavelet::~Wavelet()
{
    delete [] samps;
}


Wavelet* Wavelet::get( const IOObj* ioobj )
{
    if ( !ioobj ) return 0;
    WaveletTranslator* tr = (WaveletTranslator*)ioobj->getTranslator();
    if ( !tr ) return 0;
    Wavelet* newwv = 0;

    Conn* connptr = ioobj->getConn( Conn::Read );
    if ( connptr && !connptr->bad() )
    {
	newwv = new Wavelet;
	if ( !tr->read( newwv, *connptr ) )
	{
	    ErrMsg( "Problem reading Wavelet from file (format error?)" );
	    delete newwv;
	    newwv = 0;
	}
    }
    else
	ErrMsg( "Cannot open Wavelet file" );

    delete connptr; delete tr;
    return newwv;
}


bool Wavelet::put( const IOObj* ioobj ) const
{
    if ( !ioobj ) return false;
    WaveletTranslator* tr = (WaveletTranslator*)ioobj->getTranslator();
    if ( !tr ) return false;
    bool retval = false;

    Conn* connptr = ioobj->getConn( Conn::Write );
    if ( connptr && !connptr->bad() )
    {
	if ( tr->write( this, *connptr ) )
	    retval = true;
	else
	    ErrMsg( "Cannot write Wavelet" );
    }
    else
	ErrMsg( "Cannot open Wavelet file for write" );

    delete connptr; delete tr;
    return retval;
}


void Wavelet::reSize( int newsz )
{
    if ( newsz < 1 ) { delete [] samps; samps = 0; sz = 0; return; }

    float* newsamps = new float [newsz];
    if ( !newsamps ) return;

    delete [] samps;
    samps = newsamps;
    sz = newsz;
}


void Wavelet::set( int center, float samplerate )
{
    iw = -center;
    dpos = samplerate;
}


void Wavelet::transform( float constant, float factor )
{
    for ( int idx=0; idx<sz; idx++ )
	samps[idx] = constant + samps[idx] * factor;
}


int WaveletTranslatorGroup::selector( const char* key )
{
    int retval = defaultSelector( theInst().userName(), key );
    if ( retval ) return retval;

    if ( defaultSelector("Wavelet directory",key)
      || defaultSelector("Seismic directory",key) ) return 1;
    return 0;
}

mDefSimpleTranslatorioContext(Wavelet,Seis)


static char* sLength	= "Length";
static char* sIndex	= "Index First Sample";
static char* sSampRate	= "Sample Rate";


bool dgbWaveletTranslator::read( Wavelet* wv, Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )	return false;

    ascistream astream( ((StreamConn&)conn).iStream() );
    if ( !astream.isOfFileType(mTranslGroupName(Wavelet)) )
	return false;

    const float scfac = 1000;
    int iw = 0; float sr = scfac * SI().zStep();
    while ( !atEndOfSection( astream.next() ) )
    {
        if ( astream.hasKeyword( sLength ) )
	    wv->reSize( astream.getVal() );
        else if ( astream.hasKeyword( sIndex ) )
	    iw = astream.getVal();
        else if ( astream.hasKeyword(sKey::Name) )
	    wv->setName( astream.value() );
        else if ( astream.hasKeyword( sSampRate ) )
	    sr = astream.getValue();
    }
    wv->set( -iw, sr / scfac );

    for ( int idx=0; idx<wv->size(); idx++ )
	astream.stream() >> wv->samples()[idx];

    return astream.stream().good();
}


bool dgbWaveletTranslator::write( const Wavelet* wv, Conn& conn )
{
    if ( !conn.forWrite() || !conn.isStream() )	return false;

    ascostream astream( ((StreamConn&)conn).oStream() );
    UserIDString head( mTranslGroupName(Wavelet) );
    head += " file";
    if ( !astream.putHeader( head ) ) return false;

    if ( *(const char*)wv->name() ) astream.put( sKey::Name, wv->name() );
    astream.put( sLength, wv->size() );
    astream.put( sIndex, -wv->centerSample() );
    astream.put( sSampRate, wv->sampleRate() * SI().zFactor() );
    astream.newParagraph();
    for ( int idx=0; idx<wv->size(); idx++ )
	astream.stream() << wv->samples()[idx] << '\n';
    astream.newParagraph();

    return astream.stream().good();
}


Table::FormatDesc* WaveletAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Wavelet" );
    fd->headerinfos_ += new Table::TargetInfo( "Center sample (empty=mid)",
						IntInpSpec(), Table::Optional );
    BufferString txt( "Sample interval " ); txt += SI().getZUnit();
    float defsi = SI().zStep() * SI().zFactor();
    fd->headerinfos_ += new Table::TargetInfo( txt, FloatInpSpec(defsi),
	    				       Table::Required );
    fd->bodyinfos_ += new Table::TargetInfo( "Data samples", FloatInpSpec(),
					     Table::Required );
    return fd;
}


#define mErrRet(s) { if ( s ) errmsg_ = s; return 0; }

Wavelet* WaveletAscIO::get( std::istream& strm ) const
{
    if ( !getHdrVals(strm) )
	return 0;

    int centersmp = mUdf(int);
    if ( !vals_.get(0).isEmpty() )
    {
	centersmp = atoi(vals_.get(0));
	if ( centersmp > 0 )
	    centersmp = -centersmp + 1; // Users start at 1
    }
    float sr = atof( vals_.get(1) );
    if ( sr == 0 || mIsUdf(sr) )
	sr = SI().zStep() * SI().zFactor();
    else if ( sr < 0 )
	sr = -sr;
    sr /= SI().zFactor();

    TypeSet<float> samps;
    while ( true )
    {
	int ret = getNextBodyVals( strm );
	if ( ret < 0 ) mErrRet(0)
	if ( ret == 0 ) break;

	const char* vstr = vals_.get( 0 ).buf();
	if ( isNumberString(vstr,NO) )
	    samps += atof( vstr );
    }
    if ( samps.isEmpty() )
	mErrRet( "No valid data samples found" )
    if ( mIsUdf(centersmp) || centersmp > samps.size() )
	centersmp = -samps.size() / 2;

    Wavelet* ret = new Wavelet( "", centersmp, sr );
    ret->reSize( samps.size() );
    memcpy( ret->samples(), samps.arr(), ret->size() * sizeof(float) );
    return ret;
}


bool WaveletAscIO::put( std::ostream& ) const
{
    errmsg_ = "TODO: WaveletAscIO::put not implemented";
    return false;
}
