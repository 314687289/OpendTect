#ifndef horizonmodifier_h
#define horizonmodifier_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		April 2006
 RCS:		$Id: horizonmodifier.h,v 1.5 2009-01-15 06:46:03 cvsraman Exp $
________________________________________________________________________

-*/

#include "emposid.h"
#include "multiid.h"
#include "ranges.h"

namespace EM { class Horizon; }
class BinID;
class BufferStringSet;
class HorSamplingIterator;

mClass HorizonModifier
{
public:

				HorizonModifier(bool is2d=false);
				~HorizonModifier();

    enum ModifyMode		{ Shift, Remove };

    bool			setHorizons(const MultiID&,const MultiID&);
    void			setStaticHorizon(bool tophor);
    void			setMode(ModifyMode);

    void			doWork();

protected:

    bool			getNextNode(BinID&);
    bool			getNextNode3D(BinID&);
    bool			getNextNode2D(BinID&);
    void			getLines(const EM::Horizon*);
    float			getDepth2D(const EM::Horizon*,const BinID&);
    void			shiftNode(const BinID&);
    void			removeNode(const BinID&);

    EM::Horizon*		tophor_;
    EM::Horizon*		bothor_;

    bool			is2d_;
    BufferStringSet*		linenames_;
    TypeSet<StepInterval<int> >	trcrgs_;
    HorSamplingIterator*	iter_;

    ModifyMode			modifymode_;
    bool			topisstatic_;
};


#endif
