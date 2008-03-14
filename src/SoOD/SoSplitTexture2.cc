/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          December 2006
 RCS:           $Id: SoSplitTexture2.cc,v 1.11 2008-03-14 14:03:03 cvskris Exp $
________________________________________________________________________

-*/


#include "SoSplitTexture2.h"

#include <Inventor/misc/SoGLImage.h>
#include <Inventor/C/glue/gl.h>
#include <SoSplitTexture2Element.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoTextureQualityElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLTextureImageElement.h>
#include <Inventor/elements/SoGLTexture3EnabledElement.h>
#include <Inventor/elements/SoGLTextureEnabledElement.h>
#include <Inventor/elements/SoGLMultiTextureImageElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include "Inventor/actions/SoGLRenderAction.h"
#include "Inventor/actions/SoCallbackAction.h"
#include "Inventor/actions/SoRayPickAction.h"
#include "Inventor/sensors/SoFieldSensor.h"

#include <Inventor/system/gl.h>

SO_NODE_SOURCE( SoSplitTexture2 );

void SoSplitTexture2::initClass()
{
    SO_NODE_INIT_CLASS(SoSplitTexture2, SoNode, "Node");

    SO_ENABLE(SoGLRenderAction, SoSplitTexture2Element);
}


SoSplitTexture2::SoSplitTexture2()
{
    SO_NODE_CONSTRUCTOR( SoSplitTexture2 );
    SO_NODE_ADD_FIELD( image, (SbVec2s(0,0),0,0,SoSFImage::COPY));
}


SoSplitTexture2::~SoSplitTexture2()
{
}


void SoSplitTexture2::GLRender( SoGLRenderAction* action )
{
    SbVec2s sz;
    int nrcomp;
    const unsigned char* values = image.getValue( sz, nrcomp );

    SoState* state = action->getState();
    const int unit = SoTextureUnitElement::get( state );
    SoSplitTexture2Element::set( state, this, unit, sz, nrcomp, values );
    SoCacheElement::setInvalid( true );
    if ( state->isCacheOpen() )
	SoCacheElement::invalidate(state);

}


SO_NODE_SOURCE( SoSplitTexture2Part );

void SoSplitTexture2Part::initClass()
{
    SO_NODE_INIT_CLASS(SoSplitTexture2Part, SoNode, "Node");
    SO_ENABLE( SoGLRenderAction, SoSplitTexture2Element);
    SO_ENABLE( SoGLRenderAction, SoCacheElement );
    SO_ENABLE( SoGLRenderAction, SoTextureImageElement );
    SO_ENABLE( SoGLRenderAction, SoGLTexture3EnabledElement );
    SO_ENABLE( SoGLRenderAction, SoGLTextureEnabledElement );

    SO_ENABLE( SoCallbackAction, SoTexture3EnabledElement );
    SO_ENABLE( SoCallbackAction, SoTextureImageElement );
    SO_ENABLE( SoCallbackAction, SoTextureEnabledElement );
    SO_ENABLE( SoCallbackAction, SoTextureOverrideElement );
    SO_ENABLE( SoCallbackAction, SoMultiTextureImageElement );
    SO_ENABLE( SoCallbackAction, SoMultiTextureEnabledElement );

    SO_ENABLE( SoRayPickAction, SoTexture3EnabledElement );
    SO_ENABLE( SoRayPickAction, SoTextureImageElement );
    SO_ENABLE( SoRayPickAction, SoTextureEnabledElement );
    SO_ENABLE( SoRayPickAction, SoTextureOverrideElement );
    SO_ENABLE( SoRayPickAction, SoMultiTextureImageElement );
    SO_ENABLE( SoRayPickAction, SoMultiTextureEnabledElement );
}



SoSplitTexture2Part::SoSplitTexture2Part()
    : needregenration_( true )
    , matchinfo_( 0 )
{
    SO_NODE_CONSTRUCTOR( SoSplitTexture2Part );
    SO_NODE_ADD_FIELD( origin, (SbVec2i32(0,0) ) );
    SO_NODE_ADD_FIELD( size, (SbVec2i32(0,0) ) );
    SO_NODE_ADD_FIELD( textureunits, (0) );

    originsensor_ = new SoFieldSensor( fieldChangeCB, this );
    originsensor_->attach( &origin );
}


