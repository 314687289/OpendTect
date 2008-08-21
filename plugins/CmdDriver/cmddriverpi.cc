/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Sep 2003
 RCS:           $Id: cmddriverpi.cc,v 1.19 2008-08-21 12:35:53 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "cmddriver.h"

#include "uidialog.h"
#include "uifileinput.h"
#include "uimain.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmain.h"
#include "uiodmenumgr.h"

#include "envvars.h"
#include "filegen.h"
#include "filepath.h"
#include "ioman.h"
#include "oddirs.h"
#include "plugins.h"
#include "timer.h"

extern "C" int GetCmdDriverPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetCmdDriverPluginInfo()
{
    static PluginInfo retpii = {
	"Command driver",
	"dGB (Bert/Jaap)",
	"=od",
	"Used for testing and general 'scripting'." };
    return &retpii;
}

static const char* autoexecfnm = "autoexec.cmd";


class uiCmdDriverMgr : public CallBacker
{
public:
    			uiCmdDriverMgr(uiODMain&);

    uiODMain&		appl_;
    CmdDriver*		drv_;
    Timer*		tim_;
    uiPopupMenu*	cmddrvmnu_;
    uiMenuItem*		runcmddrvitm_;
    uiMenuItem*		nameguideitm_;
    void		handleAutoExecution();
    void		doIt(CallBacker*);
    void		autoStart(CallBacker*);
    void		delayedStart(CallBacker*);
    void		toolTipChange(CallBacker*);
    void		survChg(CallBacker*);

};


class uiCmdDriverInps : public uiDialog
{
public:

uiCmdDriverInps( uiParent* p, CmdDriver& d )
        : uiDialog(p,Setup("Command execution","Specify the file with commands"
			    " to execute","0.4.6"))
	, drv_(d)
{
    fnmfld = new uiFileInput( this, "Command file", uiFileInput::Setup(lastinp_)
				.filter("*.cmd")
				.forread(true)
				.withexamine(true) );
    fnmfld->setDefaultSelectionDir( GetScriptsDir(0) );

    FilePath fp( drv_.outputDir() ); fp.add( drv_.logFileName() );
    logfnmfld = new uiFileInput( this, "Output log file",
	    			uiFileInput::Setup(fp.fullPath())
				.forread(false) );
    logfnmfld->attach( alignedBelow, fnmfld );
    logfnmfld->setDefaultSelectionDir( drv_.outputDir() );
}

bool acceptOK( CallBacker* )
{
    FilePath fp( logfnmfld->fileName() );
    if ( !File_isWritable(fp.pathOnly()) )
    {
	uiMSG().error( "Output log file cannot be written to this directory" );
	return false;
    }

    BufferString fnm = fnmfld->fileName();
    if ( File_isEmpty(fnm) )
    {
	uiMSG().error( "Invalid command file selected" );
	return false;
    }
    if ( !drv_.getActionsFromFile(fnm) )
    {
	uiMSG().error( drv_.errMsg() );
	return false;
    }

    drv_.setOutputDir( fp.pathOnly() );
    drv_.setLogFileName( fp.fileName() );
    lastinp_ = fnm;
    return true;
}

    uiFileInput*	fnmfld;
    uiFileInput*	logfnmfld;
    BufferString	fnm;
    CmdDriver&		drv_;

    static BufferString	lastinp_;

};

BufferString uiCmdDriverInps::lastinp_;


