/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Sep 2008
-*/

static const char* rcsID = "$Id: segyfiledef.cc,v 1.12 2008-12-29 11:29:27 cvsranojay Exp $";

#include "segyfiledef.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "separstr.h"

const char* SEGY::FileDef::sKeyForceRev0 = "Force Rev0";
const char* SEGY::FilePars::sKeyNrSamples = "Nr samples overrule";
const char* SEGY::FilePars::sKeyNumberFormat = "Number format";
const char* SEGY::FilePars::sKeyByteSwap = "Byte swapping";
const char* SEGY::FileSpec::sKeyFileNrs = "File numbers";
const char* SEGY::FileReadOpts::sKeyCoordScale = "Coordinate scaling overrule";
const char* SEGY::FileReadOpts::sKeyTimeShift = "Start time overrule";
const char* SEGY::FileReadOpts::sKeySampleIntv = "Sample rate overrule";
const char* SEGY::FileReadOpts::sKeyICOpt = "IC -> XY";
const char* SEGY::FileReadOpts::sKeyPSOpt = "Offset source";
const char* SEGY::FileReadOpts::sKeyOffsDef = "Generate offsets";


static const char* allsegyfmtoptions[] = {
	"From file header",
	"1 - Floating point",
	"2 - Integer (4 byte)",
	"3 - Integer (2 byte)",
	"5 - IEEE float (4 byte)",
	"8 - Signed char (1 byte)",
	0
};


const char* SEGY::FileSpec::getFileName( int nr ) const
{
    if ( !isMultiFile() )
	return fname_.buf();

    BufferString numbstr( "", nrs_.atIndex(nr) );
    BufferString replstr;
    if ( zeropad_ < 2 )
	replstr = numbstr;
    else
    {
	const int numblen = numbstr.size();
	while ( numblen + replstr.size() < zeropad_ )
	    replstr += "0";
	replstr += numbstr;
    }

    static FileNameString ret; ret = fname_;
    replaceString( ret.buf(), "*", replstr.buf() );
    return ret.buf();
}


IOObj* SEGY::FileSpec::getIOObj( bool tmp ) const
{
    IOStream* iostrm;
    if ( tmp )
    {
	BufferString idstr( "100010.", IOObj::tmpID() );
	iostrm = new IOStream( fname_, idstr );
    }
    else
    {
	iostrm = new IOStream( fname_ );
	iostrm->acquireNewKey();
    }
    iostrm->setFileName( fname_ );
    iostrm->setGroup( "Seismic Data" );
    iostrm->setTranslator( "SEG-Y" );
    const bool ismulti = !mIsUdf(nrs_.start);
    if ( ismulti )
    {   
	iostrm->fileNumbers() = nrs_;
	iostrm->setZeroPadding( zeropad_ );
    }

    return iostrm;
}


void SEGY::FileSpec::fillPar( IOPar& iop ) const
{
    iop.set( sKey::FileName, fname_ );
    if ( mIsUdf(nrs_.start) )
	iop.removeWithKey( sKeyFileNrs );
    else
    {
	FileMultiString fms;
	fms += nrs_.start; fms += nrs_.stop; fms += nrs_.step;
	if ( zeropad_ )
	    fms += zeropad_;
	iop.set( sKeyFileNrs, fms );
    }
}


void SEGY::FileSpec::usePar( const IOPar& iop )
{
    iop.get( sKey::FileName, fname_ );
    getMultiFromString( iop.find(sKeyFileNrs) );
}


void SEGY::FileSpec::getReport( IOPar& iop, bool ) const
{
    iop.set( sKey::FileName, fname_ );
    if ( mIsUdf(nrs_.start) ) return;

    BufferString str;
    str += nrs_.start; str += "-"; str += nrs_.stop;
    str += " step "; str += nrs_.step;
    if ( zeropad_ )
	{ str += "(pad to "; str += zeropad_; str += " zeros)"; }
    iop.set( "Replace '*' with", str );
}


void SEGY::FileSpec::getMultiFromString( const char* str )
{
    FileMultiString fms( str );
    const int len = fms.size();
    nrs_.start = len > 0 ? atoi( fms[0] ) : mUdf(int);
    if ( len > 1 )
	nrs_.stop = atoi( fms[1] );
    if ( len > 2 )
	nrs_.step = atoi( fms[2] );
    if ( len > 3 )
	zeropad_ = atoi( fms[3] );
}


