/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Jul 2005
 RCS:		$Id: picksettr.cc,v 1.11 2007-03-29 15:08:14 cvskris Exp $
________________________________________________________________________

-*/

#include "picksetfact.h"
#include "pickset.h"
#include "ctxtioobj.h"
#include "binidvalset.h"
#include "ascstream.h"
#include "ioobj.h"
#include "iopar.h"
#include "ptrman.h"
#include "survinfo.h"
#include "streamconn.h"
#include "ioman.h"
#include "errh.h"
#include "keystrs.h"

static const char* sKeyMarkerType = "Marker Type";

mDefSimpleTranslatorioContext(PickSet,Loc)


int PickSetTranslatorGroup::selector( const char* key )
{
    int retval = defaultSelector( theInst().userName(), key );
    if ( retval ) return retval;

    if ( defaultSelector("Locations directory",key) ) return 1;

    return 0;
}


bool PickSetTranslator::retrieve( Pick::Set& ps, const IOObj* ioobj,
				  BufferString& bs )
{
    if ( !ioobj ) { bs = "Cannot find object in data base"; return false; }
    mDynamicCastGet(PickSetTranslator*,t,ioobj->getTranslator())
    if ( !t ) { bs = "Selected object is not a Pick Set"; return false; }
    PtrMan<PickSetTranslator> tr = t;
    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
        { bs = "Cannot open "; bs += ioobj->fullUserExpr(true); return false; }
    bs = tr->read( ps, *conn );
    return bs.isEmpty();
}


bool PickSetTranslator::store( const Pick::Set& ps, const IOObj* ioobj,
				BufferString& bs )
{
    if ( !ioobj ) { bs = "No object to store set in data base"; return false; }
    mDynamicCastGet(PickSetTranslator*,tr,ioobj->getTranslator())
    if ( !tr ) { bs = "Selected object is not a Pick Set"; return false; }

    bs = "";
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
        { bs = "Cannot open "; bs += ioobj->fullUserExpr(false); }
    else
	bs = tr->write( ps, *conn );
    delete tr;
    return bs.isEmpty();
}


const char* dgbPickSetTranslator::read( Pick::Set& ps, Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    ascistream astrm( ((StreamConn&)conn).iStream() );
    std::istream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot read from input file";
    if ( !astrm.isOfFileType(mTranslGroupName(PickSet)) )
	return "Input file is not a Pick Set";
    if ( atEndOfSection(astrm) ) astrm.next();

    float zfac = 1;
    if ( astrm.hasKeyword("Z Factor") )
    {
	zfac = astrm.getValue();
	if ( mIsZero(zfac,mDefEps) || mIsUdf(zfac) )
	    zfac = 1;
	astrm.next();
	if ( atEndOfSection(astrm) ) astrm.next();
    }

    if ( atEndOfSection(astrm) )
	return "Input file contains no pick sets";

    ps.setName( conn.ioobj ? (const char*)conn.ioobj->name() : "" );

    Pick::Location loc;
    // In old format we can find mulitple pick sets. Just gather them all
    // in the pick set
    for ( int ips=0; !atEndOfSection(astrm); ips++ )
    {
	astrm.next();
	if ( astrm.hasKeyword(sKey::Color) )
	{
	    ps.disp_.color_.use( astrm.value() );
	    astrm.next();
	}
	if ( astrm.hasKeyword(sKey::Size) )
	{
	    ps.disp_.pixsize_ = astrm.getVal();
	    astrm.next();
	}
	if ( astrm.hasKeyword(sKeyMarkerType) )
	{
	    ps.disp_.markertype_ = astrm.getVal();
	    astrm.next();
	}
	while ( !atEndOfSection(astrm) )
	{
	    if ( !loc.fromString( astrm.keyWord() ) )
		break;
	    loc.pos.z *= zfac;
	    ps += loc;
	    astrm.next();
	}
	while ( !atEndOfSection(astrm) ) astrm.next();
	astrm.next();
    }

    return ps.size() ? 0 : "No valid picks found";
}


const char* dgbPickSetTranslator::write( const Pick::Set& ps, Conn& conn )
{
    if ( !conn.forWrite() || !conn.isStream() )
	return "Internal error: bad connection";

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(PickSet) );
    std::ostream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot write to output Pick Set file";

    astrm.put( "Ref", ps.name() );
    BufferString buf;
    if ( ps.disp_.color_ != Color::NoColor )
    {
	ps.disp_.color_.fill( buf.buf() );
	astrm.put( sKey::Color, buf.buf() );
    }

    if ( ps.disp_.pixsize_ != 0 )
	astrm.put( sKey::Size, ps.disp_.pixsize_ );
    astrm.put( sKeyMarkerType, ps.disp_.markertype_ );

    for ( int iloc=0; iloc<ps.size(); iloc++ )
    {
	ps[iloc].toString( buf );
	strm << buf.buf() << '\n';
    }

    astrm.newParagraph();
    return strm.good() ? 0
	:  "Error during write to output Pick Set file";
}


void PickSetTranslator::createBinIDValueSets(
			const BufferStringSet& ioobjids,
			ObjectSet<BinIDValueSet>& bivsets )
{
    for ( int idx=0; idx<ioobjids.size(); idx++ )
    {
	BinIDValueSet* bs = new BinIDValueSet( 1, true );
	bivsets += bs;

	MultiID key( ioobjids.get( idx ) );
	const int setidx = Pick::Mgr().indexOf( key );
	const Pick::Set* ps = setidx < 0 ? 0 : &Pick::Mgr().get( setidx );
	Pick::Set* createdps = 0;
	if ( !ps )
	{
	    PtrMan<IOObj>ioobj = IOM().get( key );
	    BufferString msg;
	    if ( !ioobj )
	    {
		msg = "Cannot find PickSet with key "; msg += key;
		ErrMsg( msg ); continue;
	    }
	    ps = createdps = new Pick::Set;
	    if ( !retrieve(*createdps,ioobj,msg) )
		{ ErrMsg( msg ); delete createdps; continue; }
	}

	for ( int ipck=0; ipck<ps->size(); ipck++ )
	{
	    Pick::Location pl( (*ps)[ipck] );
	    bs->add( SI().transform(pl.pos), pl.pos.z );
	}

	delete createdps;
    }
}
