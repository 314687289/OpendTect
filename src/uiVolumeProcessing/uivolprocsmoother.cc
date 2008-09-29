/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2008
-*/

static const char* rcsID = "$Id: uivolprocsmoother.cc,v 1.7 2008-09-29 19:25:23 cvskris Exp $";

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
    VolProc::uiChain::factory().addCreator( create, Smoother::sKeyType() );
}    


uiSmoother::uiSmoother( uiParent* p, Smoother* hf )
    : uiStepDialog( p, uiDialog::Setup( Smoother::sUserName(),
		    Smoother::sUserName(), mTODOHelpID ), hf )
    , smoother_( hf )
{
    operatorselfld_ = new uiWindowFunctionSel( this, "Operator",
	    		smoother_->getOperatorName(),
			smoother_->getOperatorParam() );
    operatorselfld_->attach( alignedBelow, namefld_ );

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
    zlabel += SI().getZUnit(true);

    zlenfld_ = new uiLabeledSpinBox( stepoutgroup, zlabel.buf(), 0,
	    			     "Z_spinbox" );
    zlenfld_->box()->setInterval( (float) 0, (mMaxNrSteps/2)*zstep, zstep );
    zlenfld_->box()->setValue( zstep*(smoother_->zSz()/2) );
    zlenfld_->attach( alignedBelow, crllenfld_ );

    stepoutgroup->setHAlignObj( zlenfld_ );
}


uiSmoother::~uiSmoother()
{
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

