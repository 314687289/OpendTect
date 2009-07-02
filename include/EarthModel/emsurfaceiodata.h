#ifndef emsurfaceiodata_h
#define emsurfaceiodata_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Jun 2003
 RCS:		$Id: emsurfaceiodata.h,v 1.11 2009-07-02 22:11:36 cvskris Exp $
________________________________________________________________________

-*/

#include "cubesampling.h"
#include "bufstringset.h"


namespace EM
{

class Surface;

/*!\brief Data interesting for Surface I/O */

mClass SurfaceIOData
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


mClass SurfaceIODataSelection
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
