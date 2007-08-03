/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2001
 * FUNCTION : CBVS I/O
-*/

static const char* rcsID = "$Id: pickset.cc,v 1.45 2007-08-03 09:48:17 cvsraman Exp $";

#include "pickset.h"
#include "survinfo.h"
#include "multiid.h"
#include "ioman.h"
#include "separstr.h"
#include "tabledef.h"
#include <ctype.h>


Pick::Location::~Location()
{
    if ( text ) delete text;
}


void Pick::Location::operator=( const Pick::Location& pl )
{
    pos = pl.pos;
    dir = pl.dir;
    if ( pl.text )
    {
	if ( !text )
	    text = new BufferString( *pl.text );
	else
	    *text = *pl.text;
    }
}


void Pick::Location::setText( const char* key, const char* txt )
{
    unSetText( key );
    if ( !text ) text = new BufferString;
    SeparString sepstr( *text, '\'' );

    sepstr.add( key );
    sepstr.add( txt );
    *text = sepstr;
}


void Pick::Location::unSetText( const char* key )
{
    if ( !text ) return;
    SeparString sepstr( *text, '\'' );
    for ( int idx=0; idx<sepstr.size(); idx+=2 )
    {
	if ( strcmp(key,sepstr[idx]) )
	    continue;

	SeparString copy( 0, '\'' );
	for ( int idy=0; idy<sepstr.size(); idy++ )
	{
	    if ( idy==idx || idy==idx+1 )
		continue;

	    copy.add( sepstr[idy] );
	}

	sepstr = copy;
	idx-=2;
    }

    (*text) = sepstr;
}


static double getNextVal( char*& str )
{
    if ( !*str ) return mUdf(double);
    char* endptr = str;
    while ( *endptr && !isspace(*endptr) ) endptr++;
    if ( *endptr ) *endptr++ = '\0';
    double v = atof( str );
    str = endptr; skipLeadingBlanks(str);
    return v;
}


bool Pick::Location::fromString( const char* s, bool doxy )
{
    if ( !s || !*s ) return false;

    if ( *s == '"' )
    {
	s++;
	text = new BufferString( s );
	char* start = text->buf();
	char* stop = strchr( start, '"' );
	if ( !stop )
	{
	    delete text;
	    text = 0;
	}
	else
	{
	    *stop = '\0';
	    s += stop - start + 1;
	}
    }

    BufferString bufstr( s );
    char* str = bufstr.buf();
    skipLeadingBlanks(str);

    double xread = getNextVal( str );
    double yread = getNextVal( str );
    double zread = getNextVal( str );
    if ( mIsUdf(zread) )
	return false;

    pos.x = xread;
    pos.y = yread;
    pos.z = zread;

    // Check if data is in inl/crl rather than X and Y
    if ( !SI().isReasonable(pos) || !doxy )
    {
	BinID bid( mNINT(pos.x), mNINT(pos.y) );
	SI().snap( bid, BinID(0,0) );
	Coord newpos = SI().transform( bid );
	if ( SI().isReasonable(newpos) )
	    pos.x = newpos.x; pos.y = newpos.y;
    }

    // See if there's a direction, too
    xread = getNextVal( str );
    yread = getNextVal( str );
    if ( !mIsUdf(yread) )
    {
	zread = getNextVal( str );
	if ( mIsUdf(zread) ) zread = 0;
	dir = Sphere( xread, yread, zread );
    }

    return true;
}


void Pick::Location::toString(BufferString& str) const
{
    if ( text )
    {
	str = "\"";
	str += text->buf();
	str += "\"";
	str += "\t";
	str += getStringFromDouble(0,pos.x);
    }
    else
	str = getStringFromDouble(0,pos.x);

    str += "\t"; str += getStringFromDouble(0,pos.y);
    str += "\t"; str += getStringFromDouble(0,pos.z);

    if ( hasDir() )
    {
	str += "\t"; str += getStringFromDouble(0,dir.radius);
	str += "\t"; str += getStringFromDouble(0,dir.theta);
	str += "\t"; str += getStringFromDouble(0,dir.phi);
    }
}


bool Pick::Location::getText( const char* idkey, BufferString& val ) const
{
    if ( !text )
    {
	val = "";
	return false;
    }
    
    SeparString sepstr( *text, '\'' );
    const int strsz = sepstr.size();
    if ( !strsz ) return false;

    for ( int idx=0; idx<strsz; idx+=2 )
    {
	if ( strcmp(idkey,sepstr[idx]) )
	    continue;

	val = sepstr[idx+1];
	return true;
    }

    return false;
}


Pick::SetMgr& Pick::SetMgr::getMgr( const char* nm )
{
    static ObjectSet<Pick::SetMgr>* mgrs = 0;
    Pick::SetMgr* newmgr = 0;
    if ( !mgrs )
    {
	mgrs = new ObjectSet<Pick::SetMgr>;
	newmgr = new Pick::SetMgr( 0 );
	    // ensure the first mgr has the 'empty' name
    }
    else if ( (!nm || !*nm) )
	return *((*mgrs)[0]);
    else
    {
	for ( int idx=1; idx<mgrs->size(); idx++ )
	{
	    if ( (*mgrs)[idx]->name() == nm )
		return *((*mgrs)[idx]);
	}
    }

    if ( !newmgr )
	newmgr = new Pick::SetMgr( nm );
    *mgrs += newmgr;
    IOM().surveyChanged.notify( mCB(newmgr,Pick::SetMgr,survChg) );
    IOM().entryRemoved.notify( mCB(newmgr,Pick::SetMgr,objRm) );
    return *newmgr;
}


