#ifndef uimultcomputils_h
#define uimultcomputils_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          August 2008
 RCS:           $Id: uimultcomputils.h,v 1.8 2009-07-30 13:30:04 cvshelene Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"
#include "uicompoundparsel.h"
#include "uidialog.h"

class LineKey;
class uiGenInput;
class uiLabeledListBox;
class uiListBox;


/*!\brief dialog to select (multiple) component(s) of stored data */

mClass uiMultCompDlg : public uiDialog
{
public:
			uiMultCompDlg(uiParent*,const BufferStringSet&);

    void		getCompNrs(TypeSet<int>&) const;
    const char*		getCompName(int) const;

protected:

    uiListBox*		compfld_;
};


/*!\brief CompoundParSel to capture and sum up the user-selected components */

mClass uiMultCompSel : public uiCompoundParSel
{
    public:
			uiMultCompSel(uiParent*);
			~uiMultCompSel();

    void		setUpList(LineKey);
    void		setUpList(const BufferStringSet&);
    bool		allowChoice() const	{ return compnms_.size()>1; }

    protected:
	
    BufferString        getSummary() const;
    void                doDlg(CallBacker*);
    void		prepareDlg();

    mClass MCompDlg : public uiDialog
    {
	public:
	    			MCompDlg(uiParent*,const BufferStringSet&);

	void			selChg(CallBacker*);
	uiLabeledListBox*	outlistfld_;
	uiGenInput*		useallfld_;
    };

    BufferStringSet	compnms_;
    MCompDlg*		dlg_;
};


#endif