SoSplitTexture2Part::~SoSplitTexture2Part()
{
    removeImageData();
    delete originsensor_;
    delete matchinfo_;
}


void SoSplitTexture2Part::fieldChangeCB( void* data, SoSensor* )
{
    SoSplitTexture2Part* ptr = (SoSplitTexture2Part*) data;
    ptr->needregenration_ = true;
}


#define mFastDim	0
#define mSlowDim	1

void SoSplitTexture2Part::GLRender( SoGLRenderAction* action )
{
    SoState* state = action->getState();
    const SoElement* splitelem = state->getConstElement(
	    SoSplitTexture2Element::getClassStackIndex() );

    if ( !needregenration_ && matchinfo_ && splitelem && 
	 !splitelem->matches(matchinfo_) )
	needregenration_ = true;
	
    if ( needregenration_ )
	removeImageData();

    if ( SoTextureOverrideElement::getImageOverride(state) )
	return;

    const int prevunit = SoTextureUnitElement::get( state );

    for ( int idx=textureunits.getNum()-1; idx>=0; idx-- )
	GLRenderUnit( textureunits[idx], state );

    if ( needregenration_ )
    {
	SoCacheElement::setInvalid( true );
	if ( state->isCacheOpen() )
	    SoCacheElement::invalidate(state);
    }

    SoTextureUnitElement::set( state, this, prevunit );

    needregenration_ = false;
    delete matchinfo_; matchinfo_ = splitelem->copyMatchInfo();
}


void SoSplitTexture2Part::doAction( SoAction* action )
{
    SoState * state = action->getState();

    for ( int idx=textureunits.getNum()-1; idx>=0; idx-- )
	doActionUnit( textureunits[idx], state );
}


void SoSplitTexture2Part::callback( SoCallbackAction* action )
{ doAction(action); }


void SoSplitTexture2Part::rayPick( SoRayPickAction* action )
{ doAction(action); }


void SoSplitTexture2Part::GLRenderUnit( int unit, SoState* state )
{
    const SbVec2i32 origsz = size.getValue();
    const SbVec2i32 origstart = origin.getValue();

    const SbVec2s sz( origsz[0], origsz[1] );

    const SbVec2s start( origstart[0], origstart[1] );

    SbVec2s sourcesize;
    int numcomponents;
    const unsigned char* sourcedata =
	SoSplitTexture2Element::get( state, unit, sourcesize, numcomponents );

    if ( !sourcedata )
	return;

    while ( imagedata_.getLength()<=unit )
	imagedata_.append( new ImageData );

    ImageData* imagedata = (ImageData*) imagedata_[unit];

    const int bufsize = sz[0]*sz[1];
    if ( !imagedata->imagedata_ || bufsize!=imagedata->imagesize_ )
    {
	imagedata->imagesize_ = bufsize;
	delete [] imagedata->imagedata_;
	imagedata->imagedata_ = new unsigned char[bufsize*numcomponents];
	imagedata->numcomp_ = numcomponents;

	if ( !imagedata->imagedata_ )
	    return;

	needregenration_ = true;
    }

    const float quality = SoTextureQualityElement::get(state);
    SoTextureUnitElement::set( state, this, unit );

    if ( needregenration_ )
    {
	for ( int idx=0; idx<sz[mSlowDim]; idx++ )
	{
	    int srcslowidx = start[mSlowDim]+idx;
	    if ( srcslowidx<0 )
		srcslowidx = 0;
	    else if ( srcslowidx>=sourcesize[mSlowDim] )
		srcslowidx = sourcesize[mSlowDim]-1;

	    const unsigned char* srcptr = sourcedata +
		(srcslowidx*sourcesize[mFastDim]) * numcomponents;
	    unsigned char* dstptr =
		imagedata->imagedata_ + (sz[mFastDim] * idx) * numcomponents;

	    for ( int idy=0; idy<sz[mFastDim]; idy++ )
	    {
		int srcfastidx = start[mFastDim]+idy;
		if ( !srcfastidx )
		{
		    int copysize = sz[mFastDim]-idy;
		    if ( copysize>sourcesize[mFastDim] )
			copysize = sourcesize[mFastDim];
		    if ( copysize>0 )
		    {
			memcpy( dstptr+idy*numcomponents, srcptr,
				copysize*numcomponents );
			idy += (copysize-1);
			continue;
		    }
		}

		if ( srcfastidx>=sourcesize[mFastDim] )
		    srcfastidx = sourcesize[mFastDim]-1;
		else if ( srcfastidx<0 )
		    srcfastidx = 0;

		memcpy( dstptr+idy*numcomponents,
			srcptr+srcfastidx*numcomponents, numcomponents );
	    }
	}

	imagedata->glimage_->setData( imagedata->imagedata_, sz, numcomponents,
	    SoGLImage::CLAMP, SoGLImage::CLAMP, quality, false, state );
    }

    const SoTextureImageElement::Model glmodel =
	SoTextureImageElement::MODULATE;
    const cc_glglue* glue =
	cc_glglue_instance( SoGLCacheContextElement::get(state) );

    const int maxunits = cc_glglue_max_texture_units(glue);


    if ( unit<maxunits )
    {
	SoGLMultiTextureImageElement::set( state, this, unit,
					   imagedata->glimage_, glmodel,
					   SbColor(1,1,1) );
	SoGLMultiTextureEnabledElement::set( state, this, unit,
				!needregenration_ && quality > 0.0f);
	if ( !unit && isOverride() )
	    SoTextureOverrideElement::setImageOverride( state, true );
    }
}


