/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        D. Zheng
 Date:          Feb 2013
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "vishorizondisplay.h"

#include "attribsel.h"
#include "bidvsetarrayadapter.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "datapointset.h"
#include "datacoldef.h"
#include "emhorizon3d.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceedgeline.h"
#include "isocontourtracer.h"
#include "settings.h"
#include "survinfo.h"
#include "mpeengine.h"
#include "posvecdataset.h"
#include "callback.h"

#include "viscoord.h"
#include "visdrawstyle.h"
#include "visevent.h"
#include "vishingeline.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "vismpe.h"
#include "vishorizonsection.h"
#include "vishorizonsectiondef.h"
#include "visplanedatadisplay.h"
#include "vispolyline.h"
#include "visrandomtrackdisplay.h"
#include "vistexturechannel2rgba.h"
#include "vistexturechannels.h"
#include "vismultiattribsurvobj.h"
#include "visseis2ddisplay.h"
#include "vistransform.h"
#include "zaxistransform.h"


namespace visSurvey
{

const char* HorizonDisplay::sKeyTexture()	{ return "Use texture"; }
const char* HorizonDisplay::sKeyShift()		{ return "Shift"; }
const char* HorizonDisplay::sKeyResolution()	{ return "Resolution"; }
const char* HorizonDisplay::sKeyEdgeLineRadius(){ return "Edgeline radius"; }
const char* HorizonDisplay::sKeyRowRange()	{ return "Row range"; }
const char* HorizonDisplay::sKeyColRange()	{ return "Col range"; }
const char* HorizonDisplay::sKeySurfaceGrid()	{ return "SurfaceGrid"; }
const char* HorizonDisplay::sKeyIntersectLineMaterialID()
{ return "Intsectline material id"; }


HorizonDisplay::HorizonDisplay()
    : parrowrg_( -1, -1, -1 )
    , parcolrg_( -1, -1, -1 )
    , curtextureidx_( 0 )
    , usestexture_( false )
    , translation_( 0 )
    , edgelineradius_( 3.5 )
    , validtexture_( false )
    , resolution_( 0 )
    , allowshading_( true )
    , intersectionlinematerial_( 0 )
    , displayintersectionlines_( true )
    , enabletextureinterp_( true )
    , displaysurfacegrid_( false )
{
    setLockable();
    maxintersectionlinethickness_ = 0.02f *
	mMAX( SI().inlDistance() * SI().inlRange(true).width(),
	      SI().crlDistance() * SI().crlRange(true).width() );

    as_ += new Attrib::SelSpec;
    coltabmappersetups_ += ColTab::MapperSetup();
    coltabsequences_ += ColTab::Sequence(ColTab::defSeqName());

    TypeSet<float> shift;
    shift += 0.0;
    curshiftidx_ += 0;
    BufferStringSet* attrnms = new BufferStringSet();
    attrnms->allowNull();
    userrefs_ += attrnms;
    shifts_ += new TypeSet<float>;
    enabled_ += true;
    datapackids_ += -1;

    material_->setAmbience( 0.3 );

    RefMan<visBase::Material> linemat = new visBase::Material;
    linemat->setFrom( *material_ );
    linemat->setColor( nontexturecol_ );
    linemat->setDiffIntensity( 1 );
    linemat->setAmbience( 1 );
    setIntersectLineMaterial( linemat );
}


HorizonDisplay::~HorizonDisplay()
{
    deepErase(as_);
    if ( intersectionlinematerial_ )
	intersectionlinematerial_->unRef();

    coltabmappersetups_.erase();
    coltabsequences_.erase();

    setSceneEventCatcher( 0 );
    curshiftidx_.erase();

   if ( translation_ )
    {
	removeChild( translation_->osgNode() );
	translation_->unRef();
	translation_ = 0;
    }

    removeEMStuff();
    if ( zaxistransform_ )
    {
	for ( int idx=0; idx<intersectionlinevoi_.size(); idx++ )
	    zaxistransform_->removeVolumeOfInterest(intersectionlinevoi_[idx]);

	zaxistransform_->unRef();
    }

    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );
    for ( int idx=0; idx<datapackids_.size(); idx++ )
	dpman.release( datapackids_[idx] );

    deepErase( shifts_ );

}


void HorizonDisplay::displayIntersectionLines( bool yn )
{
    displayintersectionlines_ = yn;

    hasmoved.trigger();
    changedisplay.trigger();
}


bool HorizonDisplay::displaysIntersectionLines() const
{
    return displayintersectionlines_;
}


void HorizonDisplay::setDisplayTransformation( const mVisTrans* nt )
{
    EMObjectDisplay::setDisplayTransformation( nt );

    MouseCursorChanger cursorchanger( MouseCursor::Wait );

    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->setDisplayTransformation(transformation_);

    for ( int idx=0; idx<edgelinedisplays_.size(); idx++ )
	edgelinedisplays_[idx]->setDisplayTransformation(transformation_);

    for ( int idx=0; idx<intersectionlines_.size(); idx++ )
	intersectionlines_[idx]->setDisplayTransformation(transformation_);

    for ( int idx=0; idx<intersectionpointsets_.size(); idx++ )
	intersectionpointsets_[idx]->setDisplayTransformation(transformation_);

}


bool HorizonDisplay::setZAxisTransform( ZAxisTransform* nz, TaskRunner* tr )
{
    if ( zaxistransform_ ) zaxistransform_->unRef();

    zaxistransform_ = nz;
    if ( zaxistransform_ ) zaxistransform_->ref();

    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->setZAxisTransform( nz, tr );

    return true;
}


bool HorizonDisplay::setChannels2RGBA( visBase::TextureChannel2RGBA* t )
{
    RefMan<visBase::TextureChannel2RGBA> dummy( t );
    if ( sections_.size()!=1 )
	return EMObjectDisplay::setChannels2RGBA( t );

    EMObjectDisplay::setChannels2RGBA( 0 );
    sections_[0]->setChannels2RGBA( t );

    return true;
}


visBase::TextureChannel2RGBA* HorizonDisplay::getChannels2RGBA()
{
    return sections_.size()
	? sections_[0]->getChannels2RGBA()
	: EMObjectDisplay::getChannels2RGBA();
}

const visBase::TextureChannel2RGBA* HorizonDisplay::getChannels2RGBA() const
{ return const_cast<HorizonDisplay*>(this)->getChannels2RGBA(); }


void HorizonDisplay::setSceneEventCatcher(visBase::EventCatcher* ec)
{
    EMObjectDisplay::setSceneEventCatcher( ec );

    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	sections_[idx]->setSceneEventCatcher( ec );
	if ( ec && scene_ )
	    sections_[idx]->setRightHandSystem( scene_->isRightHandSystem() );
    }

    for ( int idx=0; idx<edgelinedisplays_.size(); idx++ )
	edgelinedisplays_[idx]->setSceneEventCatcher( ec );

    for ( int idx=0; idx<intersectionlines_.size(); idx++ )
	intersectionlines_[idx]->setSceneEventCatcher( ec );

}


EM::PosID HorizonDisplay::findClosestNode( const Coord3& pickedpos ) const
{
    const mVisTrans* ztrans = scene_->getTempZStretchTransform();
    Coord3 newpos;
    ztrans->transformBack( pickedpos, newpos );
    if ( transformation_ )
	transformation_->transformBack( newpos );

    const BinID pickedbid = SI().transform( newpos );
    const EM::SubID pickedsubid = pickedbid.toInt64();
    TypeSet<EM::PosID> closestnodes;
    for ( int idx=sids_.size()-1; idx>=0; idx-- )
    {
	if ( !emobject_->isDefined( sids_[idx], pickedsubid ) )
	    continue;
	closestnodes += EM::PosID( emobject_->id(), sids_[idx], pickedsubid );
    }

    if ( closestnodes.isEmpty() ) return EM::PosID( -1, -1, -1 );

    EM::PosID closestnode = closestnodes[0];
    float mindist = mUdf(float);
    for ( int idx=0; idx<closestnodes.size(); idx++ )
    {
	const Coord3 coord = emobject_->getPos( closestnodes[idx] );
	Coord3 displaypos;
	mVisTrans::transform( transformation_, coord, displaypos );
	mVisTrans::transform( ztrans, displaypos );

	const float dist = (float) displaypos.distTo( pickedpos );
	if ( !idx || dist<mindist )
	{
	    closestnode = closestnodes[idx];
	    mindist = dist;
	}
    }

    return closestnode;
}


