/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2001
 RCS:           $Id: uishiftattrib.cc,v 1.13 2007-01-18 08:54:04 cvshelene Exp $
________________________________________________________________________

-*/

#include "uishiftattrib.h"
#include "shiftattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uisteeringsel.h"
#include "uistepoutsel.h"

using namespace Attrib;


mInitAttribUI(uiShiftAttrib,Shift,"Reference shift",sKeyPositionGrp)

uiShiftAttrib::uiShiftAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d)
{
    inpfld_ = getInpFld();

    stepoutfld_ = new uiStepOutSel( this, uiStepOutSel::Setup().seltxt("Shift"),
	    			   is2d );
    stepoutfld_->setMinValue(-100);
    stepoutfld_->attach( alignedBelow, inpfld_ );

    timefld_ = new uiGenInput( this, zDepLabel(0,0), FloatInpSpec() );
    timefld_->setElemSzPol(uiObject::Small);
    timefld_->attach( rightTo, stepoutfld_ );

    dosteerfld_ = new uiGenInput( this, "Use steering", BoolInpSpec() );
    dosteerfld_->attach( alignedBelow, stepoutfld_ );
    dosteerfld_->valuechanged.notify( mCB(this,uiShiftAttrib,steerSel) );
    dosteerfld_->setElemSzPol( uiObject::SmallVar );

    steerfld_ = new uiSteeringSel( this, 0 );
    steerfld_->attach( alignedBelow, dosteerfld_ );

    steerSel(0);
    setHAlignObj( inpfld_ );
}


void uiShiftAttrib::steerSel( CallBacker* )
{
    steerfld_->display( dosteerfld_->getBoolValue() );
}


bool uiShiftAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),Shift::attribName()) )
	return false;

    mIfGetBinID( Shift::posStr(), pos, stepoutfld_->setBinID(pos) );
    mIfGetFloat( Shift::timeStr(), time, timefld_->setValue(time) );
    mIfGetBool( Shift::steeringStr(), dosteer,	dosteerfld_->setValue(dosteer));

    steerSel(0);
    return true;
}


bool uiShiftAttrib::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    putInp( steerfld_, desc, 1 );
    return true;
}


bool uiShiftAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),Shift::attribName()) )
	return false;

    const bool dosteering = dosteerfld_->getBoolValue();
    mSetFloat( Shift::timeStr(), timefld_->getfValue() );
    mSetBool( Shift::steeringStr(), dosteering ? steerfld_->willSteer() :false);
    mSetBinID( Shift::posStr(), stepoutfld_->getBinID() );

    return true;
}


bool uiShiftAttrib::getInput( Desc& desc )
{
    fillInp( inpfld_, desc, 0 );
    fillInp( steerfld_, desc, 1 );
    return true;
}


void uiShiftAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    BufferString str = zIsTime() ? "Time" : "Depth"; str += " shift";
    params += EvalParam( str, Shift::timeStr() );
    params += EvalParam( stepoutstr, Shift::posStr() );
}
