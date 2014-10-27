#ifndef uiodpicksettreeitem_h
#define uiodpicksettreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uioddisplaytreeitem.h"
namespace Pick		{ class Set; class SetMgr; }


mDefineItem( PickSetParent, TreeItem, TreeTop, \
    ~uiODPickSetParentTreeItem(); \
    virtual bool init(); \
    virtual void removeChild(uiTreeItem*); \
    void addPickSet(Pick::Set*);\
    void setRm(CallBacker*); \
    bool display_on_add; \
    Pick::SetMgr& picksetmgr_;\
    mShowMenu mMenuOnAnyButton );


mExpClass(uiODMain) uiODPickSetTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODPickSetTreeItemFactory);
public:

    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const { return new uiODPickSetParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;

};


mExpClass(uiODMain) uiODPickSetTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODPickSetTreeItem);
public:
    			uiODPickSetTreeItem(int dispid,Pick::Set&);
    			~uiODPickSetTreeItem();
    virtual bool	actModeWhenSelected() const	{ return true; }
    void		showAllPicks(bool yn);
    Pick::Set&		getSet()			{ return set_; }

protected:

    bool		init(); 
    void		prepareForShutdown();
    bool		askContinueAndSaveIfNeeded(bool withcancel);
    void		setChg(CallBacker*);
    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    const char*		parentType() const
    			{ return typeid(uiODPickSetParentTreeItem).name(); }

    Pick::Set&		set_;

    MenuItem		storemnuitem_;
    MenuItem		storeasmnuitem_;
    MenuItem		storepolyasfaultmnuitem_;
    MenuItem		dirmnuitem_;
    MenuItem		onlyatsectmnuitem_;
    MenuItem		convertbodymnuitem_;
    MenuItem		propertymnuitem_;
    MenuItem		closepolyitem_;
};



#endif

