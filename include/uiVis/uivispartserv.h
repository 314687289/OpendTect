#ifndef uivispartserv_h
#define uivispartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.h,v 1.154 2005-12-19 08:17:15 cvshelene Exp $
________________________________________________________________________

-*/

#include "cubesampling.h"
#include "menuhandler.h"
#include "ranges.h"
#include "sets.h"
#include "thread.h"
#include "uiapplserv.h"

class BinIDValueSet;
class BufferStringSet;
class ColorTable;
class MultiID;
class PickSet;
class SeisTrcBuf;
class SurfaceInfo;
class uiMPEMan;
class uiPopupMenu;
class uiToolBar;
class uiMenuHandler;
class uiVisModeMgr;

namespace Attrib    { class ColorSelSpec; class SelSpec; class DataCubes; }
namespace visBase   { class DataObject; };
namespace visSurvey { class Scene; class PolyLineDisplay; };
namespace Threads   { class Mutex; };
namespace Tracking  { class TrackManager; };



/*! \brief The Visualisation Part Server */

class uiVisPartServer : public uiApplPartServer
{
    friend class 	uiMenuHandler;
    friend class        uiVisModeMgr;

public:
			uiVisPartServer(uiApplService&);
			~uiVisPartServer();

    const char*		name() const;
    			/*<\returns the partservers name */
    NotifierAccess&	removeAllNotifier();
    			/*<\Returns a notifier that is triggered
			            when the entire visualization is
				    closed. All visBase::DataObjects
				    must then be unrefed.
			*/

    visBase::DataObject* getObject( int id ) const;
    int			highestID() const;
    void		addObject( visBase::DataObject*, int sceneid,
				   bool saveinsessions  );
    void		shareObject( int sceneid, int id );
    void		findObject( const std::type_info&, TypeSet<int>& );
    void		findObject( const MultiID&, TypeSet<int>& );
    void		removeObject( visBase::DataObject*,int sceneid);
    void		removeObject(int id,int sceneid);
    void		setObjectName(int,const char*);
    const char*		getObjectName(int) const;

    int			addScene(visSurvey::Scene* =0);
    			/*!<Adds a scene. The argument is only used internally.
			    Don't use the argument when calling from outside.
			*/
    void		removeScene(int);

    void		getChildIds(int id,TypeSet<int>&) const;
			/*!< Gets a scenes' children or a volumes' parts
			     If id==-1, it will give the ids of the
			     scenes */

    bool		hasAttrib(int) const;
    int			getAttributeFormat(int id) const;
   			/*!\retval 0 volume
  			   \retval 1 traces
		           \retval 2 random positions */
    const Attrib::SelSpec* getSelSpec(int id) const;
    const Attrib::ColorSelSpec* getColorSelSpec(int id) const;
    void		setSelSpec(int id, const Attrib::SelSpec&);
    void		setColorSelSpec(int id, const Attrib::ColorSelSpec& );
    void		resetColorDataType(int);
    
			//Volume data stuff
    CubeSampling	getCubeSampling(int id) const;
    const Attrib::DataCubes* getCachedData(int id,bool color) const;
    bool		setCubeData(int id, bool color,
	    			    const Attrib::DataCubes*);
    			/*!< data becomes mine */

    			//Trace data
    void		getDataTraceBids(int id, TypeSet<BinID>&) const;
    Interval<float>	getDataTraceRange(int id) const;
    void		setTraceData(int id,bool color,SeisTrcBuf&);

    			// See visSurvey::SurfaceDisplay for details
    void		fetchSurfaceData(int,ObjectSet<BinIDValueSet>&) const;
    void		stuffSurfaceData(int,bool forcolordata,
					 const ObjectSet<BinIDValueSet>*);
    void		readAuxData(int);

    bool		blockMouseSelection( bool yn );
			/*!<\returns Previous status. */

    bool		disabMenus( bool yn );
			/*!<\returns The previous status. */
    bool		disabToolbars( bool yn );
			/*!<\returns The previous status. */

    bool		showMenu(int id,int menutype=0,const TypeSet<int>* =0,
	    			 const Coord3& = Coord3::udf());
    			/*!<\param menutype Please refer to \ref
				uiMenuHandler::executeMenu for a detailed
				description.
			*/

    uiMenuHandler*	getMenu(int id, bool create_if_does_not_exist=true);

    MultiID		getMultiID(int) const;
	
    int			getSelObjectId() const;
    void		setSelObjectId(int);

    			//Events and their functions
    void		unlockEvent();
    			/*!< This function _must_ be called after
			     the object has sent an event to unlock
			     the object. */
    int			getEventObjId() const;
    			/*<\returns the id that triggered the event */

    static const int	evUpdateTree;
    void		triggerTreeUpdate();

    static const int	evSelection;
    			/*<! Get the id with getEventObjId() */

    static const int	evDeSelection;
    			/*<! Get the id with getEventObjId() */

    static const int	evGetNewData;
    			/*<! Get the id with getEventObjId() */
    			/*!< Get selSpec with getSelSpec */

