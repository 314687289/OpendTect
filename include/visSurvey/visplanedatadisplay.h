#ifndef visplanedatadisplay_h
#define visplanedatadisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id$
________________________________________________________________________

-*/

#include "vissurveymod.h"
#include "vismultiattribsurvobj.h"
#include "mousecursor.h"
#include "ranges.h"
#include "enums.h"
#include "factory.h"
#include "oduicommon.h"

template <class T> class Array2D;
namespace visBase
{
    class DepthTabPlaneDragger;
    class GridLines;
    class TextureRectangle;
}

class BinIDValueSet;
class RegularSeisDataPack;

namespace visSurvey
{

class Scene;

/*!\brief Used for displaying an inline, crossline or timeslice.

    A PlaneDataDisplay object is the front-end object for displaying an inline,
    crossline or timeslice.  Use setOrientation(Orientation) for setting the
    requested orientation of the slice.
*/

mExpClass(visSurvey) PlaneDataDisplay :
				public visSurvey::MultiTextureSurveyObject
{ mODTextTranslationClass(PlaneDataDisplay);
public:

    typedef OD::SliceType	SliceType;
				DeclareEnumUtils(SliceType);

				PlaneDataDisplay();

				mDefaultFactoryInstantiation(
				    visSurvey::SurveyObject,
				    PlaneDataDisplay, "PlaneDataDisplay",
				    toUiString(sFactoryKeyword()));

    bool			isInlCrl() const { return true; }

    void			setOrientation(SliceType);
    SliceType			getOrientation() const { return orientation_; }

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
    void			setResolution(int,TaskRunner*);

    SurveyObject::AttribFormat	getAttributeFormat(int attrib=-1) const;

    TrcKeyZSampling		getTrcKeyZSampling(int attrib=-1) const;
    TrcKeyZSampling		getTrcKeyZSampling(bool manippos,
						bool displayspace,
						int attrib=-1) const;
    void			getRandomPos(DataPointSet&,TaskRunner*) const;
    void			setRandomPosData(int attrib,
						 const DataPointSet*,
						 TaskRunner*);
    void			setTrcKeyZSampling(const TrcKeyZSampling&);

    bool			setDataPackID(int attrib,DataPack::ID,
					      TaskRunner*);
    DataPack::ID		getDataPackID(int attrib) const;
    DataPack::ID		getDisplayedDataPackID(int attrib) const;
    virtual DataPackMgr::ID	getDataPackMgrID() const
				{ return DataPackMgr::SeisID(); }

    visBase::GridLines*		gridlines()		{ return gridlines_; }

    const MouseCursor*		getMouseCursor() const { return &mousecursor_; }

    void			getMousePosInfo(const visBase::EventInfo& ei,
						IOPar& iop ) const
				{ return MultiTextureSurveyObject
						::getMousePosInfo(ei,iop);}
    void			getMousePosInfo(const visBase::EventInfo&,
						Coord3&,
						BufferString& val,
						BufferString& info) const;
    void			getObjectInfo(BufferString&) const;

    virtual float		calcDist(const Coord3&) const;
    virtual float		maxDist() const;
    virtual Coord3		getNormal(const Coord3&) const;
    virtual bool		allowsPicks() const		{ return true; }

    bool			setZAxisTransform(ZAxisTransform*,TaskRunner*);
    const ZAxisTransform*	getZAxisTransform() const;

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
    bool			isVerticalPlane() const;

    virtual bool		canDuplicate() const	{ return true; }
    virtual SurveyObject*	duplicate(TaskRunner*) const;

    virtual void		annotateNextUpdateStage(bool yn);

    static const char*		sKeyDepthKey()		{ return "DepthKey"; }
    static const char*		sKeyPlaneKey()		{ return "PlaneKey"; }

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    void			setDisplayTransformation(const mVisTrans*);
    const visBase::TextureRectangle* getTextureRectangle() const
				{ return texturerect_;  }
    float			getZScale() const;
    const mVisTrans*		getDisplayTransformation() const
				{ return displaytrans_; }

protected:
				~PlaneDataDisplay();

    BaseMapObject*		createBaseMapObject();
    void			setVolumeDataPackNoCache(int attrib,
						const RegularSeisDataPack*);
    void			setRandomPosDataNoCache(int attrib,
							const BinIDValueSet*,
							TaskRunner*);
    void			interpolArray(int,float*, int, int,
				    const Array2D<float>&,TaskRunner*) const;
    void			setDisplayDataPackIDs(int attrib,
					const TypeSet<DataPack::ID>& dpids);
    void			updateChannels(int attrib,TaskRunner*);
    void			createTransformedDataPack(int attrib);

    void			updateMainSwitch();
    void			setScene(Scene*);
    void			setSceneEventCatcher(visBase::EventCatcher*);
    void			updateRanges(bool resetpos=false);
    void			updateRanges(bool resetinlcrl=false,
					     bool resetz=false);
    void			manipChanged(CallBacker*);
    void			coltabChanged(CallBacker*);
    void			draggerMotion(CallBacker*);
    void			draggerFinish(CallBacker*);
    void			draggerRightClick(CallBacker*);
    void			setDraggerPos(const TrcKeyZSampling&);
    void			dataTransformCB(CallBacker*);
    void			updateMouseCursorCB(CallBacker*);

    bool			getCacheValue(int attrib,int version,
					      const Coord3&,float&) const;
    void			addCache();
    void			removeCache(int);
    void			swapCache(int,int);
    void			emptyCache(int);
    bool			hasCache(int) const;

    TrcKeyZSampling		snapPosition(const TrcKeyZSampling&) const;
    void			setUpdateStageTextureTransform();

    visBase::EventCatcher*		eventcatcher_;
    MouseCursor				mousecursor_;
    RefMan<visBase::DepthTabPlaneDragger> dragger_;

    visBase::GridLines*			gridlines_;
    SliceType				orientation_;

    TypeSet<DataPack::ID>		datapackids_;
    TypeSet<DataPack::ID>		transfdatapackids_;

    ObjectSet< TypeSet<DataPack::ID> >	displaycache_;
    ObjectSet<BinIDValueSet>		rposcache_;

    TrcKeyZSampling			csfromsession_;
    BinID				curicstep_;
    Notifier<PlaneDataDisplay>		moving_;
    Notifier<PlaneDataDisplay>		movefinished_;

    ZAxisTransform*			datatransform_;
    int					voiidx_;

    RefMan<const mVisTrans>		displaytrans_;
    RefMan<visBase::TextureRectangle>	texturerect_;

    struct UpdateStageInfo
    {
	bool		refreeze_;
	TrcKeyZSampling oldcs_;
	SliceType	oldorientation_;
	Coord		oldimagesize_;
    };
    UpdateStageInfo			updatestageinfo_;

    static const char*		sKeyOrientation() { return "Orientation"; }
    static const char*		sKeyResolution()  { return "Resolution"; }
    static const char*		sKeyGridLinesID() { return "GridLines ID"; }
};

} // namespace visSurvey

#endif

