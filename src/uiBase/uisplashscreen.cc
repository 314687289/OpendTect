/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2006
________________________________________________________________________

-*/


#include "uisplashscreen.h"

#include "uimainwin.h"
#include "uipixmap.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QSplashScreen>

mUseQtnamespace

uiSplashScreen::uiSplashScreen( const uiPixmap& pm )
{
    QDesktopWidget* qdw = QApplication::desktop();
    QWidget* parent = qdw->screen( qdw->primaryScreen() );
    qsplashscreen_ = new QSplashScreen( parent, *pm.qpixmap() );
}


uiSplashScreen::~uiSplashScreen()
{ delete qsplashscreen_; }

void uiSplashScreen::show()
{ qsplashscreen_->show(); }

void uiSplashScreen::finish( uiMainWin* mw )
{ qsplashscreen_->finish( mw->getParentWidget() ); }

void uiSplashScreen::showMessage( const char* msg )
{ qsplashscreen_->showMessage( msg ); }
