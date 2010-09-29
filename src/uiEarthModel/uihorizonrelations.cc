/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uihorizonrelations.cc,v 1.19 2010-09-29 02:22:50 cvsnanne Exp $";

#include "uihorizonrelations.h"

#include "uibutton.h"
#include "mousecursor.h"
#include "uitaskrunner.h"
#include "uigeninput.h"
#include "uihorizonsortdlg.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"

#include "bufstringset.h"
#include "ctxtioobj.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "emobject.h"
#include "emsurfacetr.h"
#include "ctxtioobj.h"
#include "horizonmodifier.h"
#include "horizonsorter.h"
#include "ioman.h"
#include "ioobj.h"
#include "filepath.h"
#include "iopar.h"


uiHorizonRelationsDlg::uiHorizonRelationsDlg( uiParent* p, bool is2d )
    : uiDialog(p,Setup("Horizon relations",mNoDlgTitle,"104.2.2"))
    , is2d_( is2d )
{
    relationfld_ = new uiLabeledListBox( this, "Order (top to bottom)",
				         false, uiLabeledListBox::AboveLeft );

    uiPushButton* orderbut = new uiPushButton( relationfld_, "&Read Horizons",
	    				       false );
    orderbut->activated.notify( mCB(this,uiHorizonRelationsDlg,readHorizonCB) );
    orderbut->attach( rightTo, relationfld_->box() );

    crossbut_ = new uiPushButton( relationfld_, "&Check crossings",
					       false );
    crossbut_->activated.notify(
			mCB(this,uiHorizonRelationsDlg,checkCrossingsCB) );
    crossbut_->attach( alignedBelow, orderbut );

    waterbut_ = new uiPushButton( relationfld_, "&Make watertight", false );
    waterbut_->activated.notify(
			mCB(this,uiHorizonRelationsDlg,makeWatertightCB) );
    waterbut_->attach( alignedBelow, crossbut_ );

    fillRelationField( hornames_ );
    setCtrlStyle( LeaveOnly );
}


void uiHorizonRelationsDlg::readHorizonCB( CallBacker* )
{
    uiHorizonSortDlg dlg( this, is2d_, false );
    if ( !dlg.go() ) return;

    hornames_.erase();
    horids_.erase();
    dlg.getSortedHorizonIDs( horids_ );
    for ( int idx=0; idx<horids_.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( horids_[idx] );
	if ( !ioobj )
	    horids_.remove( idx-- );

	hornames_.add( ioobj->name() );
    }

    fillRelationField( hornames_ );
}


void uiHorizonRelationsDlg::fillRelationField( const BufferStringSet& strs )
{
    relationfld_->box()->empty();
    relationfld_->box()->addItems( strs );
    crossbut_->setSensitive( strs.size() > 1 );
    waterbut_->setSensitive( strs.size() > 1 );
}


