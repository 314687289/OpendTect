#ifndef file_h
#define file_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 Contents:	File utitlities
 RCS:		$Id: file.h,v 1.12 2012-03-09 12:45:26 cvsbert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
#include "timefun.h"
class BufferString;


/*!\brief Interface for several file and directory related services */

namespace File
{
    mGlobal bool	exists(const char*);
    mGlobal bool	isEmpty(const char*);
    mGlobal bool	isFile(const char*);
    mGlobal bool	isDirectory(const char*);

    mGlobal const char*	getCanonicalPath(const char*);
    mGlobal const char*	getRelativePath(const char* reltodir,const char* fnm);

    mGlobal bool	createLink(const char* from,const char* to);
    mGlobal bool	isLink(const char*);
    mGlobal const char*	linkTarget(const char* linkname);

    mGlobal bool	isWritable(const char*);
    mGlobal bool	makeWritable(const char*,bool yesno,bool recursive);
    mGlobal bool	setPermissions(const char*,const char* perms,
	    			       bool recursive);

    mGlobal bool	createDir(const char*); 
    mGlobal bool	rename(const char* oldname,const char* newname);
    mGlobal bool	copy(const char* from,const char* to);
    mGlobal bool	remove(const char*);
    mGlobal bool	saveCopy(const char* from,const char* to);
    mGlobal bool	copyDir(const char* from,const char* to);
    mGlobal bool	removeDir(const char*);

    mGlobal bool	getContent(const char*,BufferString&);
    mGlobal od_int64	getFileSize(const char*); //!<returns size in bytes
    			//!<Returns 0 on error
    mGlobal int		getKbSize(const char*);
    			//!<Returns 0 on error

    mGlobal const char* timeCreated(const char* filenm,
	    			    const char* fmt=Time::defDateTimeFmt());
    mGlobal const char*	timeLastModified(const char* filenm,
	    			    const char* fmt=Time::defDateTimeFmt());
    mGlobal od_int64	getTimeInSeconds(const char*); //! Last modified time

    mGlobal const char*	getCurrentPath();
    mGlobal const char*	getHomePath();
    mGlobal const char*	getTempPath();
    mGlobal const char* getRootPath(const char* path);

} // namespace File


#endif
