#ifndef oddlsite_h
#define oddlsite_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
 RCS:           $Id: oddlsite.h,v 1.2 2011-11-22 12:57:56 cvsbert Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "bufstring.h"
class BufferStringSet;
class TaskRunner;
class DataBuffer;
class ODHttp;


/*!\brief Access to an http site to get the contents of files.
 
  The URL defaults to opendtect.org. The subdir is going to be the current
  work directory on the site. You can change it at any time (unlike the host).

  There are two main services: get one file, or get a bunch of files. To get
  one file, you request it. Set some kind of status bar, because it won't
  return until either an error occurs (e.g. a timeout is exceeded),
  or the file is there.  This is meant for small files and minimum hassle.

  If you need bigger files and/or you want users to be able to interrupt, then
  you need the version with the TaskRunner.

 */

class ODDLSite : public CallBacker
{
public:

			ODDLSite(const char* the_host,float timeout_sec=0);
			~ODDLSite();
    bool		isOK() const			{ return !isfailed_; }
    bool		reConnect();
    			//!< usable for 'Re-try'. Done automatically if needed.

    const char*		host() const			{ return host_; }
    const char*		subDir() const			{ return subdir_; }
    void		setSubDir( const char* s )	{ subdir_ = s; }
    float		timeout() const			{ return timeout_; }
    void		setTimeOut(float,bool storeinsettings);

    const char*		errMsg() const			{ return errmsg_; }

    bool		getFile(const char* fnm,const char* outfnm=0);
    			//!< Without a file name, get the DataBuffer
    DataBuffer*		obtainResultBuf();
    			//!< the returned databuf becomes yours

    bool		getFiles(const BufferStringSet& fnms,
				  const char* outputdir,TaskRunner&);

    BufferString	fullURL(const char*) const;

protected:

    BufferString	host_;
    BufferString	subdir_;
    float		timeout_;

    ODHttp&		odhttp_;
    mutable BufferString errmsg_;
    bool		isfailed_;
    DataBuffer*		databuf_;

    void		reqFinish(CallBacker*);
    BufferString	getFileName(const char*) const;

};



#endif
