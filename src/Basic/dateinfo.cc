/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 12-3-1996
 * FUNCTION : date info
-*/
 
static const char* rcsID = "$Id: dateinfo.cc,v 1.3 2002-12-03 13:18:18 kristofer Exp $";

#include "dateinfo.h"
#include <time.h>

static int d0[] = { 31,59,90,120,151,181,212,243,273,304,334,365 };
static int d1[] = { 31,60,91,121,152,182,213,244,274,305,335,366 };

DefineEnumNames(DateInfo,Day,2,"Week day") {
	"Sunday",
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday",
	0
};

DefineEnumNames(DateInfo,Month,3,"Month") {
	"January",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December",
	0
};


DateInfo::DateInfo()
{
    days96 = (time(0) - 820454400L)/86400L;
    calcDMY();
}


DateInfo::DateInfo( int yr, int mn, int dy )
	: year_(yr-1996)
	, month_((Month)(mn-1))
	, day_(dy-1)
{
    calcDays96();
}


DateInfo::DateInfo( int yr, DateInfo::Month m, int dy )
	: year_(yr-1996)
	, month_(m)
	, day_(dy-1)
{
    calcDays96();
}


DateInfo::DateInfo( int yr, const char* mn, int dy )
	: year_(yr-1996)
	, month_(eEnum(DateInfo::Month,mn))
	, day_(dy-1)
{
    calcDays96();
}


void DateInfo::setDay( int dy )
{
    day_ = dy - 1;
    calcDays96();
}


void DateInfo::setMonth( DateInfo::Month mn )
{
    month_ = mn;
    calcDays96();
}


void DateInfo::setYear( int yr )
{
    year_ = yr - 1996;
    calcDays96();
}


DateInfo& DateInfo::operator +=( int dys )
{
    days96 += dys;
    calcDMY();
    return *this;
}


DateInfo& DateInfo::operator -=( int dys )
{
    days96 -= dys;
    calcDMY();
    return *this;
}


void DateInfo::addMonths( int mns )
{
    if ( !mns ) return;

    int nr = (int)month_ + mns;
    if ( mns > 0 )
    {
	year_ += nr / 12;
	month_ = (Month)(nr % 12);
    }
    else
    {
	if ( nr >= 0 )
	    month_ = (Month)nr;
	else
	{
	    year_ += nr / 12 - 1;
	    nr = (-nr) % 12;
	    month_ = (Month)(12 - nr);
	}
    }
    calcDays96();
}


const char* DateInfo::weekDayName() const
{
    int nr = days96 % 7 + 1;
    if ( nr > 6 ) nr = 0;
    return eString(Day,nr);
}


int DateInfo::daysInMonth( int yr, DateInfo::Month m )
{
    int* d = yr % 4 ? d0 : d1;
    int nr = d[m];
    if ( m != Jan ) nr -= d[m-1];
    return nr;
}


void DateInfo::getDayMonth( int yr, int ndays, int& dy, DateInfo::Month& mn )
{
    int* d = yr % 4 ? d0 : d1;
    int mnth = 0;
    while ( ndays >= d[mnth] ) mnth++;
    dy = mnth ? ndays - d[mnth-1] : ndays;
    mn = (Month)mnth;
}


void DateInfo::calcDMY()
{
    int yrqtets = days96 / 1461;
    int qtetrest = days96 % 1461;
    year_ = 4 * yrqtets;
    if ( qtetrest > 1095 )	{ year_ += 3; qtetrest -= 1096; }
    else if ( qtetrest > 730 )	{ year_ += 2; qtetrest -= 731; }
    else if ( qtetrest > 365 )	{ year_ += 1; qtetrest -= 366; }

    getDayMonth( year_, qtetrest, day_, month_ );
}


void DateInfo::calcDays96()
{
    days96 = (year_/4) * 1461;
    int rest = year_ % 4;
    if ( rest )	{ days96 += 366; rest--; }
    if ( rest )	{ days96 += 365; rest--; }
    if ( rest )	days96 += 365;
    if ( month_ != Jan )
    {
	int* d = year_ % 4 ? d0 : d1;
	days96 += d[((int)month_)-1];
    }
    days96 += day_;
}