void SoSplitTexture2Part::doActionUnit( int unit, SoState* state )
{
    if ( !unit && SoTextureOverrideElement::getImageOverride(state) )
	return;

    if ( imagedata_.getLength()<=unit )
	return;

    SoTextureUnitElement::set( state, this, unit );
    SbVec2s sz( size.getValue()[0], size.getValue()[1] );

    ImageData* imagedata = (ImageData*) imagedata_[unit];
    const unsigned char* bytes = imagedata->imagedata_;
    int nc = imagedata->numcomp_;

    if ( !bytes )
    {
	static const unsigned char dummytex[] = {0xff,0xff,0xff,0xff};
	bytes = dummytex;
	sz = SbVec2s(2,2);
	nc = 1;
    } 

    if ( !unit )
    {
	SoTexture3EnabledElement::set(state, this, false );
	if ( sz!=SbVec2s(0,0) )
	{
	    SoTextureImageElement::set(state, this, sz, nc, bytes,
				    0, 0, SoTextureImageElement::MODULATE,
				    SbColor( 1, 1, 1 ) );
	    SoTextureEnabledElement::set(state, this, true );
	}
	else
	{
	    SoTextureImageElement::setDefault(state, this);
	    SoTextureEnabledElement::set(state, this, false );
	}
	if ( isOverride() )
	{
		SoTextureOverrideElement::setImageOverride(state, true );
	}
    }
    else
    {
	if ( sz!=SbVec2s(0,0) )
	{
	    SoMultiTextureImageElement::set(state, this, unit,
		sz, nc, bytes, SoTextureImageElement::CLAMP,
		SoTextureImageElement::CLAMP,
		SoTextureImageElement::MODULATE,
		SbColor( 1, 1, 1 ) );
	    SoMultiTextureEnabledElement::set(state, this, unit, true );
	}
	else
	{
	    SoMultiTextureImageElement::setDefault(state, this, unit);
	    SoMultiTextureEnabledElement::set(state, this, unit, false );
	}
    }
}


void SoSplitTexture2Part::removeImageData()
{
    for ( int idx=imagedata_.getLength()-1; idx>=0; idx-- )
	delete (ImageData*) imagedata_[idx];

    imagedata_.truncate( 0 );
}


SoSplitTexture2Part::ImageData::ImageData()
    : imagedata_( 0 )
    , glimage_( new SoGLImage )
{ }


SoSplitTexture2Part::ImageData::~ImageData()
{
    delete [] imagedata_;
    if ( glimage_ ) glimage_->unref();
}
