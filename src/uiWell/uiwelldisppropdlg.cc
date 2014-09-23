/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Nov 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiwelldisppropdlg.h"

#include "uiwelldispprop.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uiobjdisposer.h"
#include "uitabstack.h"
#include "uiseparator.h"

#include "keystrs.h"
#include "welldata.h"
#include "welldisp.h"
#include "wellmarker.h"
#include "od_helpids.h"

#define mDispNot (is2ddisplay_? wd_->disp2dparschanged : wd_->disp3dparschanged)
#define mDelNot wd_->tobedeleted

uiWellDispPropDlg::uiWellDispPropDlg( uiParent* p, Well::Data* d, bool is2d )
	: uiDialog(p,uiDialog::Setup("Well display properties",
	   "", mODHelpKey(mWellDispPropDlgHelpID) ).savetext(sSaveAsDefault())
           .savebutton(true)
					.savechecked(false)
					.modal(false))
	, wd_(d)
	, applyAllReq(this)
	, savedefault_(false)
	, is2ddisplay_(is2d)
{
    setCtrlStyle( CloseOnly );

    Well::DisplayProperties& props = d->displayProperties( is2ddisplay_ );

    ts_ = new uiTabStack( this, "Well display properties tab stack" );
    ObjectSet<uiGroup> tgs;
    tgs += new uiGroup( ts_->tabGroup(),"Left log properties" );
    tgs += new uiGroup( ts_->tabGroup(),"Right log properties" );
    tgs += new uiGroup( ts_->tabGroup(), "Marker properties" );
    if ( !is2d )
	tgs += new uiGroup( ts_->tabGroup(), "Track properties" );

    uiWellLogDispProperties* wlp1 = new uiWellLogDispProperties( tgs[0],
		uiWellDispProperties::Setup( "Line thickness", "Line color"),
		props.logs_[0]->left_, &(wd_->logs()) );
    wlp1->disableLogWidth( is2d );
    uiWellLogDispProperties* wlp2 = new uiWellLogDispProperties( tgs[1],
		uiWellDispProperties::Setup( "Line thickness", "Line color"),
		props.logs_[0]->right_, &(wd_->logs()) );
    wlp2->disableLogWidth( is2d );

    propflds_ += wlp1;
    propflds_ += wlp2;

    BufferStringSet allmarkernms;
    for ( int idx=0; idx<wd_->markers().size(); idx++ )
	allmarkernms.add( wd_->markers()[idx]->name() );

    propflds_ += new uiWellMarkersDispProperties( tgs[2],
	uiWellDispProperties::Setup( "Marker size", "Marker color" )
	, props.markers_, allmarkernms, is2d );

    if ( !is2d )
	propflds_ += new uiWellTrackDispProperties( tgs[3],
			    uiWellDispProperties::Setup(), props.track_ );

    bool foundlog = false;
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	mAttachCB( propflds_[idx]->propChanged, uiWellDispPropDlg::propChg );
	if ( sKey::Log() == propflds_[idx]->props().subjectName() )
	{
	    ts_->addTab( tgs[idx], foundlog ? is2d ? "Log 2" : "Right Log"
					    : is2d ? "Log 1" : "Left Log" );
	    foundlog = true;
	}
	else
	    ts_->addTab( tgs[idx], propflds_[idx]->props().subjectName() );
    }

    uiPushButton* applbut = new uiPushButton( this, "&Apply to all wells",
			mCB(this,uiWellDispPropDlg,applyAllPush), true );
    applbut->attach( centeredBelow, ts_ );

    ts_->selChange().notify( mCB(this,uiWellDispPropDlg,tabSel) );

    setWDNotifiers( true );
    mAttachCB( windowClosed, uiWellDispPropDlg::onClose );

    tabSel( 0 );
}


uiWellDispPropDlg::~uiWellDispPropDlg()
{
    detachAllNotifiers();
}


void uiWellDispPropDlg::tabSel(CallBacker*)
{
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	int curpageid = ts_->currentPageId();
	mDynamicCastGet(
	    uiWellLogDispProperties*, curwelllogproperty,propflds_[curpageid]);
	if ( curwelllogproperty )
	   propflds_[idx]->curwelllogproperty_ =  curwelllogproperty;
	else
	   propflds_[idx]->curwelllogproperty_ = 0;
    }
}


