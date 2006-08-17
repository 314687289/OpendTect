#ifndef uiseedpropdlg_h
#define uiseedpropdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiseedpropdlg.h,v 1.1 2006-08-17 14:08:59 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uimarkerstyledlg.h"
#include "emobject.h"

class uiColorInput;
class uiGenInput;
class uiSliderExtra;

class uiSeedPropDlg : public uiMarkerStyleDlg
{
public:
    			uiSeedPropDlg(uiParent*,EM::EMObject*);
			
protected:

    EM::EMObject*	emobject_;
    MarkerStyle3D	markerstyle_;

    void		doFinalise(CallBacker*);

    void		sliderMove(CallBacker*);
    void		typeSel(CallBacker*);
    void		colSel(CallBacker*);
    void		updateMarkerStyle();
};

#endif
