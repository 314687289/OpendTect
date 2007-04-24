#ifndef attribstorprovider_h
#define attribstorprovider_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribstorprovider.h,v 1.26 2007-04-24 08:45:20 cvsbert Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"
#include "cubesampling.h"
#include "datachar.h"
class SeisMSCProvider;
class SeisTrc;

namespace Attrib
{

class DataHolder;

class StorageProvider : public Provider
{
public:

    static void		initClass();
    static const char*  attribName()		{ return "Storage"; }
    static const char*  keyStr()		{ return "id"; }

    int			moveToNextTrace(BinID startpos=BinID(-1,-1),
	    				bool firstcheck=false);
    bool		getPossibleVolume(int outp,CubeSampling&);
    BinID		getStepoutStep() const;
    void		updateStorageReqs(bool all=true);
    void		adjust2DLineStoredVolume();
    void		fillDataCubesWithTrc(DataCubes*) const;

protected:

    			StorageProvider(Desc&);
    			~StorageProvider();

    static Provider*	createFunc(Desc&);
    static void		updateDesc(Desc&);

    bool		init();
    bool		allowParallelComputation() const { return false; }

    SeisMSCProvider*	getMSCProvider() const	{ return mscprov_; }
    bool		initMSCProvider();
    bool		setMSCProvSelData();

    void		setReqBufStepout(const BinID&,bool wait=false);
    void		setDesBufStepout(const BinID&,bool wait=false);
    bool        	computeData(const DataHolder& output,
				    const BinID& relpos,
				    int t0,int nrsamples,int threadid) const;

    bool		fillDataHolderWithTrc(const SeisTrc*,
					      const DataHolder&) const;
    bool		getZStepStoredData(float& step) const
			{ step = storedvolume.zrg.step; return true; }

    BinDataDesc		getOutputFormat(int output) const;
    
    bool 		checkDesiredTrcRgOK(StepInterval<int>,
	    				    StepInterval<float>);
    bool 		checkDesiredVolumeOK();
    void		checkClassType(const SeisTrc*,BoolTypeSet&) const;
    bool		setTableSelData();
    bool		set2DRangeSelData();

    TypeSet<BinDataDesc> datachar_;
    SeisMSCProvider*	mscprov_;

    CubeSampling	storedvolume;

    enum Status        { Nada, StorageOpened, Ready } status;
};

}; // namespace Attrib

#endif
