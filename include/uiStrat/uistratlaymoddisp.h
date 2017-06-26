#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2010
________________________________________________________________________

-*/

#include "flatview.h"
#include "uistratmod.h"
#include "uigroup.h"
#include "uistring.h"
#include "coltabmappersetup.h"

class PropertyRef;
class uiGraphicsScene;
class uiStratLayModEditTools;
class uiTextItem;
class uiFlatViewer;
namespace Strat { class LayerModel; class LayerModelProvider; class Layer; }

/*!
\brief Strat Layer Model Displayer

  The world rect boundaries are [1,nrmodels+1] vs zrg_.
*/

mStruct(uiStrat) LMPropSpecificDispPars
{
			LMPropSpecificDispPars( const char* nm=0 )
			    : propnm_(nm)
			    , mappersetup_(new ColTab::MapperSetup)	{}
    bool		operator==( const LMPropSpecificDispPars& oth ) const
			{ return propnm_ == oth.propnm_; }

    RefMan<ColTab::MapperSetup>	mappersetup_;
    BufferString	colseqname_;
    float		overlap_;
    BufferString	propnm_;

};


mExpClass(uiStrat) uiStratLayerModelDisp : public uiGroup
{ mODTextTranslationClass(uiStratLayerModelDisp);
public:
    typedef TypeSet<float> LVLZVals;
    typedef TypeSet< LVLZVals > LVLZValsSet;

			uiStratLayerModelDisp(uiStratLayModEditTools&,
					    const Strat::LayerModelProvider&);
			~uiStratLayerModelDisp();

    virtual void	modelChanged()			= 0;
    virtual void	reSetView()			= 0;
    virtual uiWorldRect	zoomBox() const			= 0;
    virtual void	setZoomBox(const uiWorldRect&)	= 0;
    virtual float	getDisplayZSkip() const		= 0;
    virtual Interval<float> relDepthZoneOfInterest() const
			{ return Interval<float>::udf(); }
    virtual void	reSetRelDepthZoneOfInterest()	{}
    virtual bool	canSetDisplayProperties() const	{ return false; }

    const Strat::LayerModel& layerModel() const;
    int			selectedSequence() const	{ return selseqidx_; }
    void		selectSequence(int seqidx);

    uiFlatViewer*	getViewer() { return &vwr_; }
    bool		isFlattened() const		{ return flattened_; }
    bool		canBeFlattened() const;
    void		setFlattened(bool yn,bool trigger=true);
    bool		isFluidReplOn() const		{ return fluidreplon_; }
    void		setFluidReplOn(bool yn)		{ fluidreplon_= yn; }
    bool		isBrineFilled() const		{return isbrinefilled_;}
    void		setBrineFilled(bool yn)		{ isbrinefilled_= yn; }


    float		getLayerPropValue(const Strat::Layer&,
					  const PropertyRef*,int) const;
    bool		setPropDispPars(const LMPropSpecificDispPars&);
    bool		getCurPropDispPars(LMPropSpecificDispPars&) const;
    int			getFlattenLevelIdx() const;
    const LVLZValsSet&	getLevelDepths()	{ return lvldpths_; }
    void		clearDispPars()		{ lmdisppars_.erase(); }

    Notifier<uiStratLayerModelDisp> sequenceSelected;
    Notifier<uiStratLayerModelDisp> genNewModelNeeded;
    Notifier<uiStratLayerModelDisp> rangeChanged;
    Notifier<uiStratLayerModelDisp> modelEdited;
    CNotifier<uiStratLayerModelDisp,IOPar> infoChanged;
    Notifier<uiStratLayerModelDisp> dispPropChanged;

protected:

    uiFlatViewer&	vwr_;
    const Strat::LayerModelProvider& lmp_;
    uiStratLayModEditTools& tools_;
    uiTextItem*		frtxtitm_;
    int			selseqidx_;
    Interval<float>	zrg_;
    bool		flattened_;
    bool		fluidreplon_;
    bool		isbrinefilled_;
    LVLZValsSet		lvldpths_;
    TypeSet<LMPropSpecificDispPars> lmdisppars_;
    IOPar		dumppars_;

    bool		haveAnyZoom() const;
    uiGraphicsScene&	scene() const;
    void		displayFRText();
    virtual void	drawSelectedSequence()		= 0;

    int			getClickedModelNr() const;
    void		mouseMoved(CallBacker*);
    void		doubleClicked(CallBacker*);
    void		usrClicked(CallBacker*);
    virtual void	selPropChgCB(CallBacker*)	= 0;
    virtual void	dispLithChgCB(CallBacker*)	= 0;
    virtual void	selContentChgCB(CallBacker*)	= 0;
    virtual void	selLevelChgCB(CallBacker*)	= 0;
    virtual void	dispEachChgCB(CallBacker*)	= 0;
    virtual void	dispZoomedChgCB(CallBacker*)	= 0;
    bool		doLayerModelIO(bool);
    virtual void	doLevelChg()			= 0;
    virtual void	handleClick(bool dble)		= 0;
				//!< returns whether layermodel has changed

};
