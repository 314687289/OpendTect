/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Feb 2004
-*/

static const char* rcsID = "$Id: seisscanner.cc,v 1.26 2006-02-17 17:27:14 cvsbert Exp $";

#include "seisscanner.h"
#include "seisinfo.h"
#include "seisread.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "cubesampling.h"
#include "posgeomdetector.h"
#include "strmprov.h"
#include "sorting.h"
#include "oddirs.h"
#include "ioobj.h"
#include "iopar.h"
#include "stats.h"
#include "conn.h"
#include "errh.h"
#include "math2.h"

#ifndef mac
#include <values.h>
#endif


SeisScanner::SeisScanner( const IOObj& ioobj, int mtr )
    	: Executor( "Scan seismic volume file(s)" )
    	, reader(*new SeisTrcReader(&ioobj))
    	, geomdtector(*new PosGeomDetector(
		    	!SeisTrcTranslator::is2D(ioobj,false)))
	, trc(*new SeisTrc)
    	, chnksz(10)
    	, totalnr(mtr < 0 ? -2 : mtr)
    	, maxnrtrcs(mtr)
{
    is3d = geomdtector.is3D();
    init();
    Stat_initRandom(0);
}


SeisScanner::~SeisScanner()
{
    delete &trc;
    delete &geomdtector;
    delete &reader;
}


void SeisScanner::init()
{
    geomdtector.reInit();
    curmsg = "Scanning";
    nrnulltraces = 0;
    nrdistribvals = nrcrlsthisline = expectedcrls = 0;
    invalidsamplenr = -1;
    nonnullsamplerg.start = INT_MAX;
    nonnullsamplerg.stop = 0;
    invalidsamplebid.inl = mUdf(int);
    valrg.start = mUdf(float);
    first_trace = true;
}


const char* SeisScanner::message() const
{
    return *reader.errMsg() ? reader.errMsg() : curmsg.buf();
}


const char* SeisScanner::nrDoneText() const
{
    return "Traces handled";
}


int SeisScanner::nrDone() const
{
    return geomdtector.nrpositions;
}


int SeisScanner::totalNr() const
{
    if ( totalnr == -2 )
    {
	totalnr = -1;
	if ( reader.ioObj() )
	{
	    CubeSampling cs;
	    if ( SeisTrcTranslator::getRanges(*reader.ioObj(),cs) )
	    {
		totalnr = cs.hrg.nrInl() * cs.hrg.nrCrl();
		((SeisScanner*)this)->expectedcrls = cs.hrg.nrCrl();
	    }
	}
    }
    return totalnr;
}


bool SeisScanner::getSurvInfo( CubeSampling& cs, Coord crd[3] ) const
{
    const char* msg = geomdtector.getSurvInfo( cs.hrg, crd );
    if ( msg )
	{ curmsg = msg; return false; }

    cs.zrg.start = sampling.atIndex( nonnullsamplerg.start );
    cs.zrg.stop = sampling.atIndex( nonnullsamplerg.stop );
    cs.zrg.step = sampling.step;

    return true;
}


