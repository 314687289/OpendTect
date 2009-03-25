#ifndef uiodapplmgr_h
#define uiodapplmgr_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiodapplmgr.h,v 1.80 2009-03-25 14:30:07 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiodmain.h"

class uidTectMan;
class uiApplPartServer;
class uiApplService;
class uiAttribPartServer;
class uiEMAttribPartServer;
class uiEMPartServer;
class uiMPEPartServer;
class uiNLAPartServer;
class uiODApplService;
class uiPopupMenu;
class uiPickPartServer;
class uiSeisPartServer;
class uiSoViewer;
class uiStratTreeWin;
class uiVisPartServer;
class uiWellAttribPartServer;
class uiWellPartServer;
class uiODApplMgrDispatcher;
class uiODApplMgrAttrVisHandler;

class Color;
class Coord;
class DataPointSet;
class MultiID;
class ODSession;
namespace Pick { class Set; }

/*!\brief Application level manager - ties part servers together

  The uiODApplMgr instance can be accessed like:
  ODMainWin()->applMgr()
  For plugins it may be interesting to pop up one of the OpendTect standard
  dialogs at any point in time. The best way to do that is by calling one
  of the uiODApplMgr methods.

  A big part of this class is dedicated to handling the events from the various
  part servers.
 
 */

mClass uiODApplMgr : public CallBacker
{
public:

    uiPickPartServer*		pickServer()		{ return pickserv_; }
    uiVisPartServer*		visServer()		{ return visserv_; }
    uiSeisPartServer*		seisServer()		{ return seisserv_; }
    uiAttribPartServer*		attrServer()		{ return attrserv_; }
    uiEMPartServer*		EMServer() 		{ return emserv_; }
    uiEMAttribPartServer*	EMAttribServer()	{ return emattrserv_; }
    uiWellPartServer*		wellServer()		{ return wellserv_; }
    uiWellAttribPartServer*	wellAttribServer()	{ return wellattrserv_;}
    uiMPEPartServer*		mpeServer()		{ return mpeserv_; }
    uiNLAPartServer*		nlaServer()		{ return nlaserv_; }
    void			setNlaServer( uiNLAPartServer* s )
    				{ nlaserv_ = s; }
    uiApplService&		applService()
				{ return (uiApplService&)applservice_; }


    // Survey menu operations
    int				manageSurvey();
    enum ObjType		{ Seis, Hor, Flt, Wll, Attr, NLA, Pick, Sess,
				  Strat, Wvlt, MDef, Vel };
    enum ActType		{ Imp, Exp, Man };
    void			doOperation(ObjType,ActType,int opt=0);
    				//!< Not all combinations are available ...!
    void			manPreLoad(ObjType);

    // Processing menu operations
    void			editAttribSet();
    void			editAttribSet(bool);
    bool			editNLA(bool);
    void			createVol(bool);
    void			doXPlot();
    				//!< This plots between well and attrib
    void			crossPlot();
    				//!< This plots between attribs.
    void			createHorOutput(int,bool);
    void			reStartProc();
    void			processTime2Depth(CallBacker* =0);
    void			processPreStack(CallBacker* =0);

    // View menu operations
    void			setWorkingArea();
    void			setZStretch();
    void			setStereoOffset();

    // Scene menu operations
    void			addTimeDepthScene();

    // Utility menu operations
    void			batchProgs();
    void			pluginMan();
    void			posConversion();
    void			crDevEnv();
    void			setFonts();
    void			manageShortcuts();

    // Tree menu services
    // Selections
    void			selectWells(ObjectSet<MultiID>&);
    void			selectHorizon(MultiID&);
    void			selectFault(MultiID&);
    void			selectPolygonSurface(MultiID&);
    void			selectStickSet(MultiID&);
    bool			selectAttrib(int id,int attrib);

    // PickSets
    bool			storePickSets();
    bool			storePickSet(const Pick::Set&);
    bool			storePickSetAs(const Pick::Set&);
    bool			setPickSetDirs(Pick::Set&);
    bool			pickSetsStored() const;

    // Work. Don't use unless expert.
    bool			getNewData(int visid,int attrib);
    bool			evaluateAttribute(int visid,int attrib);
    bool			evaluate2DAttribute(int visid, int attrib);
    void			pageUpDownPressed(bool up);
    void			resetServers();
    void			modifyColorTable(int visid,int attrib,
	    					 int coltabrefattrib=mUdf(int));
    void			saveDefColTab(int visid,int attrib);
    NotifierAccess*		colorTableSeqChange();
    void			manSurvCB(CallBacker*)	  { manageSurvey(); }
    void			seisOut2DCB(CallBacker*)  { createVol(true); }
    void			seisOut3DCB(CallBacker*)  { createVol(false); }
    void			createVolProcOutput(CallBacker*);
    void			xPlot(CallBacker*)	  { doXPlot(); }
    void			editAttr2DCB(CallBacker*)
				    { editAttribSet(true); }
    void			editAttr3DCB(CallBacker*)
				    { editAttribSet(false);}
    void			doVolProc(CallBacker*);

    void			enableMenusAndToolBars(bool);
    void			enableTree(bool);
    void			enableSceneManipulation(bool);
    				/*!<Turns on/off viewMode and enables/disables
				    the possibility to go to actMode. */

    Notifier<uiODApplMgr>	getOtherFormatData;
				/*!<Is triggered when the vispartserver wants
				    data, but the format (as reported by
				    uiVisPartServer::getAttributeFormat() ) is
				    uiVisPartServer::OtherFormat. If you manage
				    a visobject with OtherFormat, you can
				    connect here to be notified when the object
				    needs data. The visid and attribidx is
				    retrieved by otherFormatVisID and
				    otherFormatAttrib. */
    int				otherFormatVisID() const
				    { return otherformatvisid_; }
    int				otherFormatAttrib() const
				    { return otherformatattrib_; }
    int				createMapDataPack(const DataPointSet&,int col);

protected:

				uiODApplMgr(uiODMain&);
				~uiODApplMgr();

    uiODMain&			appl_;
    uiODApplService&		applservice_;

    uiPickPartServer*		pickserv_;
    uiVisPartServer*		visserv_;
    uiNLAPartServer*		nlaserv_;
    uiAttribPartServer*		attrserv_;
    uiSeisPartServer*		seisserv_;
    uiEMPartServer*		emserv_;
    uiEMAttribPartServer*	emattrserv_;
    uiWellPartServer*		wellserv_;
    uiWellAttribPartServer*	wellattrserv_;
    uiMPEPartServer*		mpeserv_;
    uiODApplMgrDispatcher&	dispatcher_;
    uiODApplMgrAttrVisHandler&	attrvishandler_;

    bool			handleEvent(const uiApplPartServer*,int);
    void*			deliverObject(const uiApplPartServer*,int);

    bool			handleMPEServEv(int);
    bool			handleWellServEv(int);
    bool			handleWellAttribServEv(int);
    bool			handleEMServEv(int);
    bool			handleEMAttribServEv(int);
    bool			handlePickServEv(int);
    bool			handleVisServEv(int);
    bool			handleNLAServEv(int);
    bool			handleAttribServEv(int);

    void			surveyToBeChanged(CallBacker*);
    void			surveyChanged(CallBacker*);
    void			colSeqChg(CallBacker*);
    void			colMapperChg(CallBacker*);
    void			useDefColTab(int visid,int attrib);
    void			setHistogram(int visid,int attrib);
    void			setupRdmLinePreview(const TypeSet<Coord>&);
    void			cleanPreview();

    void			manStrat();

    void			createAndSetMapDataPack(int visid,int attrib,
	    					const DataPointSet&,int colnr);

    friend class		uiODApplService;

    inline uiODSceneMgr&	sceneMgr()	{ return appl_.sceneMgr(); }
    inline uiODMenuMgr&		menuMgr()	{ return appl_.menuMgr(); }

    int				otherformatvisid_;
    int				otherformatattrib_;

    friend class		uiODMain;
    friend class		uiODApplMgrDispatcher;
    friend class		uiODApplMgrAttrVisHandler;
};


#endif
