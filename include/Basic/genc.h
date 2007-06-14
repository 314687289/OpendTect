#ifndef genc_H
#define genc_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 RCS:		$Id: genc.h,v 1.30 2007-06-14 11:22:37 cvsbert Exp $
________________________________________________________________________

Some general utilities, that need to be accessible in many places:

-*/

#ifndef gendefs_H
#include "gendefs.h"
#endif

#ifdef __cpp__
extern "C" {
#endif

const char*	GetProjectVersionName(void);
		/*!< "dTect Vx.x" */

int		GetPID();
		/*!< returns process ID */

const char*	GetLocalHostName();
		/*!< returns (as expected) local host name */

int		ExitProgram( int ret );
		/*!< Win32: kills progam itself and ignores ret.
		     Unix: uses exit(ret).
		     Return value is convenience only, so you can use like:
		     return exitProgram( retval );
                */

void		PutIsLittleEndian(unsigned char*);
		/*!< Puts into 1 byte: 0=SunSparc/SGI (big), 1=PC (little) */

void		SwapBytes(void*,int nbytes);
		/*!< nbytes=2,4,... e.g. nbytes=4: abcd becomes cdab */

#ifdef __cpp__
}
#else
/* C only */

typedef char	FileNameString[PATH_LENGTH+1];
typedef char	UserIDString[mMaxUserIDLength+1];

#endif


#endif
