/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		April 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: od_process_2dgrid.cc,v 1.1 2010-08-26 03:55:44 cvsraman Exp $";

#include "batchprog.h"

#include "emhorizon2d.h"
#include "gridcreator.h"
#include "hor2dfrom3dcreator.h"
#include "initearthmodel.h"
#include "iopar.h"
#include "ptrman.h"

#include <iostream>


#define mErrRet(msg) { strm << msg << std::endl; return false; }

bool BatchProgram::go( std::ostream& strm )
{
    EarthModel::initStdClasses();

    PtrMan<IOPar> seispar = pars().subselect( "Seis" );
    if ( !seispar )
    {
	strm << "Cannot find necessary information in parameter file"
	     << std::endl;
	return false;
    }

    bool res = true;
    TextTaskRunner tr( strm );
    strm << "Creating 2D Grid ..." << std::endl;
    Seis2DGridCreator seiscr( *seispar );
    res = tr.execute( seiscr );
    if ( !res )
    {
	strm << "  failed.\nProcess stopped" << std::endl;
	return false;
    }

    PtrMan<IOPar> horpar = pars().subselect( "Horizon" );
    if ( !horpar )
	return res;

    strm << "\n\nCreating 2D Horizon(s) ..." << std::endl;
    Horizon2DGridCreator horcr;
    horcr.init( *horpar, &tr );
    res = tr.execute( horcr );
    if ( !res ) 
    {
	strm << "  failed.\nProcess stopped" << std::endl;
	return false;
    }

    res = horcr.finish( &tr );
    return res;
}
