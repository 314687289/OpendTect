#ifndef jobinfo_h
#define jobinfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Oct 2004
 RCS:		$Id: jobinfo.h,v 1.7 2005-04-04 15:25:10 cvsarend Exp $
________________________________________________________________________

-*/

#include "iopar.h"

class HostData;


/*!\brief All info on a job. */

class JobInfo
{
    friend class	JobRunner;
public:

    enum State		{ ToDo, Scheduled, Preparing, Working, Paused,
			  Completed, JobFailed, HostFailed };
    			//!< This is always the 'real' status
			//!< This as opposed to 'requested' or whatever

    			JobInfo( int dnr )
			    : descnr_(dnr)
			    , state_(ToDo)
			    , statusmsg_("Scheduled")
			    , infomsg_("")
			    , jobfailures_(0)
			    , hstfailures_(0)
			    , hostdata_(0)
			    , osprocid_(-1)
			    , starttime_(0)
			    , recvtime_(0) {}

    int			descnr_;	//!< JobdescProv's job number
    State		state_;
    int			jobfailures_;	//!< Failures probably caused by job
    int			hstfailures_;	//!< Failures probably caused by host
    int			nrdone_;
    int			osprocid_;	//!< OS process ID
    BufferString	statusmsg_;	
    BufferString	infomsg_;	//!< Error msg if failure

    const HostData*	hostdata_;	//!< Host currently working on job

protected:

    int			starttime_;
    int			recvtime_;
};


#endif