void SEGY::FileSpec::ensureWellDefined( IOObj& ioobj )
{
    mDynamicCastGet(IOStream*,iostrm,&ioobj)
    if ( !iostrm ) return;
    iostrm->setTranslator( "SEG-Y" );
    IOPar& iop = ioobj.pars();
    if ( !iop.find( sKey::FileName ) ) return;

    SEGY::FileSpec fs; fs.usePar( iop );
    iop.removeWithKey( sKey::FileName );
    iop.removeWithKey( sKeyFileNrs );

    iostrm->setFileName( fs.fname_ );
    if ( !fs.isMultiFile() )
	iostrm->fileNumbers().start = iostrm->fileNumbers().stop = 1;
    else
    {
	iostrm->fileNumbers() = fs.nrs_;
	iostrm->setZeroPadding( fs.zeropad_ );
    }
}


void SEGY::FileSpec::fillParFromIOObj( const IOObj& ioobj, IOPar& iop )
{
    mDynamicCastGet(const IOStream*,iostrm,&ioobj)
    if ( !iostrm ) return;

    SEGY::FileSpec fs; fs.fname_ = iostrm->fileName();
    if ( iostrm->isMulti() )
    {
	fs.nrs_ = iostrm->fileNumbers();
	fs.zeropad_ = iostrm->zeroPadding();
    }

    fs.fillPar( iop );
}


const char** SEGY::FilePars::getFmts( bool fr )
{
    return fr ? allsegyfmtoptions : allsegyfmtoptions+1;
}


void SEGY::FilePars::setForRead( bool fr )
{
    forread_ = fr;
    if ( !forread_ )
    {
	ns_ = 0;
	if ( fmt_ == 0 ) fmt_ = 1;
    }
}


void SEGY::FilePars::fillPar( IOPar& iop ) const
{
    if ( ns_ > 0 )
	iop.set( sKeyNrSamples, ns_ );
    else
	iop.removeWithKey( sKeyNrSamples );
    if ( fmt_ != 0 )
	iop.set( sKeyNumberFormat, nameOfFmt(fmt_,forread_) );
    else
	iop.removeWithKey( sKeyNumberFormat );
    iop.set( sKeyByteSwap, byteswap_ );
}


void SEGY::FilePars::usePar( const IOPar& iop )
{
    iop.get( sKeyNrSamples, ns_ );
    iop.get( sKeyByteSwap, byteswap_ );
    fmt_ = fmtOf( iop.find(sKeyNumberFormat), forread_ );
}


void SEGY::FilePars::getReport( IOPar& iop, bool ) const
{
    if ( ns_ > 0 )
	iop.set( "Number of samples used", ns_ );
    if ( fmt_ > 0 )
	iop.set( forread_ ? "SEG-Y 'format' used" : "SEG-Y 'format'",
		nameOfFmt(fmt_,forread_) );
    if ( byteswap_ )
    {
	const char* str = byteswap_ > 1
	    		? (forread_ ? "All bytes are" : "All bytes will be")
			: (forread_ ? "Data bytes are" : "Data bytes will be");
	iop.set( str, "swapped" );
    }
}


const char* SEGY::FilePars::nameOfFmt( int fmt, bool forread )
{
    const char** fmts = getFmts(true);
    if ( fmt > 0 && fmt < 4 )
	return fmts[fmt];
    if ( fmt == 5 )
	return fmts[4];
    if ( fmt == 8 )
	return fmts[5];

    return forread ? fmts[0] : nameOfFmt( 1, false );
}


int SEGY::FilePars::fmtOf( const char* str, bool forread )
{
    if ( !str || !*str || !isdigit(*str) )
	return forread ? 0 : 1;

    return (int)(*str - '0');
}


void SEGY::FileReadOpts::setGeomType( Seis::GeomType gt )
{
    geom_ = gt;
    if ( Seis::is2D(geom_) )
	icdef_ = XYOnly;
}

static int getICOpt( SEGY::FileReadOpts::ICvsXYType opt )
{
    return opt == SEGY::FileReadOpts::XYOnly ? -1
	: (opt == SEGY::FileReadOpts::ICOnly ? 1 : 0);
}
static SEGY::FileReadOpts::ICvsXYType getICType( int opt )
{
    return opt < 0 ? SEGY::FileReadOpts::XYOnly
	: (opt > 0 ? SEGY::FileReadOpts::ICOnly
	 	   : SEGY::FileReadOpts::Both);
}

#define mFillIf(cond,key,val) \
    if ( cond ) \
	iop.set( key, val ); \
    else \
	iop.removeWithKey( key )


