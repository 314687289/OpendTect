/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: wellimpasc.cc,v 1.37 2006-11-03 13:28:36 cvsbert Exp $";

#include "wellimpasc.h"
#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "filegen.h"
#include "strmprov.h"
#include "unitofmeasure.h"
#include "survinfo.h"
#include <iostream>


inline static StreamData getSD( const char* fnm )
{
    StreamProvider sp( fnm );
    StreamData sd = sp.makeIStream();
    return sd;
}


Well::AscImporter::~AscImporter()
{
    unitmeasstrs_.deepErase();
}


#define mOpenFile(fnm) \
	StreamProvider sp( fnm ); \
	StreamData sd = sp.makeIStream(); \
	if ( !sd.usable() ) \
	    return "Cannot open input file"

const char* Well::AscImporter::getTrack( const char* fnm, bool tosurf, 
					 bool zinfeet )
{
    mOpenFile( fnm );

    Coord3 c, c0, prevc;
    Coord3 surfcoord;
    float dah = 0;
    const float zfac = zinfeet ? 0.3048 : 1;
    char buf[1024]; char valbuf[256];
    while ( *sd.istrm )
    {
	sd.istrm->getline( buf, 1024 );
	const char* ptr = getNextWord( buf, valbuf );
	c.x = atof( valbuf );
	ptr = getNextWord( ptr, valbuf );
	c.y = atof( valbuf );
	if ( !*ptr ) break;
	ptr = getNextWord( ptr, valbuf );
	c.z = atof( valbuf );
	c.z *= zfac;
	if ( c.distance(c0) < 1 ) break;

	if ( wd.track().size() == 0 )
	{
	    if ( !SI().isReasonable(wd.info().surfacecoord) )
		wd.info().surfacecoord = c;
	    if ( mIsUdf(wd.info().surfaceelev) )
		wd.info().surfaceelev = -c.z;

	    surfcoord.x = wd.info().surfacecoord.x;
	    surfcoord.y = wd.info().surfacecoord.y;
	    surfcoord.z = -wd.info().surfaceelev;

	    prevc = tosurf ? surfcoord : c;
	}

	ptr = getNextWord( ptr, valbuf );
	if ( *ptr )
	    dah = atof(valbuf) * zfac;
	else
	    dah += c.distance( prevc );

	wd.track().addPoint( c, c.z, dah );
	prevc = c;
    }

    sd.close();
    return wd.track().size() ? 0 : "No valid well track points found";
}


const char* Well::AscImporter::getD2T( const char* fnm, bool istvd, bool istwt,
				       bool zinfeet )
{
    mOpenFile( fnm );
    std::istream& strm = *sd.istrm;

    if ( !wd.d2TModel() )
	wd.setD2TModel( new Well::D2TModel );
    Well::D2TModel& d2t = *wd.d2TModel();
    d2t.erase();

    const float zfac = zinfeet ? 0.3048 : 1;
    float z, tval, prevdah = mUdf(float);
    bool firstpos = true;
    bool t_in_ms = false;
    TypeSet<float> tms; TypeSet<float> dahs;
    while ( strm )
    {
	strm >> z >> tval;
	if ( !strm ) break;
	if ( mIsUdf(z) ) continue;
	z *= zfac;

	if ( istvd )
	{
	    z = wd.track().getDahForTVD( z, prevdah );
	    if ( mIsUdf(z) ) continue;
	    prevdah = z;
	}

	tms += istwt ? tval : 2*tval;
	dahs += z;
    }
    sd.close();
    if ( !tms.size() ) return "No valid Depth/Time points found";

    const bool t_in_sec = tms[tms.size()-1] < 2 * SI().zRange(false).stop;

    for ( int idx=0; idx<tms.size(); idx++ )
	d2t.add( dahs[idx], t_in_sec ? tms[idx] : tms[idx] * 0.001 );

    d2t.deInterpolate();
    return 0;
}


const char* Well::AscImporter::getMarkers( const char* fnm, bool istvd, 
					   bool zinfeet )
{
    mOpenFile( fnm );
    std::istream& strm = *sd.istrm;
    const float zfac = zinfeet ? 0.3048 : 1;
    float z, prevdah = mUdf(float);
#   define mBufSz 128
    char buf[mBufSz];
    while ( strm )
    {
	strm >> z;
	if ( !strm ) break;
	strm.getline( buf, mBufSz );
	char* ptr = buf; skipLeadingBlanks(ptr); removeTrailingBlanks(ptr);
	if ( mIsUdf(z) || !*ptr ) continue;
	z *= zfac;

	if ( istvd )
	{
	    z = wd.track().getDahForTVD( z, prevdah );
	    if ( mIsUdf(z) ) continue;
	    prevdah = z;
	}

	Well::Marker* newmrk = new Well::Marker( ptr );
	newmrk->dah = z;
	wd.markers() += newmrk;
    }
    sd.close();

    return 0;
}



