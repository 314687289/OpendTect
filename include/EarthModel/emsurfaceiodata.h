#ifndef emsurfaceiodata_h
#define emsurfaceiodata_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Jun 2003
 RCS:		$Id: emsurfaceiodata.h,v 1.6 2004-08-16 16:25:03 bert Exp $
________________________________________________________________________

-*/

#include "cubesampling.h"
#include "bufstringset.h"


namespace EM
{

class Surface;

/*!\brief Data interesting for Surface I/O */

class SurfaceIOData
{
public:
    			~SurfaceIOData()	{ clear(); }

    void                fillPar(IOPar&) const;
    void                usePar(const IOPar&);

    void		clear();
    void		use(const Surface&);

    BufferString	dbinfo;
    HorSampling		rg;
    BufferStringSet	valnames;
    BufferStringSet	sections;

};


class SurfaceIODataSelection
{
public:

    				SurfaceIODataSelection( const SurfaceIOData& s )
				    : sd(s)		{}

    const SurfaceIOData&	sd;

    HorSampling			rg;
    TypeSet<int>		selvalues; // Indexes in sd.valnames
    TypeSet<int>		selsections; // Indexes in sd.sections

    void			setDefault(); // selects all
};



}; // Namespace

#endif