class HorizonModifyDlg : public uiDialog
{
public:
HorizonModifyDlg( uiParent* p, const MultiID& mid1, const MultiID& mid2,
		  bool is2d, int nrcross )
    : uiDialog(p,Setup("Horizon relations (Solve crossings)","","104.2.3"))
    , mid1_(mid1)
    , mid2_(mid2)
    , is2d_(is2d)
    , ctio_(is2d ? mMkCtxtIOObj(EMHorizon2D) : mMkCtxtIOObj(EMHorizon3D))
{
    BufferStringSet hornms;
    hornms.add( EM::EMM().objectName(mid1) );
    hornms.add( EM::EMM().objectName(mid2) );

    BufferString msg( "'", hornms.get(0), "' crosses '" );
    msg.add( hornms.get(1) ).add( "' at " ).add( nrcross ).add( " positions." );
    uiLabel* lbl = new uiLabel( this, msg );

    horizonfld_ = new uiGenInput( this, "Modify horizon",
				  StringListInpSpec(hornms) );
    horizonfld_->valuechanged.notify( mCB(this,HorizonModifyDlg,horSel) );
    horizonfld_->attach( leftAlignedBelow, lbl );

    modefld_ = new uiGenInput( this, "Modify action",
	    		       BoolInpSpec(true,"Shift","Remove") );
    modefld_->attach( alignedBelow, horizonfld_ );

    savefld_ = new uiGenInput( this, "Save modified horizon",
			       BoolInpSpec(true,"As new","Overwrite") );
    savefld_->valuechanged.notify( mCB(this,HorizonModifyDlg,saveCB) );
    savefld_->attach( alignedBelow, modefld_ );

    ctio_->ctxt.forread = false;
    objfld_ = new uiIOObjSel( this, *ctio_, "Horizon" );
    objfld_->display( false );
    objfld_->attach( alignedBelow, savefld_ );

    saveCB(0);
    horSel(0);
}


~HorizonModifyDlg()
{
    delete ctio_->ioobj;
    delete ctio_;
}


void saveCB( CallBacker* )
{
    objfld_->display( savefld_->getBoolValue() );
}


void horSel( CallBacker* )
{
    const bool topisstatic = horizonfld_->getIntValue() == 1;
    BufferString hornm = EM::EMM().objectName( topisstatic ? mid2_ : mid1_ );
    hornm += "_edited";
    objfld_->setInputText( hornm );
}


#define mErrRet(msg) { if ( msg ) uiMSG().error(msg); return false; }

bool acceptOK( CallBacker* )
{
    HorizonModifier modifier( is2d_ );
    modifier.setHorizons( mid1_, mid2_ );
    modifier.setMode( modefld_->getBoolValue() ? HorizonModifier::Shift
					       : HorizonModifier::Remove );

    const bool topisstatic = horizonfld_->getIntValue() == 1;
    modifier.setStaticHorizon( topisstatic );
    modifier.doWork();

    const EM::ObjectID objid =
		EM::EMM().getObjectID( topisstatic ? mid2_ : mid1_ );
    EM::EMObject* emobj = EM::EMM().getObject( objid );
    PtrMan<Executor> exec = 0;
    const bool saveas = savefld_->getBoolValue();
    if ( !saveas )
	exec = emobj->saver();
    else
    {
	if ( !objfld_->commitInput() )
	    mErrRet(objfld_->isEmpty() ? "Please select output surface" : 0)

	const MultiID& outmid = ctio_->ioobj->key();
	emobj->setMultiID( outmid );
	mDynamicCastGet(EM::Surface*,surface,emobj)
	exec = surface->geometry().saver( 0, &outmid );
    }

    if ( !exec ) mErrRet("Cannot save horizon")

    uiTaskRunner taskrunner( this );
    return taskrunner.execute( *exec );
}

protected:
    bool	is2d_;
    MultiID	mid1_;
    MultiID	mid2_;

    CtxtIOObj*	ctio_;

    uiGenInput*	horizonfld_;
    uiGenInput*	modefld_;
    uiGenInput*	savefld_;
    uiIOObjSel*	objfld_;
};


void uiHorizonRelationsDlg::checkCrossingsCB( CallBacker* )
{
    MouseCursorChanger chgr( MouseCursor::Wait );

    HorizonSorter sorter( horids_,is2d_ );
    sorter.setName( "Check crossings" );
    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(sorter) ) return;
    MouseCursorManager::restoreOverride();

    int count = 0;
    for ( int idx=0; idx<horids_.size(); idx++ )
    {
	for ( int idy=idx+1; idy<horids_.size(); idy++ )
	{
	    const int nrcrossings = sorter.getNrCrossings( horids_[idx],
		    					   horids_[idy] );
	    if ( nrcrossings == 0 ) continue;

	    TypeSet<MultiID> sortedids;
	    sorter.getSortedList( sortedids );
	    const int idx1 = sortedids.indexOf( horids_[idx] );
	    const int idx2 = sortedids.indexOf( horids_[idy] );
	    HorizonModifyDlg dlg( this, sortedids[mMIN(idx1,idx2)],
		    		  sortedids[mMAX(idx1,idx2)], is2d_,
				  nrcrossings );
	    dlg.go();
	    count++;
	}
    }

    if ( count > 0 ) return;
    uiMSG().message( "No crossings found" );
}


void uiHorizonRelationsDlg::makeWatertightCB( CallBacker* )
{
    uiMSG().message( "Not implemented yet" );
}
