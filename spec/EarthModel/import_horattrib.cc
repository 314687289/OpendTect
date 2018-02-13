/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          November 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "prog.h"

#include "ioobjctxt.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfacegeometry.h"
#include "executor.h"
#include "dbman.h"
#include "ioobj.h"
#include "position.h"
#include "strmprov.h"
#include "survinfo.h"


static int prUsage( const char* msg = 0 )
{
    std::cerr << "Usage: Horizon_ID attribfile attribname ic/xy";
    if ( msg ) std::cerr << '\n' << msg;
    std::cerr << std::endl;
    return 1;
}


static int prError( const char* msg )
{
    std::cerr << msg << std::endl;
    return 1;
}


static EM::Horizon3D* loadHorizon( const char* id, BufferString& err )
{
    DBM().to( DBKey(IOObjContext::getStdDirData(IOObjContext::Surf)->id) );
    PtrMan<IOObj> ioobj = DBM().get( id );
    if ( !ioobj )
	{ err = "Horizon "; err += id; err += " not OK"; return 0; }

    std::cerr << "Reading " << ioobj->name() << " ..." << std::endl;
    EM::ObjectManager& mgr = EM::Hor3DMan();
    PtrMan<Executor> exec = mgr.objectLoader( ioobj->key() );
    exec->execute( &std::cerr );
    EM::Object* emobj = mgr.getObject( mgr.getObjectID(ioobj->key()) );
    mDynamicCastGet(EM::Horizon3D*,horizon,emobj)
    if ( !horizon )
	{ err = "ID "; err += id; err += " is not horizon"; }
    return horizon;
}


static int doWork( int argc, char** argv )
{
    if ( argc < 4 ) return prUsage();

    BufferString errmsg;
    EM::Horizon3D* horizon = loadHorizon( argv[1], errmsg );
    if ( errmsg != "" ) return prError( errmsg );

    StreamData sd = StreamProvider(argv[2]).makeIStream();
    if ( !sd.usable() ) return prError( "input_XYV_file not OK" );

    BufferString attribname = argv[3];
    const int dataidx = horizon->auxdata.addAuxData( attribname );
    if ( dataidx < 0 ) return prError( "Cannot add datavalues" );

    const bool doxy = !strcmp("xy",argv[4]);

    EM::PosID posid( horizon->id() );
    while ( sd.istrm->good() )
    {
	BinID bid;
	float val;
	if ( doxy )
	{
	    Coord crd;
	    *sd.istrm >> crd.x >> crd.y >> val;
	    bid = SI().transform( crd );
	}
	else
	    *sd.istrm >> bid.inl >> bid.crl >> val;

	posid.setSubID( bid.getSerialized() );
	if ( horizon->isDefined(posid) )
	    horizon->auxdata.setAuxDataVal( dataidx, posid, val );
    }
    sd.close();

    std::cerr << "Saving data ..." << std::endl;
    PtrMan<Executor> saver = horizon->auxdata.auxDataSaver();
    saver->execute();
    return 0;
}


int main( int argc, char** argv )
{
    return ExitProgram( doWork(argc,argv) );
}

