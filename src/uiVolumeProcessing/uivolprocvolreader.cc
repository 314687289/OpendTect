/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID = "$Id: uivolprocvolreader.cc,v 1.7 2011-03-30 14:31:06 cvsyuancheng Exp $";

#include "uivolprocvolreader.h"
#include "uimsg.h"
#include "volprocvolreader.h"

#include "ctxtioobj.h"
#include "seistrctr.h"
#include "mousecursor.h"
#include "uigeninput.h"
#include "uiseissel.h"
#include "uivolprocchain.h"


namespace VolProc
{


void uiVolumeReader::initClass()
{
    uiChain::factory().addCreator(create, VolumeReader::sKeyType(),
	   VolumeReader::sUserName() );
}    


uiVolumeReader::uiVolumeReader( uiParent* p, VolumeReader* vr )
    : uiStepDialog( p, VolumeReader::sUserName(), vr )
    , volumereader_( vr )
    , ctio_(uiSeisSel::mkCtxtIOObj(Seis::Vol,true))
{
    setHelpID( "103.6.8" );

    if ( vr )
	ctio_->setObj( vr->getVolumeID() );
    else
	ctio_->setObj( 0 );

    seissel_ = new uiSeisSel( this, *ctio_, uiSeisSel::Setup(false,false) );
    seissel_->selectionDone.notify( mCB(this,uiVolumeReader,volSel) );

    addNameFld( seissel_ );
    finaliseDone.notify( mCB(this,uiVolumeReader,volSel) );
}


uiVolumeReader::~uiVolumeReader()
{
    delete ctio_->ioobj; delete ctio_;
}


void uiVolumeReader::volSel( CallBacker* )
{
    if ( !*namefld_->text() )
    {
	seissel_->processInput();
	const IOObj* ioobj = seissel_->ctxtIOObj(true).ioobj;
	if ( ioobj )
	    namefld_->setText( ioobj->name() );
    }
}


uiStepDialog* uiVolumeReader::create( uiParent* parent, Step* ps )
{
    mDynamicCastGet( VolumeReader*, vr, ps );
    if ( !vr ) return 0;

    return new uiVolumeReader( parent, vr );
}


bool uiVolumeReader::acceptOK( CallBacker* cb )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    if ( !uiStepDialog::acceptOK( cb ) )
	return false;

    if ( !seissel_->commitInput() )
    {
	uiMSG().error("Please selectthe input velocity volume");
	return false;
    }

    if ( !volumereader_->setVolumeID( ctio_->ioobj->key() ) )
    {
	uiMSG().error("Cannot use selected volume" );
	return false;
    }

    return true;
}


};//namespace

