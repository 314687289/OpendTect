/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: uihorizontracksetup.cc 38749 2015-04-02 19:49:51Z nanne.hemstra@dgbes.com $";

#include "uimpecorrelationgrp.h"

#include "draw.h"
#include "horizonadjuster.h"
#include "mouseevent.h"
#include "mpeengine.h"
#include "sectiontracker.h"
#include "survinfo.h"

#include "uichecklist.h"
#include "uiflatviewer.h"
#include "uigeninput.h"
#include "uigraphicsview.h"
#include "uimpepreviewgrp.h"
#include "uimsg.h"
#include "uiseparator.h"

// TODO: Move preview viewer to separate object

#define mErrRet(s) { uiMSG().error( s ); return false; }

namespace MPE
{

uiCorrelationGroup::uiCorrelationGroup( uiParent* p, bool is2d )
    : uiDlgGroup(p,tr("Correlation"))
    , sectiontracker_(0)
    , adjuster_(0)
    , changed_(this)
    , seedpos_(Coord3::udf())
    , mousedown_(false)
{
    uiGroup* leftgrp = new uiGroup( this, "Left Group" );
    usecorrfld_ = new uiGenInput( leftgrp, tr("Use Correlation"),
				  BoolInpSpec(true));
    usecorrfld_->valuechanged.notify(
	    mCB(this,uiCorrelationGroup,selUseCorrelation) );
    usecorrfld_->valuechanged.notify(
	    mCB(this,uiCorrelationGroup,correlationChangeCB) );

    IntInpIntervalSpec iis; iis.setSymmetric( true );
    StepInterval<int> swin( -10000, 10000, 1 );
    iis.setLimits( swin, 0 ); iis.setLimits( swin, 1 );
    BufferString compwindtxt( "Compare window ", SI().getZUnitString() );
    compwinfld_ = new uiGenInput( leftgrp, compwindtxt, iis );
    compwinfld_->attach( alignedBelow, usecorrfld_ );
    compwinfld_->valuechanging.notify(
		mCB(this,uiCorrelationGroup,correlationChangeCB) );

    IntInpSpec tiis;
    tiis.setLimits( StepInterval<int>(0,100,1) );
    corrthresholdfld_ =
	new uiGenInput( leftgrp, tr("Correlation threshold (%)"), tiis );
    corrthresholdfld_->attach( alignedBelow, compwinfld_ );
    corrthresholdfld_->valuechanged.notify(
		mCB(this,uiCorrelationGroup,correlationChangeCB) );

    uiSeparator* sep = new uiSeparator( leftgrp, "Sep" );
    sep->attach( stretchedBelow, corrthresholdfld_ );

    const int step = mCast(int,SI().zStep()*SI().zDomain().userFactor());
    StepInterval<int> intv( -10000, 10000, step );
    IntInpIntervalSpec diis; diis.setSymmetric( true );
    diis.setLimits( intv, 0 ); diis.setLimits( intv, 1 );

    BufferString disptxt( "Data Display window ", SI().getZUnitString() );
    nrzfld_ = new uiGenInput( leftgrp, disptxt, diis );
    nrzfld_->attach( alignedBelow, corrthresholdfld_ );
    nrzfld_->attach( ensureBelow, sep );
    nrzfld_->valuechanging.notify(
		mCB(this,uiCorrelationGroup,visibleDataChangeCB) );

    tiis.setLimits( StepInterval<int>(3,99,2) );
    nrtrcsfld_ = new uiGenInput( leftgrp, "Nr Traces", tiis );
    nrtrcsfld_->attach( alignedBelow, nrzfld_ );
    nrtrcsfld_->valuechanging.notify(
		mCB(this,uiCorrelationGroup,visibleDataChangeCB) );

    previewgrp_ = new uiPreviewGroup( this );
    previewgrp_->windowChanged_.notify(
		mCB(this,uiCorrelationGroup,previewChgCB) );

    setHAlignObj( usecorrfld_ );
}


uiCorrelationGroup::~uiCorrelationGroup()
{
}


void uiCorrelationGroup::selUseCorrelation( CallBacker* )
{
    const bool usecorr = usecorrfld_->getBoolValue();
    compwinfld_->setSensitive( usecorr );
    corrthresholdfld_->setSensitive( usecorr );
    previewgrp_->setSensitive( usecorr );

    nrzfld_->setSensitive( usecorr );
    nrtrcsfld_->setSensitive( usecorr );
}


void uiCorrelationGroup::visibleDataChangeCB( CallBacker* )
{
    previewgrp_->setWindow( compwinfld_->getIInterval() );
    previewgrp_->setDisplaySize( nrtrcsfld_->getIntValue(),
				 nrzfld_->getIInterval() );
}


void uiCorrelationGroup::correlationChangeCB( CallBacker* )
{
    previewgrp_->setWindow( compwinfld_->getIInterval() );
    changed_.trigger();
}


void uiCorrelationGroup::previewChgCB(CallBacker *)
{
    const Interval<int> intv = previewgrp_->getManipWindow();
    if ( mIsUdf(intv.start) )
	compwinfld_->setValue( intv.stop, 1 );
    if ( mIsUdf(intv.stop) )
	compwinfld_->setValue( intv.start, 0 );
}


void uiCorrelationGroup::setSectionTracker( SectionTracker* st )
{
    sectiontracker_ = st;
    mDynamicCastGet(HorizonAdjuster*,horadj,sectiontracker_->adjuster())
    adjuster_ = horadj;
    if ( !adjuster_ ) return;

    init();
}


void uiCorrelationGroup::init()
{
    usecorrfld_->setValue( !adjuster_->trackByValue() );

    const Interval<int> corrintv(
	    mCast(int,adjuster_->similarityWindow().start *
		      SI().zDomain().userFactor()),
	    mCast(int,adjuster_->similarityWindow().stop *
		      SI().zDomain().userFactor()) );

    compwinfld_->setValue( corrintv );
    corrthresholdfld_->setValue( adjuster_->similarityThreshold()*100 );

    const int sample = mCast(int,SI().zStep()*SI().zDomain().userFactor());
    const Interval<int> dataintv = corrintv + Interval<int>(-2*sample,2*sample);
    nrzfld_->setValue( dataintv );
    nrtrcsfld_->setValue( 5 );
}


void uiCorrelationGroup::setSeedPos( const Coord3& crd )
{
    seedpos_ = crd;
    previewgrp_->setSeedPos( crd );
}


bool uiCorrelationGroup::commitToTracker( bool& fieldchange ) const
{
    fieldchange = false;

    const bool usecorr = usecorrfld_->getBoolValue();
    if ( adjuster_->trackByValue() == usecorr )
    {
	fieldchange = true;
	adjuster_->setTrackByValue( !usecorr );
    }

    if ( !usecorr )
	return true;

    const Interval<int> intval = compwinfld_->getIInterval();
    if ( intval.width()==0 || intval.isRev() )
	mErrRet( tr("Correlation window's start value must be less than"
		    " the stop value") );

    Interval<float> relintval(
	    (float)intval.start/SI().zDomain().userFactor(),
	    (float)intval.stop/SI().zDomain().userFactor() );
    if ( adjuster_->similarityWindow() != relintval )
    {
	fieldchange = true;
	adjuster_->setSimilarityWindow( relintval );
    }

    const int thr = corrthresholdfld_->getIntValue();
    if ( thr > 100 || thr < 0)
	mErrRet( tr("Correlation threshold must be between 0 to 100") );

    const float newthreshold = (float)thr/100.f;
    if ( !mIsEqual(adjuster_->similarityThreshold(),newthreshold,mDefEps) )
    {
	fieldchange = true;
	adjuster_->setSimilarityThreshold( newthreshold );
    }

    return true;
}

} //namespace MPE
