#ifndef uidlggroup_h
#define uidlggroup_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          13/8/2000
 RCS:           $Id: uidlggroup.h,v 1.6 2008-01-17 16:05:00 cvskris Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uigroup.h"

class uiTabStack;

/*!
Dialog that either can be used standalone (with uiSingleGroupDlg) or
in a tabstack (uiTabStackDlg) */


class uiDlgGroup : public uiGroup
{
public:
    			uiDlgGroup(uiParent* p,const char* nm)
			    : uiGroup( p, nm )		{}

    virtual bool	acceptOK()			{ return true; }
    			/*!<Commit changes. Return true if success. */
    virtual bool	rejectOK()			{ return true; }
    			/*!<Revert eventual changes allready done, so the
			    object is in it's original state. */
    virtual bool	revertChanges()			{ return true; }
    			/*!<Revert eventual changes allready done, so the
			    object is in it's original state. The difference
			    betweeen revertChanges() and rejectOK() is that
			    revertChanges() may be called after a successful
			    acceptOK() call, if another tab in the stack fails.
			*/
    virtual const char*	errMsg() const			{ return 0; }
    			
};

/*! Dialog with one uiDlgGroup. */

class uiSingleGroupDlg : public uiDialog
{
public:
		uiSingleGroupDlg( uiParent* p,const uiDialog::Setup& st )
		    : uiDialog(p,st)
		    , grp_(0)			{}
    void	setGroup( uiDlgGroup* grp )	{ grp_ = grp; }

protected:
    bool	acceptOK(CallBacker*)		{ return grp_->acceptOK(); }
    bool	rejectOK(CallBacker*)		{ return grp_->rejectOK(); }
	
    uiDlgGroup*	grp_;
};


/*! Dialog with multiple uiDlgGroup in a tabstack. */
class uiTabStackDlg : public uiDialog
{
public:
			uiTabStackDlg(uiParent*,const uiDialog::Setup&);

    uiParent*		tabParent();
    uiObject*		tabObject()		{ return (uiObject*)tabstack_; }
    void		addGroup(uiDlgGroup*);

    int			nrGroups() const	{ return groups_.size(); }
    uiDlgGroup&		getGroup(int idx)	{ return *groups_[idx]; }
    const uiDlgGroup&	getGroup(int idx) const { return *groups_[idx]; }
    void		showGroup(int idx);

protected:

    virtual bool		acceptOK(CallBacker*);
    virtual bool		rejectOK(CallBacker*);

    bool 			canrevert_;
    ObjectSet<uiDlgGroup>	groups_;
    uiTabStack*			tabstack_;
};

#endif
