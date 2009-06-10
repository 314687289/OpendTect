#ifndef uiodbodydisplaytreeitem_h
#define uiodbodydisplaytreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodbodydisplaytreeitem.h,v 1.10 2009-06-10 20:53:38 cvskris Exp $
________________________________________________________________________


-*/

#include "uioddisplaytreeitem.h"

#include "emposid.h"


namespace visSurvey { class MarchingCubesDisplay; class PolygonBodyDisplay; 
		      class RandomPosBodyDisplay; }


mDefineItem( BodyDisplayParent, TreeItem, TreeTop, mShowMenu mMenuOnAnyButton );


mClass uiODBodyDisplayTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODBodyDisplayParentTreeItem; }
    uiTreeItem*		create(int visid,uiTreeItem*) const;
};


mClass uiODBodyDisplayTreeItem : public uiODDisplayTreeItem
{
public:
    			uiODBodyDisplayTreeItem( int, bool dummy );
    			uiODBodyDisplayTreeItem( const EM::ObjectID& emid );
    			~uiODBodyDisplayTreeItem();

protected:
    void		prepareForShutdown();
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    void		colorChCB(CallBacker*);

    bool		init();
    const char*		parentType() const
			{return typeid(uiODBodyDisplayParentTreeItem).name();}

    EM::ObjectID			emid_;
    MenuItem				savemnuitem_;
    MenuItem				saveasmnuitem_;
    MenuItem				displaybodymnuitem_;
    MenuItem				displaypolygonmnuitem_;
    MenuItem				displayintersectionmnuitem_;
    MenuItem				removeselectedmnuitem_;
    MenuItem				displaymnuitem_;
    MenuItem				singlecolormnuitem_;
    visSurvey::MarchingCubesDisplay*	mcd_;
    visSurvey::PolygonBodyDisplay*	plg_;
    visSurvey::RandomPosBodyDisplay*	rpb_;
};


#endif
