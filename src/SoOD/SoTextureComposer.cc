/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          October 2008
 RCS:           $Id: SoTextureComposer.cc,v 1.6 2008-10-27 21:46:27 cvskris Exp $
________________________________________________________________________

-*/


#include "SoTextureComposer.h"
#include "SoTextureComposerElement.h"

#include <Inventor/misc/SoGLImage.h>
#include <Inventor/C/glue/gl.h>
#include <SoTextureChannelSetElement.h>
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
#include "Inventor/SbImage.h"

#include <Inventor/system/gl.h>

SO_NODE_SOURCE( SoTextureComposer );

void SoTextureComposer::initClass()
{
    SO_NODE_INIT_CLASS(SoTextureComposer, SoNode, "Node");
    SO_ENABLE( SoGLRenderAction, SoTextureChannelSetElement );
    SO_ENABLE( SoGLRenderAction, SoCacheElement );
    SO_ENABLE( SoGLRenderAction, SoTextureImageElement );
    SO_ENABLE( SoGLRenderAction, SoGLTexture3EnabledElement );
    SO_ENABLE( SoGLRenderAction, SoGLTextureEnabledElement );
    SO_ENABLE( SoGLRenderAction, SoTextureComposerElement );

    SO_ENABLE( SoCallbackAction, SoTexture3EnabledElement );
    SO_ENABLE( SoCallbackAction, SoTextureImageElement );
    SO_ENABLE( SoCallbackAction, SoTextureEnabledElement );
    SO_ENABLE( SoCallbackAction, SoTextureOverrideElement );
    SO_ENABLE( SoCallbackAction, SoMultiTextureImageElement );
    SO_ENABLE( SoCallbackAction, SoMultiTextureEnabledElement );
    SO_ENABLE( SoCallbackAction, SoTextureComposerElement );

    SO_ENABLE( SoRayPickAction, SoTexture3EnabledElement );
    SO_ENABLE( SoRayPickAction, SoTextureImageElement );
    SO_ENABLE( SoRayPickAction, SoTextureEnabledElement );
    SO_ENABLE( SoRayPickAction, SoTextureOverrideElement );
    SO_ENABLE( SoRayPickAction, SoMultiTextureImageElement );
    SO_ENABLE( SoRayPickAction, SoMultiTextureEnabledElement );
    SO_ENABLE( SoRayPickAction, SoTextureComposerElement );
}


SoTextureComposer::SoTextureComposer()
    : needregenration_( true )
    , matchinfo_( 0 )
{
    SO_NODE_CONSTRUCTOR( SoTextureComposer );
    SO_NODE_ADD_FIELD( origin, (SbVec3i32(0,0,0) ) );
    SO_NODE_ADD_FIELD( size, (SbVec3i32(-1,-1,-1) ) );

    originsensor_ = new SoFieldSensor( fieldChangeCB, this );
    originsensor_->attach( &origin );
}


SoTextureComposer::~SoTextureComposer()
{
    removeTextureData();
    delete originsensor_;
    delete matchinfo_;
}


void SoTextureComposer::fieldChangeCB( void* data, SoSensor* )
{
    SoTextureComposer* ptr = (SoTextureComposer*) data;
    ptr->needregenration_ = true;
}


#define mFastDim	2
#define mMidDim		1
#define mSlowDim	0

void SoTextureComposer::GLRender( SoGLRenderAction* action )
{
    SoState* state = action->getState();
    const SoElement* channelelem = state->getConstElement(
	    SoTextureChannelSetElement::getClassStackIndex() );

    if ( !needregenration_ && matchinfo_ && channelelem && 
	 !channelelem->matches(matchinfo_) )
	needregenration_ = true;
	
    if ( needregenration_ )
	removeTextureData();

    if ( SoTextureOverrideElement::getImageOverride(state) )
	return;

    const int prevunit = SoTextureUnitElement::get( state );

    int firstchannel = 0;

    const SbList<int> units = SoTextureComposerElement::getUnits( state );
    const int nrunits = units.getLength();

    for ( int idx=0; idx<nrunits; idx++, firstchannel+=4 )
	GLRenderUnit( units[idx], state, firstchannel );

    if ( needregenration_ )
    {
	SoCacheElement::setInvalid( true );
	if ( state->isCacheOpen() )
	    SoCacheElement::invalidate(state);
    }

    SoTextureUnitElement::set( state, this, prevunit );

    needregenration_ = false;
    delete matchinfo_; matchinfo_ = channelelem->copyMatchInfo();
}


void SoTextureComposer::doAction( SoAction* action )
{
    SoState * state = action->getState();
    const SbList<int> units = SoTextureComposerElement::getUnits( state );
    const int nrunits = units.getLength();

    for ( int idx=nrunits-1; idx>=0; idx-- )
	doActionUnit( units[idx], state );
}


