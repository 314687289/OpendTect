#ifndef uipsviewerappearancetab_h
#define uipsviewerappearancetab_h

/*+
________________________________________________________________________
 
 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          May 2008
 RCS:           $Id: uipsviewerappearancetab.h,v 1.4 2009-07-22 16:01:28 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidlggroup.h"
#include "samplingdata.h"

class uiColorTable;
class uiLabel;
class uiPushButton;
class uiGenInput;
namespace visBase { class FlatViewer; };

namespace PreStackView
{

class uiViewer3DMgr; 
class Viewer3D;

class uiViewer3DAppearanceTab : public uiDlgGroup 
{
public:
				uiViewer3DAppearanceTab(uiParent*,
						 PreStackView::Viewer3D&,
  						 uiViewer3DMgr&);
    bool			acceptOK();
    void			applyToAll(bool yn)	{ applyall_ = yn; }
    bool			applyToAll()		{ return applyall_; }
    void			saveAsDefault(bool yn)  { savedefault_ = yn; }
    bool			saveAsDefault()         { return savedefault_; }

protected:

    void			applyButPushedCB(CallBacker*);
    void			updateZFlds(CallBacker*);
    void			updateOffsFlds(CallBacker*);
    void			updateFlds(uiGenInput* gridfld,
	    				   uiGenInput* autofld,
					   uiGenInput* rgfld,
					   uiLabel* lblfld,bool x1);
    
    uiColorTable*       	uicoltab_;
    uiLabel*            	uicoltablbl_; 
    uiLabel*            	zgridrangelbl_; 
    uiLabel*            	offsgridrangelbl_; 
    uiGenInput*            	zgridfld_; 
    uiGenInput*            	zgridautofld_; 
    uiGenInput*            	zgridrangefld_; 
    uiGenInput*            	offsgridfld_; 
    uiGenInput*            	offsgridautofld_; 
    uiGenInput*            	offsgridrangefld_; 
    uiPushButton*		applybut_;
    uiViewer3DMgr&		mgr_;
    visBase::FlatViewer*	vwr_;
    bool			applyall_;
    bool			savedefault_;
    SamplingData<float>		manuzsampl_;
    SamplingData<float>		manuoffssampl_;
};


}; //namespace

#endif

