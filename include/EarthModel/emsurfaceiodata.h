#ifndef emsurfaceiodata_h
#define emsurfaceiodata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Jun 2003
 RCS:		$Id: emsurfaceiodata.h,v 1.13 2012-08-03 13:00:20 cvskris Exp $
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "cubesampling.h"
#include "bufstringset.h"


namespace EM
{

class Surface;

/*!\brief Data interesting for Surface I/O */

mClass(EarthModel) SurfaceIOData
{
public:
    			~SurfaceIOData()	{ clear(); }

    void                fillPar(IOPar&) const;
    void                usePar(const IOPar&);

    void		clear();
    void		use(const Surface&);

    BufferString	dbinfo;
    HorSampling		rg;			// 3D only
    Interval<float>	zrg;
    BufferStringSet	valnames;
    TypeSet<float>	valshifts_;
    BufferStringSet	sections;

    BufferStringSet	linenames;		// 2D only
    BufferStringSet	linesets;		// 2D only
    TypeSet<StepInterval<int> >	trcranges;	// 2D only
};


mClass(EarthModel) SurfaceIODataSelection
{
public:

    				SurfaceIODataSelection( const SurfaceIOData& s )
				    : sd(s)		{}

    const SurfaceIOData&	sd;

    HorSampling			rg;
    TypeSet<int>		selvalues; // Indexes in sd.valnames
    TypeSet<int>		selsections; // Indexes in sd.sections

    BufferStringSet		sellinenames;
    TypeSet<StepInterval<int> >	seltrcranges;

    void			setDefault(); // selects all
};



}; // namespace EM

#endif

