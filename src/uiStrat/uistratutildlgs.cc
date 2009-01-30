/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          August 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratutildlgs.cc,v 1.13 2009-01-30 16:04:23 cvshelene Exp $";

#include "uistratutildlgs.h"

#include "randcolor.h"
#include "stratlith.h"
#include "stratunitrepos.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistratmgr.h"

static const char* sNoLithoTxt      = "---None---";


uiStratUnitDlg::uiStratUnitDlg( uiParent* p, uiStratMgr* uistratmgr )
    : uiDialog(p,uiDialog::Setup("Stratigraphic Unit Properties",
				 "Specify properties of a new unit",
				 mTODOHelpID))
    , uistratmgr_(uistratmgr)
{
    unitnmfld_ = new uiGenInput( this, "Name", StringInpSpec() );
    unitdescfld_ = new uiGenInput( this, "Description", StringInpSpec() );
    unitdescfld_->attach( alignedBelow, unitnmfld_ );
    unitlithfld_ = new uiGenInput( this, "Lithology", StringInpSpec() );
    unitlithfld_->attach( alignedBelow, unitdescfld_ );
    CallBack cb = mCB(this,uiStratUnitDlg,selLithCB);
    uiPushButton* sellithbut = new uiPushButton( this, "&Select", cb, false );
    sellithbut->attach( rightTo, unitlithfld_ );
}


void uiStratUnitDlg::selLithCB( CallBacker* )
{
    uiStratLithoDlg lithdlg( this, uistratmgr_ );
    if ( lithdlg.go() )
	unitlithfld_->setText( lithdlg.getLithName() );
} 


const char* uiStratUnitDlg::getUnitName() const
{
    return unitnmfld_->text();
}


const char* uiStratUnitDlg::getUnitDesc() const
{
    return unitdescfld_->text();
}


const char* uiStratUnitDlg::getUnitLith() const
{
    const char* txt = unitlithfld_->text();
    return !strcmp( txt, sNoLithoTxt ) ? 0 : txt;
}


bool uiStratUnitDlg::acceptOK( CallBacker* )
{
    if ( !strcmp( getUnitName(), "" ) )
	{ uiMSG().error( "Please specify the unit name" ); return false; }
    return true;
}


void uiStratUnitDlg::setUnitName( const char* unitnm )
{
    unitnmfld_->setText( unitnm );
    //TODO: rename unit needs extra work to update all the paths to the subunits
    unitnmfld_->setSensitive(false);
}


void uiStratUnitDlg::setUnitDesc( const char* unitdesc )
{
    unitdescfld_->setText( unitdesc );
}


void uiStratUnitDlg::setUnitLith( const char* unitlithnm )
{
    unitlithfld_->setText( unitlithnm );
}


void uiStratUnitDlg::setUnitIsLeaf( bool yn )
{
    unitlithfld_->setSensitive( yn );
}


uiStratLithoDlg::uiStratLithoDlg( uiParent* p, uiStratMgr* uistratmgr )
    : uiDialog(p,uiDialog::Setup("Select Lithology",mNoDlgTitle,mTODOHelpID))
    , uistratmgr_(uistratmgr)
    , prevlith_(0)
    , nmfld_(0)
{
    selfld_ = new uiListBox( this, "Lithology", false );
    const CallBack cb( mCB(this,uiStratLithoDlg,selChg) );
    selfld_->selectionChanged.notify( cb );
    fillLiths();

    uiGroup* rightgrp = new uiGroup( this, "right group" );
    nmfld_ = new uiGenInput( rightgrp, "Name", StringInpSpec() );
    isporbox_ = new uiCheckBox( rightgrp, "Porous" );
    isporbox_->attach( alignedBelow, nmfld_ );
    uiPushButton* newlithbut = new uiPushButton( rightgrp, "&Add as new",
	    		mCB(this,uiStratLithoDlg,newLith), true );
    newlithbut->attach( alignedBelow, isporbox_ );

    uiSeparator* sep = new uiSeparator( this, "Sep", false );
    sep->attach( rightTo, selfld_ );
    sep->attach( heightSameAs, selfld_ );
    rightgrp->attach( rightTo, sep );

    uiButton* rmbut = new uiPushButton( this, "&Remove selected",
	    				mCB(this,uiStratLithoDlg,rmSel), true );
    rmbut->attach( alignedBelow, rightgrp );

    finaliseDone.notify( cb );
}


