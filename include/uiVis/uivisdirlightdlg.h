#ifndef uivisdirlightdlg_h
#define uivisdirlightdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Sep 2009
 RCS:           $Id: uivisdirlightdlg.h,v 1.13 2009-12-24 09:45:18 cvskarthika Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uipolardiagram.h"

class uiVisPartServer;
class uiGroup;
class uiLabel;
class uiSliderExtra;
class uiLabeledComboBox;
class uiGenInput;
class uiSeparator;
class uiRadioButton;
class uiPushButton;
class uiRGBArrayCanvas;

namespace visBase { class DirectionalLight; }

mClass uiDirLightDlg : public uiDialog
{
public:
				uiDirLightDlg(uiParent*, uiVisPartServer*);
				~uiDirLightDlg();

    float			getHeadOnIntensity() const;
    void			setHeadOnIntensity(float);

protected:

    visBase::DirectionalLight*	getDirLight(int) const;
    void			setDirLight();
    float			getHeadOnLight(int) const;
    void			setHeadOnLight();
    float			getAmbientLight(int) const;
    void			setAmbientLight();
    void			turnOnDirLight(bool);
    int				updateSceneSelector();	
    void			updateInitInfo();
    void			saveInitInfo();
    void			resetWidgets();
    void			setWidgets(bool);
    void			showWidgets(bool);
    void			validateInput();
    bool			isInSync();
    void			removeSceneNotifiers();

    bool			acceptOK(CallBacker*);
    bool			rejectOK(CallBacker*);
    void			pdDlgDoneCB(CallBacker*);
    void			showPolarDiagramCB(CallBacker*);
    void			lightSelChangedCB(CallBacker*);
    void			sceneSelChangedCB(CallBacker*);
    void			fieldChangedCB(CallBacker*);
    void			polarDiagramCB(CallBacker*);
    void			headOnChangedCB(CallBacker*);
    void			ambientChangedCB(CallBacker*);
    void			nrScenesChangedCB(CallBacker*);
    void			sceneNameChangedCB(CallBacker*);
    void			activeSceneChangedCB(CallBacker*);
    
    uiVisPartServer*		visserv_;
    uiGroup*			lightgrp_;
    uiLabel*			lightlbl_;
    uiRadioButton*		cameralightfld_;
    uiRadioButton*		scenelightfld_;
    uiRGBArrayCanvas*		cameralightcanvas_; 
    uiRGBArrayCanvas*		scenelightcanvas_; 
    uiLabeledComboBox*		scenefld_;
    uiSliderExtra*		azimuthfld_;
    uiSliderExtra*		dipfld_;
    uiSliderExtra*		intensityfld_;
    uiSliderExtra*		headonintensityfld_;
    uiSliderExtra*		ambintensityfld_;
    uiSeparator*		sep1_;
    uiSeparator*		sep2_;
    uiPushButton*		showpdfld_;
    uiPolarDiagram*		pd_;
    uiDialog*			pddlg_;

    typedef mStruct InitInfo
    {
		int		sceneid_;
        	
        	float		azimuth_;  // user degrees
	        float		dip_;  // degrees
        	
		float		intensity_;
	        float		headonintensity_;
        	float		ambintensity_;
	
	public:

				InitInfo();
		void		reset(bool resetheadonval=true);

        	InitInfo& 	operator = (const InitInfo&);
        	bool		operator == (const InitInfo&) const;
        	bool	 	operator != (const InitInfo&) const;

    } InitInfoType;

    TypeSet<InitInfoType>	initinfo_;

    bool			initlighttype_;
    				// initial light type: 0 - headon light, 
    				// 1 - scene (directional light)

    bool			currlighttype_;	// current light type
				
};

#endif
