#ifndef vissurvscene_h
#define vissurvscene_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vissurvscene.h,v 1.37 2006-02-23 17:34:48 cvskris Exp $
________________________________________________________________________


-*/

#include "visscene.h"
#include "position.h"

class HorSampling;
class CubeSampling;
class ZAxisTransform;
class Color;

namespace visBase
{
    class Annotation;
    class EventCatcher;
    class Transformation;
    class VisualObject;
    class Marker;
};

namespace visSurvey
{

/*!\brief Database for 3D objects

<code>VisSurvey::Scene</code> is the database for all 'xxxxDisplay' objects.
Use <code>addObject(visBase::SceneObject*)</code> to add an object to the Scene.

It also manages the size of the survey cube. The ranges in each direction are
obtained from <code>SurveyInfo</code> class.<br>
The display coordinate system is given in [m/m/ms] if the survey's depth is
given in time. If the survey's depth is given in meters, the display coordinate
system is given as [m/m/m]. The display coordinate system is _righthand_
oriented!<br>

OpenInventor(OI) has difficulties handling real-world coordinates (like xy UTM).
Therefore the coordinates given to OI must be transformed from the UTM system
to the display coordinate system. This is done by the display transform, which
is given to all objects in the UTM system. These object are responsible to
transform their coords themselves before giving them to OI.<br>

The visSurvey::Scene has two domains:<br>
1) the UTM coordinate system. It is advised that most objects are here.
The objects added to this domain will have their transforms set to the
displaytransform which transforms their coords from UTM lefthand
(x, y, time[s] or depth[m] ) to display coords (righthand).<br>

2) the InlCrl domain. Here, OI takes care of the transformation between
inl/crl/t to display coords, so the objects does not need any own transform.

*/

class Scene : public visBase::Scene
{
public:
    static Scene*		create()
				mCreateDataObj(Scene);

    virtual void		removeAll();
    virtual void		addObject(visBase::DataObject*);
    				/*!< If the object is a visSurvey::SurveyObject
				     it will ask if it's an inlcrl-object or
				     not. If it's not an
				     visSurvey::SurveyObject, it will be put in
				     displaydomain
				*/
    void			addUTMObject(visBase::VisualObject*);
    void			addInlCrlTObject(visBase::DataObject*);
    virtual void		removeObject(int idx);

    void			setAnnotationCube(const CubeSampling&);
    void			showAnnotText(bool);
    bool			isAnnotTextShown() const;
    void			showAnnotScale(bool);
    bool			isAnnotScaleShown() const;
    void			showAnnot(bool);
    bool			isAnnotShown() const;

    Notifier<Scene>		mouseposchange;
    Coord3			getMousePos(bool xyt) const;
    				/*! If not xyt it is inlcrlt */
    float			getMousePosValue() const;
    BufferString		getMousePosString() const;

    void			objectMoved(CallBacker*);

    Notifier<Scene>		zscalechange;
    void			setZScale(float);
    float			getZScale() const;

    mVisTrans*			getZScaleTransform() const;
    mVisTrans*			getInlCrl2DisplayTransform() const;
    mVisTrans*			getUTM2DisplayTransform() const;
    void			setDataTransform( ZAxisTransform* );
    ZAxisTransform*		getDataTransform();

    void			setMarkerPos( const Coord3& );
    void			setMarkerSize( float );
    float			getMarkerSize() const;
    const Color&		getMarkerColor() const;
    void			setMarkerColor( const Color& );

    virtual void		fillPar(IOPar&,TypeSet<int>&) const;
    virtual int			usePar(const IOPar&);

protected:
    				~Scene();

    void			init();
    void			createTransforms(const HorSampling&);
    void			mouseMoveCB(CallBacker*);
    
    visBase::Transformation*	zscaletransform_;
    visBase::Transformation*	inlcrl2disptransform_;
    visBase::Transformation*	utm2disptransform_;
    ZAxisTransform*		datatransform_;

    visBase::Annotation*	annot_;
    visBase::Marker*		marker_;

    Coord3			xytmousepos_;
    float			mouseposval_;
    BufferString		mouseposstr_;
    float			curzscale_;

    static const char*		sKeyShowAnnot();
    static const char*		sKeyShowScale();
    static const char*		sKeyShoCube();
};

}; // namespace visSurvey


#endif
