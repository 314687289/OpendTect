/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          Oct 2004
 RCS:           $Id: jobiomgr.cc,v 1.5 2004-11-05 13:21:15 arend Exp $
________________________________________________________________________

-*/

#include "jobiomgr.h"
#include "filepath.h"
#include "debugmasks.h"
#include "strmprov.h"
#include "ioparlist.h"
#include "filegen.h"
#include "hostdata.h"
#include "ioman.h"
#include "jobinfo.h"
#include "mmdefs.h"
#include "queue.h"
#include "socket.h"
#include "errh.h"
#include "separstr.h"
#include "timefun.h"


#define sTmpStor	"Temporary storage directory"
#define sLogFil		"Log file"
#define sSurvey		"Survey"

#define mDebugOn	(DBG::isOn(DBG_MM))

#define mErrRet(msg) \
    {\
	msg_ = msg;\
	DBG::message(DBG_MM,msg);\
	return false;\
    }


class CommandString
{
public:
			CommandString( const char* init=0 ) 
			    : cmd( init ) {}

    void		addWoSpc( const char* txt ) { cmd += txt; }
    void		add( const char* txt ) { cmd += " "; cmd += txt; }
    void		addFlag( const char* f, const char* v )
			{
			    if ( !v || !*v ) return;
			    add(f); add(v);
			}
    void		addFlag( const char* f, int v )
			     { add(f); cmd += " "; cmd += v; }

    void		addFilePath( const FilePath& fp, FilePath::Style stl )
			{
			    cmd += " '";
			    cmd += fp.fullPathStyled(stl);
			    cmd += "'";
			}
    void		addFilePathFlag( const char* flag, const FilePath& fp,
	    			     FilePath::Style stl )
			{
			    cmd += " "; cmd += flag; cmd += " ";
			    addFilePath( fp, stl );
			}

    const BufferString&	string() { return cmd; }

    inline CommandString& operator=( const char* str )
			   { cmd = str; return *this; }
protected:
 
    BufferString	cmd;
};

/*!\brief Connects job to host.
 *
 */
class JobHostRespInfo
{
public:
			JobHostRespInfo( const HostData& hd, int descnr,
					 char resp = mRSP_WORK )
			: hostdata_(hd)
			, descnr_(descnr)
			, response_( resp ) {} 

    HostData		hostdata_;
    int			descnr_;
    char		response_;
};


/*!\brief Sets up a thread that waits for clients to connect.

  The task of the JobIOHandler is to maintain contact with clients.
  Most of the fuctionality could be implemented in the JobIOMgr,
  but we want to be absolutely sure the listening thread
  does not interfear with the normal exection thread's data.

  Therefore, all requests to and from the JobIOHandler are
  made mutex protected.
 
*/
class JobIOHandler : public CallBacker
{
public:
    			JobIOHandler( int firstport )
			    : sockprov_ ( *new SocketProvider( firstport ) )
			    , thread( 0 )
			    , exitreq_( false )
			    {
				threadmutex.lock();

				thread = new Threads::Thread(
					mCB(this,JobIOHandler,doDispatch) );

				threadmutex.unlock();
			    }

    virtual		~JobIOHandler()
			    {
				exitreq_ = true;
				if ( thread )
				{
				    thread->stop();
				}
				threadmutex.unlock();

				delete &sockprov_; 
			    }

    int			port()	{ return sockprov_.port(); }

    void		addJobDesc( const HostData& hd, int descnr )
			    {
				jhrespmutex_.lock();

				jobhostresps_ +=
				    new JobHostRespInfo( hd, descnr );

				jhrespmutex_.unlock();
			    }

    void		removeJobDesc( const char* hostnm, int descnr )
			    {
				jhrespmutex_.lock();

				JobHostRespInfo* jhri =
						getJHRFor( descnr, hostnm );

				if ( jhri )
				    { jobhostresps_ -= jhri; delete jhri; }
				    
				jhrespmutex_.unlock();
			    }

    void		reqModeForJob( const JobInfo&, JobIOMgr::Mode );

    ObjQueue<StatusInfo>& statusQueue() { return statusqueue_; }

protected:

    bool			exitreq_;
    SocketProvider& 		sockprov_;
    ObjQueue<StatusInfo>	statusqueue_;

    Threads::Mutex		jhrespmutex_;
    ObjectSet<JobHostRespInfo>	jobhostresps_;
    
    char			getRespFor( int desc, const char* hostnm );
    JobHostRespInfo*		getJHRFor( int desc, const char* hostnm );

