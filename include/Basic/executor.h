#ifndef executor_H
#define executor_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		11-7-1996
 RCS:		$Id: executor.h,v 1.15 2005-09-23 12:36:33 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidobj.h"
#include "basictask.h"
#include <iosfwd>

template <class T> class ObjectSet;

/*!\brief specification to enable chunkwise execution a process.

Interface enabling separation of the control of execution of any process from
what actually is going on. The work is done by calling the doStep() method
until either ErrorOccurred or Finished is returned. To enable logging and/or
communication with the user, two types of info can be made available (the
methods will be called before the step is executed). Firstly, a message.
Secondly, info on the progress.
It is common that Executors are combined to a new Executor object. This is
the most common reason why totalNr() can change.

If doStep returns -1 (Failure) the error message should be in message().

The execute() utility executes the process while logging message() etc. to
a stream. Useful in batch situations.

*/

class Executor : public UserIDObject
	       , public BasicTask
{
public:
			Executor( const char* nm )
			: UserIDObject(nm)
			, prestep(this), poststep(this)	{}
    virtual		~Executor()			{}

    virtual int		doStep();

    static const int	ErrorOccurred;		//!< -1
    static const int	Finished;		//!< 0
    static const int	MoreToDo;		//!< 1
    static const int	WarningAvailable;	//!< 2

    virtual const char*	message() const			{ return "Working"; }
    virtual int		totalNr() const			{ return -1; }
    			//!< If unknown, return -1
    virtual int		nrDone() const			{ return 0; }
    virtual const char*	nrDoneText() const		{ return "Nr done"; }

    virtual bool	execute(std::ostream* log=0,bool isfirst=true,
	    			bool islast=true,int delaybetwnstepsinms=0);

    Notifier<Executor>	prestep;
    Notifier<Executor>	poststep; //!< Only when MoreToDo will be returned.

};


class ExecutorRunner : public BasicTaskRunner
{
public:

    			ExecutorRunner( Executor* ex )
			: BasicTaskRunner(ex)	{}

    virtual const char* lastMsg() const		= 0;

};


class ExecutorBatchTaskRunner : public ExecutorRunner
{
public:
			ExecutorBatchTaskRunner( Executor* ex,
					std::ostream* strm=0,
					bool isfirst=true, bool islast=true )
			: ExecutorRunner(ex)
			, logstrm_(strm)
			, isfirst_(isfirst)
			, islast_(islast)		{}

    virtual bool	execute()
    			{
			    mDynamicCastGet(Executor*,ex,task_);
			    if ( !ex )
			    	{ lastmsg_ = "No Executor!"; return false; }
			    bool res = ex->execute(logstrm_,isfirst_,islast_);
			    lastmsg_ = ex->message();
			    return res;
			}
    virtual const char*	lastMsg() const		{ return lastmsg_; }

protected:

    std::ostream*	logstrm_;
    bool		isfirst_;
    bool		islast_;
    BufferString	lastmsg_;
};


/*!\brief Executor consisting of other executors.

Executors may be added on the fly while processing. Depending on the
paralell flag, the executors are executed in the order in which they were added
or in paralell (but still single-threaded).

*/


class ExecutorGroup : public Executor
{
public:
    			ExecutorGroup( const char* nm, bool paralell=false );
    virtual		~ExecutorGroup();
    virtual void	add( Executor* );
    			/*!< You will become mine!! */

    virtual const char*	message() const;
    virtual int		totalNr() const;
    virtual int		nrDone() const;
    virtual const char*	nrDoneText() const;
    
    int			nrExecutors()
    			    { return executors.size(); }
    Executor*		getExecutor(int idx)
    			    { return executors[idx]; }

    void		setNrDoneText( const char* txt )
			    { nrdonetext = txt; }
    			//!< If set, will use this and the counted nrdone

protected:

    virtual int		nextStep();
    bool		goToNextExecutor();
    bool		similarLeft() const;

    const bool		paralell;
    int			currentexec;
    int			nrdone;
    BufferString	nrdonetext;
    ObjectSet<Executor>& executors;
    TypeSet<int>	executorres;

};

#endif
