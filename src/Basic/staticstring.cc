/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2011
 * FUNCTION : Functions for string manipulations
-*/

static const char* rcsID = "$Id: staticstring.cc,v 1.5 2011-09-20 13:03:15 cvskris Exp $";

#include "staticstring.h"

#include "keystrs.h"

BufferString& StaticStringManager::getString()
{
    const void* threadid = Threads::Thread::currentThread();
    Threads::MutexLocker lock( lock_ );
    int idx = threadids_.indexOf( threadid );
    if ( idx<0 )
    {
	idx = threadids_.size();
	threadids_ += threadid;
	strings_.add( sKey::EmptyString );
    }

    return *strings_[idx];
}


StaticStringManager::~StaticStringManager()
{
}