const char* Well::AscImporter::getLogInfo( const char* fnm,
					   LasFileInfo& lfi ) const
{
    mOpenFile( fnm );
    const char* res = getLogInfo( *sd.istrm, lfi );
    sd.close();
    return res;
}

#define mIsKey(s) caseInsensitiveEqual(keyw,s,0)
#define mErrRet(s) { lfi.depthcolnr = -1; return s; }

const char* Well::AscImporter::getLogInfo( std::istream& strm,
					   LasFileInfo& lfi ) const
{
    convs_.allowNull();
    convs_.erase();

    char linebuf[4096]; char wordbuf[64];
    const char* ptr;
    char section = '-';
    lfi.depthcolnr = -1;
    int colnr = 0;

    while ( strm )
    {
	strm.getline( linebuf, 4096 );
	ptr = linebuf; skipLeadingBlanks(ptr);
	if ( *ptr == '#' || *ptr == '\0' ) continue;

	if ( *ptr == '~' )
	{
	    section = *(++ptr);
	    if ( section == 'A' ) break;
	    continue;
	}
	else if ( section == '-' )
	{
	    // This is not LAS really, just one line of header and then go
	    skipLeadingBlanks(ptr);
	    while ( *ptr )
	    {
		ptr = getNextWord( ptr, wordbuf );
		skipLeadingBlanks(ptr);
		char* unstr = strchr( wordbuf, '(' );
		if ( unstr )
		{
		    *unstr++ = '\0';
		    char* closeparptr = strchr( unstr, ')' );
		    if ( closeparptr ) *closeparptr = '\0';
		}
		if ( lfi.depthcolnr < 0 && matchStringCI("dept",wordbuf) )
		    lfi.depthcolnr = colnr;
		else
		{
		    if ( colnr == 1 && ( !strcmp(wordbuf,"in:")
				    || isdigit(wordbuf[0]) || wordbuf[0] == '+'
				    || wordbuf[1] == '-' || wordbuf[2] == '.' ))
			mErrRet( "Invalid LAS-like file" )

		    lfi.lognms += new BufferString( wordbuf );
		}
		convs_ += UnitOfMeasure::getGuessed( unstr );
		unitmeasstrs_.add( unstr );
		colnr++;
	    }
	    break;
	}

	char* keyw = const_cast<char*>(ptr);
	char* val1; char* val2; char* info;
	parseHeader( keyw, val1, val2, info );

	switch ( section )
	{
	case 'C':

	    if ( lfi.depthcolnr < 0 && (mIsKey("dept") || mIsKey("depth")) )
		lfi.depthcolnr = colnr;
	    else
	    {
		BufferString lognm( info );
		if ( lognm == "" )
		    lognm = keyw;
		else if ( *lognm.buf() >= '0' && *lognm.buf() <= '9' )
		{
		    // Leading curve number. Remove it.
		    BufferString newnm( lognm );
		    char* newptr = (char*)getNextWord( newnm.buf(), wordbuf );
		    if ( newptr && *newptr )
			{ skipLeadingBlanks(newptr); }
		    if ( newptr && *newptr )
			lognm = newptr;
		}
		if ( matchString("Name = ",lognm.buf()) )
		{
		    // Possible HRS 'encoding'. Awful for user display.
		    BufferString newnm = lognm.buf() + 7;
		    char* newptr = strstr( newnm, " - Type = " );
		    if ( !newptr )
			lognm = newnm;
		    else
		    {
			*newptr = '\0'; lognm = newnm; lognm += " (";
			newptr += 10; lognm += newptr; lognm += ")";
		    }
		}
		lfi.lognms += new BufferString( lognm );
	    }

	    colnr++;
	    convs_ += UnitOfMeasure::getGuessed( val1 );
	    unitmeasstrs_.add( val1 );

	break;
	case 'W':
	    if ( mIsKey("STRT") )
		lfi.zrg.start = atof(val2);
	    if ( mIsKey("STOP") )
		lfi.zrg.stop = atof(val2);
	    if ( mIsKey("NULL") )
		lfi.undefval = atof( val1 );
	    if ( mIsKey("WELL") )
	    {
		lfi.wellnm = val1;
		if ( val2 ) { lfi.wellnm += " "; lfi.wellnm += val2; }
	    }
	break;
	default:
	break;
	}
    }

    if ( !convs_.size() )
	mErrRet( "Could not any valid log in file" )

    lfi.zrg.sort();
    const UnitOfMeasure* unmeas = convs_[lfi.depthcolnr];
    if ( unmeas )
    {
	lfi.zunitstr = unmeas->symbol();
	if ( !mIsUdf(lfi.zrg.start) )
	    lfi.zrg.start = unmeas->internalValue(lfi.zrg.start);
	if ( !mIsUdf(lfi.zrg.stop) )
	    lfi.zrg.stop = unmeas->internalValue(lfi.zrg.stop);
    }

    if ( !strm.good() )
	mErrRet( "Only header found; No data" )
    else if ( lfi.lognms.size() < 1 )
	mErrRet( "No logs present" )
    else if ( lfi.depthcolnr < 0 )
	mErrRet( "'DEPTH' not present in file" )

    return 0;
}


