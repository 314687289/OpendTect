/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : May 1995
 * FUNCTION : Seg-Y headers
-*/

static const char* rcsID = "$Id: segyhdr.cc,v 1.55 2008-09-15 10:10:36 cvsbert Exp $";


#include "segyhdr.h"
#include "fixstring.h"
#include "string2.h"
#include "survinfo.h"
#include "ibmformat.h"
#include "settings.h"
#include "seisinfo.h"
#include "cubesampling.h"
#include "msgh.h"
#include "math2.h"
#include "envvars.h"
#include "timefun.h"
#include "linekey.h"
#include <string.h>
#include <ctype.h>
#include <iostream>
#include <stdio.h>
#include <math.h>

const char* SEGY::TrcHeaderDef::sXCoordByte = "X-coord byte";
const char* SEGY::TrcHeaderDef::sYCoordByte = "Y-coord byte";
const char* SEGY::TrcHeaderDef::sInlByte = "In-line byte";
const char* SEGY::TrcHeaderDef::sCrlByte = "Cross-line byte";
const char* SEGY::TrcHeaderDef::sOffsByte = "Offset byte";
const char* SEGY::TrcHeaderDef::sAzimByte = "Azimuth byte";
const char* SEGY::TrcHeaderDef::sTrNrByte = "Trace number byte";
const char* SEGY::TrcHeaderDef::sPickByte = "Pick byte";
const char* SEGY::TrcHeaderDef::sInlByteSz = "Nr bytes for In-line";
const char* SEGY::TrcHeaderDef::sCrlByteSz = "Nr bytes for Cross-line";
const char* SEGY::TrcHeaderDef::sOffsByteSz = "Nr bytes for Offset";
const char* SEGY::TrcHeaderDef::sAzimByteSz = "Nr bytes for Azimuth";
const char* SEGY::TrcHeaderDef::sTrNrByteSz = "Nr bytes for trace number";
bool SEGY::TxtHeader::info2d = false;


static void Ebcdic2Ascii(unsigned char*,int);
static void Ascii2Ebcdic(unsigned char*,int);


SEGY::TxtHeader::TxtHeader( bool rev1 )
{
    char cbuf[3];
    int nrlines = SegyTxtHeaderLength / 80;

    memset( txt, ' ', SegyTxtHeaderLength );
    for ( int i=0; i<nrlines; i++ )
    {
	int i80 = i*80; txt[i80] = 'C';
	sprintf( cbuf, "%02d", i+1 );
	txt[i80+1] = cbuf[0]; txt[i80+2] = cbuf[1];
    }

    FileNameString buf;
    buf = "Created by: ";
    buf += Settings::common()[ "Company" ]; buf += " (";
    buf += Time_getFullDateString(); buf += ")";
    putAt( 1, 6, 75, buf );
    putAt( 2, 6, 75, SI().name() );
    BinID bid = SI().sampling(false).hrg.start;
    Coord coord = SI().transform( bid );
    coord.x = fabs(coord.x); coord.y = fabs(coord.y);
    if ( !mIsEqual(bid.inl,coord.x,mDefEps)
      && !mIsEqual(bid.crl,coord.x,mDefEps)
      && !mIsEqual(bid.inl,coord.y,mDefEps)
      && !mIsEqual(bid.crl,coord.y,mDefEps) )
    {
	char* pbuf = buf.buf();
	coord = SI().transform( bid );
	bid.fill( pbuf ); buf += " = "; coord.fill( pbuf + strlen(buf) );
	putAt( 12, 6, 75, buf );
	bid.crl = SI().sampling(false).hrg.stop.crl;
	coord = SI().transform( bid );
	bid.fill( pbuf ); buf += " = "; coord.fill( pbuf + strlen(buf) );
	putAt( 13, 6, 75, buf );
	bid.inl = SI().sampling(false).hrg.stop.inl;
	bid.crl = SI().sampling(false).hrg.start.crl;
	coord = SI().transform( bid );
	bid.fill( pbuf ); buf += " = "; coord.fill( pbuf + strlen(buf) );
	putAt( 14, 6, 75, buf );
    }

    if ( !SI().zIsTime() )
    {
	buf = "Depth survey: 1 SEG-Y millisec = 1 ";
	buf += SI().getZUnit(false);
	putAt( 18, 6, 75, buf.buf() );
    }
    BufferString rvstr( "SEG Y REV" );
    rvstr += rev1 ? 1 : 0;
    putAt( 39, 6, 75, rvstr.buf() );
    putAt( 40, 6, 75, "END TEXTUAL HEADER" );
}


