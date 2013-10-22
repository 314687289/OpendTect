/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : March 1994
 * FUNCTION : general utilities
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "genc.h"
#include "envvars.h"
#include "debug.h"
#include "oddirs.h"
#include "svnversion.h"
#include "bufstring.h"
#include "ptrman.h"
#include "filepath.h"
#include "staticstring.h"
#include "threadlock.h"
#include "od_iostream.h"
#include "iopar.h"
#include <iostream>

#include <math.h>
#include <stdlib.h>
#include <string.h>
#ifndef __win__
# include <unistd.h>
# include <errno.h>
# include <sys/types.h>
# include <signal.h>
#else
# include <float.h>
# include <time.h>
# include <sys/timeb.h>
# include <shlobj.h>
#endif

static IOPar envvar_entries;
static int insysadmmode_ = 0;
mExternC( Basic ) int InSysAdmMode(void) { return insysadmmode_; }
mExternC( Basic ) void SetInSysAdmMode(void) { insysadmmode_ = 1; }

#ifdef __win__
const char* GetLocalIP(void)
{
    static char ret[16];
    struct in_addr addr;
    struct hostent* remotehost = gethostbyname( GetLocalHostName() );
    addr.s_addr = *(u_long *)remotehost->h_addr_list[0];
    strcpy( ret, inet_ntoa(addr) );
    return ret;
}


int initWinSock()
{
    WSADATA wsaData;
    WORD wVersion = MAKEWORD( 2, 0 ) ;
    return !WSAStartup( wVersion, &wsaData );
}
#endif


const char* GetLocalHostName()
{ 
    static char ret[256];
#ifdef __win__
    initWinSock();
#endif
    gethostname( ret, 256 );
    return ret;
}


void SwapBytes( void* p, int n )
{
    int nl = 0;
    unsigned char* ptr = (unsigned char*)p;
    unsigned char c;

    if ( n < 2 ) return;
    n--;
    while ( nl < n )
    { 
	c = ptr[nl]; ptr[nl] = ptr[n]; ptr[n] = c;
	nl++; n--;
    }
}


void PutIsLittleEndian( unsigned char* ptr )
{
#ifdef __little__
    *ptr = 1;
#else
    *ptr = 0;
#endif
}

#ifdef __msvc__
#include <process.h>
#include <direct.h>
#define getpid	_getpid
#define getcwd  _getcwd
#endif

int GetPID()
{
    return getpid();
}


void NotifyExitProgram( PtrAllVoidFn fn )
{
    static int nrfns = 0;
    static PtrAllVoidFn fns[100];
    int idx;


    if ( fn == ((PtrAllVoidFn)(-1)) )
    {
	for ( idx=0; idx<nrfns; idx++ )
	    (*(fns[idx]))();
    }
    else
    {
	fns[nrfns] = fn;
	nrfns++;
    }
}


mExternC(Basic) const char* GetLastSystemErrorMessage()
{
    mDeclStaticString( ret );
#ifndef __win__
    char buf[1024];
    ret = strerror_r( errno, buf, 1024 );
#endif
    return ret.buf();
}


mExternC(Basic) void ForkProcess(void)
{
#ifndef __win__
    switch ( fork() )
    {
    case 0:
	break;
    case -1:
	std::cerr << "Cannot fork: " << GetLastSystemErrorMessage() <<std::endl;
    default:
	ExitProgram( 0 );
    }
#endif
}


#define isBadHandle(h) ( (h) == NULL || (h) == INVALID_HANDLE_VALUE )

int isProcessAlive( int pid )
{
#ifdef __win__
    HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, 
				   GetPID() );
    return isBadHandle(hProcess) ? 0 : 1;
#else
    const int res = kill( pid, 0 );
    return res == 0 ? 1 : 0;
#endif
}


int ExitProgram( int ret )
{
    if ( AreProgramArgsSet() && od_debug_isOn(DBG_PROGSTART) )
    {
	std::cerr << "\nExitProgram (PID: " << GetPID() << std::endl;
#ifndef __win__
	system( "date" );
#endif
    }

    NotifyExitProgram( (PtrAllVoidFn)(-1) );

// On Mac OpendTect crashes when calling the usual exit and shows error message
// dyld: odmain bad address of lazy symbol pointer passed to 
// stub_binding_helper
// _Exit does not call registered exit functions and prevents crash
#ifdef __mac__
    _Exit( ret );
#endif

#ifdef __msvc__
    exit( ret ? EXIT_FAILURE : EXIT_SUCCESS );
#else
    exit(ret);
    return ret; // to satisfy (some) compilers
#endif
}


/*-> envvar.h */

mExternC(Basic) char* GetOSEnvVar( const char* env )
{
    return getenv( env );
}


