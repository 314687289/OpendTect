#ifndef uiodemsurftreeitem_h
#define uiodemsurftreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodemsurftreeitem.h,v 1.8 2009-01-08 10:47:25 cvsranojay Exp $
________________________________________________________________________


-*/

#include "uiodattribtreeitem.h"
#include "uioddisplaytreeitem.h"
#include "emposid.h"
class uiVisEMObject;
class uiODDataTreeItem;


mClass uiODEarthModelSurfaceTreeItem : public uiODDisplayTreeItem
{
public:

    uiVisEMObject*	visEMObject() const	{ return uivisemobj_; }

protected:
    			uiODEarthModelSurfaceTreeItem(const EM::ObjectID&);
    			~uiODEarthModelSurfaceTreeItem();
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);

    uiODDataTreeItem*	createAttribItem(const Attrib::SelSpec*) const;

    void		finishedEditingCB(CallBacker*);
    void		prepareForShutdown();

    EM::ObjectID	emid_;
    uiVisEMObject*	uivisemobj_;

    MenuItem		createflatscenemnuitem_;

private:
    bool		init();
    virtual void	initNotify() {};

    virtual void	checkCB(CallBacker*);

    bool		treeitemwasenabled_;
    bool		prevtrackstatus_;

    MenuItem		savemnuitem_;
    MenuItem		saveasmnuitem_;
    MenuItem		enabletrackingmnuitem_;
    MenuItem		changesetupmnuitem_;
    MenuItem		reloadmnuitem_;
    MenuItem		starttrackmnuitem_;
};


mClass uiODEarthModelSurfaceDataTreeItem : public uiODAttribTreeItem
{
public:
    			uiODEarthModelSurfaceDataTreeItem( EM::ObjectID,
				       uiVisEMObject*, const char* parenttype );
protected:
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    BufferString	createDisplayName() const;

    MenuItem		depthattribmnuitem_;
    MenuItem		savesurfacedatamnuitem_;
    MenuItem		loadsurfacedatamnuitem_;
    MenuItem		algomnuitem_;
    MenuItem		fillholesmnuitem_;
    MenuItem		filtermnuitem_;

    bool		changed_;
    EM::ObjectID	emid_;
    uiVisEMObject*	uivisemobj_;
};


#endif
