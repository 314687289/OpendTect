/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 3-8-1994
-*/

static const char* rcsID = "$Id: dirlist.cc,v 1.9 2004-04-01 13:39:50 bert Exp $";

#include "dirlist.h"
#include "globexpr.h"
#include "filepath.h"
#include "filegen.h"

#ifdef __win__
#include <windows.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif



DirList::DirList( const char* dirname, DirList::Type t, const char* msk )
	: BufferStringSet(true)
    	, dir_(dirname?dirname:".")
	, type_(t)
    	, mask_(msk)
{
    update();
}


void DirList::update()
{
    deepErase();
    const bool havemask = mask_ != "";
    GlobExpr ge( mask_.buf() );
    FilePath fp( dir_ ); fp.add( "X" );

#ifdef __win__
    WIN32_FIND_DATA	dat;
    HANDLE		mhndl;

    BufferString dirnm = dir_;
    dirnm += "\\*";

    mhndl = FindFirstFile( (const char*)dirnm, &dat );

    do
    {
        if ( (dat.cFileName)[0] == '.' && (dat.cFileName)[1] == '\0' ) continue;
        if ( (dat.cFileName)[0] == '.' && (dat.cFileName)[1] == '.'
	  && (dat.cFileName)[2] == '\0' ) continue;

	if ( type_ != AllEntries )
	{
	    fp.setFileName( dat.cFileName );
	    if ( (type_ == FilesOnly) == (bool)File_isDirectory(fp.fullPath()) )
		continue;
	}

	if ( havemask && !ge.matches(dat.cFileName) )
	    continue;

	add( dat.cFileName );

    } while ( FindNextFile(mhndl,&dat) );

#else
    DIR* dirp = opendir( dir_.buf() );
    if ( !dirp ) return;

    struct dirent* dp;
    for ( dp = readdir(dirp); dp; dp = readdir(dirp) )
    {
        if ( (dp->d_name)[0] == '.' && (dp->d_name)[1] == '\0' ) continue;
        if ( (dp->d_name)[0] == '.' && (dp->d_name)[1] == '.'
	  && (dp->d_name)[2] == '\0' ) continue;

	if ( type_ != AllEntries )
	{
	    fp.setFileName( dp->d_name );
	    if ( (type_ == FilesOnly) == (bool)File_isDirectory(fp.fullPath()) )
		continue;
	}

	if ( havemask && !ge.matches(dp->d_name) )
	    continue;

	add( dp->d_name );
    }
    closedir(dirp);
#endif
}
