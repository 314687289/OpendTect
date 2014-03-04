/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "wellwriter.h"
#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "welldisp.h"
#include "ascstream.h"
#include "od_ostream.h"
#include "keystrs.h"
#include "envvars.h"

#define mGetOutStream(ext,nr,todo) \
        od_ostream strm( getFileName(ext,nr) ); \
    if ( !strm.isOK() ) { todo; }

Well::Writer::Writer( const char* f, const Well::Data& w )
	: Well::IO(f)
	, wd(w)
	, binwrlogs_(false)
{
}


bool Well::Writer::wrHdr( od_ostream& strm, const char* fileky ) const
{
    ascostream astrm( strm );
    if ( !astrm.putHeader(fileky) )
    {
	BufferString msg( "Cannot write to " );
	msg.add( fileky ).add( " file" );
	strm.addErrMsgTo( msg );
	ErrMsg( msg );
	return false;
    }
    return true;
}


bool Well::Writer::put() const
{
    return putInfoAndTrack()
	&& putLogs()
	&& putMarkers()
	&& putD2T()
	&& putCSMdl()
	&& putDispProps();
}


bool Well::Writer::putInfoAndTrack() const
{
    mGetOutStream( sExtWell(), 0, return false )
    return putInfoAndTrack( strm );
}


bool Well::Writer::putInfoAndTrack( od_ostream& strm ) const
{
    if ( !wrHdr(strm,sKeyWell()) ) return false;

    ascostream astrm( strm );
    astrm.put( Well::Info::sKeyuwid(), wd.info().uwid );
    astrm.put( Well::Info::sKeyoper(), wd.info().oper );
    astrm.put( Well::Info::sKeystate(), wd.info().state );
    astrm.put( Well::Info::sKeycounty(), wd.info().county );
    if ( wd.info().surfacecoord != Coord(0,0) )
	astrm.put( Well::Info::sKeycoord(), wd.info().surfacecoord.toString());
    astrm.put( Well::Info::sKeySRD(), wd.info().srdelev );
    astrm.put( Well::Info::sKeyreplvel(), wd.info().replvel );
    astrm.put( Well::Info::sKeygroundelev(), wd.info().groundelev );
    astrm.newParagraph();

    return putTrack( strm );
}


bool Well::Writer::putTrack( od_ostream& strm ) const
{
    for ( int idx=0; idx<wd.track().size(); idx++ )
    {
	const Coord3& c = wd.track().pos(idx);
	    // don't try to do the following in one statement
	    // (unless for educational purposes)
	strm << c.x << od_tab;
	strm << c.y << od_tab;
	strm << c.z << od_tab;
	strm << wd.track().dah(idx) << od_newline;
    }
    return strm.isOK();
}


bool Well::Writer::putTrack() const
{
    mGetOutStream( sExtTrack(), 0, return false )
    return putTrack( strm );
}


bool Well::Writer::putLogs() const
{
    for ( int idx=0; idx<wd.logs().size(); idx++ )
    {
	mGetOutStream( sExtLog(), idx+1, break )

	const Well::Log& wl = wd.logs().getLog(idx);
	if ( !putLog(strm,wl) )
	{
	    BufferString msg( "Could not write log: '", wl.name(), "'" );
	    strm.addErrMsgTo( msg );
	    ErrMsg( msg ); return false;
	}
    }

    return true;
}


