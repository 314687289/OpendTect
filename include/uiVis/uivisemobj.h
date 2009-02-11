#ifndef uivisemobj_h
#define uivisemobj_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		May 2004
 RCS:		$Id: uivisemobj.h,v 1.28 2009-02-11 10:35:05 cvsranojay Exp $
________________________________________________________________________


-*/

#include "callback.h"
#include "emposid.h"
#include "menuhandler.h"

namespace EM { class EdgeLineSet; class EdgeLineSegment; }
namespace visSurvey { class EMObjectDisplay; }

class uiParent;
class uiMenuHandler;
class uiVisPartServer;
class MultiID;


mClass uiVisEMObject : public CallBacker
{
public:
    			uiVisEMObject(uiParent*,int displayid,
				      uiVisPartServer* );
    			uiVisEMObject(uiParent*,const EM::ObjectID&,int sceneid,
				      uiVisPartServer*);
			~uiVisEMObject();
    bool		isOK() const;

    static const char*	getObjectType(int displayid);
    int			id() const { return displayid_; }
    EM::ObjectID	getObjectID() const;

    float		getShift() const;
    void		setDepthAsAttrib(int attrib);
    void		setOnlyAtSectionsDisplay(bool);
    uiMenuHandler&	getNodeMenu() { return nodemenu_; }

    int			nrSections() const;
    EM::SectionID	getSectionID(int idx) const;
    EM::SectionID	getSectionID(const TypeSet<int>* pickedpath) const;

    void		checkTrackingStatus();
    			/*!< Checks if there is a tracker for this object and
			     turns on wireframe, singlecolor and full res if
			     a tracker if found. */

    static const char*	trackingmenutxt();

protected:
    void		setUpConnections();
    void		connectEditor();
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);

    void		interactionLineRightClick(CallBacker*);
    void		createInteractionLineMenuCB(CallBacker*) {}
    void		handleInteractionLineMenuCB(CallBacker*) {}

    void		edgeLineRightClick(CallBacker*);
    void		createEdgeLineMenuCB(CallBacker*);
    void		handleEdgeLineMenuCB(CallBacker*);

    void		nodeRightClick(CallBacker*);
    void		createNodeMenuCB(CallBacker*);
    void		handleNodeMenuCB(CallBacker*);

    visSurvey::EMObjectDisplay*		getDisplay();
    const visSurvey::EMObjectDisplay*	getDisplay() const;



    uiParent*		uiparent_;
    uiVisPartServer*	visserv_;

    uiMenuHandler&	nodemenu_;
    uiMenuHandler&	edgelinemenu_;
    uiMenuHandler&	interactionlinemenu_;
    
    int			displayid_;

    MenuItem		singlecolmnuitem_;
    MenuItem		wireframemnuitem_;
    MenuItem		trackmenuitem_;
    MenuItem		editmnuitem_;
    MenuItem		removesectionmnuitem_;
    MenuItem		seedsmenuitem_;
    MenuItem		showseedsmnuitem_;
    MenuItem		seedpropmnuitem_;
    MenuItem		lockseedsmnuitem_;

    MenuItem		showonlyatsectionsmnuitem_;
    MenuItem		changesectionnamemnuitem_;
    bool		showedtexture_;

    MenuItem		makepermnodemnuitem_;
    MenuItem		removecontrolnodemnuitem_;
};

#endif
