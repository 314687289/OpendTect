/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiattrinpdlg.h"

#include "bufstringset.h"
#include "uisteeringsel.h"
#include "uitextedit.h"
#include "seistrctr.h"
#include "seisselection.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "uilabel.h"
#include "uimsg.h"
#include "keystrs.h"
#include "oddirs.h"
#include "perthreadrepos.h"
#include "od_helpids.h"


uiAttrInpDlg::uiAttrInpDlg( uiParent* p, const BufferStringSet& refset, 
			    bool issteer, bool is2d, const char* prevrefnm )
    : uiDialog(p,uiDialog::Setup(tr("Attribute set definition"),
		       issteer ? tr("Select Steering input")
			       : tr("Select Seismic input"),
				 mODHelpKey(mAttrInpDlgHelpID) ))
    , multiinpcube_(true)
    , is2d_(is2d)
    , seisinpfld_(0)
    , steerinpfld_(0)
{
    uiString infotxt = tr( "Provide input for the following attributes: " );
    uiLabel* infolbl = new uiLabel( this, infotxt );

    BufferString txt;
    for ( int idx=0; idx<refset.size(); idx++ )
	{ txt += refset.get(idx); txt += "\n"; }
    
    uiTextEdit* txtfld = new uiTextEdit( this, "File Info", true );
    txtfld->setPrefHeightInChar( 10 );
    txtfld->setPrefWidthInChar( 40 );
    txtfld->setText( txt );
    txtfld->attach( alignedBelow, infolbl );

    uiString seltext = issteer ? uiStrings::phrInput(tr("SteeringCube")) 
			       : uiStrings::phrInput(uiStrings::sSeismics());
    if ( prevrefnm )
    {
	seltext = tr("%1\n (replacing '%2')").arg(seltext)
					     .arg(toUiString(prevrefnm));
    }

    if ( issteer )
    {
	steerinpfld_ = new uiSteerCubeSel( this, is2d, true, seltext );
	steerinpfld_->attach( alignedBelow, txtfld );
    }
    else
    {
	uiSeisSel::Setup sssu( is2d, false );
	sssu.seltxt( seltext );
	const IOObjContext& ioctxt =
	    uiSeisSel::ioContext( is2d ? Seis::Line : Seis::Vol, true );
	seisinpfld_ = new uiSeisSel( this, ioctxt, sssu );
	seisinpfld_->attach( alignedBelow, txtfld );
    }

}


uiAttrInpDlg::uiAttrInpDlg( uiParent* p, bool hasseis, bool hassteer,
       			    bool is2d )
    : uiDialog(p,uiDialog::Setup(tr("Attribute set definition"),
	       hassteer ? (hasseis ? tr("Select Seismic & Steering input")
			: tr("Select Steering input")) 
                        : tr("Select Seismic input"),
			  mODHelpKey(mAttrInpDlgHelpID) ))
    , multiinpcube_(false)
    , is2d_(is2d)
    , seisinpfld_(0)
    , steerinpfld_(0)
{
    uiSeisSel::Setup sssu( is2d, false );
    if ( hasseis )
    {
	sssu.seltxt( uiStrings::phrInput(uiStrings::sSeismics()) );
	const IOObjContext& ioctxt =
	    uiSeisSel::ioContext( is2d ? Seis::Line : Seis::Vol, true );
	seisinpfld_ = new uiSeisSel( this, ioctxt, sssu );
    }

    if ( hassteer )
    {
	steerinpfld_ = new uiSteerCubeSel( this, is2d, true, uiStrings::phrInput
						     (uiStrings::sSteering()) );
	if ( hasseis )
	    steerinpfld_->attach( alignedBelow, seisinpfld_ );
    }
}


#define mErrRetSelInp() \
    { \
	uiMSG().error( errmsg ); \
	return false; \
    }


bool uiAttrInpDlg::acceptOK( CallBacker* )
{
    const uiString errmsg = tr("Please select the input for the attributes");

    if ( steerinpfld_ && !steerinpfld_->commitInput() && !multiinpcube_ )
	mErrRetSelInp();

    if ( seisinpfld_ && !seisinpfld_->commitInput() && !multiinpcube_ )
	mErrRetSelInp();

    return true;
}


uiAttrInpDlg::~uiAttrInpDlg()
{
}


const char* uiAttrInpDlg::getSeisRef() const
{
    return seisinpfld_ ? seisinpfld_->getInput() : 0;
}


const char* uiAttrInpDlg::getSteerRef() const
{
    return steerinpfld_ ? steerinpfld_->getInput() : 0;
}


const char* uiAttrInpDlg::getSeisKey() const
{
    LineKey lk;
    const IOObj* ioobj = seisinpfld_->ioobj( true );
    if ( !ioobj )
	return 0;

    lk.setLineName( ioobj->key() );
    mDeclStaticString( buf );
    buf = is2D() ? lk : lk.lineName();
    return buf;
}


const char* uiAttrInpDlg::getSteerKey() const
{
    static LineKey lk;
    const IOObj* ioobj = steerinpfld_->ioobj( true );
    if ( !ioobj )
	return 0;

    lk.setLineName( ioobj->key() );
    mDeclStaticString( buf );
    buf = is2D() ? lk : lk.lineName();
    return buf;
}

