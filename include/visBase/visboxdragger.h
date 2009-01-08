#ifndef visboxdragger_h
#define visboxdragger_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		August 2002
 RCS:		$Id: visboxdragger.h,v 1.12 2009-01-08 10:15:41 cvsranojay Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "position.h"

class SoTabBoxDragger;
class SoDragger;
class SoSwitch;

template <class T> class Interval;

namespace visBase
{

mClass BoxDragger : public DataObject
{
public:
    static BoxDragger*		create()
				mCreateDataObj(BoxDragger);

    void			setCenter(const Coord3&);
    Coord3			center() const;
    
    void			setWidth(const Coord3&);
    Coord3			width() const;

    void			setSpaceLimits( const Interval<float>&,
	    					const Interval<float>&,
						const Interval<float>&);

    void			turnOn(bool yn);
    bool			isOn() const;


    bool			selectable() const { return selectable_; }
    void			setSelectable(bool yn) { selectable_ = yn; }

    Notifier<BoxDragger>	started;
    Notifier<BoxDragger>	motion;
    Notifier<BoxDragger>	changed;
    Notifier<BoxDragger>	finished;

    SoNode*			getInventorNode();

protected:
				~BoxDragger();
    void			setOwnShapeHints();

    static void			startCB( void*, SoDragger* );
    static void			motionCB( void*, SoDragger* );
    static void			valueChangedCB(void*, SoDragger* );
    static void			finishCB( void*, SoDragger* );

    SoSwitch*			onoff_;
    SoTabBoxDragger*		boxdragger_;

    Interval<float>*		xinterval_;
    Interval<float>*		yinterval_;
    Interval<float>*		zinterval_;

    Coord3			prevwidth_;
    Coord3			prevcenter_;
    bool			selectable_;
};

};
	
#endif
