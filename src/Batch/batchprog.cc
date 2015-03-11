/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 14-9-1998
 * FUNCTION : Batch Program 'driver'
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "batchprog.h"

#include "envvars.h"
#include "commandlineparser.h"
#include "file.h"
#include "genc.h"
#include "ioman.h"
#include "iodir.h"
#include "iopar.h"
#include "moddepmgr.h"
#include "strmprov.h"
#include "filepath.h"
#include "sighndl.h"
#include "hostdata.h"
#include "plugins.h"
#include "strmprov.h"
#include "ctxtioobj.h"
#include "jobcommunic.h"
#include "keystrs.h"
#include "ascstream.h"
#include "debug.h"

#include <iostream>

#ifndef __win__
# include "_execbatch.h"
#endif

#include "oddirs.h"


BatchProgram* BatchProgram::inst_ = 0;

BatchProgram& BP()
{
    if ( !BatchProgram::inst_ )
	BatchProgram::inst_ = new BatchProgram;
    return *BatchProgram::inst_;
}


void BatchProgram::deleteInstance()
{
    delete BatchProgram::inst_;
}


BatchProgram::BatchProgram()
    : NamedObject("")
    , stillok_(false)
    , finishmsg_("Finished batch processing.")
    , inbg_(false)
    , sdout_(*new StreamData)
    , iopar_(new IOPar)
    , comm_(0)
    , clparser_(0)
{
}


void BatchProgram::init()
{
    delete clparser_;

    OD::ModDeps().ensureLoaded( "Batch" );
    clparser_ = new CommandLineParser;
    clparser_->setKeyHasValue( sKeyMasterHost() );
    clparser_->setKeyHasValue( sKeyMasterPort() );
    clparser_->setKeyHasValue( sKeyJobID() );
    clparser_->setKeyHasValue( sKeyDataDir() );

    inbg_ = clparser_->hasKey( sKeyBG() );

    BufferString masterhost;
    clparser_->getVal( sKeyMasterHost(), masterhost );

    int masterport = -1;
    clparser_->getVal( sKeyMasterPort(), masterport );

    clparser_->getVal( sKeyJobID(), jobid_ );

    if ( masterhost.size() && masterport > 0 )  // both must be set.
	comm_ = new JobCommunic( masterhost, masterport, jobid_, sdout_ );

    BufferStringSet normalargs;
    clparser_->getNormalArguments( normalargs );

    BufferString parfilnm;
    for ( int idx=normalargs.size()-1; idx>=0; idx-- )
    {
	const FilePath parfp( normalargs.get(idx) );

	parfilnm = parfp.fullPath();
	parfilnm.replace( '%', ' ' );
	if ( File::exists( parfilnm ) )
	    break;

	parfilnm.setEmpty();
    }

    if ( parfilnm.isEmpty() )
    {
	BufferString msg( clparser_->getExecutableName() );
	msg += ": No existing parameter file name specified";

	errorMsg( msg );
	return;
    }

    setName( parfilnm );
    od_istream odstrm( parfilnm );
    if ( !odstrm.isOK() )
    {
	BufferString msg( clparser_->getExecutableName(),
		       ": Cannot open parameter file: ", parfilnm );
	odstrm.addErrMsgTo( msg );
	errorMsg( msg ); return;
    }

    ascistream aistrm( odstrm, true );
    if ( sKey::Pars() != aistrm.fileType() )
    {
	BufferString errmsg( clparser_->getExecutableName(),
			    ": Input file ",parfilnm);
	errmsg += " is not a parameter file";
	errorMsg( errmsg );
	od_cerr() << aistrm.fileType() << od_endl;
	return;
    }

    iopar_->getFrom( aistrm );
    odstrm.close();

    if ( iopar_->size() == 0 )
    {
	errorMsg( BufferString( clparser_->getExecutableName(),
			       ": Invalid input file: ",parfilnm) );
        return;
    }

    BufferString res = iopar_->find( sKey::LogFile() );
    if ( !res )
	iopar_->set( sKey::LogFile(), StreamProvider::sStdErr() );

    res = iopar_->find( sKey::DataRoot() );
    if ( !res.isEmpty() && File::exists(res) )
	SetEnvVar( "DTECT_DATA", res );

    if ( clparser_->getVal(sKeyDataDir(),res) && !res.isEmpty() &&
	 File::exists(res) )
	SetEnvVar( "DTECT_DATA", res );

    res = iopar_->find( sKey::Survey() );
    if ( res.isEmpty() )
	IOMan::newSurvey();
    else
    {
	if ( DBG::isOn(DBG_PROGSTART) )
	{
	    const char* oldsnm = IOM().surveyName();
	    if ( !oldsnm ) oldsnm = "<empty>";
	    if ( res!=oldsnm )
	    {
		BufferString msg( "Using survey from par file: ", res,
				  ". was: " ); msg += oldsnm;
		infoMsg( msg );
	    }
	}
	IOMan::setSurvey( res );
    }

    killNotify( true );

    stillok_ = true;
}


