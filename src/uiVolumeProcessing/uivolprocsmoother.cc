/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2008
-*/

static const char* rcsID = "$Id: uivolprocsmoother.cc,v 1.12 2011-03-23 14:47:28 cvsbruno Exp $";

#include "uivolprocsmoother.h"

#include "survinfo.h"
#include "uimsg.h"
#include "volprocsmoother.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uispinbox.h"
#include "uiwindowfunctionsel.h"
#include "uivolprocchain.h"
#include "volprocsmoother.h"


#define mMaxNrSteps	10

namespace VolProc
{


void uiSmoother::initClass()
{
    uiChain::factory().addCreator( create, Smoother::sKeyType() );
}    


uiSmoother::uiSmoother( uiParent* p, Smoother* hf )
    : uiStepDialog( p, Smoother::sUserName(), hf )
    , smoother_( hf )
{
    setHelpID( "103.6.4" );

    uiWindowFunctionSel::Setup su; su.label_= "Operator";
    su.winname_ = smoother_->getOperatorName(); 
    su.winparam_= smoother_->getOperatorParam();
    operatorselfld_ = new uiWindowFunctionSel( this, su );

    uiLabel* label = new uiLabel( this, "Stepout" );
    label->attach( alignedBelow, operatorselfld_ );

    uiGroup* stepoutgroup = new uiGroup( this, "Stepout" );
    stepoutgroup->setFrame( true );
    stepoutgroup->attach( alignedBelow, label );

    inllenfld_ = new uiLabeledSpinBox( stepoutgroup, "In-line", 0,
	    			  	"Inline_spinbox" );

    const BinID step( SI().inlStep(), SI().crlStep() );
    inllenfld_->box()->setInterval( 0, (mMaxNrSteps/2)*step.inl, step.inl );
    inllenfld_->box()->setValue( step.inl*(smoother_->inlSz()/2) );

    crllenfld_ = new uiLabeledSpinBox( stepoutgroup, "Cross-line", 0,
	    			       "Crline_spinbox" );
    crllenfld_->box()->setInterval( 0, (mMaxNrSteps/2)*step.crl, step.crl );
    crllenfld_->box()->setValue( step.crl*(smoother_->crlSz()/2) );
    crllenfld_->attach( alignedBelow, inllenfld_ );

    const float zstep = SI().zStep();
    BufferString zlabel = "Vertical ";
    zlabel += SI().getZUnitString(true);

    zlenfld_ = new uiLabeledSpinBox( stepoutgroup, zlabel.buf(), 0,
	    			     "Z_spinbox" );
    zlenfld_->box()->setInterval( (float) 0, (mMaxNrSteps/2)*zstep, zstep );
    zlenfld_->box()->setValue( zstep*(smoother_->zSz()/2) );
    zlenfld_->attach( alignedBelow, crllenfld_ );

    stepoutgroup->setHAlignObj( zlenfld_ );
    addNameFld( stepoutgroup );
}


uiStepDialog* uiSmoother::create( uiParent* parent, Step* ps )
{
    mDynamicCastGet( Smoother*, hf, ps );
    if ( !hf ) return 0;

    return new uiSmoother( parent, hf );
}


bool uiSmoother::acceptOK( CallBacker* cb )
{
    if ( !uiStepDialog::acceptOK( cb ) )
	return false;

    const int inlsz = mNINT(inllenfld_->box()->getFValue()/SI().inlStep() )*2+1;
    const int crlsz = mNINT(crllenfld_->box()->getFValue()/SI().crlStep() )*2+1;
    const int zsz = mNINT(zlenfld_->box()->getFValue()/SI().zStep() )*2+1;

    if ( !inlsz && !crlsz && !zsz )
    {
	uiMSG().error("At least one size must be non-zero" );
	return false;
    }

    if ( !smoother_->setOperator( operatorselfld_->windowName(),
				  operatorselfld_->windowParamValue(),
				  inlsz, crlsz, zsz ) )
    {
	uiMSG().error( "Cannot set selected operator" );
	return false;
    }

    return true;
}


};//namespace

