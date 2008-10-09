/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegydefdlg.cc,v 1.8 2008-10-09 09:09:47 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisegydefdlg.h"

#include "uisegydef.h"
#include "uisegyexamine.h"
#include "uisegyexamine.h"
#include "uitoolbar.h"
#include "uicombobox.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uiseparator.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "keystrs.h"
#include "segytr.h"
#include "seisioobjinfo.h"


uiSEGYDefDlg::Setup::Setup()
    : uiDialog::Setup("SEG-Y tool","Specify basic properties",mTODOHelpID)
    , defgeom_(Seis::Vol)
{
}


uiSEGYDefDlg::uiSEGYDefDlg( uiParent* p, const uiSEGYDefDlg::Setup& su,
			  IOPar& iop )
    : uiDialog( p, su )
    , setup_(su)
    , pars_(iop)
    , geomfld_(0)
    , geomtype_(Seis::Vol)
    , readParsReq(this)
{
    uiSEGYFileSpec::Setup sgyfssu; sgyfssu.forread(true).pars(&iop);
    sgyfssu.canbe3d( su.geoms_.indexOf( Seis::Vol ) >= 0
	    	  && su.geoms_.indexOf( Seis::VolPS ) >= 0 );
    filespecfld_ = new uiSEGYFileSpec( this, sgyfssu );

    uiGroup* lastgrp = filespecfld_;
    if ( su.geoms_.size() == 1 )
	    geomtype_ = su.geoms_[0];
    else
    {
	if ( su.geoms_.isEmpty() )
	    uiSEGYRead::Setup::getDefaultTypes( setup_.geoms_ );
	if ( setup_.geoms_.indexOf( setup_.defgeom_ ) < 0 )
	    setup_.defgeom_ = setup_.geoms_[0];

	uiLabeledComboBox* lcb = new uiLabeledComboBox( this, "File type" );
	geomfld_ = lcb->box();
	for ( int idx=0; idx<su.geoms_.size(); idx++ )
	    geomfld_->addItem( Seis::nameOf( (Seis::GeomType)su.geoms_[idx] ) );
	geomfld_->setCurrentItem( setup_.geoms_.indexOf(setup_.defgeom_) );
	geomfld_->selectionChanged.notify( mCB(this,uiSEGYDefDlg,geomChg) );
	lcb->attach( alignedBelow, filespecfld_ );
	lastgrp = lcb;
    }

    uiSeparator* sep = new uiSeparator( this, "hor sep", true, false );
    sep->attach( stretchedBelow, lastgrp );

    nrtrcexfld_ = new uiGenInput( this, "Examine first",
			      IntInpSpec(100).setName("Traces to Examine") );
    nrtrcexfld_->attach( alignedBelow, lastgrp );
    nrtrcexfld_->attach( ensureBelow, sep );
    uiLabel* lbl = new uiLabel( this, "traces" );
    lbl->attach( rightOf, nrtrcexfld_ );
    fileparsfld_ = new uiSEGYFilePars( this, true, &iop );
    fileparsfld_->attach( alignedBelow, nrtrcexfld_ );
    fileparsfld_->readParsReq.notify( mCB(this,uiSEGYDefDlg,readParsCB) );

    finaliseDone.notify( mCB(this,uiSEGYDefDlg,initFlds) );
    	// Need this to get zero padding right
}


void uiSEGYDefDlg::initFlds( CallBacker* )
{
    usePar( pars_ );
    geomChg( 0 );
}


uiSEGYDefDlg::~uiSEGYDefDlg()
{
}


Seis::GeomType uiSEGYDefDlg::geomType() const
{
    if ( !geomfld_ )
	return geomtype_;

    return Seis::geomTypeOf( geomfld_->textOfItem( geomfld_->currentItem() ) );
}


int uiSEGYDefDlg::nrTrcExamine() const
{
    return nrtrcexfld_->getIntValue();
}


void uiSEGYDefDlg::use( const IOObj* ioobj, bool force )
{
    filespecfld_->use( ioobj, force );
    fileparsfld_->use( ioobj, force );
    SeisIOObjInfo oinf( ioobj );
    if ( oinf.isOK() )
    {
	if ( geomfld_ )
	{
	    geomfld_->setCurrentItem( Seis::nameOf(oinf.geomType()) );
	    geomChg( 0 );
	}
	usePar( ioobj->pars() );
    }
}


void uiSEGYDefDlg::fillPar( IOPar& iop ) const
{
    iop.merge( pars_ );
    filespecfld_->fillPar( iop );
    fileparsfld_->fillPar( iop );
    iop.set( uiSEGYExamine::Setup::sKeyNrTrcs, nrTrcExamine() );
    iop.set( sKey::Geometry, Seis::nameOf(geomType()) );
}


void uiSEGYDefDlg::usePar( const IOPar& iop )
{
    pars_.merge( iop );
    filespecfld_->usePar( pars_ );
    fileparsfld_->usePar( pars_ );
    int nrex = nrTrcExamine();
    iop.get( uiSEGYExamine::Setup::sKeyNrTrcs, nrex );
    nrtrcexfld_->setValue( nrex );   
    const char* res = iop.find( sKey::Geometry );
    if ( res && *res )
    {
	geomfld_->setCurrentItem( res );
	geomChg( 0 );
    }
}


void uiSEGYDefDlg::readParsCB( CallBacker* )
{
    readParsReq.trigger();
}


void uiSEGYDefDlg::geomChg( CallBacker* )
{
    filespecfld_->setInp2D( Seis::is2D(geomType()) );
}


bool uiSEGYDefDlg::acceptOK( CallBacker* )
{
    IOPar tmp;
    if ( !filespecfld_->fillPar(tmp) || !fileparsfld_->fillPar(tmp) )
	return false;

    fillPar( pars_ );
    return true;
}
