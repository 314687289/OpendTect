/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "batchprog.h"
#include "process_time2depth.h"

#include "trckeyzsampling.h"
#include "ioman.h"
#include "iopar.h"
#include "ioobj.h"
#include "multiid.h"
#include "progressmeter.h"
#include "seiszaxisstretcher.h"
#include "survinfo.h"
#include "timedepthconv.h"
#include "moddepmgr.h"

#include "prog.h"

bool BatchProgram::go( od_ostream& strm )
{
    OD::ModDeps().ensureLoaded("Seis");
    OD::ModDeps().ensureLoaded("Well");

    TrcKeyZSampling outputcs;
    if ( !outputcs.hsamp_.usePar( pars() ) )
    { outputcs.hsamp_.init( true ); }

    if ( !pars().get( SurveyInfo::sKeyZRange(), outputcs.zsamp_ ) )
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

    PtrMan<IOPar> ztranspar =
    	pars().subselect( ProcessTime2Depth::sKeyZTransPar() );

    if ( !ztranspar )
    {
	strm << "Cannot find tranformation parameters.";
	return false;
    }

    RefMan<ZAxisTransform> ztransform = ZAxisTransform::create( *ztranspar );
    if ( !ztransform )
    {
	strm << "Cannot construct transform.";
	return false;
    }

    if ( !ztransform->isOK() )
    {
	strm << "Velocity model is not usable";
	return false;
    }


    bool istime2depth;
    if ( !pars().getYN( ProcessTime2Depth::sKeyIsTimeToDepth(), istime2depth ) )
    {
	strm << "Cannot read direction";
	return false;
    }

    VelocityDesc veldesc;
    const bool isvel = veldesc.usePar( inputioobj->pars() ) &&
			veldesc.isVelocity();

    PtrMan<SeisZAxisStretcher> exec =
    	new SeisZAxisStretcher( *inputioobj, *outputioobj, outputcs,
			    	*ztransform, true, isvel );
    if ( isvel )
    {
	strm << "\nDetected that the stretching will be done on velocities.\n"
	        "Will stretch in z-domain and convert back to velocities.\n";

	//would we convert Thomsen? nothing prepared for this now
	exec->setVelTypeIsVint( veldesc.type_ == VelocityDesc::Interval );

	const bool isvrms = veldesc.type_ == VelocityDesc::RMS;
	if ( isvrms )
	{
	    exec->setVelTypeIsVrms( isvrms );
	    strm << "\nDetected that the input cube contains RMS velocities.\n"
		"RMS velocities are not present in Depth domain;\n"
		"a conversion to interval velocities will thus be processed.\n";
	}
    }

    exec->setName( "Time to depth conversion");
    if ( !exec->isOK() )
    {
	strm << "Cannot initialize readers/writers";
	return false;
    }

    TextStreamProgressMeter progressmeter( strm );
    exec->setProgressMeter( &progressmeter );

    return exec->execute();
}

