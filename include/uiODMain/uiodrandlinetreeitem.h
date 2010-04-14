#ifndef uiodrandlinetreeitem_h
#define uiodrandlinetreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodrandlinetreeitem.h,v 1.15 2010-04-14 08:05:02 cvsnanne Exp $
________________________________________________________________________


-*/

#include "uioddisplaytreeitem.h"

class IOObj;
class uiRandomLinePolyLineDlg;

mDefineItem( RandomLineParent, TreeItem, TreeTop, mShowMenu \
    bool load(const IOObj&); \
    const IOObj* selRandomLine(); \
    void genRandLine(int); \
    void genRandLineFromWell();\
    void genRandLineFromTable();\
    void loadRandLineFromWell(CallBacker*);\
    void genRandomLineFromPickPolygon();\
    void rdlPolyLineDlgCloseCB(CallBacker*);\
    uiRandomLinePolyLineDlg* rdlpolylinedlg_;
    mMenuOnAnyButton
);

namespace visSurvey { class RandomTrackDisplay; };


mClass uiODRandomLineTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODRandomLineParentTreeItem; }
    uiTreeItem*		create(int visid,uiTreeItem*) const;
};


mClass uiODRandomLineTreeItem : public uiODDisplayTreeItem
{
public:
    			uiODRandomLineTreeItem( int );
    bool		init();

protected:

    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    void                changeColTabCB(CallBacker*);
    void		remove2DViewerCB(CallBacker*);
    const char*		parentType() const
			{ return typeid(uiODRandomLineParentTreeItem).name(); }

    void		editNodes();

    MenuItem		editnodesmnuitem_;
    MenuItem		insertnodemnuitem_;
    MenuItem		usewellsmnuitem_;
    MenuItem		saveasmnuitem_;
    MenuItem		saveas2dmnuitem_;
    MenuItem		create2dgridmnuitem_;
};


#endif