void HorizonDisplay::edgeLineRightClickCB( CallBacker* )
{}


void HorizonDisplay::removeEMStuff()
{
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	removeChild( sections_[idx]->osgNode() );
	sections_[idx]->unRef();
    }

    sections_.erase();
    sids_.erase();

    while ( intersectionlines_.size() )
    {
	intersectionlines_.removeSingle(0)->unRef();
	intersectionpointsets_.removeSingle(0)->unRef();
	intersectionlineids_.removeSingle(0);
	if ( zaxistransform_ )
	    zaxistransform_->removeVolumeOfInterest( intersectionlinevoi_[0] );
	intersectionlinevoi_.removeSingle(0);
    }

    mDynamicCastGet( EM::Horizon3D*, emhorizon, emobject_ );
    if ( emhorizon )
	emhorizon->edgelinesets.addremovenotify.remove(
			mCB(this,HorizonDisplay,emEdgeLineChangeCB ));

    while ( !edgelinedisplays_.isEmpty() )
    {
	EdgeLineSetDisplay* elsd = edgelinedisplays_[0];
	edgelinedisplays_ -= elsd;
	elsd->rightClicked()->remove(
		 mCB(this,HorizonDisplay,edgeLineRightClickCB) );
	removeChild( elsd->osgNode() );
	elsd->unRef();
    }

    EMObjectDisplay::removeEMStuff();
}


bool HorizonDisplay::setEMObject( const EM::ObjectID& newid, TaskRunner* tr )
{
    if ( !EMObjectDisplay::setEMObject( newid, tr ) )
	return false;

    mDynamicCastGet( EM::Horizon3D*, emhorizon, emobject_ );
    if ( emhorizon )
	emhorizon->edgelinesets.addremovenotify.notify(
		mCB(this,HorizonDisplay,emEdgeLineChangeCB) );

    return true;
}


StepInterval<int> HorizonDisplay::geometryRowRange() const
{
    mDynamicCastGet(const EM::Horizon3D*, surface, emobject_ );
    if ( !surface ) return parrowrg_;

    return surface->geometry().rowRange();
}


StepInterval<int> HorizonDisplay::geometryColRange() const
{
    mDynamicCastGet(const EM::Horizon3D*,horizon3d,emobject_);
    return horizon3d ? horizon3d->geometry().colRange() : parcolrg_;
}


bool HorizonDisplay::updateFromEM( TaskRunner* tr )
{
    if ( !EMObjectDisplay::updateFromEM( tr ) )
	return false;
    updateSingleColor();
    return true;
}


void HorizonDisplay::updateFromMPE()
{
    if ( geometryRowRange().nrSteps()<=1 || geometryColRange().nrSteps()<=1 )
	setResolution( 0, 0 ); //Automatic resolution
    EMObjectDisplay::updateFromMPE();
}


bool HorizonDisplay::addEdgeLineDisplay( const EM::SectionID& sid )
{
    mDynamicCastGet( EM::Horizon3D*, emhorizon, emobject_ );
    EM::EdgeLineSet* els = emhorizon
	? emhorizon->edgelinesets.getEdgeLineSet(sid,false) : 0;

    if ( els )
    {
	bool found = false;
	for ( int idx=0; idx<edgelinedisplays_.size(); idx++ )
	{
	    if ( edgelinedisplays_[idx]->getEdgeLineSet()==els )
	    {
		found = true;
		break;
	    }
	}

	if ( !found )
	{
	    visSurvey::EdgeLineSetDisplay* elsd =
		visSurvey::EdgeLineSetDisplay::create();
	    elsd->ref();
	    elsd->setConnect(true);
	    elsd->setEdgeLineSet(els);
	    elsd->setRadius(edgelineradius_);
	    addChild( elsd->osgNode() );
	    elsd->setDisplayTransformation(transformation_);
	    elsd->rightClicked()->notify(
		    mCB(this,HorizonDisplay,edgeLineRightClickCB));
	    edgelinedisplays_ += elsd;
	}
    }

    return true;
}


void HorizonDisplay::useTexture( bool yn, bool trigger )
{
    if ( yn && !validtexture_ )
    {
	for ( int idx=0; idx<nrAttribs(); idx++ )
	{
	    if ( as_[idx]->id().asInt() == Attrib::SelSpec::cNoAttrib().asInt())
	    {
		usestexture_ = yn;
		setDepthAsAttrib(idx);
		return;
	    }
	}
    }

    usestexture_ = yn;

    updateSingleColor();

    if ( trigger )
	changedisplay.trigger();
}


bool HorizonDisplay::usesTexture() const
{ return usestexture_; }


bool HorizonDisplay::showingTexture() const
{
    return validtexture_ && usesTexture();
}


bool HorizonDisplay::shouldUseTexture() const
{
    if ( !validtexture_ || !usestexture_ )
	 return false;

    for ( int idx=nrAttribs()-1; idx>=0; idx-- )
    {
	if ( isAttribEnabled( idx ) )
	    return true;
    }

    return false;
}


bool HorizonDisplay::getOnlyAtSectionsDisplay() const
{ return displayonlyatsections_; }


bool HorizonDisplay::canHaveMultipleAttribs() const
{ return true; }


int HorizonDisplay::nrTextures( int channel ) const
{
    if ( channel<0 || channel>=nrAttribs() || !sections_.size() )
	return 0;

    return sections_[0]->nrVersions( channel );
}


void HorizonDisplay::selectTexture( int channel, int textureidx )
{
    curtextureidx_ = textureidx;
    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->selectActiveVersion( channel, textureidx );

    if ( !as_.validIdx(channel) || !userrefs_.validIdx(channel) ||
	 userrefs_[channel]->isEmpty() )
	return;

    if ( userrefs_[channel]->get(0)=="Section ID" )
	textureidx++;

    BufferString usrref = userrefs_[channel]->validIdx(textureidx) ?
	userrefs_[channel]->get(textureidx) : "<No name>";
    as_[channel]->setUserRef( userrefs_[channel]->get(textureidx) );
}


int HorizonDisplay::selectedTexture( int channel ) const
{
    if ( channel<0 || channel>=nrAttribs() || !sections_.size() )
	return 0;

    return sections_[0]->activeVersion( channel );
}


SurveyObject::AttribFormat HorizonDisplay::getAttributeFormat( int ) const
{
    return sections_.size() ? SurveyObject::RandomPos : SurveyObject::None;
}


int HorizonDisplay::nrAttribs() const
{ return as_.size(); }


bool HorizonDisplay::canAddAttrib( int nr ) const
{
    if ( !sections_.size() )
	return false;

    const int maxnr =  sections_[0]->getChannels2RGBA()->maxNrChannels();
    if ( !maxnr ) return true;

    return nrAttribs()+nr<=maxnr;
}


bool HorizonDisplay::canRemoveAttrib() const
{
    if ( !sections_.size() )
	return false;

    const int newnrattribs = nrAttribs()-1;
    return newnrattribs>=sections_[0]->getChannels2RGBA()->minNrChannels();
}


void HorizonDisplay::setAttribShift( int channel, const TypeSet<float>& shifts )
{
    (*shifts_[channel]) = shifts;
}


bool HorizonDisplay::addAttrib()
{
    as_ += new Attrib::SelSpec;
    TypeSet<float> shift;
    shift += 0.0;
    curshiftidx_ += 0;
    BufferStringSet* attrnms = new BufferStringSet();
    attrnms->allowNull();
    userrefs_ += attrnms;
    enabled_ += true;
    shifts_ += new TypeSet<float>;
    datapackids_ += -1;
    coltabmappersetups_ += ColTab::MapperSetup();
    coltabsequences_ += ColTab::Sequence(ColTab::defSeqName());

    const int curchannel = coltabmappersetups_.size()-1;
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	sections_[idx]->addChannel();
	sections_[idx]->setColTabSequence( curchannel,
		coltabsequences_[curchannel] );

	sections_[idx]->setColTabMapperSetup( curchannel,
		coltabmappersetups_[curchannel], 0 );
    }

    return true;
}