void SEGY::FileReadOpts::fillPar( IOPar& iop ) const
{
    iop.set( sKeyICOpt, getICOpt( icdef_ ) );

    mFillIf(icdef_!=XYOnly,TrcHeaderDef::sInlByte,thdef_.inl);
    mFillIf(icdef_!=XYOnly,TrcHeaderDef::sInlByteSz,thdef_.inlbytesz);
    mFillIf(icdef_!=ICOnly,TrcHeaderDef::sXCoordByte,thdef_.xcoord);
    mFillIf(icdef_!=ICOnly,TrcHeaderDef::sYCoordByte,thdef_.ycoord);

    const bool is2d = Seis::is2D( geom_ );
    mFillIf(is2d,TrcHeaderDef::sTrNrByte,thdef_.trnr);
    mFillIf(is2d,TrcHeaderDef::sTrNrByteSz,thdef_.trnrbytesz);

    mFillIf(!mIsUdf(coordscale_),sKeyCoordScale,coordscale_);
    mFillIf(!mIsUdf(timeshift_),sKeyTimeShift,timeshift_);
    mFillIf(!mIsUdf(sampleintv_),sKeySampleIntv,sampleintv_);

    if ( !Seis::isPS(geom_) ) return;

    mFillIf(true,sKeyPSOpt,(int)psdef_);
    mFillIf(psdef_==UsrDef,sKeyOffsDef,offsdef_);
    mFillIf(psdef_==InFile,TrcHeaderDef::sOffsByte,thdef_.offs);
    mFillIf(psdef_==InFile,TrcHeaderDef::sOffsByteSz,thdef_.offsbytesz);
    mFillIf(psdef_==InFile,TrcHeaderDef::sAzimByte,thdef_.azim);
    mFillIf(psdef_==InFile,TrcHeaderDef::sAzimByteSz,thdef_.azimbytesz);

}


void SEGY::FileReadOpts::usePar( const IOPar& iop )
{
    thdef_.usePar( iop );
    int icopt = getICOpt( icdef_ );
    iop.get( sKeyICOpt, icopt );
    icdef_ = getICType( icopt );
    int psopt = (int)psdef_;
    iop.get( sKeyPSOpt, psopt );
    psdef_ = (PSDefType)psopt;
    iop.get( sKeyOffsDef, offsdef_ );
    iop.get( sKeyCoordScale, coordscale_ );
    iop.get( sKeyTimeShift, timeshift_ );
    iop.get( sKeySampleIntv, sampleintv_ );
}


static void setIntByte( IOPar& iop, const char* nm,
			unsigned char bytenr, unsigned char bytesz )
{
    BufferString keyw( nm ); keyw += " byte";
    BufferString val; val = (int)bytenr;
    val += " (size "; val += (int)bytesz; val += ")";
    iop.set( keyw.buf(), val.buf() );
}


void SEGY::FileReadOpts::getReport( IOPar& iop, bool rev1 ) const
{
    if ( !mIsUdf(coordscale_) )
	iop.set( sKeyCoordScale, coordscale_ );
    if ( !mIsUdf(timeshift_) )
	iop.set( sKeyTimeShift, timeshift_ );
    if ( !mIsUdf(sampleintv_) )
	iop.set( sKeySampleIntv, sampleintv_ );

    const bool is2d = Seis::is2D( geom_ );
    const bool isps = Seis::isPS( geom_ );

    if ( !rev1 )
    {
	if ( is2d )
	    setIntByte( iop, sKey::TraceNr, thdef_.trnr, thdef_.trnrbytesz );
	else
	{
	    iop.set( "Positioning defined by",
		    icdef_ == XYOnly ? "Coordinates" : "Inline/Crossline" );
	    if ( icdef_ != XYOnly )
	    {
		setIntByte( iop, "Inline", thdef_.inl, thdef_.inlbytesz );
		setIntByte( iop, "Crossline", thdef_.crl, thdef_.crlbytesz );
	    }
	    else
	    {
		iop.set( TrcHeaderDef::sXCoordByte, thdef_.xcoord );
		iop.set( TrcHeaderDef::sYCoordByte, thdef_.ycoord );
	    }
	}
    }

    if ( !isps )
	return;

    iop.set( "Offsets", psdef_ == UsrDef ? "User defined"
	    : (psdef_ == InFile ? "In file" : "Source/Receiver coordinates") );
    if ( psdef_ == UsrDef )
	iop.set( sKeyOffsDef, offsdef_ );
    else if ( psdef_ != SrcRcvCoords )
    {
	setIntByte( iop, sKey::Offset, thdef_.offs, thdef_.offsbytesz );
	if ( thdef_.azim < 255 )
	    setIntByte( iop, sKey::Azimuth, thdef_.azim, thdef_.azimbytesz );
    }
}
