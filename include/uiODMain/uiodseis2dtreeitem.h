#ifndef uiodseis2dtreeitem_h
#define uiodseis2dtreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodseis2dtreeitem.h,v 1.1 2006-05-09 11:00:53 cvsbert Exp $
________________________________________________________________________


-*/

#include "uiodattribtreeitem.h"
#include "uioddisplaytreeitem.h"
#include "multiid.h"


mDefineItem( Seis2DParent, TreeItem, TreeTop, mShowMenu );


class Seis2DTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODSeis2DParentTreeItem; }
    uiTreeItem*		create(int visid,uiTreeItem*) const;
};


class uiOD2DLineSetTreeItem : public uiODTreeItem
{
public:
    			uiOD2DLineSetTreeItem(const MultiID&);
    void		selectAddLines();
    bool		showSubMenu();

    const MultiID&	lineSetID() const { return setid_; }

protected:
    			~uiOD2DLineSetTreeItem();
    bool                init();

    void                createMenuCB(CallBacker*);
    void                handleMenuCB(CallBacker*);

    bool                isExpandable() const            { return true; }
    const char*         parentType() const;

    MultiID             setid_;
    RefMan<uiMenuHandler> menuhandler_;

    MenuItem            addlinesitm_;
    MenuItem            showitm_;
    MenuItem            hideitm_;
    MenuItem            showlineitm_;
    MenuItem            hidelineitm_;
    MenuItem            showlblitm_;
    MenuItem            hidelblitm_;
    MenuItem            removeitm_;
    MenuItem            storeditm_;
    MenuItem            selattritm_;
    MenuItem            coltabselitm_;
};


class uiOD2DLineSetSubItem : public uiODDisplayTreeItem
{
public:
			uiOD2DLineSetSubItem(const char* nm,int displayid=-1);

    bool		displayStoredData(const char*);
    void		setAttrib(const Attrib::SelSpec& );
    void		showLineName(bool);

protected:
			~uiOD2DLineSetSubItem();
    bool		init();
    const char*		parentType() const;
    BufferString	createDisplayName() const;

    uiODDataTreeItem*	createAttribItem(const Attrib::SelSpec*) const;

    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    void		getNewData(CallBacker*);

private:
    MenuItem		linenmitm_;
    MenuItem		positionitm_;
};


class uiOD2DLineSetAttribItem : public uiODAttribTreeItem
{
public:
				uiOD2DLineSetAttribItem(const char* parenttype);
    bool			displayStoredData(const char*);
    void			setAttrib(const Attrib::SelSpec& );

protected:
    void			createMenuCB(CallBacker*);
    void			handleMenuCB(CallBacker*);

    MenuItem			storeditm_;
    MenuItem			attrnoneitm_;
};


#endif
