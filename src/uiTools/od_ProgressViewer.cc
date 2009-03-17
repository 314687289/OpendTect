/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          August 2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: od_ProgressViewer.cc,v 1.22 2009-03-17 07:02:45 cvsranojay Exp $";

#include "uidesktopservices.h"
#include "uifiledlg.h"
#include "uifont.h"
#include "uigroup.h"
#include "uimain.h"
#include "uimainwin.h"
#include "uistatusbar.h"
#include "uitextedit.h"
#include "uitoolbar.h"

#include <iostream>
#include <ctype.h>
#include <signal.h>

#include "filepath.h"
#include "oddirs.h"
#include "prog.h"
#include "sighndl.h"
#include "strmprov.h"
#include "timer.h"

#define mBufLen 81


class uiProgressViewer : public uiMainWin
{
public:

    		uiProgressViewer(uiParent*,std::istream&,int);
    		~uiProgressViewer();

protected:

    void	quitFn(CallBacker*);
    void	helpFn(CallBacker*);
    void	saveFn(CallBacker*);

    uiToolBar*	tb_;

    std::istream& strm;
    uiTextEdit*	txtfld;
    int		quitid_;
    Timer*	tim;
    int		ppid;
    int		delay;
    bool	newlineseen;
    char	fullline[mBufLen];

    void	doWork(CallBacker*);
    bool	getChunk(char*, int);
    void	appendToText();
};


#define mAddButton(fnm,txt,fn) \
    tb_->addButton( fnm, mCB(this,uiProgressViewer,fn), txt, false );

uiProgressViewer::uiProgressViewer( uiParent* p, std::istream& s, int i )
	: uiMainWin(p,"Progress",1)
	, tim(0)
	, strm(s)
	, ppid(i)
	, delay(0)
	, newlineseen(false)
{
    fullline[0] = '\0';
    topGroup()->setBorder(0);
    topGroup()->setSpacing(0);

    tb_ = new uiToolBar( this, "ToolBar" );
    quitid_ = mAddButton( "stop.png", "Stop process and Quit", quitFn );
    mAddButton( "saveflow.png", "Save log", saveFn );
    mAddButton( "contexthelp.png", "Help", helpFn );

    txtfld = new uiTextEdit( this, "", true );
    uiFont& fnt = FontList().add( "Non-prop",
	    FontData(FontData::defaultPointSize(),"Courier") );
    txtfld->setFont( fnt );

    tim = new Timer( "Progress" );
    tim->tick.notify( mCB(this,uiProgressViewer,doWork) );
    tim->start( 50, true );
}


uiProgressViewer::~uiProgressViewer()
{
    delete tim;
}


void uiProgressViewer::appendToText()
{
    txtfld->append( fullline );
    uiMain::theMain().flushX();
    fullline[0] = '\0';
}


void uiProgressViewer::doWork( CallBacker* )
{
    bool ateof = strm.eof();
    if ( !isProcessAlive(ppid) )
    {
	txtfld->append( "Program Terminated" );
	tb_->setToolTip( quitid_, "Close" );
	ppid = 0;
	return;
    }

    if ( strm.eof() || strm.fail() )
    {
	appendToText();
	statusBar()->message( fullline );
	tb_->setToolTip( quitid_, "Close" );
	ppid = 0;
	return;
    }

    int orglen = strlen( fullline );
    static char buf[mBufLen];
    if ( getChunk( buf, mBufLen - orglen ) )
    {
	int len = buf[1] ? strlen( buf ) : 1;

	bool needappend = buf[len-1] == '\n';
	if ( !needappend )
	    needappend = len + orglen >= mBufLen - 1;
	else
	{
	    newlineseen = true;
	    buf[len--] = '\0';
	}

	// cat buf to fullline
	for ( int idx=0; idx<len; idx++ )
	    fullline[orglen+idx] = buf[idx];
	fullline[orglen+len] = '\0';

	if ( needappend )
	    appendToText();

	statusBar()->message( fullline );

    }

    tim->start( delay, true );
}


bool uiProgressViewer::getChunk( char* buf, int maxnr )
{
    int sz = 0;
    while ( 1 )
    {
	int c = strm.peek();
	if ( c == EOF )
	{
	    if ( !sz ) return false;
	    break;
	}

	strm.ignore( 1 );
	buf[sz] = (char)c;
	sz++;

	if ( !delay || (delay && isspace(buf[sz-1]))
	  || sz == maxnr || buf[sz-1] == '\n' )
	    break;

    }

    buf[sz] = '\0';
    if ( !newlineseen && fullline[0] == 'd' && fullline[1] == 'G' )
	delay = 1;

    return true;
}


void uiProgressViewer::quitFn( CallBacker* )
{
    if ( ppid )
	SignalHandling::stopProcess( ppid );
    uiMain::theMain().exit(0);
}


void uiProgressViewer::helpFn( CallBacker* )
{
    FilePath fp( mGetUserDocDir() ); fp.add( "base" ).add( "index.html" );
    uiDesktopServices::openUrl( fp.fullPath() );
}


void uiProgressViewer::saveFn( CallBacker* )
{
    uiFileDialog dlg( this, false, GetProcFileName("log.txt"),
	    	      "*.txt", "Save log" );
    dlg.setAllowAllExts( true );
    if ( dlg.go() )
    {
	StreamData sd( StreamProvider(dlg.fileName()).makeOStream() );
	if ( sd.usable() )
	   *sd.ostrm << txtfld->text() << std::endl;
	sd.close();
    }
}


int main( int argc, char** argv )
{
    uiMain app( argc, argv );
    int ppid = argc > 1 ? atoi(argv[1]) : 0;
    uiProgressViewer* pv = new uiProgressViewer( 0, std::cin, ppid );

    app.setTopLevel( pv );
    pv->show();
    ExitProgram( app.exec() ); return 0;
}