bool HorizonDisplay::removeAttrib( int channel )
{
    if ( channel<0 || channel>=nrAttribs() )
       return true;

    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->removeChannel( channel );

    curshiftidx_.removeSingle( channel );
    userrefs_.removeSingle( channel );
    enabled_.removeSingle( channel );
    delete shifts_.removeSingle( channel );
    DPM( DataPackMgr::FlatID() ).release( datapackids_[channel] );
    datapackids_.removeSingle( channel );
    coltabmappersetups_.removeSingle( channel );
    coltabsequences_.removeSingle( channel );
    delete as_.removeSingle( channel );

    for ( int chan=channel; chan<nrAttribs(); chan++ )
    {
	for ( int idx=0; idx<sections_.size(); idx++ )
	    sections_[idx]->setColTabSequence( chan, coltabsequences_[chan] );
    }

    return true;
}


bool HorizonDisplay::swapAttribs( int a0, int a1 )
{
    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->swapChannels( a0, a1 );

    coltabmappersetups_.swap( a0, a1 );
    coltabsequences_.swap( a0, a1 );

    as_.swap( a0, a1 );
    enabled_.swap( a0, a1 );
    shifts_.swap( a0, a1 );
    curshiftidx_.swap( a0, a1 );
    userrefs_.swap( a0, a1 );
    datapackids_.swap( a0, a1 );
    return true;
}


void HorizonDisplay::setAttribTransparency( int channel, unsigned char nt )
{
    if ( channel<0 || channel>=nrAttribs() )
       return;

    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->setTransparency( channel, nt );
}


unsigned char HorizonDisplay::getAttribTransparency( int channel ) const
{
    if ( channel<0 || channel>=nrAttribs() )
       return 0;

    return sections_.size() ? sections_[0]->getTransparency(channel) : 0;
}


void HorizonDisplay::enableAttrib( int channelnr, bool yn )
{
    enabled_[channelnr] = yn;
    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->getChannels2RGBA()->setEnabled( channelnr, yn );

    updateSingleColor();
}


bool HorizonDisplay::isAttribEnabled( int channel ) const
{
    if ( channel<0 || channel>=nrAttribs() )
       return false;

    return enabled_[channel];
}


void HorizonDisplay::allowShading( bool yn )
{
    allowshading_ = yn;
    for ( int idx=sections_.size()-1; idx>=0; idx-- )
	sections_[idx]->getChannels2RGBA()->allowShading( yn );

}


const Attrib::SelSpec* HorizonDisplay::getSelSpec( int channel ) const
{ return as_.validIdx( channel ) ? as_[channel] : 0; }


void HorizonDisplay::setSelSpec( int channel, const Attrib::SelSpec& as )
{
    if ( as_.validIdx(channel) && as_[channel] )
	(*as_[channel]) = as;
}


void HorizonDisplay::setDepthAsAttrib( int channel )
{
    as_[channel]->set( "Depth", Attrib::SelSpec::cNoAttrib(), false, "" );

    TypeSet<DataPointSet::DataRow> pts;
    ObjectSet<DataColDef> defs;
    DataColDef depthdef( "Depth" );
    defs += &depthdef;
    DataPointSet positions( pts, defs, false, true );
    getRandomPos( positions, 0 );

    if ( !positions.size() ) return;

    BinIDValueSet& bivs = positions.bivSet();
    if ( bivs.nrVals()!=3 )
    {
	pErrMsg( "Hmm" );
	return;
    }

    int depthcol =
	positions.dataSet().findColDef( depthdef, PosVecDataSet::NameExact );
    if ( depthcol==-1 )
	depthcol = 1;

    BinIDValueSet::SPos pos;
    while ( bivs.next(pos,true) )
    {
	float* vals = bivs.getVals(pos);
	if ( zaxistransform_ )
	{
	    vals[depthcol] = zaxistransform_->transform(
		    BinIDValue( bivs.getBinID(pos), vals[0] ) );
	}
	else
	    vals[depthcol] = vals[0];
    }

    createAndDispDataPack( channel, &positions, 0 );

    BufferString seqnm;
    Settings::common().get( "dTect.Color table.Horizon", seqnm );
    ColTab::Sequence seq( seqnm );
    setColTabSequence( channel, seq, 0 );
}


void HorizonDisplay::createAndDispDataPack( int channel,
					    const DataPointSet* positions,
					    TaskRunner* tr )
{
    if ( !positions || sections_.isEmpty() )
	return;

    BufferStringSet* attrnms = new BufferStringSet();
    for ( int idx=0; idx<positions->nrCols(); idx++ )
	attrnms->add( positions->colDef(idx).name_ );
    userrefs_.replace( channel, attrnms );

    setRandomPosData( channel, positions, tr );
    const BinIDValueSet* cache =
	sections_.isEmpty() ? 0 : sections_[0]->getCache( channel );
    const bool isz = attrnms->size()>=1 && attrnms->get(0)=="Depth";

    StepInterval<int> dispinlrg = sections_[0]->displayedRowRange();
    StepInterval<int> dispcrlrg = sections_[0]->displayedColRange();
    BinID step( dispinlrg.step, dispcrlrg.step );

    mDeclareAndTryAlloc(BIDValSetArrAdapter*, bvsarr,
			BIDValSetArrAdapter(*cache,isz?0:2,step));
    const char* catnm = isz ? "Geometry" : "Horizon Data";
    const char* dpnm = isz ? "Depth"
			   : (attrnms->size()>1 ? attrnms->get(1).buf() : "");
    mDeclareAndTryAlloc(MapDataPack*,newpack,MapDataPack(catnm,dpnm,bvsarr));

    StepInterval<double> inlrg( (double)dispinlrg.start, (double)dispinlrg.stop,
				(double)dispinlrg.step );
    StepInterval<double> crlrg( (double)dispcrlrg.start, (double)dispcrlrg.stop,
				(double)dispcrlrg.step );
    BufferStringSet dimnames;
    dimnames.add("X").add("Y").add("In-Line").add("Cross-line");
    newpack->setProps( inlrg, crlrg, true, &dimnames );
    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );
    dpman.add( newpack );
    setDataPackID( channel, newpack->id(), tr );
}


void HorizonDisplay::getRandomPos( DataPointSet& data, TaskRunner* tr ) const
{
    //data.bivSet().allowDuplicateBids(false);
    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->getDataPositions( data, getTranslation().z,
					  sids_[idx], tr );

    data.dataChanged();
}


void HorizonDisplay::getRandomPosCache( int channel, DataPointSet& data ) const
{
    if ( channel<0 || channel>=nrAttribs() )
       return;

    data.clearData();
    for ( int idx=0; idx<userrefs_[channel]->size(); idx++ )
	data.dataSet().add( new DataColDef(userrefs_[channel]->get(idx)) );

    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	const BinIDValueSet* cache = sections_[idx]->getCache( channel );
	if ( cache )
	{
	    data.bivSet().setNrVals( cache->nrVals(), false );
	    data.bivSet().append( *cache );
	}
    }

    data.dataChanged();
}


void HorizonDisplay::updateSingleColor()
{
    const bool usesinglecol = !shouldUseTexture();
    const Color col = usesinglecol  ? nontexturecol_ : Color::White();
    material_->setColor( col );
    if ( intersectionlinematerial_ )
	intersectionlinematerial_->setColor( nontexturecol_ );

    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	sections_[idx]->useChannel( !usesinglecol );
	sections_[idx]->setWireframeColor( col );
    }
}


void HorizonDisplay::setRandomPosData( int channel, const DataPointSet* data,
				       TaskRunner* tr )
{
    if ( channel<0 || channel>=nrAttribs() )
       return;

    if ( !data || !data->size() )
    {
	validtexture_ = false;
	usestexture_ = false;
	updateSingleColor();
	return;
    }

    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->setTextureData( channel, data, sids_[idx], tr );

    //We should really scale here, and then update sections. This
    //works for single sections though.
    if ( sections_.size() && sections_[0]->getColTabMapperSetup( channel ) )
    {
	coltabmappersetups_[channel] =
	    *sections_[0]->getColTabMapperSetup( channel );
	coltabmappersetups_[channel].triggerRangeChange();
    }

    validtexture_ = true;
    usestexture_ = true;
    updateSingleColor();
}


bool HorizonDisplay::hasStoredAttrib( int channel ) const
{
    const char* userref = as_[channel]->userRef();
    return as_[channel]->id()==Attrib::SelSpec::cOtherAttrib() &&
	   userref && *userref;
}


bool HorizonDisplay::hasDepth( int channel ) const
{ return as_[channel]->id()==Attrib::SelSpec::cNoAttrib(); }


Coord3 HorizonDisplay::getTranslation() const
{
    if ( !translation_ ) return Coord3(0,0,0);

    const Coord3 current = translation_->getTranslation();

    Coord3 origin( 0, 0, 0 );
    Coord3 shift( current );
    shift  *= -1;

    mVisTrans::transformBack( transformation_, origin );
    mVisTrans::transformBack( transformation_, shift );

    const Coord3 translation = origin - shift;

    return translation;
}


