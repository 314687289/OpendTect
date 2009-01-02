#ifndef visplanedatadisplay_h
#define visplanedatadisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visplanedatadisplay.h,v 1.110 2009-01-02 11:31:50 cvsranojay Exp $
________________________________________________________________________


-*/


#include "vismultiattribsurvobj.h"
#include "mousecursor.h"
#include "ranges.h"
#include "enums.h"

template <class T> class Array2DImpl;
namespace visBase
{
    class Coordinates;
    class DepthTabPlaneDragger;
    class DrawStyle;
    class FaceSet;
    class GridLines;
    class PickStyle;
    class SplitTexture2Rectangle;
};

class BinIDValueSet;
class FlatDataPack;
namespace Attrib { class Flat3DDataPack; }

namespace visSurvey
{

class Scene;

/*!\brief Used for displaying an inline, crossline or timeslice.

    A PlaneDataDisplay object is the front-end object for displaying an inline,
    crossline or timeslice.  Use <code>setOrientation(Orientation)</code> for
    setting the requested orientation of the slice.
*/

class PlaneDataDisplay :  public visSurvey::MultiTextureSurveyObject
{
public:

    bool			isInlCrl() const	{ return true; }

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
    NotifierAccess*		getMovementNotifier()
    				{ return &movefinished_; }

    bool			allowMaterialEdit() const	{ return true; }

    int				nrResolutions() const;
    void			setResolution(int);

    SurveyObject::AttribFormat	getAttributeFormat() const;

    CubeSampling		getCubeSampling(int attrib=-1) const;
    CubeSampling		getCubeSampling(bool manippos,
	    					bool displayspace,
						int attrib=-1) const;
    void			getRandomPos(DataPointSet&) const;
    void			setRandomPosData(int attrib,
	    					 const DataPointSet*);
    void			setCubeSampling(CubeSampling);

    bool			setDataPackID(int attrib,DataPack::ID);
    DataPack::ID		getDataPackID(int attrib) const;
    virtual DataPackMgr::ID	getDataPackMgrID() const
    				{ return DataPackMgr::FlatID(); }
    const Attrib::DataCubes*	getCacheVolume(int attrib) const;
   
    visBase::GridLines*		gridlines()		{ return gridlines_; }

    const MouseCursor*		getMouseCursor() const { return &mousecursor_; }
    void			getMousePosInfo(const visBase::EventInfo&,
	    					const Coord3&,
	    					BufferString& val,
	    					BufferString& info) const;
    void			getObjectInfo(BufferString&) const;

    virtual void		fillPar(IOPar&, TypeSet<int>&) const;
    virtual int			usePar(const IOPar&);

    virtual float		calcDist(const Coord3&) const;
    virtual float		maxDist() const;
    virtual Coord3		getNormal(const Coord3&) const;
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
    bool                	isVerticalPlane() const;

    virtual bool		canDuplicate() const	{ return true; }
    virtual SurveyObject*	duplicate() const;
   
    const TypeSet<DataPack::ID>* getDisplayDataPackIDs(int attrib);

    static const char*		sKeyDepthKey()		{ return "DepthKey"; }
    static const char*		sKeyPlaneKey()		{ return "PlaneKey"; }

protected:

				~PlaneDataDisplay();
    void			setVolumeDataPackNoCache(int attrib,
	    				const Attrib::Flat3DDataPack*);
    void			setRandomPosDataNoCache(int attrib,
	    						const BinIDValueSet*);
    void			setDisplayDataPackIDs(int attrib,
	    				const TypeSet<DataPack::ID>& dpids);
    void			updateFromDisplayIDs(int attrib);

    void			updateMainSwitch();
    void			setSceneEventCatcher(visBase::EventCatcher*);
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
    void			updateMouseCursorCB(CallBacker*);
    
    bool			getCacheValue(int attrib,int version,
					      const Coord3&,float&) const;
    void			addCache();
    void			removeCache(int);
    void			swapCache(int,int);
    void			emptyCache(int);
    bool			hasCache(int) const;
    void			triggerSel();
    void			triggerDeSel();

    CubeSampling		snapPosition(const CubeSampling&) const;

    visBase::EventCatcher*		eventcatcher_;
    MouseCursor				mousecursor_;
    visBase::DepthTabPlaneDragger*	dragger_;
    visBase::Material*			draggermaterial_;
    visBase::PickStyle*			rectanglepickstyle_;
    visBase::SplitTexture2Rectangle*	rectangle_;

    visBase::GridLines*			gridlines_;
    Orientation				orientation_;
    visBase::FaceSet*			draggerrect_;
    visBase::DrawStyle*			draggerdrawstyle_;
    
    ObjectSet< TypeSet<DataPack::ID> >	displaycache_;
    ObjectSet<BinIDValueSet>		rposcache_;
    ObjectSet<const Attrib::Flat3DDataPack> volumecache_;

    CubeSampling			csfromsession_;
    BinID				curicstep_;
    float				curzstep_;
    ZAxisTransform*			datatransform_;
    Notifier<PlaneDataDisplay>		moving_;
    Notifier<PlaneDataDisplay>		movefinished_;

    static const char*		sKeyOrientation() { return "Orientation"; }
    static const char*		sKeyTextureRect() { return "Texture rectangle";}
    static const char*		sKeyResolution()  { return "Resolution"; }
    static const char*		sKeyGridLinesID() { return "GridLines ID"; }
};

} // namespace visSurvey


#endif