void Well::AscImporter::parseHeader( char* startptr, char*& val1, char*& val2,
				     char*& info ) const
{
    val1 = 0; val2 = 0; info = "";
    char* ptr = strchr( startptr, '.' );
    if ( ptr ) *ptr++ = '\0';
    removeTrailingBlanks( startptr );
    if ( !ptr ) return;

    info = strchr( ptr, ':' );
    if ( info )
    {
	*info++ = '\0';
	skipLeadingBlanks(info); removeTrailingBlanks(info);
    }

    skipLeadingBlanks( ptr );
    val1 = ptr;
    while ( *ptr && !isspace(*ptr) ) ptr++;
    val2 = ptr;
    if ( *ptr )
    {
	*val2++ = '\0';
	skipLeadingBlanks(val2); removeTrailingBlanks(val2);
    }
}


const char* Well::AscImporter::getLogs( const char* fnm, const LasFileInfo& lfi,
					bool istvd )
{
    mOpenFile( fnm );
    const char* res = getLogs( *sd.istrm, lfi, istvd );
    sd.close();
    return res;
}


const char* Well::AscImporter::getLogs( std::istream& strm,
					const LasFileInfo& lfi, bool istvd )
{
    LasFileInfo inplfi;
    const char* res = getLogInfo( strm, inplfi );
    if ( res )
	return res;
    if ( lfi.lognms.size() == 0 )
	return "No logs selected";
    if ( inplfi.depthcolnr < 0 )
	return "Input file is invalid";

    if ( lfi.depthcolnr < 0 )
	const_cast<LasFileInfo&>(lfi).depthcolnr = inplfi.depthcolnr;
    const int addstartidx = wd.logs().size();
    BoolTypeSet issel( inplfi.lognms.size(), false );

    for ( int idx=0; idx<inplfi.lognms.size(); idx++ )
    {
	const int colnr = idx + (idx >= lfi.depthcolnr ? 1 : 0);
	const BufferString& lognm = inplfi.lognms.get(idx);
	const bool ispresent = indexOf( lfi.lognms, lognm ) >= 0;
	if ( !ispresent )
	    continue;

	issel[idx] = true;
	Well::Log* newlog = new Well::Log( lognm );
	BufferString unlbl;
	if ( convs_[colnr] )
	{
	    if ( useconvs_ )
		unlbl = "Converted to SI from ";
	    unlbl += unitmeasstrs_.get( colnr );
	}
	newlog->setUnitMeasLabel( unlbl );
	wd.logs().add( newlog );
    }

    return getLogData( strm, issel, lfi, istvd, addstartidx,
	    		inplfi.lognms.size() + 1 );
}


const char* Well::AscImporter::getLogData( std::istream& strm,
	const BoolTypeSet& issel, const LasFileInfo& lfi,
	bool istvd, int addstartidx, int totalcols )
{
    Interval<float> reqzrg;
    assign( reqzrg, lfi.zrg );
    const bool havestart = !mIsUdf(reqzrg.start);
    const bool havestop = !mIsUdf(reqzrg.stop);
    if ( havestart && havestop )
	reqzrg.sort();

    float prevdpth = mUdf(float);
    int nradded = 0;
    while ( true )
    {
	TypeSet<float> vals;
	bool atend = false;
	float val;
	for ( int icol=0; icol<totalcols; icol++ )
	{
	    strm >> val;
	    if ( strm.fail() || (icol<totalcols-1 && strm.eof()) )
		{ atend = true; break; }
	    if ( mIsEqual(val,lfi.undefval,mDefEps) )
		val = mUdf(float);
	    else if ( useconvs_ && convs_[icol] )
		val = convs_[icol]->internalValue( val );
	    vals += val;
	}
	if ( atend )
	    break;

	float dpth = vals[ lfi.depthcolnr ];
	if ( mIsUdf(dpth) )
	    continue;

	if ( convs_[lfi.depthcolnr] )
	    dpth = convs_[lfi.depthcolnr]->internalValue( dpth );

	if ( (havestart && dpth < reqzrg.start - mDefEps)
	  || mIsEqual(prevdpth,dpth,mDefEps) )
	    continue;
	else if ( havestop && dpth > reqzrg.stop + mDefEps )
	    break;


	TypeSet<float> selvals;
	for ( int icol=0; icol<totalcols; icol++ )
	{
	    if ( icol != lfi.depthcolnr && issel[icol] )
		selvals += vals[icol];
	}
	if ( !selvals.size() ) continue;

	const float z = istvd ? wd.track().getDahForTVD( dpth, prevdpth )
	    		      : dpth;
	for ( int idx=0; idx<selvals.size(); idx++ )
	    wd.logs().getLog(addstartidx+idx).addValue( z, selvals[idx] );

	nradded++; prevdpth = dpth;
    }

    if ( nradded == 0 )
	return "No matching log data found";

    wd.logs().updateDahIntvs();
    return 0;
}
