#ifndef visdragger_h
#define visdragger_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          December 2003
 RCS:           $Id: visdragger.h,v 1.13 2006-01-23 14:19:05 cvskris Exp $
________________________________________________________________________

-*/

#include "visobject.h"
#include "position.h"

class Color;

class SoDragger;
class SoSeparator;


namespace visBase
{

/*! \brief Class for simple draggers
*/

class Transformation;


class Dragger : public DataObject
{
public:
    static Dragger*		create()
    				mCreateDataObj(Dragger);

    enum Type			{ Translate1D, Translate2D, Translate3D };
    void			setDraggerType(Type);

    void			setPos(const Coord3&);
    Coord3			getPos() const;

    void			setSize(const Coord3&);
    Coord3			getSize() const;

    void			setRotation(const Coord3&,float);
    void			setDefaultRotation();
    void			setColor(const Color&);

    void			turnOn(bool);
    bool			isOn() const;

    void			setDisplayTransformation( Transformation* );
    Transformation*		getDisplayTransformation();

    void			setOwnShape(DataObject*,
	    				    const char* partname );
    				/*!< Sets a shape on the dragger.
				    \note The object will not be reffed,
					  so it's up to the caller to make sure
					  it remains in memory */
    SoNode*			getShape( const char* name );
    bool			selectable() const;

    Notifier<Dragger>		started;
    Notifier<Dragger>		motion;
    Notifier<Dragger>		finished;
    NotifierAccess*		rightClicked() { return &rightclicknotifier; }
    const TypeSet<int>*		rightClickedPath() const;
    const EventInfo*		rightClickedEventInfo() const;

    SoNode*			getInventorNode();

protected:
    				~Dragger();
    void			triggerRightClick(const EventInfo* eventinfo);

    static void			startCB(void*,SoDragger*);
    static void			motionCB(void*,SoDragger*);
    static void			finishCB(void*,SoDragger*);
    
    Notifier<Dragger>		rightclicknotifier;
    const EventInfo*		rightclickeventinfo;

    SoSwitch*			onoff;
    SoSeparator*		separator;
    Transformation*		positiontransform;
    SoDragger*			dragger;
    Transformation*		displaytrans;
};

} // namespace visBase

#endif
