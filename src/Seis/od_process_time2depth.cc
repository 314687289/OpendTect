/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID = "$Id: od_process_time2depth.cc,v 1.5 2010-07-08 18:38:27 cvskris Exp $";

#include "batchprog.h"
#include "process_time2depth.h"

#include "cubesampling.h"
#include "ioman.h"
#include "iopar.h"
#include "ioobj.h"
#include "multiid.h"
#include "seiszaxisstretcher.h"
#include "survinfo.h"
#include "timedepthconv.h"

#include "prog.h"

bool BatchProgram::go( std::ostream& strm )
{ 
    CubeSampling outputcs;
    if ( !outputcs.hrg.usePar( pars() ) )
    { outputcs.hrg.init( true ); }

    if ( !pars().get( SurveyInfo::sKeyZRange(), outputcs.zrg ) )
    {
	strm << "Cannot read output sampling";
	return false;
    }


    MultiID inputmid;
    if ( !pars().get( ProcessTime2Depth::sKeyInputVolume(), inputmid) )
    {
	strm << "Cannot read input volume id"; 
	return false;
    }

    PtrMan<IOObj> inputioobj = IOM().get( inputmid );
    if ( !inputioobj )
    {
	strm << "Cannot read input volume object"; 
	return false;
    }

    MultiID outputmid;
    if ( !pars().get( ProcessTime2Depth::sKeyOutputVolume(), outputmid ) )
    {
	strm << "Cannot read output volume id"; 
	return false;
    }

    PtrMan<IOObj> outputioobj = IOM().get( outputmid );
    if ( !outputioobj )
    {
	strm << "Cannot read output volume object"; 
	return false;
    }

    MultiID velmid;
    if ( !pars().get( ProcessTime2Depth::sKeyVelocityModel(), velmid) )
    {
	strm << "Cannot read velocity volume id"; 
	return false;
    }

    bool istime2depth;
    if ( !pars().getYN( ProcessTime2Depth::sKeyIsTimeToDepth(), istime2depth ) )
    {
	strm << "Cannot read direction";
	return false;
    }
    
    RefMan<VelocityStretcher> ztransform = istime2depth
	? (VelocityStretcher*) new Time2DepthStretcher
	: (VelocityStretcher*) new Depth2TimeStretcher;

    if ( !ztransform->setVelData( velmid ) || !ztransform->isOK() )
    {
	strm << "Velocity model is not usable";
	return false;
    }

    SeisZAxisStretcher exec( *inputioobj, *outputioobj, outputcs, *ztransform,
	   		     true );
    exec.setName( "Time to depth conversion");
    if ( !exec.isOK() )
    {
	strm << "Cannot initialize readers/writers";
	return false;
    }

    return exec.execute( &strm );
}
