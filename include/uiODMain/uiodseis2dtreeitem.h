#ifndef uiodseis2dtreeitem_h
#define uiodseis2dtreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodseis2dtreeitem.h,v 1.7 2008-02-04 07:11:22 cvsraman Exp $
________________________________________________________________________


-*/

#include "uiodattribtreeitem.h"
#include "uioddisplaytreeitem.h"
#include "multiid.h"
#include "ranges.h"


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

    void		createAttrMenu(uiMenuHandler*);
    bool                isExpandable() const            { return true; }
    const char*         parentType() const;

    Interval<float>	curzrg_;
    MultiID             setid_;
    RefMan<uiMenuHandler> menuhandler_;

    MenuItem            addlinesitm_;
    MenuItem            zrgitm_;
    MenuItem            addattritm_;
    MenuItem            removeattritm_;
    MenuItem		editcoltabitm_;
    MenuItem            editattritm_;
    MenuItem            showitm_;
    MenuItem            hideitm_;
    MenuItem            showlineitm_;
    MenuItem            hidelineitm_;
    MenuItem            showlblitm_;
    MenuItem            hidelblitm_;
    MenuItem            removeitm_;
    MenuItem            storeditm_;
    MenuItem            coltabselitm_;
};


class uiOD2DLineSetSubItem : public uiODDisplayTreeItem
{
public:
			uiOD2DLineSetSubItem(const char* nm,int displayid=-1);

    bool		addStoredData(const char*);
    void		addAttrib(const Attrib::SelSpec& );
    void		showLineName(bool);
    void		setZRange(const Interval<float>);
    void		removeAttrib(const char*);

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
    void			clearAttrib();

protected:
    void			createMenuCB(CallBacker*);
    void			handleMenuCB(CallBacker*);

    MenuItem			storeditm_;
    MenuItem			attrnoneitm_;
};


#endif
