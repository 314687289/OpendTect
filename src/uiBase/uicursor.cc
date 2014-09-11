/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          12/05/2004
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uicursor.h"
#include "uipixmap.h"
#include "uirgbarray.h"

#include <QCursor>
#include <QApplication>

mUseQtnamespace

void uiCursorManager::initClass()
{
    mDefineStaticLocalObject( uiCursorManager, uimgr, );
    MouseCursorManager::setMgr( &uimgr );
}


uiCursorManager::uiCursorManager()
{} 


uiCursorManager::~uiCursorManager()
{}


uiPoint uiCursorManager::cursorPos()
{ return uiPoint( QCursor::pos().x(), QCursor::pos().y() ); }


void uiCursorManager::fillQCursor( const MouseCursor& mc,
				   QCursor& qcursor )
{
    if ( mc.shape_==MouseCursor::Bitmap )
    {
	if ( !mc.filename_.isEmpty() )
	{
	    uiPixmap pixmap( mc.filename_ );
	    qcursor = QCursor( *pixmap.qpixmap(), mc.hotx_, mc.hoty_ );
	}
	else
	{
	    uiPixmap pixmap( uiRGBArray(*mc.image_)) ;
	    qcursor = QCursor( *pixmap.qpixmap(), mc.hotx_, mc.hoty_ );
	}
    }
    else
    {
	const Qt::CursorShape qshape =
	    			     (Qt::CursorShape)(int) mc.shape_;
	qcursor.setShape( qshape );
    }
}


static bool prioritycursoractive_ = false;
static MouseCursor::Shape overrideshape_ = MouseCursor::NotSet;


void uiCursorManager::setPriorityCursor( MouseCursor::Shape mcshape )
{
    if ( prioritycursoractive_ )
	unsetPriorityCursor();

    setOverride( mcshape );
    prioritycursoractive_ = true;
}


void uiCursorManager::unsetPriorityCursor()
{
    if ( !prioritycursoractive_ )
	return;

    prioritycursoractive_ = false;
    restoreOverride();
}


MouseCursor::Shape uiCursorManager::overrideCursorShape()
{ return overrideshape_; }


static void setOverrideQCursor( const QCursor& qcursor, bool replace )
{
    overrideshape_ = (MouseCursor::Shape) qcursor.shape();

    QCursor topcursor;
    const bool stackwasempty = !QApplication::overrideCursor();
    if ( !stackwasempty )
	topcursor = *QApplication::overrideCursor();

    if ( prioritycursoractive_ && !stackwasempty )
	QApplication::restoreOverrideCursor();

    if ( replace )
	QApplication::changeOverrideCursor( qcursor );
    else
	QApplication::setOverrideCursor( qcursor );

    if ( prioritycursoractive_ && !stackwasempty )
	QApplication::setOverrideCursor( topcursor );
}


void uiCursorManager::setOverrideShape( MouseCursor::Shape sh, bool replace )
{
    Qt::CursorShape qshape = (Qt::CursorShape)(int) sh;
    QCursor qcursor;
    qcursor.setShape( qshape );
    setOverrideQCursor( qcursor, replace );
}


void uiCursorManager::setOverrideFile( const char* fn, int hotx, int hoty,
				       bool replace )
{
    uiPixmap pixmap( fn );
    QCursor qcursor( *pixmap.qpixmap(), hotx, hoty );
    setOverrideQCursor( qcursor, replace );
}


void uiCursorManager::setOverrideCursor( const MouseCursor& mc, bool replace )
{
    QCursor qcursor;
    fillQCursor( mc, qcursor );
    setOverrideQCursor( qcursor, replace );
}


#define mStoreOverrideShape() \
{ \
    overrideshape_ = MouseCursor::NotSet; \
    if ( QApplication::overrideCursor() ) \
    { \
	const QCursor overridecursor = \
					*QApplication::overrideCursor(); \
	overrideshape_ = (MouseCursor::Shape) overridecursor.shape(); \
    } \
}

void uiCursorManager::restoreInternal()
{
    if ( !QApplication::overrideCursor() )
	return;

    const QCursor topcursor =
				     *QApplication::overrideCursor();
    QApplication::restoreOverrideCursor();

    if ( !prioritycursoractive_ )
    {
	mStoreOverrideShape();
	return;
    }

    QApplication::restoreOverrideCursor();
    mStoreOverrideShape();
    QApplication::setOverrideCursor( topcursor );
}
