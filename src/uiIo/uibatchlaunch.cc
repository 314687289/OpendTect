/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          January 2002
 RCS:           $Id: uibatchlaunch.cc,v 1.66 2008-08-04 11:41:56 cvsraman Exp $
________________________________________________________________________

-*/

#include "uibatchlaunch.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uispinbox.h"

#include "filegen.h"
#include "filepath.h"
#include "hostdata.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "oddirs.h"
#include "ptrman.h"
#include "strmdata.h"
#include "strmprov.h"
#include "ascstream.h"

static const char* sSingBaseNm = "batch_processing";
static const char* sMultiBaseNm = "cube_processing";


static void getProcFilename( const char* basnm, const char* altbasnm,
			     BufferString& tfname )
{
    if ( !basnm || !*basnm ) basnm = altbasnm;
    tfname = basnm;
    cleanupString( tfname.buf(), NO, NO, YES );
    tfname += ".par";
    tfname = GetProcFileName( tfname );
}


static bool writeProcFile( IOPar& iop, const char* tfname )
{
    const_cast<IOPar&>(iop).set( sKey::Survey, IOM().surveyName() );
    if ( !iop.write(tfname,sKey::Pars) )
    {
	BufferString msg = "Cannot write to:\n"; msg += tfname;
	uiMSG().error( msg );
	return false;
    }

    return true;
}

#ifdef HAVE_OUTPUT_OPTIONS

uiBatchLaunch::uiBatchLaunch( uiParent* p, const IOPar& ip,
			      const char* hn, const char* pn, bool wp )
        : uiDialog(p,uiDialog::Setup("Batch launch","Specify output mode",
		   "0.1.4"))
	, iop_(*new IOPar(ip))
	, hostname_(hn)
	, progname_(pn)
{
    finaliseDone.notify( mCB(this,uiBatchLaunch,remSel) );
    HostDataList hdl;
    rshcomm_ = hdl.rshComm();
    if ( rshcomm_.isEmpty() ) rshcomm_ = "rsh";
    nicelvl_ = hdl.defNiceLevel();

    BufferString dispstr( "Remote (using " );
    dispstr += rshcomm_; dispstr += ")";
    remfld_ = new uiGenInput( this, "Execute",
			     BoolInpSpec(true,"Local",dispstr) );
    remfld_->valuechanged.notify( mCB(this,uiBatchLaunch,remSel) );

    opts_.add( "Output window" ).add( "Log file" ).add( "Standard output" );
    if ( wp )
	opts_.add( "Parameter report (no run)" );
    optfld_ = new uiLabeledComboBox( this, opts_, "Output to" );
    optfld_->attach( alignedBelow, remfld_ );
    optfld_->box()->setCurrentItem( 0 );
    optfld_->box()->selectionChanged.notify( mCB(this,uiBatchLaunch,optSel) );

    StringListInpSpec spec;
    for ( int idx=0; idx<hdl.size(); idx++ )
	spec.addString( hdl[idx]->name() );
    remhostfld_ = new uiGenInput( this, "Hostname", spec );
    remhostfld_->attach( alignedBelow, remfld_ );

    static BufferString fname = "";
    if ( fname.isEmpty() )
    {
	fname = GetProcFileName( "log" );
	if ( GetSoftwareUser() )
	    { fname += "_"; fname += GetSoftwareUser(); }
	fname += ".txt";
    }
    filefld_ = new uiFileInput( this, "Log file",
	   		       uiFileInput::Setup(fname)
				.forread(false)
	   			.filter("*.log;;*.txt") );
    filefld_->attach( alignedBelow, optfld_ );

    nicefld_ = new uiLabeledSpinBox( this, "Nice level" );
    nicefld_->attach( alignedBelow, filefld_ );
    nicefld_->box()->setInterval( 0, 19 );
    nicefld_->box()->setValue( nicelvl_ );
}


uiBatchLaunch::~uiBatchLaunch()
{
    delete &iop_;
}


bool uiBatchLaunch::execRemote() const
{
    return !remfld_->getBoolValue();
}


void uiBatchLaunch::optSel( CallBacker* )
{
    const int sel = selected();
    filefld_->display( sel == 1 || sel == 3 );
}


void uiBatchLaunch::remSel( CallBacker* )
{
    bool isrem = execRemote();
    remhostfld_->display( isrem );
    optfld_->display( !isrem );
    optSel(0);
}


void uiBatchLaunch::setParFileName( const char* fnm )
{
    parfname_ = fnm;
    FilePath fp( fnm );
    fp.setExtension( "log", true );
    filefld_->setFileName( fp.fullPath() );
}


int uiBatchLaunch::selected()
{
    return execRemote() ? 1 : optfld_->box()->currentItem();
}


