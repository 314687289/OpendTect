#ifndef uistratlaymoddisp_h
#define uistratlaymoddisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "flatview.h"
#include "uistratmod.h"
#include "uigroup.h"

class PropertyRef;
class uiGraphicsScene;
class uiStratLayModEditTools;
class uiTextItem;
namespace Strat { class LayerModel; class LayerModelProvider; class Layer; }

/*!
\brief Strat Layer Model Displayer

  The world rect boundaries are [1,nrmodels+1] vs zrg_.
*/

mStruct(uiStrat) LMPropSpecificDispPars
{
    			LMPropSpecificDispPars( const char* nm=0 )
			    : propnm_(nm)	{}
    bool		operator==( const LMPropSpecificDispPars& oth ) const
			{ return propnm_ == oth.propnm_; }

    ColTab::MapperSetup	mapper_;
    BufferString	ctab_;
    float		overlap_;
    BufferString	propnm_;
};


mExpClass(uiStrat) uiStratLayerModelDisp : public uiGroup
{ mODTextTranslationClass(uiStratLayerModelDisp);
public:

			uiStratLayerModelDisp(uiStratLayModEditTools&,
					    const Strat::LayerModelProvider&);
			~uiStratLayerModelDisp();

    virtual void	modelChanged()			= 0;
    virtual void	reSetView()			= 0;
    virtual uiWorldRect	zoomBox() const			= 0;
    virtual void	setZoomBox(const uiWorldRect&)	= 0;
    virtual float	getDisplayZSkip() const		= 0;

    const Strat::LayerModel& layerModel() const;
    const TypeSet<float>& levelDepths() const		{ return lvldpths_; }
    int			selectedSequence() const	{ return selseqidx_; }
    void		selectSequence(int seqidx);

    virtual uiBaseObject* getViewer() { return 0; }
    bool		isFlattened() const		{ return flattened_; }
    void		setFlattened(bool yn,bool trigger=true);
    bool		isFluidReplOn() const		{ return fluidreplon_; }
    void		setFluidReplOn(bool yn)		{ fluidreplon_= yn; }
    bool		isBrineFilled() const		{return isbrinefilled_;}
    void		setBrineFilled(bool yn)		{ isbrinefilled_= yn; }
    

    float		getLayerPropValue(const Strat::Layer&,
	    				  const PropertyRef*,int) const;
    bool		setPropDispPars(const LMPropSpecificDispPars&);
    bool		getCurPropDispPars(LMPropSpecificDispPars&) const;

    Notifier<uiStratLayerModelDisp> sequenceSelected;
    Notifier<uiStratLayerModelDisp> genNewModelNeeded;
    Notifier<uiStratLayerModelDisp> rangeChanged;
    Notifier<uiStratLayerModelDisp> modelEdited;
    CNotifier<uiStratLayerModelDisp,IOPar> infoChanged;
    Notifier<uiStratLayerModelDisp> dispPropChanged;

protected:

    const Strat::LayerModelProvider& lmp_;
    uiStratLayModEditTools& tools_;
    uiTextItem*		frtxtitm_;
    int			selseqidx_;
    Interval<float>	zrg_;
    bool		flattened_;
    bool		fluidreplon_;
    bool		isbrinefilled_;
    TypeSet<float>	lvldpths_;
    TypeSet<LMPropSpecificDispPars> lmdisppars_;
    BufferString	dumpfnm_;

    bool		haveAnyZoom() const;
    virtual uiGraphicsScene& scene() const		= 0;		
    void		displayFRText();
    virtual void	drawSelectedSequence()		= 0;

    bool		doLayerModelIO(bool);
    virtual void	doLevelChg()			= 0;
    				//!< returns whether layermodel has changed

};


#endif

