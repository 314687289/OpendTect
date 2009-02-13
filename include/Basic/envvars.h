#ifndef envvars_h
#define envvars_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Aug 2005
 RCS:		$Id: envvars.h,v 1.5 2009-02-13 13:31:14 cvsbert Exp $
________________________________________________________________________

-*/

#ifndef gendefs_h
#include "gendefs.h"
#endif

#ifdef __cpp__
extern "C" {
#endif

mGlobal const char* GetEnvVar(const char*);
		/*!< getenv or other source */
mGlobal int GetEnvVarYN(const char*);
		/*!< Returns 0=NO if not set or explicitly on 0 or "no". */
mGlobal int GetEnvVarIVal(const char*,int defltval);
mGlobal double GetEnvVarDVal(const char*,double defltval);

mGlobal char* GetOSEnvVar(const char*);
		/*!< Raw 'getenv' call */
mGlobal int SetEnvVar(const char* env,const char* val);
		/*!< sets environment variable to a value. */

mGlobal char GetEnvSeparChar();
		/*!< The character separating entries in an env variable */

#ifdef __cpp__
}
#endif


#endif
