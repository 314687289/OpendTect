/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiobjectitemview.cc,v 1.10 2010-04-16 11:45:31 cvsbruno Exp $";


#include "uiobjectitemview.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"

#include "geometry.h"


uiObjectItemView::uiObjectItemView( uiParent* p )
    : uiGraphicsView(p,"Object Item Viewer")
{
    uiGraphicsObjectScene* newscene = new uiGraphicsObjectScene("Object Scene");
    setScene( *newscene );
    reSize.notify( mCB(this,uiObjectItemView,resetViewArea) );
} 


#define mGetScene(act)\
    mDynamicCastGet(uiGraphicsObjectScene*,sc,&scene())\
    if ( !sc ) act;
void uiObjectItemView::resetViewArea( CallBacker* )
{
    int w = 0, h = 0;
    for ( int idx=0; idx<objectitems_.size(); idx++)
    {
	w += objectitems_[idx]->objectSize().width();
	h = objectitems_[idx]->objectSize().height();
    }
    mGetScene(return); 
    setViewArea( 0, 0, w + sc->layoutPos().x, h + sc->layoutPos().y );
}


void uiObjectItemView::setSceneLayoutPos( float x, float y )
{
    mGetScene(return); sc->setLayoutPos( x, y );
}


void uiObjectItemView::addItem( uiObjectItem* itm, int stretch )
{
    objectitems_ += itm;
    mGetScene(return) sc->addObjectItem(itm);
    sc->setItemStretch( itm, stretch );
}


void uiObjectItemView::insertItem( uiObjectItem* itm, int stretch, int posidx)
{
    objectitems_.insertAt( itm, posidx );
    mGetScene(return) 
    sc->insertObjectItem( posidx, itm );
    sc->setItemStretch( itm, stretch );
}


void uiObjectItemView::removeItem( uiObjectItem* itm )
{
    objectitems_ -= itm;
    mGetScene(return) sc->removeObjectItem( itm );
}


uiObjectItem* uiObjectItemView::getItem( int idx ) 
{
    return ( idx>=0 && nrItems()>idx ) ? objectitems_[idx] : 0; 
}


uiObjectItem* uiObjectItemView::getItemFromPos( const Geom::Point2D<int>& pos ) 
{
    mGetScene(return 0)
    //TODO in the y direction
    Interval<int> borders(0,sc->layoutPos().x); 
    for ( int idx=0; idx<objectitems_.size(); idx++ )
    {
	borders.stop += objectitems_[idx]->objectSize().width(); 
	if ( borders.includes( pos.x ) ) return objectitems_[idx];
	borders.start = borders.stop;
    }
    return 0;
}


void uiObjectItemView::getItemsFromRect( const uiRect& rect, 
				       ObjectSet<uiObjectItem>& objs ) 
{
    Interval<float> rectborders( rect.left(), rect.right() );
    Interval<int> objborders(0,0); 
    for ( int idx=0; idx<objectitems_.size(); idx++ )
    {
	objborders.stop += objectitems_[idx]->objectSize().width();
	if ( rectborders.includes( objborders.start ) ||
	   		rectborders.includes( objborders.stop ) )	
	    objs += objectitems_[idx];
	objborders.start = objborders.stop;
    }
}


int uiObjectItemView::stretchFactor( uiObjectItem* itm )
{
    mGetScene(return 0) 
    return sc->stretchFactor( itm );
}


void uiObjectItemView::setStretchFactor(uiObjectItem* itm,int stretchfactor )
{
    mGetScene(return) sc->setItemStretch( itm, stretchfactor );
}

