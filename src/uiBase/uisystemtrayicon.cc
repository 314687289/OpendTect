/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uisystemtrayicon.h"
#include "i_qsystemtrayicon.h"
#include "uistring.h"

#include "pixmap.h"

mUseQtnamespace

uiSystemTrayIcon::uiSystemTrayIcon( const uiPixmap& pm )
    : action_(-1)
    , messageClicked(this)
    , clicked(this)
    , rightClicked(this)
    , middleClicked(this)
    , doubleClicked(this)
{
    qsystemtrayicon_ = new QSystemTrayIcon();
    messenger_ = new QSystemTrayIconMessenger( qsystemtrayicon_,
	    						 this );
    setPixmap( pm );
}


uiSystemTrayIcon::~uiSystemTrayIcon()
{
    delete messenger_;
    delete qsystemtrayicon_;
}


void uiSystemTrayIcon::setPixmap( const uiPixmap& pm )
{
    QIcon qicon; 
    if ( pm.qpixmap() ) qicon = QIcon( *pm.qpixmap() );
    qsystemtrayicon_->setIcon( qicon );
}


void uiSystemTrayIcon::setToolTip( const uiString& tt )
{ qsystemtrayicon_->setToolTip( tt.getQtString() ); }

void uiSystemTrayIcon::show()
{ qsystemtrayicon_->show(); }

void uiSystemTrayIcon::hide()
{ qsystemtrayicon_->hide(); }
