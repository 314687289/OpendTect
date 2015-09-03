
#ifndef vislocationdisplay_h
#define vislocationdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		June 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "vissurveymod.h"
#include "visobject.h"
#include "vissurvobj.h"

class Sphere;
namespace Pick { class Set; class Location; class SetMgr; }
template <class T> class Selector;


namespace visSurvey
{

class Sower;

/*!\brief Used for displaying picksets of varying types.
  The class is not intended for standalone usage, but is a common ground for
  picksets and other classes, where the inheriting classes knows about display
  shapes ++.
*/

mExpClass(visSurvey) LocationDisplay : public visBase::VisualObjectImpl
				     , public visSurvey::SurveyObject
{
    friend class Sower;

public:
    virtual void		setSet(Pick::Set*); // once!
    void			setSetMgr(Pick::SetMgr*);
    				/*!<Only used for notifications. */
    Pick::Set*			getSet()		{ return set_; }
    const Pick::Set*		getSet() const		{ return set_; }

    MultiID			getMultiID() const	{ return storedmid_; }

    const char*			errMsg() const { return errmsg_.str(); }

    void			fullRedraw(CallBacker* =0);
    void			showAll(bool yn);
    bool			allShown() const	{ return showall_; }


    virtual BufferString	getManipulationString() const;
    void			getMousePosInfo(const visBase::EventInfo& ei,
	    					IOPar& iop ) const
				{ return SurveyObject::getMousePosInfo(ei,iop);}
    virtual void		getMousePosInfo(const visBase::EventInfo&,
						Coord3&,BufferString&,
						BufferString&) const;
    virtual bool		hasColor() const	{ return true; }
    virtual Color		getColor() const;
    virtual void		setColor(Color);

    virtual bool		allowsPicks() const	{ return true; }
    virtual bool		isPicking() const;
    virtual void		otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,int);
    virtual NotifierAccess*	getManipulationNotifier() { return &manip_; }
    virtual void		setDisplayTransformation(const mVisTrans*);
    virtual const mVisTrans*	getDisplayTransformation() const;
    void			setRightHandSystem(bool yn);
    virtual void		setSceneEventCatcher(visBase::EventCatcher*);
    virtual void                fillPar(IOPar&) const;
    virtual bool                usePar(const IOPar&);
    virtual void		setSelectionMode(bool){};

    int				getPickIdx(visBase::DataObject*) const;

    const SurveyObject*		getPickedSurveyObject() const;

    bool			canRemoveSelection() const	{ return true; }
    void			removeSelection(const Selector<Coord3>&,
	    					TaskRunner*);

    bool			setZAxisTransform(ZAxisTransform*,TaskRunner*);
    const ZAxisTransform*	getZAxisTransform() const;

protected:
				LocationDisplay();
    virtual void		setPosition(int idx,
	    				    const Pick::Location&);
    virtual void		setPosition(int idx,const Pick::Location&,
					    bool add) {};
    virtual void		removePosition(int);
    virtual void		removeAll();

    virtual bool		hasDirection() const { return false; }
    virtual bool		hasText() const { return false; }
    virtual int			clickedMarkerIndex(
					const visBase::EventInfo& evi)const;
    virtual bool		isMarkerClick(
					const visBase::EventInfo& evi)const;
    virtual void		updateDragger() {};
    virtual bool		removeSelections() { return false; }

    virtual int			isDirMarkerClick(const TypeSet<int>&) const;
    void			triggerDeSel();

    virtual			~LocationDisplay();

    bool			addPick(const Coord3&,const Sphere&,bool);
    void			removePick(int,bool setundo=true);

    bool			getPickSurface(const visBase::EventInfo&,
					   Coord3& pos, Coord3& normal) const;
    Coord3			display2World(const Coord3&) const;
    Coord3			world2Display(const Coord3&) const;
    bool			transformPos(Pick::Location&) const;

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

    bool			showall_;
    int				mousepressid_;
    int				pickedsobjid_; //!< Picked SurveyObject ID
    int				voiidx_;
    bool			ctrldown_;
    ObjectSet< Selector<Coord3> >   selectors_;

    visBase::EventCatcher*	eventcatcher_;
    const mVisTrans*		transformation_;
    ZAxisTransform*		datatransform_;

    MultiID			storedmid_;

    static const char*		sKeyID();
    static const char*		sKeyMgrName();
    static const char*		sKeyShowAll();
    static const char*		sKeyMarkerType();
    static const char*		sKeyMarkerSize();

    Sower*			sower_;
    Coord3			undoloccoord_;
    bool			undomove_;
    bool			selectionmodel_;

private:
    bool			removePicks(const Selector<Coord3>&);

};

};


#endif

