#ifndef visfaultsticksetdisplay_h
#define visfaultsticksetdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	J.C. Glas
 Date:		November 2008
 RCS:		$Id: visfaultsticksetdisplay.h,v 1.9 2010-05-28 07:44:34 cvsjaap Exp $
________________________________________________________________________


-*/

#include "vissurvobj.h"
#include "visobject.h"

#include "emposid.h"


namespace visBase 
{ 
    class Transformation;
    class IndexedPolyLine3D;
    class PickStyle;
}

namespace EM { class FaultStickSet; }
namespace MPE { class FaultStickSetEditor; }

namespace visSurvey
{
class MPEEditor;
class Seis2DDisplay;

/*!\brief Display class for FaultStickSets
*/

mClass FaultStickSetDisplay : public visBase::VisualObjectImpl,
			     public SurveyObject
{
public:
    static FaultStickSetDisplay* create()
				mCreateDataObj(FaultStickSetDisplay);

    MultiID			getMultiID() const;
    bool			isInlCrl() const	{ return false; }

    bool			hasColor() const		{ return true; }
    Color			getColor() const;
    void			setColor(Color);
    bool			allowMaterialEdit() const	{ return true; }
    NotifierAccess*		materialChange();

    void			showManipulator(bool);
    bool			isManipulatorShown() const;

    void			setDisplayTransformation(mVisTrans*);
    mVisTrans*			getDisplayTransformation();

    void			setSceneEventCatcher(visBase::EventCatcher*);

    bool			setEMID(const EM::ObjectID&);
    EM::ObjectID		getEMID() const;

    const char*			errMsg() const { return errmsg_.buf(); }

    void			updateSticks(bool activeonly=false);
    void			updateEditPids();
    void			updateKnotMarkers();

    Notifier<FaultStickSetDisplay> colorchange;

    void			removeSelection(const Selector<Coord3>&,
	    					TaskRunner*);

    void			setDisplayOnlyAtSections(bool yn);
    bool			displayedOnlyAtSections() const;

    void			setStickSelectMode(bool yn);
    bool			isInStickSelectMode() const;

    virtual void                fillPar(IOPar&,TypeSet<int>&) const;
    virtual int                 usePar(const IOPar&);

protected:

    virtual			~FaultStickSetDisplay();

    void   			otherObjectsMoved(
	    				const ObjectSet<const SurveyObject>&,
					int whichobj);

    void			setActiveStick(const EM::PosID&);

    static const char*		sKeyEarthModelID()	{ return "EM ID"; }

    void			mouseCB(CallBacker*);
    void			stickSelectCB(CallBacker*);
    void			emChangeCB(CallBacker*);
    void			polygonFinishedCB(CallBacker*);

    Coord3			disp2world(const Coord3& displaypos) const;

    visBase::EventCatcher*	eventcatcher_;
    visBase::Transformation*	displaytransform_;

    EM::FaultStickSet*		emfss_;
    MPE::FaultStickSetEditor*	fsseditor_;
    visSurvey::MPEEditor*	viseditor_;

    Coord3			mousepos_;
    bool			showmanipulator_;

    int				activesticknr_;

    TypeSet<EM::PosID>		editpids_;

    visBase::IndexedPolyLine3D* sticks_;
    visBase::IndexedPolyLine3D* activestick_;

    visBase::PickStyle*		stickspickstyle_;
    visBase::PickStyle*		activestickpickstyle_;

    bool			displayonlyatsections_;
    bool			stickselectmode_;

    bool			ctrldown_;

    ObjectSet<visBase::DataObjectGroup> knotmarkers_;
};

} // namespace VisSurvey


#endif
