
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : Mar 2007
-*/

static const char* rcsID = "$Id: uitutseistools.cc,v 1.9 2007-06-08 06:19:16 cvsraman Exp $";

#include "uitutseistools.h"
#include "tutseistools.h"
#include "uiseissel.h"
#include "uigeninput.h"
#include "uiexecutor.h"
#include "uimsg.h"
#include "seistrctr.h"
#include "seistrcsel.h"
#include "ctxtioobj.h"
#include "ioobj.h"

static const char* actions[] = { "Scale", "Square", "Smooth", 0 };
// Exactly the order of the Tut::SeisTools::Action enum

uiTutSeisTools::uiTutSeisTools( uiParent* p )
	: uiDialog( p, Setup( "Tut seismic tools",
			      "Specify process parameters",
			      "tut:105.0.1") )
    	, inctio_(*mMkCtxtIOObj(SeisTrc))
    	, outctio_(*mMkCtxtIOObj(SeisTrc))
    	, tst_(*new Tut::SeisTools)
{
    const CallBack choicecb( mCB(this,uiTutSeisTools,choiceSel) );

    // The input seismic object
    inpfld_ = new uiSeisSel( this, inctio_, SeisSelSetup() );

    // What seismic tool is required?
    actionfld_ = new uiGenInput( this, "Action",
	    			 StringListInpSpec(actions) );
    actionfld_->valuechanged.notify( choicecb );
    actionfld_->attach( alignedBelow, inpfld_ );

    // Parameters for scaling
    scalegrp_ = new uiGroup( this, "Scale group" );
    scalegrp_->attach( alignedBelow, actionfld_ );
    factorfld_ = new uiGenInput( scalegrp_, "Factor",
				FloatInpSpec(tst_.factor()) );
    shiftfld_ = new uiGenInput( scalegrp_, "Shift",
	    			FloatInpSpec(tst_.shift()) );
    shiftfld_->attach( alignedBelow, factorfld_ );
    scalegrp_->setHAlignObj( factorfld_ );

    // Parameters for smoothing
    smoothszfld_ = new uiGenInput( this, "Filter strength",
			       BoolInpSpec(tst_.weakSmoothing(),"Low","High") );
    smoothszfld_->attach( alignedBelow, actionfld_ );

    // The output seismic object
    outctio_.ctxt.forread = false;
    outfld_ = new uiSeisSel( this, outctio_, SeisSelSetup() );
    outfld_->attach( alignedBelow, scalegrp_ );

    // Make sure only relevant stuff is displayed on startup
    finaliseDone.notify( choicecb );
}


uiTutSeisTools::~uiTutSeisTools()
{
    delete inctio_.ioobj; delete &inctio_;
    delete outctio_.ioobj; delete &outctio_;
    delete &tst_;
}


void uiTutSeisTools::choiceSel( CallBacker* )
{
    const Tut::SeisTools::Action act
			= (Tut::SeisTools::Action)actionfld_->getIntValue();

    scalegrp_->display( act == Tut::SeisTools::Scale );
    smoothszfld_->display( act == Tut::SeisTools::Smooth );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiTutSeisTools::acceptOK( CallBacker* )
{
    // Get cubes and check
    if ( !inpfld_->commitInput(false) )
	mErrRet("Missing Input\nPlease select the input seismics")
    if ( !outfld_->commitInput(true) )
	mErrRet("Missing Output\nPlease enter a name for the output seismics")
    else if ( outctio_.ioobj->implExists(false)
	   && !uiMSG().askGoOn("Output cube exists. Overwrite?") )
	return false;

    tst_.clear();
    tst_.setInput( *inctio_.ioobj );
    tst_.setOutput( *outctio_.ioobj );

    // Set action-specific parameters
    tst_.setAction( (Tut::SeisTools::Action)actionfld_->getIntValue() );
    switch ( tst_.action() )
    {
    case Tut::SeisTools::Smooth:
	tst_.setWeakSmoothing( smoothszfld_->getBoolValue() );
    break;
    case Tut::SeisTools::Scale:
    {
	float usrfactor = factorfld_->getfValue();
	if ( mIsUdf(usrfactor) ) usrfactor = 1;
	float usrshift = shiftfld_->getfValue();
	if ( mIsUdf(usrshift) ) usrshift = 0;
	tst_.setScale( usrfactor, usrshift );
    }
    break;
    default: // No parameters to set
    break;
    }

    uiExecutor dlg( this, tst_ );
    return dlg.go();
}
