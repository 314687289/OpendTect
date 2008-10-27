/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id: flatviewaxesdrawer.cc,v 1.9 2008-10-27 11:21:08 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "flatviewaxesdrawer.h"
#include "flatview.h"
#include "flatposdata.h"
#include "datapackbase.h"
#include "iodraw.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"


FlatView::AxesDrawer::AxesDrawer( FlatView::Viewer& vwr, uiGraphicsView& view )
    : DrawAxis2D( *new DrawAxis2D(&view) )
    , vwr_(vwr)
    , altdim0_(mUdf(int))
{
}


void FlatView::AxesDrawer::draw( uiRect uir, uiWorldRect wr )
{
    setDrawRectangle( &uir ); setup( wr );

    const FlatView::Annotation& annot = vwr_.appearance().annot_;
    const FlatView::Annotation::AxisData& ad1 = annot.x1_;
    const FlatView::Annotation::AxisData& ad2 = annot.x2_;

    //drawarea_->drawTool().setPenColor( annot.color_ );

    drawAxes( ad1.showannot_, ad2.showannot_, true, true );
    drawGridLines( ad1.showgridlines_, ad2.showgridlines_ );
}


double FlatView::AxesDrawer::getAnnotTextAndPos( bool isx, double pos,
						 BufferString* txt ) const
{
    bool usewva = !vwr_.isVisible( false );
    const FlatDataPack* fdp = vwr_.pack( usewva );
    if ( !fdp )
	{ usewva = !usewva; fdp = vwr_.pack( usewva ); }

    if ( !isx || mIsUdf(altdim0_) || !fdp )
	return ::DrawAxis2D::getAnnotTextAndPos( isx, pos, txt );

    const FlatPosData& pd = fdp->posData();
    IndexInfo idxinfo( pd.indexInfo( true, pos ) );
    pos = pd.position( true, idxinfo.nearest_ );

    if ( txt )
	*txt = fdp->getAltDim0Value( altdim0_, idxinfo.nearest_ );
    return pos;
}
