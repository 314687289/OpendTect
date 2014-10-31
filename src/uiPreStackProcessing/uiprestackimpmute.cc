/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
Date:		June 2008
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiprestackimpmute.h"

#include "uifileinput.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uitblimpexpdatasel.h"

#include "ctxtioobj.h"
#include "trckeysampling.h"
#include "oddirs.h"
#include "od_istream.h"
#include "prestackmuteasciio.h"
#include "prestackmutedef.h"
#include "prestackmutedeftransl.h"
#include "survinfo.h"
#include "tabledef.h"
#include "od_helpids.h"


namespace PreStack
{

uiImportMute::uiImportMute( uiParent* p )
    : uiDialog( p,uiDialog::Setup(tr("Import Mute Function"),mNoDlgTitle,
                                  mODHelpKey(mPreStackImportMuteHelpID) ))
    , ctio_( *mMkCtxtIOObj(MuteDef) )
    , fd_( *MuteAscIO::getDesc() )
{
    setOkText( uiStrings::sImport() );

    inpfld_ = new uiFileInput( this, "Input ASCII File",
	                       uiFileInput::Setup().withexamine(true)
			       .defseldir(GetDataDir()) );

    inpfilehaveposfld_ = new uiGenInput( this, tr("File contains position"),
					 BoolInpSpec(true) );
    inpfilehaveposfld_->attach( alignedBelow, inpfld_ );
    inpfilehaveposfld_->valuechanged.notify(
				mCB(this,uiImportMute,changePrefPosInfo) );

    inlcrlfld_ = new uiGenInput( this, tr("Inl/Crl"),
				 PositionInpSpec(PositionInpSpec::Setup()) );
    inlcrlfld_->attach( alignedBelow, inpfilehaveposfld_ );

    dataselfld_ = new uiTableImpDataSel( this, fd_, 
                      mODHelpKey(mPreStackImportMuteParsHelpID) );
    dataselfld_->attach( alignedBelow, inlcrlfld_ );

    ctio_.ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, ctio_, tr("Mute Definition") );
    outfld_->attach( alignedBelow, dataselfld_ );

    postFinalise().notify( mCB(this,uiImportMute,formatSel) );
}


uiImportMute::~uiImportMute()
{
    delete ctio_.ioobj; delete &ctio_;
    delete &fd_;
}


void uiImportMute::formatSel( CallBacker* )
{
    inlcrlfld_->display( !haveInpPosData() );
    MuteAscIO::updateDesc( fd_, haveInpPosData() );
}


void uiImportMute::changePrefPosInfo( CallBacker* cb )
{
    BinID center( SI().inlRange(false).center(),
		  SI().crlRange(false).center() );
    SI().snap( center );
    inlcrlfld_->setValue( center );

    formatSel( cb );
}


#define mErrRet(msg) { if ( msg ) uiMSG().error( msg ); return false; }

bool uiImportMute::acceptOK( CallBacker* )
{
    MuteDef mutedef;

    if ( !*inpfld_->fileName() )
	mErrRet( "Please select the input file" );

    od_istream strm( inpfld_->fileName() );
    if ( !strm.isOK() )
	mErrRet( "Cannot open input file" )

    MuteAscIO muteascio( fd_, strm );

    if ( haveInpPosData() )
    {
	if ( !muteascio.getMuteDef(mutedef) )
	    mErrRet( "Failed to convert into compatible data" );
    }
    else
    {
	TrcKeySampling hs;

	if ( inlcrlfld_->getBinID() == BinID(mUdf(int),mUdf(int)) )
	    mErrRet( "Please enter Inl/Crl" )

	else if ( !hs.includes(inlcrlfld_->getBinID()) )
	    mErrRet( "Please enter Inl/Crl within survey range" )

	else if ( !muteascio.getMuteDef(mutedef, inlcrlfld_->getBinID()) )
	    mErrRet( "Failed to convert into compatible data" )
    }

    if ( !outfld_->commitInput() )
	mErrRet( outfld_->isEmpty() ? "Please select the output" : 0 )

   PtrMan<MuteDefTranslator> trans =
	    (MuteDefTranslator*)ctio_.ioobj->createTranslator();
    if ( !trans ) return false;

    uiString str;
    const bool retval = trans->store( mutedef, ctio_.ioobj, str );
    if ( !retval )
    {
	if ( str.isSet() )
	    uiMSG().error( str );
	return false;
	//TODO: integrate to mErrRet when all messages are uiString
	//should be part of another revision
    }

    uiMSG().message( tr("Import finished successfully") );
    return false;
}


bool uiImportMute::haveInpPosData() const
{
    return inpfilehaveposfld_->getBoolValue();
}

};// namespace PreStack
