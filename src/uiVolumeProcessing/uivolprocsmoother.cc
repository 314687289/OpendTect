/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2008
-*/


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
#include "od_helpids.h"


#define mMaxNrSteps	10

namespace VolProc
{


uiSmoother::uiSmoother( uiParent* p, Smoother* hf, bool is2d )
    : uiStepDialog( p, Smoother::sFactoryDisplayName(), hf, is2d )
    , smoother_( hf )
    , inllenfld_( 0 )
{
    setHelpKey( mODHelpKey(mVolumeSmootherHelpID) );

    uiWindowFunctionSel::Setup su; su.label_= "Operator";
    su.winname_ = smoother_->getOperatorName();
    su.winparam_= smoother_->getOperatorParam();
    operatorselfld_ = new uiWindowFunctionSel( this, su );

    uiLabel* label = new uiLabel( this, uiStrings::sStepout() );
    label->attach( alignedBelow, operatorselfld_ );

    uiGroup* stepoutgroup = new uiGroup( this, "Stepout" );
    stepoutgroup->setFrame( true );
    stepoutgroup->attach( alignedBelow, label );

    const BinID step( SI().inlStep(), SI().crlStep() );
    if ( !is2d )
    {
	inllenfld_ = new uiLabeledSpinBox(
		stepoutgroup, uiStrings::sInline(), 0, "Inline_spinbox" );

	inllenfld_->box()->setInterval( 0, (mMaxNrSteps/2)*step.inl(),
					step.inl() );
	inllenfld_->box()->setValue( step.inl()*(smoother_->inlSz()/2) );
    }

    crllenfld_ =
	new uiLabeledSpinBox( stepoutgroup, is2d ? uiStrings::sTraceNumber()
						 : uiStrings::sCrossline(), 0,
			      "Crline_spinbox" );
    crllenfld_->box()->setInterval( 0, (mMaxNrSteps/2)*step.crl(), step.crl() );
    crllenfld_->box()->setValue( step.crl()*(smoother_->crlSz()/2) );
    if ( inllenfld_ )
	crllenfld_->attach( alignedBelow, inllenfld_ );

    const float zstep = SI().zStep() * SI().zDomain().userFactor();
    uiString zlabel = tr("Vertical %1").arg( SI().zUnitString(true) );

    zlenfld_ = new uiLabeledSpinBox( stepoutgroup, zlabel, 0,
				     "Z_spinbox" );
    zlenfld_->box()->setInterval( (float) 0, (mMaxNrSteps/2)*zstep, zstep );
    zlenfld_->box()->setValue( zstep*(smoother_->zSz()/2) );
    zlenfld_->attach( alignedBelow, crllenfld_ );

    stepoutgroup->setHAlignObj( zlenfld_ );
    addNameFld( stepoutgroup );
}


uiStepDialog* uiSmoother::createInstance( uiParent* parent, Step* ps,
					  bool is2d )
{
    mDynamicCastGet( Smoother*, hf, ps );
    if ( !hf ) return 0;

    return new uiSmoother( parent, hf, is2d );
}


bool uiSmoother::acceptOK()
{
    if ( !uiStepDialog::acceptOK() )
	return false;

    const float zstep = SI().zStep() * SI().zDomain().userFactor();
    const int inlsz =
	is2d_ ? 1 : mNINT32(inllenfld_->box()->getFValue()/SI().inlStep())*2+1;
    const int crlsz =
	mNINT32(crllenfld_->box()->getFValue()/SI().crlStep())*2+1;
    const int zsz =
	mNINT32(zlenfld_->box()->getFValue()/zstep)*2+1;

    if ( !inlsz && !crlsz && !zsz )
    {
	uiMSG().error(tr("At least one size must be non-zero") );
	return false;
    }

    if ( !smoother_->setOperator( operatorselfld_->windowName(),
				  operatorselfld_->windowParamValue(),
				  inlsz, crlsz, zsz ) )
    {
	uiMSG().error( tr("Cannot set selected operator") );
	return false;
    }

    return true;
}

} // namespace VolProc
