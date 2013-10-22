#ifndef uivelocityvolumeconversion_h
#define uivelocityvolumeconversion_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uibatchlaunch.h"

class uiVelSel;
class uiSeisSel;
class uiPosSubSel;
class uiLabeledComboBox;

/*!\brief Velocity*/

namespace Vel
{

/*!Dialog to setup a velocity conversion for volumes on disk. */

mExpClass(uiSeis) uiBatchVolumeConversion : public uiFullBatchDialog
{
public:
			uiBatchVolumeConversion(uiParent*);

protected:

    void		inputChangeCB(CallBacker*);
    bool		prepareProcessing() { return true; }
    bool		fillPar(IOPar&);

    uiVelSel*		input_;
    uiPosSubSel*	possubsel_;
    uiLabeledComboBox*	outputveltype_;
    uiSeisSel*		outputsel_;
};

}; //namespace



#endif

