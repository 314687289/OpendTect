#ifndef timedepthconv_h
#define timedepthconv_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		September 2007
 RCS:		$Id$
________________________________________________________________________

*/


#include "seismod.h"
#include "zaxistransform.h"

#include "cubesampling.h"
#include "datapack.h"
#include "multidimstorage.h"
#include "veldesc.h"

class CubeDataPack;
class IOObj;
class SeisTrc;
class SeisTrcReader;
template <class T> class Array3D;
template <class T> class ValueSeries;


mExpClass(Seis) VelocityStretcher : public ZAxisTransform
{
public:
    virtual bool		setVelData(const MultiID&)		= 0;

    bool			canTransformSurv(Pos::SurvID sid) const
				{ return sid!=TrcKey::std2DSurvID(); }

    static const char*		sKeyTopVavg()	{ return "Top Vavg"; }
    static const char*		sKeyBotVavg()	{ return "Bottom Vavg"; }

protected:
    				VelocityStretcher(const ZDomain::Def& from,
						  const ZDomain::Def& to);
};


/*!ZAxisstretcher that converts from time to depth (or back) using a
   velocity model on disk. */

mExpClass(Seis) Time2DepthStretcher : public VelocityStretcher
{
public:
    mDefaultFactoryInstantiation( ZAxisTransform, Time2DepthStretcher,
	    			  "VelocityT2D", sFactoryKeyword() );

			Time2DepthStretcher();
    bool		setVelData(const MultiID&);
    bool		isOK() const;

    bool		needsVolumeOfInterest() const	{ return true; }
    int			addVolumeOfInterest(const CubeSampling&,bool);
    void		setVolumeOfInterest(int,const CubeSampling&,bool);
    void		removeVolumeOfInterest(int);
    bool		loadDataIfMissing(int,TaskRunner* =0);
    void		transformTrc(const TrcKey&,const SamplingData<float>&,
	    			  int,float*) const;
    void		transformTrcBack(const TrcKey&,
				const SamplingData<float>&,int,float*) const;
    Interval<float>	getZInterval(bool from) const;
    float		getGoodZStep() const;
    const char*		getToZDomainString() const;
    const char*		getFromZDomainString() const;
    const char*		getZDomainID() const;

    const Interval<float>& getVavgRg(bool start) const;
    static Interval<float> getDefaultVAvg();

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    friend		class TimeDepthDataLoader;
			~Time2DepthStretcher();
    void		releaseData();
    Interval<float>	getTimeInterval(const BinID&,int voiidx) const;
    Interval<float>	getDepthInterval(const BinID&,int voiidx) const;

    static void				udfFill(ValueSeries<float>&,int);

    ObjectSet<Array3D<float> >		voidata_;
    TypeSet<CubeSampling>		voivols_;
    BoolTypeSet				voiintime_;
    TypeSet<int>			voiids_;

    SeisTrcReader*			velreader_;
    VelocityDesc			veldesc_;
    bool				velintime_;

    Interval<float>			topvavg_; //Used to compute ranges
    Interval<float>			botvavg_; //Used to compute ranges
};


/*!ZAxisstretcher that converts from depth to time (or back). Uses
   an Time2Depth converter to do the job. */


mExpClass(Seis) Depth2TimeStretcher : public VelocityStretcher
{
public:
    mDefaultFactoryInstantiation( ZAxisTransform, Depth2TimeStretcher,
	    			  "VelocityD2T", sFactoryKeyword() );

			Depth2TimeStretcher();
    bool		setVelData(const MultiID&);
    bool		isOK() const;

    bool		needsVolumeOfInterest() const;
    int			addVolumeOfInterest(const CubeSampling&,bool);
    void		setVolumeOfInterest(int,const CubeSampling&,bool);
    void		removeVolumeOfInterest(int);
    bool		loadDataIfMissing(int,TaskRunner* =0);
    void		transformTrc(const TrcKey&,const SamplingData<float>&,
	    			  int,float*) const;
    void		transformTrcBack(const TrcKey&,
				  const SamplingData<float>&,int,float*) const;
    Interval<float>	getZInterval(bool from) const;
    float		getGoodZStep() const;
    const char*		getToZDomainString() const;
    const char*		getFromZDomainString() const;
    const char*		getZDomainID() const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
			~Depth2TimeStretcher()				{}

    RefMan<Time2DepthStretcher>		stretcher_;
};

/*! Scans a velocity model for minimum top/bottom average velocity. */

mExpClass(Seis) VelocityModelScanner : public SequentialTask
{
public:
    			VelocityModelScanner(const IOObj&,
				const VelocityDesc&);
			~VelocityModelScanner();

    uiString		uiMessage() const	{ return msg_.buf(); }
    od_int64		totalNr() const		{ return subsel_.totalNr(); }
    od_int64		nrDone() const		{ return nrdone_; }
    uiString		uiNrDoneText() const	{ return "Position scanned";}

    const Interval<float>&	getTopVAvg() const	{ return startavgvel_; }
    const Interval<float>&	getBotVAvg() const	{ return stopavgvel_; }

    int				nextStep();

protected:

    BufferString		msg_;
    HorSampling			subsel_;
    HorSamplingIterator		hsiter_;
    bool			definedv0_;
    bool			definedv1_;
    bool			zistime_;
    int				nrdone_;

    const IOObj&		obj_;
    const VelocityDesc&		vd_;

    SeisTrcReader*		reader_;

    Interval<float>		startavgvel_;
    Interval<float>		stopavgvel_;
};


mExpClass(Seis) LinearVelTransform : public ZAxisTransform
{
public:
    bool			usePar(const IOPar&);
    void			fillPar(IOPar&) const;

    bool			canTransformSurv(Pos::SurvID) const
				{ return true; }

protected:
				LinearVelTransform(const ZDomain::Def& from,
						   const ZDomain::Def& to,
						   float v0, float dv);
    void			transformT2D(const SamplingData<float>&,
					     int sz,float* res) const;
    void			transformD2T(const SamplingData<float>&,
					     int sz,float* res) const;
    
    float			startvel_;
    float			dv_;

};


mExpClass(Seis) LinearT2DTransform : public LinearVelTransform
{
public:
    mDefaultFactoryInstantiation( ZAxisTransform, LinearT2DTransform,
	    			  "LinearT2D", "Linear velocity" );

    				LinearT2DTransform(float v0=0, float dv=0);

    void			transformTrc(const TrcKey&,
	    				  const SamplingData<float>&,
					  int sz,float* res) const;
    void			transformTrcBack(const TrcKey&,
	    				      const SamplingData<float>&,
					      int sz,float* res) const;

    Interval<float>		getZInterval(bool time) const;
    float			getGoodZStep() const;
 
    bool			needsVolumeOfInterest() const
    				{ return false; }
};


mExpClass(Seis) LinearD2TTransform : public LinearVelTransform
{
public:
    mDefaultFactoryInstantiation( ZAxisTransform, LinearT2DTransform,
	    			  "LinearD2T", "Linear velocity" );

    				LinearD2TTransform(float v0=0, float dv=0);

    void			transformTrc(const TrcKey&,
	    				  const SamplingData<float>&,
					  int sz,float* res) const;
    void			transformTrcBack(const TrcKey&,
	    				      const SamplingData<float>&,
					      int sz,float* res) const;
    Interval<float>		getZInterval(bool depth) const;
    float			getGoodZStep() const;

    bool			needsVolumeOfInterest() const
    				{ return false; }
};

#endif

