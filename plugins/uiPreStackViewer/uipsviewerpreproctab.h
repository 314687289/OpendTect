#ifndef uipsviewerpreproctab_h
#define uipsviewerpreproctab_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		May 2008
 RCS:		$Id: uipsviewerpreproctab.h,v 1.7 2008-12-22 19:25:37 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "uidlggroup.h"

class uiGenInput;
class uiPushButton;

namespace PreStack { class ProcessManager; class uiProcessorManager; }

namespace PreStackView
{

class Viewer3D;
class uiViewer3DMgr;

class uiViewer3DPreProcTab :  public uiDlgGroup
{
public:
				uiViewer3DPreProcTab(uiParent*,
					PreStackView::Viewer3D&,
					uiViewer3DMgr&,
					PreStack::ProcessManager&);
				~uiViewer3DPreProcTab();
    bool			acceptOK();

    void			applyToAll(bool yn)	{ applyall_ = yn; }
    bool			applyToAll()		{ return applyall_; }

protected:
    
    bool				applyButPushedCB(CallBacker*);
    void				processorChangeCB(CallBacker*);

    PreStackView::Viewer3D&		vwr_;
    uiViewer3DMgr&			mgr_;
    PreStack::ProcessManager*		preprocmgr_;
    PreStack::uiProcessorManager*	uipreprocmgr_;

    uiPushButton*			applybut_;    
    bool				applyall_;
};


}; //namespace

#endif
