#ifndef vislocationdisplay_h
#define vislocationdisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		June 2006
 RCS:		$Id: vislocationdisplay.h,v 1.17 2008-12-11 16:13:58 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "vissurvobj.h"

class Sphere;
class SoSeparator;
namespace Pick { class Set; class Location; class SetMgr; }
template <class T> class Selector;

namespace visBase
{
    class DataObjectGroup;
    class DrawStyle;
    class EventCatcher;
    class PickStyle;
    class PolyLine;
    class Transformation;
    class TriangleStripSet;
};


namespace visSurvey
{

class Scene;

/*!\brief Used for displaying picksets of varying types.
  The class is not intended for standalone usage, but is a common ground for
  picksets and other classes, where the inheriting classes knows about display
  shapes ++.
*/

class LocationDisplay :	public visBase::VisualObjectImpl,
			public visSurvey::SurveyObject
{
public:
    virtual void		setSet(Pick::Set*); // once!
    void			setSetMgr(Pick::SetMgr*);
    				/*!<Only used for notifications. */
    Pick::Set*			getSet()		{ return set_; }
    const Pick::Set*		getSet() const		{ return set_; }

    MultiID			getMultiID() const 	{ return storedmid_; }

    void			fullRedraw(CallBacker* =0);
    void			showAll(bool yn);
    bool			allShown() const	{ return showall_; }
    
    void			displayLocationBody(bool);
    bool			isLocationBodyDisplayed() const;
    bool			setLocationBodyDisplay();
    visBase::TriangleStripSet*	getLocationBody() const { return bodydisplay_; }

    void                        createLine();
    void                        showLine(bool);
    bool                        lineShown() const;
    virtual BufferString	getManipulationString() const;
    virtual void		getMousePosInfo(const visBase::EventInfo&,
						const Coord3&,BufferString&,
						BufferString&) const;
    virtual bool		hasColor() const	{ return true; }
    virtual Color		getColor() const;
    virtual bool		isPicking() const;
    virtual void		otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,int);
    virtual NotifierAccess*	getManipulationNotifier() { return &manip_; }
    virtual void		setDisplayTransformation(mVisTrans*);
    virtual mVisTrans*		getDisplayTransformation();
    virtual void		setSceneEventCatcher(visBase::EventCatcher*);
    virtual void                fillPar(IOPar&,TypeSet<int>&) const;
    virtual int                 usePar(const IOPar&);

    int				getPickIdx(visBase::DataObject*) const;

    const SurveyObject*		getPickedSurveyObject() const;

    void			removeSelected(const Selector<Coord3>&);

    bool			setDataTransform(ZAxisTransform*);
    const ZAxisTransform*	getDataTransform() const;

protected:
					LocationDisplay();
    virtual visBase::VisualObject*	createLocation() const  = 0;
    virtual void			setPosition(int idx,
	    					    const Pick::Location&)  = 0;
    virtual bool			hasDirection() const { return false; }
    virtual bool			hasText() const { return false; }
    virtual int			isMarkerClick(const TypeSet<int>&) const;
    virtual int			isDirMarkerClick(const TypeSet<int>&) const;
    void			triggerDeSel();

    virtual			~LocationDisplay();

    bool			addPick(const Coord3&,const Sphere&,bool);
    void			removePick(int);
    void			addDisplayPick(const Pick::Location&,int);

    bool			getPickSurface(const visBase::EventInfo&,
					   Coord3& pos, Coord3& normal) const;
    Coord3			display2World(const Coord3&) const;
    Coord3			world2Display(const Coord3&) const;
    bool			transformPos(Pick::Location&) const;
    void			setUnpickable(bool yn);

    void			pickCB(CallBacker* cb);
    virtual void		locChg(CallBacker* cb);
    virtual void		setChg(CallBacker* cb);
    virtual void		dispChg(CallBacker* cb);

    Pick::Set*			set_;
    Pick::SetMgr*		picksetmgr_;
    Notifier<LocationDisplay>	manip_;
    int				waitsfordirectionid_;
    int				waitsforpositionid_;

    TypeSet<int>		invalidpicks_;

    bool			needline_;
    bool			showall_;
    int				mousepressid_;
    int				pickedsobjid_; //!< Picked SurveyObject ID

    bool			isbodydisplay_;
    visBase::TriangleStripSet*	bodydisplay_;
    visBase::PickStyle*		pickstyle_;
    visBase::DataObjectGroup*	group_;
    visBase::DrawStyle*         drawstyle_;
    visBase::EventCatcher*	eventcatcher_;
    visBase::PolyLine*          polyline_;
    visBase::Transformation*	transformation_;
    SoSeparator*                linesep_;
    ZAxisTransform*		datatransform_;

    MultiID			storedmid_;

    static const char*		sKeyID();
    static const char*		sKeyMgrName();
    static const char*		sKeyShowAll();
    static const char*		sKeyMarkerType();
    static const char*		sKeyMarkerSize();
};

};


#endif