void uiStratLithoDlg::fillLiths()
{
    BufferStringSet nms;
    nms.add( sNoLithoTxt );
    uistratmgr_->getLithoNames( nms );
    selfld_->empty();
    selfld_->addItems( nms );
}
    


void uiStratLithoDlg::newLith( CallBacker* )
{
    const BufferString nm( nmfld_->text() );
    if ( nm.isEmpty() ) return;

    if ( selfld_->isPresent( nm ) )
	{ uiMSG().error( "Please specify a new, unique name" ); return; }

    uistratmgr_->createNewLith( nm, isporbox_->isChecked() );

    selfld_->addItem( nm );
    selfld_->setCurrentItem( nm );
}


void uiStratLithoDlg::selChg( CallBacker* )
{
    if ( !nmfld_ ) return;

    if ( prevlith_ )
    {
	BufferString newnm( nmfld_->text() );
	const bool newpor = isporbox_->isChecked();
	if ( newnm != prevlith_->name() || newpor != prevlith_->porous_ )
	{
	    if ( !prevlith_->isUdf() )
	    {
		prevlith_->setName( nmfld_->text() );
		prevlith_->porous_ = isporbox_->isChecked();
		uistratmgr_->lithChanged.trigger();
	    }
	}
    }
    const BufferString nm( selfld_->getText() );
    const Strat::Lithology* lith = uistratmgr_->getLith( nm );
    if ( !lith ) lith = &Strat::Lithology::undef();
    nmfld_->setText( lith->name() );
    isporbox_->setChecked( lith->porous_ );
    prevlith_ = const_cast<Strat::Lithology*>( lith );
}


void uiStratLithoDlg::rmSel( CallBacker* )
{
    int selidx = selfld_->currentItem();
    if ( selidx < 0 ) return;

    const Strat::Lithology* lith =
			uistratmgr_->getLith( selfld_->textOfItem(selidx) );
    if ( !lith || lith->isUdf() ) return;

    prevlith_ = 0;
    uistratmgr_->deleteLith( lith->id_ );
    fillLiths();

    if ( selidx >= selfld_->size() )
	selidx = selfld_->size() - 1;

    if ( selidx < 0 )
	nmfld_->setText( "" );
    else
    {
	selfld_->setCurrentItem( selidx );
	selChg( 0 );
    }
}


const char* uiStratLithoDlg::getLithName() const
{
    const char* txt = selfld_->getText();
    return !strcmp( txt, sNoLithoTxt ) ? 0 : txt;
}


void uiStratLithoDlg::setSelectedLith( const char* lithnm )
{
    const Strat::Lithology* lith = uistratmgr_->getLith( lithnm );
    if ( !lith ) return;
    selfld_->setCurrentItem( lithnm );
}


bool uiStratLithoDlg::acceptOK( CallBacker* )
{
    selChg( 0 );
    return true;
}


uiStratLevelDlg::uiStratLevelDlg( uiParent* p, uiStratMgr* uistratmgr )
    : uiDialog(p,uiDialog::Setup("Create/Edit level",mNoDlgTitle,mTODOHelpID))
    , uistratmgr_( uistratmgr )
{
    lvlnmfld_ = new uiGenInput( this, "Name", StringInpSpec() );
    lvlcolfld_ = new uiColorInput( this,
			           uiColorInput::Setup(getRandStdDrawColor() ).
				   lbltxt("Color") );
    lvlcolfld_->attach( alignedBelow, lvlnmfld_ );
    lvltvstrgfld_ = new uiGenInput( this, "This level is ",
	    			    BoolInpSpec(true,"Isochron","Diachron") );
    lvltvstrgfld_->attach( alignedBelow, lvlcolfld_ );
    lvltvstrgfld_->valuechanged.notify( mCB(this,uiStratLevelDlg,isoDiaSel) );
    lvltimefld_ = new uiGenInput( this, "Level time (My)", FloatInpSpec() );
    lvltimefld_->attach( alignedBelow, lvltvstrgfld_ );
    lvltimergfld_ = new uiGenInput( this, "Level time range (My)",
	    			    FloatInpIntervalSpec() );
    lvltimergfld_->attach( alignedBelow, lvltvstrgfld_ );
    isoDiaSel(0);
}


