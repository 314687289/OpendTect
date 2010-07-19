/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: wellimpasc.cc,v 1.74 2010-07-19 09:36:08 cvsnageswara Exp $";

#include "wellimpasc.h"
#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "strmprov.h"
#include "unitofmeasure.h"
#include "tabledef.h"
#include <iostream>


static bool convToDah( const Well::Track& trck, float& val,
			float prev=mUdf(float) )
{
    const Interval<float> trckzrg( trck.pos(0).z - 1e-6,
	    			 trck.pos(trck.size()-1).z + 1e-6 );
    if ( !trckzrg.includes(val) )
	return false;

    val = trck.getDahForTVD( val, prev );
    return !mIsUdf(val);
}


inline static StreamData getSD( const char* fnm )
{
    StreamProvider sp( fnm );
    StreamData sd = sp.makeIStream();
    return sd;
}


Well::LASImporter::~LASImporter()
{
    unitmeasstrs_.erase();
}


#define mOpenFile(fnm) \
	StreamProvider sp( fnm ); \
	StreamData sd = sp.makeIStream(); \
	if ( !sd.usable() ) \
	    return "Cannot open input file"


const char* Well::LASImporter::getLogInfo( const char* fnm,
					   FileInfo& lfi ) const
{
    mOpenFile( fnm );
    const char* res = getLogInfo( *sd.istrm, lfi );
    sd.close();
    return res;
}

#define mIsKey(s) caseInsensitiveEqual(keyw,s,0)
#define mErrRet(s) { lfi.depthcolnr = -1; return s; }

const char* Well::LASImporter::getLogInfo( std::istream& strm,
					   FileInfo& lfi ) const
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
	ptr = linebuf; mSkipBlanks(ptr);
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
	    mSkipBlanks(ptr);
	    while ( *ptr )
	    {
		ptr = getNextWord( ptr, wordbuf );
		mSkipBlanks(ptr);
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
		if ( *lognm.buf() >= '0' && *lognm.buf() <= '9' )
		{
		    // Leading curve number. Remove it.
		    BufferString newnm( lognm );
		    char* newptr = (char*)getNextWord( newnm.buf(), wordbuf );
		    if ( newptr && *newptr )
			{ mSkipBlanks(newptr); }
		    if ( newptr && *newptr )
			lognm = newptr;
		}
		if ( matchString("Name = ",lognm.buf()) )
		{
		    // Possible HRS 'encoding'. Awful for user display.
		    BufferString newnm =  lognm.buf() + 7;
		    char* newptr = strstr( newnm.buf(), " - Type = " );
		    if ( !newptr )
			lognm = newnm;
		    else
		    {
			*newptr = '\0'; lognm = newnm; lognm += " (";
			newptr += 10; lognm += newptr; lognm += ")";
		    }
		}
		if ( lognm.isEmpty() )
		    lognm = keyw;
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

    if ( convs_.isEmpty() )
	mErrRet( "Could not find any valid log in file" )
    if ( lfi.depthcolnr < 0 )
	mErrRet( "Could not find a depth column ('DEPT' or 'DEPTH')" )

    lfi.revz = lfi.zrg.start > lfi.zrg.stop;
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

    return 0;
}


void Well::LASImporter::parseHeader( char* startptr, char*& val1, char*& val2,
				     char*& info ) const
{
    val1 = 0; val2 = 0; info = 0;
    char* ptr = strchr( startptr, '.' );
    if ( ptr ) *ptr++ = '\0';
    removeTrailingBlanks( startptr );
    if ( !ptr ) return;

    info = strchr( ptr, ':' );
    if ( info )
    {
	*info++ = '\0';
	mTrimBlanks(info);
    }

    mSkipBlanks( ptr );
    val1 = ptr;
    mSkipNonBlanks( ptr );
    val2 = ptr;
    if ( *ptr )
    {
	*val2++ = '\0';
	mTrimBlanks(val2);
    }
}


const char* Well::LASImporter::getLogs( const char* fnm, const FileInfo& lfi,
					bool istvd )
{
    mOpenFile( fnm );
    const char* res = getLogs( *sd.istrm, lfi, istvd );
    sd.close();
    return res;
}


