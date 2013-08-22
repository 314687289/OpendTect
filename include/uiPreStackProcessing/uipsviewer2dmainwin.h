#ifndef uipsviewer2dmainwin_h
#define uipsviewer2dmainwin_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "uipsviewer2dposdlg.h"
#include "uiobjectitemviewwin.h"
#include "uiflatviewwin.h"
#include "uiflatviewstdcontrol.h"

#include "multiid.h"
#include "cubesampling.h"
#include "flatview.h"


class uiSlicePos2DView;
class uiColorTableSel;

namespace PreStack { class Gather; class MuteDef; 
		     class VelocityBasedAngleComputer; }
namespace FlatView { class AuxData; }
namespace PreStackView
{
    class uiViewer2D;
    class uiViewer2DControl;
    class uiViewer2DPosDlg;
    class uiGatherDisplay;
    class uiGatherDisplayInfoHeader;

mExpClass(uiPreStackProcessing) uiViewer2DMainWin : public uiObjectItemViewWin, public uiFlatViewWin
{
public:    
			uiViewer2DMainWin(uiParent*,const char* title);
			~uiViewer2DMainWin();

    virtual void 	start()		{ show(); }
    virtual void        setWinTitle( const char* t )    { setCaption(t); }

    virtual uiMainWin*  dockParent()    { return this; }
    virtual uiParent*   viewerParent()  { return this; }
    uiViewer2DControl*	control()	{ return control_; }
    virtual void	getGatherNames(BufferStringSet& nms) const	= 0;
    virtual bool	is2D() const	{ return false; }
    bool		isStored() const;
    void		getStartupPositions(const BinID& bid,
	    				    const StepInterval<int>& trcrg,
					    bool isinl, TypeSet<BinID>&) const;

    Notifier<uiViewer2DMainWin> seldatacalled_;

protected:

    TypeSet<GatherInfo>	gatherinfos_;
    TypeSet<int> 	dpids_;
    ObjectSet<PreStack::MuteDef> mutes_;
    TypeSet<Color>	mutecolors_;
    CubeSampling 	cs_;
    uiSlicePos2DView*	slicepos_;
    uiViewer2DPosDlg* 	posdlg_;
    uiViewer2DControl*	control_;
    uiObjectItemViewAxisPainter* axispainter_;
    Interval<float>	zrg_;
    ObjectSet<uiGatherDisplay>	gd_;
    ObjectSet<uiGatherDisplayInfoHeader> gdi_;

    void		removeAllGathers();
    void		reSizeItems();
    virtual void	setGatherInfo(uiGatherDisplayInfoHeader* info,
	    			      const GatherInfo&) 	{}
    virtual void	setGather(const GatherInfo& pos)	{} 
    void		setGatherView(uiGatherDisplay*,
	    			      uiGatherDisplayInfoHeader*);
    PreStack::Gather*   getAngleGather(const PreStack::Gather& gather, 
				       const PreStack::Gather& angledata,
				       const Interval<int>& anglerange);
    void		setAngleData(int idx, PreStack::Gather* gather,
				     PreStack::Gather* angledata,
				     const Interval<int>& anglerange);
    void		setAngleGather(int idx, PreStack::Gather* anglegather);
    void		displayAngle(bool isanglegather);
    void		convAngleDatatoDegrees(PreStack::Gather* angledata);

    void 		setUpView();
    void		clearAuxData();
    void		displayMutes();

    void		displayInfo(CallBacker*);
    void		doHelp(CallBacker*);
    void		posSlcChgCB(CallBacker*);
    virtual void	posDlgChgCB(CallBacker*)			=0;
    void		posDlgPushed(CallBacker*);		
    void 		dataDlgPushed(CallBacker*);
    void		showZAxis(CallBacker*);
    void		loadMuteCB(CallBacker*);
    void		angleGatherCB(CallBacker*);
    void		angleDataCB(CallBacker*);
    void		snapshotCB(CallBacker*);
};


mExpClass(uiPreStackProcessing) uiStoredViewer2DMainWin : public uiViewer2DMainWin
{
public:
			uiStoredViewer2DMainWin(uiParent*,const char* title);

    void 		init(const MultiID&,const BinID& bid,bool isinl,
			     const StepInterval<int>&,const char* linename=0);
    void 		setIDs(const TypeSet<MultiID>&);
    void 		getIDs(TypeSet<MultiID>& mids) const 
			{ mids.copy( mids_ ); }
    void		getGatherNames(BufferStringSet& nms) const;
    virtual bool	is2D() const	{ return is2d_; }
    
protected:
    TypeSet<MultiID> 	mids_;

    bool		is2d_;
    BufferString	linename_;
    void		setGatherInfo(uiGatherDisplayInfoHeader* info,
	    			      const GatherInfo&);
    void		setGather(const GatherInfo&); 
    void		posDlgChgCB(CallBacker*);
};


mExpClass(uiPreStackProcessing) uiSyntheticViewer2DMainWin : public uiViewer2DMainWin
{
public:
    			uiSyntheticViewer2DMainWin(uiParent*,const char* title);
    			~uiSyntheticViewer2DMainWin();
    void		setGathers(const TypeSet<PreStackView::GatherInfo>&);
    void		removeGathers();
    void		getGatherNames(BufferStringSet& nms) const;
    void		setGatherNames(const BufferStringSet& nms);
protected:
    void		posDlgChgCB(CallBacker*);

    void		setGatherInfo(uiGatherDisplayInfoHeader* info,
	    			      const GatherInfo&);
    void		setGather(const GatherInfo&); 
};


mClass(uiPreStackProcessing) uiViewer2DControl : public uiFlatViewStdControl
{
public:
			uiViewer2DControl(uiObjectItemView&,uiFlatViewer&);
			~uiViewer2DControl();

    Notifier<uiViewer2DControl> posdlgcalled_;
    Notifier<uiViewer2DControl> datadlgcalled_;

    void 			removeAllViewers();
    const FlatView::DataDispPars& dispPars() const    { return app_.ddpars_; }
    FlatView::DataDispPars&	dispPars()	      { return app_.ddpars_; }

protected:

    uiObjectItemViewControl*	objectitemctrl_;
    FlatView::Appearance	app_;
    uiColorTableSel*		ctabsel_;

    void		doPropertiesDialog(int vieweridx,bool dowva);

    void		applyProperties(CallBacker*);
    void		gatherPosCB(CallBacker*);
    void		gatherDataCB(CallBacker*);
    void		coltabChg(CallBacker*);
    void		updateColTabCB(CallBacker*);
};


}; //namespace

#endif