void SEGY::TxtHeader::setAscii()
{ if ( txt[0] != 'C' ) Ebcdic2Ascii( txt, SegyTxtHeaderLength ); }
void SEGY::TxtHeader::setEbcdic()
{ if ( txt[0] == 'C' ) Ascii2Ebcdic( txt, SegyTxtHeaderLength ); }


void SEGY::TxtHeader::setUserInfo( const char* infotxt )
{
    if ( !infotxt || !*infotxt ) return;

    FileNameString buf;
    int lnr = 16;
    while ( lnr < 35 )
    {
	char* ptr = buf.buf();
	int idx = 0;
	while ( *infotxt && *infotxt != '\n' && ++idx < 75 )
	    *ptr++ = *infotxt++;
	*ptr = '\0';
	putAt( lnr, 5, 80, buf );

	if ( !*infotxt ) break;
	lnr++;
	infotxt++;
    }
}


#define mPutBytePos(line,s,memb) \
    buf = s; buf += thd.memb; \
    putAt( line, 6, 6+buf.size(), buf )
#define mPutBytePosSize(line,s,memb) \
    buf = s; buf += thd.memb; \
    buf += " ("; buf += thd.memb##bytesz; buf += "-byte int)"; \
    putAt( line, 6, 6+buf.size(), buf )

void SEGY::TxtHeader::setPosInfo( const SEGY::TrcHeaderDef& thd )
{
    BufferString buf;
    buf = "Byte positions (in addition to REV. 1 standard positions):";
    putAt( 5, 6, 6+buf.size(), buf );
    mPutBytePos( 6, "X-coordinate: ", xcoord );
    mPutBytePos( 7, "Y-coordinate: ", ycoord );

    if ( info2d )
    {
	mPutBytePosSize( 8, "Trace number: ", trnr );
	if ( !thd.linename.isEmpty() )
	{
	    LineKey lk( thd.linename );
	    putAt( 3, 6, 20, "Line name:" );
	    putAt( 3, 20, 75, lk.lineName() );
	    putAt( 3, 45, 75, lk.attrName() );
	}
	if ( thd.pinfo )
	{
	    putAt( 4, 6, 20, "CDP range: " );
	    BufferString posstr;
	    posstr = thd.pinfo->crlrg.start;
	    putAt( 4, 20, 30, posstr );
	    posstr = thd.pinfo->crlrg.stop;
	    putAt( 4, 30, 75, posstr );
	}
	buf = "2-D line";
    }
    else
    {
	mPutBytePosSize( 8, "In-line:      ", inl );
	mPutBytePosSize( 9, "X-line:       ", crl );
	buf = "I/X bytes: "; buf += thd.inl;
	buf += " / "; buf += thd.crl;
    }
    putAt( 36, 6, 75, buf );
}


void SEGY::TxtHeader::setStartPos( float sp )
{
    BufferString buf;
    if ( !mIsZero(sp,mDefEps) )
    {
	buf = "First sample ";
	buf += SI().zIsTime() ? "time " : "depth ";
	buf += SI().getZUnit(); buf += ": ";
	buf += sp * SI().zFactor();
    }
    putAt( 37, 6, 75, buf );
}


void SEGY::TxtHeader::getText( BufferString& bs )
{
    char buf[80];
    bs = "";
    for ( int idx=0; idx<40; idx++ )
    {
	getFrom( idx, 3, 80, buf );
	if ( !buf[0] ) continue;
	if ( *(const char*)bs ) bs += "\n";
	bs += buf;
    }
}


void SEGY::TxtHeader::getFrom( int line, int pos, int endpos, char* str ) const
{
    if ( !str ) return;

    int charnr = (line-1)*80 + pos - 1;
    if ( endpos > 80 ) endpos = 80;
    int maxcharnr = (line-1)*80 + endpos;

    while ( isspace(txt[charnr]) && charnr < maxcharnr ) charnr++;
    while ( charnr < maxcharnr ) *str++ = txt[charnr++];
    *str = '\0';
    removeTrailingBlanks( str );
}


