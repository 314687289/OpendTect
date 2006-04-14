/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uisellinest.cc,v 1.15 2006-04-14 08:24:28 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uisellinest.h"
#include "draw.h"
#include "uilabel.h"
#include "uicombobox.h"
#include "uicolor.h"
#include "uispinbox.h"
#include "bufstringset.h"


static const int sMinWidth = 1;
static const int sMaxWidth = 10;

uiSelLineStyle::uiSelLineStyle( uiParent* p, const LineStyle& ls,
				const char* txt, bool wdraw, bool wcol, 
				bool wwidth )
    : uiGroup(p,"Line style selector")
    , linestyle(*new LineStyle(ls))
    , stylesel(0)
    , colinp(0)
    , widthbox(0)
    , changed(this)
{
    if ( wdraw )
    {
	BufferStringSet itms( LineStyle::TypeNames );
	stylesel = new uiComboBox( this, itms, "Line Style" );
	stylesel->setCurrentItem( (int)linestyle.type );
	stylesel->selectionChanged.notify( mCB(this,uiSelLineStyle,changeCB) );
	new uiLabel( this, txt, stylesel );
    }

    if ( wcol )
    {
	colinp = new uiColorInput( this, linestyle.color );
	colinp->colorchanged.notify( mCB(this,uiSelLineStyle,changeCB) );
	if ( stylesel )
	    colinp->attach( rightTo, stylesel );
	else
	    new uiLabel( this, txt, colinp );
    }

    if ( wwidth )
    {
	widthbox = new uiLabeledSpinBox( this, "Width" );
	widthbox->box()->valueChanged.notify( 
					mCB(this,uiSelLineStyle,changeCB) );
	widthbox->box()->setValue( linestyle.width );
	widthbox->box()->setMinValue( sMinWidth );
  	widthbox->box()->setMaxValue( sMaxWidth );
	if ( colinp )
	    widthbox->attach( rightTo, colinp );
	else if ( stylesel )
	    widthbox->attach( rightTo, stylesel );
    }

    setHAlignObj( stylesel ? (uiObject*)stylesel 
	    		   : ( colinp ? (uiObject*)colinp->attachObj() 
			       	      : (uiObject*)widthbox ) );
    setHCentreObj( stylesel ? (uiObject*)stylesel
	    		    : ( colinp ? (uiObject*)colinp->attachObj()
				       : (uiObject*)widthbox ) );
}


uiSelLineStyle::~uiSelLineStyle()
{
    delete &linestyle;
}


const LineStyle& uiSelLineStyle::getStyle() const
{
    return linestyle;
}


void uiSelLineStyle::setStyle( const LineStyle& ls )
{
    setColor( ls.color );
    setWidth( ls.width );
    setType( (int)ls.type );
}


void uiSelLineStyle::setColor( const Color& col )
{
    linestyle.color = col;
    if ( colinp ) colinp->setColor( col );
}


const Color& uiSelLineStyle::getColor() const
{
    return linestyle.color;
}


void uiSelLineStyle::setWidth( int width )
{
    linestyle.width = width;
    if ( widthbox ) widthbox->box()->setValue( width );
}


int uiSelLineStyle::getWidth() const
{
    return linestyle.width;
}


void uiSelLineStyle::setType( int tp )
{
    linestyle.type = (LineStyle::Type)tp;
    if ( stylesel ) stylesel->setCurrentItem( tp );
}


int uiSelLineStyle::getType() const
{
    return (int)linestyle.type;
}


void uiSelLineStyle::changeCB( CallBacker* cb )
{
    if ( stylesel )
	linestyle.type = (LineStyle::Type)stylesel->currentItem();
    if ( colinp ) 
	linestyle.color = colinp->color();
    if ( widthbox ) 
	linestyle.width = widthbox->box()->getValue();
    changed.trigger(cb);
}


// ========================================================

LineStyleDlg::LineStyleDlg( uiParent* p, const LineStyle& ls, const char* lbl,
			    bool wdraw, bool wcol, bool wwidth )
        : uiDialog(p,uiDialog::Setup("Display","Select linestyle"))
{   
    lsfld = new uiSelLineStyle( this, ls, lbl ? lbl : "Line style", 
				wdraw, wcol, wwidth ); 
}


const LineStyle& LineStyleDlg::getLineStyle() const
{   
    return lsfld->getStyle();
}