Pick::SetMgr::SetMgr( const char* nm )
    : NamedObject(nm)
    , locationChanged(this), setToBeRemoved(this)
    , setAdded(this), setChanged(this)
    , setDispChanged(this)
{}


void Pick::SetMgr::add( const MultiID& ky, Set* st )
{
    pss_ += st; ids_ += ky; changed_ += false;
    setAdded.trigger( st );
}


void Pick::SetMgr::set( const MultiID& ky, Set* newset )
{
    Set* oldset = find( ky );
    if ( !oldset )
    {
	if ( newset )
	    add( ky, newset );
    }
    else if ( newset != oldset )
    {
	const int idx = pss_.indexOf( oldset );
	//Must be removed from list before trigger, otherwise
	//other users may remove it in calls invoded by the cb.
	pss_.remove( idx ); 
	setToBeRemoved.trigger( oldset );
	delete oldset; 
	ids_.remove( idx );
	changed_.remove( idx );
	if ( newset )
	    add( ky, newset );
    }
}


const MultiID& Pick::SetMgr::id( int idx ) const
{ return ids_[idx]; }


void Pick::SetMgr::setID( int idx, const MultiID& mid )
{
    ids_[idx] = mid;
}


int Pick::SetMgr::indexOf( const Pick::Set& st ) const
{
    return pss_.indexOf( &st );
}


int Pick::SetMgr::indexOf( const MultiID& ky ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( ky == ids_[idx] )
	    return idx;
    }
    return -1;
}


int Pick::SetMgr::indexOf( const char* nm ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( pss_[idx]->name() == nm )
	    return idx;
    }
    return -1;
}


Pick::Set* Pick::SetMgr::find( const MultiID& ky ) const
{
    const int idx = indexOf( ky );
    return idx < 0 ? 0 : const_cast<Pick::Set*>( pss_[idx] );
}


MultiID* Pick::SetMgr::find( const Pick::Set& st ) const
{
    const int idx = indexOf( st );
    return idx < 0 ? 0 : const_cast<MultiID*>( &ids_[idx] );
}


Pick::Set* Pick::SetMgr::find( const char* nm ) const
{
    const int idx = indexOf( nm );
    return idx < 0 ? 0 : const_cast<Pick::Set*>( pss_[idx] );
}


void Pick::SetMgr::reportChange( CallBacker* sender, const ChangeData& cd )
{
    const int setidx = pss_.indexOf( cd.set_ );
    if ( setidx >= 0 )
    {
	changed_[setidx] = true;
	locationChanged.trigger( const_cast<ChangeData*>( &cd ), sender );
    }
}


void Pick::SetMgr::reportChange( CallBacker* sender, const Set& s )
{
    const int setidx = pss_.indexOf( &s );
    if ( setidx >= 0 )
    {
	changed_[setidx] = true;
	setChanged.trigger( const_cast<Pick::Set*>(&s), sender );
    }
}


void Pick::SetMgr::reportDispChange( CallBacker* sender, const Set& s )
{
    const int setidx = pss_.indexOf( &s );
    if ( setidx >= 0 )
    {
	changed_[setidx] = true;
	setDispChanged.trigger( const_cast<Pick::Set*>(&s), sender );
    }
}


void Pick::SetMgr::removeCBs( CallBacker* cb )
{
    locationChanged.removeWith( cb );
    setToBeRemoved.removeWith( cb );
    setAdded.removeWith( cb );
    setChanged.removeWith( cb );
    setDispChanged.removeWith( cb );
}


void Pick::SetMgr::survChg( CallBacker* )
{
    locationChanged.cbs.erase();
    setToBeRemoved.cbs.erase();
    setAdded.cbs.erase();
    setChanged.cbs.erase();
    setDispChanged.cbs.erase();
    deepErase( pss_ );
    ids_.erase();
    changed_.erase();
}


void Pick::SetMgr::objRm( CallBacker* cb )
{
    mCBCapsuleUnpack(MultiID,ky,cb);
    if ( indexOf(ky) >= 0 )
	set( ky, 0 );
}


Table::FormatDesc* PickSetAscIO::getDesc( bool isxy, bool iszreq )
{
    Table::FormatDesc* fd = new Table::FormatDesc( "PickSet" );
    fd->bodyinfos_ += new Table::TargetInfo( isxy ? "X Coordinate" : "Inl No.", 
				 FloatInpSpec(), Table::Required );
    fd->bodyinfos_ += new Table::TargetInfo( isxy ? "Y Coordinate" : "Crl No.",
				 FloatInpSpec(), Table::Required );
    if ( iszreq )
	fd->bodyinfos_ += new Table::TargetInfo( "Z Values", FloatInpSpec(), 
				 Table::Required, PropertyRef::surveyZType() );

    return fd;
}

#define mErrRet(s) { if ( s ) errmsg_ = s; return 0; }

bool PickSetAscIO::get( std::istream& strm, Pick::Set& ps, bool isxy, 
			bool iszreq, const float constz ) const
{
    while ( true )
    {
	int ret = getNextBodyVals( strm );
	if ( ret < 0 ) mErrRet(0)
	if ( ret == 0 ) break;

	const float xread = getfValue( 0 );
	const float yread = getfValue( 1 );
	if ( mIsUdf(xread) || mIsUdf(yread) ) continue;

	Coord pos( xread, yread );
	if ( !isxy || !SI().isReasonable(pos) )
	{
	    BinID bid( mNINT(xread), mNINT(yread) );
	    SI().snap( bid );
	    pos = SI().transform( bid );
	}

	if ( !SI().isReasonable(pos) ) continue;
	
	const float zread = iszreq ? getfValue( 2 ) : constz;
	Pick::Location ploc( pos, mIsUdf(zread) ? 0 : zread );
	ps += ploc;
    }

    return true;
}

