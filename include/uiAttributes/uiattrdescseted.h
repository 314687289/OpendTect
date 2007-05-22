#ifndef uiattrdescseted_h
#define uiattrdescseted_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiattrdescseted.h,v 1.8 2007-05-22 07:02:09 cvsraman Exp $
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

class uiAttrDescEd;
class uiAttrTypeSel;
class uiGenInput;
class uiLineEdit;
class uiListBox;
class uiPushButton;
class uiToolBar;
class BufferStringSet;
class CtxtIOObj;


/*! \brief Editor for Attribute sets */

class uiAttribDescSetEd : public uiDialog
{
public:

			uiAttribDescSetEd(uiParent*,Attrib::DescSetMan* adsm);
			~uiAttribDescSetEd();

    Attrib::DescSet*	getSet()		{ return attrset; }
    const MultiID&	curSetID() const	{ return setid; }

    void		autoSet();
    uiAttrDescEd*	curDescEd();
    			//!< Use during operation only!
    Attrib::Desc*		curDesc() const;
    			//!< Use during operation only!
    int			curDescNr() const;
    			//!< Use during operation only!
    void		updateCurDescEd();
    bool		is2D() const;

    Notifier<uiAttribDescSetEd>		dirshowcb;
    Notifier<uiAttribDescSetEd>		evalattrcb;

    static const char* sKeyUseAutoAttrSet;
    static const char* sKeyAutoAttrSetID;

protected:

    Attrib::DescSetMan*	inoutadsman;
    Attrib::DescSetMan*	adsman;
    Attrib::DescSet*	attrset;
    Attrib::Desc*	prevdesc;
    MultiID		setid;
    ObjectSet<uiAttrDescEd> desceds;
    ObjectSet<Attrib::Desc> attrdescs;
    BufferStringSet&	userattrnames;
    CtxtIOObj&		setctio;
    MultiID		cancelsetid;
    bool		updating_fields;

    uiToolBar*		toolbar;
    uiListBox*		attrlistfld;
    uiAttrTypeSel*	attrtypefld;
    uiPushButton*	rmbut;
    uiPushButton*	addbut;
    uiPushButton*	revbut;
    uiLineEdit*		attrnmfld;
    uiGenInput*		attrsetfld;

    void		attrTypSel(CallBacker*);
    void		selChg(CallBacker*);
    void		revPush(CallBacker*);
    void		addPush(CallBacker*);
    void		rmPush(CallBacker*);

    void		newSet(CallBacker*);
    void		openSet(CallBacker*);
    void		savePush(CallBacker*);
    void		changeInput(CallBacker*);
    void		defaultSet(CallBacker*);
    void		importSet(CallBacker*);
    void		importFile(CallBacker*);
    void		job2Set(CallBacker*);
    void		crossPlot(CallBacker*);
    void		directShow(CallBacker*);
    void		evalAttribute(CallBacker*);
    void		importFromFile(const char*);

    bool		offerSetSave();
    bool		doSave(bool);
    void		replaceStoredAttr();
    void		removeNotUsedAttr();
    bool		hasInput(const Attrib::Desc&,const Attrib::DescID&);

    bool		acceptOK(CallBacker*);
    bool		rejectOK(CallBacker*);

    void		newList(int);
    void		updateFields(bool settype=true);
    bool		doCommit(bool prevdesc=false);
    void		handleSensitivity();
    void		updateUserRefs();
    bool		validName(const char*) const;
    bool		setUserRef(Attrib::Desc*);
    void		updateAttrName();
    bool		doSetIO(bool);

    void		createMenuBar();
    void		createToolBar();
    void		createGroups();
    void		init();

};


#endif
