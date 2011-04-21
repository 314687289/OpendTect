/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene Huck
 Date:		April 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiflatviewslicepos.cc,v 1.8 2011-04-21 13:09:13 cvsbert Exp $";

#include "uiflatviewslicepos.h"

#include "survinfo.h"
#include "uispinbox.h"


uiSlicePos2DView::uiSlicePos2DView( uiParent* p )
    : uiSlicePos( p )
{
}


static uiSlicePos::Orientation getOrientation( const CubeSampling& cs )
{
    if ( cs.defaultDir() == CubeSampling::Crl )
	return uiSlicePos::Crossline;
    if ( cs.defaultDir() == CubeSampling::Z )
	return uiSlicePos::Zslice;

    return uiSlicePos::Inline;
}


void uiSlicePos2DView::setCubeSampling( const CubeSampling& cs )
{
    curorientation_ = getOrientation( cs );
    curcs_ = cs;

    setBoxLabel( curorientation_ );
    setBoxRanges();
    setPosBoxValue();
    setStepBoxValue();
}


void uiSlicePos2DView::setLimitSampling( const CubeSampling& cs )
{
    limitscs_ = cs;
    setCubeSampling( curcs_ );
}


void uiSlicePos2DView::setBoxRanges()
{
    setBoxRg( curorientation_, limitscs_ );
}


void uiSlicePos2DView::setPosBoxValue()
{
    setPosBoxVal( curorientation_, curcs_ ); 
}


void uiSlicePos2DView::setStepBoxValue()
{
    slicestepbox_->setValue( laststeps_[(int)curorientation_] );
    sliceStepChg( 0 );
}


void uiSlicePos2DView::slicePosChg( CallBacker* )
{
    slicePosChanged( curorientation_, curcs_ );
}


void uiSlicePos2DView::sliceStepChg( CallBacker* )
{
    sliceStepChanged( curorientation_ );
}