uiCmdDriverMgr::uiCmdDriverMgr( uiODMain& a )
    	: appl_(a)
    	, drv_(0)
    	, tim_(0)
{
    uiODMenuMgr& mnumgr = appl_.menuMgr();
    cmddrvmnu_ = new uiPopupMenu( &appl_, "Command &Driver" );
    mnumgr.utilMnu()->insertItem( cmddrvmnu_ );
    runcmddrvitm_ = new uiMenuItem( "&Run ...",
				    mCB(this,uiCmdDriverMgr,doIt) );
    cmddrvmnu_->insertItem( runcmddrvitm_ );
    nameguideitm_ = new uiMenuItem( "&Tooltip name guide",
				    mCB(this,uiCmdDriverMgr,toolTipChange) );
    cmddrvmnu_->insertItem( nameguideitm_ );
    nameguideitm_->setCheckable( true );
    nameguideitm_->setChecked( GetEnvVarYN("DTECT_USE_TOOLTIP_NAMEGUIDE") );
    toolTipChange(0);

    handleAutoExecution();
}


void uiCmdDriverMgr::handleAutoExecution()
{
    appl_.justBeforeGo.notify( mCB(this,uiCmdDriverMgr,delayedStart) );

    BufferStringSet cmdline; uiMain::theMain().getCmdLineArgs( cmdline );
    for ( int idx=1; idx<cmdline.size(); idx++ )
    {
	char* str = cmdline.get(idx).buf();
	char* ptr = strchr( str, '=' );
	if ( !ptr ) continue;
	*ptr++ = '\0';
	if ( strcmp(str,"cmd") ) continue;

	BufferString fnm( ptr );
	if ( !ptr || fnm == "none" )
	    return;

	if ( !File_exists(fnm) && !FilePath(fnm).isAbsolute() )
	    fnm = GetProcFileName( fnm );
	if ( !File_exists(fnm) )
	{
	    FilePath fp( fnm );
	    fp.setExtension( "cmd", true );
	    fnm = fp.fullPath();
	}
	if ( File_exists(fnm) )
	    { uiCmdDriverInps::lastinp_ = fnm; return; }
    }

    FilePath fp( GetSettingsDir() );
    fp.add( autoexecfnm );
    BufferString fnm( fp.fullPath() );
    if ( File_exists(fnm) )
	uiCmdDriverInps::lastinp_ = fnm;
    else
    {
	fnm = GetProcFileName( autoexecfnm );
	if ( File_exists(fnm) )
	    uiCmdDriverInps::lastinp_ = fnm;
    }
}


void uiCmdDriverMgr::delayedStart( CallBacker* )
{
    delete tim_;
    tim_ = new Timer( "CmdDriver startup" );
    tim_->start( 1500, true );
    tim_->tick.notify( mCB(this,uiCmdDriverMgr,autoStart) );
}


void uiCmdDriverMgr::autoStart( CallBacker* cb )
{
    IOM().afterSurveyChange.notify( mCB(this,uiCmdDriverMgr,survChg) );
    if ( !uiCmdDriverInps::lastinp_.isEmpty() )
	doIt( 0 );
}



void uiCmdDriverMgr::survChg( CallBacker* cb )
{
    if ( CmdDriver::nowExecuting() )
	return;

    const BufferString fnm( GetProcFileName(autoexecfnm) );
    if ( File_exists(fnm) )
    {
	uiCmdDriverInps::lastinp_ = fnm;
	doIt( 0 );
    }
}

void uiCmdDriverMgr::doIt( CallBacker* cb )
{
    if ( drv_ ) delete drv_;
    drv_ = new CmdDriver( cmddrvmnu_ );
    bool res;
    if ( !cb )
	res = drv_->getActionsFromFile( uiCmdDriverInps::lastinp_ );
    else
    {
	uiCmdDriverInps* dlg = new uiCmdDriverInps( &appl_, *drv_ );
	res = dlg->go();
	delete dlg;
    }
    
    if ( res )
	drv_->execute();
    else
	{ delete drv_; drv_ = 0; }
}


void uiCmdDriverMgr::toolTipChange( CallBacker* cb )
{
    uiObject::useNameToolTip( nameguideitm_->isChecked() );
}


extern "C" const char* InitCmdDriverPlugin( int, char** )
{
    (void)new uiCmdDriverMgr( *ODMainWin() );
    return 0;
}
