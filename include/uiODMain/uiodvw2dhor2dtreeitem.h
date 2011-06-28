#ifndef uiodvw2dhor2dtreeitem_h
#define uiodvw2dhor2dtreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
 RCS:		$Id: uiodvw2dhor2dtreeitem.h,v 1.8 2011-06-28 13:35:43 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uiodvw2dtreeitem.h"

#include "emposid.h"

class Vw2DHorizon2D;


mClass uiODVw2DHor2DParentTreeItem : public uiODVw2DTreeItem
{
public:
    				uiODVw2DHor2DParentTreeItem();
				~uiODVw2DHor2DParentTreeItem();

    bool			showSubMenu();

protected:

    bool			init();
    bool                        handleSubMenu(int);
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
    void			tempObjAddedCB(CallBacker*);
};


mClass uiODVw2DHor2DTreeItemFactory : public uiODVw2DTreeItemFactory
{
public:
    const char*		name() const 	{ return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODVw2DHor2DParentTreeItem(); }
    uiTreeItem*         createForVis(const uiODViewer2D&,int visid) const;
};


mClass uiODVw2DHor2DTreeItem : public uiODVw2DTreeItem
{
public:
    			uiODVw2DHor2DTreeItem(const EM::ObjectID&);
    			uiODVw2DHor2DTreeItem(int dispid,bool dummy);
			~uiODVw2DHor2DTreeItem();
    
    bool		showSubMenu();			
    bool		select();

protected:

    bool		init();
    const char*		parentType() const
			{ return typeid(uiODVw2DHor2DParentTreeItem).name(); }
    bool		isSelectable() const			{ return true; }

    void                updateSelSpec(const Attrib::SelSpec*,bool wva);    
    void		deSelCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		emobjAbtToDelCB(CallBacker*);
    void		mousePressInVwrCB(CallBacker*);
    void		mouseReleaseInVwrCB(CallBacker*);
    void		displayMiniCtab();

    const int		cPixmapWidth()				{ return 16; }
    const int		cPixmapHeight()				{ return 10; }
    void		emobjChangeCB(CallBacker*);

    EM::ObjectID	emid_;
    Vw2DHorizon2D*	horview_;
    bool		trackerefed_;
};

#endif
