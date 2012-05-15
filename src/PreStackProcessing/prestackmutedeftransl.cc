/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Nov 2006
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: prestackmutedeftransl.cc,v 1.13 2012-05-15 06:13:21 cvskris Exp $";

#include "prestackmutedeftransl.h"

#include "ascstream.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "keystrs.h"
#include "mathfunc.h"
#include "prestackmutedef.h"
#include "ptrman.h"
#include "odver.h"
#include "streamconn.h"

static const char* sKeyPBMFSetup = "PBMF setup";

defineTranslatorGroup(MuteDef,"Mute Definition");
defineTranslator(dgb,MuteDef,mDGBKey);

mDefSimpleTranslatorioContext(MuteDef,Misc)


int MuteDefTranslatorGroup::selector( const char* key )
{
    return defaultSelector( theInst().userName(), key );
}


bool MuteDefTranslator::retrieve( PreStack::MuteDef& md, const IOObj* ioobj,
				  BufferString& bs )
{
    if ( !ioobj ) { bs = "Cannot find object in data base"; return false; }
    mDynamicCastGet(MuteDefTranslator*,t,ioobj->getTranslator())
    if ( !t ) { bs = "Selected object is not a Mute Definition"; return false; }
    PtrMan<MuteDefTranslator> tr = t;
    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
        { bs = "Cannot open "; bs += ioobj->fullUserExpr(true); return false; }
    bs = tr->read( md, *conn );
    return bs.isEmpty();
}


bool MuteDefTranslator::store( const PreStack::MuteDef& md, const IOObj* ioobj,
				BufferString& bs )
{
    if ( !ioobj ) { bs = "No object to store set in data base"; return false; }
    mDynamicCastGet(MuteDefTranslator*,tr,ioobj->getTranslator())
    if ( !tr ) { bs = "Selected object is not a Mute Definition"; return false;}

    bs = "";
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
        { bs = "Cannot open "; bs += ioobj->fullUserExpr(false); }
    else
    {
	bs = tr->write( md, *conn );
    }

    delete tr;
    return bs.isEmpty();
}


const char* dgbMuteDefTranslator::read( PreStack::MuteDef& md, Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    ascistream astrm( ((StreamConn&)conn).iStream() );
    std::istream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot read from input file";
    if ( !astrm.isOfFileType(mTranslGroupName(MuteDef)) )
	return "Input file is not a Mute Definition file";

    const bool hasiopar = hasIOPar( astrm.majorVersion(),
				    astrm.minorVersion() );

    if ( hasiopar )
    {
	IOPar pars( astrm );
	MultiID hormid;
	pars.get( sKeyRefHor(), hormid );
	md.setReferenceHorizon( hormid );
    }

    if ( atEndOfSection(astrm) ) astrm.next();
    if ( atEndOfSection(astrm) )
	return "Input file contains no Mute Definition locations";

    while ( md.size() ) md.remove( 0 );
    md.setName( conn.ioobj ? (const char*)conn.ioobj->name() : "" );

    for ( int ifn=0; !atEndOfSection(astrm); ifn++ )
    {
	if ( astrm.hasKeyword(sKeyRefHor()) )
	{
	    MultiID hormid = astrm.value();
	    md.setReferenceHorizon( hormid );
	    astrm.next();
	}

	BinID bid;
	bool extrapol = true;
	bool rejectpt = false;
	PointBasedMathFunction::InterpolType it =PointBasedMathFunction::Linear;

	if ( astrm.hasKeyword(sKey::Position) )
	{
	    bid.use( astrm.value() );
	    if ( !bid.inl || !bid.crl )
		rejectpt = true;

	    astrm.next();
	}
	if ( astrm.hasKeyword(sKeyPBMFSetup) )
	{
	    const char* val = astrm.value();
	    if ( *val )
	    {
		it = (*val == 'S' ? PointBasedMathFunction::Snap
		   : (*val == 'P' ? PointBasedMathFunction::Poly
			    	  : PointBasedMathFunction::Linear));
		extrapol = *(val+1) != 'N';
	    }
	    astrm.next();
	}

	const PointBasedMathFunction::ExtrapolType et = extrapol
	    ? PointBasedMathFunction::EndVal
	    : PointBasedMathFunction::None;
	PointBasedMathFunction* fn = new PointBasedMathFunction( it, et );
	while ( !atEndOfSection(astrm) )
	{
	    BufferString val( astrm.keyWord() );
	    char* ptrx = val.buf();
	    mSkipBlanks(ptrx);
	    char* ptrz = ptrx;
	    mSkipNonBlanks( ptrz );
	    if ( !*ptrz )
		{ astrm.next(); continue; }
	    *ptrz = '\0'; ptrz++;
	    mSkipBlanks(ptrz);
	    if ( !*ptrz )
		{ astrm.next(); continue; }

	    float x = toFloat( ptrx ); float z = toFloat( ptrz );
	    if ( !mIsUdf(x) && !mIsUdf(z) )
		fn->add( x, z );

	    astrm.next();
	}

	if ( rejectpt || fn->size()<1 )
	    delete fn;
	else
	    md.add( fn, bid );

	astrm.next();
    }

    if ( md.size() )
    {
	md.setChanged( false );
	return 0;
    }

    return "No valid mute points found";
}


bool dgbMuteDefTranslator::hasIOPar(int majorversion, int minorversion )
{
    if ( majorversion<3 )
	return false;
    if ( majorversion>4 )
	return true;

    return minorversion>3;
}


const char* dgbMuteDefTranslator::write( const PreStack::MuteDef& md,Conn& conn)
{
    if ( !conn.forWrite() || !conn.isStream() )
	return "Internal error: bad connection";

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(MuteDef) );

    const bool hasiopar = hasIOPar( mODMajorVersion, mODMinorVersion );

    if ( hasiopar )
    {
	IOPar pars;
	pars.set( sKeyRefHor(), md.getReferenceHorizon() );
	pars.putTo( astrm );
	astrm.newParagraph();
    }

    std::ostream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot write to output Mute Definition file";

    for ( int imd=0; imd<md.size(); imd++ )
    {
	if ( !imd && !hasiopar )
	    astrm.put( sKeyRefHor(), md.getReferenceHorizon() );

	char buf[80]; md.getPos(imd).fill( buf );
	astrm.put( sKey::Position, buf );
	const PointBasedMathFunction& pbmf = md.getFn( imd );
	buf[0] =  pbmf.interpolType() == PointBasedMathFunction::Snap
	 ? 'S' : (pbmf.interpolType() == PointBasedMathFunction::Poly
	 ? 'P' : 'L');
	buf[1] = pbmf.extrapolate() ? 'Y' : 'N';
	buf[2] = '\0';
	astrm.put( sKeyPBMFSetup, buf );

	for ( int idx=0; idx<pbmf.size(); idx++ )
	    strm << pbmf.xVals()[idx] << '\t' << pbmf.yVals()[idx] << '\n';

	astrm.newParagraph();
    }

    if ( strm.good() )
    {
	const_cast<PreStack::MuteDef&>(md).setChanged( false );
	return 0;
    }

    return "Error during write to output Mute Definition file";
}
