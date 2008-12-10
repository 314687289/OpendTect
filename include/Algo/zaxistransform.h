#ifndef zaxistransform_h
#define zaxistransform_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          October 2006
 RCS:           $Id: zaxistransform.h,v 1.18 2008-12-10 13:45:17 cvskris Exp $
________________________________________________________________________

-*/

#include "enums.h"
#include "factory.h"
#include "position.h"
#include "ranges.h"
#include "refcount.h"
#include "samplingdata.h"

class Coord3;
class CubeSampling;
class IOPar;
class TaskRunner;

/*! Baseclass for z stretching in different ways. The z-stretch may be dependent
on the location (binid). The various transforms can be retrieved from factory
ZATF().
*/

class ZAxisTransform
{ mRefCountImpl(ZAxisTransform);
public:
    			ZAxisTransform();
    virtual const char*	name() const 				= 0;
    virtual bool	isOK() const				{ return true; }

    virtual bool	needsVolumeOfInterest() const		{ return true; }
    virtual int		addVolumeOfInterest(const CubeSampling&,
	    				    bool zistrans=false);
    virtual void	setVolumeOfInterest(int,const CubeSampling&,
	    				    bool zistrans=false);
    virtual void	removeVolumeOfInterest(int);
    virtual bool	loadDataIfMissing(int,TaskRunner* =0);
    				
    virtual void		transform(const BinID&, 
	    				  const SamplingData<float>&,
					  int sz,float* res) const	= 0;
    virtual float		transform(const BinIDValue&) const;
    float			transform(const Coord3&) const;
    virtual void		transformBack(const BinID&,
	    				   const SamplingData<float>&,
					   int sz,float* res) const	= 0;
    virtual float		transformBack(const BinIDValue&) const;
    float			transformBack(const Coord3&) const;

    virtual Interval<float>	getZInterval(bool from) const		= 0;
    				/*!<\Returns the z interval in either to
				     or from domain. */
    virtual float		getZIntervalCenter(bool from) const;
    				/*!\Returns a position within the
				    z-range that is a logical 'center' */
    virtual float		getGoodZStep() const;
    				/*!\returns a reasonable step in the transformed domain. Default
					    implementation gives the same step as in SI()
					    (i.e. non transformed domain) */

    virtual int			lineIndex( const char* linename ) const
				{ return 0; }
    				//!\Returns the index of a line in a 2D lineset.

    virtual NotifierAccess*	changeNotifier()		{ return 0; }
    virtual void		fillPar(IOPar&) const		{}
    virtual bool		usePar(const IOPar&)		{ return true; }
};


mDefineFactory( ZAxisTransform, ZATF );


class ZAxisTransformSampler
{
public:
    				ZAxisTransformSampler( const ZAxisTransform&,
				   bool back, const BinID& bid,
				   const SamplingData<double>& );
    virtual			~ZAxisTransformSampler();
    void			setBinID(const BinID& nbid) { bid_=nbid; }

    float			operator[](int idx) const;
    void			computeCache(const Interval<int>& range);

protected:
    const ZAxisTransform&	transform_;
    bool			back_;
    BinID			bid_;
    const SamplingData<double>	sd_;

    TypeSet<float>		cache_;
    int				firstcachesample_;
};

#endif
