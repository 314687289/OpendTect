#ifndef randcolor_h
#define randcolor_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		November 2006
 RCS:		$Id: randcolor.h,v 1.3 2009-07-22 16:01:16 cvsbert Exp $
________________________________________________________________________

-*/

#include "color.h"
#include "statrand.h"

inline Color getRandomColor( bool withtransp=false )
{
    Stats::RandGen::init();
    return Color( Stats::RandGen::getIndex(255),
	          Stats::RandGen::getIndex(255),
		  Stats::RandGen::getIndex(255),
		  withtransp ? Stats::RandGen::getIndex(255) : 0 );
}


static Color getRandStdDrawColor()
{
    static int curidx = -1;
    if ( curidx == -1 )
    {
	Stats::RandGen::init();
	curidx = Stats::RandGen::getIndex( Color::nrStdDrawColors() );
    }
    else
    {
	curidx++;
	if ( curidx == Color::nrStdDrawColors() )
	    curidx = 0;
    }

    return Color::stdDrawColor( curidx );
}

#endif
