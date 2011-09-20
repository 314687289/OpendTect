#ifndef staticstring_h
#define staticstring_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2011
 RCS:		$Id: staticstring.h,v 1.5 2011-09-20 13:03:15 cvskris Exp $
________________________________________________________________________
-*/


#include "sets.h"
#include "thread.h"
#include "bufstringset.h"


/*!Class that keeps one static string per thread. This enables temporary passing
   of static strings where needed. */

mClass StaticStringManager
{
public:
    BufferString&		getString();

    				~StaticStringManager();
protected:

    BufferStringSet     	strings_;
    ObjectSet<const void>     	threadids_;
    Threads::Mutex		lock_;
};

#endif