    void		calculateAllAttribs();
    bool		calculateAttrib(int id,bool newsel);
    bool		calculateColorAttrib(int,bool);

    bool		canHaveMultipleTextures(int) const;
    int			nrTextures(int) const;
    void		selectTexture(int id,int texture);
    void		selectNextTexture(int id,bool next);

    static const int	evMouseMove;
    Coord3		getMousePos(bool xyt) const;
			/*!< If !xyt mouse pos will be in inl, crl, t */
    BufferString	getMousePosVal() const;
    BufferString	getMousePosString() const	{ return mouseposstr; }


    static const int	evSelectAttrib;

    static const int	evSelectColorAttrib;
    static const int	evGetColorData;

    static const int	evInteraction;
    			/*<! Get the id with getEventObjId() */
    BufferString	getInteractionMsg(int id) const;
    			/*!< Returns dragger position or
			     Nr positions in picksets */

    static const int	evViewAll;
    static const int	evToHomePos;

    			// ColorTable stuff
    int				getColTabId(int) const;
    void			setClipRate(int,float);
    const TypeSet<float>*	getHistogram(int) const;

				//General stuff
    bool			deleteAllObjects();
    void			setZScale();
    bool			setWorkingArea();
    void			setViewMode(bool yn);
    void			setSoloMode(bool, TypeSet< TypeSet<int> >, int);
    bool                        isSoloMode() const;
    bool			isViewMode() const;
    void			turnOn(int,bool,bool doclean=false);
    bool			isOn(int) const;
    void			updateDisplay(bool, int, int refid =-1);

    bool			canDuplicate(int) const;
    int				duplicateObject(int id,int sceneid);
    				/*!< \returns id of new object */

    void			lockUnlockObject(int);
    bool			isLocked(int) const;

    				// Tracking stuff
    static const int		evAddSeedToCurrentObject;
    bool			sendAddSeedEvent();
    void			turnSeedPickingOn(bool yn);

    void			showMPEToolbar();
    uiToolBar*			getTrackTB() const;
    void			initMPEStuff();

    bool			dumpOI(int id) const;
    
    bool			usePar(const IOPar&);
    void			fillPar(IOPar&) const;

    void			setupRdmLinePreview( TypeSet<Coord> );
    void			cleanPreview();

protected:

    void			createMenuCB(CallBacker*);
    void			handleMenuCB(CallBacker*);

    visSurvey::Scene*		getScene(int);
    const visSurvey::Scene*	getScene(int) const;

    bool			selectAttrib(int id);

    bool			hasColorAttrib(int) const;
    bool			selectColorAttrib(int);
    void			removeColorData(int);

    bool			isManipulated(int id) const;
    void			acceptManipulation(int id);
    bool			resetManipulation(int id);

    bool			hasMaterial(int id) const;
    bool			setMaterial(int id);

    bool			hasColor(int id) const;

    void			setUpConnections(int id);
    				/*!< Should set all cbs for the object */
    void			removeConnections(int id);

    void			toggleDraggers();
    int				getTypeSetIdx(int);

    ObjectSet<visSurvey::Scene>	scenes;

    ObjectSet<uiMenuHandler>	menus;
    uiMenuHandler*		vismenu;

    uiMPEMan*			mpetools;

    Coord3			xytmousepos;
    Coord3			inlcrlmousepos;
    float			mouseposval;
    BufferString		mouseposstr;

    bool			viewmode;
    bool			issolomode;
    Threads::Mutex&		eventmutex;
    int				eventobjid;

    void			rightClickCB(CallBacker*);
    void			selectObjCB(CallBacker*);
    void			deselectObjCB(CallBacker*);
    void			interactionCB(CallBacker*);
    void			mouseMoveCB(CallBacker*);
    //void			updatePlanePos(CallBacker*);
    void			vwAll(CallBacker*);
    void			toHome(CallBacker*);
    void			colTabChangeCB(CallBacker*);

    MenuItem			selcolorattrmnuitem;
    MenuItem			resetmanipmnuitem;
    MenuItem			changecolormnuitem;
    MenuItem			changematerialmnuitem;
    MenuItem			resmnuitem;

    TypeSet<int>		lockedobjects;
    TypeSet< TypeSet<int> >	displayids_;

    static const char*		workareastr;
    static const char*		appvelstr;

    uiVisModeMgr*		vismgr;
    bool			blockmenus;

    visSurvey::PolyLineDisplay* pldisplay_;
};


/*!\mainpage Visualisation User Interface

  This module provides the plain user interface classes necessary to do the
  3D visualisation in the way that the user wants.

  Main task of this server is adding and removing scene objects and 
  transfer of data to be displayed. All supported scene objects are inheriting
  visSurvey::SurveyObject.

  A lot of user interaction is done via popupmenus, and each object has an
  own menu which can be accessed via getMenu. To add items or manipulate the
  menus, please refer to the uiMenuHandler documentation.

  */


class uiVisModeMgr 
{
public:
				uiVisModeMgr(uiVisPartServer*);
				~uiVisModeMgr() {};

	bool			allowTurnOn(int,bool);

protected:

	uiVisPartServer&	visserv;	
};

#endif