void SEGY::TxtHeader::putAt( int line, int pos, int endpos, const char* str )
{
    if ( !str || !*str ) return;

    int charnr = (line-1)*80 + pos - 1;
    if ( endpos > 80 ) endpos = 80;
    int maxcharnr = (line-1)*80 + endpos;

    while ( charnr < maxcharnr && *str )
    {
	txt[charnr] = *str;
	charnr++; str++;
    }
}


void SEGY::TxtHeader::dump( std::ostream& stream ) const
{
    char buf[81];
    buf[80] = '\0';

    for ( int line=0; line<40; line++ )
    {
	memcpy( buf, &txt[80*line], 80 );
	stream << buf << std::endl;
    }
}


static const unsigned char* getBytes( const unsigned char* inpbuf,
				      bool needswap, int bytenr, int nrbytes )
{
    static unsigned char swpbuf[4];
    if ( !needswap ) return inpbuf + bytenr;

    memcpy( swpbuf, inpbuf+bytenr, nrbytes );
    SwapBytes( swpbuf, nrbytes );
    return swpbuf;
}

#define mGetBytes(bnr,nrb) getBytes( buf, needswap, bnr, nrb )


SEGY::BinHeader::BinHeader( bool rev1 )
	: needswap(false)
    	, isrev1(rev1 ? 256 : 0)
    	, nrstzs(0)
    	, fixdsz(1)
{
    memset( &jobid, 0, SegyBinHeaderLength );
    mfeet = format = 1;
    float fhdt = SeisTrcInfo::defaultSampleInterval() * 1000;
    if ( SI().zIsTime() && fhdt < 32.768 )
	fhdt *= 1000;
    hdt = (short)fhdt;
}


#define mSBHGet(memb,typ,nb) \
    if ( needswap ) SwapBytes( b, nb ); \
    memb = IbmFormat::as##typ( b ); \
    if ( needswap ) SwapBytes( b, nb ); \
    b += nb;

void SEGY::BinHeader::getFrom( const void* buf )
{
    unsigned char* b = (unsigned char*)buf;

    mSBHGet(jobid,Int,4);
    mSBHGet(lino,Int,4);
    mSBHGet(reno,Int,4);
    mSBHGet(ntrpr,Short,2);
    mSBHGet(nart,Short,2);
    mSBHGet(hdt,Short,2);
    mSBHGet(dto,Short,2);
    mSBHGet(hns,Short,2);
    mSBHGet(nso,Short,2);
    mSBHGet(format,Short,2);
    mSBHGet(fold,Short,2);
    mSBHGet(tsort,Short,2);
    mSBHGet(vscode,Short,2);
    mSBHGet(hsfs,Short,2);
    mSBHGet(hsfe,Short,2);
    mSBHGet(hslen,Short,2);
    mSBHGet(hstyp,Short,2);
    mSBHGet(schn,Short,2);
    mSBHGet(hstas,Short,2);
    mSBHGet(hstae,Short,2);
    mSBHGet(htatyp,Short,2);
    mSBHGet(hcorr,Short,2);
    mSBHGet(bgrcv,Short,2);
    mSBHGet(rcvm,Short,2);
    mSBHGet(mfeet,Short,2);
    mSBHGet(polyt,Short,2);
    mSBHGet(vpol,Short,2);

    for ( int i=0; i<SegyBinHeaderUnassUShorts; i++ )
    {
	mSBHGet(hunass[i],UnsignedShort,2);
    }
    isrev1 = hunass[120];
    fixdsz = hunass[121];
    nrstzs = hunass[122];
}


#define mSBHPut(memb,typ,nb) \
    IbmFormat::put##typ( memb, b ); \
    if ( needswap ) SwapBytes( b, nb ); \
    b += nb;


