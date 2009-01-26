#ifndef uipsviewerscalingtab_h
#define uipsviewerscalingtab_h

/*+
________________________________________________________________________
 
 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          May 2008
 RCS:           $Id: uipsviewerscalingtab.h,v 1.1 2009-01-26 15:09:08 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiflatviewproptabs.h"

class uiPushButton;

namespace PreStackView
{

class uiViewer3DMgr; 
class Viewer3D;

class uiViewer3DScalingTab : public uiFlatViewDataDispPropTab
{
public:
				uiViewer3DScalingTab(uiParent*,
						 PreStackView::Viewer3D&,
  						 uiViewer3DMgr&);
    virtual void		putToScreen();
    virtual void		setData()		{ doSetData(true); }

    bool			acceptOK();
    void			applyToAll(bool yn)	{ applyall_ = yn; }
    bool			applyToAll()		{ return applyall_; }
    void			saveAsDefault(bool yn)  { savedefault_ = yn; }
    bool			saveAsDefault()         { return savedefault_; }

protected:

    bool			applyButPushedCB(CallBacker*);
    bool			settingCheck();

    virtual const char* 	dataName() const;
    void                	dispSel(CallBacker*);
    virtual void		handleFieldDisplay(bool) {}
    FlatView::DataDispPars::Common& commonPars();
    
    uiPushButton*		applybut_;
    uiViewer3DMgr&		mgr_;
    bool			applyall_;
    bool			savedefault_;
};


}; //namespace

#endif

