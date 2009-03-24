/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: vismultiattribsurvobj.cc,v 1.29 2009-03-24 14:44:34 cvshelene Exp $";

#include "vismultiattribsurvobj.h"

#include "attribsel.h"
#include "visdataman.h"
#include "vismaterial.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "viscolortab.h"
#include "vismultitexture2.h"
#include "vistexturechannel2rgba.h"
#include "vistexturechannels.h"
#include "iopar.h"
#include "keystrs.h"
#include "math2.h"


namespace visSurvey {

const char* MultiTextureSurveyObject::sKeyResolution()	{ return "Resolution"; }
const char* MultiTextureSurveyObject::sKeyTextTrans()	{ return "Trans"; }
const char* MultiTextureSurveyObject::sKeySequence()	{ return "Sequence"; }
const char* MultiTextureSurveyObject::sKeyMapper()	{ return "Mapper"; }

MultiTextureSurveyObject::MultiTextureSurveyObject( bool dochannels )
    : VisualObjectImpl(true)
    , DisplayPropertyHolder(true)
    , texture_( dochannels ? 0 : visBase::MultiTexture2::create() )
    , channels_( dochannels ? visBase::TextureChannels::create() : 0 )
    , onoffstatus_( true )
    , resolution_( 0 )
{
    if ( texture_ )
    {
	texture_->ref();
	addChild( texture_->getInventorNode() );
	texture_->setTextureRenderQuality(1);
    }
    else
    {
	channels_->ref();
	addChild( channels_->getInventorNode() );
	channels_->setChannels2RGBA( 
		visBase::ColTabTextureChannel2RGBA::create() );
    }
	
    material_->setColor( Color::White() );
    material_->setAmbience( 0.8 );
    material_->setDiffIntensity( 0.8 );
}


MultiTextureSurveyObject::~MultiTextureSurveyObject()
{
    deepErase( as_ );
    setDataTransform( 0 );
    if ( texture_ )
	texture_->unRef();
    else
	channels_->unRef();
}


bool MultiTextureSurveyObject::_init()
{
    return visBase::DataObject::_init() && addAttrib();
}


void MultiTextureSurveyObject::allowShading( bool yn )
{
    if ( texture_ )
	texture_->allowShading( yn );
    else if ( channels_ && channels_->getChannels2RGBA() )
	channels_->getChannels2RGBA()->allowShading( yn );
}


void MultiTextureSurveyObject::turnOn( bool yn )
{
    onoffstatus_ = yn;
    updateMainSwitch();
}


bool MultiTextureSurveyObject::isOn() const
{ return onoffstatus_; }


int MultiTextureSurveyObject::getResolution() const
{ return resolution_; }


void
MultiTextureSurveyObject::setChannel2RGBA( visBase::TextureChannel2RGBA* t )
{
    RefMan<visBase::TextureChannel2RGBA> dummy( t );
    if ( !channels_ ) return;

    channels_->setChannels2RGBA( t );
}


visBase::TextureChannel2RGBA* MultiTextureSurveyObject::getChannel2RGBA()
{ return channels_ ? channels_->getChannels2RGBA() : 0; }


void MultiTextureSurveyObject::updateMainSwitch()
{
    bool newstatus = onoffstatus_;
    if ( newstatus )
    {
	newstatus = false;
	for ( int idx=nrAttribs()-1; idx>=0; idx-- )
	{
	    if ( isAttribEnabled(idx) )
	    {
		newstatus = true;
		break;
	    }
	}
    }

    VisualObjectImpl::turnOn( newstatus );
}


bool MultiTextureSurveyObject::isShown() const
{ return VisualObjectImpl::isOn(); }


bool MultiTextureSurveyObject::canHaveMultipleAttribs() const
{ return true; }


int MultiTextureSurveyObject::nrAttribs() const
{ return as_.size(); }


bool MultiTextureSurveyObject::canAddAttrib( int nr ) const
{
    const int maxnr = texture_
	? texture_->maxNrTextures()
	: channels_->getChannels2RGBA()->maxNrChannels();
    if ( !maxnr ) return true;

    return nrAttribs()+nr<=maxnr;
}


bool MultiTextureSurveyObject::canRemoveAttrib() const
{
    if ( texture_ )
	return SurveyObject::canRemoveAttrib();
    const int newnrattribs = nrAttribs()-1;
    if ( newnrattribs<channels_->getChannels2RGBA()->minNrChannels() )
	return false;

    return true;
}


bool MultiTextureSurveyObject::addAttrib()
{
    as_ += new Attrib::SelSpec;
    addCache();

    if ( texture_ )
    {
	while ( texture_->nrTextures()<as_.size() )
	{
	    texture_->addTexture("");
	    texture_->setOperation( texture_->nrTextures()-1,
				    visBase::MultiTexture::BLEND );
	}
    }
    else
    {
	while ( channels_->nrChannels()<as_.size() )
	    channels_->addChannel();
    }

    updateMainSwitch();
    return true;
}


bool MultiTextureSurveyObject::removeAttrib( int attrib )
{
    if ( as_.size()<2 || attrib<0 || attrib>=as_.size() )
	return false;

    delete as_[attrib];
    as_.remove( attrib );

    removeCache( attrib );
    if ( texture_ ) texture_->removeTexture( attrib );
    else channels_->removeChannel( attrib );

    updateMainSwitch();
    return true;
}


bool MultiTextureSurveyObject::swapAttribs( int a0, int a1 )
{
    if ( a0<0 || a1<0 || a0>=as_.size() || a1>=as_.size() )
	return false;

    if ( texture_ ) texture_->swapTextures( a0, a1 );
    else channels_->swapChannels( a0, a1 );
    as_.swap( a0, a1 );
    swapCache( a0, a1 );

    return true;
}


void MultiTextureSurveyObject::clearTextures()
{
    for ( int idx=nrAttribs()-1; idx>=0; idx-- )
    {
	Attrib::SelSpec as; setSelSpec( idx, as );

	for ( int idy=nrTextures(idx)-1; idy>=0; idy-- )
	{
	    if ( texture_ )
		texture_->setData( idx, idy, 0, false );
	    else
	    {
		channels_->setUnMappedData( idx, idy, 0,
			visBase::TextureChannels::None );
	    }
	}
    }
}


void MultiTextureSurveyObject::setAttribTransparency( int attrib,
						      unsigned char nt )
{
    if ( texture_ ) texture_->setTextureTransparency( attrib, nt );
    else
    {
	mDynamicCastGet( visBase::ColTabTextureChannel2RGBA*, cttc2rgba,
		channels_->getChannels2RGBA() );
	if ( cttc2rgba )
	    cttc2rgba->setTransparency( attrib, nt );
    }
}



unsigned char
MultiTextureSurveyObject::getAttribTransparency( int attrib ) const
{
    if ( texture_ )
	return texture_->getTextureTransparency( attrib );

    mDynamicCastGet( visBase::ColTabTextureChannel2RGBA*, cttc2rgba,
	    channels_->getChannels2RGBA() );
    if ( cttc2rgba )
	return cttc2rgba->getTransparency( attrib );

    return 0;
}


const Attrib::SelSpec* MultiTextureSurveyObject::getSelSpec( int attrib ) const
{ return attrib>=0 && attrib<as_.size() ? as_[attrib] : 0; }


void MultiTextureSurveyObject::setSelSpec( int attrib,
					   const Attrib::SelSpec& as )
{
    if ( attrib>=0 && attrib<as_.size() )
	*as_[attrib] = as;

    emptyCache( attrib );

    const char* usrref = as.userRef();
    if ( !usrref || !*usrref )
    {
	if ( texture_ )
	    texture_->enableTexture( attrib, false );
	else
	    channels_->getChannels2RGBA()->setEnabled( attrib, false );
    }
}


bool MultiTextureSurveyObject::isClassification( int attrib ) const
{
    return attrib>=0 && attrib<isclassification_.size()
	? isclassification_[attrib] : false;
}


void MultiTextureSurveyObject::setClassification( int attrib, bool yn )
{
    if ( attrib<0 || attrib>=as_.size() )
	return;

    if ( yn )
    {
	while ( attrib>=isclassification_.size() )
	    isclassification_ += false;
    }
    else if ( attrib>=isclassification_.size() )
	return;

    isclassification_[attrib] = yn;
}


bool MultiTextureSurveyObject::isAngle( int attrib ) const
{
    return texture_ ? texture_->isAngle( attrib ) : false;
}


void MultiTextureSurveyObject::setAngleFlag( int attrib, bool yn )
{
    if ( texture_ )
	texture_->setAngleFlag( attrib, yn );
}


bool MultiTextureSurveyObject::isAttribEnabled( int attrib ) const 
{
    if ( texture_ )
	return texture_->isTextureEnabled( attrib );

    return channels_->getChannels2RGBA()->isEnabled( attrib );
}


void MultiTextureSurveyObject::enableAttrib( int attrib, bool yn )
{
    if ( texture_ )
    {
	texture_->enableTexture( attrib, yn );
	updateMainSwitch();
    }
    else
	channels_->getChannels2RGBA()->setEnabled( attrib, yn );
}


const TypeSet<float>*
MultiTextureSurveyObject::getHistogram( int attrib ) const
{
    return texture_ 
	? texture_->getHistogram( attrib, texture_->currentVersion( attrib ) )
	: 0;
}


int MultiTextureSurveyObject::getColTabID( int attrib ) const
{
    return texture_ ? texture_->getColorTab( attrib ).id() : -1;
}


void MultiTextureSurveyObject::setColTabSequence( int attrib,
						  ColTab::Sequence const& seq )
{
    if ( attrib<0 || attrib>=nrAttribs() )
	return;

    if ( texture_ )
    {
	visBase::VisColorTab& vt = texture_->getColorTab( attrib );
	vt.colorSeq().colors() = seq;
	vt.colorSeq().colorsChanged();
    }
    else
    {
	mDynamicCastGet( visBase::ColTabTextureChannel2RGBA*, cttc2rgba,
		channels_->getChannels2RGBA() );
	if ( cttc2rgba )
	    cttc2rgba->setSequence( attrib, seq );
    }
}


bool MultiTextureSurveyObject::canSetColTabSequence() const
{
    if ( texture_ ) return true;
    return channels_->getChannels2RGBA()->canSetSequence();
}



const ColTab::Sequence*
MultiTextureSurveyObject::getColTabSequence( int attrib ) const
{
    if ( attrib<0 || attrib>=nrAttribs() )
	return 0;

    if ( texture_ )
    {
	const visBase::VisColorTab& vt = texture_->getColorTab( attrib );
	return &vt.colorSeq().colors();
    }

    return channels_->getChannels2RGBA()->getSequence( attrib );
}


void MultiTextureSurveyObject::setColTabMapperSetup( int attrib,
					  const ColTab::MapperSetup& mapper )
{
    if ( attrib<0 || attrib>=nrAttribs() )
	return;

    if ( texture_ )
    {
	visBase::VisColorTab& vt = texture_->getColorTab( attrib );
	const bool autoscalechange =
	    mapper.type_!=vt.colorMapper().setup_.type_ &&
	    mapper.type_!=ColTab::MapperSetup::Fixed;

	vt.colorMapper().setup_ = mapper;

	if ( autoscalechange )
	{
	    vt.colorMapper().setup_.triggerAutoscaleChange();
	    vt.autoscalechange.trigger();
	}
	else
	{
	    vt.colorMapper().setup_.triggerRangeChange();
	    vt.rangechange.trigger();
	}
    }
    else
    {
	channels_->setColTabMapperSetup( attrib, mapper );
    }
}


const ColTab::MapperSetup*
MultiTextureSurveyObject::getColTabMapperSetup( int attrib ) const
{
    return getColTabMapperSetup( attrib, mUdf(int) );
}


const ColTab::MapperSetup*
MultiTextureSurveyObject::getColTabMapperSetup( int attrib, int version ) const
{
    if ( attrib<0 || attrib>=nrAttribs() )
	return 0;

    if ( texture_ )
    {
	//TODO: could it be handy to use other version than the current one?
	const visBase::VisColorTab& vt = texture_->getColorTab( attrib );
	return &vt.colorMapper().setup_;
    }

    if ( mIsUdf(version) || version<0
	    		 || version >= channels_->nrVersions(attrib) )
	version = channels_->currentVersion( attrib );

    return &channels_->getColTabMapperSetup( attrib, version );
}


int MultiTextureSurveyObject::nrTextures( int attrib ) const
{
    return texture_
	? texture_->nrVersions( attrib )
	: channels_->nrVersions( attrib );
}


void MultiTextureSurveyObject::selectTexture( int attrib, int idx )
{
    if ( attrib<0 || attrib>=nrAttribs() || idx<0 )
	return;

    if ( texture_ )
    {
	if ( idx>=texture_->nrVersions(attrib) ) return;

	texture_->setCurrentVersion( attrib, idx );
    }
    else
    {
	if ( idx>=channels_->nrVersions(attrib) )
	    return;

	channels_->setCurrentVersion( attrib, idx );
    }
}


int MultiTextureSurveyObject::selectedTexture( int attrib ) const
{ 
    if ( attrib<0 || attrib>=nrAttribs() ) return 0;

    return texture_
	? texture_->currentVersion( attrib )
	: channels_->currentVersion( attrib );
}


void MultiTextureSurveyObject::fillPar( IOPar& par,
					TypeSet<int>& saveids ) const
{
    visBase::VisualObject::fillPar( par, saveids );

    par.set( sKeyResolution(), resolution_ );
    par.setYN( visBase::VisualObjectImpl::sKeyIsOn(), isOn() );
    for ( int attrib=as_.size()-1; attrib>=0; attrib-- )
    {
	IOPar attribpar;
	as_[attrib]->fillPar( attribpar );

	if ( canSetColTabSequence() && getColTabSequence( attrib ) )
	{
	    IOPar seqpar;
	    const ColTab::Sequence* seq = getColTabSequence( attrib );
	    if ( seq->isSys() )
		seqpar.set( sKey::Name, seq->name() );
	    else
		seq->fillPar( seqpar );

	    attribpar.mergeComp( seqpar, sKeySequence() );
	}

	if ( getColTabMapperSetup( attrib ) )
	{
	    IOPar mapperpar;
	    getColTabMapperSetup( attrib )->fillPar( mapperpar );
	    attribpar.mergeComp( mapperpar, sKeyMapper() );
	}

	attribpar.set( sKeyTextTrans(),
		       getAttribTransparency( attrib ) );
	attribpar.setYN( visBase::VisualObjectImpl::sKeyIsOn(),
			 isAttribEnabled( attrib ) );
	    	     
	BufferString key = sKeyAttribs();
	key += attrib;
	par.mergeComp( attribpar, key );
    }

    par.set( sKeyNrAttribs(), as_.size() );
    fillSOPar( par );
}


int MultiTextureSurveyObject::usePar( const IOPar& par )
{
    const int res =  visBase::VisualObject::usePar( par );
    if ( res!=1 ) return res;

    par.get( sKeyResolution(), resolution_ );

    bool ison = true;
    par.getYN( visBase::VisualObjectImpl::sKeyIsOn(), ison );
    turnOn( ison );

    int nrattribs;
    if ( par.get(sKeyNrAttribs(),nrattribs) ) //current format
    {
	TypeSet<int> coltabids( nrattribs, -1 );

	//This loop is only needed for reading pars up to od3.3
	for ( int attrib=0; attrib<nrattribs; attrib++ )
	{
	    BufferString key = sKeyAttribs();
	    key += attrib;
	    PtrMan<const IOPar> attribpar = par.subselect( key );
	    if ( !attribpar )
		continue;

	    if ( attribpar->get(sKeyColTabID(),coltabids[attrib]) )
	    {
		visBase::DataObject* dataobj =
		    visBase::DM().getObject( coltabids[attrib] );
		if ( !dataobj ) return 0;
		mDynamicCastGet(const visBase::VisColorTab*,coltab,dataobj);
		if ( !coltab ) coltabids[attrib] = -1;
	    }
	}

	bool firstattrib = true;
	for ( int attrib=0; attrib<nrattribs; attrib++ )
	{
	    BufferString key = sKeyAttribs();
	    key += attrib;
	    PtrMan<const IOPar> attribpar = par.subselect( key );
	    if ( !attribpar )
		continue;

	    if ( !firstattrib )
		addAttrib();
	    else
		firstattrib = false;

	    const int attribnr = as_.size()-1;

	    as_[attribnr]->usePar( *attribpar );
	    const int coltabid = coltabids[attribnr];
	    if ( coltabid!=-1 ) //format up to od3.3
	    {
		mDynamicCastGet(visBase::VisColorTab*,coltab, 
		       		visBase::DM().getObject(coltabid) );
		if ( texture_ )
		    texture_->setColorTab( attribnr, *coltab );
		else
		{
		    channels_->setColTabMapperSetup( attribnr,
			        coltab->colorMapper().setup_ );
		    channels_->getChannels2RGBA()->setSequence( attribnr,
			    coltab->colorSeq().colors() );
		}
	    }
	    else
	    {
		PtrMan<IOPar> seqpar = attribpar->subselect( sKeySequence() );
		if ( seqpar )
		{
		    ColTab::Sequence seq;
		    if ( !seq.usePar( *seqpar ) )
		    {
			BufferString seqname;
			if ( seqpar->get( sKey::Name, seqname ) ) //Sys
			    ColTab::SM().get( seqname.buf(), seq );
		    }

		    setColTabSequence( attribnr, seq );
		}

		PtrMan<IOPar> mappar = attribpar->subselect( sKeyMapper() );
		if ( mappar )
		{
		    ColTab::MapperSetup mapper;
		    mapper.usePar( *mappar );
		    setColTabMapperSetup( attribnr, mapper );
		}
	    }

	    ison = true;
	    attribpar->getYN( visBase::VisualObjectImpl::sKeyIsOn(), ison );
	    enableAttrib( attribnr, ison );

	    unsigned int trans = 0;
	    attribpar->get( sKeyTextTrans(), trans );
	    setAttribTransparency( attribnr, trans );
	}
    }

    if ( !useSOPar( par ) )
	return -1;

    return 1;
}


void MultiTextureSurveyObject::getValueString( const Coord3& pos,
						BufferString& val ) const
{
    val = "undef";
    BufferString valname;

    mDynamicCastGet( visBase::ColTabTextureChannel2RGBA*, ctab,
	    channels_ ? channels_->getChannels2RGBA() : 0 );

    for ( int idx=nrAttribs()-1; idx>=0; idx-- )
    {
	if ( !isAttribEnabled(idx) )
	    continue;

	int version;
	if ( texture_ )
	{
	    if ( texture_->getTextureTransparency(idx)==255 )
		continue;

	    version = texture_->currentVersion(idx);
	}
	else if ( ctab )
	{
	    if ( ctab->getTransparency(idx)==255 )
		continue;
	    version = channels_->currentVersion( idx );
	}
	else
	{
	    continue;
	}

	float fval;
	if ( !getCacheValue(idx, version, pos, fval ) )
	    continue;

	if ( !Math::IsNormalNumber(fval) )
	    fval = mUdf(float);

	bool islowest = true;
	for ( int idy=idx-1; idy>=0; idy-- )
	{
	    if ( !hasCache(idy) ||
		 !isAttribEnabled(idy) ||
		 (texture_ && texture_->getTextureTransparency(idy)==255 ) ||
		 (ctab && ctab->getTransparency(idx)==255 ) )
		continue;

	    islowest = false;
	    break;
	}    

	if ( !islowest )
	{
	    Color col;
	    if ( texture_ )
	    {
		col = texture_->getColorTab(idx).color(fval);
	    }
	    else if ( ctab )
	    {
		const ColTab::Sequence* seq = ctab->getSequence( idx );
		const ColTab::Mapper& map = channels_->getColTabMapper(idx,
			channels_->currentVersion( idx ) );

		col = mIsUdf(fval)
		    ? seq->undefColor()
		    : seq->color( map.position(fval) );
	    }
	    else
	    {
		continue;
	    }

	    if ( col.t()==255 )
		continue;
	}

	if ( !mIsUdf(fval) )
	    val = fval;

	if ( nrAttribs()>1 )
	{
	    BufferString attribstr = "(";
	    attribstr += as_[idx]->userRef();
	    attribstr += ")";
	    val.replaceAt( cValNameOffset(), (const char*)attribstr);
	}

	return;
    }
}


}; // namespace visSurvey
