/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 18-4-1996
-*/


/*! \brief Several implementations for UI-related things.

The main chunk is color table related.
*/

#include "draw.h"
#include "separstr.h"
#include "bufstringset.h"
#include "iopar.h"

// Beware if you add or change: there is a 'mirror' in
// uiGraphicsItem::setFillPattern. Not an enum to keep full flexibility.

static const int cDotsFillPatternType = 1;
static const int cLinesFillPatternType = 2;

mDefineEnumUtils(OD::Alignment,HPos,"OD::Alignment")
{ "Left", "Right", "Center", 0 };
mDefineEnumUtils(OD::Alignment,VPos,"OD::Alignment")
{ "Top", "Bottom", "Center", 0 };
mDefineEnumUtils(OD::MarkerStyle2D,Type,"Marker type")
{ "None", "Square", "Circle", "Cross", "Plus", "Target",
  "Horizontal line", "Vertical line", "Plane", "Triangle", "Arrow", 0 };
mDefineEnumUtils(OD::MarkerStyle3D,Type,"Marker type")
{ "None", "Cube", "Cone", "Cylinder", "Sphere", "Arrow", "Cross",
  "Point", "Plane", 0 };
mDefineEnumUtils(OD::LineStyle,Type,"Line style")
{ "None", "Solid", "Dashed", "Dotted", "Dash-Dotted", "Dash-Dot-Dotted",0 };

OD::Alignment::Alignment( HPos h, VPos v )
    : hor_(h), ver_(v)                                  {}
OD::Alignment::Alignment( Pos h, Pos v )
    : hor_(h==Start?Left:(h==Stop?Right:HCenter))
    , ver_(v==Start?Top:(v==Stop?Bottom:VCenter))       {}


OD::Alignment::HPos OD::Alignment::opposite( HPos p )
{ return p == Left ? Right : (p == Right ? Left : HCenter); }


OD::Alignment::VPos OD::Alignment::opposite( VPos p )
{ return p == Top ? Bottom : (p == Bottom ? Top : VCenter); }

OD::Alignment::Pos OD::Alignment::pos( bool hor ) const
{
    if ( hor )
	return hor_ == Left ? Start : (hor_ == Right ? Stop : Center);
    return ver_ == Top ? Start : (ver_ == Bottom ? Stop : Center);
}


void OD::Alignment::set( OD::Alignment::Pos h, OD::Alignment::Pos v )
{
    hor_ = h == Start ? Left : (h == Stop ? Right : HCenter);
    ver_ = v == Start ? Top : (v == Stop ? Bottom : VCenter);
}


int OD::Alignment::uiValue() const
{
    int ret = hor_ == Left ? 0x0001 : (hor_ == Right ? 0x0002 : 0x0004);
    ret |= ver_ == Top ? 0x0020 : (ver_ == Bottom ? 0x0040 : 0x0080);
    return ret;
}


void OD::Alignment::setUiValue( int v )
{
    hor_ = v&0x0001 ? Left : (v&0x0002 ? Right : HCenter);
    ver_ = v&0x0020 ? Top : (v&0x0040 ? Bottom : VCenter);
}


#define mToStringImpl( clss, par ) \
void clss::toString( BufferString& bs ) const \
{ \
    FileMultiString fms; \
    fms = toString(type_); \
    fms += par; \
    color_.fill( bs ); \
    fms += FileMultiString(bs); \
    bs = fms; \
}


#define mFromStringImpl( clss, par ) \
void clss::fromString( const char* s ) \
{ \
    FileMultiString fms( s ); \
    TypeDef().parse( fms[0], type_ ); \
    par = fms.getIValue( 1 ); \
    FileMultiString colfms( fms.from(2) ); \
    color_.use( colfms ); \
}


mToStringImpl( OD::MarkerStyle2D, size_ )
mToStringImpl( OD::LineStyle, width_ )

mFromStringImpl( OD::MarkerStyle2D, size_ )
mFromStringImpl( OD::LineStyle, width_ )

OD::MarkerStyle2D::MarkerStyle2D(Type tp, int sz, Color col, float rot )
    : type_(tp), size_(sz), color_(col), rotation_(rot)
{}


bool OD::MarkerStyle2D::operator==( const OD::MarkerStyle2D& b ) const
{
    return type_==b.type_ && size_==b.size_ && color_==b.color_ &&
	mIsEqual(rotation_,b.rotation_,mDefEps);
}


const OD::MarkerStyle2D& OD::MarkerStyle2D::operator=(
    const OD::MarkerStyle2D& a )
{
    type_ = a.type_ ; size_ = a.size_; color_ = a.color_;
    rotation_ = a.rotation_; return *this;
}


