#ifndef visemobjdisplay_h
#define visemobjdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          May 2004
 RCS:           $Id: visemobjdisplay.h,v 1.63 2010-03-23 21:16:37 cvsyuancheng Exp $
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
    class TextureChannel2RGBA;
}

namespace visSurvey
{

class MPEEditor;
class EdgeLineSetDisplay;


mClass EMObjectDisplay :  public  visBase::VisualObjectImpl,
                          public visSurvey::SurveyObject
{
public:
    				EMObjectDisplay();
    mVisTrans*			getDisplayTransformation();
    void			setDisplayTransformation(mVisTrans*);
    void			setSceneEventCatcher( visBase::EventCatcher* );

    virtual bool		setEMObject(const EM::ObjectID&,TaskRunner*);
    EM::ObjectID		getObjectID() const;
    virtual bool		updateFromEM(TaskRunner*);
    virtual void		updateFromMPE();

    void			showPosAttrib( int attr, bool yn);
    				/*!<Turns position attributes (as defined in
				    EM::EMObject) to be marked with a marker. */
    bool			showsPosAttrib( int attr ) const;
    				/*!<\returns wether a position attribute (as
				     defined in EM::EMObject) to be marked
				     with a marker. */
    const char*			errMsg() const { return errmsg_.buf(); }

    MultiID			getMultiID() const;
    BufferStringSet		displayedSections() const;

    virtual void		setOnlyAtSectionsDisplay(bool yn);
    virtual bool		getOnlyAtSectionsDisplay() const;

    bool			allowMaterialEdit() const { return true; }
    const LineStyle*		lineStyle() const;
    void			setLineStyle(const LineStyle&);
    void			getLineWidthBounds( int& min, int& max );

    bool			hasColor() const;
    void			setColor(Color);
    Color			getColor() const;

    bool			allowsPicks() const	{ return true; }
    virtual void		getMousePosInfo(const visBase::EventInfo&,
	    					Coord3&,
	    					BufferString& val,
						BufferString& info) const;
    MPEEditor*			getEditor();
    void			enableEditing(bool yn);
    bool			isEditingEnabled() const;

    virtual EM::SectionID	getSectionID(int visid) const		= 0;
    EM::SectionID		getSectionID(const TypeSet<int>* path) const;

    EM::PosID			getPosAttribPosID(int attrib,
					   const TypeSet<int>& path ) const;

    bool			canRemoveSelecion()	{ return true; }
    void                        removeSelection(const Selector<Coord3>&,
	    					TaskRunner*);

    virtual bool		setChannels2RGBA(visBase::TextureChannel2RGBA*);
    virtual visBase::TextureChannel2RGBA* getChannels2RGBA();

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);

    NotifierAccess*		getMovementNotifier()	{ return &hasmoved; }
    Notifier<EMObjectDisplay>	changedisplay;

    void			lock(bool yn);
    NotifierAccess*		getLockNotifier()	{ return &locknotifier;}

protected:
    				~EMObjectDisplay();
    virtual void		removeEMStuff();

    virtual void		removeSectionDisplay(const EM::SectionID&) = 0;
    virtual bool		addSection(const EM::SectionID&,TaskRunner*)=0;

    virtual EM::PosID 		findClosestNode(const Coord3&) const;
    virtual void		emChangeCB(CallBacker*);
    virtual void		clickCB(CallBacker*);
    virtual void		updatePosAttrib(int attrib);

    Notifier<EMObjectDisplay>	hasmoved;
    Notifier<EMObjectDisplay>	locknotifier;

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
    visBase::TextureChannel2RGBA*	channel2rgba_;
					//set in usePar,
					//should be nil when given
					//to channels_.

    mutable Color			nontexturecol_;
    mutable bool			nontexturecolisset_;
    visBase::DrawStyle*			drawstyle_;
    bool				displayonlyatsections_;
    bool				enableedit_;
    bool				restoresessupdate_;

    bool				burstalertison_;

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


