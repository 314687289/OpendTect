/*+
 *  (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *  AUTHOR   : Y.C. Liu
 *  DATE     : April 2010
-*/

static const char* rcsID = "$Id: uihorauxdatasel.cc,v 1.2 2010-04-23 19:20:27 cvsyuancheng Exp $";


#include "uihorauxdatasel.h"

#include "ctxtioobj.h"
#include "datainpspec.h"
#include "emsurfacetr.h"
#include "emmanager.h"
#include "ioman.h"
#include "iodirentry.h"
#include "uibutton.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"

class uiHorizonAuxDataDlg: public uiDialog
{
public:
		uiHorizonAuxDataDlg( uiParent* p, 
			const uiHorizonAuxDataSel::HorizonAuxDataInfo& info )
		: uiDialog( p, uiDialog::Setup("Horizon data selection","",""))
		, auxidx_( 0 )
		, selmid_( 0 )
		, auxinfo_( new uiHorizonAuxDataSel::HorizonAuxDataInfo(info) )
		{
		    if ( !auxinfo_->mids_.size() )
			uiMSG().warning("No data available for any horizon, \
				please make sure the horizon you want to use \
				has surface data on it.");

		    uiGroup* grp = new uiGroup( this, "Group" );
		    uiLabel* horlabel = new uiLabel( grp, "Horizons" );
		    horlistfld_ = new uiListBox( grp );
		    horlistfld_->addItems( auxinfo_->hornms_ );
		    horlistfld_->attach( ensureBelow, horlabel );
		    horlistfld_->setHSzPol( uiObject::Wide );
		    horlistfld_->selectionChanged.notify( 
			    mCB(this,uiHorizonAuxDataDlg,selChg) );
		    
		    datalistfld_ = new uiListBox( grp );
		    datalistfld_->attach( rightTo, horlistfld_ );
		    datalistfld_->setHSzPol( uiObject::Wide );
		    datalistfld_->selectionChanged.notify(
			    mCB(this, uiHorizonAuxDataDlg, auxidxChg) );
		    
		    uiLabel* directlabel = new uiLabel( grp, "Available data" );
		    directlabel->attach( alignedAbove, datalistfld_ );
		    directlabel->attach( rightTo, horlabel );
		}
		
		~uiHorizonAuxDataDlg()	{ delete auxinfo_; }
const MultiID&	selected() const	{ return selmid_; }
int		dataidx()		{ return auxidx_; }
void		setSelection( const MultiID& mid, int auxidx )
    		{
		    if ( auxidx_==auxidx && selmid_==mid )
			return;

		    auxidx_ = auxidx;
		    if ( auxidx_>=0 )
			datalistfld_->setCurrentItem( auxidx_ );

		    const bool hchanged = !selmid_.isEmpty() && selmid_==mid;
		    selmid_ = mid;
		    for ( int idx=0; idx<auxinfo_->mids_.size(); idx++ )
		    {
			if ( auxinfo_->mids_[idx]==selmid_ )
			{
			    horlistfld_->setCurrentItem( idx );
			    return;
			}
		    }	    
		    
		    if ( hchanged ) selChg( 0 );
		}

protected:
void		auxidxChg( CallBacker* )
		{ auxidx_ = datalistfld_->currentItem(); }

void		selChg( CallBacker* )
		{
		    const int hidx = horlistfld_->currentItem();
		    if ( !auxinfo_->mids_.size() || hidx<0 )
			return;

		    int oldidx =  auxinfo_->mids_[hidx]!=selmid_ ? 0 : auxidx_;
		    selmid_ = auxinfo_->mids_[hidx];

		    datalistfld_->empty();
		    datalistfld_->addItems( auxinfo_->auxdatanms_[hidx] ); 
		    if ( oldidx>=0 )
			datalistfld_->setCurrentItem( oldidx );
		}

uiListBox*	horlistfld_;
uiListBox*	datalistfld_;
int		auxidx_;
MultiID		selmid_;
uiHorizonAuxDataSel::HorizonAuxDataInfo* auxinfo_;
};