bool uiBatchLaunch::acceptOK( CallBacker* )
{
    const bool dormt = execRemote();
    if ( dormt )
    {
	hostname_ = remhostfld_->text();
	if ( hostname_.isEmpty() )
	{
	    uiMSG().error( "Please specify the name of the remote host" );
	    return false;
	}
    }

    const int sel = selected();
    BufferString fname = sel == 0 ? "window"
		       : (sel == 2 ? "stdout" : filefld_->fileName());
    if ( fname.isEmpty() ) fname = "/dev/null";
    iop_.set( sKey::LogFile, fname );
    iop_.set( sKey::Survey, IOM().surveyName() );

    if ( selected() == 3 )
    {
	iop_.set( sKey::LogFile, "stdout" );
	if ( !iop_.write(fname,sKey::Pars) )
	{
	    uiMSG().error( "Cannot write parameter file" );
            return false;
	}
	return true;
    }

    if ( parfname_.isEmpty() )
	getProcFilename( sSingBaseNm, sSingBaseNm, parfname_ );
    if ( !writeProcFile(iop_,parfname_) )
	return false;

    BufferString comm( "@" );
    comm += GetExecScript( dormt );
    if ( dormt )
    {
	comm += hostname_;
	comm += " --rexec ";
	comm += rshcomm_;
    }

    const bool inbg=dormt;
#ifdef __win__ 

    comm += " --inbg "; comm += progname_;
    FilePath parfp( parfname_ );

    BufferString _parfnm( parfp.fullPath(FilePath::Unix) );
    replaceCharacter(_parfnm.buf(),' ','%');
    comm += " \""; comm += _parfnm; comm += "\"";

#else

    nicelvl_ = nicefld_->box()->getValue();
    if ( nicelvl_ != 0 )
	{ comm += " --nice "; comm += nicelvl_; }
    comm += " "; comm += progname_;
    comm += " -bg "; comm += parfname_;

#endif

    if ( !StreamProvider( comm ).executeCommand(inbg) )
    {
	uiMSG().error( "Cannot start batch program" );
	return false;
    }
    return true;
}

#endif // HAVE_OUTPUT_OPTIONS


uiFullBatchDialog::uiFullBatchDialog( uiParent* p, const Setup& s )
    : uiDialog(p,uiDialog::Setup(s.wintxt_,"X",0).oktext("Proceed")
						 .modal(s.modal_)
						 .menubar(s.menubar_))
    , uppgrp_(new uiGroup(this,"Upper group"))
    , procprognm_(s.procprognm_.isEmpty() ? "process_attrib" : s.procprognm_)
    , multiprognm_(s.multiprocprognm_.isEmpty() ? "SeisMMBatch"
					       : s.multiprocprognm_)
    , redo_(false)
    , parfnamefld_(0)
    , singmachfld_( 0 )
{
    setParFileNmDef( 0 );
}


void uiFullBatchDialog::addStdFields( bool forread, bool onlysinglemachine )
{
    uiGroup* dogrp = new uiGroup( this, "Proc details" );
    if ( !redo_ )
    {
	uiSeparator* sep = new uiSeparator( this, "Hor sep" );
	sep->attach( stretchedBelow, uppgrp_ );
	dogrp->attach( alignedBelow, uppgrp_ );
	dogrp->attach( ensureBelow, sep );
    }

    if ( !onlysinglemachine )
    {
	singmachfld_ = new uiGenInput( dogrp, "Submit to",
		    BoolInpSpec(true,"Single machine","Multiple machines") );
	singmachfld_->valuechanged.notify(
		mCB(this,uiFullBatchDialog,singTogg) );
    }
    const char* txt = redo_ ? "Processing specification file"
			    : "Store processing specification as";
    parfnamefld_ = new uiFileInput( dogrp,txt, uiFileInput::Setup(singparfname_)
					       .forread(forread)
					       .filter("*.par;;*")
					       .confirmoverwrite(false) );
    if ( !onlysinglemachine )
	parfnamefld_->attach( alignedBelow, singmachfld_ );

    dogrp->setHAlignObj( parfnamefld_ );
}


void uiFullBatchDialog::setParFileNmDef( const char* nm )
{
    getProcFilename( nm, sSingBaseNm, singparfname_ );
    getProcFilename( nm, sMultiBaseNm, multiparfname_ );
    if ( parfnamefld_ )
	parfnamefld_->setFileName( !singmachfld_ || singmachfld_->getBoolValue()
		? singparfname_
		: multiparfname_ );
}

void uiFullBatchDialog::singTogg( CallBacker* cb )
{
    const BufferString inpfnm = parfnamefld_->fileName();
    const bool issing = !singmachfld_ || singmachfld_->getBoolValue();
    if ( issing && inpfnm == multiparfname_ )
	parfnamefld_->setFileName( singparfname_ );
    else if ( !issing && inpfnm == singparfname_ )
	parfnamefld_->setFileName( multiparfname_ );
}


