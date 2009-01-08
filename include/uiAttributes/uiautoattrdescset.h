
#ifndef uiautoattrdescset_h
#define uiautoattrdescset_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        R. K. Singh
 Date:          June 2007
 RCS:           $Id: uiautoattrdescset.h,v 1.3 2009-01-08 08:50:11 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "multiid.h"
#include "bufstringset.h"

class uiAttrDescEd;
class uiCheckBox;
class uiGenInput;
class uiLabel;
class uiLabeledListBox;
class uiPushButton;
class uiIOObjSelGrp;
class CtxtIOObj;
class IOObj;


/*! \brief class for selecting Auto-load Attribute Set  */


mClass uiAutoAttrSelDlg : public uiDialog
{
public:
    				uiAutoAttrSelDlg(uiParent* p,bool);
				~uiAutoAttrSelDlg();

	IOObj*			getObj();
	bool			useAuto();
	bool			loadAuto();

protected:
	
	CtxtIOObj&          	ctio_;
	bool			is2d_;

	uiGenInput*         	usefld_;
	uiIOObjSelGrp*      	selgrp_;
	uiLabel*            	lbl_;
	uiCheckBox*         	loadbutton_;

	void			useChg(CallBacker*);
	bool			acceptOK(CallBacker*);

};


mClass uiAutoAttrSetOpen : public uiDialog
{
public:
				uiAutoAttrSetOpen(uiParent*,BufferStringSet&,
							BufferStringSet&);
				~uiAutoAttrSetOpen();
	IOObj*			getObj();
	const char*		getAttribname();
	const char*		getAttribfile();
	bool			isUserDef()		{ return usrdef_; }
	bool			isAuto()		{ return isauto_; }
	
protected:
	
	CtxtIOObj&              ctio_;

	uiIOObjSelGrp*          selgrp_;
	uiLabel*                lbl_;
	uiLabeledListBox*	defattrlist_;
	uiGenInput*		defselfld_;
	uiGenInput*		autoloadfld_;

	BufferStringSet		attribfiles_;
	BufferStringSet		attribnames_;
	int			defselid_;
	bool			usrdef_;
	bool			isauto_;

	void			setChg(CallBacker*);
	bool                    acceptOK(CallBacker*);

};


#endif
