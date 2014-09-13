#ifndef ui2dsip_h
#define ui2dsip_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H.Bril
 Date:          Oct 2004
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uisip.h"


mExpClass(uiIo) ui2DSurvInfoProvider : public uiSurvInfoProvider
{ mODTextTranslationClass(ui2DSurvInfoProvider);
public:
    			ui2DSurvInfoProvider()
			    : xyft_(false)	{}

    const char*		usrText() const		{ return "Set for 2D only"; }
    uiDialog*		dialog(uiParent*);
    bool		getInfo(uiDialog*,TrcKeyZSampling&,Coord crd[3]);

    bool		xyInFeet() const	{ return xyft_; }

protected:

    bool		xyft_;
};


#endif

