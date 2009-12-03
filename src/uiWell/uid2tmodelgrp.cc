/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uid2tmodelgrp.cc,v 1.17 2009-12-03 14:47:46 cvsbert Exp $";

#include "uid2tmodelgrp.h"
#include "uitblimpexpdatasel.h"

#include "uifileinput.h"
#include "uigeninput.h"

#include "ctxtioobj.h"
#include "welld2tmodel.h"
#include "wellimpasc.h"
#include "welldata.h"
#include "welltrack.h"
#include "strmprov.h"


uiD2TModelGroup::uiD2TModelGroup( uiParent* p, const Setup& su )
    : uiGroup(p,"D2TModel group")
    , velfld_(0)
    , csfld_(0)
    , dataselfld_(0)
    , setup_(su)
    , fd_( *Well::D2TModelAscIO::getDesc(setup_.withunitfld_) )
{
    filefld_ = new uiFileInput( this, setup_.filefldlbl_,
				uiFileInput::Setup().withexamine(true) );
    if ( setup_.fileoptional_ )
    {
	filefld_->setWithCheck( true ); filefld_->setChecked( true );
	filefld_->checked.notify( mCB(this,uiD2TModelGroup,fileFldChecked) );
	velfld_ = new uiGenInput( this, "Temporary model velocity (m/s)",
				  FloatInpSpec(4000) );
	velfld_->attach( alignedBelow, filefld_ );
    }

    dataselfld_ = new uiTableImpDataSel( this, fd_, "105.0.5" );
    dataselfld_->attach( alignedBelow, setup_.fileoptional_ ? velfld_
	    						    : filefld_ );
    
    if ( setup_.asksetcsmdl_ )
    {
	csfld_ = new uiGenInput( this, "Is this checkshot data?",
				 BoolInpSpec(true) );
	csfld_->attach( alignedBelow, dataselfld_ );
    }

    setHAlignObj( filefld_ );
    finaliseDone.notify( mCB(this,uiD2TModelGroup,fileFldChecked) );
}


void uiD2TModelGroup::fileFldChecked( CallBacker* )
{
    const bool havefile = setup_.fileoptional_ ? filefld_->isChecked() : true;
    if ( csfld_ ) csfld_->display( havefile );
    if ( velfld_ ) velfld_->display( !havefile );
    dataselfld_->display( havefile );
    dataselfld_->updateSummary();
}


const char* uiD2TModelGroup::getD2T( Well::Data& wd, bool cksh ) const
{
    if ( setup_.fileoptional_ && !filefld_->isChecked() )
    {
	if ( velfld_->isUndef() )
	    return "Please enter the velocity for generating the D2T model";
    }
    
    if ( cksh )
	wd.setCheckShotModel( new Well::D2TModel );
    else
	wd.setD2TModel( new Well::D2TModel );

    Well::D2TModel& d2t = *(cksh ? wd.checkShotModel() : wd.d2TModel());
    if ( !&d2t )
	return "D2Time model not set properly";

    if ( filefld_->isCheckable() && !filefld_->isChecked() )
    {
	if ( wd.track().isEmpty() )
	    return "Cannot generate D2Time model without track";
	
	const float twtvel = velfld_->getfValue() * .5;
	const float dah0 = wd.track().dah( 0 );
	const float dah1 = wd.track().dah( wd.track().size()-1 );
	d2t.erase();
	d2t.add( dah0, dah0 / twtvel );
	d2t.add( dah1, dah1 / twtvel );
    }
    else
    {
	const char* fname = filefld_->fileName();
	StreamData sdi = StreamProvider( fname ).makeIStream();
	if ( !sdi.usable() )
	{
	    sdi.close();
	    return "Could not open input file";
	}

	BufferString errmsg;
	if ( !dataselfld_->commit() )
	    return "Please specify data format";

	d2t.setName( fname );
	Well::D2TModelAscIO aio( fd_ );
	aio.get( *sdi.istrm, d2t, wd.track() );
    }

    d2t.deInterpolate();
    return 0;
}


bool uiD2TModelGroup::wantAsCSModel() const
{
    return csfld_ && csfld_->getBoolValue() && filefld_->isChecked();
}
