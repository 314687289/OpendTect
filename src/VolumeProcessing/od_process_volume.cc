/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID = "$Id: od_process_volume.cc,v 1.12 2009-01-28 22:21:44 cvskris Exp $";

#include "batchprog.h"

#include "attribdatacubeswriter.h"
#include "ioman.h"
#include "volprocchain.h"
#include "volproctrans.h"

#include "initalgo.h"
#include "initgeometry.h"
#include "initearthmodel.h"
#include "initvolumeprocessing.h"

bool BatchProgram::go( std::ostream& strm )
{ 
    Algo::initStdClasses();
    VolumeProcessing::initStdClasses();
    Geometry::initStdClasses();
    EarthModel::initStdClasses();
    
    MultiID chainid;
    pars().get( VolProcessingTranslatorGroup::sKeyChainID(), chainid );
    PtrMan<IOObj> ioobj = IOM().get( chainid );
    
    RefMan<VolProc::Chain> chain = new VolProc::Chain;
    BufferString errmsg;
    if ( !VolProcessingTranslator::retrieve( *chain, ioobj, errmsg ) )
    {
   	 chain = 0;
	 strm << "Could not open storage: " << chainid <<
	     	 ". Error description: ";
	 strm << errmsg.buf();
	 return false;
    }

    if ( chain->nrSteps()<1 )
    {
	strm << "Chain is empty - nothing to do.";
	return true;
    }

    CubeSampling cs( true );
    if ( !cs.usePar( pars() ) )
	strm << "Could not read ranges - Will process full survey\n";
    
    PtrMan<VolProc::ChainExecutor> pce = new VolProc::ChainExecutor( *chain );
    RefMan<Attrib::DataCubes> cube = new Attrib::DataCubes;

    cube->setSizeAndPos( cs );
    if ( !pce->setCalculationScope( cube ) )
    {
	strm << "Could not set calculation scope!";
	return false;
    } 

    if ( !pce->execute( &strm ) )
    {
	strm << "Unexecutable Chain!";
	return false;
    }	

    pce = 0; //delete all internal volumes.
   
    MultiID outputid;
    pars().get( VolProcessingTranslatorGroup::sKeyOutputID(), outputid );
    PtrMan<IOObj> outputobj = IOM().get( outputid );
    if ( !outputobj )
    {
	strm << "Could not find output ID!";
	return false;
    }	

    const TypeSet<int> indices( 1, 0 );
    Attrib::DataCubesWriter writer( outputid, *cube, indices );
    if ( !writer.execute( &strm ) )
    {
	return false;
    }

    return true;
}
