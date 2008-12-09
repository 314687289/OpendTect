#ifndef vissurvscene_h
#define vissurvscene_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vissurvscene.h,v 1.54 2008-12-09 16:39:50 cvskris Exp $
________________________________________________________________________


-*/

#include "visscene.h"
#include "bufstring.h"
#include "position.h"

class HorSampling;
class CubeSampling;
class MouseCursor;
class ZAxisTransform;
class Color;
template <class T> class Selector;

namespace visBase
{
    class Annotation;
    class EventCatcher;
    class Marker;
    class PolygonSelection;
    class SceneColTab;
    class Transformation;
    class VisualObject;
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
    void			setAnnotText(int dim,const char*);

    visBase::PolygonSelection*	getPolySelection() { return polyselector_; }
    const Selector<Coord3>*	getSelector() const;	/*! May be NULL */
    visBase::SceneColTab*	getSceneColTab()     { return scenecoltab_; }

    Notifier<Scene>		mouseposchange;
    Coord3			getMousePos(bool xyt) const;
    				/*! If not xyt it is inlcrlt */
    BufferString		getMousePosValue() const;
    BufferString		getMousePosString() const;
    const MouseCursor*		getMouseCursor() const;

    void			objectMoved(CallBacker*);

    Notifier<Scene>		zscalechange;
    void			setZScale(float);
    				/*!<Used to set the z-strecthing according
				    to the user's preference. Is unitless -
				    all entities (i.e. distance vs time) 
				    should be handled by zfactor.
				*/
    float			getZScale() const;

    void			setZFactor(float);
    				/*!<The zfactor should compensate for different
				   entities in xy and z respectively and
				   remain constant through the life of the scene.
				*/
    float			getZFactor() const;

    mVisTrans*			getZScaleTransform() const;
    mVisTrans*			getInlCrl2DisplayTransform() const;
    mVisTrans*			getUTM2DisplayTransform() const;
    void			setDataTransform( ZAxisTransform* );
    ZAxisTransform*		getDataTransform();


    bool			isRightHandSystem() const;

    void			setZDomainKey( const char* key )
				    { zdomainkey_ = key; }
    const char*			getZDomainKey() const
				    { return zdomainkey_; }

    void			setMarkerPos( const Coord3& );
    void			setMarkerSize( float );
    float			getMarkerSize() const;
    const Color&		getMarkerColor() const;
    void			setMarkerColor( const Color& );

    void			allowShading( bool yn )	{ allowshading_ = yn; }
    bool			allowsShading() const	{ return allowshading_;}

    virtual void		fillPar(IOPar&,TypeSet<int>&) const;
    virtual int			usePar(const IOPar&);
    bool			acceptsIncompletePar() const { return true; }

protected:
    				~Scene();

    void			init();
    void			createTransforms(const HorSampling&);
    void			mouseMoveCB(CallBacker*);
    visBase::Marker*		createMarker() const;
    static const Color&		cDefaultMarkerColor();
    
    visBase::Transformation*	zscaletransform_;
    visBase::Transformation*	inlcrl2disptransform_;
    visBase::Transformation*	utm2disptransform_;
    ZAxisTransform*		datatransform_;

    visBase::Annotation*	annot_;
    visBase::Marker*		marker_;
    visBase::PolygonSelection*	polyselector_;
    Selector<Coord3>*		coordselector_;
    visBase::SceneColTab*	scenecoltab_;

    Coord3			xytmousepos_;
    BufferString		mouseposval_;
    BufferString		mouseposstr_;
    const MouseCursor*		mousecursor_;
    float			curzscale_;
    float			zfactor_;
    BufferString		zdomainkey_;
    bool			allowshading_;

    static const char*		sKeyShowAnnot();
    static const char*		sKeyShowScale();
    static const char*		sKeyShowCube();
    static const char*		sKeyZScale();
    static const char*		sKeyZDataTransform();
    static const char*		sKeyAllowShading();
};

}; // namespace visSurvey


#endif
