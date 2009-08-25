/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Nov 2006
-*/

static const char* rcsID = "$Id: posimpexppars.cc,v 1.2 2009-08-25 13:25:36 cvsbert Exp $";

#include "posimpexppars.h"
#include "survinfo.h"
#include "keystrs.h"
#include "ioman.h"
#include "iopar.h"


const char* PosImpExpPars::sKeyOffset()		{ return sKey::Offset; }
const char* PosImpExpPars::sKeyScale()		{ return sKey::Scale; }
const char* PosImpExpPars::sKeyTrcNr()		{ return sKey::TraceNr; }


const PosImpExpPars& PosImpExpPars::SVY()
{
    static PosImpExpPars* thinst = 0;
    if ( !thinst )
    {
	thinst = new PosImpExpPars;
	thinst->getFromSI();
	IOM().afterSurveyChange.notify( mCB(thinst,PosImpExpPars,survChg) );
    }
    return *thinst;
}

PosImpExpPars& PosImpExpPars::getSVY()
{
    return const_cast<PosImpExpPars&>( SVY() );
}


void PosImpExpPars::getFromSI()
{
    clear();
    usePar( SI().pars() );
}


const char* PosImpExpPars::fullKey( const char* attr, bool scl )
{
    static BufferString ret;
    ret = IOPar::compKey( sKeyBase(), attr );
    ret = IOPar::compKey( ret.buf(), scl ? sKeyScale() : sKeyOffset() );
    return ret.buf();
}


void PosImpExpPars::usePar( const IOPar& iop )
{
    iop.get( fullKey(sKeyBinID(),true), binidscale_ );
    iop.get( fullKey(sKeyBinID(),false), binidoffs_ );
    iop.get( fullKey(sKeyTrcNr(),true), trcnrscale_ );
    iop.get( fullKey(sKeyTrcNr(),false), trcnroffs_ );
    iop.get( fullKey(sKeyCoord(),true), coordscale_ );
    iop.get( fullKey(sKeyCoord(),false), coordoffs_ );
    iop.get( fullKey(sKeyZ(),true), zscale_ );
    iop.get( fullKey(sKeyZ(),false), zoffs_ );
    iop.get( fullKey(sKeyOffset(),true), offsscale_ );
    iop.get( fullKey(sKeyOffset(),false), offsoffs_ );
}


#define mChkUdf(v) if ( mIsUdf(v) ) return


void PosImpExpPars::adjustBinID( BinID& bid, bool inw ) const
{
    mChkUdf(bid.inl); mChkUdf(bid.crl);
    if ( inw )
    {
	bid.inl *= binidscale_; bid.crl *= binidscale_;
	bid += binidoffs_;
    }
    else
    {
	bid -= binidoffs_;
	bid.inl /= binidscale_; bid.crl /= binidscale_;
    }
}


void PosImpExpPars::adjustTrcNr( int& tnr, bool inw ) const
{
    mChkUdf(tnr);
    if ( inw )
	{ tnr *= trcnrscale_; tnr += trcnroffs_; }
    else
	{ tnr -= trcnroffs_; tnr /= trcnrscale_; }
}


void PosImpExpPars::adjustCoord( Coord& crd, bool inw ) const
{
    mChkUdf(crd.x); mChkUdf(crd.y);
    if ( inw )
    {
	crd.x *= coordscale_; crd.y *= coordscale_;
	crd += coordoffs_;
    }
    else
    {
	crd -= coordoffs_;
	crd.x /= coordscale_; crd.y /= coordscale_;
    }
}


void PosImpExpPars::adjustZ( float& z, bool inw ) const
{
    mChkUdf(z);
    if ( inw )
	{ z *= zscale_; z += zoffs_; }
    else
	{ z -= zoffs_; z /= zscale_; }
}


void PosImpExpPars::adjustOffset( float& offs, bool inw ) const
{
    mChkUdf(offs);
    if ( inw )
	{ offs *= offsscale_; offs += offsoffs_; }
    else
	{ offs -= offsoffs_; offs /= offsscale_; }
}


void PosImpExpPars::adjustInl( int& inl, bool inw ) const
{
    mChkUdf(inl);
    if ( inw )
	{ inl *= binidscale_; inl += binidoffs_.inl; }
    else
	{ inl -= binidoffs_.inl; inl /= binidscale_; }
}


void PosImpExpPars::adjustCrl( int& crl, bool inw ) const
{
    mChkUdf(crl);
    if ( inw )
	{ crl *= binidscale_; crl += binidoffs_.crl; }
    else
	{ crl -= binidoffs_.crl; crl /= binidscale_; }
}


void PosImpExpPars::adjustX( double& x, bool inw ) const
{
    mChkUdf(x);
    if ( inw )
	{ x *= coordscale_; x += coordoffs_.x; }
    else
	{ x -= coordoffs_.x; x /= coordscale_; }
}


void PosImpExpPars::adjustY( double& y, bool inw ) const
{
    mChkUdf(y);
    if ( inw )
	{ y *= coordscale_; y += coordoffs_.y; }
    else
	{ y -= coordoffs_.y; y /= coordscale_; }
}