uiHorizonAuxDataSel::HorizonAuxDataInfo::HorizonAuxDataInfo( bool load )
{
    if ( !load ) return;

    MouseCursorChanger cursorlock( MouseCursor::Wait );
    PtrMan<CtxtIOObj> allhorio =  mMkCtxtIOObj(EMHorizon3D);
    IOM().to( allhorio->ctxt.getSelKey() );
    IODirEntryList horlist( IOM().dirPtr(), allhorio->ctxt );
	
    for ( int idx=0; idx<horlist.size(); idx++ )
    {
	const IOObj* obj = horlist[idx]->ioobj;
	EM::SurfaceIOData sd; 
	EM::EMM().getSurfaceData( obj->key(), sd ); 
	if ( sd.valnames.size() )
	{
	    mids_ += obj->key();
	    hornms_.add( obj->name() );
	    auxdatanms_ += sd.valnames;
	}
    }
}


uiHorizonAuxDataSel::HorizonAuxDataInfo::HorizonAuxDataInfo(
	const HorizonAuxDataInfo& n )
{
    hornms_ = n.hornms_;
    mids_ = n.mids_;
    auxdatanms_ = n.auxdatanms_;
}


uiHorizonAuxDataSel::HorizonAuxDataInfo::~HorizonAuxDataInfo() 
{ 
    mids_.erase(); 
    auxdatanms_.erase(); 
}


uiHorizonAuxDataSel::uiHorizonAuxDataSel( uiParent* p, const MultiID& mid, 
	int auxidx, const HorizonAuxDataInfo* auxinfo )
    : uiGroup( p )
    , selmid_( mid )
    , auxidx_( auxidx )
    , nrhorswithdata_( 0 )
{
    horfld_ = new uiGenInput( this, "Horizon", StringInpSpec() );
    selbut_ = new uiPushButton( this, "&Select",
	    mCB(this,uiHorizonAuxDataSel,selCB), false );
    selbut_->attach( rightOf, horfld_ );
 
    StringListInpSpec str; 
    PtrMan<IOObj> obj = IOM().get(mid);
    const bool hasobj = !mid.isEmpty() && obj;
    if ( hasobj )
    {
	EM::SurfaceIOData sd;
	EM::EMM().getSurfaceData( mid, sd );
	for ( int idx=0; idx<sd.valnames.size(); idx++ )
	    str.addString( *sd.valnames[idx] );
	
	horfld_->setText( obj->name() );
    }    
    
    auxfld_ = new uiGenInput( this, "Surface Data", str ); 
    auxfld_->attach( rightOf, selbut_ );
    auxfld_->setPrefWidthInChar( 60 );
    auxfld_->valuechanged.notify(
	    mCB(this,uiHorizonAuxDataSel,auxidxChg) );
    if ( hasobj && auxidx>=0 ) auxfld_->setValue(auxidx);
    
    setHAlignObj( horfld_ );
    if ( auxinfo )
    {
	dlg_ = new uiHorizonAuxDataDlg( p, *auxinfo );
	nrhorswithdata_ = auxinfo->mids_.size();
    }
    else
    {
	HorizonAuxDataInfo ainfo( true );
	dlg_ = new uiHorizonAuxDataDlg( p, ainfo );
	nrhorswithdata_ = ainfo.mids_.size();
    }
}


void uiHorizonAuxDataSel::auxidxChg( CallBacker* ) 
{ auxidx_ = auxfld_->getIntValue(); }


void uiHorizonAuxDataSel::selCB( CallBacker* ) 
{ 
    dlg_->setSelection( selmid_, auxidx_ );
    dlg_->go();

    selmid_ = dlg_->selected();
    PtrMan<IOObj> obj = IOM().get(selmid_);
    if ( !obj )	return;
 
    horfld_->setText( obj->name() );	    
    
    EM::SurfaceIOData sd;
    EM::EMM().getSurfaceData( selmid_, sd );

    StringListInpSpec str;
    for ( int idx=0; idx<sd.valnames.size(); idx++ )
	str.addString( *sd.valnames[idx] );
    auxfld_->clear(); 
    auxfld_->newSpec( str, 0 );
					    
    auxidx_ = dlg_->dataidx();
    auxfld_->setValue( auxidx_ );
}

