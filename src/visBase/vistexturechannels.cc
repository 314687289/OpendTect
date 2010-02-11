/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jun 2008
___________________________________________________________________

-*/

static const char* rcsID = "$Id: vistexturechannels.cc,v 1.31 2010-02-11 23:13:36 cvsyuancheng Exp $";

#include "vistexturechannels.h"

#include "vistexturechannel2rgba.h"
#include "SoTextureChannelSet.h"
#include "Inventor/nodes/SoSwitch.h"
#include "coltabmapper.h"

#define mNrColors	255


mCreateFactoryEntry( visBase::TextureChannels );


namespace visBase
{


class ChannelInfo : public CallBacker
{
public:
				ChannelInfo( TextureChannels& );
				~ChannelInfo();

    void			setColTabMapperSetup(
	    					const ColTab::MapperSetup&);
    const ColTab::MapperSetup&	getColTabMapperSetup(int version) const;
    const ColTab::Mapper&	getColTabMapper(int version) const;
    bool			reMapData(TaskRunner*);
    const TypeSet<float>&	getHistogram() const	{ return histogram_; }

    void			setNrVersions(int);
    int				nrVersions() const;

    void			removeCaches();

    bool			setUnMappedData(int version,const float*,
	    					OD::PtrPolicy, TaskRunner*);
    bool			setMappedData(int version,unsigned char*,
	    				      OD::PtrPolicy);

    void			clipData(int version,TaskRunner*);
    				//!<If version==-1, all versions will be clipped
    bool			mapData(int version,TaskRunner*);
    int				getCurrentVersion() const;
    void			setCurrentVersion( int );

