#ifndef vishingline_h
#define vishingline_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          4-11-2002
 RCS:           $Id: vishingeline.h,v 1.7 2004-11-16 09:30:23 kristofer Exp $
________________________________________________________________________

          
-*/

#include "visobject.h"

#include "emposid.h"

class RowCol;

namespace visBase { class IndexedPolyLine3D; class Transformation; };
namespace EM { class EdgeLineSet; };

namespace visSurvey
{

class EdgeLineSetDisplay : public visBase::VisualObjectImpl
{
public:
    static EdgeLineSetDisplay*	create()
				mCreateDataObj(EdgeLineSetDisplay);

    void			setRadius(float);
    float			getRadius() const;

    void			setConnect(bool);
    				/*!<Sets wether the first and last node in a 
				    line should be connected, given that they
				    are neighbors */
    bool			getConnect() const;
    				/*!<\returns true if the first and last node
				     in a line is connected, given that they
				     are neighbors */
    void			setShowDefault(bool);
    				/*!<Sets wether default edgesegments should be
				    shown */
    bool			getShowDefault() const;
    				/*!<\returns wether default edgesegments
				  should be shown */

    void			setEdgeLineSet(const EM::EdgeLineSet*);
    bool			setEdgeLineSet(int);
    const EM::EdgeLineSet*	getEdgeLineSet() const { return edgelineset; }
    void			setDisplayTransformation(
	    					visBase::Transformation*);
    visBase::Transformation*	getDisplayTransformation();


    int				getRightClickedLine() const
    				{ return rightclidedline; }
    int				getRightClickedSegment() const
    				{ return rightclidedsegment; }
    int				getRightClickedSegmentPos() const
    				{ return rightclidedsegmentpos; }

protected:
    				~EdgeLineSetDisplay();
    void			updateEdgeLineSetChangeCB(CallBacker*);
    void			triggerRightClick(const visBase::EventInfo*);
    const EM::EdgeLineSet*	edgelineset;
    ObjectSet<visBase::IndexedPolyLine3D>	polylines;
    ObjectSet<TypeSet<int> >			polylinesegments;
    ObjectSet<TypeSet<int> >			polylinesegmentpos;
    visBase::Transformation*	transformation;
    bool			connect;
    bool			showdefault;

    int				rightclidedline;
    int				rightclidedsegment;
    int				rightclidedsegmentpos;
};


}; // namespace visSurvey

#endif
