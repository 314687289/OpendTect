/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : June 2011
-*/

static const char* rcsID = "$Id: uivoxelconnectivityfilterpi.cc,v 1.3 2012-03-27 10:25:34 cvsbert Exp $";

#include "odplugin.h"

#include "uivoxelconnectivityfilter.h"

mDefODPluginInfo(uiVoxelConnectivityFilter)
{
    static PluginInfo retpi = {
	"VoxelConnectivityFilter plugin",
	"Kristofer",
	"1.0",
   	"(c) dGB Beheer BV. Devlopment funded by Tetrale Technologies." };
    return &retpi;
}


mDefODInitPlugin(uiVoxelConnectivityFilter)
{
    VolProc::uiVoxelConnectivityFilter::initClass();
    return 0; // All OK - no error messages
}
