#ifndef uivelocitygridder_h
#define uivelocitygridder_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uivolumeprocessingmod.h"
#include "uivolprocstepdlg.h"
#include "velocitygridder.h"

class uiGridder2DSel;
class uiInterpolationLayerModel;
namespace Vel { class uiFunctionSel; }


namespace VolProc
{

class VelGriddingStep;

mExpClass(uiVolumeProcessing) uiVelocityGridder : public uiStepDialog
{ mODTextTranslationClass(uiVelocityGridder)
public:
		mDefaultFactoryInstanciationBase(
		    VolProc::VelGriddingStep::sFactoryKeyword(),
		    VolProc::VelGriddingStep::sFactoryDisplayName())
		    mDefaultFactoryInitClassImpl(uiStepDialog,createInstance)

protected:
				uiVelocityGridder(uiParent*,VelGriddingStep*);

    bool			acceptOK(CallBacker*);
    static uiStepDialog*	createInstance(uiParent*,VolProc::Step*);

    void			pickSelChange(CallBacker*);
    void			nameChangeCB(CallBacker*);
    void			sourceChangeCB(CallBacker*);

    uiInterpolationLayerModel*	layermodelfld_;
    uiGridder2DSel*		griddersel_;
    Vel::uiFunctionSel*		velfuncsel_;
    VelGriddingStep*		operation_;

    bool			namenotset_;
};


} // namespace VolProc

#endif
