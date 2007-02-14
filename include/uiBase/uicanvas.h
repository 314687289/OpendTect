#ifndef uicanvas_h
#define uicanvas_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uicanvas.h,v 1.15 2007-02-14 12:38:00 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidrawable.h"
#include "mouseevent.h"

class uiCanvasBody;
class uiScrollViewBody;

class uiCanvas : public uiDrawableObj
{
public:
				uiCanvas(uiParent*,const char *nm="uiCanvas");
    virtual			~uiCanvas()			{}

    void			update();

private:

    uiCanvasBody*		body_;
    uiCanvasBody&		mkbody(uiParent*,const char*);
};


class uiScrollView : public uiDrawableObj
{
public:

				uiScrollView(uiParent*,
					     const char *nm ="uiScrollView");
    virtual			~uiScrollView()			{}

    void			update();

    enum ScrollBarMode		{ Auto, AlwaysOff, AlwaysOn };
    void			setScrollBarMode(ScrollBarMode,bool hor);
    ScrollBarMode		getScrollBarMode(bool hor) const;

    void			resizeContents(int w,int h);
    inline void			resizeContents( uiSize s ) 
				    { resizeContents(s.hNrPics(),s.vNrPics()); }
    void			setContentsPos(uiPoint topleft);

    void			updateContents();
    void			updateContents(uiRect area,bool erase=true);
    uiRect			visibleArea() const;

    int				frameWidth() const;

    virtual void		rubberBandHandler(uiRect)	{}

    virtual void		setPrefWidth(int);
    virtual void		setPrefHeight(int);
    virtual void		setMaximumWidth(int);
    virtual void		setMaximumHeight(int);

    void			setRubberBandingOn(OD::ButtonState);
    OD::ButtonState		rubberBandingOn() const;
    void			setAspectRatio(float);
    float			aspectRatio();
 
    				// revieve mouse events w/o pressing button
    void			setMouseTracking(bool yn=true);

private:
    uiScrollViewBody*		body_;
    uiScrollViewBody&		mkbody(uiParent*,const char*);
};

#endif