void HorizonDisplay::setTranslation( const Coord3& nt )
{
    if ( !translation_ )
    {
	translation_ = visBase::Transformation::create();
	translation_->ref();
	addChild( translation_->osgNode() );

	for ( int idx = 0; idx< sections_.size(); idx++ )
	{
	    removeChild( sections_[idx]->osgNode() );
	    translation_->addObject( sections_[idx] );
	}
    }

    Coord3 origin( 0, 0, 0 );
    Coord3 aftershift( nt );
    aftershift.z *= -1;

    mVisTrans::transform( transformation_, origin );
    mVisTrans::transform( transformation_, aftershift );

    const Coord3 shift = origin - aftershift;

    translation_->setTranslation( shift );

    setOnlyAtSectionsDisplay( displayonlyatsections_ );		/* retrigger */
}


void HorizonDisplay::setEdgeLineRadius(float nr)
{
    edgelineradius_ = nr;
    for ( int idx=0; idx<edgelinedisplays_.size(); idx++ )
	edgelinedisplays_[idx]->setRadius(nr);
}


float HorizonDisplay::getEdgeLineRadius() const
{ return edgelineradius_; }


void HorizonDisplay::removeSectionDisplay( const EM::SectionID& sid )
{
    const int idx = sids_.indexOf( sid );
    if ( idx<0 ) return;

    removeChild( sections_[idx]->osgNode() );
    sections_.removeSingle( idx )->unRef();
    sids_.removeSingle( idx );
};


bool HorizonDisplay::addSection( const EM::SectionID& sid, TaskRunner* tr )
{
    RefMan<visBase::HorizonSection> surf = visBase::HorizonSection::create();
    surf->ref();
    surf->setDisplayTransformation( transformation_ );
    surf->setZAxisTransform( zaxistransform_, tr );
    if ( scene_ ) surf->setRightHandSystem( scene_->isRightHandSystem() );

    while ( surf->nrChannels()<nrAttribs() )
	surf->addChannel();

    for ( int idx=0; idx<nrAttribs(); idx++ )
    {
	surf->setColTabMapperSetup( idx, coltabmappersetups_[idx], 0 );
	surf->setColTabSequence( idx, coltabsequences_[idx] );
	surf->getChannels2RGBA()->setEnabled( idx, enabled_[idx] );
    }

    if ( !sections_.size() && channel2rgba_ )
    {
	surf->setChannels2RGBA( channel2rgba_ );
	EMObjectDisplay::setChannels2RGBA( 0 );
    }

    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    mDynamicCastGet( EM::Horizon3D*, horizon, emobject_ );
    surf->setSurface( horizon->geometry().sectionGeometry(sid), true, tr );

    surf->getChannels2RGBA()->allowShading( allowshading_ );
    surf->getChannels()->enableTextureInterpolation( enabletextureinterp_ );
    surf->setResolution( resolution_-1, tr );

    surf->setMaterial( 0 );

    if ( translation_ )
	translation_->addObject( surf );
    else
	addChild( surf->osgNode() );

    surf->turnOn( !displayonlyatsections_ );

    sections_ += surf;
    sids_ += sid;
    hasmoved.trigger();

    displaysSurfaceGrid( displaysurfacegrid_ );

    return addEdgeLineDisplay( sid );

}


void HorizonDisplay::enableTextureInterpolation( bool yn )
{
    if ( enabletextureinterp_==yn )
	return;

    enabletextureinterp_ = yn;
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	if ( !sections_[idx] || !sections_[idx]->getChannels() )
	    continue;

	sections_[idx]->getChannels()->enableTextureInterpolation( yn );

    }
}


void HorizonDisplay::setOnlyAtSectionsDisplay( bool yn )
{
    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->turnOn(!yn);

    EMObjectDisplay::setOnlyAtSectionsDisplay( yn );

    for ( int idx=0; yn && idx<intersectionlines_.size(); idx++ )
	intersectionlines_[idx]->setMaterial( intersectionlinematerial_ );

    for ( int idx=0; yn && idx<intersectionpointsets_.size(); idx++ )
    {
	visBase::DataObjectGroup* pointgroup = intersectionpointsets_[idx];
	mDynamicCastGet( visBase::Material*, material,
		   pointgroup->getObject( 0 ) );
	if ( material )
	    pointgroup->removeObject( 0 );
	//pointgroup->insertObject( 0, intersectionlinematerial_ );
    }
}


visBase::Material* HorizonDisplay::getMaterial()
{
    return material_;
}


void HorizonDisplay::setIntersectLineMaterial( visBase::Material* nm )
{
    if ( intersectionlinematerial_ )
	intersectionlinematerial_->unRef();

    intersectionlinematerial_ = nm;
    if ( intersectionlinematerial_ )
	intersectionlinematerial_->ref();

}


void HorizonDisplay::emChangeCB( CallBacker* cb )
{
    EMObjectDisplay::emChangeCB( cb );
    mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);
    if ( cbdata.event==EM::EMObjectCallbackData::PositionChange )
    {
	validtexture_ = false;
	const EM::SectionID sid = cbdata.pid0.sectionID();
	const int idx = sids_.indexOf( sid );
	if ( idx>=0 && idx<sections_.size() )
	    sections_[idx]->inValidateCache(-1);
    }
    else if ( cbdata.event==EM::EMObjectCallbackData::PrefColorChange )
	nontexturecol_ = emobject_->preferredColor();

    updateSingleColor();

}


void HorizonDisplay::emEdgeLineChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpack(EM::SectionID,section,cb);

    mDynamicCastGet(const EM::Horizon3D*,horizon3d,emobject_);
    if ( !horizon3d ) return;

    if ( horizon3d->edgelinesets.getEdgeLineSet( section, false ) )
	 addEdgeLineDisplay(section);
    else
    {
	for ( int idx=0; idx<edgelinedisplays_.size(); idx++ )
	{
	    if (edgelinedisplays_[idx]->getEdgeLineSet()->getSection()==section)
	    {
		EdgeLineSetDisplay* elsd = edgelinedisplays_[idx--];
		edgelinedisplays_ -= elsd;
		elsd->rightClicked()->remove(
			 mCB( this, HorizonDisplay, edgeLineRightClickCB ));
		removeChild( elsd->osgNode() );
		elsd->unRef();
		break;
	    }
	}
    }
}


int HorizonDisplay::nrResolutions() const
{
    return sections_.size() ? sections_[0]->nrResolutions()+1 : 1;
}


BufferString HorizonDisplay::getResolutionName( int res ) const
{
    BufferString str;
    if ( !res ) str = "Automatic";
    else
    {
	res--;
	int val = 1;
	for ( int idx=0; idx<res; idx++ )
	    val *= 2;

	if ( val==2 )		str = "Half";
	else if ( val==1 )	str = "Full";
	else			{ str = "1 / "; str += val; }
    }

    return str;
}


int HorizonDisplay::getResolution() const
{
    return sections_.size() ? sections_[0]->currentResolution()+1 : 0;
}


bool HorizonDisplay::displaysSurfaceGrid() const
{
    if ( sections_.size() )
       return  sections_[0]->isWireframeDisplayed();
    return false;
}

void HorizonDisplay::setResolution( int res, TaskRunner* tr )
{
    resolution_ = (char)res;
    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->setResolution( resolution_-1, tr );
}


void HorizonDisplay::displaysSurfaceGrid( bool yn )
{
    displaysurfacegrid_ = yn;
    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->enableGeometryTypeDisplay( WireFrame, yn );
}

const ColTab::Sequence* HorizonDisplay::getColTabSequence( int channel ) const
{
    if ( channel<0 || channel>=nrAttribs() )
       return 0;

    return sections_.size()
	? sections_[0]->getColTabSequence( channel )
	: &coltabsequences_[channel];
}


bool HorizonDisplay::canSetColTabSequence() const
{
    if ( !usesTexture() || !sections_.size() ||
	 !sections_[0]->getChannels2RGBA() )
	return false;

    return sections_[0]->getChannels2RGBA()->canSetSequence();
}


void HorizonDisplay::setColTabSequence( int chan, const ColTab::Sequence& seq,
					TaskRunner* )
{
    if ( chan<0 || chan>=nrAttribs() )
       return;

    coltabsequences_[chan] = seq;

    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->setColTabSequence( chan, seq );
}


