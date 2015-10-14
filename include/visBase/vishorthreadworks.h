#ifndef vishorthreadworks_h
#define vishorthreadworks_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		March 2009
 RCS:		$Id$
________________________________________________________________________
-*/
// this header file only be used in the classes related to Horzonsection .
// don't include it in somewhere else !!!


#include "threadwork.h"
#include "paralleltask.h"
#include "thread.h"
#include "ranges.h"

namespace osg { class CullStack; }
namespace Geometry { class BinIDSurface; }

class BinIDSurface;
class ZAxisTransform;


namespace visBase
{
    class HorizonSection;
    class HorizonSectionTile;


class HorizonTileRenderPreparer: public ParallelTask
{ mODTextTranslationClass(HorizonTileRenderPreparer);
public:
    HorizonTileRenderPreparer( HorizonSection& hrsection,
			       const osg::CullStack* cs, char res );

    ~HorizonTileRenderPreparer()
    { delete [] permutation_; }

    od_int64 nrIterations() const { return nrtiles_; }
    od_int64 totalNr() const { return nrtiles_ * 2; }
    uiString uiMessage() const { return tr("Updating Horizon Display"); }
    uiString uiNrDoneText() const { return tr("Parts completed"); }

    bool doPrepare( int );
    bool doWork( od_int64, od_int64, int );

    od_int64*			permutation_;
    HorizonSectionTile**	hrsectiontiles_;
    HorizonSection&		hrsection_;
    int				nrtiles_;
    int				nrcoltiles_;
    char			resolution_;
    int				nrthreads_;
    int				nrthreadsfinishedwithres_;
    Threads::Barrier		barrier_;
    const osg::CullStack*	tkzs_;
};


class TileTesselator : public SequentialTask
{
public:
    TileTesselator( HorizonSectionTile* tile, char res );

    int	nextStep();
    HorizonSectionTile*		tile_;
    char			res_;
    bool			doglue_;
};


class HorizonSectionTilePosSetup: public ParallelTask
{ mODTextTranslationClass(HorizonSectionTilePosSetup);
public:
    HorizonSectionTilePosSetup(ObjectSet<HorizonSectionTile> tiles, 
	HorizonSection* horsection,StepInterval<int>rrg,StepInterval<int>crg );

    ~HorizonSectionTilePosSetup();

    od_int64	nrIterations() const { return hrtiles_.size(); }
    uiString	uiMessage() const { return tr("Creating Horizon Display"); }
    uiString	uiNrDoneText() const { return tr("Parts completed"); }

protected:

    bool doWork(od_int64, od_int64, int);
    bool doFinish(bool);

    int					nrcrdspertileside_;
    char				lowestresidx_;
    ObjectSet<HorizonSectionTile>	hrtiles_;
    const Geometry::BinIDSurface*	geo_;
    StepInterval<int>			rrg_, crg_;
    ZAxisTransform*			zaxistransform_;
    HorizonSection*			horsection_;
};


class TileGlueTesselator : public SequentialTask
{
public:
    TileGlueTesselator( HorizonSectionTile* tile );

    int	nextStep();
    HorizonSectionTile*		tile_;
};


}

#endif


