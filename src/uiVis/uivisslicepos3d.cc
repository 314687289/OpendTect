/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		July 2006
 RCS:		$Id: uivisslicepos3d.cc,v 1.2 2006-07-19 15:21:25 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uislicepos.h"

#include "uilabel.h"
#include "uispinbox.h"
#include "uitoolbar.h"
#include "uivispartserv.h"
#include "visplanedatadisplay.h"

#include "cubesampling.h"
#include "survinfo.h"


#define Display visSurvey::PlaneDataDisplay

uiSlicePos::uiSlicePos( uiParent* p )
    : toolbar_(new uiToolBar(p,"Slice position"))
    , curpdd_(0)
    , positionChg(this)
{
    toolbar_->setCloseMode( 2 );

    sliceposbox_ = new uiLabeledSpinBox( toolbar_, "Crossline", 0,
	    				 "Slice position" );
    sliceposbox_->setSensitive( curpdd_ );
    sliceposbox_->box()->valueChanged.notify(
	    			mCB(this,uiSlicePos,slicePosChg) );

    slicestepbox_ = new uiLabeledSpinBox( toolbar_, "Step", 0, "Slice step" );
    slicestepbox_->setSensitive( curpdd_ );
    slicestepbox_->box()->valueChanged.notify(
	    			mCB(this,uiSlicePos,sliceStepChg) );

    laststeps_[0] = SI().inlStep();
    laststeps_[1] = SI().crlStep();
    laststeps_[2] = mNINT( SI().zStep()*SI().zFactor() );
}


uiSlicePos::~uiSlicePos()
{
    delete toolbar_;
}


void uiSlicePos::setDisplay( Display* pdd )
{
    if ( curpdd_ )
    {
	curpdd_->getManipulationNotifier()->remove(
					mCB(this,uiSlicePos,updatePos) );
	curpdd_->unRef();
    }
    curpdd_ = pdd;
    if ( curpdd_ )
    {
	curpdd_->ref();
	curpdd_->getManipulationNotifier()->notify(
					mCB(this,uiSlicePos,updatePos) );
    }

    sliceposbox_->setSensitive( curpdd_ );
    slicestepbox_->setSensitive( curpdd_ );

    if ( !curpdd_ ) return;

    setBoxLabel();
    setBoxRanges();
    setPosBoxValue();
    setStepBoxValue();
}


int uiSlicePos::getDisplayID() const
{
    return curpdd_ ? curpdd_->id() : -1;
}


void uiSlicePos::setBoxLabel()
{
    const Display::Orientation orientation = curpdd_->getOrientation();
    if ( orientation == Display::Inline )
	sliceposbox_->label()->setText( "Inline" );
    else if ( orientation == Display::Crossline )
	sliceposbox_->label()->setText( "Crossline" );
    else
	sliceposbox_->label()->setText( SI().zIsTime() ? "Time" : "Depth" );
}


void uiSlicePos::setBoxRanges()
{
    if ( !curpdd_ ) return;

    uiSpinBox* posbox = sliceposbox_->box();
    uiSpinBox* stepbox = slicestepbox_->box();
    NotifyStopper posstop( posbox->valueChanged );
    NotifyStopper stepstop( stepbox->valueChanged );

    const CubeSampling& survey = SI().sampling( true );
    const Display::Orientation orientation = curpdd_->getOrientation();
    if ( orientation == Display::Inline )
    {
	posbox->setInterval( survey.hrg.start.inl, survey.hrg.stop.inl );
	stepbox->setInterval( survey.hrg.step.inl,
			      survey.hrg.stop.inl-survey.hrg.start.inl,
			      survey.hrg.step.inl );
    }
    else if ( orientation == Display::Crossline )
    {
	posbox->setInterval( survey.hrg.start.crl, survey.hrg.stop.crl );
	stepbox->setInterval( survey.hrg.step.crl,
			      survey.hrg.stop.crl-survey.hrg.start.crl,
			      survey.hrg.step.crl );
    }
    else
    {
	const float zfac = SI().zFactor();
	posbox->setInterval( survey.zrg.start*zfac, survey.zrg.stop*zfac);
	stepbox->setInterval( survey.zrg.step*zfac,
			      (survey.zrg.stop-survey.zrg.start)*zfac,
			      survey.zrg.step*zfac );
    }
}


void uiSlicePos::setPosBoxValue()
{
    if ( !curpdd_ ) return;

    uiSpinBox* posbox = sliceposbox_->box();
    NotifyStopper posstop( posbox->valueChanged );

    const CubeSampling cs = curpdd_->getCubeSampling();
    const Display::Orientation orientation = curpdd_->getOrientation();
    if ( orientation == Display::Inline )
	posbox->setValue( cs.hrg.start.inl );
    else if ( orientation == Display::Crossline )
	posbox->setValue( cs.hrg.start.crl );
    else
	posbox->setValue( cs.zrg.start*SI().zFactor() );
}


void uiSlicePos::setStepBoxValue()
{
    if ( !curpdd_ ) return;

    const Display::Orientation orientation = curpdd_->getOrientation();
    slicestepbox_->box()->setValue( laststeps_[(int)orientation] );
    sliceStepChg( 0 );
}


void uiSlicePos::slicePosChg( CallBacker* )
{
    if ( !curpdd_ ) return;

    uiSpinBox* posbox = sliceposbox_->box();
    CubeSampling oldcs = curpdd_->getCubeSampling();
    curcs_ = oldcs;
    const Display::Orientation orientation = curpdd_->getOrientation();
    if ( orientation == Display::Inline )
	curcs_.hrg.start.inl = curcs_.hrg.stop.inl = posbox->getValue();
    else if ( orientation == Display::Crossline )
	curcs_.hrg.start.crl = curcs_.hrg.stop.crl = posbox->getValue();
    else
	curcs_.zrg.start = curcs_.zrg.stop = posbox->getValue()/SI().zFactor();

    if ( oldcs == curcs_ )
	return;

    positionChg.trigger();
}


void uiSlicePos::sliceStepChg( CallBacker* )
{
    if ( !curpdd_ ) return;

    const Display::Orientation orientation = curpdd_->getOrientation();
    laststeps_[(int)orientation] = slicestepbox_->box()->getValue();

    sliceposbox_->box()->setStep( laststeps_[(int)orientation] );
}


void uiSlicePos::updatePos( CallBacker* )
{
    setPosBoxValue();
}