    void			stopDispatching()
				{
				    if ( thread ) thread->threadExit();
				}

    void 			doDispatch( CallBacker* ); //!< work thread

    mThreadDeclaredMutexedVar(Threads::Thread*,thread);
};


JobHostRespInfo* JobIOHandler::getJHRFor( int descnr, const char* hostnm )
{
    JobHostRespInfo* jhri = 0;

    bool unlock = jhrespmutex_.tryLock();

    int sz = jobhostresps_.size();
    for ( int idx=0; idx < sz; idx++ )
    {
	JobHostRespInfo* jhri_ = jobhostresps_[idx];
	if ( jhri_->descnr_ == descnr )
	{
	    if ( !hostnm || !*hostnm )  { jhri = jhri_; break; }		

	    if ( jhri_->hostdata_.isKnownAs(hostnm) )
		{ jhri = jhri_; break; }		
	}
    }

    if ( unlock ) jhrespmutex_.unlock();

    return jhri;
}


char JobIOHandler::getRespFor( int descnr , const char* hostnm )
{
    char resp = mRSP_STOP;

    jhrespmutex_.lock();

    JobHostRespInfo* jhri = getJHRFor( descnr, hostnm );
    if ( jhri ) resp = jhri->response_;

    jhrespmutex_.unlock();

    return resp;
}


void JobIOHandler::reqModeForJob( const JobInfo& ji, JobIOMgr::Mode mode )
{
    char resp = mRSP_STOP;

    switch( mode )
    {
    case JobIOMgr::Work		: resp = mRSP_WORK; break;
    case JobIOMgr::Pause	: resp = mRSP_PAUSE; break;
    case JobIOMgr::Stop		: resp = mRSP_STOP; break;
    }

    int descnr = ji.descnr_;
    BufferString hostnm;
    if ( ji.hostdata_ ) hostnm = ji.hostdata_->name();

    jhrespmutex_.lock();

    JobHostRespInfo* jhri = getJHRFor( descnr, hostnm );
    if ( jhri ) jhri->response_ = resp;

    jhrespmutex_.unlock();
}


void JobIOHandler::doDispatch( CallBacker* )
{
    while( 1 ) 
    {
	Socket* sock_ = sockprov_.makeConnection(1); // 1 sec timeout

	if ( exitreq_ ) stopDispatching();

	char tag=mCTRL_STATUS;
	int jobid=-1;
	int status=mSTAT_UNDEF;
	BufferString hostnm;
	BufferString errmsg;

	if ( sock_ && sock_->ok() )
	{
	    SeparString statstr;
	    bool ok = sock_->readtag( tag, statstr );

	    if ( !ok )
	    {
		sock_->fetchMsg( errmsg );
		ok = sock_->readtag( tag, statstr );
	    } 

	    if ( ok )
	    {
		getFromString( jobid, statstr[0] );
		getFromString(status, statstr[1] );
		hostnm = statstr[2];
		errmsg = statstr[3];

		char response = getRespFor( jobid, hostnm );

		statusqueue_.add( new StatusInfo( tag, jobid, status, errmsg,
					    hostnm, Time_getMilliSeconds()) );

		sock_->writetag( response );
	    }
	    else
	    {
		pErrMsg( "Error in socket communication." );
	    }

	    delete sock_; sock_ =0;
	}
    }
}




JobIOMgr::JobIOMgr( int firstport, int niceval )
    : iohdlr_( *new JobIOHandler(firstport) )
    , niceval_( niceval )
{}

JobIOMgr::~JobIOMgr()
{ delete &iohdlr_; }


void JobIOMgr::reqModeForJob( const JobInfo& ji, Mode m )
    { iohdlr_.reqModeForJob(ji,m); } 


void JobIOMgr::removeJob( const char* hostnm, int descnr )
    { iohdlr_.removeJobDesc(hostnm,descnr); } 


ObjQueue<StatusInfo>& JobIOMgr::statusQueue()
    { return iohdlr_.statusQueue(); }


