#ifndef uidrawable_h
#define uidrawable_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          01/02/2000
 RCS:           $Id: uidrawable.h,v 1.8 2007-02-12 13:53:02 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiobj.h"
#include "iodraw.h"
#include "mouseevent.h"

class uiDrawableObj : public uiObject, public ioDrawArea
{
    mTTFriend(C,T,uiDrawableObjBody);
public:
			uiDrawableObj(uiParent* parnt,const char* nm, 
				      uiObjectBody&);
    virtual		~uiDrawableObj()	{}

    void		setRubberBandingOn( bool yn )	{ rubberbandon_ = yn; }
    bool		isRubberBandingOn() const	{ return rubberbandon_;}
    void		setRubberBandButton( OD::ButtonState bs )
			{ rubberbandbutton_ = bs; }
    OD::ButtonState	rubberBandButton()
    			{ return rubberbandbutton_; }

    Notifier<uiDrawableObj> preDraw;
    Notifier<uiDrawableObj> postDraw;
    Notifier<uiDrawableObj> reSized;

mProtected:

/*! \brief handler for additional redrawing stuff

reDrawHandler() is called from uiDrawableObjBody::handlePaintEvent, which

1) triggers preDraw on associated uiDrawableObj (i.e. this)

2) calls Qt's paintEvent handler on associated widget

3) calls reDrawHandler() on associated uiDrawableObj (i.e. this)

4) triggers postDraw on associated uiDrawableObj (i.e. this)

Subclasses can override this method to do some additional drawing.

\sa uiDrawableObjBody::handlePaintEvent( uiRect r, QPaintEvent* QPEv=0 )

*/
    virtual void        reDrawHandler( uiRect updateArea )      {}
    virtual void        reSizeHandler( uiSize, uiSize )         {}

    virtual ioDrawTool* drawTool_( int x0, int y0 );

    OD::ButtonState	rubberbandbutton_;
    bool		rubberbandon_;
};


#endif