void SoTextureComposer::callback( SoCallbackAction* action )
{ doAction(action); }


void SoTextureComposer::rayPick( SoRayPickAction* action )
{ doAction(action); }


void SoTextureComposer::GLRenderUnit( int unit, SoState* state,
				      int firstchannel )
{
    int nrchannels =
	SoTextureChannelSetElement::getNrChannels(state)-firstchannel;
    if ( nrchannels>4 )
	nrchannels = 4;

    if ( nrchannels<1 )
	return; //correct return?

    int nrcomponents = nrchannels; if ( nrcomponents==2 ) //Only allow 1, 3 or 4 components
	nrcomponents = 3;

    const SbImage* channelsimages =
	SoTextureChannelSetElement::getChannels( state )+firstchannel;
    SbVec3s channelsize;

    const unsigned char* channels[4];
    int bytesperpixel[4];
    for ( int idx=0; idx<nrchannels; idx++ )
    {
	channels[idx] =
	    channelsimages[idx].getValue( channelsize, bytesperpixel[idx] );
    }
    
    const SbVec3i32 origstart = origin.getValue();
    SbVec3i32 origsz = size.getValue();
    if ( origsz[0]==-1 )
    {
	origsz[0] = channelsize[0] - origstart[0];
	origsz[1] = channelsize[1] - origstart[1];
	origsz[2] = channelsize[2] - origstart[2];
    }

    const SbVec3s sz( origsz[0], origsz[1], origsz[2] );
    const SbVec3s start( origstart[0], origstart[1], origstart[2] );

    while ( texturedata_.getLength()<=unit )
	texturedata_.append( 0 );

    if ( !texturedata_[unit] )
	texturedata_[unit] = new TextureData;

    TextureData* texturedata = (TextureData*) texturedata_[unit];

    const int bufsize = sz[0]*sz[1]*sz[2];
    if ( !texturedata->imagedata_ || bufsize!=texturedata->imagesize_ )
    {
	texturedata->imagesize_ = bufsize;
	delete [] texturedata->imagedata_;
	texturedata->imagedata_ = new unsigned char[bufsize*nrcomponents];
	texturedata->numcomp_ = nrcomponents;

	if ( !texturedata->imagedata_ )
	    return;

	needregenration_ = true;
    }

    const SoTextureComposer::ForceTransparency ft =
	SoTextureComposerElement::getForceTrans( state );

    if ( texturedata->ft_!=ft )
    {
	texturedata->ft_ = ft;
	uint32_t flags = texturedata->glimage_->getFlags();
	if ( ft==SoTextureComposer::DONT_FORCE )
	{
	    uint32_t mask = 0xFFFFFFFF;
	    mask = mask^SoGLImage::FORCE_TRANSPARENCY_TRUE;
	    mask = mask^SoGLImage::FORCE_TRANSPARENCY_FALSE;
	    flags &= mask;
	}
	else if ( ft==SoTextureComposer::FORCE_ON )
	    flags |= SoGLImage::FORCE_TRANSPARENCY_TRUE;
	else
	    flags |= SoGLImage::FORCE_TRANSPARENCY_FALSE;

	texturedata->glimage_->setFlags( flags );
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
	    else if ( srcslowidx>=channelsize[mSlowDim] )
		srcslowidx = channelsize[mSlowDim]-1;

	    for ( int idy=0; idy<sz[mMidDim]; idy++ )
	    {
		int srcmididx = start[mMidDim]+idy;
		if ( srcmididx<0 )
		    srcmididx = 0;
		else if ( srcmididx>=channelsize[mMidDim] )
		    srcmididx = channelsize[mMidDim]-1;
		const int firstsourcesample = 
		    srcslowidx*channelsize[mFastDim]*channelsize[mMidDim]+
		    srcmididx*channelsize[mFastDim];
		//const unsigned char* srcptr = sourcedata + firstsourcesample;

		const int firstdstsample =
		    sz[mFastDim] * sz[mMidDim] * idx +
		    sz[mFastDim] * idy;

		//unsigned char* dstptr =
		//texturedata->imagedata_ + (sz[mFastDim] * idx) * nrcomponents;
		for ( int idz=0; idz<sz[mFastDim]; idz++ )
		{
		    int srcfastidx = start[mFastDim]+idz;
		    if ( srcfastidx<0 )
			srcfastidx = 0;
		    else if ( srcfastidx>=channelsize[mFastDim] )
			srcfastidx = channelsize[mFastDim]-1;
		    //if ( nrchannels==nrcomponents )
			//We can copy

		    for ( int idc=0; idc<nrcomponents; idc++ )
		    {
			if ( idc>=nrchannels )
			    continue; //set dummy value?

			const int ressample = 
			    (firstdstsample+idz)*nrcomponents+idc;
			const int sourcesample =
			    (firstsourcesample+srcfastidx)*bytesperpixel[idc];

			texturedata->imagedata_[ressample] = 
					    channels[idc][sourcesample];
		    }
		}
	    }
	}

	if ( sz[0]==1 )
	{
	    texturedata->glimage_->setData( texturedata->imagedata_,
		SbVec2s(sz[1],sz[2]),nrcomponents,SoGLImage::CLAMP,SoGLImage::CLAMP,
		quality,false,0 );
	}
	else
	{
	    texturedata->glimage_->setData( texturedata->imagedata_, sz,
		nrcomponents,SoGLImage::CLAMP,SoGLImage::CLAMP,
		SoGLImage::CLAMP,quality,false,0 );
	}
    }

    const SoTextureImageElement::Model glmodel =
	SoTextureImageElement::MODULATE;
    const cc_glglue* glue =
	cc_glglue_instance( SoGLCacheContextElement::get(state) );

    const int maxunits = cc_glglue_max_texture_units(glue);

    if ( unit==0 )
    {
	SoGLTextureImageElement::set( state, this, texturedata->glimage_, glmodel,
				      SbColor(1,1,1) );
	SoGLTexture3EnabledElement::set(state, this, false );
	SoGLTextureEnabledElement::set(state, this, true );
	if ( isOverride() )
	    SoTextureOverrideElement::setImageOverride( state, true );
    }
    else if ( unit<maxunits )
    {
	SoGLMultiTextureImageElement::set( state, this, unit,
					   texturedata->glimage_, glmodel,
					   SbColor(1,1,1) );
	SoGLMultiTextureEnabledElement::set( state, this, unit,
				!needregenration_ && quality > 0.0f);
    }
}


