#ifndef attribdataholder_h
#define attribdataholder_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribdataholder.h,v 1.20 2009-05-18 10:31:39 cvshelene Exp $
________________________________________________________________________

-*/

#include "cubesampling.h"
#include "refcount.h"
#include "samplingdata.h"
#include "sets.h"
#include "valseries.h"

class SeisTrcInfo;

namespace Attrib
{
class DataCubes;

    /*!\brief Holds the data used in the attribute data

      Basically, this is a set of ValueSeries<float> objects, the size of
      each of these, and the start Z in the AE Z-Axis definition:
      N = N times the Z step. z0_ is therefore the amount of steps away from 0.

      The AE will work with any type of ValueSeries<float>. Internally,
      ArrayValueSeries<float,float> objects are always allocated.

      The class variable extrazfromsamppos_ is to keep track of an eventual 
      exact position which would not be exactly on a sample ( in the case of 
      horizons, picksets... )

      */

mClass DataHolder
{
public:
			DataHolder( int z0, int nrsamples );
			~DataHolder();

    DataHolder*	        clone() const;
    ValueSeries<float>*	add(bool addnull=false);
			//!< Adds an ArrayValueSeries if !addnull

    int			nrSeries() const		{ return data_.size(); }
    inline ValueSeries<float>* series( int idx ) const	{ return gtSer(idx); }
    void		replace(int idx,ValueSeries<float>*);
    bool                dataPresent(int samplenr) const;
    TypeSet<int>	validSeriesIdx() const;
    float		getValue(int serieidx,float exactz,float refstep) const;

    inline bool		isEmpty() const		{ return nrSeries() == 0; }

    int			z0_;	//!< See class comments
    int			nrsamples_;
    float		extrazfromsamppos_;	//!< See class comments
    TypeSet<int>	classstatus_;	//each serie can have a different status
	    			 	// -1 Unknown
    					//  0 Interpolate
    					//  1 Classification

protected:

    ObjectSet< ValueSeries<float> >	data_;
    ValueSeries<float>*	gtSer( int idx ) const
    			{ return const_cast<ValueSeries<float>*>(data_[idx]); }

};


/*!Class that holds 2d data seismic or attribute data. */

mClass Data2DHolder
{ mRefCountImpl(Data2DHolder);
public:

    inline int			size() const	{ return dataset_.size(); }
    bool			fillDataCube(DataCubes&) const;
    CubeSampling		getCubeSampling() const;
    ObjectSet<DataHolder>	dataset_;
    				/*!<\note that z0 on the dataholder refers
				 	  to samples in trcinfoset_.sampling. */
    ObjectSet<SeisTrcInfo>	trcinfoset_;
    				/*!<\note that the sampling is the same
					  for all traces. */

    inline bool			isEmpty() const	{ return size() == 0; }

};


}; //Namespace


#endif