void HorizonDisplay::setColTabMapperSetup( int channel,
			   const ColTab::MapperSetup& ms, TaskRunner* tr )
{
    if ( channel<0 || channel>=nrAttribs() )
       return;

    if ( coltabmappersetups_.validIdx(channel) )
	coltabmappersetups_[channel] = ms;

    for ( int idx=0; idx<sections_.size(); idx++ )
	sections_[idx]->setColTabMapperSetup( channel, ms, tr );

    //We should really scale here, and then update sections. This
    //works for single sections though.
    if ( sections_.size() && sections_[0]->getColTabMapperSetup( channel ) )
    {
	coltabmappersetups_[channel] =
	    *sections_[0]->getColTabMapperSetup( channel );
    }
}


const ColTab::MapperSetup* HorizonDisplay::getColTabMapperSetup( int ch,
								 int ) const
{
    if ( ch<0 || ch>=nrAttribs() )
       return 0;

    return &coltabmappersetups_[ch];
}


const TypeSet<float>* HorizonDisplay::getHistogram( int attrib ) const
{
    if ( !sections_.size() )
	return 0;

    return sections_[0]->getHistogram( attrib );
}


float HorizonDisplay::calcDist( const Coord3& pickpos ) const
{
    if ( !emobject_ ) return mUdf(float);

    const mVisTrans* utm2display = scene_->getUTM2DisplayTransform();
    Coord3 xytpos;
    utm2display->transformBack( pickpos, xytpos );
    mDynamicCastGet(const EM::Horizon3D*,hor,emobject_)
    if ( hor )
    {
	const BinID bid = SI().transform( xytpos );
	const EM::SubID bidid = bid.toInt64();
	TypeSet<Coord3> positions;
	for ( int idx=sids_.size()-1; idx>=0; idx-- )
	{
	    if ( !emobject_->isDefined( sids_[idx], bidid ) )
		continue;

	    positions += emobject_->getPos( sids_[idx], bidid );
	}

	float mindist = mUdf(float);
	for ( int idx=0; idx<positions.size(); idx++ )
	{
	    const float zfactor = scene_
		? scene_->getZScale()
		: s3dgeom_->zScale();
	    const Coord3& pos = positions[idx] + getTranslation()/zfactor;
	    const float dist = (float) fabs(xytpos.z-pos.z);
	    if ( dist < mindist ) mindist = dist;
	}

	return mindist;
    }

    return mUdf(float);
}


float HorizonDisplay::maxDist() const
{
    return s3dgeom_->zStep();
}


visBase::HorizonSection* HorizonDisplay::getHorizonSection(
	const EM::SectionID& sid )
{
    for ( int idx=0; idx<sids_.size(); idx++ )
    {
	if ( sids_[idx]==sid )
	    return sections_[idx];
    }

    return 0;
}


EM::SectionID HorizonDisplay::getSectionID( int visid ) const
{
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	if ( sections_[idx]->id()==visid )
		return sids_[idx];
    }

    return -1;
}


void HorizonDisplay::getMousePosInfo( const visBase::EventInfo& eventinfo,
				       Coord3& pos, BufferString& val,
				       BufferString& info ) const
{
    EMObjectDisplay::getMousePosInfo( eventinfo, pos, val, info );
    if ( !emobject_ || !usesTexture() ) return;

    const EM::SectionID sid =
	EMObjectDisplay::getSectionID(&eventinfo.pickedobjids);

    const int sectionidx = sids_.indexOf( sid );
    if ( sectionidx<0 || sectionidx>=sections_.size() )
	return;

    const BinID bid( SI().transform(pos) );

    for ( int idx=as_.size()-1; idx>=0; idx-- )
    {
	if ( as_[idx]->id().isUnselInvalid() )
	    return;

	if ( !sections_[sectionidx]->getChannels2RGBA()->isEnabled(idx) ||
	      sections_[sectionidx]->getTransparency(idx)==255 )
	    continue;

	const BinIDValueSet* bidvalset = sections_[sectionidx]->getCache( idx );
	if ( !bidvalset || bidvalset->nrVals()<2 ) continue;

	const BinIDValueSet::SPos setpos = bidvalset->find( bid );
	if ( !setpos.isValid() )
	    continue;

	const float* vals = bidvalset->getVals( setpos );
	int curtexture = selectedTexture(idx);
	if ( curtexture>bidvalset->nrVals()-2 ) curtexture = 0;

	const float fval = vals[curtexture+2];
	bool islowest = true;
	for ( int idy=idx-1; idy>=0; idy-- )
	{
	    if ( !sections_[sectionidx]->getCache(idy) ||
		 !isAttribEnabled(idy) ||
		 sections_[sectionidx]->getTransparency(idy)==255 )
		continue;

		islowest = false;
		break;
	}

	if ( !islowest )
	{
	    const ColTab::Sequence* coltabseq = getColTabSequence( idx );
	    const Color col = coltabseq ? coltabseq->color(fval) : Color();
	    if ( (!coltabseq || col!=coltabseq->undefColor()) && col.t()==255 )
		continue;
	}

	if ( !mIsUdf(fval) )
	{
	    val = fval;
	    float attribshift = 0;
	    const TypeSet<float>& attribshifts = *shifts_[idx];
	    const int version = selectedTexture( idx );
	    if ( attribshifts.validIdx(version) )
	    {
		attribshift =
		  attribshifts[version] * s3dgeom_->zDomain().userFactor();
	    }

	    const float zshift =
	      (float) getTranslation().z*s3dgeom_->zDomain().userFactor();

	    const bool hasshift = !mIsZero(attribshift,0.1) ||
				  !mIsZero(zshift,0.1);
	    if ( as_.size() > 1 || hasshift )
	    {
		BufferString channelstr = "(";
		bool first = true;
		if ( as_.size()>1 )
		{
		    channelstr += as_[idx]->userRef();
		    first = false;
		}

		if ( hasshift )
		{
		    if ( !first )
			channelstr += ", ";
		    channelstr += "shift=";
		    channelstr += attribshift;
		}

		channelstr += ")";
		val.replaceAt( cValNameOffset(), channelstr );
	    }
	}

	return;
    }
}


#define mAddLinePrimitiveSet()\
{\
    if ( idxps.size()>=2 )\
    {\
	Geometry::IndexedPrimitiveSet* primitiveset =  \
	Geometry::IndexedPrimitiveSet::create( false );\
	primitiveset->ref();\
	primitiveset->append( idxps.arr(), idxps.size() );\
	line->addPrimitiveSet( primitiveset ); \
	primitiveset->unRef();\
    }\
}


#define mEndLine \
{ \
    if ( curline.size()==1 ) \
    { \
	visBase::MarkerSet* markerset = visBase::MarkerSet::create(); \
	MarkerStyle3D markerstyle;\
	markerstyle.size_ = ( int )mCast(float,lineStyle()->width_);\
	markerstyle.type_ = MarkerStyle3D::Sphere;\
	markerset->setMarkerStyle( markerstyle );\
	markerset->setDisplayTransformation(transformation_); \
	markerset->setMaterial( 0 ); \
	markerset->setMarkerHeightRatio( 1.0f );\
	markerset->addPos( curline[0] ); \
	points->addObject( markerset ); \
    } \
    else \
    { \
	TypeSet<int> idxps;\
	for ( int idx=0; idx<curline.size(); idx++ ) \
	{ \
	    idxps.add( cii++ );\
	    line->getCoordinates()->addPos(curline[idx]);\
	} \
	if ( curline.size() ) \
	mAddLinePrimitiveSet();\
    } \
    curline.erase(); \
}