void SoTextureComposer::doActionUnit( int unit, SoState* state )
{
    if ( !unit && SoTextureOverrideElement::getImageOverride(state) )
	return;

    if ( texturedata_.getLength()<=unit )
	return;

    SoTextureUnitElement::set( state, this, unit );
    SbVec3s sz( size.getValue()[0], size.getValue()[1], size.getValue()[2] );

    TextureData* texturedata = (TextureData*) texturedata_[unit];
    const unsigned char* bytes = texturedata->imagedata_;
    int nc = texturedata->numcomp_;

    if ( !bytes )
    {
	static const unsigned char dummytex[] = {0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff};
	bytes = dummytex;
	sz = SbVec3s(2,2,2);
	nc = 1;
    } 

    if ( !unit )
    {
	SoTexture3EnabledElement::set(state, this, false );
	if ( sz!=SbVec3s(0,0,0) )
	{
	    if ( sz[0]==1 )
	    {
		SoTextureImageElement::set(state, this, SbVec2s(sz[1],sz[2]),
			nc, bytes, 0, 0, SoTextureImageElement::MODULATE,
			SbColor( 1, 1, 1 ) );
	    }
	    else
	    {
		SoTextureImageElement::set(state, this, sz,
			nc, bytes, 0, 0, 0, SoTextureImageElement::MODULATE,
			SbColor( 1, 1, 1 ) );
	    }

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
	if ( sz!=SbVec3s(0,0,0) )
	{
	    if ( sz[0]==1 )
	    {
		SoMultiTextureImageElement::set(state, this, unit,
		    SbVec2s(sz[1],sz[2]), nc, bytes,
		    SoTextureImageElement::CLAMP,
		    SoTextureImageElement::CLAMP,
		    SoTextureImageElement::MODULATE,
		    SbColor( 1, 1, 1 ) );
	    }
	    else
	    {
		SoMultiTextureImageElement::set(state, this, unit,
		    sz, nc, bytes,
		    SoTextureImageElement::CLAMP,
		    SoTextureImageElement::CLAMP,
		    SoTextureImageElement::CLAMP,
		    SoTextureImageElement::MODULATE,
		    SbColor( 1, 1, 1 ) );
	    }

	    SoMultiTextureEnabledElement::set(state, this, unit, true );
	}
	else
	{
	    SoMultiTextureImageElement::setDefault(state, this, unit);
	    SoMultiTextureEnabledElement::set(state, this, unit, false );
	}
    }
}


void SoTextureComposer::removeTextureData()
{
    for ( int idx=texturedata_.getLength()-1; idx>=0; idx-- )
	delete (TextureData*) texturedata_[idx];

    texturedata_.truncate( 0 );
}


SoTextureComposer::TextureData::TextureData()
    : imagedata_( 0 )
    , glimage_( new SoGLImage )
    , ft_( SoTextureComposer::DONT_FORCE )
{ }


SoTextureComposer::TextureData::~TextureData()
{
    delete [] imagedata_;
    if ( glimage_ ) glimage_->unref();
}
