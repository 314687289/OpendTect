#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2014
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uidialog.h"

class uiFileSel;
class uiIOObjSelGrp;

mExpClass(uiIo) uiExp2DGeom : public uiDialog
{ mODTextTranslationClass(uiExp2DGeom)
public:
			uiExp2DGeom(uiParent*);
			~uiExp2DGeom();

protected:

    bool		acceptOK();

    uiIOObjSelGrp*	geomfld_;
    uiFileSel*		outfld_;
};
