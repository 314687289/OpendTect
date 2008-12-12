/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
Date:		Aug 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uicoltabtools.cc,v 1.13 2008-12-12 06:02:40 cvssatyaki Exp $";

#include "uicoltabtools.h"
#include "uicanvas.h"
#include "uirgbarray.h"
#include "uigeom.h"
#include "uiworld2ui.h"

#include "coltab.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "coltabindex.h"
#include "pixmap.h"
#include "bufstringset.h"


uiColorTableCanvas::uiColorTableCanvas( uiParent* p, const ColTab::Sequence& ct,
       					bool withalpha, bool vert )
    : uiRGBArrayCanvas(p, uiRGBArrayCanvas::Setup(false), mkRGBArr(withalpha))
    , vertical_(vert)
    , ctseq_(ct)
{
    setDragMode( uiGraphicsView::NoDrag );
    setBackgroundQpaque( withalpha );
    setRGB();
}


uiColorTableCanvas::~uiColorTableCanvas()
{
    delete rgbarr_;
}


uiRGBArray& uiColorTableCanvas::mkRGBArr( bool withalpha )
{
    rgbarr_ = new uiRGBArray( withalpha );
    return *rgbarr_;
}


void uiColorTableCanvas::setRGB()
{
    if ( ctseq_.name().isEmpty() )
	return;

    beforeDraw();
    const int sz0 = rgbarr_->getSize( !vertical_ );
    const int sz1 = rgbarr_->getSize( vertical_ );
    const ColTab::IndexedLookUpTable indextable( ctseq_, sz0 );
    for ( int idx=0; idx<sz0; idx++ )
    {
	const Color color = indextable.colorForIndex( idx );
	for ( int idy=0; idy<sz1; idy++ )
	{
	    if ( vertical_ )
		rgbarr_->set( idy, sz0-1-idx, color );
	    else
		rgbarr_->set( idx, idy, color );
	}
    }

    ioPixmap* pixmap = new ioPixmap( sz0, sz1 );
    pixmap->convertFromRGBArray( *rgbarr_ );
    setPixmap( *pixmap );
    draw();
}