BatchProgram::~BatchProgram()
{
    infoMsg( finishmsg_ );
    IOM().applClosing();

    if ( comm_ )
    {

	JobCommunic::State s = comm_->state();

	bool isSet =  s == JobCommunic::AllDone
	           || s == JobCommunic::JobError
		   || s == JobCommunic::HostError;

	if ( !isSet )
	    comm_->setState( stillok_ ? JobCommunic::AllDone
				    : JobCommunic::HostError );

	bool ok = comm_->sendState( true );

	if ( ok )	infoMsg( "Successfully wrote final status" );
	else		infoMsg( "Could not write final status" );

	comm_->disConnect();
    }

    killNotify( false );

    sdout_.close();
    delete &sdout_;
    delete clparser_;

    // Do an explicit exitProgram() here, so we are sure the program
    // is indeed ended and we won't get stuck while cleaning up things
    // that we don't care about.
    ExitProgram( stillok_ ? 0 : 1 );

    // These will never be reached...
    delete iopar_;
    delete comm_;
}


void BatchProgram::progKilled( CallBacker* )
{
    infoMsg( "BatchProgram Killed." );

    if ( comm_ )
    {
	comm_->setState( JobCommunic::Killed );
	comm_->sendState( true );
    }

    killNotify( false );

#ifdef __debug__
    abort();
#endif
}


void BatchProgram::killNotify( bool yn )
{
    CallBack cb( mCB(this,BatchProgram,progKilled) );

    if ( yn )
	SignalHandling::startNotify( SignalHandling::Kill, cb );
    else
	SignalHandling::stopNotify( SignalHandling::Kill, cb );
}


bool BatchProgram::pauseRequested() const
    { return comm_ ? comm_->pauseRequested() : false; }


bool BatchProgram::errorMsg( const uiString& msg, bool cc_stderr )
{
    if ( sdout_.ostrm )
	*sdout_.ostrm << '\n' << msg.getFullString() << '\n' << std::endl;

    if ( !sdout_.ostrm || cc_stderr )
	std::cerr << '\n' << msg.getFullString() << '\n' << std::endl;

    if ( comm_ && comm_->ok() ) return comm_->sendErrMsg(msg.getFullString());

    return true;
}


bool BatchProgram::infoMsg( const char* msg, bool cc_stdout )
{
    if ( sdout_.ostrm )
	*sdout_.ostrm << '\n' << msg << '\n' << std::endl;

    if ( !sdout_.ostrm || cc_stdout )
	od_cout() << '\n' << msg << '\n' << od_endl;

    return true;
}



bool BatchProgram::initOutput()
{
    stillok_ = false;
    if ( comm_ && !comm_->sendPID(GetPID()) )
    {
	errorMsg( tr("Could not contact master. Exiting."), true );
	exit( 0 );
    }

    BufferString res = pars().find( sKey::LogFile() );
    if ( res == "stdout" ) res.setEmpty();

    bool hasviewprogress = true;
#ifdef __cygwin__
    hasviewprogress = false;
#endif

    if ( hasviewprogress && res && res=="window" )
    {
	BufferString cmd = "@";
	cmd += "\"";
	cmd += FilePath(GetBinPlfDir()).add("od_ProgressViewer").fullPath();
	cmd += "\" ";

	cmd += GetPID();
	StreamProvider sp( cmd );
	sdout_ = sp.makeOStream();
	if ( !sdout_.usable() )
	{
	    std::cerr << name() << ": Cannot open window for output"<<std::endl;
	    std::cerr << "Using std output instead" << std::endl;
	    res = 0;
	}
    }

    if ( res != "window" )
    {
	if ( res.isEmpty() )
	    res = StreamProvider::sStdErr();
	sdout_ = StreamProvider(res).makeOStream();
	if ( !sdout_.ostrm )
	{
	    std::cerr << name() << ": Cannot open log file" << std::endl;
	    std::cerr << "Using stderror instead" << std::endl;
	    sdout_.ostrm = &std::cerr;
	}
    }

    stillok_ = sdout_.usable();
    if ( stillok_ )
	PIM().loadAuto( true );
    return stillok_;
}


IOObj* BatchProgram::getIOObjFromPars(	const char* bsky, bool mknew,
					const IOObjContext& ctxt,
					bool msgiffail ) const
{
    BufferString errmsg;
    IOObj* ioobj = IOM().getFromPar( pars(), bsky, ctxt, mknew, errmsg );
    if ( !ioobj && msgiffail && !errmsg.isEmpty() )
	*sdout_.ostrm << errmsg.buf() << std::endl;

    return ioobj;
}

