/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uivarwizard.cc,v 1.6 2012-05-22 08:51:39 cvsbert Exp $";

#include "uivarwizard.h"
#include "uivarwizarddlg.h"
#include "uiobjdisposer.h"


#define mSetState(st) { state_ = st; nextAction(); return; }
const char* uiVarWizardDlg::sProceedButTxt()	{ return "Next >&>"; }
const char* uiVarWizardDlg::sBackButTxt()	{ return "&<< Back"; }


uiVarWizard::uiVarWizard( uiParent* p )
    : state_(-1)
    , afterfinishedstate_(-1)
    , parent_(p)
    , processEnded(this)
{
}

void uiVarWizard::closeDown()
{
    uiOBJDISP()->go( this );
    processEnded.trigger();
}


void uiVarWizard::nextAction()
{
    if ( state_ <= cFinished() )
    {
	if ( state_ == cCancelled() || afterfinishedstate_ < 0 )
	    { closeDown(); return; }
	state_ = afterfinishedstate_;
    }

    if ( state_ != cWait4Dialog() )
	doPart();
}


bool uiVarWizard::mustLeave( uiVarWizardDlg* dlg )
{
    return dlg && dlg->leave_;
}


uiVarWizardDlg::uiVarWizardDlg( uiParent* p, const uiDialog::Setup& su,
			IOPar& pars, uiVarWizardDlg::Position pos )
    : uiDialog(p,uiDialog::Setup(su).okcancelrev(true))
    , pars_(pars)
    , pos_(pos)
{
    setModal( false );

    if ( pos_ != End )
	setOkText( sProceedButTxt() );
    if ( pos_ != Start )
	setCancelText( sBackButTxt() );
}


uiVarWizardDlg::~uiVarWizardDlg()
{
}

bool uiVarWizardDlg::rejectOK( CallBacker* )
{
    leave_ = !cancelpushed_;
    return true;
}
