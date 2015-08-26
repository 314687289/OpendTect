/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2014
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uistring.h"
#include "testprog.h"

#include <QString>


bool testSetEmpty()
{
    uiString str = uiString("Hello");
    str.setEmpty();
    mRunStandardTest( str.isEmpty(), "isEmpty" );

    str.setFrom( QString("Hello" ) );
    str.setEmpty();
    mRunStandardTest( str.isEmpty(), "isEmpty after setFrom(QString)" );

    return true;
}


bool testArg()
{
    const char* desoutput = "Hello Dear 1";

    uiString string = uiString( "Hello %1 %2").arg( "Dear" ).arg( toString(1) );
    mRunStandardTest( string.getQtString()==QString( desoutput ),
		     "Standard argument order");

    string = uiString( "Hello %2 %1").arg( toString( 1 ) ).arg( "Dear" );
    mRunStandardTest( string.getQtString()==QString(desoutput),
		     "Reversed argument order");

    string = uiString( "Hello %1 %2");
    string.arg( "Dear" ).arg( toString(1) );
    mRunStandardTest( string.getQtString()==QString(desoutput),
		     "In-place");


    BufferString expargs = string.getFullString();

    mRunStandardTest( expargs==desoutput, "Argument expansion" );

    uiString cloned;
    cloned.setFrom( string );

    mRunStandardTest( string.getQtString()==cloned.getQtString(), "copyFrom" );

    uiString part1( "Part 1" );
    part1.append( ", Part 2" );
    mRunStandardTest(
	    FixedString(part1.getFullString())=="Part 1, Part 2", "append" );

    return true;
}


bool testSharedData()
{
    uiString a = uiString("Hello %1%2").arg( "World" );
    uiString b = a;

    b.arg( "s" );
    mRunStandardTest( b.getFullString()=="Hello Worlds" &&
		      BufferString(a.getFullString())!=
		      BufferString(b.getFullString()), "arg on copy" );

    uiString c = b;
    c = "Another message";
    mRunStandardTest( BufferString(c.getFullString())!=
		      BufferString(b.getFullString()),
		      "assignment of copy" );

    uiString d = b;
    mRunStandardTest( d.getOriginalString()==b.getOriginalString(),
		      "Use of same buffer on normal operations" );

    return true;
}


int main( int argc, char** argv )
{
    mInitTestProg();

    if ( !testArg() || !testSharedData() || !testSetEmpty() )
	ExitProgram( 1 );

    ExitProgram( 0 );
}