void SeisScanner::report( IOPar& iopar ) const
{
    iopar.clear();
    if ( !reader.ioObj() ) { iopar.setName( "No scan executed" ); return; }

    BufferString str = "Report for "; str += reader.ioObj()->translator();
    str += is3d ? " cube '" : " line set '";
    str += reader.ioObj()->name(); str += "'\n\n";
    iopar.setName( str );

    iopar.add( "->", "Sampling info" );
    iopar.set( "Z step", sampling.step );
    iopar.set( "Z start in file", sampling.start );
    iopar.set( "Z stop in file", zRange().stop );

    iopar.set( "Number of samples in file", (int)nrsamples );
    if ( nonnullsamplerg.start != 0 )
	iopar.set( "First non-zero sample", nonnullsamplerg.start + 1 );
    if ( nonnullsamplerg.stop != nrsamples-1 )
	iopar.set( "Last non-zero sample", nonnullsamplerg.stop + 1 );

    iopar.add( "->", "Global stats" );
    iopar.set( "Number of null traces", (int)nrnulltraces );
    geomdtector.report( iopar );
    if ( is3d )
    {
	CubeSampling cs; Coord crd[3];
	getSurvInfo(cs,crd);
	iopar.set( "Z.start", cs.zrg.start );
	iopar.set( "Z.stop", cs.zrg.stop );
	iopar.set( "Z.step", cs.zrg.step );
    }

    if ( !Values::isUdf(valrg.start) )
    {
	iopar.add( "->", "Data values" );
	iopar.set( "Minimum value", valrg.start );
	iopar.set( "Maximum value", valrg.stop );
	iopar.set( "Median value", distribvals[nrdistribvals/2] );
	iopar.set( "1/4 value", distribvals[nrdistribvals/4] );
	iopar.set( "3/4 value", distribvals[3*nrdistribvals/4] );
	if ( !Values::isUdf(invalidsamplebid.inl) )
	{
	    iopar.set( "First invalid value at", invalidsamplebid );
	    iopar.set( "First invalid value sample number", invalidsamplenr );
	}
	iopar.add( "0.1% clip range", getClipRgStr(0.1) );
	iopar.add( "0.2% clip range", getClipRgStr(0.2) );
	iopar.add( "0.3% clip range", getClipRgStr(0.3) );
	iopar.add( "0.5% clip range", getClipRgStr(0.5) );
	iopar.add( "1% clip range", getClipRgStr(1) );
	iopar.add( "1.5% clip range", getClipRgStr(1.5) );
	iopar.add( "2% clip range", getClipRgStr(2) );
	iopar.add( "3% clip range", getClipRgStr(3) );
	iopar.add( "5% clip range", getClipRgStr(5) );
	iopar.add( "10% clip range", getClipRgStr(10) );
	iopar.add( "20% clip range", getClipRgStr(20) );
	iopar.add( "30% clip range", getClipRgStr(30) );
	iopar.add( "50% clip range", getClipRgStr(50) );
	iopar.add( "90% clip range", getClipRgStr(90) );
    }
}


const char* SeisScanner::getClipRgStr( float pct ) const
{
    const float ratio = nrdistribvals * .005 * pct;
    int idx0 = mNINT(ratio);
    int idx1 = nrdistribvals - idx0 - 1;
    if ( idx0 > idx1 ) Swap( idx0, idx1 );

    static BufferString ret;
    ret = distribvals[idx0]; ret += " - "; ret += distribvals[idx1];
    return ret.buf();
}


const char* SeisScanner::defaultUserInfoFile( const char* trnm )
{
    static BufferString ret;
    ret = GetProcFileName( "scan_seis" );
    if ( trnm && *trnm )
	{ ret += "_"; ret += trnm; }
    if ( GetSoftwareUser() )
	{ ret += "_"; ret += GetSoftwareUser(); }
    ret += ".txt";
    return ret.buf();
}


void SeisScanner::launchBrowser( const char* fnm ) const
{
    if ( !fnm || !*fnm )
	fnm = defaultUserInfoFile( reader.ioObj()
				 ? reader.ioObj()->translator() : "" );
    IOPar iopar; report( iopar );
    iopar.dump( fnm, "_pretty" );

    BufferString nospcfname( fnm );
    replaceCharacter( nospcfname.buf(), ' ', (char)128 );
    BufferString cmd( "@" ); cmd += mGetExecScript();
    cmd += " FileBrowser "; cmd += nospcfname;
    StreamProvider strmprov( cmd );
    if ( !strmprov.executeCommand(false) )
    {
	BufferString s( "Failed to submit command '" );
	s += strmprov.command(); s += "'";
	ErrMsg( s );
    }
}


