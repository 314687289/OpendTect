#ifndef uiobj_H
#define uiobj_H

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          25/08/1999
 RCS:           $Id: uiobj.h,v 1.46 2007-05-03 18:13:24 cvskris Exp $
________________________________________________________________________

-*/

#include "uihandle.h"
#include "uigeom.h"
#include "uilayout.h"
#include "errh.h"

#include <stdlib.h>

class uiFont;
class uiCursor;
class uiObjectBody;
class uiParent;
class uiGroup;
class uiMainWin;
class uiButtonGroup;
class i_LayoutItem;
class ioPixmap;
class Color;
class QWidget;


/*!\ The base class for most UI elements. */

class uiObject : public uiObjHandle
{
    friend class	uiObjectBody;
    friend class	i_LayoutItem;
public:
			uiObject(uiParent*,const char* nm);
			uiObject(uiParent*,const char* nm,uiObjectBody&);
			~uiObject()			{}

/*! \brief How should the object's size behave? 
    Undef       : use default.
    Small       : 1 base sz.
    Medium      : 2* base sz + 1.
    Wide        : 4* base sz + 3.
    The xxVar options specify that the element may have a bigger internal
    preferred size. In that case, the maximum is taken.
    The xxMax options specify that the element should take all available
    space ( stretch = 2 )
*/
    enum		SzPolicy{ Undef, Small, Medium, Wide,
				  SmallVar, MedVar, WideVar,
				  SmallMax, MedMax, WideMax };


    void		setHSzPol(SzPolicy);
    void		setVSzPol(SzPolicy);
    SzPolicy		szPol( bool hor=true) const;

    virtual int		width() const;	//!< Actual size in pixels
    virtual int		height() const;	//!< Actual size in pixels

    void		setToolTip(const char*);
#ifdef USEQT3
    static void		enableToolTips(bool yn=true);
    static bool		toolTipsEnabled();
#endif

    void		display(bool yn = true,bool shrink=false,
				bool maximised=false);
    void		setFocus();
    bool		hasFocus() const;

    void		setCursor(const uiCursor&);

    const Color&	backgroundColor() const;
    void                setBackgroundColor(const Color&);
    void		setBackgroundPixmap(const char**);
    void		setBackgroundPixmap(const ioPixmap&);
    void		setSensitive(bool yn=true);
    bool		sensitive() const;

    int			prefHNrPics() const;
    virtual void	setPrefWidth(int);
    void                setPrefWidthInChar(float);
    int			prefVNrPics() const;
    virtual void	setPrefHeight(int);
    void		setPrefHeightInChar(float);

/*! \brief Sets stretch factors for object
    If stretch factor is > 1, then object will already grow at pop-up.
*/
    void                setStretch(int hor,int ver);


/*! \brief attaches object to another
    In case the stretched... options are used, margin=-1 (default) stretches
    the object not to cross the border.
    margin=-2 stretches the object to fill the parent's border. This looks nice
    with separators.
*/
    void		attach(constraintType,int margin=-1);
    void		attach(constraintType,uiObject*,int margin=-1,
				bool reciprocal=true);
    void		attach(constraintType,uiParent*,int margin=-1,
				bool reciprocal=true);

    static void		setTabOrder(uiObject* first, uiObject* second);

    void 		setFont(const uiFont&);
    const uiFont*	font() const;
    void		setCaption(const char*);


    void		shallowRedraw(CallBacker* =0)	{ reDraw( false ); }
    void		deepRedraw(CallBacker* =0)	{ reDraw( true ); }
    void		reDraw(bool deep);

    uiSize		actualsize(bool include_border=true) const;

    uiParent*		parent()			{ return parent_; }
    uiMainWin*		mainwin();
    QWidget*		qwidget();
    const QWidget*	qwidget() const
			{ return const_cast<uiObject*>(this)->qwidget(); }

    Notifier<uiObject>	closed;
			//!< Triggered when object closes.
    void		close();


			/*! \brief triggered when getting a new geometry 
			    A reference to the new geometry is passed 
			    which *can* be manipulated, before the 
			    geometry is actually set to the QWidget.
			*/
    CNotifier<uiObject,uiRect&>	setGeometry;

    static int		baseFldSize();

protected:

                        //! hook. Accepts/denies closing of window.
    virtual bool	closeOK()	{ closed.trigger(); return true; } 

			//! setGeometry should be triggered by this's layoutItem
    void 		triggerSetGeometry(const i_LayoutItem*, uiRect&);

private:

    static int		basefldsize_; 
    uiParent*		parent_;

};


#define mTemplTypeDef(fromclass,templ_arg,toclass) \
	typedef fromclass<templ_arg> toclass;
#define mTemplTypeDefT(fromclass,templ_arg,toclass) \
	mTemplTypeDef(fromclass,templ_arg,toclass)


/*! \mainpage Basic User Interface (uiBase)

  \section intro Introduction

This module is a set of cooperating classes that enable creating User
Interfaces. This layer on top of the already wonderful Qt package was created
because of the following problems:

- Qt provides an enormous set of tools, of which we need only a fraction
- On the other hand, Qt does not offer things like:
   - Selection of widget on basis of data characteristics
   - Automated 'black-box' layouting (enabling full grouping into new classes)
   - Integration with our data types
   - Generalised Callback/Notifier support (instead of Qt's Signal/Slot)
- If we use Qt directly, no isolation is present.

Therefore, as with most external libraries, we chose to make a new layer to
combine the power of Qt with the flexibility of more generalised design
principles.


\section usage Usage

The basic principles are:
- Objects are linked to each other in the window by attaching. This determines
  the layout of the window.
- Events are notified by specifically asking a Notifier in the class.
- All objects can be grouped; every group must be prepared to be attached to
  other UI elements; this is done by assigning one of the objects as being
  the 'align object'.
- 'Simple' data input should preferably be done through the uiGenInput
  class (see uiTools module).

The Qt window painting facilities are only used for quick sketching, the
code generation capacity is not used. Example:

<code>
    IntInpSpec spec( lastnrclasses );<br>
    spec.setLimits( Interval<int>( 0, 100 ) );<br>
    nrclassfld = new uiGenInput( this, "Number of classes", spec );<br>
<br>
    FloatInpSpec inpspec( lastratiotst*100 );<br>
    inpspec.setLimits( Interval<float>( 0, 100 ) );<br>
    perctstfld = new uiGenInput( this, "Percentage used for test set",
	    				inpspec );<br>
    perctstfld->attach( alignedBelow, nrclassfld );<br>
    <br>
    defbut = new uiPushButton( this, "Default" ); <br>
    defbut->activated.notify( mCB(this,ThisClass,setPercToDefault) ); <br>
    defbut->attach( rightOf, perctstfld );
</code>

Note that all objects could have been made:
- Conditional (only if a condition is met)
- Iterative (any number that may be necessary)
- As part of a group (e.g. to later treat as one uiObject)
- From a Factory (for example an object transforming a string into a uiObject).


\section design Design

In the uiBase directory, you'll find classes that directly communicate with Qt
to implement (parts of) this strategy. To keep the header files uncoupled
from the Qt header files, there is a mechanism where the 'ui' class has a
'ui'-Body class that is a subclass of a Qt class.

Almost every 'visible' object is a uiObject. Besides the different subclasses,
there is also the uiGroup which is just another uiObject. The windows holding
these are (like uiObjects) uiObjHandle's. The uiMainWin is a subclass, and
the ubiquitous uiDialog.

*/


#endif
