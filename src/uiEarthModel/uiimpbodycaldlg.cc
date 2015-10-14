/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2008
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiimpbodycaldlg.h"

#include "bodyvolumecalc.h"
#include "embody.h"
#include "executor.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "veldesc.h"
#include "od_helpids.h"


uiImplBodyCalDlg::uiImplBodyCalDlg( uiParent* p, const EM::Body& eb )
    : uiDialog(p,Setup(tr("Calculate volume"),tr("Body volume estimation"),
                        mODHelpKey(mImplBodyCalDlgHelpID) ))
    , embody_(eb)  
    , velfld_(0)
    , volfld_(0)
    , impbody_(0)  
{
    setCtrlStyle( CloseOnly );
    
    if ( SI().zIsTime() )
    {
	velfld_ = new uiGenInput( this, 
			 VelocityDesc::getVelVolumeLabel(),
			 FloatInpSpec( SI().depthsInFeet()?10000.0f:3000.0f) );
    }
    
    volfld_ = new uiGenInput( this, uiStrings::sVolume() );
    volfld_->setReadOnly( true );
    if ( velfld_ )
	volfld_->attach( alignedBelow, velfld_ );
    
    uiPushButton* calcbut = new uiPushButton( this, tr("Estimate"), true );
    calcbut->activated.notify( mCB(this,uiImplBodyCalDlg,calcCB) );
    calcbut->attach( rightTo, volfld_ );
}


uiImplBodyCalDlg::~uiImplBodyCalDlg()
{ delete impbody_; }


#define mErrRet(s) { uiMSG().error(s); return; }

void uiImplBodyCalDlg::calcCB( CallBacker* )
{
    if ( !impbody_ )
    {
	getImpBody();
    	if ( !impbody_ || !impbody_->arr_ )
    	    mErrRet(tr("Checking body failed"));
    }

    float vel = 1;
    if ( velfld_ )
    {
	vel = velfld_->getFValue();
	if ( mIsUdf(vel) || vel < 0.1 )
	    mErrRet(tr("Please provide the velocity"))
	if ( SI().depthsInFeet() )
	    vel *= mFromFeetFactorF;
    }

    uiTaskRunner taskrunner(this);
    BodyVolumeCalculator bc( impbody_->tkzs_, *impbody_->arr_,
	    impbody_->threshold_, vel );
    TaskRunner::execute( &taskrunner, bc );

    BufferString txt;
    txt += bc.getVolume();
    txt += "m^3";
    volfld_->setText( txt.buf() );
}


void uiImplBodyCalDlg::getImpBody()
{
    delete impbody_;

    uiTaskRunner taskrunner(this);
    impbody_ = embody_.createImplicitBody(&taskrunner,false);
}

