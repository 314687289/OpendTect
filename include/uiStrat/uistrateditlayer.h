#ifndef uistrateditlayer_h
#define uistrateditlayer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          August 2012
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uidialog.h"
class UnitOfMeasure;
class uiGenInput;
class uiStratLayerContent;
class uiPropertyValFld;
namespace Strat { class Layer; class LayerSequence; }

/*!\brief Displays and optionally edits a Strat::Layer instance */

mExpClass(uiStrat) uiStratEditLayer : public uiDialog
{
public:
			uiStratEditLayer(uiParent*,Strat::Layer&,
					 const Strat::LayerSequence&,
					 bool editable=true);

    void		getUnits(ObjectSet<const UnitOfMeasure>&) const;

protected:

    bool			editable_;
    Strat::Layer&		lay_;
    uiGenInput*			lithfld_;
    uiGenInput*			topfld_;
    ObjectSet<uiPropertyValFld>	valflds_;
    uiStratLayerContent*	contfld_;

    bool			acceptOK(CallBacker*);

};


#endif
