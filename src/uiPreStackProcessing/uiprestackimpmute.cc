
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
Date:		June 2008
 RCS:		$Id: uiprestackimpmute.cc,v 1.6 2008-06-27 04:35:20 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uiprestackimpmute.h"

#include "prestackmutedeftransl.h"
#include "prestackmuteasciio.h"
#include "ctxtioobj.h"
#include "uiseparator.h"
#include "tabledef.h"
#include "uifileinput.h"
#include "oddirs.h"
#include "uitblimpexpdatasel.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "prestackmutedef.h"
#include "strmprov.h"
#include "uimsg.h"
#include "horsampling.h"
#include "survinfo.h"

namespace PreStack
{


uiImportMute::uiImportMute( uiParent* p )
    : uiDialog( p,uiDialog::Setup("Import Mute","Specify Parameters",0) )
    , ctio_( *mMkCtxtIOObj(MuteDef) )
    , fd_( *MuteAscIO::getDesc() )
{
    setCtrlStyle( DoAndStay );

    inpfld_ = new uiFileInput( this, "Input ASCII File", 
	                       uiFileInput::Setup().withexamine(true)
			       .defseldir(GetDataDir()) );

    inpfilehaveposfld_ = new uiGenInput( this, "File contains position",
	   				 BoolInpSpec(true) );
    inpfilehaveposfld_->attach( alignedBelow, inpfld_ );
    inpfilehaveposfld_->valuechanged.notify(
	      			mCB(this,uiImportMute,changePrefPosInfo) );
  
    inlcrlfld_ = new uiGenInput( this, "Inl/Crl", 
				 PositionInpSpec(PositionInpSpec::Setup()) );
    inlcrlfld_->attach( alignedBelow, inpfilehaveposfld_ );

    uiSeparator* sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, inlcrlfld_ );

    dataselfld_ = new uiTableImpDataSel( this, fd_, 0 );
    dataselfld_->attach( alignedBelow, inlcrlfld_ ); 
    dataselfld_->attach( ensureBelow, sep );

    sep = new uiSeparator( this, "H sep" );
    sep->attach( alignedBelow, dataselfld_ );

    ctio_.ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, ctio_, "Output Mute" );
    outfld_->attach( alignedBelow, dataselfld_ );
    outfld_->attach( ensureBelow, sep );

    finaliseDone.notify( mCB(this,uiImportMute,formatSel) );
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


#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiImportMute::acceptOK( CallBacker* )
{
    MuteDef mutedef;

    if ( !*inpfld_->fileName() )
	mErrRet( "Please select the input file" );

    StreamData sd = StreamProvider( inpfld_->fileName() ).makeIStream();
    if ( !sd.usable() )
	mErrRet( "Cannot open input file" )

    MuteAscIO muteascio( fd_, *sd.istrm );

    if ( haveInpPosData() )
    {
	if ( !muteascio.getMuteDef(mutedef) )
	    mErrRet( "Failed to convert into compatible data" );
    }
    else
    {
	HorSampling hs;

	if ( inlcrlfld_->getBinID() == BinID(mUdf(int),mUdf(int)) )
	    mErrRet( "Please enter Inl/Crl" )

	else if ( !hs.includes(inlcrlfld_->getBinID()) )
	    mErrRet( "Please enter Inl/Crl within survey range" )

	else if ( !muteascio.getMuteDef(mutedef, inlcrlfld_->getBinID()) )
	    mErrRet( "Failed to convert into compatible data" )
    }

    sd.close();
    
    if ( !outfld_->commitInput(true) )
	mErrRet( "Please select the output" )
		    
    MuteDefTranslator* tr = (MuteDefTranslator*)ctio_.ioobj->getTranslator();
    if ( !tr ) return false;

    BufferString bs;
    const bool retval = tr->store( mutedef, ctio_.ioobj, bs );
    if ( !retval )
	mErrRet( bs.buf() );

    uiMSG().message( "Import finished successfully" );
    return false;
}


bool uiImportMute::haveInpPosData() const
{
    return inpfilehaveposfld_->getBoolValue();
}

};// namespace PreStack
