/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Apr 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: vistext.cc,v 1.17 2008-11-25 15:35:27 cvsbert Exp $";

#include "vistext.h"

#include "iopar.h"
#include "vistransform.h"
#include "vismaterial.h"
#include "separstr.h"

#include <Inventor/nodes/SoFont.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoAsciiText.h>
#include <Inventor/nodes/SoTranslation.h>

mCreateFactoryEntry( visBase::Text2 );
mCreateFactoryEntry( visBase::TextBox );

namespace visBase
{

const char* Text::sKeyString() 		{ return "Text"; }
const char* Text::sKeyFontSize() 	{ return "Font size"; }
const char* Text::sKeyJustification() 	{ return "Justification"; }
const char* Text::sKeyPosition() 	{ return "Position"; }


Text::Text()
    : VisualObjectImpl( false )
    , textpos_(new SoTranslation)
    , font_(new SoFont)
    , transformation_( 0 )
{
    addChild( textpos_ );
    addChild( font_ );
}


Text::~Text()
{
    if ( transformation_ ) transformation_->unRef();
    removeChild( textpos_ );
    removeChild( font_ );
}


void Text::setPosition( const Coord3& lpos )
{
    const Coord3 pos = transformation_
	? transformation_->transform( lpos ) : lpos;
    textpos_->translation.setValue( pos.x, pos.y, pos.z );
}


Coord3 Text::position() const
{
    SbVec3f pos = textpos_->translation.getValue();
    Coord3 res( pos[0], pos[1], pos[2] );
    return transformation_ ? transformation_->transformBack( res ) : res;
}


void Text::setSize( float ns )
{
    font_->size.setValue( ns );
}


float Text::size() const
{
    return font_->size.getValue();
}


void Text::setDisplayTransformation( Transformation* nt )
{
    const Coord3 pos = position();

    if ( transformation_ ) transformation_->unRef();
    transformation_ = nt;
    if ( transformation_ ) transformation_->ref();
    setPosition( pos );
}


Transformation* Text::getDisplayTransformation()
{
    return transformation_;
}


void Text::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    par.set( sKeyPosition(), position() );
    par.set( sKeyJustification(), (int)justification() );
    par.set( sKeyFontSize(), size() );
    par.set( sKeyString(), getText() );
}


int Text::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    Coord3 pos;
    if ( !par.get(sKeyPosition(),pos) )
	return -1;

    int just = 1;
    par.get( sKeyJustification(), just );

    float fontsz = size();
    par.get( sKeyFontSize(), fontsz );

    BufferString str( "" );
    par.get( sKeyString(), str );

    setText( str );
    setPosition( pos );
    setSize( fontsz );
    setJustification( (Justification)just );

    return 1;
}


// Text2 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Text2::Text2()
    : Text()
    , text_(new SoText2)
{
    addChild( text_ );
    text_->ref();
}


Text2::~Text2()
{
    removeChild( text_ );
    text_->unref();
}


void Text2::setText( const char* newtext )
{
    text_->string.setValue( newtext );
}


const char* Text2::getText() const
{
    SbString val;
    text_->string.get( val );
    static BufferString res;
    res = val.getString();
    return res;
}


void Text2::setJustification( Justification just )
{
    if ( just==Center )
	text_->justification.setValue( SoText2::CENTER );
    else if ( just==Left )
	text_->justification.setValue( SoText2::LEFT );
    else
	text_->justification.setValue( SoText2::RIGHT );
}


Text::Justification Text2::justification() const
{
    if ( text_->justification.getValue() == SoText2::CENTER )
	return Center;
    if ( text_->justification.getValue() == SoText2::LEFT )
	return Left;

    return Right;
}


// TextBox +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TextBox::TextBox()
    : Text()
    , text_(new SoAsciiText)
{
    addChild( text_ );
    text_->ref();
}


TextBox::~TextBox()
{
    removeChild( text_ );
    text_->unref();
}


void TextBox::setText( const char* newtext )
{
    SeparString sepstr( newtext, '\n' );
    text_->string.deleteValues( sepstr.size() );
    for ( int idx=0; idx<sepstr.size(); idx++ )
	text_->string.set1Value( idx, sepstr[idx] );
}


const char* TextBox::getText() const
{
    static BufferString res;
    res = "";
    for ( int idx=0; idx<text_->string.getNum(); idx++ )
    {
	if ( idx>0 ) res += "\n";
	res += text_->string[idx].getString();
    }

    return res;
}


void TextBox::setJustification( Justification just )
{
    if ( just==Center )
	text_->justification.setValue( SoAsciiText::CENTER );
    else if ( just==Left )
	text_->justification.setValue( SoAsciiText::LEFT );
    else
	text_->justification.setValue( SoAsciiText::RIGHT );
}


Text::Justification TextBox::justification() const
{
    if ( text_->justification.getValue() == SoAsciiText::CENTER )
	return Center;
    if ( text_->justification.getValue() == SoAsciiText::LEFT )
	return Left;

    return Right;
}

}; // namespace visBase
