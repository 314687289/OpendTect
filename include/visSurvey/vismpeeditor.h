#ifndef vismpeeditor_h
#define vismpeeditor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vismpeeditor.h,v 1.8 2005-10-19 15:47:53 cvskris Exp $
________________________________________________________________________


-*/

#include "visobject.h"

#include "emposid.h"
#include "geomelement.h"


namespace Geometry { class ElementEditor; };
namespace MPE { class ObjectEditor; };
namespace EM { class EdgeLineSet; }

namespace visBase
{

class DataObjectGroup;
class Marker;
class Dragger;
};


namespace visSurvey
{
class EdgeLineSetDisplay;

/*!\brief
*/


class MPEEditor : public visBase::VisualObjectImpl
{
public:
    static MPEEditor*	create()
			mCreateDataObj( MPEEditor );

    void		setEditor( Geometry::ElementEditor* );
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

    void			mouseClick( const EM::PosID&, bool shift,
					    bool alt, bool ctrl );
    				/*!<Notify the object that someone
				    has clicked on the object that's being
				    edited. Clicks on the editor's draggers
				    themselves are handled by clickCB. */

    bool			clickCB( CallBacker* );
    				/*!<Since the event should be handled
				    with this object before the main object,
				    the main object has to pass eventcatcher
				    calls here manually.
				    \returns wether the main object should
				    continue to process the event.
				*/
			
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

    Geometry::ElementEditor*	geeditor;
    MPE::ObjectEditor*		emeditor;

    visBase::Material*		nodematerial;
    visBase::Material*		activenodematerial;

    ObjectSet<visBase::Dragger>		draggers;
    ObjectSet<visBase::DataObjectGroup>	draggersshapesep;
    ObjectSet<visBase::Marker>		draggermarkers;
    TypeSet<EM::PosID>			posids;
    float				markersize;

    visBase::EventCatcher*	eventcatcher;
    visBase::Transformation*	transformation;
    EM::PosID			activedragger;

    bool			isdragging;

    EdgeLineSetDisplay*		interactionlinedisplay;
    void			setupInteractionLineDisplay();
    void			extendInteractionLine(const EM::PosID&);
};

};

#endif
