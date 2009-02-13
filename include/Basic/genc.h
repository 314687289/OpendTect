#ifndef genc_h
#define genc_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 RCS:		$Id: genc.h,v 1.38 2009-02-13 13:31:14 cvsbert Exp $
________________________________________________________________________

Some general utilities, that need to be accessible in many places:

-*/

#ifndef gendefs_h
#include "gendefs.h"
#endif

#include "string2.h"

#ifdef __cpp__
extern "C" {
#endif

mGlobal const char* GetProjectVersionName(void);
		/*!< "dTect Vx.x" */

mGlobal int GetPID();
		/*!< returns process ID */

mGlobal const char* GetLocalHostName();
		/*!< returns (as expected) local host name */

mGlobal int isProcessAlive(int pid);
		/*!< returns 1 if the process is still running */

mGlobal int ExitProgram( int ret );
		/*!< Win32: kills progam itself and ignores ret.
		     Unix: uses exit(ret).
		     Return value is convenience only, so you can use like:
		     return exitProgram( retval );
                */

typedef void (*PtrAllVoidFn)(void);
mGlobal void NotifyExitProgram(PtrAllVoidFn);
		/*!< Function will be called on 'ExitProgram' */

mGlobal void PutIsLittleEndian(unsigned char*);
		/*!< Puts into 1 byte: 0=SunSparc/SGI (big), 1=PC (little) */

mGlobal void SwapBytes(void*,int nbytes);
		/*!< nbytes=2,4,... e.g. nbytes=4: abcd becomes cdab */

mGlobal int InSysAdmMode();
		/*!< returns 0 unless in sysadm mode */

#ifdef __cpp__
}
#else
/* C only */

typedef char	FileNameString[mMaxFilePathLength+1];
typedef char	UserIDString[mMaxUserIDLength+1];

#endif


#endif
