#ifndef seisparallelreader_h
#define seisparallelreader_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		July 2010
 RCS:		$Id$
________________________________________________________________________

*/

#include "seismod.h"
#include "cubesampling.h"
#include "fixedstring.h"
#include "sets.h"
#include "paralleltask.h"

class BinIDValueSet;
class CubeSampling;
class IOObj;

template <class T> class Array3D;


namespace Seis
{


/*!Reads a 3D Seismic volume in parallel into an Array3D<float> or
   into a BinIDValueSet */

mExpClass(Seis) ParallelReader : public ParallelTask
{
public:
			ParallelReader(const IOObj&,
			    const TypeSet<int>& components,
			    const ObjectSet<Array3D<float> >&,
			    const CubeSampling&);
			/*!<Allocates & resizes the cubes to fit the cs and the
			    nr of comps. If data is missing in the storage, the
			    cube will not be overwritten in those locations. */

			ParallelReader(const IOObj&,
				const CubeSampling&);
			/*!<Calculates nr of comps and allocates cubes to
			    fit the cs. */

			ParallelReader(const IOObj&,
			    BinIDValueSet&,
			    const TypeSet<int>& components);
			/*!<Will read the z from the first value. Will add
			    values to accomodate nr of components. If data
			    cannot be read, that binid/z will be set to
			    mUdf */

			~ParallelReader();

    const ObjectSet<Array3D<float> >* getArrays() const	{ return arrays_; }

protected:
    od_int64		nrIterations() const { return totalnr_; }
    bool		doPrepare(int nrthreads);
    bool		doWork(od_int64,od_int64,int);
    bool		doFinish(bool);
    uiStringCopy		uiNrDoneText() const { return "Traces read"; }
    uiStringCopy	uiMessage() const { return errmsg_.isEmpty() ?
					   "Reading" : errmsg_;}


    TypeSet<int>		components_;

    BinIDValueSet*		bidvals_;

    ObjectSet<Array3D<float> >*	arrays_;
    CubeSampling		cs_;

    IOObj*			ioobj_;
    od_int64			totalnr_;

    uiString			errmsg_;
};

} // namespace Seis

#endif