bool OD::MarkerStyle2D::isVisible() const
{ return type_!=None && size_>0 && color_.isVisible(); }


OD::MarkerStyle3D::MarkerStyle3D(Type tp, int sz, Color col )
    : type_(tp), size_(sz), color_(col)
{}


void OD::MarkerStyle3D::toString( BufferString& bs ) const
{
    FileMultiString fms;
    fms = toString( type_ );
    fms += size_;
    color_.fill( bs );
    fms += FileMultiString( bs );
    bs = fms;
}


void OD::MarkerStyle3D::fromString( const char* s, bool v6_or_earlier )
{
    FileMultiString fms( s );
    TypeDef().parse( fms[0], type_ );

    if ( v6_or_earlier )
    {
	const int typeidx = TypeDef().indexOf( type_ ) + 1;
	if ( typeidx<TypeDef().size() )
	    type_ = TypeDef().getEnumForIndex( typeidx );
    }

    size_ = fms.getIValue(1);
    FileMultiString colfms( fms.from(2) );
    color_.use( colfms );
}


bool OD::MarkerStyle3D::operator==(const OD::MarkerStyle3D& b) const
{ return type_==b.type_ && size_==b.size_ && color_==b.color_; }


bool OD::MarkerStyle3D::operator!=(const OD::MarkerStyle3D& b) const
{ return !(*this==b); }


bool OD::MarkerStyle3D::isVisible() const
{ return type_!=None && size_>0 && color_.isVisible(); }


OD::MarkerStyle2D::Type OD::MarkerStyle3D::getMS2DType(
	MarkerStyle3D::Type ms3d )
{
    switch ( ms3d )
    {
	case None: 	return MarkerStyle2D::None;
	case Cube:	return MarkerStyle2D::Square;
	case Cone:	return MarkerStyle2D::Triangle;
	case Cylinder:	return MarkerStyle2D::Square;
	case Sphere:	return MarkerStyle2D::Circle;
	case Arrow:	return MarkerStyle2D::Arrow;
	case Cross:	return MarkerStyle2D::Cross;
	case Point:	return MarkerStyle2D::Target;
	case Plane:	return MarkerStyle2D::HLine;
	break;
    }

    return MarkerStyle2D::None;
}


OD::LineStyle::LineStyle( Type t, int w, Color c )
    : type_(t), width_(w), color_(c)
{}


bool OD::LineStyle::operator ==( const OD::LineStyle& ls ) const
{ return type_ == ls.type_ && width_ == ls.width_ && color_ == ls.color_; }


bool OD::LineStyle::operator !=( const OD::LineStyle& ls ) const
{ return !(*this == ls); }


bool OD::LineStyle::isVisible() const
{ return type_!=None && width_>0 && color_.isVisible();}


void OD::FillPattern::getTypeNames( BufferStringSet& res )
{
    res.add( "No Fill" );
    res.add( "Dots" );
    res.add( "Lines" );
}


void OD::FillPattern::getOptNames( int typ, BufferStringSet& res )
{
    res.setEmpty();
    if ( typ == cDotsFillPatternType )
    {
	res.add( "Uniform color" );
	res.add( "Extremely dense" );
	res.add( "Very dense" );
	res.add( "Somewhat dense" );
	res.add( "Half dense" );
	res.add( "Somewhat sparse" );
	res.add( "Very sparse" );
	res.add( "Extremely sparse" );
    }
    else if ( typ == cLinesFillPatternType )
    {
	res.add( "Horizontal lines" );
	res.add( "Vertical lines" );
	res.add( "Crossing horizontal and vertical lines" );
	res.add( "Backward diagonal lines" );
	res.add( "Forward diagonal lines" );
	res.add( "Crossing diagonal lines" );
    }
    // else none
}



OD::ArrowHeadStyle::ArrowHeadStyle( int sz, Type t, HandedNess h )
    : sz_(sz), type_(t), handedness_(h)
{}


void OD::ArrowHeadStyle::setBoldNess( int b )
{ sz_ = 3*b; }



OD::ArrowStyle::ArrowStyle( int boldness, Type t )
    : type_(t)
    , linestyle_(OD::LineStyle::Solid,boldness)
{ setBoldNess(boldness); }


void OD::ArrowStyle::setBoldNess( int b )
{ linestyle_.width_ = b; headstyle_.setBoldNess(b); tailstyle_.setBoldNess(b); }


bool OD::ArrowStyle::hasHead() const
{ return headstyle_.sz_ > 0 && type_ < TailOnly; }


bool OD::ArrowStyle::hasTail() const
{ return tailstyle_.sz_ > 0 && (type_ == TwoSided || type_ == TailOnly); }