void uiStratLevelDlg::isoDiaSel( CallBacker* )
{
    bool isiso = lvltvstrgfld_->getBoolValue();
    lvltimefld_->display( isiso );
    lvltimergfld_->display( !isiso );
}


void uiStratLevelDlg::setLvlInfo( const char* lvlnm )
{
    Interval<float> lvltrg;
    Color lvlcol;
    if ( !lvlnm || !*lvlnm || !uistratmgr_->getLvlPars(lvlnm,lvltrg,lvlcol) )
	return;

    lvlnmfld_->setText( lvlnm );
    bool isiso = mIsUdf(lvltrg.stop);
    lvltvstrgfld_->setValue( isiso );
    if ( isiso && !mIsUdf(lvltrg.start) )
	lvltimefld_->setValue( lvltrg.start );
    else if ( !isiso )
	lvltimergfld_->setValue( lvltrg );

    lvlcolfld_->setColor( lvlcol );
    oldlvlnm_ = lvlnm;
}


bool uiStratLevelDlg::acceptOK( CallBacker* )
{
    BufferString newlvlnm = lvlnmfld_->text();
    Color newlvlcol = lvlcolfld_->color();
    Interval<float> newlvlrg;
    if ( lvltvstrgfld_->getBoolValue() )
    {
	newlvlrg.start = lvltimefld_->getfValue();
	newlvlrg.stop = lvltimefld_->getfValue();
    }
    else
	newlvlrg = lvltimergfld_->getFInterval();
    
    uistratmgr_->setLvlPars( oldlvlnm_, newlvlnm, newlvlcol, newlvlrg );
    return true;
}


#define mCreateList(loc,str)\
{\
    BufferString loc##bs = "Select "; loc##bs += str; loc##bs += " level";\
    lvl##loc##listfld_ = new uiGenInput( this, loc##bs,\
	    				 StringListInpSpec( lvlnms ) );\
    lvl##loc##listfld_->setWithCheck();\
    lvl##loc##listfld_->setChecked( loc##idx>-1 );\
    if ( loc##idx>-1 )\
	lvl##loc##listfld_->setValue( loc##idx );\
}

uiStratLinkLvlUnitDlg::uiStratLinkLvlUnitDlg( uiParent* p, const char* urcode,
       					      uiStratMgr* uistratmgr )
    : uiDialog(p,uiDialog::Setup("Link levels and stratigraphic unit",
				mNoDlgTitle,mTODOHelpID))
    , uistratmgr_(uistratmgr)
{
    BufferStringSet lvlnms;
    TypeSet<Color> colors; int topidx, baseidx;
    uistratmgr_->getLvlsTxtAndCol( lvlnms, colors );
    uistratmgr_->getIdxTBLvls( urcode, topidx, baseidx );
    mCreateList(top,"top")
    mCreateList(base,"base")
    lvlbaselistfld_->attach( alignedBelow, lvltoplistfld_ );
}


bool uiStratLinkLvlUnitDlg::acceptOK( CallBacker* )
{
    bool hastoplvl = lvltoplistfld_->isChecked();
    bool hasbaselvl = lvlbaselistfld_->isChecked();
    if ( hastoplvl && hasbaselvl )
    {
	BufferString msg = uistratmgr_->checkLevelsOk( lvltoplistfld_->text(),
						       lvlbaselistfld_->text());
	if ( !msg.isEmpty() ) 
	    { uiMSG().error( msg ); return false; }
    }

    return true;
}