void SEGY::BinHeader::putTo( void* buf ) const
{
    unsigned char* b = (unsigned char*)buf;

    mSBHPut(jobid,Int,4);
    mSBHPut(lino,Int,4);
    mSBHPut(reno,Int,4);
    mSBHPut(ntrpr,Short,2);
    mSBHPut(nart,Short,2);
    mSBHPut(hdt,Short,2);
    mSBHPut(dto,Short,2);
    mSBHPut(hns,Short,2);
    mSBHPut(nso,Short,2);
    mSBHPut(format,Short,2);
    mSBHPut(fold,Short,2);
    mSBHPut(tsort,Short,2);
    mSBHPut(vscode,Short,2);
    mSBHPut(hsfs,Short,2);
    mSBHPut(hsfe,Short,2);
    mSBHPut(hslen,Short,2);
    mSBHPut(hstyp,Short,2);
    mSBHPut(schn,Short,2);
    mSBHPut(hstas,Short,2);
    mSBHPut(hstae,Short,2);
    mSBHPut(htatyp,Short,2);
    mSBHPut(hcorr,Short,2);
    mSBHPut(bgrcv,Short,2);
    mSBHPut(rcvm,Short,2);
    mSBHPut(mfeet,Short,2);
    mSBHPut(polyt,Short,2);
    mSBHPut(vpol,Short,2);

    unsigned short* v = const_cast<unsigned short*>( hunass );
    v[120] = isrev1; v[121] = fixdsz; v[122] = nrstzs;
    for ( int i=0; i<SegyBinHeaderUnassUShorts; i++ )
	{ mSBHPut(hunass[i],UnsignedShort,2); }
}


static void Ebcdic2Ascii( unsigned char *chbuf, int len )
{
    int i;
    static unsigned char e2a[256] = {
0x00,0x01,0x02,0x03,0x9C,0x09,0x86,0x7F,0x97,0x8D,0x8E,0x0B,0x0C,0x0D,0x0E,0x0F,
0x10,0x11,0x12,0x13,0x9D,0x85,0x08,0x87,0x18,0x19,0x92,0x8F,0x1C,0x1D,0x1E,0x1F,
0x80,0x81,0x82,0x83,0x84,0x0A,0x17,0x1B,0x88,0x89,0x8A,0x8B,0x8C,0x05,0x06,0x07,
0x90,0x91,0x16,0x93,0x94,0x95,0x96,0x04,0x98,0x99,0x9A,0x9B,0x14,0x15,0x9E,0x1A,
0x20,0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0x5B,0x2E,0x3C,0x28,0x2B,0x21,
0x26,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB0,0xB1,0x5D,0x24,0x2A,0x29,0x3B,0x5E,
0x2D,0x2F,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0x7C,0x2C,0x25,0x5F,0x3E,0x3F,
0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,0xC0,0xC1,0xC2,0x60,0x3A,0x23,0x40,0x27,0x3D,0x22,
0xC3,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,
0xCA,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,
0xD1,0x7E,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,
0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,
0x7B,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,
0x7D,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,0x51,0x52,0xEE,0xEF,0xF0,0xF1,0xF2,0xF3,
0x5C,0x9F,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,
0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
    };

    for ( i=0; i<len; i++ ) chbuf[i] = e2a[chbuf[i]];
}


static void Ascii2Ebcdic( unsigned char *chbuf, int len )
{
    int i;
    static unsigned char a2e[256] = {
0x00,0x01,0x02,0x03,0x37,0x2D,0x2E,0x2F,0x16,0x05,0x25,0x0B,0x0C,0x0D,0x0E,0x0F,
0x10,0x11,0x12,0x13,0x3C,0x3D,0x32,0x26,0x18,0x19,0x3F,0x27,0x1C,0x1D,0x1E,0x1F,
0x40,0x4F,0x7F,0x7B,0x5B,0x6C,0x50,0x7D,0x4D,0x5D,0x5C,0x4E,0x6B,0x60,0x4B,0x61,
0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0x7A,0x5E,0x4C,0x7E,0x6E,0x6F,
0x7C,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,
0xD7,0xD8,0xD9,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0x4A,0xE0,0x5A,0x5F,0x6D,
0x79,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x91,0x92,0x93,0x94,0x95,0x96,
0x97,0x98,0x99,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xC0,0x6A,0xD0,0xA1,0x07,
0x20,0x21,0x22,0x23,0x24,0x15,0x06,0x17,0x28,0x29,0x2A,0x2B,0x2C,0x09,0x0A,0x1B,
0x30,0x31,0x1A,0x33,0x34,0x35,0x36,0x08,0x38,0x39,0x3A,0x3B,0x04,0x14,0x3E,0xE1,
0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x51,0x52,0x53,0x54,0x55,0x56,0x57,
0x58,0x59,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x70,0x71,0x72,0x73,0x74,0x75,
0x76,0x77,0x78,0x80,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,0x90,0x9A,0x9B,0x9C,0x9D,0x9E,
0x9F,0xA0,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,
0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xDA,0xDB,
0xDC,0xDD,0xDE,0xDF,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
    };

    for ( i=0; i<len; i++ ) chbuf[i] = a2e[chbuf[i]];
}


