/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          September 2006
 RCS:           $Id: uiworld2ui.cc,v 1.2 2007-02-06 17:15:13 cvskris Exp $
 ________________________________________________________________________

-*/

#include "uiworld2ui.h"

#include "errh.h"
#include "posgeom.h"
#include "ranges.h"
#include "linear.h"

World2UiData::World2UiData()
{}


World2UiData::World2UiData( uiSize s, const uiWorldRect& w )
    : sz(s), wr(w)
{}


World2UiData::World2UiData( const uiWorldRect& w, uiSize s )
    : sz(s), wr(w)
{}


bool World2UiData::operator ==( const World2UiData& w2ud ) const
{
    return sz == w2ud.sz
	    && crd(wr.topLeft()) == crd(w2ud.wr.topLeft())
	    && crd(wr.bottomRight()) == crd(w2ud.wr.bottomRight());
}


bool World2UiData::operator !=( const World2UiData& w2ud ) const
{ return !( *this == w2ud ); }


uiWorld2Ui::uiWorld2Ui()
{ };


uiWorld2Ui::uiWorld2Ui( uiSize sz, const uiWorldRect& wr )
{ set(sz,wr); }


uiWorld2Ui::uiWorld2Ui( const uiWorldRect& wr, uiSize sz )
{ set(sz,wr); }


uiWorld2Ui::uiWorld2Ui( const World2UiData& w )
{ set( w.sz, w.wr ); }


bool uiWorld2Ui::operator==( const uiWorld2Ui& ) const
{
    pErrMsg("Not implemented.");
    return false;
}


void uiWorld2Ui::set( const World2UiData& w )
{ set( w.sz, w.wr ); }


void uiWorld2Ui::set( const uiWorldRect& wr, uiSize sz )
{ set( sz, wr ); }


void uiWorld2Ui::set( uiSize sz, const uiWorldRect& wr )
{
    w2ud = World2UiData( sz, wr );
    p0 = wr.topLeft();
    fac.setXY( wr.width()/(sz.hNrPics()-1),
    wr.height()/(sz.vNrPics()-1));
    if ( wr.left() > wr.right() ) fac.x = -fac.x;
    if ( wr.top() > wr.bottom() ) fac.y = -fac.y;
    wrdrect_ = wr;
    uisize_ = sz;
    uiorigin.x = 0; uiorigin.y = 0;
}


void uiWorld2Ui::set( const uiRect& rc, const uiWorldRect& wr )
{
    uiSize sz( rc.hNrPics(), rc.vNrPics(), true );
    set( sz, wr );
    uiorigin = rc.topLeft();
}


void uiWorld2Ui::setRemap( const uiSize& sz, const uiWorldRect& wrdrc )
{
    uiWorldRect wr = wrdrc;
    // Recalculate a 'good' left/right boundary
    float left, right;
    getAppopriateRange(wr.left(), wr.right(), left, right);
    wr.setRight( right ); wr.setLeft( left );
    // recalculate a 'good' top/bottom boundary
    float top, bot;
    getAppopriateRange(wr.bottom(), wr.top(), bot, top);
    wr.setTop( top ); wr.setBottom( bot );

    set( sz, wr );
}


void uiWorld2Ui::setRemap( const uiRect& rc, const uiWorldRect& wrdrc )
{
    uiSize sz( rc.hNrPics(), rc.vNrPics(), true );
    setRemap( sz, wrdrc );
    uiorigin = rc.topLeft();
}


void uiWorld2Ui::setCartesianRemap( const uiSize& sz,
float minx, float maxx, float miny, float maxy )
{
    uiWorldRect wr = xyRng2WrdRect(minx,maxx,miny,maxy);
    setRemap( sz, wr );
}


void uiWorld2Ui::setCartesianRemap( const uiRect& rc,
float minx, float maxx, float miny, float maxy )
{
    uiWorldRect wr = xyRng2WrdRect(minx,maxx,miny,maxy);
    setRemap( rc, wr );
}


void uiWorld2Ui::resetUiSize( const uiSize& sz )
{ set( sz, wrdrect_ ); }


void uiWorld2Ui::resetUiRect( const uiRect& rc )
{ set( rc, wrdrect_ ); }


