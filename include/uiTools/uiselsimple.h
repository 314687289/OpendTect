#ifndef uiselsimple_h
#define uiselsimple_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"

class uiGroup;
class uiListBox;
class uiGenInput;
class uiCheckList;
class BufferStringSet;

/*!\brief Select entry from list */

mExpClass(uiTools) uiSelectFromList : public uiDialog
{
public:

    mExpClass(uiTools) Setup : public uiDialog::Setup
    {
    public:
			Setup( const uiString& wintitl,
			       const TypeSet<uiString>& its )
			    : uiDialog::Setup(wintitl,0,mNoHelpKey)
			    , items_(its)
			    , current_(0)		{}
			Setup( const uiString& wintitl,
			       const BufferStringSet& its )
			    : uiDialog::Setup(wintitl,0,mNoHelpKey)
			    , current_(0)		{ its.fill(items_); }

	mDefSetupMemb(int,current);

	TypeSet<uiString>	items_;

    };

			uiSelectFromList(uiParent*,const Setup&);
			~uiSelectFromList()	{}

    int			selection() const	{ return setup_.current_; }
			//!< -1 = no selection made (cancelled or 0 list items)

    uiListBox*		selFld()		{ return selfld_; }
    uiGenInput*		filtFld()		{ return filtfld_; }
    uiObject*		bottomFld(); //!< is selFld()

protected:

    Setup		setup_;

    uiListBox*		selfld_;
    uiGenInput*		filtfld_;

    void		filtChg(CallBacker*);
    virtual bool	acceptOK(CallBacker*);

private:

    void		init(const char**,int,const char*);

};


/*!\brief Get a name from user, whilst displaying names that already exist */

mExpClass(uiTools) uiGetObjectName : public uiDialog
{
public:

    mExpClass(uiTools) Setup : public uiDialog::Setup
    {
    public:
			Setup( const uiString& wintitl,
			       const BufferStringSet& its )
			: uiDialog::Setup(wintitl,0,mNoHelpKey)
			, items_(its)
			, inptxt_( "Name" )	{}

	mDefSetupMemb(BufferString,deflt);
	mDefSetupMemb(uiString,inptxt);

	const BufferStringSet&	items_;

    };

			uiGetObjectName(uiParent*,const Setup&);
			~uiGetObjectName()	{}

    const char*		text() const;

    uiGenInput*		inpFld()		{ return inpfld_; }
    uiListBox*		selFld()		{ return listfld_; }
    uiGroup*		bottomFld(); //!< is inpFld()

protected:

    uiGenInput*		inpfld_;
    uiListBox*		listfld_;

    void		selChg(CallBacker*);
    virtual bool	acceptOK(CallBacker*);

};


/*!\brief Get an action from a series of possibilities from user */

mExpClass(uiTools) uiGetChoice : public uiDialog
{
public:

			uiGetChoice(uiParent*,const char* question=0,
				    bool allowcancel=true,
				    const HelpKey& helpkey=mNoHelpKey);
			uiGetChoice(uiParent*,
				    const BufferStringSet& options,
				    const char* question=0,
				    bool allowcancel=true,
				    const HelpKey& helpkey=mNoHelpKey);
			uiGetChoice(uiParent*,uiDialog::Setup,
				    const BufferStringSet& options, bool wc);

    void		addChoice(const char* txt,const char* iconnm=0);
    void		setDefaultChoice(int);
    int			choice() const		{ return choice_; }
			//!< on cancel will be -1

    uiCheckList*	checkList();
    uiGroup*		bottomFld(); //!< is checkList()

protected:

    uiCheckList*	inpfld_;
    int			choice_;
    const bool		allowcancel_;

    virtual bool	acceptOK(CallBacker*);
    virtual bool	rejectOK(CallBacker*);

};


#endif
