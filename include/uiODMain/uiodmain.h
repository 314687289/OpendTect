#ifndef uiodmain_h
#define uiodmain_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodmain.h,v 1.8 2004-02-17 10:58:52 bert Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"
class IOPar;
class uicMain;
class uiODMain;
class ODSession;
class CtxtIOObj;
class uiDockWin;
class uiODApplMgr;
class uiODMenuMgr;
class uiODSceneMgr;
class uiVisColTabEd;


uiODMain* ODMainWin();
//!< Top-level access for plugins


/*!\brief OpendTect application top level object */

class uiODMain : public uiMainWin
{
public:

			uiODMain(uicMain&);
			~uiODMain();

    bool		go();
    void		exit();

    uiODApplMgr&	applMgr()	{ return *applmgr; }
    uiODMenuMgr&	menuMgr()	{ return *menumgr; } //!< + toolbar
    uiODSceneMgr&	sceneMgr()	{ return *scenemgr; }
    uiVisColTabEd&	colTabEd()	{ return *ctabed; }

    Notifier<uiODMain>	sessionSave;	//!< When triggered, put data in pars
    Notifier<uiODMain>	sessionRestore;	//!< When triggered, get data from pars
    IOPar&		sessionPars();	//!< On session save or restore
    					//!< notification, to get/put data

    bool		hasSessionChanged(); /*!< Compares current session with 
    						  last saved. */
    void		saveSession();	//!< pops up the save session dialog
    void		restoreSession(); //!< pops up the restore session dlg

protected:

    uiODApplMgr*	applmgr;
    uiODMenuMgr*	menumgr;
    uiODSceneMgr*	scenemgr;
    uiVisColTabEd*	ctabed;
    uicMain&		uiapp;
    ODSession*		cursession;
    ODSession&		lastsession;
    uiDockWin*		ctabwin;

    bool		failed;

    virtual bool	closeOK();

private:

    bool		ensureGoodDataDir();
    bool		ensureGoodSurveySetup();
    bool		buildUI();

    CtxtIOObj*		getUserSessionIOData(bool);
    bool		updateSession();
    void		doRestoreSession();

public:

    bool		sceneMgrAvailable() const	{ return scenemgr; }
    bool		menuMgrAvailable() const	{ return menumgr; }

};


/*!\mainpage OpendTect Top Level (uiODMain)

  \section Introduction Introduction

  'Real systems have no top' (Betrand Meyer). What he meant is that real
  sytems cannot be characterised by a single function which can be split up
  into smaller functions and so on - the classical functional decomposition
  problem. Still, Object-oriented systems tend to have a top - a top level
  object containing or initiating the work.

  In OpendTect, this object is of the class uiODMain. There is one instance
  which is accessible through the global function ODMainWin().

  If you go down the responsibility graph from here you will find yourself
  in the world of the 'Application Logic'. This is software with the sole
  purpose of making this application do its work. In OO systems, as much
  intelligence and functionality needs to be put in general-purpose objects.
  In that sense, the usage of application logic objects needs to be minimised,
  and generalised objects re-usable in other applications should be preferred.

  OpendTect obeys that rule but still a considerable amount of
  application-specific coding is unavoidable, especially in the area of the
  user interface. And in larger systems things get out of hand in no time
  because of the complexity of the task to manage everything. Therefore,
  we designed some 'complexity-stoppers'. The trouble with those kind of
  objects is that they themselves make it harder to understand what's going
  on in the system. It is much easier to understand:

  "object A uses object B and C and then displays using D."

  than:

  "object A asks its service manager S that it needs input data. S asks B and
  C for the data and passes it to A. A does its thing and reports S it's ready.
  S then asks D to display the data it asks from A."

  In reality it's even worse than this. We wouldn't have done that if OpendTect
  were a nice little app of a few thousand lines of code. That is not the case.
  In larger systems <b>dependency management</b> is the name of the game.
  It becomes important to figure out what object 'knows' about what other
  objects and who is responsible for what. Further it becomes increasingly
  important to not only hide low-level details from high-level functionality,
  but also to make them independent of it. This is the foundation of the
  'Dependency Inversion' principle. In the overview of the Programmer's manual
  you will see a reference to documentation on this.

  \section Objects Objects

  As things are, two major things need to be introduced: the top-level manager
  objects and their responsibilities, and the 'Part Server' architecture.

  First of all, the top-level manager objects. This is fairly straightforward.
  The application level problems are foremost:
  <ul>
  <li>managing the menu and tool buttons
  <li>managing the scenes and related trees of user objects
  <li>doing the things that need to be done
  <li>color table and other 'rest' things
  </ul>

  All these things are uiODMain's responsibilities. The last item is done
  'directly' by uiODMain (of course it uses some fairly high level objects
  to do that, but still). The first three are delegated to three closely
  cooperating manager objects: uiODMenuMgr, uiODSceneMgr, and uiODApplMgr .
  
  To go to the second issue we'll ask the question: how does the application
  manager (the uiODApplMgr instance) get things done? Answer: it asks its
  'Part Manager' objects to do the work. The only thing uiODApplMgr does is
  coordinate everything. The next question then is: what is such a 'Part
  Manager'? Answer: an object providing services for a certain type of
  things needed in OpendTect.

  Example: The menu manager finds out that the user wants to do attribute
  editing (probably a menu item or button was clicked). The menu manager asks
  the application manager to take care of that. The application manager knows
  this is Attribute stuff, so it uses its uiAttribPartServer instance: it
  calls the editSet() method. During the editing however, the attrib part
  server may need data or services that are not strictly 'attribute' stuff.
  For example, when the user presses 'direct display' of attribute the attrib
  part server must know where to calculate the current attribute and it needs
  to deliver the data when finished.

  When a part server needs external data or when it has info that may be
  important for other part servers, it calls the 'sendEvent()' method to
  notify (via its 'uiApplService' instance) the application manager. In this
  case the application manager will then ask its uiVisPartServer instance
  about the position of the current element.

  Part servers are an absolute must to keep the complexity on the application
  level manageable. Already things are pretty complex; if all objects would
  'know' about all other objects things would get out of hand very quickly
  to the point that lots of bugs are very hard to solve and maintenance
  would be a nightmare.

*/


#endif
