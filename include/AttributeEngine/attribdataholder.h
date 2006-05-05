#ifndef attribdataholder_h
#define attribdataholder_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribdataholder.h,v 1.14 2006-05-05 20:35:16 cvskris Exp $
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
      ArrayValueSeries<float> objects are always allocated.

      */

class DataHolder
{
public:
			DataHolder( int z0, int nrsamples );
			~DataHolder();

    DataHolder*	        clone() const;
    ValueSeries<float>*	add(bool addnull=false);
			//!< Adds an ArrayValueSeries if !addnull

    int			nrSeries() const	{ return data_.size(); }
    ValueSeries<float>*	series(int idx) const	{ return data_[idx]; }
    void		replace(int idx,ValueSeries<float>*);
    bool                dataPresent(int samplenr) const;
    TypeSet<int>	validSeriesIdx() const;

    int			z0_;	//!< See class comments
    int			nrsamples_;

protected:

    ObjectSet< ValueSeries<float> >	data_;

};


/*!Class that holds 2d data seismic or attribute data. */

class Data2DHolder
{ mRefCountImpl(Data2DHolder);
public:
    int				size() const { return dataset_.size(); }
    bool			fillDataCube(DataCubes&) const;
    CubeSampling		getCubeSampling() const;
    ObjectSet<DataHolder>	dataset_;
    ObjectSet<SeisTrcInfo>	trcinfoset_;
};


}; //Namespace


#endif