void uiWorld2Ui::resetWorldRect( const uiWorldRect& wr )
{ set( uisize_, wr ); }


void uiWorld2Ui::resetWorldRectRemap( const uiWorldRect& wr )
{ setRemap( uisize_, wr ); }


uiWorld2Ui uiWorld2Ui::getMirrowed( bool lr, bool tb ) const
{
    World2UiData newdata = w2ud;
    if ( lr ) newdata.wr.swapHor();
    if ( tb ) newdata.wr.swapVer();

    return uiWorld2Ui( newdata );
}


const World2UiData& uiWorld2Ui::world2UiData() const
{ return w2ud; }


uiWorldPoint uiWorld2Ui::transform( uiPoint p ) const
{
    return uiWorldPoint( toWorldX( p.x ), toWorldY( p.y ) );
}


uiWorldRect uiWorld2Ui::transform( uiRect area ) const
{
    return uiWorldRect( transform(area.topLeft()),
			transform(area.bottomRight()) );
}


uiPoint uiWorld2Ui::transform( uiWorldPoint p ) const
{
    return uiPoint( toUiX( p.x ), toUiY( p.y ) );
}


uiRect uiWorld2Ui::transform( uiWorldRect area ) const
{
    return uiRect( transform(area.topLeft()),
		   transform(area.bottomRight()) );
}

// Since the compiler will be comfused if two functions
// only differ in return type, Geom::Point2D<float> is
// set rather than be returned.


void uiWorld2Ui::transform( const uiPoint& upt,
			    Geom::Point2D<float>& pt ) const
{ pt.setXY( toWorldX(upt.x), toWorldY(upt.y) ); }


void uiWorld2Ui::transform( const Geom::Point2D<float>& pt, uiPoint& upt) const
{  upt.setXY( toUiX( pt.x ), toUiY( pt.y ) ); };


int uiWorld2Ui::toUiX ( float wrdx ) const
{ return (int)((wrdx-p0.x)/fac.x+uiorigin.x+.5); }


int uiWorld2Ui::toUiY ( float wrdy ) const
{ return (int)((wrdy-p0.y)/fac.y+uiorigin.y+.5); }


float uiWorld2Ui::toWorldX ( int uix ) const
{ return p0.x + (uix-uiorigin.x)*fac.x; }


float uiWorld2Ui::toWorldY ( int uiy ) const
{ return p0.y + (uiy-uiorigin.y)*fac.y; }


uiWorldRect uiWorld2Ui::xyRng2WrdRect( float minx, float maxx,
				       float miny, float maxy )
{ return uiWorldRect( minx, maxy, maxx, miny ); }


uiWorldPoint uiWorld2Ui::origin() const
{ return p0; }


uiWorldPoint uiWorld2Ui::worldPerPixel() const
{ return fac; }


void uiWorld2Ui::getWorldXRange( float& xmin, float& xmax ) const
{
    xmax=wrdrect_.right();  xmin=wrdrect_.left();
    if ( xmin > xmax ) Swap( xmax, xmin ); 
}


void uiWorld2Ui::getWorldYRange( float& ymin, float& ymax ) const
{
    ymax=wrdrect_.top();  ymin=wrdrect_.bottom();
    if ( ymin > ymax ) Swap( ymax, ymin ); 
}


void uiWorld2Ui::getRecmMarkStep( float& xstep, float& ystep ) const
{
    float min, max;
    getWorldXRange( min, max );
    getRecmMarkStep( min, max, xstep );
    getWorldYRange( min, max );
    getRecmMarkStep( min, max, ystep );
}


void uiWorld2Ui::getAppopriateRange( float min, float max,
				     float& newmin, float& newmax )
{
    bool rev = min > max;
    if ( rev )	Swap( min, max );
    Interval<float> intv( min, max );
    AxisLayout al( intv );
    newmin = al.sd.start;
    newmax = al.findEnd( max );
    if ( rev )	Swap( newmin, newmax );
}


void uiWorld2Ui::getRecmMarkStep( float min, float max,
				  float& step ) const
{
    if ( min > max ) Swap( min, max );
    Interval<float> intv( min, max );
    AxisLayout al( intv );
    step = al.sd.step;
}
