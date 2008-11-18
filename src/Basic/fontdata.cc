/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : May 2001
-*/

static const char* rcsID = "$Id: fontdata.cc,v 1.1 2008-11-18 10:52:12 cvsbert Exp $";

#include "fontdata.h"
#include "separstr.h"


DefineEnumNames(FontData,Weight,2,"Font weight")
    { "Light", "Normal", "Demi-Bold", "Bold", "Black", 0 };
const char* FontData::universalfamilies[] =
    { "Helvetica", "Courier", "Times", 0 };
BufferString		FontData::defaultfamily(FontData::universalfamilies[0]);
int			FontData::defaultpointsize = 12;
FontData::Weight	FontData::defaultweight = FontData::Bold;
bool			FontData::defaultitalic = false;
const char* FontData::defaultkeys[] =
{ "Control",
  "Graphics medium", "Graphics small", "Graphics large",
  "Small control", "Large control", "Fixed width", 0 };
static const int numwghts[] = { 25, 50, 63, 75, 87, 0 };
int FontData::numWeight( FontData::Weight w )
{ return numwghts[(int)w]; }
FontData::Weight FontData::enumWeight( int w )
{
    int idx = 0;
    while ( numwghts[idx] && numwghts[idx] < w ) idx++;
    if ( !numwghts[idx] ) idx--;
    return (FontData::Weight)idx;
}

const char* const* FontData::universalFamilies() { return universalfamilies; }
const char* const* FontData::defaultKeys()	 { return defaultkeys; }

const char* FontData::defaultFamily()		{ return defaultfamily; }
int FontData::defaultPointSize()		{ return defaultpointsize; }
FontData::Weight FontData::defaultWeight()	{ return defaultweight; }
bool FontData::defaultItalic() 			{ return defaultitalic; }

void FontData::setDefaultFamily( const char* f)	{ defaultfamily = f; }
void FontData::setDefaultPointSize( int ps )	{ defaultpointsize = ps; }
void FontData::setDefaultWeight( Weight w )	{ defaultweight = w; }
void FontData::setDefaultItalic( bool yn )      { defaultitalic = yn; }

const char* FontData::key( StdSz ss )		{ return defaultkeys[(int)ss];}


void FontData::getFrom( const char* s )
{
    FileMultiString fms( s );
    const int nr = fms.size();
    if ( nr < 1 ) return;

    family_ = fms[0];
    if ( nr > 1 ) pointsize_ = atoi( fms[1] );
    if ( nr > 2 ) weight_ = eEnum(FontData::Weight,fms[2]);
    if ( nr > 3 ) italic_ = yesNoFromString(fms[3]);
}


void FontData::putTo( BufferString& s )
{
    FileMultiString fms;
    fms += family_;
    fms += pointsize_;
    fms += eString(FontData::Weight,weight_);
    fms += getYesNoString( italic_ );
    s = fms;
}
