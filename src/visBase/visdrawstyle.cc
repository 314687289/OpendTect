/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: visdrawstyle.cc,v 1.7 2005-02-07 12:45:40 nanne Exp $
________________________________________________________________________

-*/

#include "visdrawstyle.h"
#include "iopar.h"

#include <Inventor/nodes/SoDrawStyle.h>

mCreateFactoryEntry( visBase::DrawStyle );

namespace visBase
{

DefineEnumNames( DrawStyle, Style, 1, "Style" )
{ "Filled", "Lines", "Points", "Invisible", 0 };

const char* DrawStyle::linestylestr = "Line Style";
const char* DrawStyle::drawstylestr = "Draw Style";
const char* DrawStyle::pointsizestr = "Point Size";

DrawStyle::DrawStyle()
    : drawstyle( new SoDrawStyle )
{
    drawstyle->ref();
}


DrawStyle::~DrawStyle()
{
    drawstyle->unref();
}


void DrawStyle::setDrawStyle( Style s )
{
    if ( s==Filled ) drawstyle->style = SoDrawStyle::FILLED;
    else if ( s==Lines ) drawstyle->style = SoDrawStyle::LINES;
    else if ( s==Points ) drawstyle->style = SoDrawStyle::POINTS;
    else if ( s==Invisible ) drawstyle->style = SoDrawStyle::INVISIBLE;
}


DrawStyle::Style DrawStyle::getDrawStyle() const
{
    SoDrawStyle::Style style = (SoDrawStyle::Style) drawstyle->style.getValue();
    if ( style==SoDrawStyle::FILLED ) return Filled;
    if ( style==SoDrawStyle::LINES ) return Lines;
    if ( style==SoDrawStyle::POINTS ) return Points;
    return Invisible;
}


void DrawStyle::setPointSize( float nsz )
{
    drawstyle->pointSize.setValue( nsz );
}


float DrawStyle::getPointSize() const
{
    return drawstyle->pointSize.getValue();
}


void DrawStyle::setLineStyle( const LineStyle& nls )
{
    linestyle = nls;
    updateLineStyle();
}



SoNode* DrawStyle::getInventorNode()
{
    return drawstyle;
}


void DrawStyle::updateLineStyle()
{
    drawstyle->lineWidth.setValue( linestyle.width );

    unsigned short pattern;

    if ( linestyle.type==LineStyle::None )      pattern = 0;
    else if ( linestyle.type==LineStyle::Solid )pattern = 0xFFFF;
    else if ( linestyle.type==LineStyle::Dash ) pattern = 0xF0F0;
    else if ( linestyle.type==LineStyle::Dot )  pattern = 0xAAAA;
    else if ( linestyle.type==LineStyle::DashDot ) pattern = 0xF6F6;
    else pattern = 0xEAEA;

    drawstyle->linePattern.setValue( pattern );
}

int DrawStyle::usePar( const IOPar& par )
{
    const char* linestylepar = par.find( linestylestr );
    if ( !linestylepar ) return -1;

    linestyle.fromString( linestylepar );
    updateLineStyle();

    const char* stylepar = par.find( drawstylestr );
    if ( !stylepar ) return -1;

    int enumid = getEnumDef( stylepar, StyleNames, 0, 1, -1 );
    if ( enumid<0 ) return -1;

    setDrawStyle( (Style) enumid );

    float pointsize;
    if ( !par.get( pointsizestr, pointsize ) )
	return -1;
    setPointSize( pointsize );

    return 1;
}


void DrawStyle::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( par, saveids );

    BufferString linestyleval;
    linestyle.toString( linestyleval );
    par.set( linestylestr, linestyleval );

    par.set( drawstylestr, StyleNames[(int) getDrawStyle()] );
    par.set( pointsizestr, getPointSize() );
}

}; // namespace visBase
