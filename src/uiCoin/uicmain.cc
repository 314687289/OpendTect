/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          06/02/2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uicmain.cc,v 1.21 2008-11-25 15:35:24 cvsbert Exp $";

#include "uicmain.h"

#include "debug.h"
#include "debugmasks.h"
#include "envvars.h"

#include <Inventor/Qt/SoQt.h>
#include <Inventor/SoDB.h>


uicMain::uicMain( int& argc, char** argv )
    : uiMain( argc, argv )
{}


void uicMain::init( QWidget* widget )
{
    if ( !GetOSEnvVar("SOQT_BRIL_X11_SILENCER_HACK") )
	SetEnvVar( "SOQT_BRIL_X11_SILENCER_HACK", "1" );
    if ( !GetOSEnvVar("COIN_FULL_INDIRECT_RENDERING") )
	SetEnvVar( "COIN_FULL_INDIRECT_RENDERING", "1" );

    if ( DBG::isOn(DBG_UI) )
	DBG::message( "uicMain::init()" );
    SoQt::init( widget );
    SoDB::init();
}


int uicMain::exec()
{
    if ( DBG::isOn(DBG_UI) )
	DBG::message( "uicMain::exec(): Entering SoQt::mainLoop()." );
    SoQt::mainLoop();
    return 0;
}
