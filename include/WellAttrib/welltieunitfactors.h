#ifndef welltieunitfactors_h
#define welltieunitfactors_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: welltieunitfactors.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "commondefs.h"

class UnitOfMeasure;
namespace Well { class Log; }

namespace WellTie
{

mClass UnitFactors
{
public:

    double    		getVelFactor(const Well::Log&,bool issonic) const;
    double    		getDenFactor(const Well::Log&) const;

protected:
 
    const UnitOfMeasure* getUOM(const Well::Log&) const;
    double	 	calcVelFactor(const char*,bool) const;
    double 	 	calcVelFactor(const char*) const;
    double 	 	calcSonicVelFactor(const char*) const;
    double  	 	calcDensFactor(const char*) const;
};

};//namespace WellTie

#endif
