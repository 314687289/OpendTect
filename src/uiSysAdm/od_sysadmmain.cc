/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H.Bril
 Date:          Jul 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: od_sysadmmain.cc,v 1.1 2010-01-06 13:11:25 cvsbert Exp $";


#include "prog.h"
#include "genc.h"
#include "errh.h"
#include <iostream>

extern int ODSysAdmMain(int,char**);

int main( int argc, char** argv )
{
    od_putProgInfo( argc, argv );

    int ret = ODSysAdmMain( argc, argv );
    ExitProgram( ret );
}
