#ifndef uiodvw2dhor3dtreeitem_h
#define uiodvw2dhor3dtreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
 RCS:		$Id: uiodvw2dhor3dtreeitem.h,v 1.3 2010-09-02 08:56:14 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uiodvw2dtreeitem.h"

#include "emposid.h"

class Vw2DHorizon3D;


mClass uiODVw2DHor3DParentTreeItem : public uiODVw2DTreeItem
{
public:
    				uiODVw2DHor3DParentTreeItem();
				~uiODVw2DHor3DParentTreeItem();

    bool			showSubMenu();

protected:

    bool			init();
    bool                        handleSubMenu(int);
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
    void			tempObjAddedCB(CallBacker*);
};


mClass uiODVw2DHor3DTreeItemFactory : public uiTreeItemFactory
{
public:
    const char*		name() const		{ return typeid(*this).name(); }
    uiTreeItem*         create() const
			{ return new uiODVw2DHor3DParentTreeItem(); }
};


mClass uiODVw2DHor3DTreeItem : public uiODVw2DTreeItem
{
public:
    			uiODVw2DHor3DTreeItem(const EM::ObjectID&);
			~uiODVw2DHor3DTreeItem();

    bool		select();
    bool		showSubMenu();

protected:

    bool		init();
    const char*		parentType() const
    			{ return typeid(uiODVw2DHor3DParentTreeItem).name(); }
    bool		isSelectable() const			{ return true; }


    void		updateSelSpec(const Attrib::SelSpec*,bool wva);
    void		updateCS(const CubeSampling&,bool upd);
    void                checkCB(CallBacker*);
    void		deSelCB(CallBacker*);
    void		mousePressInVwrCB(CallBacker*);
    void		musReleaseInVwrCB(CallBacker*);
    void		msRelEvtCompletedInVwrCB(CallBacker*);
    void		displayMiniCtab();

    const int		cPixmapWidth()				{ return 16; }
    const int		cPixmapHeight()				{ return 10; }
    void		emobjChangeCB(CallBacker*);
    
    EM::ObjectID        emid_;
    Vw2DHorizon3D*	horview_;
    bool		oldactivevolupdated_;
    void                emobjAbtToDelCB(CallBacker*);
};

#endif
