/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2005
___________________________________________________________________

-*/

static const char* rcsID = "$Id: vismultitexture2.cc,v 1.24 2007-02-02 23:12:17 cvskris Exp $";


#include "vismultitexture2.h"

#include "arrayndimpl.h"
#include "array2dresample.h"
#include "interpol2d.h"
#include "errh.h"
#include "simpnumer.h"
#include "thread.h"
#include "viscolortab.h"

#include "Inventor/nodes/SoShaderProgram.h"
#include "Inventor/nodes/SoFragmentShader.h"
#include "Inventor/nodes/SoShaderParameter.h"
#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoComplexity.h"
#include "Inventor/nodes/SoTextureUnit.h"

#include "SoColTabMultiTexture2.h"
#include "SoShaderTexture2.h"


mCreateFactoryEntry( visBase::MultiTexture2 );

#define mLayersPerUnit		4


namespace visBase
{

inline int getPow2Sz( int actsz, bool above=true, int minsz=1,
		      int maxsz=INT_MAX )
{
    char npow = 0; char npowextra = actsz == 1 ? 1 : 0;
    int sz = actsz;
    while ( sz>1 )
    {
	if ( above && !npowextra && sz % 2 )
	npowextra = 1;
	sz /= 2; npow++;
    }

    sz = intpow( 2, npow + npowextra );
    if ( sz<minsz ) sz = minsz;
    if ( sz>maxsz ) sz = maxsz;
    return sz;
}


inline int nextPower2( int nr, int minnr, int maxnr )
{
    if ( nr > maxnr )
	return maxnr;

    int newnr = minnr;
    while ( nr > newnr )
	newnr *= 2;

    return newnr;
}



MultiTexture2::MultiTexture2()
    : switch_( new SoSwitch )
    , texture_( new SoColTabMultiTexture2 )
    , complexity_( new SoComplexity )
    , nonshadinggroup_( new SoGroup )
    , shadinggroup_( new SoGroup )
    , size_( -1, -1 )
    , useshading_( SoFragmentShader::isSupported(SoShaderObject::GLSL_PROGRAM) )
    , ctabtexture_( 0 )
    , datatexturegrp_( 0 )
    , shaderprogram_( 0 )
    , fragmentshader_( 0 )
    , numlayers_( 0 )
    , layeropacity_( 0 )
    , layersize0_( 0 )
    , layersize1_( 0 )
    , ctabunit_( 0 )
{
    switch_->ref();
    switch_->addChild( nonshadinggroup_ );
    switch_->addChild( shadinggroup_ );

    nonshadinggroup_->addChild( complexity_ );
    complexity_->type.setIgnored( true );
    complexity_->value.setIgnored( true );

    texture_->setNrThreads( Threads::getNrProcessors() );
    nonshadinggroup_->addChild( texture_ );

    turnOn( true );
}


MultiTexture2::~MultiTexture2()
{ switch_->unref(); }


int MultiTexture2::maxNrTextures() const
{ return (SoTextureUnit::getMaxTextureUnit()-1)*mLayersPerUnit; }


SoNode* MultiTexture2::getInventorNode()
{ return switch_; }


bool MultiTexture2::turnOn( bool yn )
{
    const bool res = isOn();
    if ( !yn )
	switch_->whichChild = SO_SWITCH_NONE;
    else
	switch_->whichChild = useshading_ ? 1 : 0;

    return res;
}


bool MultiTexture2::isOn() const
{
    return switch_->whichChild.getValue()!=SO_SWITCH_NONE;
}


void MultiTexture2::clearAll()
{
    size_.row = -1; size_.col = -1;

    for ( int idx=0; idx<nrTextures(); idx++ )
    {
	for ( int idy=0; idy<nrVersions(idx); idy++ )
	{
	    setData( idx, idy, 0 );
	}
    }
}


bool MultiTexture2::useShading( bool yn )
{
    if ( yn==useshading_ )
	return useshading_;

    bool oldval = useshading_;
    useshading_ = yn;
    if ( yn )
	createShadingVars();

    turnOn( isOn() );
    return oldval;
}


void MultiTexture2::setTextureTransparency( int texturenr, unsigned char trans )
{
    if ( useshading_ )
    {
	createShadingVars();

	while ( layeropacity_->value.getNum()<texturenr )
	    layeropacity_->value.set1Value( layeropacity_->value.getNum(),
		isTextureEnabled(layeropacity_->value.getNum()) &&
		getCurrentTextureIndexData(layeropacity_->value.getNum()) ? 1 : 0 );

	const float opacity = 1.0 - (float) trans/255;
	layeropacity_->value.set1Value( texturenr,
		isTextureEnabled( texturenr ) &&
		getCurrentTextureIndexData( texturenr ) ? opacity : 0 );
	opacity_.setSize( nrTextures(), 1 );
	opacity_[texturenr] = opacity;
	updateShadingVars();
    }
    else
    {
	while ( texture_->opacity.getNum()<texturenr )
	    texture_->opacity.set1Value( texture_->opacity.getNum(), 255 );
	texture_->opacity.set1Value( texturenr, 255-trans );
    }
}


unsigned char MultiTexture2::getTextureTransparency( int texturenr ) const
{
    if ( useshading_ )
    {
	return texturenr<opacity_.size()
	    ? 255-mNINT(opacity_[texturenr]*255) : 0;
    }
    else
    {
	if ( texturenr>=texture_->opacity.getNum() )
	    return 0;

	return 255-texture_->opacity[texturenr];
    }
}


void MultiTexture2::setOperation( int texturenr, MultiTexture::Operation op )
{
    if ( useshading_ )
    {
	if ( op!=MultiTexture::BLEND )
	    pErrMsg("Not implemented");
	return;
    }

    SoColTabMultiTexture2::Operator nop = SoColTabMultiTexture2::BLEND;
    if ( op==MultiTexture::REPLACE)
	nop = SoColTabMultiTexture2::REPLACE;
    else if ( op==MultiTexture::ADD )
	nop = SoColTabMultiTexture2::ADD;

    while ( texture_->operation.getNum()<texturenr )
	texture_->operation.set1Value( texture_->operation.getNum(),
				       SoColTabMultiTexture2::BLEND  );

    texture_->operation.set1Value( texturenr, nop );
}


MultiTexture::Operation MultiTexture2::getOperation( int texturenr ) const
{
    if ( useshading_ )
	return MultiTexture::BLEND;

    if ( texturenr>=texture_->operation.getNum() ||
	 texture_->operation[texturenr]==SoColTabMultiTexture2::BLEND )
	return MultiTexture::BLEND;
    else if ( texture_->operation[texturenr]==SoColTabMultiTexture2::REPLACE )
	return MultiTexture::REPLACE;

    return MultiTexture::ADD;
}


void MultiTexture2::setTextureRenderQuality( float val )
{
    if ( useshading_ )
	return;

    complexity_->textureQuality.setValue( val );
}


float MultiTexture2::getTextureRenderQuality() const
{
    if ( useshading_ )
	return 1;

    return complexity_->textureQuality.getValue();
}


bool MultiTexture2::setDataOversample( int texture, int version,
				       int resolution, bool interpol,
	                               const Array2D<float>* data, bool copy )
{
    if ( !data ) return setData( texture, version, data );

    const int datax0size = data->info().getSize(0);
    const int datax1size = data->info().getSize(1);
    if ( datax0size<2 || datax1size<2  )
	return setData( texture, version, data, copy );

    const static int minpix2d = 128;
    const static int maxpix2d = 1024;

    int newx0 = getPow2Sz( datax0size, true, minpix2d, maxpix2d );
    int newx1 = getPow2Sz( datax1size, true, minpix2d, maxpix2d );

    if ( resolution )
    {
	newx0 = nextPower2( datax0size, minpix2d, maxpix2d ) * resolution;
	newx1 = nextPower2( datax1size, minpix2d, maxpix2d ) * resolution;
    }

    if ( !setSize( newx0, newx1 ) )
	return false;

    Array2DImpl<float> interpoldata( newx0, newx1 );
    if ( interpol )
	polyInterp( *data, interpoldata );
    else
	nearestValInterp( *data, interpoldata );

    const int totalsz = interpoldata.info().getTotalSz();
    float* arr = new float[totalsz];
    memcpy( arr, interpoldata.getData(), totalsz*sizeof(float) );
    return setTextureData( texture, version, arr, totalsz, true );
}


bool MultiTexture2::setSize( int sz0, int sz1 )
{
    if ( size_.row==sz0 && size_.col==sz1 )
	return true;

    if ( size_.row>=0 && size_.col>=0 &&
		(nrTextures()>1 || (nrTextures() && nrVersions(0)>1)) )
    {
	pErrMsg("Invalid size" );
	return false;
    }

    size_.row = sz0;
    size_.col = sz1;
    if ( layersize0_ )
    {
	layersize0_->value.setValue( size_.col );
	layersize1_->value.setValue( size_.row );
    }

    return true;
}

void MultiTexture2::nearestValInterp( const Array2D<float>& inp,
				      Array2D<float>& out ) const
{   
    const int inpsize0 = inp.info().getSize( 0 );
    const int inpsize1 = inp.info().getSize( 1 );
    const int outsize0 = out.info().getSize( 0 );
    const int outsize1 = out.info().getSize( 1 );
    const float x0step = (inpsize0-1)/(float)(outsize0-1);
    const float x1step = (inpsize1-1)/(float)(outsize1-1);

    for ( int x0=0; x0<outsize0; x0++ )
    {
	const int x0sample = mNINT( x0*x0step );
	for ( int x1=0; x1<outsize1; x1++ )
	{
	    const float x1pos = x1*x1step;
	    out.set( x0, x1, inp.get( x0sample, mNINT(x1pos) ) );
	}
    }
}


void MultiTexture2::polyInterp( const Array2D<float>& inp,
				Array2D<float>& out ) const
{
    Array2DReSampler<float,float> resampler( inp, out, true );
    resampler.execute();
}


bool MultiTexture2::setData( int texture, int version,
			     const Array2D<float>* data, bool copy )
{
    if ( data && !setSize( data->info().getSize(0), data->info().getSize(1) ) )
	return false;

    const int totalsz = data ? data->info().getTotalSz() : 0;
    const float* dataarray = data ? data->getData() : 0;
    bool manage = false;
    if ( data && (!dataarray || copy ) )
    {
	float* arr = new float[totalsz];

	if ( data->getData() )
	    memcpy( arr, data->getData(), totalsz*sizeof(float) );
	else
	{
	    ArrayNDIter iter( data->info() );
	    int idx=0;
	    do
	    {
		arr[idx++] = data->get(iter.getPos());
	    } while ( iter.next() );
	}

	manage = true;
	dataarray = arr;
    }

    return setTextureData( texture, version, dataarray, totalsz, manage );
}


bool MultiTexture2::setIndexData( int texture, int version,
				  const Array2D<unsigned char>* data )
{
    const int totalsz = data ? data->info().getTotalSz() : 0;
    const unsigned char* dataarray = data ? data->getData() : 0;
    float manage = false;
    if ( data && !dataarray )
    {
	unsigned char* arr = new unsigned char[totalsz];
	ArrayNDIter iter( data->info() );
	int idx=0;
	do
	{
	    arr[idx++] = data->get(iter.getPos());
	} while ( iter.next() );

	manage = true;
	dataarray = arr;
    }

    return setTextureIndexData( texture, version, dataarray, totalsz, manage );
}


void MultiTexture2::updateSoTextureInternal( int texturenr )
{
    const unsigned char* texture = getCurrentTextureIndexData(texturenr);
    if ( size_.row<0 || size_.col<0 || !texture )
    {
	if ( useshading_ && layeropacity_ )
	    layeropacity_->value.set1Value( texturenr, 0 );
	else 
	    texture_->enabled.set1Value( texturenr, false );

	return;
    }

    if ( useshading_ )
    {
	const int nrelem = size_.col*size_.row;

	const int unit = texturenr/mLayersPerUnit;
	int texturenrbase = unit*mLayersPerUnit;
	int num = nrTextures()-texturenrbase;
	if ( num>4 ) num = 4;
	else if ( num==2 ) num=3;

	unsigned const char* t0 = getCurrentTextureIndexData(texturenrbase);
	unsigned const char* t1 = num>1
	    ? getCurrentTextureIndexData(texturenrbase+1) : 0;
	unsigned const char* t2 = num>2
	    ? getCurrentTextureIndexData(texturenrbase+2) : 0;
	unsigned const char* t3 = num>3
	    ? getCurrentTextureIndexData(texturenrbase+3) : 0;

	ArrPtrMan<unsigned char> ptr = new unsigned char[num*nrelem];
	if ( !ptr ) return;

	unsigned char* curptr = ptr;

	for ( int idx=0; idx<nrelem; idx++, curptr+= num )
	{
	    curptr[0] = t0 ? *t0++ : 255;
	    if ( num==1 ) continue;

	    curptr[1] = t1 ? *t1++ : 255;
	    curptr[2] = t2 ? *t2++ : 255;
	    if ( num!=4 ) continue;
	    curptr[3] = t3 ? *t3++ : 255;
	}

	createShadingVars();

	mDynamicCastGet( SoShaderTexture2*, texture,
			 datatexturegrp_->getChild( unit*2+1 ) );
	texture->image.setValue( SbVec2s(size_.col,size_.row), num, ptr,
				 SoSFImage::COPY );
	//TODO: change native format and use it directly
    }
    else
    {
	const SbImage image( texture, SbVec2s(size_.col,size_.row), 1 );
	texture_->image.set1Value( texturenr, image );
    }

    updateColorTables();
}


void MultiTexture2::updateColorTables()
{
    if ( useshading_ )
    {
	if ( !ctabtexture_ ) return;

	const int nrtextures = nrTextures();
	unsigned char* arrstart = 0;

	SbVec2s cursize;
	int curnc;
	bool finishedit = false;
	unsigned char* curarr =
	    ctabtexture_->image.startEditing(cursize,curnc);

	if ( curnc==4 && cursize[1]==nrtextures && cursize[0]==256 )
	{
	    arrstart = curarr;
	    finishedit = true;
	}
	else
	    arrstart = new unsigned char[nrtextures*256*4];

	unsigned char* arr = arrstart;

	opacity_.setSize( nrtextures, 1 );

	for ( int idx=0; idx<nrtextures; idx++ )
	{
	    const VisColorTab& ctab = getColorTab( idx );
	    const int nrsteps = ctab.nrSteps();

	    for ( int idy=0; idy<256; idy++ )
	    {
		const Color col =
		    idy<=nrsteps ? ctab.tableColor( idy ) : Color::Black;

		*(arr++) = col.r();
		*(arr++) = col.g();
		*(arr++) = col.b();
		*(arr++) = 255-col.t();
	    }
	}

	if ( finishedit )
	    ctabtexture_->image.finishEditing();
	else
	    ctabtexture_->image.setValue( SbVec2s(256,nrtextures), 4, arrstart,
				      SoSFImage::NO_COPY_AND_DELETE );

	updateShadingVars();
    }
    else
    {
	int totalnr = 0;
	const int nrtextures = nrTextures();
	for ( int idx=0; idx<nrtextures; idx++ )
	    totalnr += getColorTab( idx ).nrSteps() + 1;

	unsigned char* arrstart = 0;

	SbVec2s cursize;
	int curnc;
	bool finishedit = false;
	unsigned char* curarr = texture_->colors.startEditing( cursize, curnc );
	if ( curnc==4 && cursize[1]==totalnr )
	{
	    arrstart = curarr;
	    finishedit = true;
	}
	else
	    arrstart = new unsigned char[totalnr*4];

	unsigned char* arr = arrstart;

	if ( texture_->numcolor.getNum()>nrtextures )
	    texture_->numcolor.deleteValues( nrtextures, -1 );
	if ( texture_->component.getNum()>nrtextures )
	    texture_->component.deleteValues( nrtextures, -1 );
	if ( texture_->enabled.getNum()>nrtextures )
	    texture_->enabled.deleteValues( nrtextures, -1 );

	for ( int idx=0; idx<nrtextures; idx++ )
	{
	    if ( !isTextureEnabled(idx) || !getCurrentTextureIndexData(idx) )
	    {
		texture_->enabled.set1Value( idx, false );
		continue;
	    }

	    texture_->enabled.set1Value( idx, true );

	    const VisColorTab& ctab = getColorTab( idx );
	    const int nrsteps = ctab.nrSteps();

	    texture_->numcolor.set1Value( idx, nrsteps+1 ); //one extra for udf
	    for ( int idy=0; idy<=nrsteps; idy++ )
	    {
		const Color col = ctab.tableColor( idy );
		*(arr++) = col.r();
		*(arr++) = col.g();
		*(arr++) = col.b();
		*(arr++) = 255-col.t();
	    }

	    SoColTabMultiTexture2::Operator op = SoColTabMultiTexture2::BLEND;
	    if ( !idx || getOperation(idx)==MultiTexture::REPLACE)
		op = SoColTabMultiTexture2::REPLACE;
	    else if ( getOperation(idx)==MultiTexture::ADD )
		op = SoColTabMultiTexture2::ADD;

	    texture_->component.set1Value( idx, getComponents(idx) );
	}

	if ( finishedit )
	    texture_->colors.finishEditing();
	else
	    texture_->colors.setValue( SbVec2s(totalnr,1), 4, arrstart,
				      SoSFImage::NO_COPY_AND_DELETE );
    }
}


void MultiTexture2::updateShadingVars()
{
    const int nrtextures = nrTextures();

    if ( layeropacity_->value.getNum()>nrtextures )
	layeropacity_->value.deleteValues( nrtextures, -1 );

    int firstlayer = -1;

    numlayers_->value.setValue( nrtextures );

    for ( int idx=nrtextures-1; idx>=0; idx-- )
    {
	if ( isTextureEnabled(idx) && getCurrentTextureIndexData(idx) )
	{
	    firstlayer = idx;
	    if ( !hasTransparency(idx) )
		break;
	}
    }

    if ( firstlayer>=0 )
    {
	for ( int idx=0; idx<nrtextures; idx++ )
	{
	    layeropacity_->value.set1Value( idx,
		    isTextureEnabled(idx) && getCurrentTextureIndexData(idx)
		    ? opacity_[idx] : 0 );
	}
    }

    startlayer_->value.setValue( firstlayer );
}
    

	
void MultiTexture2::insertTextureInternal( int texturenr )
{
    texture_->image.insertSpace( texturenr, 1 );
    updateSoTextureInternal( texturenr );
}


void MultiTexture2::removeTextureInternal( int texturenr )
{
    if ( useshading_ )
    {
	updateSoTextureInternal( texturenr );
    }
    else if ( texture_->image.getNum()>texturenr )
	texture_->image.deleteValues( texturenr, 1 );

    updateColorTables();
}


void MultiTexture2::createShadingVars()
{
    if ( !ctabtexture_ )
    {
	SoComplexity* complexity = new SoComplexity;
	complexity->textureQuality.setValue( 0.1 );
	shadinggroup_->addChild( complexity );

	ctabtexture_ = new SoShaderTexture2;
	shadinggroup_->addChild( ctabtexture_ );

	complexity = new SoComplexity;
	complexity->textureQuality.setValue( 0.3 );
	shadinggroup_->addChild( complexity );

	datatexturegrp_ = new SoGroup;
	shadinggroup_->addChild( datatexturegrp_ );

	shaderprogram_ = new SoShaderProgram();

	fragmentshader_ = new SoFragmentShader;
	fragmentshader_->sourceType = SoShaderObject::GLSL_PROGRAM;
	BufferString shadingprog;
	createShadingProgram( maxNrTextures(), shadingprog );
	fragmentshader_->sourceProgram.setValue( shadingprog.buf() );
	numlayers_ = new SoShaderParameter1i;
	numlayers_->name.setValue("numlayers");
	fragmentshader_->parameter.addNode( numlayers_ );

	startlayer_ = new SoShaderParameter1i;
	startlayer_->name.setValue("startlayer");
	fragmentshader_->parameter.addNode( startlayer_ );
	startlayer_->value.setValue( 0 );

	layeropacity_ = new SoShaderParameterArray1f;
	layeropacity_->name.setValue("trans");
	fragmentshader_->parameter.addNode( layeropacity_ );

	ctabunit_ = new SoShaderParameter1i;
	ctabunit_->name.setValue("ctabunit");
	ctabunit_->value.setValue( 0 );
	fragmentshader_->parameter.addNode( ctabunit_ );

	layersize0_ = new SoShaderParameter1i;
	layersize0_->name.setValue("texturesize0");
	layersize0_->value.setValue(size_.col);
	fragmentshader_->parameter.addNode( layersize0_ );

	layersize1_ = new SoShaderParameter1i;
	layersize1_->name.setValue("texturesize1");
	layersize1_->value.setValue(size_.row);
	fragmentshader_->parameter.addNode( layersize1_ );

	const int maxnrunits = (MultiTexture2::maxNrTextures()-1)/
	    			mLayersPerUnit+1;
	for ( int idx=0; idx<maxnrunits; idx++ )
	{
	    SoShaderParameter1i* dataunit = new SoShaderParameter1i;
	    BufferString nm = "dataunit";
	    nm += idx;
	    dataunit->name.setValue( nm.buf() );
	    dataunit->value.setValue( idx+1 );
	    fragmentshader_->parameter.addNode( dataunit );
	}

	shaderprogram_->shaderObject.addNode( fragmentshader_ );
	shadinggroup_->addChild( shaderprogram_ );

	//Reset the texture unit so the shape's
	//tcoords come in the right unit.
	SoTextureUnit* unit = new SoTextureUnit;
	unit->unit = 0;
	shadinggroup_->addChild( unit );
    }

    const int nrunits = (nrTextures()-1)/mLayersPerUnit+1;
    for ( int idx=datatexturegrp_->getNumChildren()/2; idx<nrunits; idx++ )
    {
	SoTextureUnit* u1 = new SoTextureUnit;
	u1->unit = idx+1;
	datatexturegrp_->addChild( u1 );
	datatexturegrp_->addChild( new SoShaderTexture2 );
    }
}


void  MultiTexture2::createShadingProgram( int nrlayers,
					   BufferString& res ) const
{
    const char* variables = 
"#extension GL_ARB_texture_rectangle : enable				\n\
uniform sampler2DRect   ctabunit;					\n\
uniform int             startlayer;					\n\
uniform int             numlayers;					\n\
uniform int             texturesize0;					\n\
uniform int             texturesize1;\n";

    const char* functions = 

"void processLayer( in float val, in float layeropacity, in int layer )	\n\
{									\n\
    if ( layer==startlayer )						\n\
    {									\n\
	gl_FragColor = texture2DRect( ctabunit,				\n\
				  vec2(val*255.0, float(layer)+0.5) );	\n\
	gl_FragColor.a *= layeropacity;					\n\
    }									\n\
    else if ( layeropacity>0.0 )					\n\
    {									\n\
	vec4 col = texture2DRect( ctabunit, 				\n\
				   vec2(val*255.0, float(layer)+0.5) );	\n\
	layeropacity *= col.a;						\n\
	gl_FragColor.rgb = mix(gl_FragColor.rgb,col.rgb,layeropacity);	\n\
	if ( layeropacity>gl_FragColor.a )				\n\
	    gl_FragColor.a = layeropacity;				\n\
    }									\n\
}\n\n";


    const char* mainprogstart =
"void main()								\n\
{									\n\
    if ( gl_FrontMaterial.diffuse.a<=0.0 )				\n\
	discard;							\n\
    if ( startlayer<0 )							\n\
	gl_FragColor = vec4(1,1,1,1);					\n\
    else								\n\
    {									\n\
	vec2 tcoord = gl_TexCoord[0].st;				\n\
	tcoord.s *= texturesize0;					\n\
	tcoord.t *= texturesize1;\n";

    res = variables;
    res += "uniform float           trans["; res += nrlayers; res += "];\n";

    const int nrunits = nrlayers ? (nrlayers-1)/mLayersPerUnit+1 : 0;
    for ( int idx=0; idx<nrunits; idx++ )
    {
	res += "uniform sampler2DRect   dataunit";
	res += idx; res += ";\n";
    }

    res += functions;
    res += mainprogstart;

    int layer = 0;
    for ( int unit=0; unit<nrunits; unit++ )
    {
	if ( !unit )
	    res += "\tif ( startlayer<4 )\n\t{\n";
	else
	{
	    res += "\tif ( startlayer<"; res += (unit+1)*4;
	    res += " && numlayers>"; res += unit*4; res += ")\n\t{\n";
	}

	res += "\t    const vec4 data = texture2DRect( dataunit";
	res += unit;
	res += ", tcoord );\n";

	for ( int idx=0; idx<mLayersPerUnit && layer<nrlayers; idx++,layer++ )
	{
	    res += "\t    if ( startlayer<="; res += layer; 
	    res += " && numlayers>"; res += layer; res += " )\n";
	    res += "\t\tprocessLayer( data["; res +=
	    idx; res += "], trans[";
	    res += layer; res += "], "; res += layer; res += ");\n";
	}

	res += "\t}\n";
    }

    res += "    }\n";
    res += "    gl_FragColor.a *= gl_FrontMaterial.diffuse.a;";
    //TODO lightning?
    res += "}";
}


}; //namespace
