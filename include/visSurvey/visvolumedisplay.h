#ifndef visvolumedisplay_h
#define visvolumedisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		August 2002
 RCS:		$Id: visvolumedisplay.h,v 1.70 2009-09-04 01:35:35 cvskris Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "mousecursor.h"
#include "vissurvobj.h"
#include "ranges.h"

class CubeSampling;
class MarchingCubesSurface;
class ZAxisTransform;
class TaskRunner;
class ZAxisTransformer;
template <class T> class Array3D;

namespace Attrib { class SelSpec; class DataCubes; }

namespace visBase
{
    class MarchingCubesSurface;
    class Material;
    class BoxDragger;
    class VolumeRenderScalarField;
    class VolrenDisplay;
    class OrthogonalSlice;
}


namespace visSurvey
{

class Scene;

mClass VolumeDisplay : public visBase::VisualObjectImpl,
		      public SurveyObject
{
public:
    static VolumeDisplay*	create()
				mCreateDataObj(VolumeDisplay);
    bool			isInlCrl() const	{ return true; }

    static int			cInLine() 		{ return 2; }
    static int			cCrossLine() 		{ return 1; }
    static int			cTimeSlice() 		{ return 0; }

    int				addSlice(int dim);
    				/*!\note return with removeChild(displayid). */
    void			showVolRen(bool yn);
    bool			isVolRenShown() const;
    int				volRenID() const;

    int				addIsoSurface(TaskRunner* = 0,
	    				      bool updateisosurface = true);
    				/*!\note return with removeChild(displayid). */
    void			removeChild(int displayid);
    
    visBase::MarchingCubesSurface* getIsoSurface(int idx);
    void			updateIsoSurface(int,TaskRunner* = 0);
    const int			getNrIsoSurfaces();
    int				getIsoSurfaceIdx(
	    			    const visBase::MarchingCubesSurface*) const;
    float			defaultIsoValue() const;
    float			isoValue(
				    const visBase::MarchingCubesSurface*) const;
    				/*<Set isovalue and do update. */
    void			setIsoValue(
				    const visBase::MarchingCubesSurface*,
				    float,TaskRunner* =0);

    				/*<Seed based settings. set only, no update. */
    char			isFullMode(
				    const visBase::MarchingCubesSurface*)const;
    				/*<Return -1 if undefined, 1 if full, 
				   0 if seed based. */
    void			setFullMode(
	    			    const visBase::MarchingCubesSurface*,
				    bool full=1);
    				/*<If 0, it is seed based. */
    char			seedAboveIsovalue(
				    const visBase::MarchingCubesSurface*) const;
    				/*<-1 undefined, 1 above, 0 below. */
    void			setSeedAboveIsovalue(
				    const visBase::MarchingCubesSurface*,bool);
    MultiID			getSeedsID(
	    			    const visBase::MarchingCubesSurface*) const;
    void			setSeedsID(const visBase::MarchingCubesSurface*,					   MultiID);

    void			showManipulator(bool yn);
    bool			isManipulatorShown() const;
    bool			isManipulated() const;
    bool			canResetManipulation() const;
    void			resetManipulation();
    void			acceptManipulation();
    NotifierAccess*		getMovementNotifier() { return &slicemoving; }
    NotifierAccess*		getManipulationNotifier() {return &slicemoving;}
    BufferString		getManipulationString() const;

    SurveyObject::AttribFormat	getAttributeFormat(int attrib) const;
    const Attrib::SelSpec*	getSelSpec(int attrib) const;
    const TypeSet<float>* 	getHistogram(int attrib) const;
    void			setSelSpec(int attrib,const Attrib::SelSpec&);

    float			slicePosition(visBase::OrthogonalSlice*) const;
    float			getValue(const Coord3&) const;

    CubeSampling		getCubeSampling(int attrib) const;
    void			setCubeSampling(const CubeSampling&);
    bool			setDataVolume(int attrib,
	    				      const Attrib::DataCubes*,
					      TaskRunner*);
    const Attrib::DataCubes*	getCacheVolume(int attrib) const;
    bool			setDataPackID(int attrib,DataPack::ID,
	    				      TaskRunner*);
    DataPack::ID		getDataPackID(int attrib) const;
    virtual DataPackMgr::ID     getDataPackMgrID() const
	                                { return DataPackMgr::CubeID(); }
    void			getMousePosInfo(const visBase::EventInfo&,
	    			     		Coord3&,BufferString& val,
	    					BufferString& info) const;

    const ColTab::MapperSetup*	getColTabMapperSetup(int) const;
    void			setColTabMapperSetup(int,
					const ColTab::MapperSetup&,TaskRunner*);
    const ColTab::Sequence*	getColTabSequence(int) const;
    void			setColTabSequence(int,const ColTab::Sequence&,
	    				TaskRunner*);
    bool			canSetColTabSequence() const;

    void			setMaterial(visBase::Material*);
    bool			allowMaterialEdit() const	{ return true; }
    virtual bool		allowPicks() const;
    bool			canDuplicate() const		{ return true; }
    visSurvey::SurveyObject*	duplicate(TaskRunner*) const;

    void			allowShading(bool yn ) { allowshading_ = yn; }

    SoNode*			getInventorNode();

    Notifier<VolumeDisplay>	slicemoving;

    void			getChildren(TypeSet<int>&) const;

    bool			setDataTransform(ZAxisTransform*,TaskRunner*);
    const ZAxisTransform*	getDataTransform() const;

    void			setRightHandSystem(bool yn);

    virtual void		fillPar(IOPar&,TypeSet<int>&) const;
    virtual int			usePar(const IOPar&);

protected:
				~VolumeDisplay();
    bool			updateSeedBasedSurface(int,TaskRunner* = 0);
    CubeSampling		getCubeSampling(bool manippos,bool display,
	    					int attrib) const;
    void			materialChange(CallBacker*);
    void			updateIsoSurfColor();
    bool			pickable() const { return true; }
    bool			rightClickable() const { return true; }
    bool			selectable() const { return false; }
    				//!<Makes impossible to click on it and
				//!<by mistake get the drag-box up
    bool			isSelected() const;
    const MouseCursor*		getMouseCursor() const { return &mousecursor_; }

    visBase::Transformation*			voltrans_;
    visBase::BoxDragger*			boxdragger_;
    visBase::VolumeRenderScalarField*		scalarfield_;
    visBase::VolrenDisplay*			volren_;
    ObjectSet<visBase::OrthogonalSlice>		slices_;
    ObjectSet<visBase::MarchingCubesSurface>	isosurfaces_;
    struct IsosurfaceSetting
    {
				IsosurfaceSetting();
	bool			operator==(const IsosurfaceSetting&) const;
	IsosurfaceSetting& 	operator=(const IsosurfaceSetting&);

	float			isovalue_;
	char			mode_;
	char			seedsaboveisoval_;
	MultiID			seedsid_;
    };

    TypeSet<IsosurfaceSetting>	isosurfsettings_;
    TypeSet<char>		sections_;

    void			manipMotionFinishCB(CallBacker*);
    void			sliceMoving(CallBacker*);
    void			setData(const Attrib::DataCubes*,
	    				int datatype=0);

    void			dataTransformCB(CallBacker*);
    void			updateRanges(bool updateic,bool updatez);
    void			updateMouseCursorCB(CallBacker*);
    void			setSceneEventCatcher(visBase::EventCatcher*);

    void			triggerSel() { updateMouseCursorCB( 0 ); }
    void			triggerDeSel() { updateMouseCursorCB( 0 ); }


    ZAxisTransform*		datatransform_;
    ZAxisTransformer*		datatransformer_;

    DataPack::ID		cacheid_;
    const Attrib::DataCubes*	cache_;
    Attrib::SelSpec&		as_;
    bool			allowshading_;
    BufferString		sliceposition_;
    BufferString		slicename_;
    CubeSampling		csfromsession_;

    MouseCursor			mousecursor_;
    visBase::EventCatcher*	eventcatcher_;

    bool			isinited_;

    static const char*		sKeyVolumeID();
    static const char*		sKeyInline();
    static const char*		sKeyCrossLine();
    static const char*		sKeyTime();
    static const char*		sKeyVolRen();
    static const char*		sKeyNrSlices();
    static const char*		sKeySlice();
    static const char*		sKeyTexture();

    static const char*		sKeyNrIsoSurfaces();
    static const char*		sKeyIsoValueStart();
    static const char*		sKeyIsoOnStart();
    static const char*		sKeySurfMode();
    static const char*		sKeySeedsMid();
    static const char*		sKeySeedsAboveIsov();

};

} // namespace visSurvey


#endif