int SeisScanner::nextStep()
{
    if ( *reader.errMsg() )
	return Executor::ErrorOccurred;

    for ( int itrc=0; itrc<chnksz; itrc++ )
    {
	int res = reader.get( trc.info() );
	if ( res < 1 )
	{
	    curmsg = "Done";
	    if ( res != 0 )
	    {
		curmsg = "Error during read of trace header after ";
		if ( geomdtector.atFirstPos() )
		    curmsg += "opening file";
		else if ( is3d )
		{
		    curmsg += geomdtector.prevbid.inl; curmsg += "/";
		    curmsg += geomdtector.prevbid.crl;
		}
		else
		{
		    curmsg += "trace number ";
		    curmsg += geomdtector.prevbid.crl;
		}
	    }
	    wrapUp();
	    return res;
	}
	if ( res > 1 ) continue;

	if ( !reader.get( trc ) )
	{
	    curmsg = "Error during read of trace data at ";
	    if ( is3d )
	    {
		curmsg += trc.info().binid.inl; curmsg += "/";
		curmsg += trc.info().binid.crl;
	    }
	    else
	    {
		curmsg += "trace number ";
		curmsg += trc.info().binid.crl;
	    }
	    wrapUp();
	    return Executor::ErrorOccurred;
	}

	if ( doValueWork() )
	{
	    if ( first_trace )
		handleFirstTrc();
	    else
		handleTrc();
	}

	if ( maxnrtrcs > -1 && geomdtector.nrpositions >= maxnrtrcs )
	    return Executor::Finished;
    }

    return Executor::MoreToDo;
}


void SeisScanner::wrapUp()
{
    reader.close();
    sort_array( distribvals, nrdistribvals );
}


void SeisScanner::handleFirstTrc()
{
    first_trace = false;
    sampling = trc.info().sampling;
    nrsamples = trc.size();
    nrcrlsthisline = 1;
    geomdtector.add( trc.info().binid, trc.info().coord, trc.info().nr );
}


void SeisScanner::handleTrc()
{
    if ( geomdtector.add(trc.info().binid,trc.info().coord,trc.info().nr)
	    		!= mInlChange )
	nrcrlsthisline++;
    else
    {
	if ( expectedcrls > 0 && expectedcrls != nrcrlsthisline )
	{
	    totalnr -= expectedcrls - nrcrlsthisline;
	    if ( totalnr < nrDone() )
		totalnr = nrDone() + expectedcrls;
	}
	nrcrlsthisline = 1;
    }
}


bool SeisScanner::doValueWork()
{
    const bool adddistribvals = nrdistribvals < mMaxNrDistribVals;
    float thresh = 1. / (1. + 0.01 * geomdtector.nrpositions);
    const bool selected_trc = !adddistribvals
			   && Stat_getRandom() < 0.01;
    unsigned int selsieve = (unsigned int)(1. + 0.01 * geomdtector.nrpositions);
    if ( selsieve > 10 ) selsieve = 10;
    float sievethresh = 1. / selsieve;

    bool nonnull_seen = false;
    int nullstart = trc.size();
    for ( int idx=nullstart-1; idx!=-1; idx-- )
    {
	float val = trc.get(idx,0);
	if ( !mIsZero(val,mDefEps) ) break;
	nullstart = idx;
    }
   if ( nullstart-1 > nonnullsamplerg.stop )
       nonnullsamplerg.stop = nullstart - 1;

    bool needinitvalrg = Values::isUdf(valrg.start);
    for ( int idx=0; idx<nullstart; idx++ )
    {
	float val = trc.get(idx,0);
	bool iszero = mIsZero(val,mDefEps);
	if ( !nonnull_seen )
	{
	   if ( iszero ) continue;
	   else if ( nonnullsamplerg.start > idx )
	       nonnullsamplerg.start = idx;
	   nonnull_seen = true;
	}

	if ( !IsNormalNumber(val) || val > 1e30 || val < -1e30 )
	{
	    if ( invalidsamplenr < 0 )
	    {
		invalidsamplenr = idx;
		invalidsamplebid = trc.info().binid;
	    }
	    continue;
	}

	if ( !needinitvalrg )
	    valrg.include( val );
	else
	{
	    valrg.start = valrg.stop = val;
	    needinitvalrg = false;
	}

	if ( nrdistribvals < mMaxNrDistribVals )
	    distribvals[nrdistribvals++] = val;
	else if ( selected_trc && Stat_getRandom() < sievethresh )
	{
	    int arrnr = Stat_getIndex(nrdistribvals);
	    distribvals[ arrnr ] = val;
	}
    }

    if ( !nonnull_seen )
    {
	nrnulltraces++;
	return false;
    }
    return true;
}
