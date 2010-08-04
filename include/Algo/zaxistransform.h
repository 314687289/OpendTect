#ifndef zaxistransform_h
#define zaxistransform_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          October 2006
 RCS:           $Id: zaxistransform.h,v 1.28 2010-08-04 13:30:46 cvsbert Exp $
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

namespace ZDomain { class Def; class Info; }

/*! Baseclass for z stretching in different ways. The z-stretch may be dependent
on the location (binid). The various transforms can be retrieved from factory
ZATF().
*/

mClass ZAxisTransform
{ mRefCountImpl(ZAxisTransform);
public:

    static ZAxisTransform*	create(const IOPar&);
    				/*!<\note Result will be reffed once. It is
				          caller's responsibility to unref. */
    virtual const char*		name() const 		= 0;
    virtual bool		isOK() const		{ return true; }
    virtual const char*		errMsg()		{ return errmsg_; }

    virtual bool		needsVolumeOfInterest() const	{ return true; }
    virtual int			addVolumeOfInterest(const CubeSampling&,
	    					    bool zistrans=false);
    virtual int			addVolumeOfInterest(const char* linenm,
						    const CubeSampling&,
						    bool zistrans=false);
    virtual void		setVolumeOfInterest(int,const CubeSampling&,
	    					    bool zistrans=false);
    virtual void		setVolumeOfInterest(int,const char* linenm,
						    const CubeSampling&,
						    bool zistrans=false);
    virtual void		removeVolumeOfInterest(int);
    virtual bool		loadDataIfMissing(int,TaskRunner* =0);
    				
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

    				// 2D
    virtual void		transform(const char* linenm,int trcnr,
	    				  const SamplingData<float>&,
					  int sz,float* res) const;
    virtual float		transform(const char* linenm,int trcnr,
					  float z) const;
    virtual void		transformBack(const char* linenm,int trcnr,
					      const SamplingData<float>&,
					      int sz,float* res) const;
    virtual float		transformBack(const char* linenm,int trcnr,
					      float z) const;

    virtual Interval<float>	getZInterval(bool from) const		= 0;
    				/*!<\Returns the z interval in either to
				     or from domain. */
    virtual float		getZIntervalCenter(bool from) const;
    				/*!\Returns a position within the
				    z-range that is a logical 'center' */
    virtual float		getGoodZStep() const;
    				/*!\returns a reasonable step in the
				    transformed domain. Default
				    implementation gives the same step as in
				    SI() (i.e. non transformed domain) */

    ZDomain::Info&		fromZDomainInfo() { return fromzdomaininfo_; }	
    ZDomain::Info&		toZDomainInfo()	  { return tozdomaininfo_; }
    const ZDomain::Info&	fromZDomainInfo() const;
    const ZDomain::Info&	toZDomainInfo() const;
    const char*			fromZDomainKey() const;
    const char*			toZDomainKey() const;

    virtual int			lineIndex( const char* linename ) const
				{ return 0; }
    				//!\Returns the index of a line in a 2D lineset.

    virtual NotifierAccess*	changeNotifier()		{ return 0; }
    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

protected:
    				ZAxisTransform(const ZDomain::Def& from,
					       const ZDomain::Def& to);

    ZDomain::Info&		tozdomaininfo_;
    ZDomain::Info&		fromzdomaininfo_;
    mutable BufferString	errmsg_;
};


mDefineFactory( ZAxisTransform, ZATF );


mClass ZAxisTransformSampler
{
public:
    				ZAxisTransformSampler(const ZAxisTransform&,
				   bool back,const SamplingData<double>&);
    virtual			~ZAxisTransformSampler();

    void			setBinID( const BinID& bid )	{ bid_ = bid; }
    void			setLineName(const char*);
    void			setTrcNr(int);

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