void HorizonDisplay::traverseLine( bool oninline, const CubeSampling& cs,
	EM::SectionID sid, visBase::VertexShape* line, int& cii,
	visBase::DataObjectGroup* points ) const
{
    mDynamicCastGet( EM::Horizon3D*, hor, emobject_ );
    const Geometry::BinIDSurface* geom = hor->geometry().sectionGeometry( sid );
    const StepInterval<int> inlrg = geom->rowRange();
    const StepInterval<int> crlrg = geom->colRange();

    StepInterval<int> rg; int targetline; BinID startbid;
    int faststop, faststep, slowdim, fastdim;
    if ( oninline )
    {
	rg = inlrg; targetline = cs.hrg.start.inl();
	startbid = BinID( targetline, crlrg.start );
	faststop = crlrg.stop; faststep = crlrg.step;
	slowdim = 0; fastdim = 1;
    }
    else
    {
	rg = crlrg; targetline = cs.hrg.start.crl();
	startbid = BinID( inlrg.start, targetline );
	faststop = inlrg.stop; faststep = inlrg.step;
	slowdim = 1; fastdim = 0;
    }

    if ( !rg.includes(targetline,false) )
	return;

    const int rgindex = rg.getIndex(targetline);
    const int prevline = rg.atIndex(rgindex);
    const int nextline = prevline<targetline ? rg.atIndex(rgindex+1) : prevline;

    const TypeSet<EM::PosID>* seedposids = hor->getPosAttribList
						( EM::EMObject::sSeedNode() );
    TypeSet<Coord3> curline;
    for ( BinID bid=startbid; bid[fastdim]<=faststop; bid[fastdim]+=faststep )
    {
	if ( !cs.hrg.includes(bid) )
	{
	    mEndLine;
	    continue;
	}

	BinID prevbid( bid ); prevbid[slowdim] = prevline;
	BinID nextbid( bid ); nextbid[slowdim] = nextline;
	Coord3 prevpos( hor->getPos(sid,prevbid.toInt64()) );
	if ( zaxistransform_ )
	    prevpos.z = zaxistransform_->transform( prevpos );
	Coord3 pos = prevpos;
	if ( nextbid!=prevbid && prevpos.isDefined() )
	{
	    Coord3 nextpos = hor->getPos(sid,nextbid.toInt64());
	    if ( zaxistransform_ )
		nextpos.z = zaxistransform_->transform(nextpos);
	    if ( nextpos.isDefined() )
	    {
		const float frac = float( targetline - prevline ) / rg.step;
		pos = (1-frac)*prevpos + frac*nextpos;
	    }
	}

	if ( !pos.isDefined() || !cs.zrg.includes(pos.z,false) )
	{
	    if ( curline.size() == 1 )
	    {
		const BinID curlinebid = SI().transform( curline[0].coord() );
		bool hasseed = false;
		for ( int idx=0; seedposids && idx<seedposids->size(); idx++ )
		{
		    const BinID seedbid = BinID::fromInt64(
						(*seedposids)[idx].subID() );
		    if ( seedbid[fastdim] == curlinebid[fastdim] )
		    {
			hasseed = true;
			break;
		    }
		}

		if ( !hasseed )
		{
		    curline.erase();
		    continue;
		}
	    }

	    mEndLine;
	    continue;
	}

	curline += pos;
    }

    mEndLine;

}


void HorizonDisplay::drawHorizonOnRandomTrack( const TypeSet<Coord>& trclist,
	const StepInterval<float>& zrg,
	const EM::SectionID&  sid,
	visBase::VertexShape* line, int& cii,
	visBase::DataObjectGroup* points ) const
{
    mDynamicCastGet( EM::Horizon3D*, hor, emobject_ );

    const Geometry::BinIDSurface* geom = hor->geometry().sectionGeometry( sid );
    const StepInterval<int> inlrg = geom->rowRange();
    const StepInterval<int> crlrg = geom->colRange();

    int startidx;
    int stopidx = 0;
    int jumpstart = 0;

    TypeSet<Coord3> curline;
    TypeSet<int> idxps;
    while ( true )
    {
	startidx = stopidx;
	while ( startidx<trclist.size()-1 &&
		trclist[startidx]==trclist[startidx+1] ) startidx++;

	stopidx = startidx + 1;
	while ( stopidx<trclist.size()-1 &&
		trclist[stopidx]!=trclist[stopidx+1] ) stopidx++;

	if ( stopidx >= trclist.size() )
	    break;

	const Coord startcrd = SI().binID2Coord().transform(trclist[startidx]);
	const Coord stopcrd = SI().binID2Coord().transform(trclist[stopidx]);

	for ( int cidx=startidx+jumpstart; cidx<=stopidx; cidx++ )
	{
	    const int inlidx = inlrg.getIndex( trclist[cidx].x );
	    const int inl0 = inlrg.atIndex( inlidx );
	    const int inl1 = inl0<trclist[cidx].x ?
			     inlrg.atIndex( inlidx+1 ) : inl0;

	    const int crlidx = crlrg.getIndex( trclist[cidx].y );
	    const int crl0 = crlrg.atIndex( crlidx );
	    const int crl1 = crl0<trclist[cidx].y ?
			     crlrg.atIndex( crlidx+1 ) : crl0;

	    Coord3 p00 = hor->getPos( sid, BinID(inl0,crl0).toInt64() );
	    Coord3 p01 = hor->getPos( sid, BinID(inl0,crl1).toInt64() );
	    Coord3 p10 = hor->getPos( sid, BinID(inl1,crl0).toInt64() );
	    Coord3 p11 = hor->getPos( sid, BinID(inl1,crl1).toInt64() );

	    if ( zaxistransform_ )
	    {
		p00.z = zaxistransform_->transform( p00 );
		p01.z = zaxistransform_->transform( p01 );
		p10.z = zaxistransform_->transform( p10 );
		p11.z = zaxistransform_->transform( p11 );
	    }

	    if ( p00.isDefined() && p01.isDefined() &&
		 p10.isDefined() && p11.isDefined() )
	    {
		const float frac = float(cidx-startidx) / (stopidx-startidx);
		Coord3 pos = (1-frac) * Coord3(startcrd,0) +
				frac  * Coord3(stopcrd, 0);

		const float ifrac =
		    (float) (trclist[cidx].x - inl0) / inlrg.step;
		const float cfrac =
		    (float) (trclist[cidx].y - crl0) / crlrg.step;
		pos.z = (1-ifrac)*( (1-cfrac)*p00.z + cfrac*p01.z ) +
			   ifrac *( (1-cfrac)*p10.z + cfrac*p11.z );

		if ( zrg.includes(pos.z,true) )
		{
		    line->getCoordinates()->addPos(pos);
		    idxps.add( cii++ );
		    continue;
		}
	    }
	    mAddLinePrimitiveSet();
	}

	jumpstart = 1;
    }

    mAddLinePrimitiveSet();

}


static void drawHorizonOnZSlice( const CubeSampling& cs, float zshift,
			const EM::Horizon3D* hor, const EM::SectionID&  sid,
			const ZAxisTransform* zaxistransform,
			visBase::VertexShape* line, int& cii )
{
    const Geometry::BinIDSurface* geom = hor->geometry().sectionGeometry( sid );
    const Array2D<float>* field = geom->getArray();
    if ( !field ) return;

    if ( zaxistransform )
	field = hor->createArray2D( sid, zaxistransform );

    IsoContourTracer ictracer( *field );
    ictracer.setSampling( geom->rowRange(), geom->colRange() );
    ictracer.selectRectROI( cs.hrg.inlRange(), cs.hrg.crlRange() );
    ObjectSet<ODPolygon<float> > isocontours;
    ictracer.getContours( isocontours, cs.zrg.start-zshift, false );

    TypeSet<int> idxps;
    for ( int cidx=0; cidx<isocontours.size(); cidx++ )
    {
	const ODPolygon<float>& ic = *isocontours[cidx];
	for ( int vidx=0; vidx<ic.size(); vidx++ )
	{
	    const Geom::Point2D<float> vertex = ic.getVertex( vidx );
	    Coord vrtxcoord( vertex.x, vertex.y );
	    vrtxcoord = SI().binID2Coord().transform( vrtxcoord );
	    const Coord3 pos( vrtxcoord, cs.zrg.start-zshift );
	    line->getCoordinates()->addPos( pos );
	    idxps.add( cii++ );
	}

	if ( ic.isClosed() )
	    idxps.add( idxps[0] );

	mAddLinePrimitiveSet();
	idxps.erase();
    }

    if ( zaxistransform ) delete field;
    deepErase( isocontours );
}


