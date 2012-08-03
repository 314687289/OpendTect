#ifndef uiodfaulttreeitem_h
#define uiodfaulttreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodfaulttreeitem.h,v 1.24 2012-08-03 13:01:03 cvskris Exp $
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uioddisplaytreeitem.h"

#include "emposid.h"


namespace visSurvey { class FaultDisplay; class FaultStickSetDisplay; }


mDefineItem( FaultParent, TreeItem, TreeTop, mShowMenu mMenuOnAnyButton );


mClass(uiODMain) uiODFaultTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODFaultParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mClass(uiODMain) uiODFaultTreeItem : public uiODDisplayTreeItem
{
public:
    			uiODFaultTreeItem(int,bool dummy);
    			uiODFaultTreeItem(const EM::ObjectID&);
    			~uiODFaultTreeItem();

    EM::ObjectID	emObjectID() const	{ return emid_; }

protected:
    bool		askContinueAndSaveIfNeeded(bool withcancel);
    void		prepareForShutdown();
    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		colorChCB(CallBacker*);

    			/*Workaround to know which Fault is active is 3D*/
    void		selChgCB(CallBacker*);
    void		deSelChgCB(CallBacker*);

    bool		init();
    const char*		parentType() const
			{return typeid(uiODFaultParentTreeItem).name();}

    EM::ObjectID		emid_;
    MenuItem			savemnuitem_;
    MenuItem			saveasmnuitem_;
    MenuItem			displayplanemnuitem_;
    MenuItem			displaystickmnuitem_;
    MenuItem			displayintersectionmnuitem_;
    MenuItem			displayintersecthorizonmnuitem_;
    MenuItem			singlecolmnuitem_;
    visSurvey::FaultDisplay*	faultdisplay_;
};


mDefineItem( FaultStickSetParent, TreeItem, TreeTop,mShowMenu mMenuOnAnyButton);


mClass(uiODMain) uiODFaultStickSetTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODFaultStickSetParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mClass(uiODMain) uiODFaultStickSetTreeItem : public uiODDisplayTreeItem
{
public:
    			uiODFaultStickSetTreeItem(int,bool dummy);
    			uiODFaultStickSetTreeItem(const EM::ObjectID&);
    			~uiODFaultStickSetTreeItem();

    EM::ObjectID	emObjectID() const	{ return emid_; }

protected:
    bool		askContinueAndSaveIfNeeded( bool withcancel );
    void		prepareForShutdown();
    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		colorChCB(CallBacker*);

    			/*Workaround to know which fss is active in 3D*/
    void		selChgCB(CallBacker*);
    void		deSelChgCB(CallBacker*);

    bool		init();
    const char*		parentType() const
			{return typeid(uiODFaultStickSetParentTreeItem).name();}


    EM::ObjectID			emid_;
    MenuItem				onlyatsectmnuitem_;
    MenuItem				savemnuitem_;
    MenuItem				saveasmnuitem_;
    visSurvey::FaultStickSetDisplay*	faultsticksetdisplay_;
};


#endif

