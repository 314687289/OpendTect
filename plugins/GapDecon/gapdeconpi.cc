/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : H. Huck
 * DATE     : Aug 2006
-*/

static const char* rcsID = "$Id: gapdeconpi.cc,v 1.6 2011-04-21 13:09:13 cvsbert Exp $";

#include "odplugin.h"
#include "gapdeconattrib.h"

mDefODPluginEarlyLoad(GapDecon)
mDefODPluginInfo(GapDecon)
{
    static PluginInfo retpii = {
	"Gap Decon Base",
	"dGB - Helene",
	"=od",
	"Gap Decon (Prediction Error Filter) attribute plugin.\n\n" };
    return &retpii;
}


mDefODInitPlugin(GapDecon)
{
    Attrib::GapDecon::initClass();

    return 0; // All OK - no error messages
}
