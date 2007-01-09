#ifndef uiodmenumgr_h
#define uiodmenumgr_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodmenumgr.h,v 1.22 2007-01-09 09:45:12 cvsnanne Exp $
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

    // TODO: fileMnu() only here for backward compatibility
    // Remove in version 3.2
    uiPopupMenu*	fileMnu()		{ return surveymnu_; }
    uiPopupMenu*	surveyMnu()		{ return surveymnu_; }
    uiPopupMenu*	procMnu()		{ return procmnu_; }
    uiPopupMenu*	winMnu()		{ return winmnu_; }
    uiPopupMenu*	viewMnu()		{ return viewmnu_; }
    uiPopupMenu*	utilMnu()		{ return utilmnu_; }
    uiPopupMenu*	helpMnu()		{ return helpmnu_; }
    uiPopupMenu*	settMnu()		{ return settmnu_; }

    uiPopupMenu*	getBaseMnu(uiODApplMgr::ActType);
    			//! < Within Survey menu
    uiPopupMenu*	getMnu(bool imp,uiODApplMgr::ObjType);
    			//! < Within Survey - Import or Export

    uiToolBar*		dtectTB()		{ return dtecttb_; }
    uiToolBar*		coinTB()		{ return cointb_; }
    uiToolBar*		manTB()			{ return mantb_; }


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
    Notifier<uiODMenuMgr> dTectProcMnuChanged;
    Notifier<uiODMenuMgr> dTectSurveyMnuChanged;

protected:

			uiODMenuMgr(uiODMain*);
			~uiODMenuMgr();
    void		initSceneMgrDepObjs();

    uiODMain&		appl_;
    Timer&		timer_;
    uiODHelpMenuMgr*	helpmgr_;

    uiPopupMenu*	surveymnu_;
    uiPopupMenu*	procmnu_;
    uiPopupMenu*	winmnu_;
    uiPopupMenu*	viewmnu_;
    uiPopupMenu*	utilmnu_;
    uiPopupMenu*	impmnu_;
    uiPopupMenu*	expmnu_;
    uiPopupMenu*	manmnu_;
    uiPopupMenu*	helpmnu_;
    uiPopupMenu*	settmnu_;
    ObjectSet<uiPopupMenu> impmnus_;
    ObjectSet<uiPopupMenu> expmnus_;

    uiToolBar*		dtecttb_;
    uiToolBar*		cointb_;
    uiToolBar*		mantb_;

    void		fillSurveyMenu();
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
    void		updateDTectProcMnu(CallBacker*);
    void		updateDTectSurveyMnu(CallBacker*);

    uiMenuItem*		stereooffitm_;
    uiMenuItem*		stereoredcyanitm_;
    uiMenuItem*		stereoquadbufitm_;
    uiMenuItem*		stereooffsetitm_;
    int			axisid_, actid_, viewid_, cameraid_, soloid_;

    inline uiODApplMgr&	applMgr()	{ return appl_.applMgr(); }
    inline uiODSceneMgr& sceneMgr()	{ return appl_.sceneMgr(); }

    void		showLogFile();
    void		mkViewIconsMnu();
};


#endif
