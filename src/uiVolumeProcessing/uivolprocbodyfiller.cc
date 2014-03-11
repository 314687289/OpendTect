/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uivolprocbodyfiller.h"
#include "volprocbodyfiller.h"

#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uivolprocchain.h"

#include "embodytr.h"
#include "separstr.h"


namespace VolProc
{

void uiBodyFiller::initClass()
{
    SeparString str( sFactoryKeyword(), uiStepDialog::factory().cSeparator() );
    str += BodyFiller::sKeyOldType();

    uiStepDialog::factory().addCreator( createInstance, str,
	    			        sFactoryDisplayName() );
}    


uiBodyFiller::uiBodyFiller( uiParent* p, BodyFiller* mp )
    : uiStepDialog( p, BodyFiller::sFactoryDisplayName(), mp )
    , bodyfiller_( mp )
{
    setHelpKey( "103.6.2" );

    IOObjContext ctxt = mIOObjContext( EMBody );
    ctxt.forread = true;
    uinputselfld_ = new uiIOObjSel( this, ctxt, "Input Body" );
    uinputselfld_->selectionDone.notify( mCB(this,uiBodyFiller,bodySel) );
    if ( mp )
	uinputselfld_->setInput( mp->getSurfaceID() );
    
    const bool showinside = !mIsUdf( mp->getInsideValue() ); 
    useinsidefld_ = new uiGenInput( this, "Inside",
	    BoolInpSpec( showinside, "Value", "Transparent" ) );
    useinsidefld_->attach( alignedBelow, uinputselfld_ );
    useinsidefld_->valuechanged.notify( mCB(this,uiBodyFiller,updateFlds) );
    insidevaluefld_ = new uiGenInput( this, "Inside-Value",
	    FloatInpSpec( showinside ? mp->getInsideValue() : -2000 ) );
    insidevaluefld_->attach( alignedBelow, useinsidefld_ );
    
    const bool showoutside = !mIsUdf( mp->getOutsideValue() ); 
    useoutsidefld_ = new uiGenInput( this, "Outside",
	    BoolInpSpec( showoutside, "Value", "Transparent" ) );
    useoutsidefld_->attach( alignedBelow, insidevaluefld_ );
    useoutsidefld_->valuechanged.notify(
	    mCB(this,uiBodyFiller, updateFlds) );
    outsidevaluefld_ = new uiGenInput( this, "Outside-Value",
	    FloatInpSpec( showoutside ? mp->getOutsideValue() : 2000 ) );
    outsidevaluefld_->attach( alignedBelow, useoutsidefld_ );

    addNameFld( outsidevaluefld_ );
    updateFlds( 0 );
}


uiBodyFiller::~uiBodyFiller()
{
}


void uiBodyFiller::bodySel( CallBacker* )
{
    const IOObj* ioobj = uinputselfld_->ioobj();
    if ( ioobj )
	namefld_->setText( ioobj->name() );
}


void uiBodyFiller::updateFlds( CallBacker* )
{
    insidevaluefld_->display( useinsidefld_->getBoolValue() );
    outsidevaluefld_->display( useoutsidefld_->getBoolValue() );
}


uiStepDialog* uiBodyFiller::createInstance(uiParent* parent,Step* ps)
{
    mDynamicCastGet( BodyFiller*, mp, ps );
    if ( !mp ) return 0;

    return new uiBodyFiller( parent, mp );
}


bool uiBodyFiller::acceptOK( CallBacker* cb )
{
    if ( !uiStepDialog::acceptOK( cb ) )
	return false;

    const IOObj* ioobj = uinputselfld_->ioobj();
    if ( !ioobj ) return false;

    bodyfiller_->setSurface( ioobj->key() );

    if ( useinsidefld_->getBoolValue() && !useoutsidefld_->getBoolValue() )
    {
	if ( mIsUdf(insidevaluefld_->getfValue()) ) 
	{
	    uiMSG().error("Set the inside value");
	    return false;
	}

	bodyfiller_->setInsideOutsideValue( 
		insidevaluefld_->getfValue(), mUdf(float) );
    }

    if ( useoutsidefld_->getBoolValue() && !useinsidefld_->getBoolValue() )
    {
	if ( mIsUdf(outsidevaluefld_->getfValue()) ) 
	{
	    uiMSG().error("Set the outside value");
	    return false;
	}

	bodyfiller_->setInsideOutsideValue( 
		mUdf(float), outsidevaluefld_->getfValue() );
    }


    if ( useinsidefld_->getBoolValue() && useoutsidefld_->getBoolValue() )
    {
	if ( mIsUdf(insidevaluefld_->getfValue()) &&
      	     mIsUdf(outsidevaluefld_->getfValue()) ) 
	{
	    uiMSG().error("Set at lease one value");
	    return false;
	}

	bodyfiller_->setInsideOutsideValue( 
		insidevaluefld_->getfValue(), outsidevaluefld_->getfValue() );
    }

    if ( !useinsidefld_->getBoolValue() && !useoutsidefld_->getBoolValue() )
    {
	uiMSG().error("Select either Inside or Outside value to proceed.");
	return false;
    }

    return true;
}

} // namespace VolProc

