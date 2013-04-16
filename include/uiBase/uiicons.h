#ifndef uiicons_h
#define uiicons_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "commondefs.h"

class ioPixmap;

namespace uiIcon
{

    mGlobal(uiBase) const char*		save();
    mGlobal(uiBase) const char*		saveAs();
    mGlobal(uiBase) const char*		openObject();
    mGlobal(uiBase) const char*		newObject();
    mGlobal(uiBase) const char*		removeObject();

    mGlobal(uiBase) const char*		None();
    				//!< Avoids pErrMsg

};


#endif

