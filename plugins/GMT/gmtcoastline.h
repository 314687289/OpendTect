#ifndef gmtcoastline_h
#define gmtcoastline_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id: gmtcoastline.h,v 1.2 2009-07-22 16:01:26 cvsbert Exp $
________________________________________________________________________

-*/

#include "gmtpar.h"


class GMTCoastline : public GMTPar
{
public:

    static void		initClass();

    			GMTCoastline(const char* nm)
			    : GMTPar(nm)	{}
			GMTCoastline(const IOPar& par)
			    : GMTPar(par) {}

    virtual bool	execute(std::ostream&,const char*);
    virtual const char* userRef() const;
    bool		fillLegendPar(IOPar&) const;

protected:

    static GMTPar*	createInstance(const IOPar&);
    static int		factoryid_;

    bool		makeLLRangeFile(const char*) const;
};

#endif