const char* Well::LASImporter::getLogs( std::istream& strm,
					const FileInfo& lfi, bool istvd )
{
    FileInfo inplfi;
    const char* res = getLogInfo( strm, inplfi );
    if ( res )
	return res;
    if ( lfi.lognms.size() == 0 )
	return "No logs selected";
    if ( inplfi.depthcolnr < 0 )
	return "Input file is invalid";

    if ( lfi.depthcolnr < 0 )
	const_cast<FileInfo&>(lfi).depthcolnr = inplfi.depthcolnr;
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


const char* Well::LASImporter::getLogData( std::istream& strm,
	const BoolTypeSet& issel, const FileInfo& lfi,
	bool istvd, int addstartidx, int totalcols )
{
    Interval<float> reqzrg( Interval<float>().setFrom( lfi.zrg ) );
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

	const bool afterstop = havestop && dpth > reqzrg.stop + mDefEps;
	const bool beforestart = havestart && dpth < reqzrg.start - mDefEps;
	if ( (lfi.revz && beforestart) || (!lfi.revz && afterstop) )
	    break;
	if ( beforestart || afterstop || mIsEqual(prevdpth,dpth,mDefEps) )
	    continue;

	TypeSet<float> selvals;
	for ( int icol=0; icol<totalcols; icol++ )
	{
	    if ( icol == lfi.depthcolnr )
		continue;

	    const int valnr = icol - (icol > lfi.depthcolnr ? 1 : 0);
	    if ( icol != lfi.depthcolnr && issel[valnr] )
		selvals += vals[icol];
	}
	if ( selvals.isEmpty() ) continue;

	float z = dpth;
	if ( istvd && !convToDah(wd.track(),z,prevdpth) )
	    continue;

	for ( int idx=0; idx<selvals.size(); idx++ )
	    wd.logs().getLog(addstartidx+idx).addValue( z, selvals[idx] );

	nradded++; prevdpth = dpth;
    }

    if ( nradded == 0 )
	return "No matching log data found";

    wd.logs().updateDahIntvs();
    return 0;
}


Table::FormatDesc* Well::TrackAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "WellTrack" );
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true );
    fd->bodyinfos_ += Table::TargetInfo::mkZPosition( true );
    Table::TargetInfo* ti = new Table::TargetInfo( "MD", FloatInpSpec(),
	   					   Table::Optional );
    ti->setPropertyType( PropertyRef::Dist );
    ti->selection_.unit_ = UnitOfMeasure::surveyDefDepthUnit();
    fd->bodyinfos_ += ti;
    return fd;
}


bool Well::TrackAscIO::getData( Well::Data& wd, bool tosurf ) const
{
    if ( !getHdrVals(strm_) )
	return false;

    Coord3 c, c0, prevc;
    Coord3 surfcoord;
    float dah = 0;
    
    char buf[1024]; char valbuf[256];
    const bool isxy = fd_.bodyinfos_[0]->selection_.form_ == 0;

    while ( true )
    {
	const int ret = getNextBodyVals( strm_ );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	c.x = getdValue(0); c.y = getdValue(1);
	if ( !isxy && !mIsUdf(c.x) && !mIsUdf(c.y) )
	{
	    Coord wc( SI().transform( BinID( mNINT(c.x), mNINT(c.y) ) ) );
	    c.x = wc.x; c.y = wc.y;
	}
	c.z = getfValue(2);
	if ( !c.isDefined() )
	    continue;

	if ( c.distTo(c0) < 1 ) break;

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

	float newdah = getfValue( 3 );
	if ( mIsUdf(newdah) )
	    dah += c.distTo( prevc );
	else
	    dah = newdah;

	wd.track().addPoint( c, c.z, dah );
	prevc = c;
    }

    return wd.track().size();
}


static Table::TargetInfo* gtDepthTI( bool withuns )
{
    Table::TargetInfo* ti = new Table::TargetInfo( "Depth", FloatInpSpec(),
	   					   Table::Required );
    if ( withuns )
    {
	ti->setPropertyType( PropertyRef::Dist );
	ti->selection_.unit_ = UnitOfMeasure::surveyDefDepthUnit();
    }

    ti->form(0).setName( "MD" );
    ti->add( new Table::TargetInfo::Form( "TVDSS", FloatInpSpec() ) );
    return ti;
}