static UserIDString buf;

const char* DateInfo::whenRelative( const DateInfo* di ) const
{

    if ( di )	getRel( *di );
    else	getRelToday();

    return buf;
}


#define mRet(s) { buf = s; return; }

void DateInfo::getRel( const DateInfo& reld ) const
{
    if ( reld.days96 == days96 ) mRet("that day")
    int diff = days96 - reld.days96;
    if ( diff < 0 )
    {
	if ( diff == -1 ) mRet("the previous day")
	if ( diff == -7 ) mRet("one week earlier")
	if ( diff == -14 ) mRet("two weeks earlier")
	if ( diff == -21 ) mRet("three weeks earlier")
	if ( diff > -21 ) { buf = (-diff); buf += " days earlier"; return; }
    }
    else
    {
	if ( diff == 1 ) mRet("the next day")
	if ( diff == 7 ) mRet("one week later")
	if ( diff == 14 ) mRet("two weeks later")
	if ( diff == 21 ) mRet("three weeks later")
	if ( diff < 21 ) { buf = diff; buf += " days later"; return; }
    }

    if ( reld.month_ == month_ && reld.day_ == day_ )
    {
	buf = "exactly ";
	int difference = year_ - reld.year_;
	buf += abs(difference);
	buf += " year";
	if ( abs(difference) != 1 ) buf += "s";
	buf += difference > 0 ? " later" : " earlier";
	return;
    }

    if ( reld.year_ == year_ )
    {
	int difference = month_ - reld.month_;
	if ( difference > -2 && difference < 2 )
	{
	    if ( reld.day_ == day_ )
		mRet(difference<0?"one month earlier":"one month later")
	    buf = "the ";
	    addDay();
	    buf += " of ";
	    buf += difference ? (difference > 0 ? "the following" : "the previous") :"that";
	    buf += " month";
	    return;
	}
    }

    buf = monthName();
    buf += " the ";
    addDay();
    if ( reld.year_ != year_ ) { buf += ", "; buf += year(); }
}


void DateInfo::getRelToday() const
{
    DateInfo today;

    if ( today.days96 == days96 ) mRet("today")
    int diff = days96 - today.days96;
    if ( diff < 0 )
    {
	if ( diff == -1 ) mRet("yesterday")
	if ( diff == -2 ) mRet("the day before yesterday")
	if ( diff == -7 ) mRet("One week ago")
	if ( diff == -14 ) mRet("two weeks ago")
	if ( diff == -21 ) mRet("three weeks ago")
	if ( diff > -21 ) { buf = (-diff); buf += " days ago"; return; }
    }
    else
    {
	if ( diff == 1 ) mRet("tomorrow")
	if ( diff == 2 ) mRet("the day after tomorrow")
	if ( diff == 7 ) mRet("next week")
	if ( diff == 14 ) mRet("two weeks from now")
	if ( diff == 21 ) mRet("three weeks from now")
	if ( diff < 21 ) { buf = (-diff); buf += " days from now"; return; }
    }

    if ( today.month_ == month_ && today.day_ == day_ )
    {
	buf = "exactly ";
	int difference = year_ - today.year_;
	buf += abs(difference);
	buf += " years ";
	buf += difference > 0 ? "from now" : "ago";
	return;
    }

    if ( today.year_ == year_ )
    {
	int difference = month_ - today.month_;
	if ( difference > -2 && difference < 2 )
	{
	    if ( today.day_ == day_ )
		mRet(difference<0?"one month ago":"one month from now")
	    buf = "the ";
	    addDay();
	    buf += " of ";
	    buf += difference ? (difference > 0 ? "next" : "last") : "this";
	    buf += " month";
	    return;
	}
    }

    buf = day(); buf += " ";
    buf += monthName();
    if ( today.year_ != year_ ) { buf += " "; buf += year(); }
}


void DateInfo::addDay() const
{
    buf += day();
    int swdy = day_ > 12 ? day_%10 : day_;
    switch ( swdy )
    {
    case 0: buf += "st"; break; case 1: buf += "nd"; break;
    case 2: buf += "rd"; break; default: buf += "th"; break;
    }
}
