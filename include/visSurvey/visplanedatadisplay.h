#ifndef visplanedatadisplay_h
#define visplanedatadisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visplanedatadisplay.h,v 1.90 2007-01-17 10:33:06 cvshelene Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"

template <class T> class Array2D;

namespace visBase
{
    class Coordinates;
    class DepthTabPlaneDragger;
    class DrawStyle;
    class FaceSet;
    class GridLines;
    class MultiTexture2; 
    class PickStyle;
};


namespace Attrib { class SelSpec; class DataCubes; }

namespace visSurvey
{

class Scene;

/*!\brief Used for displaying an inline, crossline or timeslice.

    A PlaneDataDisplay object is the front-end object for displaying an inline,
    crossline or timeslice.  Use <code>setOrientation(Orientation)</code> for
    setting the requested orientation of the slice.
*/

class PlaneDataDisplay :  public visBase::VisualObjectImpl,
			  public visSurvey::SurveyObject
{
public:

    bool			isInlCrl() const	{ return true; }
    bool			isOn() const	 	{ return onoffstatus_; }
    void			turnOn(bool);

    enum Orientation		{ Inline=0, Crossline=1, Timeslice=2 };
    				DeclareEnumUtils(Orientation);

    static PlaneDataDisplay*	create()
				mCreateDataObj(PlaneDataDisplay);

    void			setOrientation(Orientation);
    Orientation			getOrientation() const { return orientation_; }

    void			showManipulator(bool);
    bool			isManipulatorShown() const;
    bool			isManipulated() const;
    bool			canResetManipulation() const	{ return true; }
    void			resetManipulation();
    void			acceptManipulation();
    BufferString		getManipulationString() const;
    NotifierAccess*		getManipulationNotifier();
    NotifierAccess*		getMovementNotification()
    				{ return &movefinished_; }

    bool			allowMaterialEdit() const	{ return true; }

    int				nrResolutions() const;
    int				getResolution() const;
    void			setResolution(int);

    SurveyObject::AttribFormat	getAttributeFormat() const;
    bool			canHaveMultipleAttribs() const;
    int				nrAttribs() const;
    bool			addAttrib();
    bool			removeAttrib(int attrib);
    bool			swapAttribs(int attrib0,int attrib1);
    void			setAttribTransparency(int,unsigned char);
    unsigned char		getAttribTransparency(int) const;

    bool			hasColorAttribute() const	{ return false;}
    const Attrib::SelSpec*	getSelSpec(int) const;
    void			setSelSpec(int,const Attrib::SelSpec&);
    bool 			isClassification(int attrib) const;
    void			setClassification(int attrib,bool yn);
    bool 			isAngle(int attrib) const;
    void			setAngleFlag(int attrib,bool yn);
    void			enableAttrib(int attrib,bool yn);
    bool			isAttribEnabled(int attrib) const;
    const TypeSet<float>*	getHistogram(int) const;
    int				getColTabID(int) const;

    CubeSampling		getCubeSampling(int attrib=-1) const;
    CubeSampling		getCubeSampling(bool manippos,
	    					bool displayspace,
						int attrib=-1) const;
    void			getRandomPos( ObjectSet<BinIDValueSet>& ) const;
    void			setRandomPosData( int attrib,
					    const ObjectSet<BinIDValueSet>*);
    void			setCubeSampling(CubeSampling);
    bool			setDataVolume(int attrib,
	    				      const Attrib::DataCubes*);
    const Attrib::DataCubes*	getCacheVolume(int attrib) const;
   
    bool			canHaveMultipleTextures() const { return true; }
    int				nrTextures(int attrib) const;
    void			selectTexture(int attrib, int texture );
    int				selectedTexture(int attrib) const;

    visBase::GridLines*		gridlines()		{ return gridlines_; }

    void			getMousePosInfo(const visBase::EventInfo&,
	    					const Coord3&,
	    					BufferString& val,
	    					BufferString& info) const;

    virtual void		fillPar(IOPar&, TypeSet<int>&) const;
    virtual int			usePar(const IOPar&);

    virtual float		calcDist(const Coord3&) const;
    virtual float		maxDist() const;
    virtual bool		allowPicks() const		{ return true; }

    bool			setDataTransform(ZAxisTransform*);
    const ZAxisTransform*	getDataTransform() const;

    void			setTranslationDragKeys(bool depth, int );
    				/*!<\param depth specifies wheter the depth or
						 the plane setting should be
						 changed.
				    \param keys   combination of OD::ButtonState
				    \note only shift/ctrl/alt are used. */

    int				getTranslationDragKeys(bool depth) const;
    				/*!<\param depth specifies wheter the depth or
						 the plane setting should be
						 returned.
				    \returns	combination of OD::ButtonState*/
    bool                	canBDispOn2DViewer() const;


protected:
				~PlaneDataDisplay();
    void			updateMainSwitch();

    void			setData(int attrib,const Attrib::DataCubes*);
    void			setData(int attrib,
	    				const ObjectSet<BinIDValueSet>*);
    void			updateRanges(bool resetpos=false);
    void			updateRanges(bool resetinlcrl=false,
	    				     bool resetz=false);
    void			manipChanged(CallBacker*);
    void			coltabChanged(CallBacker*);
    void			draggerMotion(CallBacker*);
    void			draggerFinish(CallBacker*);
    void			draggerRightClick(CallBacker*);
    void			setDraggerPos(const CubeSampling&);
    void			dataTransformCB(CallBacker*);
    void			setTextureCoords(int sz0,int sz1);

    CubeSampling		snapPosition(const CubeSampling&) const;

    visBase::DepthTabPlaneDragger*	dragger_;
    visBase::Material*			draggermaterial_;
    visBase::PickStyle*			rectanglepickstyle_;
    visBase::MultiTexture2*		texture_;
    visBase::FaceSet*			rectangle_;
    visBase::GridLines*			gridlines_;
    Orientation				orientation_;
    visBase::FaceSet*			draggerrect_;
    visBase::DrawStyle*			draggerdrawstyle_;


    ObjectSet<Attrib::SelSpec>		as_;
    BoolTypeSet				isclassification_;
    ObjectSet<const Attrib::DataCubes>	volumecache_;
    ObjectSet<BinIDValueSet>		rposcache_;

    int				resolution_;
    BinID			curicstep_;
    float			curzstep_;
    ZAxisTransform*		datatransform_;
    int				datatransformvoihandle_;
    Notifier<PlaneDataDisplay>	moving_;
    Notifier<PlaneDataDisplay>	movefinished_;
    bool			onoffstatus_;

    static const char*		sKeyOrientation() { return "Orientation"; }
    static const char*		sKeyTextureRect() { return "Texture rectangle";}
    static const char*		sKeyResolution()  { return "Resolution";}
    static const char*		sKeyDepthKey() { return "DepthKey"; }
    static const char*		sKeyPlaneKey() { return "PlaneKey"; }
};

};


#endif
