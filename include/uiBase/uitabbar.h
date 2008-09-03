#ifndef uitabbar_h
#define uitabbar_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          14/02/2003
 RCS:           $Id: uitabbar.h,v 1.16 2008-09-03 16:31:07 cvskris Exp $
________________________________________________________________________

-*/

#include "uiobj.h"

class uiTabBarBody;
class uiGroup;
class uiTabBody;
class uiTab;


//! TabBar widget only. Normally you'd want to use the uiTabStack class.
class uiTabBar : public uiObject
{
friend class		i_tabbarMessenger;
friend class		uiTabStack;
public:
			uiTabBar(uiParent*,const char* nm,
				 const CallBack* cb=0);

    int			addTab(uiTab*);
    void		removeTab(uiTab*);
    void		removeTab(uiGroup*);

    void		setTabEnabled(int idx,bool);
    bool		isTabEnabled(int idx) const;

    void		setCurrentTab(int idx);
    int			currentTabId() const;
    const char*		textOfTab(int idx) const;
    
    int			size() const;

    Notifier<uiTabBar>  selected;

    int			indexOf(const uiGroup*) const;
    int			indexOf(const uiTab*) const;
    uiGroup*		page(int idx) const;

    			//! Force activation in GUI thread
    void		activate(int idx);
    Notifier<uiTabBar>	activatedone;

protected:

    uiTabBarBody*	body_;
    uiTabBarBody&	mkbody(uiParent*,const char*);

    ObjectSet<uiTab>	tabs_;
};


class uiTab : public NamedObject
{
friend class		uiTabBar;
public:
			uiTab(uiGroup&);

    uiGroup&		group()		{ return grp_; }
    const uiGroup&	group() const	{ return grp_; }

protected:

    uiGroup&		grp_;
};

#endif
