/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2006
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uid2tmodelgrp.h"
#include "uitblimpexpdatasel.h"

#include "uifileinput.h"
#include "uigeninput.h"

#include "ctxtioobj.h"
#include "od_istream.h"
#include "survinfo.h"
#include "veldesc.h"
#include "unitofmeasure.h"
#include "welld2tmodel.h"
#include "wellimpasc.h"
#include "welldata.h"
#include "welltrack.h"


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
	BufferString velllbl( "Temporary model velocity ");
	velllbl.add( VelocityDesc::getVelUnit( true ) );
	const float vel = Well::getDefaultVelocity();
	filefld_->setWithCheck( true ); filefld_->setChecked( true );
	filefld_->checked.notify( mCB(this,uiD2TModelGroup,fileFldChecked) );
	velfld_ = new uiGenInput( this, velllbl, FloatInpSpec(vel) );
	velfld_->attach( alignedBelow, filefld_ );
    }

    dataselfld_ = new uiTableImpDataSel( this, fd_, "107.0.3" );
    dataselfld_->attach( alignedBelow, setup_.fileoptional_ ? velfld_
							    : filefld_ );

    if ( setup_.asksetcsmdl_ )
    {
	csfld_ = new uiGenInput( this, "Is this checkshot data?",
				 BoolInpSpec(false) );
	csfld_->attach( alignedBelow, dataselfld_ );
    }

    setHAlignObj( filefld_ );
    postFinalise().notify( mCB(this,uiD2TModelGroup,fileFldChecked) );
}



void uiD2TModelGroup::fileFldChecked( CallBacker* )
{
    const bool havefile = setup_.fileoptional_ ? filefld_->isChecked() : true;
    if ( csfld_ ) csfld_->display( havefile );
    if ( velfld_ ) velfld_->display( !havefile );
    dataselfld_->display( havefile );
    dataselfld_->updateSummary();
}


#define mGetVel(var,fac) \
	const float var = velfld_->getfValue() * fac * \
			( SI().depthsInFeet() ? mFromFeetFactorF : 1.0f )


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

	d2t.setEmpty();
	const UnitOfMeasure* zun_ = UnitOfMeasure::surveyDefDepthUnit();
	float srd = mCast( float, SI().seismicReferenceDatum() );
	float kb  = wd.track().getKbElev();
	if ( SI().depthsInFeet() && zun_ )
	{
	    srd = zun_->userValue( srd );
	    kb  = zun_->userValue( kb );
	}
	if ( mIsZero(srd,0.01f) ) srd = 0.f;
	mGetVel(twtvel,0.5f);
	const float bulkshift = mIsUdf( wd.info().replvel ) ? 0 : ( kb-srd )*
				( (1 / twtvel) - (2 / wd.info().replvel) );
	int idahofminz = 0;
	int idahofmaxz = wd.track().size()-1;
	float tvdmin = 1e10;
	float tvdmax = -1;
	for ( int idx=0; idx<wd.track().size(); idx++ )
	{
	    const float zpostrack = (float)wd.track().pos(idx).z;
	    if ( zpostrack > tvdmax )
	    {
		tvdmax = zpostrack;
		idahofmaxz = idx;
	    }

	    if ( zpostrack < tvdmin )
	    {
		tvdmin = zpostrack;
		idahofminz = idx;
	    }
	}

	d2t.add( wd.track().dah(idahofminz), ( tvdmin+srd )/twtvel+ bulkshift );
	d2t.add( wd.track().dah(idahofmaxz), ( tvdmax+srd )/twtvel+ bulkshift );
    }
    else
    {
	const char* fname = filefld_->fileName();
	od_istream strm( fname );
	if ( !strm.isOK() )
	    return "Could not open input file";

	BufferString errmsg;
	if ( !dataselfld_->commit() )
	    return "Please specify data format";

	d2t.setName( fname );
	Well::D2TModelAscIO aio( fd_ );
	aio.get( strm, d2t, wd );
    }

    d2t.deInterpolate();
    return 0;
}


bool uiD2TModelGroup::wantAsCSModel() const
{
    return csfld_ && csfld_->getBoolValue() && filefld_->isChecked();
}


BufferString uiD2TModelGroup::dataSourceName() const
{
    BufferString ret;
    if ( !filefld_->isCheckable() || filefld_->isChecked() )
	ret.set( filefld_->fileName() );
    else
    {
	mGetVel(vel,1.0f);
	ret.set( "[V=" ).add( vel ).add( " " );
	ret.add( VelocityDesc::getVelUnit(true) ).add( "]" );
    }
    return ret;
}
