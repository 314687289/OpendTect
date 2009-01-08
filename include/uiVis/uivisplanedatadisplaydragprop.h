#ifndef uivisplanedatadisplaydragprop_h
#define uivisplanedatadisplaydragprop_h
/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id: uivisplanedatadisplaydragprop.h,v 1.2 2009-01-08 10:37:54 cvsranojay Exp $
________________________________________________________________________

*/

#include "uidlggroup.h"

namespace visSurvey { class PlaneDataDisplay; }

class uiButtonStateEdit;

mClass uiVisPlaneDataDisplayDragProp : public uiDlgGroup
{
public:
    			uiVisPlaneDataDisplayDragProp(uiParent*,
			    visSurvey::PlaneDataDisplay* );
    bool		acceptOK();
    bool		revertChanges();

protected:
    visSurvey::PlaneDataDisplay*	pdd_;
    uiButtonStateEdit*			scrollstate_;
    uiButtonStateEdit*			panstate_;

    const int				initialscrollstate_;
    const int				initialpanstate_;
};

#endif
