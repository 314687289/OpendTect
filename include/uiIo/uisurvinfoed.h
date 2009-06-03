#ifndef uisurvinfoed_h
#define uisurvinfoed_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvinfoed.h,v 1.31 2009-06-03 10:41:49 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "ranges.h"
class IOPar;
class SurveyInfo;
class uiCheckBox;
class uiComboBox;
class uiGenInput;
class uiGroup;
class uiSurvInfoProvider;


/*\brief The survey info editor */

mClass uiSurveyInfoEditor : public uiDialog
{

public:

			uiSurveyInfoEditor(uiParent*,SurveyInfo&);
			~uiSurveyInfoEditor();

    bool		dirnmChanged() const	{ return dirnamechanged; }
    const char*		dirName() const;
    Notifier<uiSurveyInfoEditor> survparchanged;

    static int		addInfoProvider(uiSurvInfoProvider*);
    static bool		copySurv(const char* frompath,const char* fromdirnm,
	    			 const char* topath,const char* todirnm);
    static bool		renameSurv(const char* path,const char* fromdirnm,
				   const char* todirnm);
    static const char*	newSurvTempDirName();
    			
protected:

    SurveyInfo&		si_;
    BufferString	orgdirname_;
    BufferString	orgstorepath_;
    const FileNameString rootdir_;
    bool		isnew_;
    IOPar*		impiop_;
    uiSurvInfoProvider*	lastsip_;

    uiGenInput*		survnmfld_;
    uiGenInput*		pathfld_;
    uiGenInput*		inlfld_;
    uiGenInput*		crlfld_;
    uiGenInput*		zfld_;
    uiComboBox*		zunitfld_;
    uiComboBox*		pol2dfld_;

    uiGenInput*		x0fld_;
    uiGenInput*		xinlfld_;
    uiGenInput*		xcrlfld_;
    uiGenInput*		y0fld_;
    uiGenInput*		yinlfld_;
    uiGenInput*		ycrlfld_;
    uiGenInput*		ic0fld_;
    uiGenInput*		ic1fld_;
    uiGenInput*		ic2fld_;
    uiGenInput*		xy0fld_;
    uiGenInput*		xy1fld_;
    uiGenInput*		xy2fld_;
    uiGenInput*		coordset;
    uiGroup*		topgrp_;
    uiGroup*		crdgrp_;
    uiGroup*		trgrp_;
    uiGroup*		rangegrp_;
    uiComboBox*		sipfld_;
    uiCheckBox*		overrulefld_;
    uiCheckBox*		xyinftfld_;

    bool		dirnamechanged;
    void		mkSIPFld(uiObject*);
    void		mkRangeGrp();
    void		mkCoordGrp();
    void		mkTransfGrp();
    void		setValues();
    bool		setRanges();
    bool		setSurvName();
    bool		setCoords();
    bool		setRelation();
    bool		doApply();
    bool		acceptOK(CallBacker*);
    bool		rejectOK(CallBacker*);
    void		sipCB(CallBacker*);
    void		doFinalise(CallBacker*);
    void		setInl1Fld(CallBacker*);
    void		chgSetMode(CallBacker*);
    void		pathbutPush(CallBacker*);
    void		updStatusBar(const char*);
    void		edGCrdSetup(CallBacker*);
    void		appButPushed(CallBacker*);

    friend class	uiSurvey;

};

#endif
