#ifndef uihorizontracksetup_h
#define uihorizontracksetup_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          December 2005
 RCS:           $Id: uihorizontracksetup.h 38749 2015-04-02 19:49:51Z nanne.hemstra@dgbes.com $
________________________________________________________________________

-*/

#include "uimpemod.h"
#include "color.h"
#include "draw.h"
#include "valseriesevent.h"
#include "emseedpicker.h"

#include "uimpe.h"

class uiButtonGroup;
class uiColorInput;
class uiGenInput;
class uiIOObjSel;
class uiSeisSel;
class uiSlider;
class uiTabStack;
class uiToolBar;
class uiToolButton;


namespace MPE
{

class HorizonAdjuster;
class HorizonTrackerMgr;
class SectionTracker;
class uiCorrelationGroup;
class uiEventGroup;


/*!\brief Horizon tracking setup dialog. */

mExpClass(uiMPE) uiHorizonSetupGroup : public uiSetupGroup
{ mODTextTranslationClass(uiHorizonSetupGroup)
public:
				uiHorizonSetupGroup(uiParent*,const char*);
				~uiHorizonSetupGroup();

    void			setSectionTracker(SectionTracker*);

    void			setMode(const EMSeedPicker::SeedModeOrder);
    int				getMode();
    void			setSeedPos(const Coord3&);
    void			setColor(const Color&);
    const Color&		getColor();
    int				getLineWidth() const;
    void			setLineWidth(int);
    void			setMarkerStyle(const MarkerStyle3D&);
    const MarkerStyle3D&	getMarkerStyle();

    NotifierAccess*		modeChangeNotifier()
				{ return &modeChanged_; }
    NotifierAccess*		propertyChangeNotifier()
				{ return &propertyChanged_; }
    NotifierAccess*		eventChangeNotifier();
    NotifierAccess*		correlationChangeNotifier();
    NotifierAccess*		varianceChangeNotifier()
				{ return &varianceChanged_; }

    virtual bool		commitToTracker() const
				{ bool b; return commitToTracker(b); }
    bool			commitToTracker(bool& fieldchange) const;

    void			showGroupOnTop(const char* grpnm);

    enum State			{ Started, Paused, Stopped };
    State			getState() const	{ return state_; }

protected:

    virtual void		initStuff();

// General
    uiTabStack*			tabgrp_;
    uiIOObjSel*			horizonfld_;
    uiToolBar*			toolbar_;
    int				startbutid_;
    int				stopbutid_;
    int				savebutid_;
    int				retrackbutid_;
    State			state_;

    void			initToolBar();
    void			updateButtonSensitivity();
    bool			trackInVolume();
    void			startCB(CallBacker*);
    void			stopCB(CallBacker*);
    void			saveCB(CallBacker*);
    void			retrackCB(CallBacker*);
    void			horizonSelCB(CallBacker*);
    void			trackingFinishedCB(CallBacker*);

// Mode Group
    uiGroup*			createModeGroup();
    void			initModeGroup();
    void			seedModeChange(CallBacker*);

    uiButtonGroup*		modeselgrp_;
    uiGenInput*			methodfld_;

// Event and Correlation Group
    uiEventGroup*		eventgrp_;
    uiCorrelationGroup*		correlationgrp_;

// Variance Group
    uiGroup*			createVarianceGroup();
    void			initVarianceGroup();
    void			selUseVariance(CallBacker*);
    void			varianceChangeCB(CallBacker*);

    uiGenInput*			usevarfld_;
    uiSeisSel*			variancefld_;
    uiGenInput*			varthresholdfld_;


// Property Group
    uiGroup*			createPropertyGroup();
    void			initPropertyGroup();
    void			colorChangeCB(CallBacker*);
    void			seedTypeSel(CallBacker*);
    void			seedColSel(CallBacker*);
    void			seedSliderMove(CallBacker*);

    uiColorInput*		colorfld_;
    uiSlider*			linewidthfld_;
    uiGenInput*			seedtypefld_;
    uiColorInput*		seedcolselfld_;
    uiSlider*			seedsliderfld_;


    bool			is2d_;
    EMSeedPicker::SeedModeOrder	mode_;
    MarkerStyle3D		markerstyle_;

    HorizonTrackerMgr*		trackmgr_;
    SectionTracker*		sectiontracker_;
    HorizonAdjuster*		horadj_;

    Notifier<uiHorizonSetupGroup> modeChanged_;
    Notifier<uiHorizonSetupGroup> varianceChanged_;
    Notifier<uiHorizonSetupGroup> propertyChanged_;

    static const char**		sKeyEventNames();
    static const VSEvent::Type*	cEventTypes();
};


mExpClass(uiMPE) uiBaseHorizonSetupGroup : public uiHorizonSetupGroup
{
public:
    static void			initClass();

protected:
				uiBaseHorizonSetupGroup(uiParent*,const char*);
    static uiSetupGroup*	create(uiParent*,const char* typestr);
};


} // namespace MPE

#endif

