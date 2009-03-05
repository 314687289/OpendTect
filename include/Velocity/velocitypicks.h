#ifndef poststackpick_h
#define poststackpick_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: velocitypicks.h,v 1.1 2009-03-05 13:25:32 cvskris Exp $
________________________________________________________________________


-*/

#include "multidimstorage.h"
#include "callback.h"
#include "emposid.h"
#include "multiid.h"
#include "ranges.h"
#include "refcount.h"
#include "rowcol.h"

class IOObjContext;
class Undo;
class IOObj;
class BinID;
template <class T> class Smoother1D;
class BinIDValueSet;

namespace EM { class Horizon3D; }

namespace Vel
{

class PicksMgr;

class Pick
{
public:
    			Pick(float depth=mUdf(float),
			     float vel=mUdf(float),
			     float offset=mUdf(float),EM::ObjectID=-1);
    bool		operator==(const Pick& b) const;

    float		depth_;
    float		offset_;
    float		vel_;
    EM::ObjectID	emobjid_;
};


/*!Holds picks that the user has done, typically in a semblance plot. */

class Picks : public CallBacker
{ mRefCountImpl(Picks);
public:
    			Picks();
    			Picks(bool zit);

    Undo&		undo();

    void		removeAll(bool undo=true,bool interactionend=true);
    bool		isEmpty() const;


    void			setSnappingInterval(const StepInterval<float>&);
    const StepInterval<float>&	getSnappingInterval() const { return snapper_; }
    RowCol			find(const BinID&,const Pick&) const;
    RowCol			set(const BinID&, const Pick&,
	    			    bool undo=true,bool interactionend=true);
    void			set(const RowCol&,
	    			    const Pick&,bool undo=true,
				    bool interactionend=true);
    int				get(const BinID&, TypeSet<float>* depths,
				    TypeSet<float>* vals,
				    TypeSet<RowCol>*,
	   			    TypeSet<EM::ObjectID>*,
	   			    bool interpolatehors ) const;
    				//!<\returns number of picks
    void			get(const BinID&, TypeSet<Pick>&,
	   			    bool interpolatehors,
	   			    bool normalizerefoffset ) const;
    				//!<\returns number of picks
    bool			get(const RowCol&, BinID&, Pick& );
    void			get(const EM::ObjectID&,TypeSet<RowCol>&) const;
    void			remove(const RowCol&,
	   			       bool undo=true,bool interactionend=true);

    const MultiDimStorage<Pick>& getAll() const { return picks_; }

    CNotifier<Picks,const BinID&> change;
    CNotifier<Picks,const BinID&> changelate;
    				/*!<Triggers after pickchange. */

    bool			isChanged() const;

    bool			store(const IOObj*);
    				/*!< ioobj is not transferred */

    const MultiID&		storageID() const;
    bool			zIsTime() const		{ return zit_; }

    Smoother1D<float>*		getSmoother()		{ return smoother_; }
    const Smoother1D<float>*	getSmoother() const	{ return smoother_; }
    void			setSmoother(Smoother1D<float>*);
    				//!<Becomes mine

    const char*			errMsg() const;
    static const char*		sKeyIsTime();

    void			setAll(float vel,bool undo=true);

    static const IOObjContext&	getStorageContext();

    float			refOffset() const	{ return refoffset_; }
    void			setReferenceOffset(float n);

    const MultiID&		gatherID() const;
    void			setGatherID(const MultiID&);


    void			addHorizon(const MultiID&,
	    				   bool addzeroonfail=false);
    void			addHorizon(EM::Horizon3D*);
    int				nrHorizons() const;

    EM::ObjectID		getHorizonID(int) const;
    void			removeHorizon(EM::ObjectID);
    EM::Horizon3D*		getHorizon(EM::ObjectID);
    const EM::Horizon3D*	getHorizon(EM::ObjectID) const;
    bool			interpolateVelocity(EM::ObjectID,
	    			    float searchradius,BinIDValueSet&) const;
    				/*!<Interpolates vel at all locations in
				    the valset. First value in valset will
				    be horizon depth, second will be velocity.*/

    static const char*		sKeyIsVelPick();
    static const char*		sKeyRefOffset();
    static const char*		sKeyGatherID();
    static const char*		sKeyNrHorizons();
    static const char*		sKeyHorizonPrefix();

protected:
    friend			class PicksMgr;
    void			fillIOObjPar(IOPar&) const;
    bool			useIOObjPar(const IOPar&);
    float			normalizeRMO(float depth,float rmo,
	    				     float offset) const;
    				/*!<Given an rmo at a certain offset,
				    what is the rmo at refoffset_. */

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    void			horizonChangeCB(CallBacker*);

    float			refoffset_;
    MultiID			gatherid_;
    bool			load(const IOObj*);
    StepInterval<float>		snapper_;
    MultiID			storageid_;
    MultiDimStorage<Pick>	picks_;

    Undo*			undo_;

    BufferString		errmsg_;
    bool			changed_;
    bool			zit_;
    Smoother1D<float>*		smoother_;

    ObjectSet<EM::Horizon3D>	horizons_;
};


class PicksMgr : public CallBacker
{
public:
    				PicksMgr();
				~PicksMgr();

    Picks*		get(const MultiID&,bool gathermid,
	    			    bool create, bool forcefromstorage );

    const char*			errMsg() const;
protected:
    friend			class Picks;

    void			surveyChange(CallBacker*);

    BufferString		errmsg_;
    ObjectSet<Picks>	velpicks_;
};


PicksMgr& VPM();

const IOObjContext& getRMOPickStorageContext();


}; //namespace

#endif