// Comments added after request from Hamish McIntyre

#define mPrHead(mem,byt,comm) \
	if ( mem ) \
	{ \
	    strm << '\t' << #mem << '\t' << byt+1 << '\t' << mem \
		 << "\t(" << comm << ')'; \
	    strm << '\n'; \
	}

void SEGY::BinHeader::dump( std::ostream& strm ) const
{
    mPrHead( jobid, 0, "job identification number" )
    mPrHead( lino, 4, "line number (only one line per reel)" )
    mPrHead( reno, 8, "reel number" )
    mPrHead( ntrpr, 12, "number of data traces per record" )
    mPrHead( nart, 14, "number of auxiliary traces per record" )
    mPrHead( hdt, 16, "sample interval in micro secs for this reel" )
    mPrHead( dto, 18, "same for original field recording" )
    mPrHead( hns, 20, "number of samples per trace for this reel" )
    mPrHead( nso, 22, "same for original field recording" )
    mPrHead( format, 24, "sample format (1=float, 3=16 bit, 8=8-bit)" )
    mPrHead( fold, 26, "CDP fold expected per CDP ensemble" )
    mPrHead( tsort, 28, "trace sorting code" )
    mPrHead( vscode, 30, "vertical sum code" )
    mPrHead( hsfs, 32, "sweep frequency at start" )
    mPrHead( hsfe, 34, "sweep frequency at end" )
    mPrHead( hslen, 36, "sweep length (ms)" )
    mPrHead( hstyp, 38, "sweep type code" )
    mPrHead( schn, 40, "trace number of sweep channel" )
    mPrHead( hstas, 42, "sweep trace taper length at start" )
    mPrHead( hstae, 44, "sweep trace taper length at end" )
    mPrHead( htatyp, 46, "sweep trace taper type code" )
    mPrHead( hcorr, 48, "correlated data traces code" )
    mPrHead( bgrcv, 50, "binary gain recovered code" )
    mPrHead( rcvm, 52, "amplitude recovery method code" )
    mPrHead( mfeet, 54, "measurement system code (1=m 2=ft)" )
    mPrHead( polyt, 56, "impulse signal polarity code" )
    mPrHead( vpol, 58, "vibratory polarity code" )

    SEGY::BinHeader& self = *const_cast<SEGY::BinHeader*>(this);
    unsigned short tmpisrev1 = isrev1;
    if ( isrev1 ) self.isrev1 = 1;
    mPrHead( isrev1, 300, "[REV1 only] SEG-Y revision code" )
    self.isrev1 = tmpisrev1;
    mPrHead( fixdsz, 302, "[REV1 only] Fixed trace size?" )
    mPrHead( nrstzs, 304, "[REV1 only] Number of extra headers" );

    for ( int i=0; i<SegyBinHeaderUnassUShorts; i++ )
	if ( hunass[i] != 0 && (i < 120 || i > 122) )
	    strm << "\tExtra\t" << 60 + 2*i << '\t' << hunass[i]
		 << "\t(Non-standard - unassigned)\n";
    strm << std::endl;
}


SEGY::TrcHeader::TrcHeader( unsigned char* b, bool rev1,
			    const SEGY::TrcHeaderDef& hd )
    : buf(b)
    , hdef(hd)
    , needswap(false)
    , isrev1(rev1)
    , seqnr(1)
    , lineseqnr(1)
    , previnl(-1)
{
}


#undef mSEGYTrcDef
#define mSEGYTrcDef(attrnr,mem,byt,fun,nrbyts) \
    strm << '\t' << #mem << '\t' << byt+1 << '\t' \
         << IbmFormat::as##fun( getBytes(buf,needswap,byt,nrbyts) ) << '\n'
