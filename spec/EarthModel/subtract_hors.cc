/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:           2005
 RCS:           $Id: subtract_hors.cc,v 1.6 2006-05-09 15:22:18 cvsnanne Exp $
________________________________________________________________________

-*/



#include "prog.h"

#include "binidvalset.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "emmanager.h"
#include "emhorizon.h"
#include "emsurfaceauxdata.h"
#include "emsurfacegeometry.h"
#include "emposid.h"
#include "executor.h"
#include "ioman.h"
#include "ioobj.h"
#include "position.h"
#include "ranges.h"
#include "survinfo.h"


static int prUsage( const char* msg = 0 )
{
    std::cerr << "Usage: Top_Horizon_ID Bot_Horizon_ID add_res_to[T/B] "
		 "name[Thickness]";
    if ( msg ) std::cerr << '\n' << msg;
    std::cerr << std::endl;
    return 1;
}


static int prError( const char* msg )
{
    std::cerr << msg << std::endl;
    return 1;
}


static EM::Horizon* loadHorizon( const char* id, BufferString& err )
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Surf)->id) );
    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj ) { err = "Horizon "; err += id; err += " not OK"; return 0; }

    std::cerr << "Reading " << ioobj->name() << " ..." << std::endl;
    EM::EMManager& em = EM::EMM();
    PtrMan<Executor> exec = em.objectLoader( ioobj->key() );
    exec->execute( &std::cerr );
    EM::EMObject* emobj = em.getObject( em.getObjectID(ioobj->key()) );
    mDynamicCastGet(EM::Horizon*,horizon,emobj)
    if ( !horizon ) { err = "ID "; err += id; err += " is not horizon"; }
    return horizon;
}


static int doWork( int argc, char** argv )
{
    if ( argc < 4 ) return prUsage();

    BufferString errmsg;
    EM::Horizon* horizon1 = loadHorizon( argv[1], errmsg );
    if ( errmsg != "" ) return prError( errmsg );
    EM::Horizon* horizon2 = loadHorizon( argv[2], errmsg );
    if ( errmsg != "" ) return prError( errmsg );

    EM::Horizon& addtohor = !strcasecmp(argv[3],"T") ? *horizon1 : *horizon2;

    BufferString attribname = argv[4] ? argv[4] : "Thickness";
    const int dataidx = addtohor.auxdata.addAuxData( attribname );
    if ( dataidx < 0 ) return prError( "Cannot add datavalues" );

    EM::PosID posid( addtohor.id() );
    StepInterval<int> inlrg = addtohor.geometry().rowRange();
    StepInterval<int> crlrg = addtohor.geometry().colRange();
    HorSampling hs; hs.set( inlrg, crlrg );
    HorSamplingIterator iter( hs );
    BinID bid;
    while ( iter.next(bid) )
    {
	const EM::SubID subid = bid.getSerialized();
	const float z1 = horizon1->getPos( horizon1->sectionID(0), subid ).z;
	const float z2 = horizon2->getPos( horizon2->sectionID(0), subid ).z;

	float val = mUdf(float);
	if ( !mIsUdf(z1) && !mIsUdf(z2) )
	{
	    val = z2 - z1;
	    if ( SI().zIsTime() ) val *= 1000;
	}

	posid.setSubID( subid );
	addtohor.auxdata.setAuxDataVal( dataidx, posid, val );
    }

    std::cerr << "Saving data ..." << std::endl;
    PtrMan<Executor> saver = addtohor.auxdata.auxDataSaver();
    saver->execute();
    return 0;
}


int main( int argc, char** argv )
{
    return ExitProgram( doWork(argc,argv) );
}

