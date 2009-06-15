/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Helene Huck
 Date:		April 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiflatviewslicepos.cc,v 1.2 2009-06-15 12:17:23 cvsnanne Exp $";

#include "uiflatviewslicepos.h"

#include "survinfo.h"


uiSlicePos2DView::uiSlicePos2DView( uiParent* p )
    : uiSlicePos( p )
{
}


uiSlicePos::Orientation getOrientation( const CubeSampling& cs )
{
    if ( cs.defaultDir() == CubeSampling::Crl )
	return uiSlicePos::Crossline;
    if ( cs.defaultDir() == CubeSampling::Z )
	return uiSlicePos::Timeslice;

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


void uiSlicePos2DView::setBoxRanges()
{
    setBoxRg( curorientation_, SI().sampling(true) );
}


void uiSlicePos2DView::setPosBoxValue()
{
    setPosBoxVal( curorientation_, curcs_ ); 
}


void uiSlicePos2DView::setStepBoxValue()
{
}


void uiSlicePos2DView::slicePosChg( CallBacker* )
{
    slicePosChanged( curorientation_, curcs_ );
}


void uiSlicePos2DView::sliceStepChg( CallBacker* )
{
    sliceStepChanged( curorientation_ );
}
