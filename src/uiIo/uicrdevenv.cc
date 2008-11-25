/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          Jan 2004
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uicrdevenv.cc,v 1.27 2008-11-25 15:35:25 cvsbert Exp $";

#include "uicrdevenv.h"

#include "uidesktopservices.h"
#include "uifileinput.h"
#include "uimain.h"
#include "uimsg.h"

#include "envvars.h"
#include "filegen.h"
#include "filepath.h"
#include "ioman.h"
#include "oddirs.h"
#include "settings.h"
#include "strmprov.h"

#ifdef __win__
# include "winutils.h"
#endif

static void showProgrDoc()
{
    FilePath fp( mGetProgrammerDocDir() );
    fp.add( __iswin__ ? "windows.html" : "unix.html" );
    uiDesktopServices::openUrl( fp.fullPath() );
}

#undef mHelpFile


uiCrDevEnv::uiCrDevEnv( uiParent* p, const char* basedirnm,
			const char* workdirnm, const char* cygwin )
	: uiDialog(p,uiDialog::Setup("Create Work Enviroment",
		    		     "Specify a work directory",
		    		     "8.0.1"))
	, workdirfld(0)
	, basedirfld(0)
{

    const char* titltxt =
    "For OpendTect development you'll need a $WORK dir\n"
    "Please specify where this directory should be created.";

    setTitleText( titltxt );

    basedirfld = new uiFileInput( this, "Parent Directory",
			  uiFileInput::Setup(basedirnm).directories(true) );

    workdirfld = new uiGenInput( this, "Directory name", workdirnm );
    workdirfld->attach( alignedBelow, basedirfld );

}


bool uiCrDevEnv::isOK( const char* datadir )
{
    FilePath datafp( datadir );

    if ( !datafp.nrLevels() ) return false;

    if ( !datafp.nrLevels() || !File_isDirectory( datafp.fullPath() ) )
	return false;

    datafp.add( "Pmake" );
    if ( !File_isDirectory(datafp.fullPath()) )
	return false;

    datafp.set( datadir ).add( "include" );
    if ( !File_isDirectory(datafp.fullPath()) )
	return false;

    datafp.set( datadir ).add( "plugins" );
    if ( !File_isDirectory(datafp.fullPath()) )
	return false;

    return true;
}


#undef mErrRet
#define mErrRet(s) { uiMSG().error(s); return; }

