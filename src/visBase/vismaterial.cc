/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: vismaterial.cc,v 1.15 2007-10-10 03:59:24 cvsnanne Exp $
________________________________________________________________________

-*/

#include "vismaterial.h"
#include "iopar.h"

#include <Inventor/nodes/SoMaterial.h>

mCreateFactoryEntry( visBase::Material );

namespace visBase
{

const char* Material::sKeyColor()		{ return "Color"; }
const char* Material::sKeyAmbience()		{ return "Ambient Intensity"; }
const char* Material::sKeyDiffIntensity()	{ return "Diffuse Intensity"; }
const char* Material::sKeySpectralIntensity()	{ return "Specular Intensity"; }
const char* Material::sKeyEmmissiveIntensity()	{ return "Emmissive Intensity";}
const char* Material::sKeyShininess()		{ return "Shininess"; }
const char* Material::sKeyTransparency()	{ return "Transparency"; }

Material::Material()
    : material_( new SoMaterial )
    , ambience_( 0.8 )
    , specularintensity_( 0 )
    , emmissiveintensity_( 0 )
    , shininess_( 0 )
    , transparency_( 0 )
    , change( this )
{
    material_->ref();
    setMinNrOfMaterials(0);
    updateMaterial(0);
}


Material::~Material()
{ material_->unref(); }

#define mSetProp( prop ) prop = mat.prop
void Material::setFrom( const Material& mat )
{
    mSetProp( color_ );
    mSetProp( diffuseintencity_ );
    mSetProp( ambience_ );
    mSetProp( specularintensity_ );
    mSetProp( emmissiveintensity_ );
    mSetProp( shininess_ );
    mSetProp( transparency_ );

    for ( int idx=color_.size()-1; idx>=0; idx-- )
	updateMaterial( idx );
}


void Material::setColor( const Color& n, int idx )
{
    setMinNrOfMaterials(idx);
    color_[idx] = n;
    updateMaterial( idx );
}


const Color& Material::getColor( int idx ) const
{
    if ( idx>=0 && idx<color_.size() )
	return color_[idx];
    return color_[0];
}


void Material::setDiffIntensity( float n, int idx )
{
    setMinNrOfMaterials(idx);
    diffuseintencity_[idx] = n;
    updateMaterial( idx );
}


float Material::getDiffIntensity( int idx ) const
{
    if ( idx>=0 && idx<diffuseintencity_.size() )
	return diffuseintencity_[idx];
    return diffuseintencity_[0];
}


#define mSetGetProperty( Type, func, var ) \
void Material::set##func( Type n ) \
{ \
    var = n; \
    updateMaterial( 0 ); \
} \
Type Material::get##func() const \
{ \
    return var; \
}


mSetGetProperty( float, Ambience, ambience_ );
mSetGetProperty( float, SpecIntensity, specularintensity_ );
mSetGetProperty( float, EmmIntensity, emmissiveintensity_ );
mSetGetProperty( float, Shininess, shininess_ );
mSetGetProperty( float, Transparency, transparency_ );


void Material::updateMaterial(int idx)
{
    if ( !idx )
    {
	material_->ambientColor.set1Value( 0, color_[0].r() * ambience_/255,
					     color_[0].g() * ambience_/255,
					     color_[0].b() * ambience_/255 );
	material_->specularColor.set1Value( 0,
		    color_[0].r() * specularintensity_/255,
		    color_[0].g() * specularintensity_/255,
		    color_[0].b() * specularintensity_/255 );

	material_->emissiveColor.set1Value( idx,
		    color_[0].r() * emmissiveintensity_/255,
		    color_[0].g() * emmissiveintensity_/255,
		    color_[0].b() * emmissiveintensity_/255);

	material_->shininess.set1Value(0, shininess_ );
	material_->transparency.set1Value(0, transparency_ );
    }

    material_->diffuseColor.set1Value( idx,
		color_[idx].r() * diffuseintencity_[idx]/255,
		color_[idx].g() * diffuseintencity_[idx]/255,
		color_[idx].b() * diffuseintencity_[idx]/255 );

    change.trigger();
}


void Material::setMinNrOfMaterials(int minnr)
{
    while ( color_.size()<=minnr )
    {
	color_ += Color(179,179,179);
	diffuseintencity_ += 0.8;
    }
}


SoNode* Material::getInventorNode() { return material_; }


int Material::usePar( const IOPar& iopar )
{
    int res = DataObject::usePar( iopar );
    if ( res!=1 ) return res;

    int r,g,b;

    if ( iopar.get( sKeyColor(), r, g, b ) )
	setColor( Color( r,g,b ));

    float val;
    if ( iopar.get( sKeyAmbience(), val ))
	setAmbience( val );

    if ( iopar.get( sKeyDiffIntensity(), val ))
	setDiffIntensity( val );

    if ( iopar.get( sKeySpectralIntensity(), val ))
	setSpecIntensity( val );

    if ( iopar.get( sKeyEmmissiveIntensity(), val ))
	setEmmIntensity( val );

    if ( iopar.get( sKeyShininess(), val ))
	setShininess( val );

    if ( iopar.get( sKeyTransparency(), val ))
	setTransparency( val );

    return 1;
}


void Material::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( iopar, saveids );

    Color tmpcolor = getColor();
    iopar.set( sKeyColor(), tmpcolor.r(), tmpcolor.g(), tmpcolor.b() ) ;
    iopar.set( sKeyAmbience(), getAmbience() );
    iopar.set( sKeyDiffIntensity(), getDiffIntensity() );
    iopar.set( sKeySpectralIntensity(), getSpecIntensity() );
    iopar.set( sKeyEmmissiveIntensity(), getEmmIntensity() );
    iopar.set( sKeyShininess(), getShininess() );
    iopar.set( sKeyTransparency(), getTransparency() );
}

}; // namespace visBase
