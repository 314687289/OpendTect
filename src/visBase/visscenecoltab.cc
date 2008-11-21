/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		May 2007
 RCS:		$Id: visscenecoltab.cc,v 1.8 2008-11-21 14:58:20 cvsbert Exp $
________________________________________________________________________

-*/

#include "visscenecoltab.h"

#include "coltabindex.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "linear.h"
#include "scaler.h"

#include "LegendKit.h"
#include <Inventor/SbColor.h>

mCreateFactoryEntry( visBase::SceneColTab );

namespace visBase
{

SceneColTab::SceneColTab()
    : VisualObjectImpl( false )
    , legendkit_(new LegendKit)
{
    addChild( legendkit_ );
    legendkit_->ref();
    legendkit_->setDiscreteMode( true );
    legendkit_->enableBackground( false );
    legendkit_->setTickAndLinesColor( SbColor(170./255,170./255,170./255) );
    setColTabSequence( ColTab::Sequence("") );
}


SceneColTab::~SceneColTab()
{
    removeChild( legendkit_ );
    legendkit_->unref();
}


void SceneColTab::setColTabSequence( const ColTab::Sequence& ctseq )
{
    const int nrcols = 256;
    legendkit_->clearColors();
    ColTab::IndexedLookUpTable table( ctseq, nrcols );
    for ( int idx=0; idx<nrcols; idx++ )
    {
	Color col = table.colorForIndex( idx );
	od_uint32 val = ( (unsigned int)(col.r()&0xff) << 24 ) |
		        ( (unsigned int)(col.g()&0xff) << 16 ) |
		        ( (unsigned int)(col.b()&0xff) <<  8 ) |
		        (col.t()&0xff);
	legendkit_->addDiscreteColor( double(idx)/256., val );
    }
}


void SceneColTab::setColTabMapperSetup( const ColTab::MapperSetup& ms )
{
    Interval<float> rg( ms.start_, ms.start_+ms.width_ );
    legendkit_->clearTicks();
    AxisLayout al; al.setDataRange( rg );
    LinScaler scaler( rg.start, 0, rg.stop, 1 );

    int idx = 0;
    while ( true )
    {
	const float val = al.sd.start + idx*al.sd.step;
	if ( val > al.stop ) break;

	const float normval = scaler.scale( val );
	if ( normval>=0 && normval<=1 )
	    legendkit_->addBigTick( normval, val );
	idx++;
    }

    legendkit_->minvalue = toString( rg.start );
    legendkit_->maxvalue = toString( rg.stop );
}


} // namespace visBase