bool uiFullBatchDialog::acceptOK( CallBacker* cb )
{
    if ( !prepareProcessing() ) return false;
    const bool issing = !singmachfld_ || singmachfld_->getBoolValue();

    BufferString fnm = parfnamefld_ ? parfnamefld_->fileName()
				    : ( issing ? singparfname_.buf()
					       : multiparfname_.buf() );
    if ( fnm.isEmpty() )
	getProcFilename( 0, "tmp_proc", fnm );
    else if ( !FilePath(fnm).isAbsolute() )
	getProcFilename( fnm, sSingBaseNm, fnm );

    if ( redo_ )
    {
	if ( File_isEmpty(fnm) )
	{
	    uiMSG().error( "Invalid (epty or not readable) specification file" );
	    return false;
	}
    }
    else if ( File_exists(fnm) )
    {
	if ( !File_isWritable(fnm) )
	{
	    BufferString msg( "Cannot write specifications to\n", fnm.buf(),
		"\nPlease select another file, or make sure file is writable.");
	    uiMSG().error( msg );
	    return false;
	}
    }

    PtrMan<IOPar> iop = 0;
    if ( redo_ )
    {
	if ( issing )
	{
	    StreamData sd = StreamProvider( fnm ).makeIStream();
	    if ( !sd.usable() )
		{ uiMSG().error( "Cannot open parameter file" ); return false; }
	    ascistream aistrm( *sd.istrm, YES );
	    if ( strcmp(aistrm.fileType(),sKey::Pars) )
	    {
		sd.close();
		uiMSG().error(BufferString(fnm," is not a parameter file"));
		return false;
	    }
	    const float ver = atof( aistrm.version() );
	    if ( ver < 3.1999 )
	    {
		sd.close();
		uiMSG().error( "Invalid parameter file: pre-3.2 version");
		return false;
	    }
	    iop = new IOPar; iop->getFrom( aistrm );
	    sd.close();
	}
    }
    else
    {
	iop = new IOPar( "Processing" );
	if ( !fillPar(*iop) )
	    return false;
    }

    if ( !issing && !redo_ && !writeProcFile(*iop,fnm) )
	return false;

    bool res = issing ? singLaunch( *iop, fnm ) : multiLaunch( fnm );
    return ctrlstyle_ == DoAndStay ? false : res; 
}


bool uiFullBatchDialog::singLaunch( const IOPar& iop, const char* fnm )
{
#ifdef HAVE_OUTPUT_OPTIONS
    uiBatchLaunch dlg( this, iop, 0, procprognm_, false );
    dlg.setParFileName( fnm );
    return dlg.go();
#else

    BufferString fname = "stdout";

    IOPar& workiop( const_cast<IOPar&>( iop ) );
    workiop.set( "Log file", fname );

    FilePath parfp( fnm );
    if ( !parfp.nrLevels() )
        parfp = singparfname_;
    if ( !writeProcFile(workiop,parfp.fullPath()) )
	return false;

    const bool dormt = false;
    BufferString comm( "@" );
    comm += GetExecScript( dormt );

#ifdef __win__ 
    comm += " --inxterm+askclose "; comm += procprognm_;

    BufferString _parfnm( parfp.fullPath(FilePath::Unix) );
    replaceCharacter(_parfnm.buf(),' ','%');

    comm += " \""; comm += _parfnm; comm += "\"";

#else
    comm += " "; comm += procprognm_;
    comm += " -bg "; comm += parfp.fullPath();
#endif

    const bool inbg=dormt;
    if ( !StreamProvider( comm ).executeCommand(inbg) )
    {
	uiMSG().error( "Cannot start batch program" );
	return false;
    }
    return true;

#endif

}


bool uiFullBatchDialog::multiLaunch( const char* fnm )
{
    BufferString comm( multiprognm_ );	comm += " ";
    comm += procprognm_;		comm += " \"";
    comm += fnm; 
    comm += "\"";

    if ( !ExecOSCmd( comm, true ) )
	{ uiMSG().error( "Cannot start multi-machine program" ); return false;}

    return true;
}


uiRestartBatchDialog::uiRestartBatchDialog( uiParent* p, const char* ppn,
       					    const char* mpn )
    	: uiFullBatchDialog(p,Setup("(Re-)Start processing")
				.procprognm(ppn).multiprocprognm(mpn))
{
    redo_ = true;
    setHelpID( "101.2.1" );
    setTitleText( "Run a saved processing job" );
    setCtrlStyle( DoAndLeave );
    addStdFields( true );
}
