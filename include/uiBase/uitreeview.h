#ifndef uilistview_h
#define uilistview_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          29/01/2002
 RCS:           $Id: uitreeview.h,v 1.35 2009-01-09 04:26:14 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiobj.h"
#include "bufstringset.h"
#include "keyenum.h"
#include "draw.h"


class QTreeWidget;
class QTreeWidgetItem;
class uiListViewBody;
class uiListViewItem;
class ioPixmap;

mClass uiListView : public uiObject
{
    friend class		i_listVwMessenger;
    friend class		uiListViewBody;
    friend class		uiListViewItem;

public:
			uiListView(uiParent* parnt,
				   const char* nm="uiListView",
				   int preferredNrLines=0,
				   bool rootdecorated=true);

    virtual		~uiListView()			{}

			// 0: use nr itms in list
    uiListViewBody& 	mkbody(uiParent*,const char*,int);
    void		setLines(int prefNrLines);

    enum		ScrollMode { Auto, AlwaysOff, AlwaysOn };
    void		setHScrollBarMode(ScrollMode);
    void		setVScrollBarMode(ScrollMode);

    void		setTreeStepSize(int);

    bool		rootDecorated() const;
    void		setRootDecorated(bool yn);

    // take & insert are meant to MOVE an item to another point in the tree 
    void		takeItem(uiListViewItem*);
    void		insertItem(int,uiListViewItem*);

    void		addColumns(const BufferStringSet&);
    int			nrColumns() const;

    void		removeColumn(int index );
    void		setColumnText(int column,const char* label);
    const char*		columnText(int column) const;
    void		setColumnWidth(int column,int width);
    int			columnWidth(int column) const;

    enum		WidthMode { Manual, Maximum };
    void		setColumnWidthMode(int column,WidthMode);
    WidthMode		columnWidthMode(int column) const;

    void		setColumnAlignment(int,OD::Alignment);
    OD::Alignment	columnAlignment(int) const;

    void		ensureItemVisible(const uiListViewItem*);

    enum		SelectionMode
    			{ NoSelection=0, Single, Multi, Extended, Contiguous };
    enum		SelectionBehavior 
			{ SelectItems, SelectRows, SelectColumns };
    void		setSelectionMode(SelectionMode);
    SelectionMode	selectionMode() const;
    void		setSelectionBehavior(SelectionBehavior);
    SelectionBehavior	selectionBehavior() const;

    void		clearSelection();
    void		setSelected(uiListViewItem*,bool);
    bool		isSelected(const uiListViewItem*) const;
    uiListViewItem*	selectedItem() const;

    void		setCurrentItem(uiListViewItem*,int column=0);

    uiListViewItem*	currentItem() const;
    uiListViewItem*	getItem(int) const; 
    uiListViewItem*	firstItem() const;
    uiListViewItem*	lastItem() const;

    int			nrItems() const;

    void		setItemMargin(int);
    int			itemMargin() const;

    void		setShowToolTips(bool);
    bool		showToolTips() const;

    uiListViewItem*	findItem(const char*,int,bool) const;
    uiParent*		parent()		{ return parent_; }

    void		clear();
    void		invertSelection();
    void		selectAll();
    void		expandAll();
    void		collapseAll();

			//! re-draws at next X-loop
    void		triggerUpdate();

			//! item last notified. See notifiers below
    uiListViewItem*	itemNotified()		{ return lastitemnotified_; }
    void		unNotify()		{ lastitemnotified_ = 0; }
    int			columnNotified()	{ return column_; }

    Notifier<uiListView> selectionChanged;
    Notifier<uiListView> currentChanged;
    Notifier<uiListView> itemChanged;
    Notifier<uiListView> returnPressed;
    Notifier<uiListView> rightButtonClicked;
    Notifier<uiListView> rightButtonPressed;
    Notifier<uiListView> leftButtonClicked;
    Notifier<uiListView> leftButtonPressed;
    Notifier<uiListView> mouseButtonPressed;
    Notifier<uiListView> mouseButtonClicked;
    Notifier<uiListView> contextMenuRequested;
    //! the user moves the mouse cursor onto the item
    Notifier<uiListView> onItem;
    Notifier<uiListView> itemRenamed;
    Notifier<uiListView> expanded;
    Notifier<uiListView> collapsed;
    Notifier<uiListView> unusedKey;

    			//! Force activation in GUI thread
    void		activateClick(uiListViewItem&,int column,
				      bool leftclick=true);
    void		activateButton(uiListViewItem&,bool expand);
    Notifier<uiListView> activatedone;

protected:

    mutable BufferString rettxt;
    uiListViewItem*	lastitemnotified_;
    uiParent*		parent_;
    int			column_;
    OD::ButtonState     buttonstate_;

    void 		cursorSelectionChanged( CallBacker* );
    void		setNotifiedItem( QTreeWidgetItem* );
    void		setNotifiedColumn( int col )	{ column_ = col; }

    uiListViewBody*		lvbody()	{ return body_; }
    const uiListViewBody*	lvbody() const	{ return body_; }

private:

    uiListViewBody*	body_;

};


