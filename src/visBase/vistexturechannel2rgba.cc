/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jun 2008
___________________________________________________________________

-*/

static const char* rcsID = "$Id: vistexturechannel2rgba.cc,v 1.12 2008-12-04 13:53:19 cvskris Exp $";

#include "vistexturechannel2rgba.h"

#include "vistexturechannels.h"
#include "color.h"
#include "coltabsequence.h"
#include "coltab.h"
#include "simpnumer.h"

#include "SoColTabTextureChannel2RGBA.h"
#include "SoTextureComposer.h"
#include "Inventor/nodes/SoComplexity.h"
#include "Inventor/nodes/SoFragmentShader.h"
#include "Inventor/nodes/SoShaderProgram.h"
#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoTextureUnit.h"
#include "Inventor/nodes/SoTexture2.h"
#include "Inventor/nodes/SoVertexShader.h"
#include "Inventor/nodes/SoShaderParameter.h"

#define mNrColors	255
#define mUndefColIdx	255
#define mLayersPerUnit	4

mCreateFactoryEntry( visBase::ColTabTextureChannel2RGBA );

namespace visBase
{

TextureChannel2RGBA::TextureChannel2RGBA()
    : channels_( 0 )
    , shadingallowed_( true )
{}


void TextureChannel2RGBA::setChannels( TextureChannels* ch )
{
    channels_ = ch;
}


void TextureChannel2RGBA::allowShading( bool yn )
{
    shadingallowed_ = yn;
}


ColTabTextureChannel2RGBA::ColTabTextureChannel2RGBA()
    : converter_( new SoColTabTextureChannel2RGBA )
    , shaderswitch_( new SoSwitch )
    , shadinggroup_( 0 )
    , shaderctab_( 0 )
    , fragmentshader_( 0 )
    , numlayers_( 0 )
    , startlayer_( 0 )
    , layeropacity_( 0 )
{
    shaderswitch_->ref();
    shaderswitch_->addChild( converter_ );
}


int ColTabTextureChannel2RGBA::maxNrChannels() const
{ return 8; } //TODO


bool ColTabTextureChannel2RGBA::canUseShading() const
{ return true; }//TODO


ColTabTextureChannel2RGBA::~ColTabTextureChannel2RGBA()
{
    shaderswitch_->unref();
}


void ColTabTextureChannel2RGBA::setChannels( TextureChannels* ch )
{
    TextureChannel2RGBA::setChannels( ch );
    adjustNrChannels();
}


void ColTabTextureChannel2RGBA::adjustNrChannels() const
{
    const int nr = channels_ ? channels_->nrChannels() : 0;

    while ( coltabs_.size()<nr )
    {
	coltabs_ += ColTab::Sequence( ColTab::defSeqName() );
	enabled_ += true;
	opacity_ += 255;
    }

    while ( coltabs_.size()>nr )
    {
	coltabs_.remove( nr );
	enabled_.remove( nr );
	opacity_.remove( nr );
    }
}

void ColTabTextureChannel2RGBA::swapChannels( int ch0, int ch1 )
{
    coltabs_.swap( ch0, ch1 );
    enabled_.swap( ch0, ch1 );
    opacity_.swap( ch0, ch1 );

    update();
}


void ColTabTextureChannel2RGBA::setSequence( int channel,
	const ColTab::Sequence& seq )
{
    while ( coltabs_.size()<=channel )
    {
	coltabs_ += ColTab::Sequence( ColTab::defSeqName() );
    }

    coltabs_[channel] = seq;

    update();
}


const ColTab::Sequence*
ColTabTextureChannel2RGBA::getSequence( int channel ) const
{
    if ( channel>=coltabs_.size() )
	adjustNrChannels();

    return &coltabs_[channel];
}


void ColTabTextureChannel2RGBA::setEnabled(int ch,bool yn)
{
    if ( ch>=coltabs_.size() )
	adjustNrChannels();

    enabled_[ch] = yn;
    update();
}


bool ColTabTextureChannel2RGBA::isEnabled(int ch) const
{
    if ( ch>=enabled_.size() )
	adjustNrChannels();

    return enabled_[ch];
}


void ColTabTextureChannel2RGBA::setTransparency(int ch,unsigned char t)
{
    if ( ch>=opacity_.size() )
	adjustNrChannels();

    opacity_[ch] = 255-t;
    update();
}


unsigned char ColTabTextureChannel2RGBA::getTransparency(int ch) const
{
    if ( ch>=opacity_.size() )
	adjustNrChannels();

    return 255-opacity_[ch];
}


void ColTabTextureChannel2RGBA::allowShading( bool yn )
{
    TextureChannel2RGBA::allowShading( yn );
    update();

    shaderswitch_->whichChild = yn && shadinggroup_ ? 1 : 0;
}


bool ColTabTextureChannel2RGBA::usesShading() const
{
    return shaderswitch_->whichChild.getValue()==1;
}


bool ColTabTextureChannel2RGBA::createRGBA( SbImage& res ) const
{
    if ( !channels_ )
	return false;

    channels_->ref();
    
    SoColTabTextureChannel2RGBA* conv = new SoColTabTextureChannel2RGBA;
    conv->ref();
    doFill( conv );

    conv->processChannels( channels_->getChannels(), channels_->nrChannels() );

    od_int64 nrpixels;    
    for ( int rgba=0; rgba<4; rgba++ )
    {
	const SbImage& image = conv->getRGBA( rgba );
	if ( !rgba )
	{
	    const SbVec3s size = image.getSize();
	    nrpixels = size[0];
	    nrpixels *= size[1];
	    nrpixels *= size[2];

	    res.setValue( image.getSize(), 4, 0 );
	}

	SbVec3s dummy;
	int dummy2;

	const unsigned char* src = image.getValue( dummy, dummy2 );
	unsigned char* dst = res.getValue( dummy, dummy2 )+rgba;

	for ( int idx=0; idx<nrpixels; idx++, dst+=4, src++ )
	    *dst = *src;
    }

    conv->unref();
    channels_->unRef();
    return true;
}


void ColTabTextureChannel2RGBA::update()
{
    adjustNrChannels();

    if ( shadingallowed_ && canUseShading() )
	setShadingVars();
    else
	doFill( converter_ );
}


void ColTabTextureChannel2RGBA::setShadingVars()
{
    const bool douseshading = shadingallowed_ && canUseShading();

    if ( douseshading && !shadinggroup_ )
    {
	shadinggroup_ = new SoGroup;
	shaderswitch_->addChild( shadinggroup_ );

	tci_ = new SoTextureComposerInfo;
	shadinggroup_->addChild( tci_ );
	tci_->units.set1Value( 0, 0 );
	tci_->units.set1Value( 1, 1 );

	const int ctabunitnr = SoTextureUnit::getMaxTextureUnit()-1;
	SoTextureUnit* ctabunit = new SoTextureUnit;
	shadinggroup_->addChild( ctabunit );
	ctabunit->unit = ctabunitnr;

	SoComplexity* complexity = new SoComplexity;
	complexity->textureQuality.setValue( 0.1 );
	shadinggroup_->addChild( complexity );

	shaderctab_ = new SoTexture2;
	shadinggroup_->addChild( shaderctab_ );

	SoShaderProgram* shaderprogram = new SoShaderProgram;

	SoVertexShader* vertexshader = new SoVertexShader;
	vertexshader->sourceType = SoShaderObject::GLSL_PROGRAM;
	vertexshader->sourceProgram.setValue( sVertexShaderProgram() );

	shaderprogram->shaderObject.addNode( vertexshader );

	SoFragmentShader* fragmentshader = new SoFragmentShader;
	fragmentshader->sourceType = SoShaderObject::GLSL_PROGRAM;
	BufferString shadingprog;
	createFragShadingProgram( maxNrChannels(), shadingprog );
	fragmentshader->sourceProgram.setValue( shadingprog.buf() );

	numlayers_ = new SoShaderParameter1i;
	numlayers_->name.setValue("numlayers");
	fragmentshader->parameter.addNode( numlayers_ );

	SoShaderParameter1i* ctabunitvar = new SoShaderParameter1i;
	ctabunitvar->name.setValue("ctabunit");
	ctabunitvar->value.setValue( ctabunitnr );
	fragmentshader->parameter.addNode( ctabunitvar );

	SoShaderParameter1i* dataunit0 = new SoShaderParameter1i;
	dataunit0->name.setValue("dataunit0");
	dataunit0->value.setValue( 0 );
	fragmentshader->parameter.addNode( dataunit0 );

	SoShaderParameter1i* dataunit1 = new SoShaderParameter1i;
	dataunit1->name.setValue("dataunit1");
	dataunit1->value.setValue( 1 );
	fragmentshader->parameter.addNode( dataunit1 );

	startlayer_ = new SoShaderParameter1i;
	startlayer_->name.setValue("startlayer");
	fragmentshader->parameter.addNode( startlayer_ );
	startlayer_->value.setValue( 0 );

	layeropacity_ = new SoShaderParameterArray1f;
	layeropacity_->name.setValue("layeropacity");
	fragmentshader->parameter.addNode( layeropacity_ );

	shaderprogram->shaderObject.addNode( fragmentshader );
	shadinggroup_->addChild( shaderprogram );

	//Reset texture unit.
	ctabunit = new SoTextureUnit;
	shadinggroup_->addChild( ctabunit );
	ctabunit->unit = 0;

	complexity = new SoComplexity;
	complexity->textureQuality.setValue( 0.8 );
	shadinggroup_->addChild( complexity );

    }

    if ( !coltabs_.size() )
	return;

    const int nrchannels = coltabs_.size()>1
	? getPow2Sz( coltabs_.size() ) : 1;
    shaderctab_->image.setValue( SbVec2s( 256, nrchannels ), 4, 0 );
    
    SbVec2s dummy;
    int dummy2;
    unsigned char* textureptr = shaderctab_->image.startEditing(dummy,dummy2);
    
    TypeSet<unsigned char> cols;
    for ( int channelidx=0; channelidx<nrchannels; channelidx++ )
    {
	getColors( channelidx, cols );
	memcpy( textureptr+channelidx*256*4, cols.arr(), 256*4 );
    }

    shaderctab_->image.finishEditing();

    if ( layeropacity_->value.getNum()>nrchannels )
	layeropacity_->value.deleteValues( nrchannels, -1 );

    int firstlayer = -1;
    bool firstlayerhastrans;

    numlayers_->value.setValue( nrchannels );

    for ( int idx=nrchannels-1; idx>=0; idx-- )
    {
	const SbImage& channel = channels_->getChannels()[idx];
	SbVec3s size; int dummy2;
	const unsigned char* vals = channel.getValue( size, dummy2 );
	if ( enabled_[idx] && vals )
	{
	    firstlayer = idx;
	    firstlayerhastrans = hasTransparency(idx);
	    if ( !firstlayerhastrans )
		break;
	}
    }

    if ( firstlayer>=0 )
    {
	for ( int idx=0; idx<nrchannels; idx++ )
	{
	    const SbImage& channel = channels_->getChannels()[idx];
	    SbVec3s size; int dummy2;
	    const unsigned char* vals = channel.getValue( size, dummy2 );
	    layeropacity_->value.set1Value( idx,
		enabled_[idx] && vals ? (float) opacity_[idx]/255 : 0.0 );
	}

	tci_->transparencyInfo = firstlayerhastrans
	    ? SoTextureComposerInfo::cHasTransparency()
	    : SoTextureComposerInfo::cHasNoTransparency();
    }

    startlayer_->value.setValue( firstlayer );
}


void ColTabTextureChannel2RGBA::createFragShadingProgram(int nrchannels,
						BufferString& res ) const
{
    const char* variables =
	"uniform sampler2D   ctabunit;				\n"
	"uniform int	    startlayer;				\n"
	"varying vec3	    ecPosition3;			\n"
	"varying vec3	    fragmentNormal;			\n"
	"uniform int	    numlayers;\n";

    const char* functions =
	"void processLayer( in float val, in float layeropacity, in float ctab, in int layer)\n"
	"{								\n"
	"    if ( layer==startlayer )					\n"
	"    {								\n"
	"	gl_FragColor = texture2D( ctabunit, vec2( val, ctab ) );\n"
	"	gl_FragColor.a *= layeropacity;				\n"
	"    }								\n"
	"    else if ( layeropacity>0.0 )				\n"
	"    {								\n"
	"	vec4 col = texture2D( ctabunit,	vec2( val, ctab ) );	\n"
	"	layeropacity *= col.a;					\n"
	"	gl_FragColor.rgb = mix(gl_FragColor.rgb,col.rgb,layeropacity); \n"
	"	if ( layeropacity>gl_FragColor.a )				\n"
	"	    gl_FragColor.a = layeropacity;				\n"
	"    }								\n"
	"}\n\n";

    BufferString mainprogstart =
	"void main()							\n"
	"{								\n"
	"    if ( gl_FrontMaterial.diffuse.a<=0.0 )			\n"
	"	discard;						\n"
	"    if ( startlayer<0 )					\n"
	"	gl_FragColor = vec4(1,1,1,1);				\n"
	"    else							\n"
	"    {								\n"
	"	vec2 tcoord = gl_TexCoord[0].st;			\n"
	"	vec4 data;\n";


    res = variables;

    res += "uniform float	   layeropacity["; res += nrchannels; res += "];\n";

    const int nrunits = nrchannels ? (nrchannels-1)/mLayersPerUnit+1 : 0;
    for ( int idx=0; idx<nrunits; idx++ )
    {
	res += "uniform sampler2D     dataunit";
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

	res += "\t    data = texture2D( dataunit";
	res += unit;
	res += ", tcoord );\n";

	for ( int idx=0; idx<mLayersPerUnit && layer<nrchannels; idx++,layer++ )
	{
	    res += "\t    if ( startlayer<="; res += layer;
	    res += " && numlayers>"; res += layer; res += " )\n";
	    res += "\t    {\n\t\tfloat ctab= (";
	    res += layer;
	    res += ".5)/numlayers;\n";
	    res += "\t\tprocessLayer( data["; res += idx; res += "], layeropacity[";
	    res += layer; res += "], ctab, "; res += layer; res += ");\n";
	    res += "\t    }\n";
	}

	res += "\t}\n";
    }

    res += "    }\n";
    res += "    gl_FragColor.a *= gl_FrontMaterial.diffuse.a;\n";
    res += "    gl_FragColor.rgb *= gl_Color.rgb;\n";

    res += "}";
}


void ColTabTextureChannel2RGBA::doFill(
	SoColTabTextureChannel2RGBA* conv ) const
{
    const int nrchannels = coltabs_.size();

    TypeSet<unsigned char> cols;
    for ( int channelidx=0; channelidx<nrchannels; channelidx++ )
    {
	getColors( channelidx, cols );
    
	SbImage image;
	image.setValue( SbVec3s(1,1,256), 4, cols.arr() );
	conv->colorsequences.set1Value( channelidx, image );

	conv->enabled.set1Value( channelidx,enabled_[channelidx] &&
				 opacity_[channelidx]>0 );
	conv->opacity.set1Value( channelidx, opacity_[channelidx] );
    }

    if ( conv->enabled.getNum()>nrchannels )
	conv->enabled.deleteValues( nrchannels, -1 );

    if ( conv->opacity.getNum()>nrchannels )
	conv->opacity.deleteValues( nrchannels, -1 );

}


void ColTabTextureChannel2RGBA::getColors( int channelidx,
	TypeSet<unsigned char>& cols ) const
{
    if ( channelidx>=coltabs_.size() )
    {
	cols.setSize( 256*4, 255 );
	return;
    }

    const ColTab::Sequence& seq = coltabs_[channelidx];
    if ( cols.size()!=((mNrColors+1)*4) )
	cols.setSize( ((mNrColors+1)*4), 0 );

    unsigned char* arr = cols.arr();
    for ( int idx=0; idx<mNrColors; idx++ )
    {
	const float val = ((float) idx)/(mNrColors-1);
	const Color col = seq.color( val );

	(*arr++) = col.r();
	(*arr++) = col.g();
	(*arr++) = col.b();
	(*arr++) = 255-col.t();
    }

    const Color col = seq.undefColor();
    (*arr++) = col.r();
    (*arr++) = col.g();
    (*arr++) = col.b();
    (*arr++) = col.t();
}


bool ColTabTextureChannel2RGBA::hasTransparency( int channelidx ) const
{
    if ( opacity_[channelidx]!=255 )
	return true;

    if ( !enabled_[channelidx] )
	return true;

    const ColTab::Sequence& seq = coltabs_[channelidx];
    bool found = false;
    for ( int idx=seq.transparencySize()-1; idx>=0; idx-- )
    {
	if ( seq.transparency(idx).y>0 )
	{
	    found = true;
	    break;
	}
    }

    if ( !found )
	return false;

    channels_->ref();
    const SbImage& channel = channels_->getChannels()[channelidx];

    SbVec3s size;
    int dummy2;
    const unsigned char* vals = channel.getValue( size, dummy2 );
    od_int64 nrpixels = size[0];
    nrpixels *= size[1];
    nrpixels *= size[2];

    for ( od_int64 idx=0; idx<nrpixels; idx++ )
    {
	const float val = ((float) vals[idx])/(mNrColors-1);
	if ( seq.transparencyAt( val ) )
	{
	    channels_->unRef();
	    return true;
	}
    }

    channels_->unRef();
    return false;
}


SoNode* ColTabTextureChannel2RGBA::getInventorNode()
{ return shaderswitch_; }


const char* ColTabTextureChannel2RGBA::sVertexShaderProgram()
{
    return
"varying vec3 ecPosition3;\n"
"varying vec3 fragmentNormal;\n"
"void main(void)\n"
"{\n"
"    vec4 ecPosition = gl_ModelViewMatrix * gl_Vertex;\n"
"    ecPosition3 = ecPosition.xyz / ecPosition.w;\n"
"    fragmentNormal = normalize(gl_NormalMatrix * gl_Normal);\n"
"    vec3 lightDir = normalize(vec3(gl_LightSource[0].position));\n"
"    float NdotL = abs( dot(fragmentNormal, lightDir) );\n"
"\n"
"    vec4 diffuse = gl_FrontMaterial.diffuse * gl_LightSource[0].diffuse *\n"
"		    NdotL;\n"
"    vec4 ambient = gl_FrontMaterial.ambient * gl_LightSource[0].ambient;\n"
"    float pf = 0.0;\n"
"    if (NdotL != 0.0)\n"
"    {\n"
"       float nDotHV =\n"
"	    abs( dot(fragmentNormal, vec3(gl_LightSource[0].halfVector)));\n"
"       pf = pow(nDotHV, gl_FrontMaterial.shininess);\n"
"    }\n"
"    vec4 specular = gl_FrontMaterial.specular * gl_LightSource[0].specular *\n""		    pf;\n"
"\n"
"    vec3 lightning =\n"
"       gl_FrontLightModelProduct.sceneColor.rgb +\n"
"       ambient.rgb * gl_FrontMaterial.ambient.rgb +\n"
"       diffuse.rgb * gl_Color.rgb +\n"
"       specular.rgb * gl_FrontMaterial.specular.rgb;\n"
"    gl_Position = ftransform();\n"
"    gl_FrontColor.rgb = lightning;\n"
"    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;\n"
"}\n";
}



}; // namespace visBase
