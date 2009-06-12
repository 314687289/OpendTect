#ifndef uiflatviewthumbnail_h
#define uiflatviewthumbnail_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Aug 2007
 RCS:           $Id: uiflatviewthumbnail.h,v 1.5 2009-06-12 08:17:57 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"
class uiFlatViewer;
class uiRectItem;
class MouseEventHandler;


/*!\brief Shows a thumbnail with current position of a uiFlatViewer. */

mClass uiFlatViewThumbnail : public uiGraphicsView
{
public:
    			uiFlatViewThumbnail(uiParent*,uiFlatViewer&);
    			~uiFlatViewThumbnail();

    void		setColors(Color fg,Color bg);

    void		draw();
    void		draw(const uiWorldRect&);
    const uiFlatViewer&	viewer()		{ return viewer_; }

protected:

    void		getUiRect( const uiWorldRect&, uiRect& ) const;

    uiWorldRect*	feedbackwr_;
    uiFlatViewer&	viewer_;
    MouseEventHandler&	mousehandler_;
    Color		fgcolor_;
    Color		bgcolor_;

    uiRectItem*		bgrectitem_;
    uiRectItem*		fgrectitem_;

    void		reDrawHandler(uiRect);
    void		vwChg(CallBacker*);
    void		vwChging(CallBacker*);
    void		mouseRelCB(CallBacker*);
    void		mousePressCB(CallBacker*);
    void		mouseMoveCB(CallBacker*);
};

#endif