void uiCrDevEnv::crDevEnv( uiParent* appl )
{
    FilePath oldworkdir( GetEnvVar("WORK") );
    const bool oldok = isOK( oldworkdir.fullPath() );

    const char* cygwin = 0;

#ifdef __win__

    cygwin = getCygDir();

    if ( !cygwin )
    {
	const char* msg =
	    "Cygwin installation not found."
	    "Please close OpendTect and use the Cygwin installer\n"
	    "you can find in the start-menu under"
	    "\"Start-Programs-OpendTect-Install Cygwin\""
	    "\nIf you are sure you have cygwin installed, "
	    "you can safely continue."
	    "\n\nDo you want to continue anyway?";

	if ( !uiMSG().askGoOn(msg) )
	{
	    const char* closemsg = 
		"Please run the Cygwin (bash) shell "
		"at least once after the installation.\n\n"
		"This will make sure you have a Cygwin home "
		"directory for your work environment.\n\n"
		"Please refer to the documentation that will pop up to "
		"see which packages to install." 
		"\n\nIt is required to close OpendTect before installing"
		"Cygwin.\nDo you want to close OpendTect now?";

	    const bool close = uiMSG().askGoOn( closemsg );

	    showProgrDoc();

	    if ( close )
	    {
		if( uiMain::theMain().topLevel() )
		   uiMain::theMain().topLevel()->close();
		else
		   uiMain::theMain().exit();
	    }

	    return;
	}
    }

#endif

    BufferString workdirnm;

    if ( File_exists(oldworkdir.fullPath()) )
    {
	BufferString msg = "Your current work directory (";
	msg += oldworkdir.fullPath();
	msg += oldok ?  ") seems to be a valid work directory.\n" :
			") does not seem to be a valid work directory.\n";
	msg += "Do you want to completely remove the existing directory\n"
	       "and create a new work directory there?";

	if ( uiMSG().askGoOn(msg) )
	{
	    File_remove( workdirnm, mFile_Recursive );
	    workdirnm = oldworkdir.fullPath();
	}
    }

    if ( workdirnm.isEmpty() )
    {
	BufferString worksubdirm = "ODWork";
	BufferString basedirnm = GetPersonalDir();
	if ( cygwin )
	{
	    FilePath fp( cygwin ); fp.add( "home" );
	    if ( GetEnvVar("USERNAME") )
		fp.add( GetEnvVar("USERNAME") );
	    else if ( GetEnvVar("USER") )
		fp.add( GetEnvVar("USER") );

	    basedirnm = fp.fullPath();
	    if ( !File_isDirectory(basedirnm) )
	    {
		const char* msg =
		"You have installed Cygwin but seem to have never used it.\n"
		"Unfortunately, this means you have no Cygwin home directory.\n"
		"\nWe advise to close OpendTect and start a Cygwin shell.\n"
		"Then use this utility again.\n"
		"\nDo you still wish to continue?";

		if ( !uiMSG().askGoOn(msg) )
		    return;

		basedirnm = GetPersonalDir();
	    }
	}

	// pop dialog
	uiCrDevEnv dlg( appl, basedirnm, worksubdirm, cygwin );
	if ( !dlg.go() ) return;

	basedirnm = dlg.basedirfld->text();
	worksubdirm = dlg.workdirfld->text();

	if ( !File_isDirectory(basedirnm) )
	    mErrRet( "Invalid directory selected" )

	workdirnm = FilePath( basedirnm ).add( worksubdirm ).fullPath();
    }

    if ( workdirnm.isEmpty() ) return;
	
    if ( File_exists(workdirnm) )
    {
	BufferString msg;
	const bool isdir= File_isDirectory( workdirnm );
	const bool isok = isOK(workdirnm);

	if ( isdir )
	{
	    msg = "The directory you selected(";
	    msg += workdirnm;
	    msg += isok ? ") seems to already be a work directory.\n\n" :
			  ") does not seem to be a valid work directory.\n\n";
	}
	else
	{
	    msg = "You selected a file.\n\n";
	}

	msg += "Do you want to completely remove the existing";
	msg + isdir ?  "directory\n" : "file\n" ;
	msg += "and create a new work directory there?";   

	if ( !uiMSG().askGoOn(msg) )
	    return;

	File_remove( workdirnm, mFile_Recursive );
    }


    if ( !File_createDir( workdirnm, 0 ) )
	mErrRet( "Cannot create the new directory.\n"
		 "Please check if you have the required write permissions" )

    const char* aboutto =
	"The OpendTect window will FREEZE during this process\n"
	"- for upto a few minutes.\n\n"
	"Meanwhile, please take a look at the developers documentation."
    ;

    uiMSG().message(aboutto);

    showProgrDoc();

    BufferString cmd( "@'" );
    FilePath fp( GetSoftwareDir() );
    fp.add( "bin" ).add( "od_cr_dev_env" );
    cmd += fp.fullPath();
    cmd += "' '"; cmd += GetSoftwareDir();
    cmd += "' '"; cmd += workdirnm; cmd += "'";

    StreamProvider( cmd ).executeCommand( false );

    BufferString relfile = FilePath(workdirnm).add(".rel.od.doc").fullPath();
    if ( !File_exists(relfile) )
	mErrRet( "Creation seems to have failed" )
    else
	uiMSG().message( "Creation seems to have succeeded.\n\n"
			 "Source 'init.csh' or 'init.bash' before starting." );
}



#undef mErrRet
#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiCrDevEnv::acceptOK( CallBacker* )
{
    BufferString workdir = basedirfld->text();
    if ( workdir.isEmpty() || !File_isDirectory(workdir) )
	mErrRet( "Please enter a valid (existing) location" )

    if ( workdirfld )
    {
	BufferString workdirnm = workdirfld->text();
	if ( workdirnm.isEmpty() )
	    mErrRet( "Please enter a (sub-)directory name" )

	workdir = FilePath( workdir ).add( workdirnm ).fullPath();
    }

    if ( !File_exists(workdir) )
    {
#ifdef __win__
	if ( !strncasecmp("C:\\Program Files", workdir, 16)
	  || strstr( workdir, "Program Files" )
	  || strstr( workdir, "program files" )
	  || strstr( workdir, "PROGRAM FILES" ) )
	    mErrRet( "Please do not try to use 'Program Files' for data.\n"
		     "A directory like 'My Documents' would be good." )
#endif
    }

    return true;
}
