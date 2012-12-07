#ifndef visevent_h
#define visevent_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "keyenum.h"
#include "visdata.h"
#include "visosg.h"
#include "position.h"
#include "trigonometry.h"

class TabletInfo;

namespace osg { class Node; }


namespace visBase
{

/*!\brief


*/

class EventCatchHandler;
class Detail;


enum EventType		{ Any=7, MouseClick=1, Keyboard=2, MouseMovement=4 };

mClass(visBase) EventInfo
{
public:
    				EventInfo();
				EventInfo(const EventInfo&);

				~EventInfo();
    EventInfo&			operator=(const EventInfo&);

    EventType			type;

    OD::ButtonState		buttonstate_;

    Coord			mousepos;

    Line3			mouseline;
    				/*!< The line projected from the mouse-position
				     into the scene. Line is in worldcoords.
				*/

    bool			pressed;
				/*!< Only set if type == MouseClick or Keyboard
				     If false, the button has been released.
				*/
    bool			dragging;
    				//!< Only set if type == MouseMovement

    int				key;
    				//!< Only set if type == Keyboard

    TypeSet<int>		pickedobjids;

    Coord3			displaypickedpos;	// display space
    Coord3			localpickedpos; 	// object space
    Coord3			worldpickedpos; 	// world space

    TabletInfo*			tabletinfo;
    void			setTabletInfo(const TabletInfo*);
};


mClass(visBase) EventCatcher : public DataObject
{
    friend class EventCatchHandler;

public:

    static EventCatcher*	create()
				mCreateDataObj(EventCatcher);

    void			setEventType( int type );
    int				eventType() const { return type_; }

    CNotifier<EventCatcher, const EventInfo&>		eventhappened;
    CNotifier<EventCatcher, const EventInfo&>		nothandled;

    bool			isHandled() const;
    void			setHandled();
    void			reHandle(const EventInfo&);

    void			setUtm2Display(ObjectSet<Transformation>&);

protected:
    				~EventCatcher();

    int				type_;
    ObjectSet<Transformation>	utm2display_;

    static const char*		eventtypestr();

    bool			rehandling_;
    bool			rehandled_;

    virtual osg::Node*		gtOsgNode();

    osg::Node*			osgnode_;
    EventCatchHandler*		eventcatchhandler_;
};

}; // Namespace


#endif