Table::FormatDesc* Well::MarkerSetAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "MarkerSet" );

    fd->bodyinfos_ += gtDepthTI( true );

#define mAddNmSpec(nm,typ) \
    fd->bodyinfos_ += new Table::TargetInfo(nm,StringInpSpec(),Table::typ)
    mAddNmSpec( "Marker name", Required );
    mAddNmSpec( "Nm p2", Hidden );
    mAddNmSpec( "Nm p3", Hidden );
    mAddNmSpec( "Nm p4", Hidden );

    return fd;
}


bool Well::MarkerSetAscIO::get( std::istream& strm, Well::MarkerSet& ms,
       				const Well::Track& trck ) const
{
    ms.erase();

    const int dpthcol = columnOf( false, 0, 0 );
    const int nmcol = columnOf( false, 1, 0 );
    if ( nmcol > dpthcol )
    {
	// We'll assume that the name occupies up to 4 words
	for ( int icol=nmcol+1; icol<5; icol++ )
	{
	    if ( icol == dpthcol ) break;

	    if ( fd_.bodyinfos_[icol-nmcol+1]->selection_.elems_.isEmpty() )
		fd_.bodyinfos_[icol-nmcol+1]->selection_.elems_ +=
		    Table::TargetInfo::Selection::Elem( RowCol(0,icol), 0 );
	    else
		fd_.bodyinfos_[icol-nmcol+1]->selection_.elems_[0].pos_.col
		    = icol;
	}
    }

    while ( true )
    {
	int ret = getNextBodyVals( strm );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	float dah = getfValue( 0 );
	BufferString namepart = text( 1 );
	if ( mIsUdf(dah) || namepart.isEmpty() )
	    continue;
	if ( formOf(false,0) == 1 && !convToDah(trck,dah) )
	    continue;

	BufferString fullnm( namepart );
	for ( int icol=nmcol+1; ; icol++ )
	{
	    if ( icol == dpthcol ) break;
	    namepart = text( icol );
	    if ( namepart.isEmpty() ) break;

	    fullnm += " "; fullnm += namepart;
	}
	ms += new Well::Marker( fullnm, dah );
    }

    return true;
}


Table::FormatDesc* Well::D2TModelAscIO::getDesc( bool withunitfld )
{
    Table::FormatDesc* fd = new Table::FormatDesc( "DepthTimeModel" );
    fd->headerinfos_ +=
	new Table::TargetInfo( "Undefined Value", StringInpSpec(sKey::FloatUdf),
				Table::Required );
    createDescBody( fd, withunitfld );
    return fd;
}


void Well::D2TModelAscIO::createDescBody( Table::FormatDesc* fd,
					  bool withunits )
{
    fd->bodyinfos_ += gtDepthTI( withunits );

    Table::TargetInfo* ti = new Table::TargetInfo( "Time", FloatInpSpec(),
	    			Table::Required, PropertyRef::Time );
    ti->form(0).setName( "TWT" );
    ti->add( new Table::TargetInfo::Form( "One-way TT", FloatInpSpec() ) );
    fd->bodyinfos_ += ti;
}


void Well::D2TModelAscIO::updateDesc( Table::FormatDesc& fd, bool withunitfld )
{
    fd.bodyinfos_.erase();
    createDescBody( &fd, withunitfld );
}


bool Well::D2TModelAscIO::get( std::istream& strm, Well::D2TModel& d2t,
       				const Well::Track& trck ) const
{
    d2t.erase();
    if ( trck.isEmpty() ) return true;

    while ( true )
    {
	int ret = getNextBodyVals( strm );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	float dah = getfValue( 0 );
	float time = getfValue( 1 );
	if ( mIsUdf(dah) || mIsUdf(time) )
	    continue;
	if ( formOf(false,0) == 1 && !convToDah(trck,dah) )
	    continue;
	
	if ( formOf(false,1) == 1 )
	    time *= 2;

	d2t.add( dah, time );
    }

    return true;
}
