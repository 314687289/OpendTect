#ifndef uiodmenumgr_h
#define uiodmenumgr_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodmenumgr.h,v 1.20 2006-12-15 13:28:14 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiodapplmgr.h"

class Timer;
class uiMenuItem;
class uiODHelpMenuMgr;
class uiPopupMenu;
class uiToolBar;


/*!\brief The OpendTect menu manager

  The uiODMenuMgr instance can be accessed like:
  ODMainWin()->menuMgr()

  All standard menus should be reachable directly without searching for
  the text. It is easy to add your own menu items. And tool buttons, for that
  matter.

*/

class uiODMenuMgr : public CallBacker
{

    friend class	uiODMain;
    friend class	uiODHelpMenuMgr;

public:

    uiPopupMenu*	fileMnu()		{ return filemnu; }
    uiPopupMenu*	procMnu()		{ return procmnu; }
    uiPopupMenu*	winMnu()		{ return winmnu; }
    uiPopupMenu*	viewMnu()		{ return viewmnu; }
    uiPopupMenu*	utilMnu()		{ return utilmnu; }
    uiPopupMenu*	helpMnu()		{ return helpmnu; }
    uiPopupMenu*	settMnu()		{ return settmnu; }

    uiPopupMenu*	getBaseMnu(uiODApplMgr::ActType);
    			//! < Within File menu
    uiPopupMenu*	getMnu(bool imp,uiODApplMgr::ObjType);
    			//! < Within File - Import or Export

    uiToolBar*		dtectTB()		{ return dtecttb; }
    uiToolBar*		coinTB()		{ return cointb; }
    uiToolBar*		manTB()			{ return mantb; }


    			// Probably not needed by plugins
    void		storePositions();
    void		updateStereoMenu();
    void		updateViewMode(bool);
    void		updateAxisMode(bool);
    bool		isSoloModeOn() const;
    void		enableMenuBar(bool);
    void		enableActButton(bool);
    void		setCameraPixmap(bool isperspective);

    Notifier<uiODMenuMgr> dTectTBChanged;

protected:

			uiODMenuMgr(uiODMain*);
			~uiODMenuMgr();
    void		initSceneMgrDepObjs();

    uiODMain&		appl;
    Timer&		timer;
    uiODHelpMenuMgr*	helpmgr;

    uiPopupMenu*	filemnu;
    uiPopupMenu*	procmnu;
    uiPopupMenu*	winmnu;
    uiPopupMenu*	viewmnu;
    uiPopupMenu*	utilmnu;
    uiPopupMenu*	impmnu;
    uiPopupMenu*	expmnu;
    uiPopupMenu*	manmnu;
    uiPopupMenu*	helpmnu;
    uiPopupMenu*	settmnu;
    ObjectSet<uiPopupMenu> impmnus;
    ObjectSet<uiPopupMenu> expmnus;

    uiToolBar*		dtecttb;
    uiToolBar*		cointb;
    uiToolBar*		mantb;

    void		fillFileMenu();
    void		fillProcMenu();
    void		fillWinMenu();
    void		fillViewMenu();
    void		fillUtilMenu();
    void		fillDtectTB();
    void		fillCoinTB();
    void		fillManTB();

    void		handleClick(CallBacker*);
    void		timerCB(CallBacker*);
    void		manSeis(CallBacker*);
    void		manHor(CallBacker*);
    void		manFlt(CallBacker*);
    void		manWll(CallBacker*);
    void		manPick(CallBacker*);
    void		manWvlt(CallBacker*);
    void		updateDTectToolBar(CallBacker*);

    uiMenuItem*		stereooffitm;
    uiMenuItem*		stereoredcyanitm;
    uiMenuItem*		stereoquadbufitm;
    uiMenuItem*		stereooffsetitm;
    int			axisid, actid, viewid, cameraid, soloid;

    inline uiODApplMgr&	applMgr()	{ return appl.applMgr(); }
    inline uiODSceneMgr& sceneMgr()	{ return appl.sceneMgr(); }

    void		showLogFile();
    void		mkViewIconsMnu();
};


#endif
