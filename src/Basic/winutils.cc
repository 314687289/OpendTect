/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Lammertink
 * DATE     : November 2004
 * FUNCTION : Utilities for win32, amongst others path conversion
-*/

static const char* rcsID mUnusedVar = "$Id: winutils.cc,v 1.27 2012-05-02 15:11:28 cvskris Exp $";


#include "winutils.h"
#include "bufstring.h"
#include "envvars.h"
#include "file.h"
#include "debugmasks.h"
#include "string2.h"
#include "staticstring.h"
#include "strmprov.h"
#ifdef __win__
# include <windows.h>
# include <shlobj.h>
# include <process.h>

// registry stuff
# include <regstr.h>
# include <winreg.h>

#include <iostream>

#endif

#include <ctype.h>
#include <string.h>

static const char* drvstr="/cygdrive/";

const char* getCleanUnxPath( const char* path )
{
    if ( !path || !*path ) return 0;

    static StaticStringManager stm;
    BufferString& res = stm.getString();

    BufferString buf; buf = path;
    char* ptr = buf.buf();
    mTrimBlanks( ptr );
    replaceCharacter( ptr, '\\' , '/' );
    replaceCharacter( ptr, ';' , ':' );

    char* drivesep = strstr( ptr, ":" );
    if ( !drivesep ) { res = ptr; return res; }
    *drivesep = '\0';

    res = drvstr;
    *ptr = tolower(*ptr);
    res += ptr;
    res += ++drivesep;

    return res;
}

#define mRet(ret) \
    replaceCharacter( ret.buf(), '/', '\\' ); \
    if ( __do_debug_cleanpath ) \
    { \
        BufferString msg("getCleanWinPath for:"); \
	msg += path; \
	msg += " : "; \
	msg += ret; \
        od_debug_message( msg ); \
    } \
    return ret;  

const char* getCleanWinPath( const char* path )
{
    if ( !path || !*path ) return 0;

    static StaticStringManager stm;
    BufferString& ret = stm.getString();
    ret = path;
    replaceCharacter( ret.buf(), ';' , ':' );

    BufferString buf( ret );
    char* ptr = buf.buf();

    mTrimBlanks( ptr );

    static bool __do_debug_cleanpath = GetEnvVarYN("DTECT_DEBUG_WINPATH");

    if ( *(ptr+1) == ':' ) // already in windows style.
	{ ret = ptr;  mRet( ret ); }

    bool isabs = *ptr == '/';
                
    char* cygdrv = strstr( ptr, drvstr );
    if ( cygdrv )
    {
	char* drv = cygdrv + strlen( drvstr );
	char* buffer = ret.buf();

	*buffer = *drv; *(buffer+1) = ':'; *(buffer+2) = '\0';
	ret += ++drv;
    }

    char* drivesep = strstr( ret.buf(), ":" );
    if ( isabs && !drivesep ) 
    {
	const char* cygdir =
#ifdef __win__
				getCygDir();
#else 
				0;
#endif
	if ( !cygdir || !*cygdir )
	{
	    if ( GetEnvVar("CYGWIN_DIR") ) // anyone got a better idea?
		cygdir = GetEnvVar("CYGWIN_DIR");
	    else
		cygdir = "c:\\cygwin";
	}

	ret = cygdir;

	ret += ptr;
    }

    mRet( ret );
}


#ifdef __win__

const char* getCygDir()
{
    static StaticStringManager stm;
    BufferString& answer = stm.getString();
    if ( !answer.isEmpty() )  return answer;

    HKEY hKeyRoot = HKEY_CURRENT_USER;
    LPTSTR subkey="Software\\Cygnus Solutions\\Cygwin\\mounts v2\\/";
    LPTSTR Value="native"; 

    BYTE Value_data[256];
    DWORD Value_size = sizeof(Value_data);

    HKEY hKeyNew = 0;
    DWORD retcode = 0;
    DWORD Value_type = 0;

    retcode = RegOpenKeyEx ( hKeyRoot, subkey, 0, KEY_QUERY_VALUE, &hKeyNew );

    if (retcode != ERROR_SUCCESS)
    {
	hKeyRoot = HKEY_LOCAL_MACHINE;
	subkey="Software\\Cygnus Solutions\\Cygwin\\mounts v2\\/";

	retcode = RegOpenKeyEx( hKeyRoot, subkey, 0, KEY_QUERY_VALUE, &hKeyNew);
	if (retcode != ERROR_SUCCESS) return 0;
    }

    retcode = RegQueryValueEx( hKeyNew, Value, NULL, &Value_type, Value_data,
                               &Value_size);

    if (retcode != ERROR_SUCCESS) return 0;

    answer = (const char*) Value_data;
    return answer;
}


const char* GetSpecialFolderLocation(int nFolder)
{
    static StaticStringManager stm;
    BufferString& Result = stm.getString();

    LPITEMIDLIST pidl;
    HRESULT hr = SHGetSpecialFolderLocation(NULL, nFolder, &pidl);
    
    if ( !SUCCEEDED(hr) ) return 0;

    char szPath[_MAX_PATH];

    if ( !SHGetPathFromIDList(pidl, szPath)) return 0;

    Result = szPath;

    return Result;
}



static int initialise_Co( void )
{
    static int initialised = 0;
    if ( !initialised )
    {
	if ( !SUCCEEDED( CoInitialize(NULL) ) )
	    return 0;

	initialised = 1;
    }
    return initialised;
}


bool winCopy( const char* from, const char* to, bool isfile )
{
    if ( isfile && File::getKbSize(from) < 1024 )
    {
	BufferString cmd;
	cmd = "copy /Y";
	cmd.add(" \"").add(from).add("\" \"").add(to).add("\"");
	const bool ret = system( cmd ) != -1;
	return ret;
    }

    SHFILEOPSTRUCT fileop;
    BufferString frm( from );
    if ( !isfile )
	frm += "\\*";

    const int sz = frm.size();
    frm[sz+1] = '\0';
     
    ZeroMemory( &fileop, sizeof(fileop) );
    fileop.hwnd = NULL; fileop.wFunc = FO_COPY;
    fileop.pFrom = frm; fileop.pTo = to; 
    fileop.fFlags = ( isfile ? FOF_FILESONLY : FOF_MULTIDESTFILES )
			       | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION;

    int res = SHFileOperation( &fileop );
    return !res;
}


bool winRemoveDir( const char* dirnm )
{
    SHFILEOPSTRUCT fileop;
    BufferString frm( dirnm );
    const int sz = frm.size();
    frm[sz+1] = '\0';
    ZeroMemory( &fileop, sizeof(fileop) );
    fileop.hwnd = NULL;
    fileop.wFunc = FO_DELETE;
    fileop.pFrom = frm;
    fileop.fFlags = FOF_MULTIDESTFILES | FOF_NOCONFIRMATION;
    int res = SHFileOperation( &fileop );
    return !res;
}

#endif // __win__

