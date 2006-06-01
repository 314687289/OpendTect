/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Lammertink
 * DATE     : November 2004
 * FUNCTION : Utilities for win32, amongst others path conversion
-*/

static const char* rcsID = "$Id: winutils.cc,v 1.12 2006-06-01 10:07:00 cvskris Exp $";


#include "winutils.h"
#include "bufstring.h"
#include "filegen.h"
#include "envvars.h"
#include "debugmasks.h"
#include "string2.h"

#ifdef __win__
# include <windows.h>
# include <shlobj.h>
# include <process.h>
//# include <float.h>

// registry stuff
# include <regstr.h>
# include <winreg.h>

#include <iostream>

#endif

# include <ctype.h>


#ifdef __cpp__
extern "C" {
#endif

static const char* drvstr="/cygdrive/";
static bool __do_debug_cleanpath = GetEnvVarYN("DTECT_DEBUG_WINPATH");


const char* getCleanUnxPath( const char* path )
{
    if ( !path || !*path ) return 0;

    static BufferString res;

    BufferString buf; buf = path;
    char* ptr = buf.buf();
    skipLeadingBlanks( ptr ); removeTrailingBlanks( ptr );
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

    static BufferString ret;
    ret = path;
    replaceCharacter( ret.buf(), ';' , ':' );

    BufferString buf( ret );
    char* ptr = buf.buf();

    skipLeadingBlanks( ptr ); removeTrailingBlanks( ptr );

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
    static BufferString answer;
    if ( answer != "" )  return answer;

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
    static BufferString Result;

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

#endif // __win__

#ifdef __cpp__
}
#endif
