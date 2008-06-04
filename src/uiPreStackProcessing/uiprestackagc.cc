/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uiprestackagc.cc,v 1.5 2008-06-04 09:12:33 cvsbert Exp $";

#include "uiprestackagc.h"

#include "uiprestackprocessor.h"
#include "prestackagc.h"
#include "uigeninput.h"
#include "uimsg.h"

namespace PreStack
{

void uiAGC::initClass()
{
    uiPSPD().addCreator( create, AGC::sName() );
}


uiDialog* uiAGC::create( uiParent* p, Processor* sgp )
{
    mDynamicCastGet( AGC*, sgagc, sgp );
    if ( !sgagc ) return 0;

    return new uiAGC( p, sgagc );
}


uiAGC::uiAGC( uiParent* p, AGC* sgagc )
    : uiDialog( p, uiDialog::Setup("AGC setup",0,"103.2.1") )
    , processor_( sgagc )
{
    BufferString label = "Window width ";
    BufferString unit;
    processor_->getWindowUnit( unit, true );
    label += unit;
    windowfld_ = new uiGenInput( this, label.buf(),
			     FloatInpSpec(processor_->getWindow().width() ));
    lowenergymute_ = new uiGenInput( this, "Low energy mute (%)",
	    			     FloatInpSpec() );
    lowenergymute_->attach( alignedBelow, windowfld_ );
    const float lowenergymute = processor_->getLowEnergyMute();
    lowenergymute_->setValue(
	    mIsUdf(lowenergymute) ? mUdf(float) : lowenergymute*100 );
}


bool uiAGC::acceptOK( CallBacker* )
{
    if ( !processor_ ) return true;

    const float width = windowfld_->getfValue();
    if ( mIsUdf(width) )
    {
	uiMSG().error("Window width is not set");
	return false;
    }

    processor_->setWindow( Interval<float>( -width/2, width/2 ) );
    const float lowenerymute = lowenergymute_->getfValue();
    if ( mIsUdf(lowenerymute) ) processor_->setLowEnergyMute( mUdf(float) );
    else
    {
	if ( lowenerymute<0 || lowenerymute>99 )
	{
	    uiMSG().error("Low energy mute must be between 0 and 99");
	    return false;
	}

	processor_->setLowEnergyMute( lowenerymute*100 );
    }

    return true;
}



}; //namespace