bool Well::Writer::putLog( od_ostream& strm, const Well::Log& wl ) const
{
    if ( !wrHdr(strm,sKeyLog()) ) return false;

    ascostream astrm( strm );
    astrm.put( sKey::Name(), wl.name() );
    const bool haveunits = *wl.unitMeasLabel();
    const bool havepars = !wl.pars().isEmpty();
    if ( haveunits )
	astrm.put( Well::Log::sKeyUnitLbl(), wl.unitMeasLabel() );
    astrm.putYN( Well::Log::sKeyHdrInfo(), havepars );
    const char* stortyp = binwrlogs_ ? (__islittle__ ? "Binary" : "Swapped")
				     : "Ascii";
    astrm.put( Well::Log::sKeyStorage(), stortyp );
    astrm.newParagraph();
    if ( havepars )
	wl.pars().putTo( astrm );

    Interval<int> wrintv( 0, wl.size()-1 );
    float dah, val;
    for ( ; wrintv.start<wl.size(); wrintv.start++ )
    {
	dah = wl.dah(wrintv.start); val = wl.value(wrintv.start);
	if ( !mIsUdf(dah) && !mIsUdf(val) )
	    break;
    }
    for ( ; wrintv.stop>=0; wrintv.stop-- )
    {
	dah = wl.dah(wrintv.stop); val = wl.value(wrintv.stop);
	if ( !mIsUdf(dah) && !mIsUdf(val) )
	    break;
    }

    float v[2];
    for ( int idx=wrintv.start; idx<=wrintv.stop; idx++ )
    {
	v[0] = wl.dah( idx );
	if ( mIsUdf(v[0]) )
	    continue;

	v[1] = wl.value( idx );
	if ( binwrlogs_ )
	    strm.addBin( &v, sizeof(v) );
	else
	{
	    strm << v[0] << od_tab;
	    if ( mIsUdf(v[1]) )
		strm << sKey::FloatUdf();
	    else
		strm << v[1];

	    strm << od_newline;
	}
    }

    return strm.isOK();
}


bool Well::Writer::putMarkers() const
{
    mGetOutStream( sExtMarkers(), 0, return false )
    return putMarkers( strm );
}


bool Well::Writer::putMarkers( od_ostream& strm ) const
{
    if ( !wrHdr(strm,sKeyMarkers()) ) return false;

    ascostream astrm( strm );
    for ( int idx=0; idx<wd.markers().size(); idx++ )
    {
	BufferString basekey; basekey += idx+1;
	const Well::Marker& wm = *wd.markers()[idx];
	const float dah = wm.dah();
	if ( mIsUdf(dah) )
	    continue;

	astrm.put( IOPar::compKey(basekey,sKey::Name()), wm.name() );
	astrm.put( IOPar::compKey(basekey,Well::Marker::sKeyDah()), dah );
	astrm.put( IOPar::compKey(basekey,sKey::StratRef()), wm.levelID() );
	BufferString bs; wm.color().fill( bs );
	astrm.put( IOPar::compKey(basekey,sKey::Color()), bs );
    }

    return strm.isOK();
}


bool Well::Writer::putD2T() const	{ return doPutD2T( false ); }
bool Well::Writer::putCSMdl() const	{ return doPutD2T( true ); }
bool Well::Writer::doPutD2T( bool csmdl ) const
{
    if ( (csmdl && !wd.checkShotModel()) || (!csmdl && !wd.d2TModel()) )
	return true;

    mGetOutStream( csmdl ? sExtCSMdl() : sExtD2T(), 0, return false )
    return doPutD2T( strm, csmdl );
}


bool Well::Writer::putD2T( od_ostream& strm ) const
{ return doPutD2T( strm, false ); }
bool Well::Writer::putCSMdl( od_ostream& strm ) const
{ return doPutD2T( strm, true ); }
bool Well::Writer::doPutD2T( od_ostream& strm, bool csmdl ) const
{
    if ( !wrHdr(strm,sKeyD2T()) ) return false;

    ascostream astrm( strm );
    const Well::D2TModel& d2t = *(csmdl ? wd.checkShotModel(): wd.d2TModel());
    astrm.put( sKey::Name(), d2t.name() );
    astrm.put( sKey::Desc(), d2t.desc );
    astrm.put( D2TModel::sKeyDataSrc(), d2t.datasource );
    astrm.newParagraph();

    for ( int idx=0; idx<d2t.size(); idx++ )
    {
	const float dah = d2t.dah( idx );
	if ( mIsUdf(dah) )
	    continue;

	strm << dah << od_tab << d2t.t(idx) << od_newline;
    }

    return strm.isOK();
}


bool Well::Writer::putDispProps() const
{
    mGetOutStream( sExtDispProps(), 0, return false )
    return putDispProps( strm );
}


bool Well::Writer::putDispProps( od_ostream& strm ) const
{
    if ( !wrHdr(strm,sKeyDispProps()) ) return false;

    ascostream astrm( strm );
    IOPar iop;
    wd.displayProperties(true).fillPar( iop );
    wd.displayProperties(false).fillPar( iop );
    iop.putTo( astrm );
    return strm.isOK();
}

