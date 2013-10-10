#ifndef uit2dvelconvselgroup_h
#define uit2dvelconvselgroup_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          July 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uit2dconvsel.h"

#include "multiid.h"

class uiVelSel;

mExpClass(uiSeis) uiT2DVelConvSelGroup : public uiT2DConvSelGroup
{
public:
    				uiT2DVelConvSelGroup(uiParent*);

    static void			initClass();
    static uiT2DConvSelGroup*	create( uiParent* p )
				{ return new uiT2DVelConvSelGroup(p); }

    bool			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:
    uiVelSel*			velsel_;
};


#endif

