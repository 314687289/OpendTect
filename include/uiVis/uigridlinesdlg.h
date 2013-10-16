#ifndef uigridlinesdlg_h
#define uigridlinesdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          February 2006
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidialog.h"
#include "ranges.h"

class uiCheckBox;
class uiGenInput;
class uiSelLineStyle;

namespace visSurvey { class PlaneDataDisplay; }

mExpClass(uiVis) uiGridLinesDlg : public uiDialog
{
public:
			uiGridLinesDlg(uiParent*,visSurvey::PlaneDataDisplay*);
			~uiGridLinesDlg();
protected:
    void		setParameters();
    void 		showGridLineCB(CallBacker*);
    bool                acceptOK(CallBacker*);
    bool                rejectOK(CallBacker*);
    bool		updateGridLines();
    void		settingChangedCB(CallBacker*);

    uiCheckBox*		inlfld_;
    uiCheckBox*		crlfld_;
    uiCheckBox*		zfld_;
    uiGenInput*		inlspacingfld_;
    uiGenInput*		crlspacingfld_;
    uiGenInput*		zspacingfld_;
    uiSelLineStyle*     lsfld_;
    uiCheckBox*		applyallfld_;

    visSurvey::PlaneDataDisplay*	pdd_;
};

#endif

