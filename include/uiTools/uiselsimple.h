#ifndef uiselsimple_h
#define uiselsimple_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2001
 RCS:           $Id: uiselsimple.h,v 1.13 2009-02-11 12:04:18 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiListBox;
class uiGenInput;
class BufferStringSet;

/*!\brief Select entry from list */

mClass uiSelectFromList : public uiDialog
{ 	
public:

    mClass Setup : public uiDialog::Setup
    {
    public:
			Setup( const char* wintitl, const BufferStringSet& its )
			: uiDialog::Setup(wintitl,0,mNoHelpID)
			, items_(its)
			, current_(0)		{}

	mDefSetupMemb(int,current);

	const BufferStringSet&	items_;

    };

			uiSelectFromList(uiParent*,const Setup&);
			~uiSelectFromList()	{}

    int			selection() const	{ return setup_.current_; }
    			//!< -1 = no selection made (cancelled or 0 list items)

    uiListBox*		selFld()		{ return selfld_; }
    uiGenInput*		filtFld()		{ return filtfld_; }

protected:

    Setup		setup_;

    uiListBox*		selfld_;
    uiGenInput*		filtfld_;

    void		filtChg(CallBacker*);
    bool		acceptOK(CallBacker*);

private:

    void		init(const char**,int,const char*);

};


/*!\brief Get a name from user, whilst displaying names that already exist */

mClass uiGetObjectName : public uiDialog
{ 	
public:

    mClass Setup : public uiDialog::Setup
    {
    public:
			Setup( const char* wintitl,const BufferStringSet& its )
			: uiDialog::Setup(wintitl,0,mNoHelpID)
			, items_(its)
			, inptxt_("Name")	{}

	mDefSetupMemb(BufferString,deflt);
	mDefSetupMemb(BufferString,inptxt);

	const BufferStringSet&	items_;

    };

			uiGetObjectName(uiParent*,const Setup&);
			~uiGetObjectName()	{}

    const char*		text() const;

    uiGenInput*		inpFld()		{ return inpfld_; }
    			//!< Is the lowest field
    uiListBox*		selFld()		{ return listfld_; }

protected:

    uiGenInput*		inpfld_;
    uiListBox*		listfld_;

    void		selChg(CallBacker*);
    bool		acceptOK(CallBacker*);

};


#endif