bool JobIOMgr::startProg( const char* progname, const HostData& machine,
	IOPar& iop, const FilePath& basefp, const JobInfo& ji,
	const char* rshcomm )
{
    DBG::message(DBG_MM,"JobIOMgr::startProg");

    FilePath ioparfp;
    if ( !mkIOParFile( ioparfp, basefp, machine, iop ) )
	return false;
    
    CommandString cmd;
    mkCommand( cmd, machine, progname, basefp, ioparfp, ji, rshcomm );

    iohdlr_.addJobDesc( machine, ji.descnr_ );

    if ( mDebugOn )
    {
	BufferString msg("Executing: ");
	msg += cmd.string();
	DBG::message(msg);
    }

    StreamProvider strmprov( cmd.string() );
    if ( !strmprov.executeCommand(true) ) // true: in background
    {
	BufferString s( "Failed to submit command '" );
	s += strmprov.command(); s += "'";

	iohdlr_.removeJobDesc( machine.name(), ji.descnr_ );
	mErrRet(s);
    }
    
    return true;
}


bool JobIOMgr::mkIOParFile( FilePath& iopfp, const FilePath& basefp,
			    const HostData& machine, const IOPar& iop )
{
    IOParList iopl;
    iopl.deepErase(); // Protect against a file '.Parameters' in survdir
    IOPar* newiop = new IOPar( iop ); iopl += newiop;

    iopfp = basefp; iopfp.setExtension( ".par", false );
    FilePath logfnm(basefp); logfnm.setExtension( ".log", false );

    FilePath remotelogfnm( machine.convPath( HostData::Data, logfnm ));

    newiop->set( sLogFil, remotelogfnm.fullPathStyled(machine.pathStyle()) );

    if ( newiop->find( sTmpStor ) )
    {
	FilePath remotetmpdir = machine.convPath( HostData::Data,
						  newiop->find( sTmpStor ) );

	newiop->set( sTmpStor,
		remotetmpdir.fullPathStyled( machine.pathStyle() ) );
    }

    newiop->set( sSurvey, IOM().surveyName() );


    if ( File_exists(iopfp.fullPath()) )
	File_remove( iopfp.fullPath(), NO );
    if ( File_exists(logfnm.fullPath()) )
	File_remove( logfnm.fullPath(), NO );

    StreamData ioplsd = StreamProvider(iopfp.fullPath()).makeOStream();
    if ( !ioplsd.usable() )
    {
	BufferString s( "Cannot open '" );
	s += iopfp.fullPath(); s += "' for write ...";
	mErrRet(s);
    }
    bool res = iopl.write( *ioplsd.ostrm );
    ioplsd.close();
    if ( !res )
    {
	BufferString s( "Cannot write parameters into '" );
	s += iopfp.fullPath(); s += "'";
	mErrRet(s);
    }

    return true;
}


void JobIOMgr::mkCommand( CommandString& cmd, const HostData& machine,
			  const char* progname, const FilePath& basefp,
			  const FilePath& iopfp, const JobInfo& ji,
			  const char* rshcomm )
{
    const bool remote = machine.isKnownAs( HostData::localHostName() );

    cmd = "@";
    cmd.addWoSpc( GetExecScript(remote) );

    if ( remote )
    {
	cmd.add( machine.name() );

	cmd.addFlag( "--rexec", rshcomm ); // rsh/ssh/rcmd

	if ( machine.isWin()  ) cmd.add( "--iswin" );

	cmd.addFilePathFlag( "--with-dtect-appl", 
			     machine.convPath(HostData::Appl, GetSoftwareDir()),
			     FilePath::Unix );

	cmd.addFilePathFlag( "--with-dtect-data", 
			     machine.convPath(HostData::Appl, GetBaseDataDir()),
			     FilePath::Unix );

	cmd.addFilePathFlag( "--with-local-file-base", basefp, FilePath::Unix);
	cmd.addFilePathFlag( "--with-remote-file-base",
	    machine.convPath(HostData::Data, basefp), FilePath::Unix );

	if ( machine.isWin() && machine.shareData() )
	{
	    const ShareData& sd = *machine.shareData();
	    cmd.addFlag( "--data-host", sd.hostName() );
	    cmd.addFlag( "--data-drive", sd.drive() );
	    cmd.addFlag( "--data-share", sd.share() );
	    cmd.addFlag( "--remotepass", sd.pass() );
	}
    }

    cmd.addFlag( "--nice", niceval_ );
    cmd.addFlag( "--inbg", progname );

    cmd.addFlag( "-masterhost", HostData::localHostName() ); 
    cmd.addFlag( "-masterport", iohdlr_.port() );
    cmd.addFlag( "-jobid", ji.descnr_ );

   
    FilePath riopfp( remote ? machine.convPath( HostData::Data, iopfp )
			     : iopfp );
    cmd.addFilePath( riopfp, machine.pathStyle() );
}
