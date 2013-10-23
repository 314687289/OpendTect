#ifndef visrandomtrackdragger_h
#define visrandomtrackdragger_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Feb 2006
 RCS:           $Id$
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visobject.h"
#include "position.h"
#include "ranges.h"


namespace visBase
{

/*! \brief Class for simple draggers
*/

class Transformation;


mExpClass(visBase) RandomTrackDragger : public VisualObjectImpl
{
public:
    static RandomTrackDragger*	create()
    				mCreateDataObj(RandomTrackDragger);


    void			setSize(const Coord3&);
    Coord3			getSize() const;

    int				nrKnots() const;
    Coord			getKnot(int) const;
    void			setKnot(int,const Coord&);
    void			insertKnot(int,const Coord&);
    void			removeKnot(int);

    Interval<float>		getDepthRange() const;
    void			setDepthRange(const Interval<float>&);

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    void			setLimits(const Coord3& start,
	    				  const Coord3& stop,
					  const Coord3& step); 

    void			showFeedback(bool yn);

    CNotifier<RandomTrackDragger,int> motion;
    NotifierAccess*		rightClicked() { return &rightclicknotifier_; }
    const TypeSet<int>*		rightClickedPath() const;
    const EventInfo*		rightClickedEventInfo() const;

protected:
    				~RandomTrackDragger();
    void			triggerRightClick(const EventInfo* eventinfo);

/* OSG-TODO: Replace with OSG based dragger manipulator
    static void		motionCB(void*,SoRandomTrackLineDragger*);
    SoRandomTrackLineDragger*	dragger_;
*/
    Notifier<RandomTrackDragger> rightclicknotifier_;
    const EventInfo*		rightclickeventinfo_;

    const mVisTrans*		displaytrans_;

    static const char*		sKeyDraggerScale() { return "subDraggerScale"; }
};

} // namespace visBase

#endif

