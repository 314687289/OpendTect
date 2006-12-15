#ifndef uiodrandlinetreeitem_h
#define uiodrandlinetreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodrandlinetreeitem.h,v 1.3 2006-12-15 14:35:57 cvsnanne Exp $
________________________________________________________________________


-*/

#include "uioddisplaytreeitem.h"


mDefineItem( RandomLineParent, TreeItem, TreeTop, mShowMenu bool load(); );
namespace visSurvey { class RandomTrackDisplay; };


class uiODRandomLineTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODRandomLineParentTreeItem; }
    uiTreeItem*		create(int visid,uiTreeItem*) const;
};


class uiODRandomLineTreeItem : public uiODDisplayTreeItem
{
public:
    			uiODRandomLineTreeItem( int );
    bool		init();

protected:
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    void                changeColTabCB(CallBacker*);
    const char*		parentType() const
			{ return typeid(uiODRandomLineParentTreeItem).name(); }

    void		editNodes();

    MenuItem		editnodesmnuitem_;
    MenuItem		insertnodemnuitem_;
    MenuItem		usewellsmnuitem_;
    MenuItem		saveasmnuitem_;
};


#endif