void HorizonDisplay::updateIntersectionLines(
	    const ObjectSet<const SurveyObject>& objs, int whichobj )
{
    mDynamicCastGet(const EM::Horizon3D*,horizon,emobject_);
    if ( !horizon ) return;

    TypeSet<int> linestoupdate;
    BoolTypeSet lineshouldexist( intersectionlineids_.size(), false );

    if ( displayonlyatsections_ || displayintersectionlines_ )
    {
	for ( int idx=0; idx<objs.size(); idx++ )
	{
	    int objectid = -1;
	    mDynamicCastGet( const PlaneDataDisplay*, plane, objs[idx] );
	    if ( plane )
		objectid = plane->id();

	    mDynamicCastGet( const MPEDisplay*, mped, objs[idx] );
	    if ( mped && mped->isDraggerShown() )
		objectid = mped->id();

	    mDynamicCastGet( const RandomTrackDisplay*, rtdisplay, objs[idx] );
	    if ( rtdisplay )
		objectid = rtdisplay->id();

	    mDynamicCastGet( const Seis2DDisplay*, seis2ddisplay, objs[idx] );
	    if ( seis2ddisplay )
		objectid = seis2ddisplay->id();

	    if ( objectid==-1 )
		continue;

	    const int idy = intersectionlineids_.indexOf(objectid);
	    if ( idy==-1 )
	    {
		linestoupdate += objectid;
		lineshouldexist += true;
	    }
	    else
	    {
		if ( ( whichobj==objectid || whichobj==id() ) &&
		    linestoupdate.indexOf(whichobj)==-1 )
		{
		    linestoupdate += objectid;
		}

		lineshouldexist[idy] = true;
	    }
	}
    }

    visBase::VisualWriteLockLocker writelock( *this );

    for ( int idx=0; idx<intersectionlineids_.size(); idx++ )
    {
	if ( !lineshouldexist[idx] )
	{
	    removeChild( intersectionlines_[idx]->osgNode() );
	    removeChild( intersectionpointsets_[idx]->osgNode() );

	    lineshouldexist.removeSingle(idx);
	    intersectionlines_.removeSingle(idx)->unRef();
	    intersectionpointsets_.removeSingle(idx)->unRef();
	    intersectionlineids_.removeSingle(idx);
	    if ( zaxistransform_ )
	    {
		zaxistransform_->removeVolumeOfInterest(
		    intersectionlinevoi_[idx] );
	    }

	    intersectionlinevoi_.removeSingle(idx);
	    idx--;
	}
    }

    for ( int idx=0; idx<linestoupdate.size(); idx++ )
    {
	CubeSampling cs(false);
	mDynamicCastGet( PlaneDataDisplay*, plane,
	    visBase::DM().getObject(linestoupdate[idx]) );
	if ( plane )
	    cs = plane->getCubeSampling(true,true,-1);

	mDynamicCastGet( const MPEDisplay*, mped,
	    visBase::DM().getObject(linestoupdate[idx]) );
	if ( mped )
	    mped->getPlanePosition(cs);

	TypeSet<Coord> trclist;
	mDynamicCastGet( const RandomTrackDisplay*, rtdisplay,
	    visBase::DM().getObject(linestoupdate[idx]) );
	if ( rtdisplay )
	{
	    cs.zrg.setFrom( rtdisplay->getDataTraceRange() );
	    cs.zrg.step = s3dgeom_->zStep();
	    TypeSet<BinID> tracebids;
	    rtdisplay->getDataTraceBids( tracebids );
	    for ( int bidx=0; bidx<tracebids.size(); bidx++ )
	    {
		cs.hrg.include( tracebids[bidx] );
		trclist += Coord( tracebids[bidx].inl(), tracebids[bidx].crl());
	    }
	}

	mDynamicCastGet( const Seis2DDisplay*, seis2ddisplay,
	    visBase::DM().getObject(linestoupdate[idx]) );
	if ( seis2ddisplay )
	{
	    cs.zrg.setFrom( seis2ddisplay->getZRange(false) );
	    cs.zrg.step = s3dgeom_->zStep();
	    const Interval<int>& trcnrrg = seis2ddisplay->getTraceNrRange();
	    for ( int trcnr=trcnrrg.start; trcnr<=trcnrrg.stop; trcnr++ )
	    {
		Coord crd = seis2ddisplay->getCoord( trcnr );
		crd = SI().binID2Coord().transformBackNoSnap( crd );
		if ( mIsUdf(crd.x) || mIsUdf(crd.y) )
		    continue;

		cs.hrg.include( BinID((int) floor(crd.x),(int) floor(crd.y)) );
		cs.hrg.include( BinID((int)  ceil(crd.x),(int)  ceil(crd.y)) );
		trclist += crd; trclist += crd;
	    }
	}

	int lineidx = intersectionlineids_.indexOf(linestoupdate[idx]);
	if ( lineidx==-1 )
	{
	    lineidx = intersectionlineids_.size();
	    intersectionlineids_ += linestoupdate[idx];
	    intersectionlinevoi_ += -2;

	    const bool do3d = lineStyle()->type_==LineStyle::Solid;

	    visBase::VertexShape* newline = do3d
		? (visBase::VertexShape*) visBase::PolyLine3D::create()
		: (visBase::VertexShape*) visBase::PolyLine::create();

	    newline->ref();

	    if ( do3d )
	    {
		((visBase::PolyLine3D* ) newline)->setLineStyle( *lineStyle() );
	    }

	    newline->setPrimitiveType( Geometry::PrimitiveSet::LineStrips );

	    newline->setDisplayTransformation(transformation_);
	    if ( intersectionlinematerial_ )
		newline->setMaterial( intersectionlinematerial_ );
	    intersectionlines_ += newline;
	    addChild( newline->osgNode() );

	    visBase::DataObjectGroup* pointgroup =
		visBase::DataObjectGroup::create();
	    pointgroup->setSeparate( false );

	    pointgroup->setDisplayTransformation(transformation_);

	    intersectionpointsets_ += pointgroup;
	    pointgroup->ref();
	    addChild( pointgroup->osgNode() );
	}

	if ( zaxistransform_ )
	{
	    if ( intersectionlinevoi_[lineidx]==-2 )
	    {
		intersectionlinevoi_[lineidx] =
		    zaxistransform_->addVolumeOfInterest(cs,true);
	    }
	    else
	    {
		zaxistransform_->setVolumeOfInterest(
		    intersectionlinevoi_[lineidx],cs,true);
	    }

	    if ( intersectionlinevoi_[lineidx]>=0 )
	    {
		zaxistransform_->loadDataIfMissing(
		    intersectionlinevoi_[lineidx] );
	    }
	}

	visBase::VertexShape* line = intersectionlines_[lineidx];
	visBase::DataObjectGroup* pointgroup = intersectionpointsets_[lineidx];

	line->getCoordinates()->setEmpty();
	line->removeAllPrimitiveSets();

	while ( pointgroup->size()>1 ) pointgroup->removeObject( 1 );
	int cii = 0;

	for ( int sectionidx=0; sectionidx<horizon->nrSections(); sectionidx++ )
	{
	    const EM::SectionID sid = horizon->sectionID(sectionidx);

	    if ( rtdisplay || seis2ddisplay )
	    {
		drawHorizonOnRandomTrack( trclist, cs.zrg, sid,
		    line, cii, pointgroup );
	    }
	    else if ( cs.hrg.start.inl()==cs.hrg.stop.inl() )
	    {
		traverseLine( true, cs, sid, line, cii, pointgroup );
	    }
	    else if ( cs.hrg.start.crl()==cs.hrg.stop.crl() )
	    {
		traverseLine( false, cs, sid, line, cii, pointgroup );
	    }
	    else
	    {
		drawHorizonOnZSlice( cs, (float) getTranslation().z, horizon,
		    sid, zaxistransform_, line, cii );
	    }
	}

    }

}


void HorizonDisplay::setLineStyle( const LineStyle& lst )
{
    if ( lst==*lineStyle() )
	return;

    const bool removelines =
	(lst.type_==LineStyle::Solid) != (lineStyle()->type_==LineStyle::Solid);

    EMObjectDisplay::setLineStyle( lst );

    const float radius = ((float) lineStyle()->width_) / 2;

    if ( removelines )
    {
	for ( int idx=0; idx<intersectionlines_.size(); idx++ )
	{
	    visBase::VertexShape* newline = lst.type_==LineStyle::Solid
		? (visBase::VertexShape*) visBase::PolyLine3D::create()
		: (visBase::VertexShape*) visBase::PolyLine::create();
	    newline->ref();
	    newline->setRightHandSystem( righthandsystem_ );
	    newline->setDisplayTransformation(transformation_);
	    newline->getCoordinates()->copyFrom(
		    *intersectionlines_[idx]->getCoordinates() );
	    if ( intersectionlinematerial_ )
		newline->setMaterial( intersectionlinematerial_ );

	    removeChild( intersectionlines_[idx]->osgNode() );
	    addChild( newline->osgNode() );

	    intersectionlines_.replace( idx, newline )->unRef();

	    if ( lst.type_==LineStyle::Solid )
	    {
		((visBase::PolyLine3D* ) newline )->setLineStyle( lst );
	    }
	}
    }
    else if ( lst.type_==LineStyle::Solid )
    {
	for ( int idx=0; idx<intersectionlines_.size(); idx++ )
	{
	   LineStyle lnstyle( lst );
	   lnstyle.width_ /=2;

	    visBase::PolyLine3D* pl =
		(visBase::PolyLine3D*) intersectionlines_[idx];
	    pl->setLineStyle( *lineStyle() );
	}
    }

    for ( int idx=0; idx<intersectionpointsets_.size(); idx++ )
    {
	visBase::DataObjectGroup* pointgroup = intersectionpointsets_[idx];
	for ( int idy=1; idy<pointgroup->size(); idy++ )
	{
	    mDynamicCastGet(visBase::MarkerSet*,markerset,
		            pointgroup->getObject(idx));
	    if ( markerset )
		markerset->setScreenSize( radius*2 );
	}
    }
}


