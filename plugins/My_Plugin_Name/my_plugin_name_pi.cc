/*+
 * (C) Your_copyright
 * AUTHOR   : You!
 * DATE     : Apr 2012
-*/

static const char* rcsID mUnusedVar = "$Id: my_plugin_name_pi.cc,v 1.3 2012-05-02 15:11:11 cvskris Exp $";

#include "my_first_separate_source.h"
#include "odplugin.h"
#include <iostream>


mDefODPluginInfo(My_Plugin_Name)
{
    static PluginInfo retpi = {
	"My plugin",
	"You!",
	"My version",
    	"My description ..."
	    "\n ... which can span many lines."
    	    "\nDon't put commas between those lines though ..." };
    return &retpi;
}


mDefODInitPlugin(My_Plugin_Name)
{
    My_Class* myclss = new My_Class;
    return 0;
}
