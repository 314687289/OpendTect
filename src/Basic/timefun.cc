/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 3-5-1994
 * FUNCTION : Functions for time
-*/

static const char* rcsID = "$Id: timefun.cc,v 1.23 2011-08-12 13:32:49 cvskris Exp $";

#include "timefun.h"
#include "bufstring.h"
#include "staticstring.h"

#include <QDateTime>
#include <QTime>

namespace Time
{

Counter::Counter()
    : qtime_(*new QTime)
{}

Counter::~Counter()
{ delete &qtime_; }

void Counter::start()
{ qtime_.start(); }

int Counter::restart()
{ return qtime_.restart(); }

int Counter::elapsed() const
{ return qtime_.elapsed(); }


int getMilliSeconds()
{
    QTime daystart;
    return daystart.msecsTo( QTime::currentTime() );
}


int passedSince( int timestamp )
{
    int elapsed = timestamp > 0 ? getMilliSeconds() - timestamp : -1;
    if ( elapsed < 0 && timestamp > 0 && (elapsed + 86486400 < 86400000) )
	elapsed += 86486400;

    return elapsed;
}


const char* defDateTimeFmt()	{ return "ddd dd MMM yyyy, hh:mm:ss"; }
const char* defDateFmt()	{ return "ddd dd MMM yyyy"; }
const char* defTimeFmt()	{ return "hh:mm:ss"; }

const char* getDateTimeString( const char* fmt, bool local )
{
    BufferString& datetimestr = StaticStringManager::STM().getString();
    QDateTime qdt = QDateTime::currentDateTime();
    if ( !local ) qdt = qdt.toUTC();
    datetimestr = qdt.toString( fmt ).toAscii().constData();
    return datetimestr.buf();
}

const char* getDateString( const char* fmt, bool local )
{ return getDateTimeString( fmt, local ); }

const char* getTimeString( const char* fmt, bool local )
{ return getDateTimeString( fmt, local ); }

bool isEarlier(const char* first, const char* second, const char* fmt )
{
    QString fmtstr( fmt );
    QDateTime qdt1 = QDateTime::fromString( first, fmtstr );
    QDateTime qdt2 = QDateTime::fromString( second, fmtstr );
    return qdt1 < qdt2;
}

} // namespace Time