#undef mSEGYTrcDefUnass
#define mSEGYTrcDefUnass(attrnr,byt,fun,nrbyts) \
    if ( IbmFormat::as##fun( getBytes(buf,needswap,byt,nrbyts) ) ) \
	strm << '\t' << '-' << '\t' << byt+1 << '\t' \
	     << IbmFormat::as##fun( getBytes(buf,needswap,byt,nrbyts) ) << '\n'

void SEGY::TrcHeader::dump( std::ostream& strm ) const
{
#include "segyhdr.inc"
}


#undef mSEGYTrcDef
#define mSEGYTrcDef(attrnr,mem,byt,fun,nrbyts) \
    if ( nr == attrnr ) \
	return SEGY::TrcHeader::Val( byt, #mem, \
		IbmFormat::as##fun( getBytes(buf,needswap,byt,nrbyts) ) )
#undef mSEGYTrcDefUnass
#define mSEGYTrcDefUnass(attrnr,byt,fun,nrbyts) \
    mSEGYTrcDef(attrnr,-,byt,fun,nrbyts)

SEGY::TrcHeader::Val SEGY::TrcHeader::getVal( int nr ) const
{
#include "segyhdr.inc"
    return Val( -1, "", 0 );
}


unsigned short SEGY::TrcHeader::nrSamples() const
{
    return IbmFormat::asUnsignedShort( getBytes(buf,needswap,114,2) );
}


void SEGY::TrcHeader::putSampling( SamplingData<float> sd, unsigned short ns )
{
    const float zfac = SI().zFactor();
    float drt = sd.start * zfac;
    short delrt = (short)mNINT(drt);
    IbmFormat::putShort( -delrt, buf+104 ); // HRS and Petrel
    IbmFormat::putShort( delrt, buf+108 );
    IbmFormat::putUnsignedShort( (unsigned short)
	    			 (sd.step*1e3*zfac+.5), buf+116 );
    if ( ns != 0 )
	IbmFormat::putUnsignedShort( ns, buf+114 );
}


static void putRev1Flds( const SeisTrcInfo& ti, unsigned char* buf )
{
    IbmFormat::putInt( mNINT(ti.coord.x*10), buf+180 );
    IbmFormat::putInt( mNINT(ti.coord.y*10), buf+184 );
    IbmFormat::putInt( ti.binid.inl, buf+188 );
    IbmFormat::putInt( ti.binid.crl, buf+192 );
    IbmFormat::putInt( ti.nr, buf+196 );
}


void SEGY::TrcHeader::use( const SeisTrcInfo& ti )
{
    if ( !isrev1 ) // starting default
	putRev1Flds( ti, buf );

    IbmFormat::putShort( 1, buf+28 ); // trid
    IbmFormat::putShort( 1, buf+34 ); // duse
    IbmFormat::putShort( 1, buf+88 ); // counit

    const bool is2d = SEGY::TxtHeader::info2d;
    if ( !is2d && ti.binid.inl != previnl )
	lineseqnr = 1;
    previnl = ti.binid.inl;
    IbmFormat::putInt( is2d ? seqnr : lineseqnr, buf );
    IbmFormat::putInt( seqnr, buf+4 );
    seqnr++; lineseqnr++;

    // Now the more general standards
    IbmFormat::putInt( is2d ? ti.nr : ti.binid.crl, buf+16 ); // ep
    IbmFormat::putInt( is2d ? ti.binid.crl : ti.nr, buf+20 ); // cdp
    IbmFormat::putShort( -10, buf+70 ); // scalco
    if ( mIsUdf(ti.coord.x) )
    {
	if ( hdef.xcoord != 255 )
	    IbmFormat::putInt( 0, buf+hdef.xcoord-1 );
	if ( hdef.ycoord != 255 )
	    IbmFormat::putInt( 0, buf+hdef.ycoord-1 );
    }
    else
    {
	if ( hdef.xcoord != 255 )
	    IbmFormat::putInt( mNINT(ti.coord.x * 10), buf+hdef.xcoord-1 );
	if ( hdef.ycoord != 255 )
	    IbmFormat::putInt( mNINT(ti.coord.y * 10), buf+hdef.ycoord-1 );
    }

#define mPutIntVal(timemb,hdmemb) \
    if ( timemb > 0 && hdef.hdmemb != 255 ) \
    { \
	if ( hdef.hdmemb##bytesz == 2 ) \
	    IbmFormat::putShort( timemb, buf+hdef.hdmemb-1 ); \
	else \
	    IbmFormat::putInt( timemb, buf+hdef.hdmemb-1 ); \
    }

    mPutIntVal(ti.binid.inl,inl);
    mPutIntVal(ti.binid.crl,crl);
    mPutIntVal(ti.nr,trnr);
    int iv = mNINT( ti.offset ); mPutIntVal(iv,offs);
    iv = mNINT( ti.azimuth * 360 / M_PI ); mPutIntVal(iv,azim);

    const float zfac = SI().zFactor();
    if ( !mIsUdf(ti.pick) && hdef.pick != 255 )
	IbmFormat::putInt( mNINT(ti.pick*zfac), buf+hdef.pick-1 );

    // Absolute priority, therefore possibly overwriting previous
    putSampling( ti.sampling, 0 );

    if ( isrev1 ) // Now this overrules everything
	putRev1Flds( ti, buf );
}


float SEGY::TrcHeader::postScale( int numbfmt ) const
{
    if ( numbfmt != 2 && numbfmt != 3 && numbfmt != 5 ) return 1.;

    // There seems to be software (Paradigm?) putting this on byte 189
    // Then indeed, we'd expect this to be 4 byte. Duh.
    static bool postscale_byte_established = false;
    static int postscale_byte;
    static bool postscale_isint;
    if ( !postscale_byte_established )
    {
	postscale_byte_established = true;
	postscale_byte = GetEnvVarIVal( "OD_SEGY_TRCSCALE_BYTE", 169 );
	postscale_isint = GetEnvVarYN( "OD_SEGY_TRCSCALE_4BYTE" );
    }

    unsigned char* ptr = buf + postscale_byte - 1 ;
    short trwf = postscale_isint ?
	IbmFormat::asInt( mGetBytes(postscale_byte-1,4) )
      : IbmFormat::asShort( mGetBytes(postscale_byte-1,2) );
    if ( trwf == 0 || trwf > 60 || trwf < -60 ) return 1;

    // According to the standard, trwf cannot be negative ...
    // But I'll support it anyway because some files are like this
    const bool isneg = trwf < 0;
    if ( isneg )
	trwf = -trwf;
    else
	trwf = trwf;
    float ret = Math::IntPowerOf( ((float)2), trwf );
    return isneg ? ret : 1. / ret;
}


static void getRev1Flds( SeisTrcInfo& ti, const unsigned char* buf,
			 bool needswap )
{
    ti.coord.x = IbmFormat::asInt( mGetBytes(180,4) );
    ti.coord.y = IbmFormat::asInt( mGetBytes(184,4) );
    ti.binid.inl = IbmFormat::asInt( mGetBytes(188,4) );
    ti.binid.crl = IbmFormat::asInt( mGetBytes(192,4) );
}


void SEGY::TrcHeader::fill( SeisTrcInfo& ti, float extcoordsc ) const
{
    if ( mIsZero(extcoordsc,1e-8)
	    && !GetEnvVarYN("OD_ALLOW_ZERO_COORD_SCALING") )
    {
	static bool warningdone = false;
	if ( !warningdone )
	{
	    ErrMsg( "Replacing requested zero scaling with 1" );
	    warningdone = true;
	}
	extcoordsc = 1;
    }

    if ( !isrev1 )
	getRev1Flds( ti, buf, needswap );

    const float zfac = 1. / SI().zFactor();
    short delrt = IbmFormat::asShort( mGetBytes(108,2) );
    if ( delrt == 0 )
    {
	delrt = -IbmFormat::asShort( mGetBytes(104,2) ); // HRS and Petrel
	float startz = delrt * zfac;
	if ( startz < -5000 || startz > 10000 )
	    delrt = 0;
    }
    ti.sampling.start = delrt * zfac;
    ti.sampling.step = IbmFormat::asUnsignedShort( mGetBytes(116,2) )
			* zfac * 0.001;

    ti.pick = ti.refpos = mUdf(float);
    ti.nr = IbmFormat::asInt( mGetBytes(0,4) );
    if ( hdef.pick != 255 )
	ti.pick = IbmFormat::asInt( mGetBytes(hdef.pick-1,4) ) * zfac;
    ti.coord.x = ti.coord.y = 0;
    if ( hdef.xcoord != 255 )
	ti.coord.x = IbmFormat::asInt( mGetBytes(hdef.xcoord-1,4));
    if ( hdef.ycoord != 255 )
	ti.coord.y = IbmFormat::asInt( mGetBytes(hdef.ycoord-1,4));

#define mGetIntVal(timemb,hdmemb) \
    if ( hdef.hdmemb != 255 ) \
	ti.timemb = hdef.hdmemb##bytesz == 2 \
	? IbmFormat::asShort( mGetBytes(hdef.hdmemb-1,2) ) \
	: IbmFormat::asInt( mGetBytes(hdef.hdmemb-1,4) )
	
    ti.binid.inl = ti.binid.crl = 0;
    mGetIntVal(binid.inl,inl);
    mGetIntVal(binid.crl,crl);
    mGetIntVal(nr,trnr);
    mGetIntVal(offset,offs);
    mGetIntVal(azimuth,azim);
    ti.azimuth *= M_PI / 360;

    if ( isrev1 )
	getRev1Flds( ti, buf, needswap );

    double scale = getCoordScale( extcoordsc );
    ti.coord.x *= scale;
    ti.coord.y *= scale;

    // Hack to enable shifts
    static double false_easting = 0; static double false_northing = 0;
    static bool false_stuff_got = false;
    if ( !false_stuff_got )
    {
	false_stuff_got = true;
	false_easting = GetEnvVarDVal( "OD_SEGY_FALSE_EASTING", 0 );
	false_northing = GetEnvVarDVal( "OD_SEGY_FALSE_NORTHING", 0 );
	if ( !mIsZero(false_easting,1e-5) || !mIsZero(false_northing,1e-5) )
	{
	    BufferString usrmsg( "For this OD run SEG-Y easting/northing " );
	    usrmsg += false_easting; usrmsg += "/"; usrmsg += false_northing;
	    usrmsg += " will be applied";
	    UsrMsg( usrmsg );
	}
    }
    ti.coord.x += false_easting;
    ti.coord.y += false_northing;
}


double SEGY::TrcHeader::getCoordScale( float extcoordsc ) const
{
    if ( !mIsUdf(extcoordsc) ) return extcoordsc;
    const short scalco = IbmFormat::asShort( mGetBytes(70,2) );
    return scalco ? (scalco > 0 ? scalco : -1./scalco) : 1;
}


Coord SEGY::TrcHeader::getCoord( bool rcv, float extcoordsc )
{
    double scale = getCoordScale( extcoordsc );
    Coord ret(	IbmFormat::asInt( mGetBytes(rcv?80:72,4) ),
		IbmFormat::asInt( mGetBytes(rcv?84:76,4) ) );
    ret.x *= scale; ret.y *= scale;
    return ret;
}


#define mGtFromPar(str,memb) \
    res = iopar.find( str ); \
    if ( res && *res ) memb = atoi( res )

void SEGY::TrcHeaderDef::usePar( const IOPar& iopar )
{
    const char*
    mGtFromPar( sInlByte, inl );
    mGtFromPar( sCrlByte, crl );
    mGtFromPar( sOffsByte, offs );
    mGtFromPar( sAzimByte, azim );
    mGtFromPar( sTrNrByte, trnr );
    mGtFromPar( sXCoordByte, xcoord );
    mGtFromPar( sYCoordByte, ycoord );
    mGtFromPar( sInlByteSz, inlbytesz );
    mGtFromPar( sCrlByteSz, crlbytesz );
    mGtFromPar( sOffsByteSz, offsbytesz );
    mGtFromPar( sAzimByteSz, azimbytesz );
    mGtFromPar( sTrNrByteSz, trnrbytesz );
    mGtFromPar( sPickByte, pick );
}


void SEGY::TrcHeaderDef::fromSettings()
{
    const IOPar* useiop = &Settings::common();
    IOPar* subiop = useiop->subselect( "SEG-Y" );
    if ( subiop && subiop->size() )
	useiop = subiop;
    usePar( *useiop );
    delete subiop;
}

#define mPutToPar(str,memb) \
    iopar.set( IOPar::compKey(key,str), memb )


void SEGY::TrcHeaderDef::fillPar( IOPar& iopar, const char* key ) const
{
    mPutToPar( sInlByte, inl );
    mPutToPar( sCrlByte, crl );
    mPutToPar( sOffsByte, offs );
    mPutToPar( sAzimByte, azim );
    mPutToPar( sTrNrByte, trnr );
    mPutToPar( sXCoordByte, xcoord );
    mPutToPar( sYCoordByte, ycoord );
    mPutToPar( sInlByteSz, inlbytesz );
    mPutToPar( sCrlByteSz, crlbytesz );
    mPutToPar( sOffsByteSz, offsbytesz );
    mPutToPar( sAzimByteSz, azimbytesz );
    mPutToPar( sTrNrByteSz, trnrbytesz );
    mPutToPar( sPickByte, pick );
}
