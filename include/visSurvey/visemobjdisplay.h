#ifndef visemobjdisplay_h
#define visemobjdisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          May 2004
 RCS:           $Id: visemobjdisplay.h,v 1.46 2007-02-27 14:11:37 cvsjaap Exp $
________________________________________________________________________


-*/


#include "bufstringset.h"
#include "draw.h"
#include "emposid.h"
#include "visobject.h"
#include "vissurvobj.h"


class Executor;

namespace EM { class EMManager; class EMObject; }
namespace Geometry { class Element; }
namespace visBase
{
    class DataObjectGroup;
    class DrawStyle;
    class IndexedPolyLine;
    class VisColorTab;
}

namespace visSurvey
{

class MPEEditor;
class EdgeLineSetDisplay;


class EMObjectDisplay :  public  visBase::VisualObjectImpl,
                         public visSurvey::SurveyObject
{
public:
    				EMObjectDisplay();
    mVisTrans*			getDisplayTransformation();
    void			setDisplayTransformation(mVisTrans*);
    void			setSceneEventCatcher( visBase::EventCatcher* );

    virtual bool		setEMObject(const EM::ObjectID&);
    EM::ObjectID		getObjectID() const;
    virtual bool		updateFromEM();
    virtual void		updateFromMPE();

    void			showPosAttrib( int attr, bool yn);
    				/*!<Turns position attributes (as defined in
				    EM::EMObject) to be marked with a marker. */
    bool			showsPosAttrib( int attr ) const;
    				/*!<\returns wether a position attribute (as
				     defined in EM::EMObject) to be marked
				     with a marker. */

    MultiID			getMultiID() const;
    BufferStringSet		displayedSections() const;

    virtual void		setOnlyAtSectionsDisplay(bool yn);
    virtual bool		getOnlyAtSectionsDisplay() const;

    bool			allowMaterialEdit() const { return true; }
    const LineStyle*		lineStyle() const;
    void			setLineStyle(const LineStyle&);

    bool			hasColor() const;
    void			setColor(Color);
    Color			getColor() const;

    bool			allowPicks() const	{ return true; }
    void			getMousePosInfo(const visBase::EventInfo& pos,
	    					const Coord3&,
	    					BufferString& val,
						BufferString& info) const;
    MPEEditor*			getEditor();
    void			enableEditing(bool yn);
    bool			isEditingEnabled() const;

    virtual EM::SectionID	getSectionID(int visid) const		= 0;
    EM::SectionID		getSectionID(const TypeSet<int>* path) const;

    EM::PosID			getPosAttribPosID(int attrib,
					   const TypeSet<int>& path ) const;

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);

    NotifierAccess*		getMovementNotification();
    Notifier<EMObjectDisplay>	hasmoved;
    Notifier<EMObjectDisplay>	changedisplay;

    void			lock(bool yn);
    NotifierAccess*		getLockNotification();
    Notifier<EMObjectDisplay>	locknotifier;

protected:
    				~EMObjectDisplay();
    virtual void		removeEMStuff();

    virtual void		removeSectionDisplay(const EM::SectionID&) = 0;
    virtual bool		addSection(const EM::SectionID&)   	   = 0;

    virtual EM::PosID 		findClosestNode(const Coord3&) const;
    virtual void		emChangeCB(CallBacker*);
    virtual void		clickCB(CallBacker*);
    virtual void		updatePosAttrib(int attrib);

    mVisTrans*				transformation_;
    visBase::EventCatcher*		eventcatcher_;

    ObjectSet<visBase::DataObjectGroup>	posattribmarkers_;
    TypeSet<int>			posattribs_;
    TypeSet<int>			parposattrshown_;

    EM::EMManager&			em_;
    EM::EMObject*			emobject_;
    MultiID				parmid_;
    BufferStringSet			parsections_;

    MPEEditor*				editor_;

    mutable Color			nontexturecol_;
    mutable bool			nontexturecolisset_;
    visBase::DrawStyle*			drawstyle_;
    bool				displayonlyatsections_;
    bool				enableedit_;
    bool				restoresessupdate_;

    bool				burstalertison_;
    int 				postponedposchanges_;

    static const char*			sKeyEarthModelID;
    static const char*			sKeyResolution;
    static const char*			sKeyEdit;
    static const char*			sKeyOnlyAtSections;
    static const char*			sKeyLineStyle;
    static const char*			sKeySections;
    static const char*			sKeyPosAttrShown;
};


};

#endif


