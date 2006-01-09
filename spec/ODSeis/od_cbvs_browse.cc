/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 2000
 * RCS      : $Id: od_cbvs_browse.cc,v 1.25 2006-01-09 10:25:44 cvsbert Exp $
-*/

static const char* rcsID = "$Id: od_cbvs_browse.cc,v 1.25 2006-01-09 10:25:44 cvsbert Exp $";

#include "seistrc.h"
#include "seiscbvs.h"
#include "cbvsinfo.h"
#include "cbvsreadmgr.h"
#include "conn.h"
#include "iostrm.h"
#include "filegen.h"
#include "filepath.h"
#include "datachar.h"
#include "strmprov.h"
#include "ptrman.h"
#include <iostream>
#include <math.h>
#include <ctype.h>

#include "prog.h"
#include "seisfact.h"


static void getInt( int& i )
{
    char buf[80];

    char* ptr;
    do
    {
	ptr = buf;
	std::cin.getline( ptr, 80 );
	while ( *ptr && isspace(*ptr) ) ptr++;
    }
    while ( ! *ptr );

    char* endptr = ptr + 1;
    while ( *endptr && !isspace(*endptr) ) endptr++;
    *endptr = '\0';
    i = atoi( ptr );
}


int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
	std::cerr << "Usage: " << argv[0] << " cbvs_file" << std::endl;
	ExitProgram( 1 );
    }

    FilePath fp( argv[1] );
    
    if ( !File_exists(fp.fullPath()) )
    {
        std::cerr << fp.fullPath() << " does not exist" << std::endl;
        ExitProgram( 1 );
    }
    
    if ( !fp.isAbsolute() )
    {
        fp.insert( File_getCurrentDir() );
    }

    BufferString fname=fp.fullPath();

    PtrMan<CBVSSeisTrcTranslator> tri = CBVSSeisTrcTranslator::getInstance();
    if ( !tri->initRead( new StreamConn(fname,Conn::Read) ) )
	{ std::cerr << tri->errMsg() << std::endl;  ExitProgram( 1 ); }

    std::cerr << "\n";
    const CBVSReadMgr& mgr = *tri->readMgr();
    mgr.dumpInfo( std::cerr, true );
    const CBVSInfo& info = mgr.info();
    const int singinl = info.geom.start.inl == info.geom.stop.inl
			? info.geom.start.inl : -999;

    SeisTrc trc; BinID bid( singinl, 0 );
    BinID step( abs(info.geom.step.inl), abs(info.geom.step.crl) );
    StepInterval<int> samps;
    const int nrcomps = info.compinfo.size();
    while ( true )
    {
	if ( singinl == -999 )
	{
	    std::cerr << "\nExamine In-line (" << info.geom.start.inl
		<< "-" << info.geom.stop.inl;
	    if ( step.inl > 1 )
		std::cerr << " [" << step.inl << "]";
	    int stopinl = info.geom.start.inl == 0 ? -1 : 0;
	    std::cerr << ", " << stopinl << " to stop): ";
	    getInt( bid.inl );
	    if ( bid.inl == stopinl ) break;
	}

	if ( info.geom.fullyrectandreg )
	{
	    if ( bid.inl < info.geom.start.inl || bid.inl > info.geom.stop.inl )
	    {
		std::cerr << "Invalid inline" << std::endl;
		continue;
	    }
	}
	else
	{
	    const PosInfo::LineData* inlinf = info.geom.getInfoFor( bid.inl );
	    if ( !inlinf )
	    {
		std::cerr << "This inline is not present in the cube"
		    	  << std::endl;
		continue;
	    }
	    std::cerr << "Xline range available: ";
	    for ( int idx=0; idx<inlinf->segments.size(); idx++ )
	    {
		std::cerr << inlinf->segments[idx].start << " - "
		     << inlinf->segments[idx].stop;
		if ( idx < inlinf->segments.size()-1 )
		    std::cerr << " and ";
	    }
	    std::cerr << std::endl;
	}

	if ( singinl == -999 )
	    std::cerr << "X-line (0 for new in-line): ";
	else
	    std::cerr << "X-line or trace number (0 to stop): ";
	getInt( bid.crl );
	if ( bid.crl == 0 )
	{
	    if ( singinl == -999 )
		continue;
	    else
		break;
	}

	if ( !tri->goTo( bid ) )
	    { std::cerr << "Cannot find this position" << std::endl; continue; }
	if ( !tri->read(trc) )
	    { std::cerr << "Cannot read trace!" << std::endl; continue; }

	if ( !mIsZero(trc.info().pick,mDefEps)
		&& !mIsUndefined(trc.info().pick) )
	    std::cerr << "Pick position: " << trc.info().pick << std::endl;
	if ( !mIsZero(trc.info().refpos,mDefEps)
		&& !mIsUndefined(trc.info().refpos) )
	    std::cerr << "Reference position: " << trc.info().refpos
		      << std::endl;
	if ( !mIsZero(trc.info().offset,mDefEps)
		&& !mIsUndefined(trc.info().offset) )
	    std::cerr << "Offset: " << trc.info().offset << std::endl;
	if ( !mIsZero(trc.info().azimuth,mDefEps)
		&& !mIsUndefined(trc.info().azimuth) )
	    std::cerr << "Azimuth: " << (trc.info().azimuth*57.29577951308232)
		      << std::endl;
	if ( !mIsZero(trc.info().coord.x,0.1) )
	{
	    BufferString str; trc.info().coord.fill(str.buf());
	    std::cerr << "Coordinate: " << str;
	    BinID bid = info.geom.b2c.transformBack( trc.info().coord );
	    if ( bid != trc.info().binid )
	    {
		bid.fill( str.buf() );
		std::cerr << " --> " << str;
	    }
	    std::cerr << std::endl;
	}

	while ( true )
	{
	    std::cerr << "Print samples ( 1 - " << trc.size() << " )."
		      << std::endl;
	    std::cerr << "From (0 to stop): ";
	    getInt( samps.start );
	    if ( samps.start < 1 ) break;

	    std::cerr << "To: "; getInt( samps.stop );
	    std::cerr << "Step: "; getInt( samps.step );
	    if ( samps.step < 1 ) samps.step = 1;
	    if ( samps.start < 1 ) samps.start = 1;
	    if ( samps.stop > trc.size() ) samps.stop = trc.size();
	    std::cerr << std::endl;
	    for ( int isamp=samps.start; isamp<=samps.stop; isamp+=samps.step )
	    {
		std::cerr << isamp << '\t';
		for ( int icomp=0; icomp<nrcomps; icomp++ )
		    std::cout << trc.get( isamp-1, icomp )
			      << (icomp == nrcomps-1 ? '\n' : '\t');
	    }
	    std::cerr << std::endl;
	}
    }

    ExitProgram( 0 ); return 0;
}