void HorizonDisplay::updateSectionSeeds(
	    const ObjectSet<const SurveyObject>& objs, int movedobj )
{
    bool refresh = movedobj==-1 || movedobj==id();
    TypeSet<int> planelist;

    for ( int idx=0; idx<objs.size(); idx++ )
    {
	mDynamicCastGet(const PlaneDataDisplay*,plane,objs[idx]);
	if ( plane && plane->getOrientation()!=OD::ZSlice )
	{
	    planelist += idx;
	    if ( movedobj==plane->id() )
		refresh = true;
	}

	mDynamicCastGet( const MPEDisplay*, mped, objs[idx] );
	if ( mped && mped->isDraggerShown() )
	{
	    CubeSampling cs;
	    if ( mped->getPlanePosition(cs) && cs.nrZ()!=1 )
	    {
		planelist += idx;
		if ( movedobj==mped->id() )
		    refresh = true;
	    }
	}
    }

    if ( !refresh ) return;

    for ( int idx=0; idx<posattribmarkers_.size(); idx++ )
    {
	visBase::MarkerSet* markerset = posattribmarkers_[idx];
	for ( int idy=0; idy<markerset->getCoordinates()->size(); idy++ )
	{
	    markerset->turnMarkerOn( idy,!displayonlyatsections_ );
	    const visBase::Coordinates* markercoords =
					markerset->getCoordinates();
	    if ( markercoords->size() )
	    {
		Coord3 markerpos = markercoords->getPos( idy );
		if ( transformation_ )
		     mVisTrans::transform( transformation_,  markerpos );

		if ( zaxistransform_ )
		{
		    markerset->turnMarkerOn( idy,false );
		    continue;
		}

		for ( int idz=0; idz<planelist.size(); idz++ )
		{
		    const float dist =
			objs[planelist[idz]]->calcDist( markerpos );
		    if ( dist < objs[planelist[idz]]->maxDist() )
		    {
			markerset->turnMarkerOn( idy,true );
			break;
		    }
		}
	    }
	}
    }
}

void HorizonDisplay::doOtherObjectsMoved(
	            const ObjectSet<const SurveyObject>& objs, int whichobj )
{
    otherObjectsMoved( objs, whichobj );
}


void HorizonDisplay::otherObjectsMoved(
	    const ObjectSet<const SurveyObject>& objs, int whichobj )
{
    updateIntersectionLines( objs, whichobj );
    updateSectionSeeds( objs, whichobj );
}


void HorizonDisplay::fillPar( IOPar& par ) const
{
    EMObjectDisplay::fillPar( par );

    if ( emobject_ && !emobject_->isFullyLoaded() )
    {
	par.set( sKeyRowRange(), geometryRowRange() );
	par.set( sKeyColRange(), geometryColRange() );
    }

    par.setYN( sKeyTexture(), usesTexture() );
    par.set( sKeyShift(), getTranslation().z );
    par.set( sKeyResolution(), getResolution() );
    par.set( sKeySurfaceGrid(), displaysSurfaceGrid() );

    /* TODO: Implement something here
    const visBase::TextureChannel2RGBA* tc2rgba = getChannels2RGBA();
    mDynamicCastGet(const visBase::ColTabTextureChannel2RGBA*,cttc2rgba,tc2rgba)
    if ( tc2rgba && !cttc2rgba )
    {
	const int ctid = tc2rgba->id();
	par.set( MultiTextureSurveyObject::sKeyTC2RGBA(), ctid );
	saveids += ctid;
    }
     */

    for ( int channel=as_.size()-1; channel>=0; channel-- )
    {
	IOPar channelpar;
	as_[channel]->fillPar( channelpar );

	getColTabMapperSetup(channel)->fillPar( channelpar );
	if ( getColTabSequence(channel) )
	    getColTabSequence(channel)->fillPar( channelpar );

	channelpar.setYN( sKeyIsOn(), isAttribEnabled(channel) );

	BufferString key = sKeyAttribs();
	key += channel;
	par.mergeComp( channelpar, key );
    }

    par.set( sKeyNrAttribs(), as_.size() );
}


bool HorizonDisplay::usePar( const IOPar& par )
{
    if ( !EMObjectDisplay::usePar( par ) )
	 return false;

    if ( scene_ )
	setDisplayTransformation( scene_->getUTM2DisplayTransform() );

    if ( !par.get(sKeyEarthModelID(),parmid_) )
	return false;

    par.get( sKeyRowRange(), parrowrg_ );
    par.get( sKeyColRange(), parcolrg_ );

    if ( !par.getYN(sKeyTexture(),usestexture_) )
	usestexture_ = true;

    int resolution = 0;
    par.get( sKeyResolution(), resolution );
    setResolution( resolution, 0 );

    Coord3 shift( 0, 0, 0 );
    par.get( sKeyShift(), shift.z );
    setTranslation( shift );

    bool displaysurfacegrid =  false;
    par.getYN( sKeySurfaceGrid(), displaysurfacegrid );
    displaysSurfaceGrid( displaysurfacegrid );

    int intersectlinematid;
    if ( par.get(sKeyIntersectLineMaterialID(),intersectlinematid) )
    {
	if ( intersectlinematid==-1 )
	    setIntersectLineMaterial( 0 );
	else
	{
	    DataObject* mat = visBase::DM().getObject( intersectlinematid );
	    if ( !mat ) return 0;
	    if ( typeid(*mat) != typeid(visBase::Material) ) return -1;

	    setIntersectLineMaterial( (visBase::Material*)mat );
	}
    }
    else
	setIntersectLineMaterial( 0 );

    return true;
}


const ObjectSet<visBase::VertexShape>&
HorizonDisplay::getIntersectionLines() const
{ return intersectionlines_; }


bool HorizonDisplay::setDataPackID( int channel, DataPack::ID dpid,
				    TaskRunner* tr)
{
    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );
    const DataPack* datapack = dpman.obtain( dpid );
    if ( !datapack ) return false;

    DataPack::ID oldid = datapackids_[channel];
    datapackids_[channel] = dpid;
    dpman.release( oldid );
    return true;
}


DataPack::ID HorizonDisplay::getDataPackID( int channel ) const
{
    return datapackids_[channel];
}


DataPack::ID HorizonDisplay::getDisplayedDataPackID( int channel ) const
{
    return getDataPackID( channel );
}


const visBase::HorizonSection* HorizonDisplay::getSection( int horsecid ) const
{
    return sections_.validIdx( horsecid ) ? sections_[horsecid] : 0;
}

HorizonDisplay* HorizonDisplay::getHorizonDisplay( const MultiID& mid )
{
    TypeSet<int> ids;
    visBase::DM().getIDs( typeid(visSurvey::HorizonDisplay), ids );

    for ( int idx=0; idx<ids.size(); idx++ )
    {
	DataObject* dataobj = visBase::DM().getObject( ids[idx] );
	mDynamicCastGet( HorizonDisplay*, hordisp, dataobj );
	if ( hordisp && mid==hordisp->getMultiID() )
	    return hordisp;
    }
    return 0;
}


void HorizonDisplay::setPixelDensity( float dpi )
{
    EMObjectDisplay::setPixelDensity( dpi );

    for ( int idx=0; idx<intersectionlines_.size(); idx++ )
	intersectionlines_[idx]->setPixelDensity( dpi );

    for ( int idx=0; idx<intersectionpointsets_.size(); idx++ )
	intersectionpointsets_[idx]->setPixelDensity( dpi );
}


}; // namespace visSurvey
