#ifndef uiviszstretchdlg_h
#define uiviszstretchdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidialog.h"

class uiCheckBox;
class uiLabeledComboBox;
class uiSlider;

/*!Dialog to set the z-stretch of a scene. */

mExpClass(uiVis) uiZStretchDlg : public uiDialog
{ mODTextTranslationClass(uiZStretchDlg);
public:
			uiZStretchDlg(uiParent*);
			~uiZStretchDlg();

    bool		valueChanged() const	{ return valchgd_; }

    CallBack		vwallcb; //!< If not set -> no button
    CallBack		homecb; //!< If not set -> no button

protected:

    uiLabeledComboBox*	scenefld_;
    uiSlider*		sliderfld_;
    uiCheckBox*		savefld_;
    uiButton*		vwallbut_;

    TypeSet<int>	sceneids_;
    float		initslval_;
    float		uifactor_;
    bool		valchgd_;

    static uiString	sZStretch() { return tr( "Z stretch" ); }
    void		setZStretch(float,bool permanent);
    void		updateSliderValues();

    void		doFinalise(CallBacker*);
    bool		acceptOK(CallBacker*);
    bool		rejectOK(CallBacker*);
    void		sliderMove(CallBacker*);
    void		butPush(CallBacker*);
    void		sceneSel(CallBacker*);
};

#endif
