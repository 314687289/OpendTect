#ifndef I_LAYOUT_H
#define I_LAYOUT_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          18/08/1999
 RCS:           $Id: i_layout.h,v 1.32 2003-03-05 12:09:34 arend Exp $
________________________________________________________________________

-*/

#include "uilayout.h"
#include "uigeom.h"
#include "uiobj.h"

#include <qlayout.h>
#include <qptrlist.h>
#include <qrect.h>

class resizeItem;
class Timer;

//!  internal enum used to determine in which direction a widget can be stretched and to check which outer limit must be checked
enum stretchLimitTp { left=1, right=2, above=4, below=8, 
                      rightLimit=16, bottomLimit=32 };


class uiConstraint
{
friend class i_LayoutItem;
public:

			uiConstraint ( constraintType t, i_LayoutItem* o,
				       int marg )
			: other(o), type(t), margin( marg ), enabled_( true )
			{
			    if( !other &&
				((type < leftBorder)||( type > hCentered))
			    )
				{ pErrMsg("No attachment defined!!"); }
			}

    bool		enabled()		{ return enabled_ ; }
    void		disable(bool yn=true)	{ enabled_ = !yn; }

protected:
    constraintType      type;
    i_LayoutItem*       other;
    int                 margin;
    bool		enabled_;
};

mTemplTypeDef(QPtrList,uiConstraint,constraintList)
mTemplTypeDef(QPtrListIterator,uiConstraint,constraintIterator)

class i_LayoutItem;


enum layoutMode { minimum=0, preferred=1, setGeom=2, all=3 };
       // all is used for setting cached positions dirty
const int nLayoutMode = 3;

//! dGB's layout manager
/*!
    This is our own layout manager for Qt. It manages widgets, etc. using
    constraints like "rightOf" etc.

Because the i_LayoutMngr is a QLayout, it can be used by 
QWidgets to automatically add new children to the manager when they are 
constructed with a QWidget (with layout) as parent. 

The actual adding to a manager is is done using QEvents.
Whenever a QObject inserts a new child, it posts a ChildInserted event
to itself. However, a QLayout constructor installs an event filter on its 
parent, and it registers itself to the parent as its layouter 
(setWidgetLayout), so future calls to the parent's sizeHint(), etc. are 
redirected to this new layoutmanager.

If setAutoAdd() is called on a layoutmanager, and the layout manager is 
"topLevel", i.e. THE manager for a certain widget, then whenever a new
widget is constructed with the manager's parent widget as parent,
the new widget is automatically added (by Qt) to the manager by the manager's 
eventfilter, using 'addItem( new QWidgetItem( w ) )'. 
Unfortunately, Qt does not call addItem before the main application loop
is running. This results to the problem that no attachments can be used until 
the main loop is running when we let Qt handle the addItem() calls. 
Therefore, we explicitily call addItem() on the correct layout manager at 
construction uiObjects. AutoAdd is also enabled in case someone wants to 
eses native Qt methods. (Multiple insertion is protected. Manager checks if 
widget already present).


*/
class i_LayoutMngr : public QLayout, public UserIDObject
{
    friend class	i_LayoutItem;
    friend class	uiGroupParentBody;
 //   friend class	i_uiGroupLayoutItem;

public:

			i_LayoutMngr( QWidget* prnt,
				      const char* name, uiObjectBody& mngbdy );

    virtual		~i_LayoutMngr();
 
    virtual void 	addItem( QLayoutItem*);
    void	 	addItem( i_LayoutItem* );

    virtual QSize 	sizeHint() const;
    virtual QSize 	minimumSize() const;

    virtual QLayoutIterator iterator();
    virtual QSizePolicy::ExpandData expanding() const;

    virtual void       	invalidate();
    virtual void       	updatedAlignment(layoutMode);
    virtual void       	initChildLayout(layoutMode);
	
    bool 		attach ( constraintType, QWidget&, QWidget*, int,
				 bool reciprocal=true );

    const uiRect&	curpos(layoutMode m) const;
    uiRect&		curpos(layoutMode m);
    uiRect		winpos(layoutMode m) const;

    void		forceChildrenRedraw( uiObjectBody*, bool deep );
    void		childrenClear( uiObject* );
    bool		isChild( uiObject* );

    int                 childStretch( bool hor ) const;

    int			borderSpace() const	{ return borderspc; }
    int			horSpacing() const;
    int			verSpacing() const	{ return vspacing; }

    void		setHSpacing( int s)	{ hspacing = s; }
    void		setVSpacing( int s)	{ vspacing = s; }
    void		setBorderSpace( int s)	{ borderspc = s; }

    void		setIsMain( bool yn )	    { ismain = yn; }
    void 		layoutChildren( layoutMode m, bool finalLoop=false );

private:

    void 		setGeometry( const QRect& );
 
    inline void 	doLayout( layoutMode m, const QRect& r ) const 
                        { const_cast<i_LayoutMngr*>(this)->doLayout(m,r); }
    void 		doLayout( layoutMode m, const QRect& );

    void	 	itemDel( CallBacker* );

    void 		moveChildrenTo( int , int, layoutMode );
    void 		fillResizeList( ObjectSet<resizeItem>&, bool ); 
    bool		tryToGrowItem( resizeItem&, const int, const int, 
				       int, int, const QRect&, int);
    void		resizeTo( const QRect& );
    void		childrenCommitGeometrySet(bool);

    uiRect 		childrenRect( layoutMode m );

    QPtrList<i_LayoutItem> childrenList;

    uiRect		layoutpos[ nLayoutMode ];
    QRect		prefGeometry;

    bool		minimumDone;
    bool		preferredDone;
    bool		prefposStored;
    bool		ismain;

    int 		hspacing;
    int 		vspacing;
    int 		borderspc;

    uiObjectBody& 	managedBody;

    void		touchPoptimer();
    void		popTimTick(CallBacker*);
    Timer&		poptimer;
    bool		popped_up;

};

#endif 
