#ifndef uitoolbar_h
#define uitoolbar_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2001
 RCS:           $Id: uitoolbar.h,v 1.34 2008-07-04 04:21:34 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiparent.h"

class ioPixmap;
class MenuItem;
class uiObject;
class uiToolBarBody;
class QToolBar;

class uiToolBar : public uiParent
{
public:
    //! ToolBar Dock Identifier
    /*
	Toolbars can be created on docks,
    */
    enum ToolBarArea 
    { 
	    Left=0x1,	//!< To the left of the central uiGroup.
	    Right=0x2,	//!< To the right of the central uiGroup.
	    Top=0x4,	//!< Above the central uiGroup, below the menubar.
	    Bottom=0x8,	//!< Below the central uiGroup, above the status bar.
	    None=0,	//!< No toolbar area
	    All=0xf	//!< All areas.
    };

    			uiToolBar(uiParent*,const char* nm,
				  ToolBarArea d=Top,bool newline=false);
			~uiToolBar();

    int 		addButton(const char*,const CallBack&,
				  const char* tooltip,bool toggle=false);
    int 		addButton(const ioPixmap&,const CallBack&,
				  const char* tooltip,bool toggle=false);
    int			addButton(const MenuItem&);
    void		addObject(uiObject*);

    void		setLabel(const char*);

    void		setPixmap(int,const char*);
    void		setPixmap(int,const ioPixmap&);
    void		setToolTip(int,const char*);
    void		setShortcut(int,const char*);
    void		turnOn(int idx,bool yn);
    			/*!< Does only work on toggle-buttons */
    bool		isOn(int idx) const;
    			/*!< Does only work on toggle-buttons */
    void		setSensitive(int idx,bool yn);
    			/*!< Does only work on buttons */
    void		setSensitive(bool yn);
    			/*!< Works on complete toolbar */

    virtual void	display(bool yn=true,bool s=false,bool m=false);
			/*!< you must call this after all buttons are added
			     s and m are not used.
			*/
    bool		isHidden() const;
    bool		isVisible() const;

    void		addSeparator();

    void		reLoadPixMaps();
    void		clear();

    virtual uiMainWin*  mainwin();

    static ToolBarArea	pluginArea()		{ return uiToolBar::Right; }
    ToolBarArea		prefArea() const	{ return tbarea_; }
    QToolBar*		qwidget()		{ return qtoolbar_; }

    const ObjectSet<uiObject>& 		objectList() const;
    static ObjectSet<uiToolBar>&	toolBars();

protected:

    QToolBar*		qtoolbar_;
    uiToolBarBody*	body_;
    uiToolBarBody&	mkbody(const char*,QToolBar&);

    uiParent*		parent_;
    ToolBarArea		tbarea_;

};

#endif
