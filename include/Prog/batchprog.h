#ifndef batchprog_h
#define batchprog_h
 
/*
________________________________________________________________________
 
 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		14-9-1998
 RCS:		$Id: batchprog.h,v 1.36 2008-07-16 15:03:27 cvsbert Exp $
________________________________________________________________________

 Batch programs should include this header, and define a BatchProgram::go().
 If program args are needed outside this method, BP() can be accessed.
 
*/

#include "prog.h"
#include "namedobj.h"
#include "bufstringset.h"
#include "genc.h"
#include <iosfwd>
class IOPar;
class IOObj;
class StreamData;
class IOObjContext;
class MMSockCommunic;

/*!\brief Main object for 'standard' batch programs.

  Most 'interesting' batch programs need a lot of parameters to do the work.
  Therefore, in OpendTect, BatchPrograms need a 'parameter file', with all
  the info needed in IOPar format, i.e. keyword/value pairs.

  This object takes over the details of reading that file, extracting
  'standard' components from the parameters, opening sockets, etc. etc.

  To use the object, instead of defining a function 'main', you should define
  the function 'BatchProgram::go'.

  If you need argc and/or argv outside go(), the BP() singleton instance can
  be accessed.

*/

class BatchProgram : public NamedObject
{
public:

    const IOPar&	pars() const		{ return *iopar; }
    IOPar&		pars()			{ return *iopar; }

    int			nrArgs() const		{ return *pargc - argshift; }
    const char*		arg( int idx ) const	{ return argv_[idx+argshift]; }
    const char*		fullPath() const	{ return fullpath; }
    const char*		progName() const;

			//! This method must be defined by user
    bool		go(std::ostream& log_stream);

			// For situations where you need the old-style stuff
    char**		argv()			{ return argv_; }
    int&		argc()			{ return *pargc; }
    int			argc() const		{ return *pargc; }
    int			realArgsStartAt() const	{ return argshift; }
    BufferStringSet&	cmdLineOpts()		{ return opts; }

    IOObj*		getIOObjFromPars(const char* keybase,bool mknew,
					 const IOObjContext& ctxt,
					 bool msgiffail=true) const;

			//! pause requested (via socket) by master?
    bool		pauseRequested() const;

    bool		errorMsg( const char* msg, bool cc_stderr=false);
    bool		infoMsg( const char* msg, bool cc_stdout=false);

protected:

    friend int		Execute_batch(int*,char**);
    friend const BatchProgram& BP();
    friend class	MMSockCommunic;

			BatchProgram(int*,char**);
			~BatchProgram();

    static BatchProgram* inst;

    int*		pargc;
    char**		argv_;
    int			argshift;
    FileNameString	fullpath;
    bool		stillok;
    bool		inbg;
    StreamData&		sdout;
    IOPar*		iopar;
    BufferStringSet	opts;
    BufferString	parversion_;
    BufferStringSet	requests_;

    bool		initOutput();
    void		progKilled(CallBacker*);
    void		killNotify( bool yn );

    MMSockCommunic*	mmComm()		{ return comm; }

    int 		jobId()			{ return jobid; }

private:

    MMSockCommunic*	comm;
    int			jobid;
};


int Execute_batch(int*,char**);
inline const BatchProgram& BP() { return *BatchProgram::inst; }

#ifdef __prog__
# ifdef __win__
#  include "_execbatch.h"
# endif

    int main( int argc, char** argv )
    {
	int ret = Execute_batch(&argc,argv);
	return ExitProgram( ret );
    }

#endif // __prog__

#endif
