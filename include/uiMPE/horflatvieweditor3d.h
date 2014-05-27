#ifndef horflatvieweditor3d_h
#define horflatvieweditor3d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uimpemod.h"
#include "uimpemod.h"
#include "cubesampling.h"
#include "emposid.h"
#include "flatview.h"

namespace Attrib { class SelSpec; }
namespace EM { class HorizonPainter3D; }
namespace FlatView { class AuxDataEditor; }

class FlatDataPack;
class MouseEvent;
class MouseEventHandler;

namespace MPE
{

class EMTracker;
class EMSeedPicker;

mExpClass(uiMPE) HorizonFlatViewEditor3D : public CallBacker
{ mODTextTranslationClass(HorizonFlatViewEditor3D);
public:
    			HorizonFlatViewEditor3D(FlatView::AuxDataEditor*,
						const EM::ObjectID&);
			~HorizonFlatViewEditor3D();
    
    void		setCubeSampling(const CubeSampling&);
    void		setPath(const TypeSet<BinID>*);
    void		setFlatPosData(const FlatPosData*);
    void		setSelSpec(const Attrib::SelSpec*,bool wva);

    FlatView::AuxDataEditor* getEditor()                { return editor_; }
    EM::HorizonPainter3D* getPainter() const		{ return horpainter_; }
    void		setMouseEventHandler(MouseEventHandler*);

    void		enableLine(bool);
    void		enableSeed(bool);
    void		paint();

    void		setSeedPicking(bool);
    void		setTrackerSetupActive(bool yn)
			{ trackersetupactive_ = yn; }

    Notifier<HorizonFlatViewEditor3D>	updseedpkingstatus_;

protected:

    void		horRepaintATSCB( CallBacker* );
    void		horRepaintedCB( CallBacker* );

    void		mouseMoveCB(CallBacker*);
    void		mousePressCB(CallBacker*);
    void		mouseReleaseCB(CallBacker*);
    void		movementEndCB(CallBacker*);
    void		removePosCB(CallBacker*);

    bool		checkSanity(EMTracker&,const EMSeedPicker&,
	    			    bool& pickinvd) const;
    bool		prepareTracking(bool pickinvd,const EMTracker&,
	    			       EMSeedPicker&,const FlatDataPack&) const;
    bool		getPosID(const Coord3&,EM::PosID&) const;
    bool		doTheSeed(EMSeedPicker&,const Coord3&,
	    			  const MouseEvent&) const;

    	mStruct(uiMPE) Hor3DMarkerIdInfo
	{
	    FlatView::AuxData*	marker_;
	    int			markerid_;
	    EM::SectionID	sectionid_;
	};

    void			cleanAuxInfoContainer();
    void			fillAuxInfoContainer();
    FlatView::AuxData*		getAuxData(int markerid);
    EM::SectionID		getSectionID(int markerid);

    EM::ObjectID		emid_;
    EM::HorizonPainter3D*	horpainter_;

    FlatView::AuxDataEditor*	editor_;
    ObjectSet<Hor3DMarkerIdInfo> markeridinfos_;
    MouseEventHandler*		mehandler_;
    CubeSampling		curcs_;
    const Attrib::SelSpec*	vdselspec_;
    const Attrib::SelSpec*	wvaselspec_;

    bool			seedpickingon_;
    bool			trackersetupactive_;
};

} //namespace MPE


#endif


