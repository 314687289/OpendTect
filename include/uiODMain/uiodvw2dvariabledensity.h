#ifndef uiodvw2dvariabledensity_h
#define uiodvw2dvariabledensity_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodvw2dtreeitem.h"

#include "datapack.h"
#include "menuhandler.h"

class uiMenuHandler;
class VW2DSeis;
namespace ColTab { class Sequence; };


mExpClass(uiODMain) uiODVW2DVariableDensityTreeItem : public uiODVw2DTreeItem
{ mODTextTranslationClass(uiODVW2DVariableDensityTreeItem);
public:
    				uiODVW2DVariableDensityTreeItem();
				~uiODVW2DVariableDensityTreeItem();

    bool                	select();
    bool                        showSubMenu();

    static int			cPixmapWidth()			{ return 16; }
    static int			cPixmapHeight()			{ return 10; }

protected:

    bool			init();
    void			initColTab();
    void			displayMiniCtab(const ColTab::Sequence*);
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
    bool			isSelectable() const            { return true; }

    VW2DSeis*			dummyview_;
    uiMenuHandler*		menu_;
    MenuItem			selattrmnuitem_;
    bool			coltabinitialized_;
   
    void			createSelMenu(MenuItem&);
    bool    			handleSelMenu(int mnuid);

    void			checkCB(CallBacker*);
    void			colTabChgCB(CallBacker*);
    void			dataChangedCB(CallBacker*);
    void			deSelectCB(CallBacker*);
    void			createMenuCB(CallBacker*);
    void			handleMenuCB(CallBacker*);
};


mExpClass(uiODMain) uiODVW2DVariableDensityTreeItemFactory
				: public uiODVw2DTreeItemFactory
{
public:
    const char*		name() const		{ return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODVW2DVariableDensityTreeItem(); }
    uiTreeItem*         createForVis(const uiODViewer2D&,int visid) const;
};


#endif

