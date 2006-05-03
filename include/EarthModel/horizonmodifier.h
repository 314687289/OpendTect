#ifndef horizonmodifier_h
#define horizonmodifier_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		April 2006
 RCS:		$Id: horizonmodifier.h,v 1.1 2006-05-03 14:40:00 cvsnanne Exp $
________________________________________________________________________

-*/

#include "emposid.h"
#include "multiid.h"

namespace EM { class Horizon; }
class HorSampling;

class HorizonModifier
{
public:

				HorizonModifier();
				~HorizonModifier();

    enum ModifyMode		{ Shift, Remove };

    bool			setHorizons(const MultiID&,const MultiID&);
    void			setStaticHorizon(bool tophor);
    void			setMode(ModifyMode);

    void			doWork();

protected:

    void			getHorSampling(HorSampling&);
    void			shiftNode(const EM::SubID&);
    void			removeNode(const EM::SubID&);

    EM::Horizon*		tophor_;
    EM::Horizon*		bothor_;

    ModifyMode			modifymode_;
    bool			topisstatic_;
};


#endif