/*!

*/

mClass uiListViewItem : public CallBacker
{
public:

    enum			Type { Standard, CheckBox };

    mClass Setup
    {
    public:
				Setup( const char* txt=0, 
				       uiListViewItem::Type tp=
						uiListViewItem::Standard,
				       bool setchecked=true )
				: type_(tp)
				, after_(0)
				, pixmap_(0)
				, setcheck_(setchecked)
				{ label( txt ); }

	mDefSetupMemb(uiListViewItem::Type,type)
	mDefSetupMemb(uiListViewItem*,after)
	mDefSetupMemb(const ioPixmap*,pixmap)
	mDefSetupMemb(bool,setcheck)
	BufferStringSet		labels_;

	Setup& 			label( const char* txt ) 
				{ 
				    if( txt ) 
					labels_ += new BufferString( txt ); 
				    return *this; 
				}
    };

			uiListViewItem(uiListViewItem* parent,const Setup&); 
			uiListViewItem(uiListView* parent,const Setup&);
			~uiListViewItem();

    QTreeWidgetItem*	qItem()			{ return qtreeitem_; }
    const QTreeWidgetItem* qItem() const	{ return qtreeitem_; }

    int			nrChildren() const;

    void		setCheckable(bool);
    bool		isCheckable() const;
    void		setChecked(bool,bool trigger=false);
    			//!< does nothing if not checkable
    bool		isChecked() const;  //!< returns false if not checkable

    void		setToolTip(int column,const char*);

    void		insertItem(int,uiListViewItem*);
    void		takeItem(uiListViewItem*);
    void		removeItem(uiListViewItem*);
    void		moveItem(uiListViewItem* after);
    int			siblingIndex() const;
    			/*!<\returns this items index of it's siblings. */

    void		setText( const char*, int column=0 );
    void		setText( int i, int column=0 )
			{ setText( toString(i), column ); }
    void		setText( float f, int column=0 )
			{ setText( toString(f), column ); }
    void		setText( double d, int column=0 )
			{ setText( toString(d), column ); }

    const char*		text( int column=0 ) const;

    void		setPixmap(int column,const ioPixmap&);

    virtual const char* key(int,bool) const		{ return 0; }
    virtual int		compare( uiListViewItem*,int column,bool) const
							{ return mUdf(int); }

    void		setOpen(bool yn=true);
    bool		isOpen() const;

    void		setSelected(bool yn);
    bool		isSelected() const;

    uiListViewItem*	getChild(int) const;
    uiListViewItem*	firstChild() const;
    uiListViewItem*	lastChild() const;
    uiListViewItem*	nextSibling() const;
    uiListViewItem*	parent() const;

    uiListViewItem*	itemAbove();
    uiListViewItem*	itemBelow();

    uiListView*		listView() const;

    void		setSelectable(bool yn);
    bool		isSelectable() const;

    void		setDragEnabled(bool);
    void		setDropEnabled(bool);
    bool		dragEnabled() const;
    bool		dropEnabled() const;

    void		setVisible(bool yn);
    bool		isVisible() const;

    void		setRenameEnabled(int column,bool);
    bool		renameEnabled(int column) const;

    void		setEnabled(bool);
    bool		isEnabled() const;


    Notifier<uiListViewItem> stateChanged; //!< only works for CheckBox type
    Notifier<uiListViewItem> keyPressed;
    			//!< passes CBCapsule<const char*>* cb
    			//!< If you handle it, set cb->data = 0;

    static QTreeWidgetItem*	 qitemFor(uiListViewItem*);
    static const QTreeWidgetItem*  qitemFor(const uiListViewItem*);

    static uiListViewItem* 	 itemFor(QTreeWidgetItem*);
    static const uiListViewItem* itemFor(const QTreeWidgetItem*);

protected:

    QTreeWidgetItem*		qtreeitem_;
    mutable BufferString	rettxt;

    void			init(const Setup&);

    bool			isselectable_;
    bool			iseditable_;
    bool			isdragenabled_;
    bool			isdropenabled_;
    bool			ischeckable_;
    bool			isenabled_;
    void			updateFlags();

};

#endif
