#ifndef uiattrdescseted_h
#define uiattrdescseted_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiattrdescseted.h,v 1.23 2009-07-22 16:01:20 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "multiid.h"

namespace Attrib
{
    class Desc;
    class DescID;
    class DescSet;
    class DescSetMan;
};

namespace Pick { class Set; }
class uiAttrDescEd;
class uiAttrTypeSel;
class uiGenInput;
class uiLineEdit;
class uiListBox;
class uiPushButton;
class uiToolBar;
class BufferStringSet;
class CtxtIOObj;
class IOObj;
class uiToolButton;

/*! \brief Editor for Attribute sets */

mClass uiAttribDescSetEd : public uiDialog
{
public:

			uiAttribDescSetEd(uiParent*,Attrib::DescSetMan* adsm,
					  const char* prefgrp =0);
			~uiAttribDescSetEd();

    Attrib::DescSet*	getSet()		{ return attrset_; }
    const MultiID&	curSetID() const	{ return setid_; }

    uiAttrDescEd*	curDescEd();
    			//!< Use during operation only!
    Attrib::Desc*		curDesc() const;
    			//!< Use during operation only!
    int			curDescNr() const;
    			//!< Use during operation only!
    void		updateCurDescEd();
    bool		is2D() const;

    void		setSensitive(bool);

    Notifier<uiAttribDescSetEd>		dirshowcb;
    Notifier<uiAttribDescSetEd>		evalattrcb;
    Notifier<uiAttribDescSetEd>		xplotcb;

    static const char* 	sKeyUseAutoAttrSet;
    static const char* 	sKeyAuto2DAttrSetID;
    static const char*  sKeyAuto3DAttrSetID;

protected:

    Attrib::DescSetMan*		inoutadsman_;
    Attrib::DescSetMan*		adsman_;
    Attrib::DescSet*		attrset_;
    Attrib::Desc*		prevdesc_;
    MultiID			setid_;
    ObjectSet<uiAttrDescEd> 	desceds_;
    ObjectSet<Attrib::Desc> 	attrdescs_;
    BufferStringSet&		userattrnames_;
    CtxtIOObj&			setctio_;
    MultiID			cancelsetid_;
    bool			updating_fields_;
    static BufferString		nmprefgrp_;

    uiToolBar*			toolbar_;
    uiListBox*			attrlistfld_;
    uiAttrTypeSel*		attrtypefld_;
    uiPushButton*		rmbut_;
    uiPushButton*		addbut_;
    uiPushButton*		revbut_;
    uiLineEdit*			attrnmfld_;
    uiGenInput*			attrsetfld_;
    uiToolButton*       	helpbut_;
    uiToolButton*       	moveupbut_;
    uiToolButton*       	movedownbut_;
    uiToolButton*       	sortbut_;


    void			attrTypSel(CallBacker*);
    void			selChg(CallBacker*);
    void			revPush(CallBacker*);
    void			addPush(CallBacker*);
    void			rmPush(CallBacker*);
    void			moveUpDownCB(CallBacker*);
    void                	sortPush(CallBacker*);
    void                	helpButPush(CallBacker*);

    void			autoSet(CallBacker*);
    void			newSet(CallBacker*);
    void			openSet(CallBacker*);
    void                	openAttribSet(const IOObj*);
    void			savePush(CallBacker*);
    void			changeInput(CallBacker*);
    void			defaultSet(CallBacker*);
    void			getDefaultAttribsets(BufferStringSet&,
	    					     BufferStringSet&);
    void			importSet(CallBacker*);
    void			importFile(CallBacker*);
    void			job2Set(CallBacker*);
    void			crossPlot(CallBacker*);
    void			directShow(CallBacker*);
    void			evalAttribute(CallBacker*);
    void			importFromFile(const char*);

    void			setButStates();
    bool			offerSetSave();
    bool			doSave(bool);
    void			replaceStoredAttr();
    void			removeNotUsedAttr();
    //bool		hasInput(const Attrib::Desc&,const Attrib::DescID&);

    bool			acceptOK(CallBacker*);
    bool			rejectOK(CallBacker*);

    void			newList(int);
    void			updateFields(bool settype=true);
    bool			doCommit(bool prevdesc=false);
    void			handleSensitivity();
    void			updateUserRefs();
    bool			validName(const char*) const;
    bool			setUserRef(Attrib::Desc*);
    void			updateAttrName();
    bool			doSetIO(bool);
    Attrib::Desc*		createAttribDesc(bool checkuref=true);

    void			createMenuBar();
    void			createToolBar();
    void			createGroups();
    void			init();
};


#endif