    ObjectSet<unsigned char>			mappeddata_;
    BoolTypeSet					ownsmappeddata_;
    ObjectSet<const float>			unmappeddata_;
    BoolTypeSet					ownsunmappeddata_;
    ObjectSet<ColTab::Mapper>			mappers_;
    int						currentversion_;
    TextureChannels&				owner_;
    TypeSet<float>				histogram_;
};


ChannelInfo::ChannelInfo( TextureChannels& nc )
    : owner_( nc )
    , currentversion_( 0 )
    , histogram_( mNrColors, 0 )
{
    mappeddata_.allowNull(true);
    unmappeddata_.allowNull(true);
    setNrVersions( 1 );
}


ChannelInfo::~ChannelInfo()
{
    setNrVersions( 0 );
}


const ColTab::Mapper& ChannelInfo::getColTabMapper( int version ) const
{ return *mappers_[version]; }


void ChannelInfo::setColTabMapperSetup( const ColTab::MapperSetup& setup )
{
    if ( !mappers_.size() )
    {
	pErrMsg("No mappers");
	return;
    }

    if ( mappers_[0]->setup_==setup )
	return;

    for ( int idx=0; idx<mappers_.size(); idx++ )
    {
	const bool autoscalechange = mappers_[idx]->setup_.type_ != setup.type_;
	mappers_[idx]->setup_ = setup;
	if ( autoscalechange )
	    mappers_[idx]->setup_.triggerAutoscaleChange();
	else
	    mappers_[idx]->setup_.triggerRangeChange();
    }

}


const ColTab::MapperSetup& ChannelInfo::getColTabMapperSetup(int channel) const
{
    if ( channel < mappers_.size() )
	return mappers_[channel]->setup_;
    else
    {
	pErrMsg( "channel >= mappers_.size()" );
	if ( mappers_.isEmpty() )
	{
	    static ColTab::MapperSetup ctms;
	    return ctms;
	}
	return mappers_[mappers_.size()-1]->setup_;
    }
}


void ChannelInfo::clipData( int version, TaskRunner* tr )
{
    od_int64 sz = owner_.size_[0];
    sz *= owner_.size_[1];
    sz *= owner_.size_[2];

    for ( int idx=0; idx<nrVersions(); idx++ )
    {
	if ( version!=-1 && idx!=version )
	    continue;

	if ( !unmappeddata_[idx] )
	    continue;

	const ArrayValueSeries<float,float> valseries(
		(float*) unmappeddata_[idx], false, sz );
	mappers_[idx]->setData( &valseries, sz, tr );
	mappers_[idx]->setup_.triggerRangeChange();
    }
}


bool ChannelInfo::reMapData( TaskRunner* tr ) 
{
    for ( int idx=nrVersions()-1; idx>=0; idx-- )
    {
	if ( mappers_[idx]->setup_.type_!=ColTab::MapperSetup::Fixed )
	    clipData( idx, tr );

	if ( !mapData( idx, tr ) )
	    return false;
    }

    return true;
}


void ChannelInfo::removeCaches()
{
    ObjectSet<unsigned char> mappeddata = mappeddata_;
    ObjectSet<const float> unmappeddata = unmappeddata_;
    for ( int idx=0; idx<ownsmappeddata_.size(); idx++ )
    {
	mappeddata_.replace( idx, 0 );
	unmappeddata_.replace( idx, 0 );
    }

    owner_.update( this, true );

    for ( int idx=0; idx<ownsmappeddata_.size(); idx++ )
    {
	if ( ownsmappeddata_[idx] )
	    delete [] mappeddata[idx];
	if ( ownsunmappeddata_[idx] )
	    delete [] unmappeddata[idx];
    }
}


void ChannelInfo::setNrVersions( int nsz )
{
    if ( nsz && currentversion_>=nsz )
	setCurrentVersion( nsz-1 );

    while ( nsz<mappeddata_.size() )
    {
	if ( ownsmappeddata_[nsz] )
	    delete [] mappeddata_[nsz];
	if ( ownsunmappeddata_[nsz] )
	    delete [] unmappeddata_[nsz];

	mappeddata_.remove( nsz );
	unmappeddata_.remove( nsz );
	ownsmappeddata_.remove( nsz );
	ownsunmappeddata_.remove( nsz );
	delete mappers_.remove( nsz );
    }

    const ColTab::MapperSetup* templ = mappers_.size()
	? &mappers_[0]->setup_
	: 0;

    while ( mappeddata_.size()<nsz )
    {
	mappeddata_ += 0;
	unmappeddata_ += 0;
	ownsmappeddata_ += false;
	ownsunmappeddata_ += false;

	ColTab::Mapper* mapper = new ColTab::Mapper;
	if ( templ )
	    mapper->setup_ = *templ;
	mappers_ += mapper;
    }
}


int ChannelInfo::nrVersions() const
{ return ownsmappeddata_.size(); }


bool ChannelInfo::setUnMappedData(int version, const float* data,
	    			  OD::PtrPolicy policy, TaskRunner* tr )
{
    if ( version<0 || version>=nrVersions() )
    {
	if ( policy==OD::TakeOverPtr ) delete [] data;
	return false;
    }

    od_int64 sz = owner_.size_[0];
    sz *= owner_.size_[1];
    sz *= owner_.size_[2];


    if ( unmappeddata_[version] )
    {
	if ( ownsunmappeddata_[version] )
	    delete [] unmappeddata_[version];

	unmappeddata_.replace( version, 0 );
    }

    if ( policy==OD::UsePtr || policy==OD::TakeOverPtr )
    {
	unmappeddata_.replace( version, data );
	ownsunmappeddata_[version] = policy==OD::TakeOverPtr;
    }
    else if ( policy==OD::CopyPtr )
    {
	if ( data )
	{
	    mDeclareAndTryAlloc(float*, newdata, float[sz] );
	    if ( !newdata ) return false;
	    MemCopier<float> copier( newdata, data, sz );
	    copier.execute();
	    unmappeddata_.replace( version, newdata );
	}
	else
	{
	    unmappeddata_.replace( version, 0 );
	}
	    
	ownsunmappeddata_[version] = true;
    }

    if ( mappers_[version]->setup_.type_!=ColTab::MapperSetup::Fixed )
	clipData( version, tr );

    return mapData( version, tr );
}


bool ChannelInfo::mapData( int version, TaskRunner* tr )
{
    if ( version<0 || version>=unmappeddata_.size() )
	return true;

    //Make sure old data is alive until it's replaced in Coin
    ArrPtrMan<unsigned char> oldptr = 0;

    if ( mappeddata_[version] &&
 	 (!ownsmappeddata_[version] || !unmappeddata_[version] ) )
    {
	if ( ownsmappeddata_[version] )
	    oldptr = mappeddata_[version];

	mappeddata_.replace( version, 0 );
    }

    if ( !unmappeddata_[version] )
    {
	owner_.update( this, true );
	return true;
    }

    od_int64 sz = owner_.size_[0];
    sz *= owner_.size_[1];
    sz *= owner_.size_[2];


    if ( !mappeddata_[version] )
    {
	mDeclareAndTryAlloc(unsigned char*, mappeddata, unsigned char[sz] );
	if ( !mappeddata ) return false;

	mappeddata_.replace( version, mappeddata );
	ownsmappeddata_[version] = true;
    }

    ColTab::MapperTask< unsigned char> 	maptask( *mappers_[version], sz,
	    mNrColors, unmappeddata_[version], mappeddata_[version] );
    if ( ( tr && tr->execute(maptask) ) || maptask.execute() )
    {
	int max = 0;
	const unsigned int* histogram = maptask.getHistogram();
	for ( int idx=mNrColors-1; idx>=0; idx-- )
	{
	    if ( histogram[idx]>max )
		max = histogram[idx];
	}

	if ( max )
	{
	    for ( int idx=mNrColors-1; idx>=0; idx-- )
		histogram_[idx] = (float) histogram[idx]/max;
	}
	else
	{
	    memset( histogram_.arr(), 0, histogram_.size()*sizeof(float) );
	}

	owner_.update( this, true );
	return true;
    }

    return false;
}


bool ChannelInfo::setMappedData( int version, unsigned char* data,
	    			  OD::PtrPolicy policy )
{
    if ( mappeddata_.size()<version )
    {
	pErrMsg("nrVersions not set" );
	return false;
    }

    if ( mappeddata_[version] )
    {
	if (  ownsmappeddata_[version] )
	    delete mappeddata_[version];
	mappeddata_.replace( version, 0 );
    }

    if ( policy==OD::UsePtr || policy==OD::TakeOverPtr )
    {
	mappeddata_.replace( version, data );
	ownsmappeddata_[version] = policy==OD::TakeOverPtr;
    }
    else if ( policy==OD::CopyPtr )
    {
	od_int64 sz = owner_.size_[0];
	sz *= owner_.size_[1];
	sz *= owner_.size_[2];

	if ( data )
	{
    	    mDeclareAndTryAlloc(unsigned char*, newdata, unsigned char[sz] );
    	    if ( !newdata ) return false;
	    MemCopier<unsigned char> copier( newdata, data, sz );
	    copier.execute();
    	    mappeddata_.replace( version, newdata );
	}
	else
	{
	    mappeddata_.replace( version, 0 );
	}
	
	ownsmappeddata_[version] = true;
    }

    owner_.update( this, true );
    return true;
}


int ChannelInfo::getCurrentVersion() const { return currentversion_; }


void ChannelInfo::setCurrentVersion( int nidx )
{
    if ( mappeddata_.size() && (nidx<0 || nidx>=mappeddata_.size() ) )
    {
	pErrMsg("Invalid index");
	return;
    }

    currentversion_ = nidx;
    owner_.update( this, true );
}


TextureChannels::TextureChannels()
    : tc_( 0 )
    , onoff_ ( new SoSwitch )
    , tc2rgba_( 0 )
{
    size_[0] = 0;
    size_[1] = 0;
    size_[2] = 0;

    onoff_->ref();
    addChannel();
    turnOn( true );
}


TextureChannels::~TextureChannels()
{
    deepErase( channelinfo_ );
    setChannels2RGBA( 0 );
    onoff_->unref();
}


void TextureChannels::setSize( int s0, int s1, int s2 )
{
    if ( size_[0]==s0 && size_[1]==s1 && size_[2]==s2 )
	return;

    size_[0]=s0;
    size_[1]=s1;
    size_[2]=s2;

    for ( int idx=0; idx<nrChannels(); idx++)
	channelinfo_[idx]->removeCaches();
}


int TextureChannels::getSize( int dim ) const
{ return size_[dim]; }


bool TextureChannels::turnOn( bool yn )
{
    const bool res = isOn();
    const int newval = yn ? SO_SWITCH_ALL : SO_SWITCH_NONE;
    if ( newval!=onoff_->whichChild.getValue() )
	onoff_->whichChild = newval;
    return res;
}


bool TextureChannels::isOn() const
{
    return onoff_->whichChild.getValue()!=SO_SWITCH_NONE;
}


int TextureChannels::nrChannels() const
{ return channelinfo_.size(); }


int TextureChannels::addChannel()
{
    ChannelInfo* newchannel = new ChannelInfo( *this );

    const int res = channelinfo_.size();
    channelinfo_ += newchannel;
    update ( res, false );
    return res;
};


void TextureChannels::swapChannels( int t0, int t1 )
{
    if ( t0<0 || t1<0 || t0>=channelinfo_.size() || t1>=channelinfo_.size() )
	return;

    channelinfo_.swap( t0, t1 );


    update( t0, false );
    update( t1, false );

    if ( tc2rgba_ )
	tc2rgba_->swapChannels( t0, t1 );
}


int TextureChannels::insertChannel( int idx )
{
    if ( idx>=channelinfo_.size() )
	return addChannel();

    if ( idx<0 )
    {
	pErrMsg("Negative index");
	idx=0;
    }

    ChannelInfo* newchannel = new ChannelInfo( *this );
    channelinfo_.insertAt( newchannel, idx );
    for ( int idy=idx; idy<nrChannels(); idy++ )
	update( idy, false );

    if ( tc2rgba_ )
	tc2rgba_->notifyChannelChange();
    
    return idx;
}


void TextureChannels::removeChannel( int idx )
{
    if ( idx<0 || idx>=channelinfo_.size() )
	return;

    PtrMan<ChannelInfo> info = channelinfo_[idx];
    channelinfo_.remove(idx);

    bool oldenable = tc_->enableNotify( false );
    for ( int idy=idx; idy<nrChannels(); idy++ )
	update( idy, false );

    tc_->setNrChannels( nrChannels() );
    tc_->enableNotify( oldenable );
    tc_->touch();

    if ( tc2rgba_ )
	tc2rgba_->notifyChannelChange();
}


void TextureChannels::setColTabMapperSetup( int channel,
					    const ColTab::MapperSetup& setup )
{
    if ( channel<0 || channel>=channelinfo_.size() )
	pErrMsg("Index out of bounds");

    channelinfo_[channel]->setColTabMapperSetup( setup );
}


void TextureChannels::reMapData( int channel, TaskRunner* tr )
{
    if ( channel<0 || channel>=channelinfo_.size() )
	pErrMsg("Index out of bounds");

    channelinfo_[channel]->reMapData( tr );
}


const TypeSet<float>* TextureChannels::getHistogram( int channel ) const
{
    if ( !channelinfo_.validIdx(channel) )
	return 0;

    return &channelinfo_[channel]->getHistogram();
}


const ColTab::MapperSetup&
TextureChannels::getColTabMapperSetup( int channel, int version ) const
{ return channelinfo_[channel]->getColTabMapperSetup( version ); }


const ColTab::Mapper&
TextureChannels::getColTabMapper( int channel, int version ) const
{ return channelinfo_[channel]->getColTabMapper( version ); }


int TextureChannels::nrVersions( int idx ) const 
{ return channelinfo_[idx]->nrVersions(); }


void TextureChannels::setNrVersions( int idx, int newsz )
{
    if ( idx<0 || idx>=channelinfo_.size() )
	return;

    channelinfo_[idx]->setNrVersions( newsz );
}


int TextureChannels::currentVersion( int idx ) const
{
    if ( idx<0 || idx>=channelinfo_.size() )
	idx = 0;

    return channelinfo_[idx]->getCurrentVersion();
}


void TextureChannels::setCurrentVersion( int idx, int version )
{
    if ( idx<0 || idx>=channelinfo_.size() )
	idx = 0;

    channelinfo_[idx]->setCurrentVersion( version );
}


#define mErrRet \
{ \
    if ( managedata ) delete [] data; \
    return false; \
}

bool TextureChannels::setUnMappedData( int channel, int version,
	const float* data, OD::PtrPolicy cp, TaskRunner* tr )
{
    if ( channel<0 || channel>=channelinfo_.size() )
    {
	if ( cp==OD::TakeOverPtr ) delete [] data;
	return false;
    }

    return channelinfo_[channel]->setUnMappedData( version, data, cp, tr );
}


bool TextureChannels::setMappedData( int channel, int version,
				     unsigned char* data, 
				     OD::PtrPolicy cp )
{
    if ( channel<0 || channel>=channelinfo_.size() )
    {
	if ( cp==OD::TakeOverPtr ) delete [] data;
	return false;
    }

    return channelinfo_[channel]->setMappedData( version, data, cp );
}


bool TextureChannels::setChannels2RGBA( TextureChannel2RGBA* nt )
{
    if ( tc2rgba_ )
    {
	onoff_->removeChild( tc2rgba_->getInventorNode() );
	tc2rgba_->setChannels( 0 );
	tc2rgba_->unRef();

	onoff_->removeChild( tc_->getInventorNode() );
	tc_->unRef();
	tc_ = 0;
    }

    tc2rgba_ = nt;

    if ( tc2rgba_ )
    {
	tc_ = tc2rgba_->createMappedDataSet();
	if ( !tc_ )
	{
	    tc2rgba_ = 0;
	    return false;
	}

	tc_->ref();
	onoff_->addChild( tc_->getInventorNode() );

	onoff_->addChild( tc2rgba_->getInventorNode() );
	tc2rgba_->setChannels( this );
	tc2rgba_->ref();
    }

    return true;
}


const TextureChannel2RGBA* TextureChannels::getChannels2RGBA() const
{ return tc2rgba_; }


TextureChannel2RGBA* TextureChannels::getChannels2RGBA()
{ return tc2rgba_; }


SoNode* TextureChannels::getInventorNode()
{ return onoff_; }


const SbImage* TextureChannels::getChannels() const
{
    return tc_->getChannelData();
}


void TextureChannels::update( ChannelInfo* ti, bool tc2rgba )
{
    const int idx= channelinfo_.indexOf( ti );
    if ( idx==-1 )
	return;

    update( idx, tc2rgba );
}


void TextureChannels::update( int channel, bool tc2rgba )
{
    if ( !tc_ )
	return;

    SbImage image;
    const int curversion = channelinfo_[channel]->getCurrentVersion();
    image.setValuePtr( SbVec3s( size_[0], size_[1], size_[2] ), 1,
	    	       channelinfo_[channel]->mappeddata_[curversion] );
    tc_->setChannelData( channel, image );

    if ( tc2rgba && tc2rgba_ )
	tc2rgba_->notifyChannelChange();
}


}; // namespace visBase