static void loadEntries( const char* fnm, IOPar* iop=0 )
{
    if ( !fnm || !*fnm )
	return;

    od_istream strm( fnm );
    if ( !strm.isOK() )
	return;

    if ( !iop ) iop = &envvar_entries;
    BufferString line;
    while ( strm.getLine(line) )
    {
	char* nmptr = line.buf();
	mSkipBlanks(nmptr);
	if ( !*nmptr || *nmptr == '#' )
	    continue;

	char* valptr = nmptr;
	mSkipNonBlanks( valptr );
	if ( !*valptr ) continue;
	*valptr++ = '\0';
	mTrimBlanks(valptr);
	if ( !*valptr ) continue;

	iop->set( nmptr, valptr );
    }
}


mExternC(Basic) const char* GetEnvVar( const char* env )
{
    if ( !env || !*env )
	{ pFreeFnErrMsg( "Asked for empty env var", "GetEnvVar" ); return 0; }
    if ( insysadmmode_ )
	return GetOSEnvVar( env );

    mDefineStaticLocalObject( bool, filesread, = false );
    if ( !filesread )
    {
	if ( !AreProgramArgsSet() )
	{
	    //We should not be here before SetProgramInfo() is called.
	    pFreeFnErrMsg( "Use SetProgramArgs()", "GetEnvVar" );
	    return GetOSEnvVar( env );
	}

	filesread = true;
	loadEntries(GetSettingsFileName("envvars") );
	loadEntries(GetSetupDataFileName(ODSetupLoc_ApplSetupOnly,"EnvVars",1));
	loadEntries(GetSetupDataFileName(ODSetupLoc_SWDirOnly,"EnvVars",1) );
    }

    const char* res = envvar_entries.find( env );
    return res ? res : GetOSEnvVar( env );
}


mExternC(Basic) int GetEnvVarYN( const char* env, int defaultval )
{
    const char* s = GetEnvVar( env );
    if ( !s )
	return defaultval;

    return *s == '0' || *s == 'n' || *s == 'N' ? 0 : 1;
}


mExternC(Basic) int GetEnvVarIVal( const char* env, int defltval )
{
    const char* s = GetEnvVar( env );
    return s ? atoi(s) : defltval;
}


mExternC(Basic) double GetEnvVarDVal( const char* env, double defltval )
{
    const char* s = GetEnvVar( env );
    return s ? atof(s) : defltval;
}


mExternC(Basic) int SetEnvVar( const char* env, const char* val )
{
    if ( !env || !*env )
	return mC_False;

    BufferString topass( env, "=", val );
#ifdef __msvc__
    _putenv( topass.buf() );
#else
    putenv( topass.buf() );
#endif

    return mC_True;
}


static bool writeEntries( const char* fnm, const IOPar& iop )
{
    od_ostream strm( fnm );
    if ( !strm.isOK() )
	return false;

    for ( int idx=0; idx<iop.size(); idx++ )
	strm << iop.getKey(idx) << od_tab << iop.getValue(idx) << od_endl;

    return strm.isOK();
}


mExternC(Basic) int WriteEnvVar( const char* env, const char* val )
{
    if ( !env || !*env )
	return 0;

    BufferString fnm( insysadmmode_
	    ? GetSetupDataFileName(ODSetupLoc_SWDirOnly,"EnvVars",1)
	    : GetSettingsFileName("envvars") );
    IOPar iop;
    loadEntries( fnm, &iop );
    iop.set( env, val );
    return writeEntries( fnm, iop ) ? 1 : 0;
}


mExternC(Basic) int GetSubversionRevision(void)
{ return mSVN_VERSION; }


mExternC(Basic) const char* GetSubversionUrl(void)
{ return mSVN_URL; }



int argc = -1;
BufferString initialdir;
char** argv = 0;


mExternC(Basic) char** GetArgV(void)
{ return argv; }


mExternC(Basic) int GetArgC(void)
{ return argc; }


mExternC(Basic) int AreProgramArgsSet(void)
{ return GetArgC()!=-1; }


mExternC(Basic) void SetProgramArgs( int newargc, char** newargv )
{
    getcwd( initialdir.buf(), initialdir.minBufSize() );

    argc = newargc;
    argv = newargv;
    
    od_putProgInfo( argc, argv );
}


static const char* getShortPathName( const char* path )
{
#ifndef __win__
    return path;
#else
    char fullpath[1024];
    GetModuleFileName( NULL, fullpath, (sizeof(fullpath)/sizeof(char)) ); 
    // get the fullpath to the exectuabe including the extension. 
    // Do not use argv[0] on Windows
    static char shortpath[1024];
    GetShortPathName( fullpath, shortpath, sizeof(shortpath) ); 
    //Extract the shortname by removing spaces
    return shortpath;
#endif
}


mExternC(Basic) const char* GetFullExecutablePath( void )
{
    static BufferString res;
    if ( res.isEmpty() )
    {
	FilePath executable = GetArgV()[0];
	if ( !executable.isAbsolute() )
	{
	    FilePath filepath = initialdir.buf();
	    filepath.add( GetArgV()[0] );
	    executable = filepath;
	}
	
	res = getShortPathName( executable.fullPath() );
    }
    
    return res;
}

