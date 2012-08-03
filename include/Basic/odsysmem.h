#ifndef odsysmem_h
#define odsysmem_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2012
 RCS:		$Id: odsysmem.h,v 1.4 2012-08-03 13:00:13 cvskris Exp $
________________________________________________________________________

*/

#include "basicmod.h"
#include "gendefs.h"
class IOPar;

namespace OD
{
    mGlobal(Basic) void	getSystemMemory(float& total,float& free);
    mGlobal(Basic) void	dumpMemInfo(IOPar&);
}


#endif

