/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "vismaterial.h"
#include "visosg.h"
#include "iopar.h"

#include <osg/Material>
#include <osg/Array>

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
    : material_( new osg::Material )
    , colorarray_( 0 )
    , ambience_( 0.8 )
    , specularintensity_( 0 )
    , emmissiveintensity_( 0 )
    , shininess_( 0 )
    , change( this )
{
    material_->ref();
    material_->setColorMode( osg::Material::AMBIENT_AND_DIFFUSE  );
    setMinNrOfMaterials(0);
    updateMaterial(0);
}


Material::~Material()
{
    material_->unref();
    if ( colorarray_ ) colorarray_->unref();
}
    

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
    if ( color_[idx]==n )
	return;

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


void Material::setTransparency( float n, int idx )
{
    setMinNrOfMaterials(idx);
    transparency_[idx] = n;
    updateMaterial( idx );
}


float Material::getTransparency( int idx ) const
{
    if ( idx>=0 && idx<transparency_.size() )
	return transparency_[idx];
    return transparency_[0];
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


void Material::updateMaterial(int idx)
{
    const osg::Vec4 diffuse(color_[0].r() * diffuseintencity_[idx]/255,
			    color_[0].g() * diffuseintencity_[idx]/255,
			    color_[0].b() * diffuseintencity_[idx]/255,
			    transparency_[idx]);
    
    if ( !idx )
    {
	
	material_->setAmbient( osg::Material::FRONT_AND_BACK,
			      osg::Vec4(color_[0].r() * ambience_/255,
				   color_[0].g() * ambience_/255,
				   color_[0].b() * ambience_/255,
				   transparency_[idx]) );
	material_->setSpecular( osg::Material::FRONT_AND_BACK,
			      osg::Vec4(color_[0].r() * specularintensity_/255,
					color_[0].g() * specularintensity_/255,
					color_[0].b() * specularintensity_/255,
					transparency_[idx]) );
	material_->setEmission( osg::Material::FRONT_AND_BACK,
			      osg::Vec4(color_[0].r() * emmissiveintensity_/255,
					color_[0].g() * emmissiveintensity_/255,
					color_[0].b() * emmissiveintensity_/255,
					transparency_[idx]) );
	material_->setShininess(osg::Material::FRONT_AND_BACK, shininess_ );
	
	material_->setDiffuse(osg::Material::FRONT_AND_BACK, diffuse );
	if ( colorarray_ )
	{
	    osg::Vec4Array& colarr = *mGetOsgVec4Arr(colorarray_);
	    if ( colarr.size() )
		colarr[0] = diffuse;
	    else
		colarr.push_back( diffuse );
	}
    }
    else
    {
	if ( !colorarray_ )
	{
	    colorarray_ = new osg::Vec4Array;
	    colorarray_->ref();
	    mGetOsgVec4Arr(colorarray_)->push_back( diffuse );
	}
	else
	{
	    osg::Vec4Array& colarr = *mGetOsgVec4Arr(colorarray_);

	    if ( colarr.size()<=idx )
		colarr.resize( idx+1 );
	    
	    colarr[idx] = diffuse;
	}
    }

    change.trigger();
}


int Material::nrOfMaterial() const
{ return color_.size(); }


void Material::setMinNrOfMaterials(int minnr)
{
    while ( color_.size()<=minnr )
    {
	color_ += Color(179,179,179);
	diffuseintencity_ += 0.8;
	transparency_ += 0.0;
    }
}


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
    
    
osg::Material* Material::getMaterial()
{ return material_; }
    
    
const osg::Array* Material::getColorArray() const
{
    return colorarray_;
}
    

void Material::createArray()
{
    if ( colorarray_ )
	return;
    
    colorarray_ = new osg::Vec4Array;
    colorarray_->ref();
    mGetOsgVec4Arr(colorarray_)->
        push_back( material_->getDiffuse( osg::Material::FRONT ) );
}

}; // namespace visBase
