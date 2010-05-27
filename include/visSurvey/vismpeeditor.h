#ifndef vismpeeditor_h
#define vismpeeditor_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vismpeeditor.h,v 1.17 2010-05-27 14:27:20 cvsjaap Exp $
________________________________________________________________________


-*/


#include "emposid.h"
#include "keyenum.h"
#include "visobject.h"


class Color;
class Coord;

namespace MPE { class ObjectEditor; };
namespace EM { class EdgeLineSet; }

namespace visBase
{

class DataObjectGroup;
class Marker;
class Dragger;
class EventInfo;
class PolyLine;
};


namespace visSurvey
{
class EdgeLineSetDisplay;
class MPEEditor;

/*!\brief
*/


mClass Sower : public visBase::VisualObjectImpl
{
    friend class	MPEEditor;

public:
    enum		SowingMode { Idle=0, Furrowing, Sowing };
    SowingMode		mode()				{ return mode_; }

    void		reverseSowingOrder(bool yn=true);
    void		alternateSowingOrder(bool yn=true);
    Coord3		pivotPos() const;

    bool		accept(const visBase::EventInfo&,
			       OD::ButtonState mask=OD::LeftButton);
    bool		activate(const Color&,const visBase::EventInfo&);

protected:
			Sower();
			~Sower();

    void		setDisplayTransformation( mVisTrans* );
    void		setEventCatcher( visBase::EventCatcher* );

    visBase::EventCatcher*		eventcatcher_;
    visBase::PolyLine*			sowingline_;
    bool				linelost_;
    SowingMode				mode_;
    ObjectSet<visBase::EventInfo>	eventlist_;
    TypeSet<Coord>			mousecoords_;

    bool				reversesowingorder_;
    bool				alternatesowingorder_;
};


mClass MPEEditor : public visBase::VisualObjectImpl
{
public:
    static MPEEditor*	create()
			mCreateDataObj( MPEEditor );

    void		setEditor( MPE::ObjectEditor* );
    MPE::ObjectEditor*	getMPEEditor() { return emeditor; }
    void		setSceneEventCatcher( visBase::EventCatcher* );

    void		setDisplayTransformation( mVisTrans* );
    mVisTrans*		getDisplayTransformation() {return transformation;}

    void		setMarkerSize(float);

    Notifier<MPEEditor>		noderightclick;
    				/*!<\ the clicked position can be retrieved
				      with getNodePosID(getRightClickNode) */
    int				getRightClickNode() const;
    EM::PosID			getNodePosID(int idx) const;

    Notifier<MPEEditor>		interactionlinerightclick;
    int				interactionLineID() const;
				/*!<\returns the visual id of the line, or -1
				     if it does not exist. */

    bool			mouseClick( const EM::PosID&, bool shift,
					    bool alt, bool ctrl );
    				/*!<Notify the object that someone
				    has clicked on the object that's being
				    edited. Clicks on the editor's draggers
				    themselves are handled by clickCB.
				    Returns true when click is handled. */

    bool			clickCB( CallBacker* );
    				/*!<Since the event should be handled
				    with this object before the main object,
				    the main object has to pass eventcatcher
				    calls here manually.
				    \returns wether the main object should
				    continue to process the event.
				*/
    EM::PosID			mouseClickDragger(const TypeSet<int>&) const;

    bool			isDragging() const	{ return isdragging; }

    Sower&			sower()			{ return *sower_; }

			
protected:
    				~MPEEditor();
    void			changeNumNodes( CallBacker* );
    void			nodeMovement( CallBacker* );
    void			dragStart( CallBacker* );
    void			dragMotion( CallBacker* );
    void			dragStop( CallBacker* );
    void			updateNodePos( int, const Coord3& );
    void			removeDragger( int );
    void			addDragger( const EM::PosID& );
    void			setActiveDragger( const EM::PosID& );

    void			interactionLineRightClickCB( CallBacker* );

    int				rightclicknode;

    MPE::ObjectEditor*		emeditor;

    visBase::Material*		nodematerial;
    visBase::Material*		activenodematerial;

    ObjectSet<visBase::Dragger>		draggers;
    ObjectSet<visBase::DataObjectGroup>	draggersshapesep;
    ObjectSet<visBase::Marker>		draggermarkers;
    visBase::DataObjectGroup*		dummyemptysep_;
    TypeSet<EM::PosID>			posids;
    float				markersize;

    visBase::EventCatcher*	eventcatcher;
    visBase::Transformation*	transformation;
    EM::PosID			activedragger;

    bool			draggerinmotion;
    bool			isdragging;

    EdgeLineSetDisplay*		interactionlinedisplay;
    void			setupInteractionLineDisplay();
    void			extendInteractionLine(const EM::PosID&);

    Sower*			sower_;
};



};

#endif
