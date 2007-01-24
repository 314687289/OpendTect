#ifndef mouseevent_h
#define mouseevent_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          September 2005
 RCS:           $Id: mouseevent.h,v 1.4 2007-01-24 14:27:11 cvskris Exp $
________________________________________________________________________

-*/

#include "keyenum.h"
#include "gendefs.h"
#include "geometry.h"

class MouseEvent
{
public:

 				MouseEvent(OD::ButtonState st=OD::NoButton,
				      int xx=0, int yy=0 );

    OD::ButtonState		buttonState() const;

    bool			leftButton() const;
    bool			rightButton() const;
    bool			middleButton() const;
    bool			ctrlStatus() const;
    bool			altStatus() const;
    bool			shiftStatus() const;

    const Geom::Point2D<int>&	pos() const;
    int				x() const;
    int				y() const;

    bool			operator ==( const MouseEvent& ev ) const;
    bool			operator !=( const MouseEvent& ev ) const;

protected:

    OD::ButtonState		butstate_;
    Geom::Point2D<int>		pos_;
};

/*!
Handles mouse events. An instance of the MouseEventHandler is provided by
the object that detects the mouse-click, e.g. a gui or visualization object.

Once the event callback is recieved, it MUST check if someone else have
handled the event and taken necessary actions. If it is not already handled,
and your class handles it, it should set the isHandled flag to prevent
other objects in the callback chain to handle the event. It is often a good
idea to be very specific what events your function should handle to avoid
interference with other objects that are in the callback chain. As an example,
see the code below: The if-statement will only let right-clicks when no other
mouse or keyboard button are pressed through.

\code

void MyClass::handleMouseClick( CallBacker* cb )
{
    if ( eventhandler_->isHandled() )
    	return;

    const MouseEvent& event = eventhandler_->event();
    if ( event.rightButton() && !event.leftButton() && !event.middleButton() &&
    	 !event.ctrlStatus() && !event.altStatus() && !event.shiftStatus() )
    {
        eventhandler_->setHandled( true );
    	//show and handle menu
    }
}

/endcode

*/

class MouseEventHandler : public CallBacker
{
public:
    				MouseEventHandler();
    void			triggerMovement(const MouseEvent&);
    void			triggerButtonPressed(const MouseEvent&);
    void			triggerButtonReleased(const MouseEvent&);
    void			triggerDoubleClick(const MouseEvent&);

    Notifier<MouseEventHandler>	buttonPressed;
    Notifier<MouseEventHandler>	buttonReleased;
    Notifier<MouseEventHandler>	movement;
    Notifier<MouseEventHandler>	doubleClick;

    const MouseEvent&		event() const		{ return *event_; }
    bool			isHandled() const	{ return ishandled_; }
    void			setHandled(bool yn)	{ ishandled_ = yn; }

protected:
    const MouseEvent*		event_;
    bool			ishandled_;
};


#endif
