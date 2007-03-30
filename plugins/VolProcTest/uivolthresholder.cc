/*+
 * COPYRIGHT	(C) dGB Beheer B.V.
 * AUTHOR	Y.C. Liu
 * DATE		March 2007
-*/

static const char* rcsID = "$Id: uivolthresholder.cc,v 1.1 2007-03-30 21:00:56 cvsyuancheng Exp $";

#include "uivolthresholder.h"

#include "uivolumeprocessing.h"
#include "volumeprocessing.h"
#include "volprocthresholder.h"
#include "uigeninput.h"

namespace VolProc
{

void uiVolumeThresholder::initClass()
{
    VolProc::uiPS().addCreator( create, ThresholdStep::sKeyType() );
}    


uiVolumeThresholder::uiVolumeThresholder( uiParent* p, ThresholdStep* ts )
	: uiDialog( p, uiDialog::Setup( "Volume Thresholder", 0, "helpid" ) )
	, thresholdstep_( ts )
{   
    thresholdfld_ = new uiGenInput( this, "Threshold",
				    FloatInpSpec(ts->getThreshold()) );
}


uiDialog* uiVolumeThresholder::create( uiParent* parent, ProcessingStep* ps )
{
    mDynamicCastGet( ThresholdStep*, ts, ps );
    if ( !ts ) return 0;

    return new uiVolumeThresholder( parent, ts );
}


bool uiVolumeThresholder::acceptOK( CallBacker* )
{
    if ( thresholdfld_->isUndef( 0 ) )
	return false;

    thresholdstep_->setThreshold( thresholdfld_->getfValue() );
    return true;
}

} //namespace
