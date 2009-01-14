#ifndef uistratutildlgs_h
#define uistratutildlgs_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          August 2007
 RCS:           $Id: uistratutildlgs.h,v 1.8 2009-01-14 14:56:59 cvshelene Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class BufferStringSet;
class uiColorInput;
class uiGenInput;
class uiListBox;
class uiCheckBox;
class uiStratMgr;
namespace Strat { class Lithology; }

/*!\brief Displays a dialog to create new stratigraphic unit */

mClass uiStratUnitDlg : public uiDialog
{
public:

			uiStratUnitDlg(uiParent*,uiStratMgr*);
    const char*		getUnitName() const;	
    const char*		getUnitDesc() const;
    const char*		getUnitLith() const;
    void		setUnitName(const char*);
    void		setUnitDesc(const char*);
    void		setUnitLith(const char*);
    void		setUnitIsLeaf(bool);

protected:
    uiGenInput*		unitnmfld_;
    uiGenInput*		unitdescfld_;
    uiGenInput*		unitlithfld_;

    uiStratMgr*		uistratmgr_;

    void		selLithCB(CallBacker*);
    bool		acceptOK(CallBacker*);

};


/*!\brief Displays a dialog to create new lithology */

mClass uiStratLithoDlg : public uiDialog
{
public:

			uiStratLithoDlg(uiParent*, uiStratMgr*);
    const char*		getLithName() const;
    void		setSelectedLith(const char*);

    Notifier<uiStratLithoDlg>	lithAdd;
    Notifier<uiStratLithoDlg>	lithChg;
    Notifier<uiStratLithoDlg>	lithRem;

protected:

    uiListBox*		selfld_;
    uiGenInput*		nmfld_;
    uiCheckBox*		isporbox_;

    Strat::Lithology*	prevlith_;
    uiStratMgr*		uistratmgr_;

    void		fillLiths();
    void		newLith(CallBacker*);
    void		selChg(CallBacker*);
    void		rmSel(CallBacker*);

    bool		acceptOK(CallBacker*);
};


/*!\brief Displays a dialog to create new level */

mClass uiStratLevelDlg : public uiDialog
{
public:

			uiStratLevelDlg(uiParent*,uiStratMgr*);

    void		setLvlInfo(const char*);
protected:

    BufferString	oldlvlnm_;
    uiStratMgr*		uistratmgr_;

    uiGenInput*		lvlnmfld_;
    uiGenInput*		lvltvstrgfld_;
    uiGenInput*		lvltimergfld_;
    uiGenInput*		lvltimefld_;
    uiColorInput*	lvlcolfld_;
    void		isoDiaSel(CallBacker*);
    bool		acceptOK(CallBacker*);
};


/*!\brief Displays a dialog to link level and stratigraphic unit*/

mClass uiStratLinkLvlUnitDlg : public uiDialog
{
public:

			uiStratLinkLvlUnitDlg(uiParent*,const char*,
					      uiStratMgr*);
    uiGenInput*		lvltoplistfld_;
    uiGenInput*		lvlbaselistfld_;

protected:

    bool		acceptOK(CallBacker*);
    
    uiStratMgr*		uistratmgr_;
};


#endif
