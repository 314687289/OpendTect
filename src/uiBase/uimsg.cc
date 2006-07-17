/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id: uimsg.cc,v 1.30 2006-07-17 15:36:23 cvsbert Exp $
________________________________________________________________________

-*/


#include "uimsg.h"

#include "uicursor.h"
#include "uimain.h"
#include "uimainwin.h"
#include "uistatusbar.h"
#include "uiobj.h"
#include "uibody.h"
#include "uiparentbody.h"

#undef Ok
#include <qmessagebox.h>
#include <iostream>

uiMsg* uiMsg::theinst_ = 0;


uiMsg::uiMsg()
	: uimainwin_(0)
{
}

uiMainWin* uiMsg::setMainWin( uiMainWin* m )
{
    uiMainWin* old = uimainwin_;
    uimainwin_ = m;
    return old;
}


uiStatusBar* uiMsg::statusBar()
{
    uiMainWin* mw = uimainwin_;/* ? uimainwin_ 
			       : uiMainWin::gtUiWinIfIsBdy( parent_ );*/

    if ( !mw || !mw->statusBar() )
	mw = uiMainWin::activeWindow();

    if ( !mw || !mw->statusBar() )
	mw = uiMain::theMain().topLevel();

    return mw ? mw->statusBar() : 0;
}


QWidget* uiMsg::popParnt()
{
    uiMainWin* mw = uiMainWin::activeWindow();
    if ( !mw ) mw = uimainwin_;
    if ( !mw ) mw = uiMain::theMain().topLevel();

    if ( !mw  )		return 0;
    return mw->body()->qwidget();
}


bool uiMsg::toStatusbar( const char* msg )
{
    if ( !statusBar() ) return false;

    statusBar()->message( msg );
    return true;
}


static BufferString& gtCaptn()
{
    static BufferString* captn = 0;
    if ( !captn )
	captn = new BufferString;
    return *captn;
}

void uiMsg::setNextCaption( const char* s )
{
    gtCaptn() = s;
}


#define mPrepCursor() \
    uiCursorChanger cc( uiCursor::Arrow )
#define mPrepTxt() \
    mPrepCursor(); \
    BufferString msg( text ); if ( p2 ) msg += p2; if ( p3 ) msg += p3
#define mCapt(s) QString( getCaptn(s) )
#define mTxt QString( msg.buf() )

static const char* getCaptn( const char* s )
{
    if ( gtCaptn() == "" )
	return s;

    static BufferString oldcaptn;
    oldcaptn = gtCaptn();
    gtCaptn() = "";

    return oldcaptn.buf();
}


#define mDeclArgs const char* text, const char* p2, const char* p3

void uiMsg::message( mDeclArgs )
{
    mPrepTxt();
    QMessageBox::information( popParnt(), mCapt("Information"), mTxt,
	    		      QString("&Ok") );
}


void uiMsg::warning( mDeclArgs )
{
    mPrepTxt();
    QMessageBox::warning( popParnt(), mCapt("Warning"), mTxt, QMessageBox::Ok,
			  QMessageBox::NoButton, QMessageBox::NoButton );
}


void uiMsg::error( mDeclArgs )
{
    mPrepTxt();
    QMessageBox::critical( popParnt(), mCapt("Error"), mTxt, QString("&Ok") );
}


int uiMsg::notSaved( const char* text, bool cancelbutt )
{
    mPrepCursor();
#if QT_VERSION < 0x030200
# define mQuestion information
#else
# define mQuestion question
#endif
    int res = QMessageBox::mQuestion( popParnt(), mCapt("Data not saved"),
	       QString(text), QString("&Save"), QString("&Don't save"),
	       cancelbutt ? QString("&Cancel") : QString::null, 0, 2 );

    return res == 0 ? 1 : (res == 1 ? 0 : -1);
}


void uiMsg::about( const char* text )
{
    mPrepCursor();
    QMessageBox::about( popParnt(), mCapt("About"), QString(text) );
}


bool uiMsg::askGoOn( const char* text, bool yn )
{
    mPrepCursor();
    return !QMessageBox::warning( popParnt(),
				  mCapt("Please specify"), QString(text),
				  QString(yn?"&Yes":"&Ok"),
				  QString(yn?"&No":"&Cancel"),
				  QString::null,0,1);
}



int uiMsg::askGoOnAfter( const char* text, const char* cnclmsg )
{
    mPrepCursor();
    if ( !cnclmsg || !*cnclmsg )
	cnclmsg = "&Cancel";
    return QMessageBox::warning( popParnt(), mCapt("Please specify"),
	    			 QString(text),
				 QString("&Yes"), QString("&No"),
				 QString(cnclmsg), 0, 2 );
}


bool uiMsg::showMsgNextTime( const char* text, const char* ntmsg )
{
    mPrepCursor();
    if ( !ntmsg || !*ntmsg )
	ntmsg = "&Don't show this message again";
    return !QMessageBox::warning( popParnt(), mCapt("Information"),
	    			  QString(text),
				  QString("&Ok"),
				  QString(ntmsg),
				  QString::null,0,1);
}
