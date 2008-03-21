/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          12/05/2004
 RCS:           $Id: uicursor.cc,v 1.10 2008-03-21 14:37:24 cvskris Exp $
________________________________________________________________________

-*/

#include "uicursor.h"
#include "pixmap.h"

#include "qcursor.h"
#include "qapplication.h"


void uiCursorManager::initClass()
{
    static uiCursorManager uimgr;
    MouseCursorManager::setMgr( &uimgr );
}

uiCursorManager::uiCursorManager()
{ } 


uiCursorManager::~uiCursorManager()
{}


void uiCursorManager::fillQCursor( const MouseCursor& mc, QCursor& qcursor )
{
    if ( mc.shape_==MouseCursor::Bitmap )
    {
	ioPixmap pixmap( mc.filename_ );
	qcursor = QCursor( *pixmap.qpixmap(), mc.hotx_, mc.hoty_ );
    }
    else
    {
	const Qt::CursorShape qshape = (Qt::CursorShape)(int) mc.shape_;
	qcursor.setShape( qshape );
    }
}


void uiCursorManager::setOverrideShape( MouseCursor::Shape sh, bool replace )
{
    Qt::CursorShape qshape = (Qt::CursorShape)(int) sh;
    QCursor qcursor;
    qcursor.setShape( qshape );
    QApplication::setOverrideCursor( qcursor,  replace );
}


void uiCursorManager::setOverrideFile( const char* fn, int hotx, int hoty,
				       bool replace )
{
    ioPixmap pixmap( fn );
    QApplication::setOverrideCursor( QCursor( *pixmap.qpixmap(), hotx, hoty ),
	    			     replace );
}


void uiCursorManager::setOverrideCursor( const MouseCursor& mc, bool replace )
{
    QCursor qcursor;
    fillQCursor( mc, qcursor );
    QApplication::setOverrideCursor( qcursor, replace );
}



void uiCursorManager::restoreInternal()
{
    QApplication::restoreOverrideCursor();
}