void uiWellDispPropDlg::setWDNotifiers( bool yn )
{
    if ( !wd_ ) return;

    if ( yn )
    {
	mAttachCB( mDispNot, uiWellDispPropDlg::wdChg );
	mAttachCB( mDelNot, uiWellDispPropDlg::welldataDelNotify );
    }
    else
    {
	mDetachCB( mDispNot, uiWellDispPropDlg::wdChg );
	mDetachCB( mDelNot, uiWellDispPropDlg::welldataDelNotify );
    }
}


void uiWellDispPropDlg::getFromScreen()
{
    for ( int idx=0; idx<propflds_.size(); idx++ )
	propflds_[idx]->getFromScreen();
}


void uiWellDispPropDlg::putToScreen()
{
    for ( int idx=0; idx<propflds_.size(); idx++ )
	propflds_[idx]->putToScreen();
}


void uiWellDispPropDlg::wdChg( CallBacker* )
{
    NotifyStopper ns( mDispNot );
    putToScreen();
}


void uiWellDispPropDlg::propChg( CallBacker* )
{
    getFromScreen();
    mDispNot.trigger();
}


void uiWellDispPropDlg::applyAllPush( CallBacker* )
{
    getFromScreen();
    applyAllReq.trigger();
}


void uiWellDispPropDlg::welldataDelNotify( CallBacker* )
{
    windowClosed.trigger();
    wd_ = 0;
    uiOBJDISP()->go( this );
}


void uiWellDispPropDlg::onClose( CallBacker* )
{
}


bool uiWellDispPropDlg::rejectOK( CallBacker* )
{
    savedefault_ = saveButtonChecked();
    return true;
}


uiMultiWellDispPropDlg::uiMultiWellDispPropDlg( uiParent* p,
					        ObjectSet<Well::Data>& wds,
						bool is2ddisplay )
	: uiWellDispPropDlg(p,wds[0],is2ddisplay)
	, wds_(wds)
	, wellselfld_(0)
{
    if ( wds_.size()>1 )
    {
	BufferStringSet wellnames;
	for ( int idx=0; idx< wds_.size(); idx++ )
	    wellnames.addIfNew( wds_[idx]->name() );

	wellselfld_ = new uiLabeledComboBox( this, tr("Select Well") );
	wellselfld_->box()->addItems( wellnames );
	mAttachCB( wellselfld_->box()->selectionChanged,
		   uiMultiWellDispPropDlg::wellSelChg );
	wellselfld_->attach( hCentered );
	ts_->attach( ensureBelow, wellselfld_ );
    }
}


void uiMultiWellDispPropDlg::resetProps( int logidx )
{
    bool first = true;
    Well::DisplayProperties& prop = wd_->displayProperties( is2ddisplay_ );
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	mDynamicCastGet( uiWellTrackDispProperties*,trckfld,propflds_[idx] );
	mDynamicCastGet( uiWellMarkersDispProperties*,mrkfld,propflds_[idx] );
	mDynamicCastGet( uiWellLogDispProperties*,logfld,propflds_[idx] );
	if ( logfld )
	{
	    logfld->setLogSet( &wd_->logs() );
	    logfld->resetProps( first ? prop.logs_[logidx]->left_
				      :	prop.logs_[logidx]->right_ );
	    first = false;
	}
	else if ( trckfld )
	    trckfld->resetProps( prop.track_ );
	else if ( mrkfld )
	{
	    BufferStringSet allmarkernms;
	    for ( int idy=0; idy<wd_->markers().size(); idy++ )
		allmarkernms.add( wd_->markers()[idy]->name() );

	    mrkfld->setAllMarkerNames( allmarkernms );
	    mrkfld->resetProps( prop.markers_ );
	}
    }
    putToScreen();
}


void uiMultiWellDispPropDlg::wellSelChg( CallBacker* )
{
    const int selidx = wellselfld_ ? wellselfld_->box()->currentItem() : 0;
    wd_ = wds_[selidx];
    resetProps( 0 );
}


void uiMultiWellDispPropDlg::setWDNotifiers( bool yn )
{
    Well::Data* curwd = wd_;
    for ( int idx=0; idx<wds_.size(); idx++ )
    {
	wd_ = wds_[idx];
	if ( yn )
	{
	    mAttachCB( mDispNot, uiMultiWellDispPropDlg::wdChg );
	    mAttachCB( mDelNot, uiMultiWellDispPropDlg::welldataDelNotify);
	}
	else
	{
	    mDetachCB( mDispNot, uiMultiWellDispPropDlg::wdChg );
	    mDetachCB( mDelNot, uiMultiWellDispPropDlg::welldataDelNotify);
	}
    }
    wd_ = curwd;
}


void uiMultiWellDispPropDlg::onClose( CallBacker* )
{
}


