#ifndef vismpeeditor_h
#define vismpeeditor_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/


#include "vissurveymod.h"
#include "emposid.h"
#include "visobject.h"
#include "vissower.h"


namespace MPE { class ObjectEditor; };
namespace EM { class EdgeLineSet; }

namespace visBase
{

class DataObjectGroup;
class MarkerSet;
class Dragger;
class EventInfo;
class PolyLine;
};


namespace visSurvey
{
class EdgeLineSetDisplay;
class Sower;

/*!\brief
*/


mExpClass(visSurvey) MPEEditor : public visBase::VisualObjectImpl
{
public:
    static MPEEditor*	create()
			mCreateDataObj( MPEEditor );

    void		setEditor( MPE::ObjectEditor* );
    MPE::ObjectEditor*	getMPEEditor() { return emeditor_; }
    void		setSceneEventCatcher( visBase::EventCatcher* );

    void		setDisplayTransformation( const mVisTrans* );
    const mVisTrans*	getDisplayTransformation() const{return transformation_;}

    void		setMarkerSize(float);
    void		turnOnMarker(EM::PosID,bool on);
    bool		allMarkersDisplayed() const;

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

    bool			isDragging() const	{ return isdragging_; }

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

    int				rightclicknode_;

    MPE::ObjectEditor*		emeditor_;

    visBase::Material*		nodematerial_;
    visBase::Material*		activenodematerial_;

    ObjectSet<visBase::Dragger>		draggers_;
    ObjectSet<visBase::MarkerSet>       draggermarkers_;
    TypeSet<EM::PosID>			posids_;
    float				markersize_;

    visBase::EventCatcher*	eventcatcher_;
    const mVisTrans*		transformation_;
    EM::PosID			activedragger_;

    bool			draggerinmotion_;
    bool			isdragging_;

    EdgeLineSetDisplay*		interactionlinedisplay_;
    void			setupInteractionLineDisplay();
    void			extendInteractionLine(const EM::PosID&);

    Sower*			sower_;
};



};

#endif

