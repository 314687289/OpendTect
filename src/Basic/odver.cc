/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id$
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "odver.h"
#include "oddirs.h"
#include "odinst.h"
#include "odplatform.h"

#include "bufstring.h"
#include "file.h"
#include "filepath.h"
#include "strmprov.h"
#include "staticstring.h"

#include <iostream>


DefineEnumNames(OD::Platform,Type,0,"Platform")
{
	"Linux (32 bits)",
	"Linux (64 bits)",
	"Windows (32 bits)",
	"Windows (64 bits)",
	"Mac OS X",
	0
};


extern "C" const char* GetFullODVersion()
{
    mDeclStaticString( ret );
    if ( !ret.isEmpty() ) return ret.buf();

    GetSpecificODVersion( 0, ret );

    if ( ret.isEmpty() )
    {
	const char* pvnm = GetProjectVersionName();
	pvnm = strrchr( pvnm, 'V' );
	if ( pvnm )
	    ret = pvnm + 1;

	if ( ret.isEmpty() )
	    ret = "0.0.0";
    }

    return ret.buf();
}


void GetSpecificODVersion( const char* typ, BufferString& res )
{
    res = ODInst::getPkgVersion( typ ? typ : "basedata" );
    if ( res.matches( "*error*" ) )
	res.setEmpty();
}


const OD::Platform& OD::Platform::local()
{
    static Platform theplf;
    return theplf;
}


OD::Platform::Platform()
    : type_(__ismac__ ?				     Mac
		      : (__iswin__ ? (__is32bits__ ? Win32 : Win64)
				   : (__is32bits__ ? Lin32 : Lin64)))
{
}


const char* OD::Platform::shortName() const
{
    return isLinux()	? (type_ == Lin32 ? "lux32" : "lux64")
	: (isWindows()	? (type_ == Win32 ? "win32" : "win64")
					  : "mac" );
}


void OD::Platform::set( const char* s, bool isshort )
{
    if ( !isshort )
	parseEnumType( s, type_ );
    else
    {
	const bool islin = *s == 'l';
	const bool iswin = *s == 'w';
	const bool is64 = *(s+strlen(s)-1) == '4';

	type_ = islin ? (is64 ? Lin64 : Lin32)
	    : (iswin ? (is64 ? Win64 : Win32)
		     : Mac);
    }
}


bool OD::Platform::isValidName( const char* s, bool isshort )
{
    if ( !s || !*s )	return false;
    if ( !isshort )	return TypeDef().isValidName( s );

    const BufferString cmp(s);
    return cmp == "lux64" || cmp == "win64"
	|| cmp == "lux32" || cmp == "win32"
	|| cmp == "mac";
}
