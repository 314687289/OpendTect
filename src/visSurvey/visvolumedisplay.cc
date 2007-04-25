/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          August 2002
 RCS:           $Id: visvolumedisplay.cc,v 1.57 2007-04-25 16:22:33 cvskris Exp $
________________________________________________________________________

-*/


#include "visvolumedisplay.h"

#include "attribdatapack.h"
#include "isosurface.h"
#include "visboxdragger.h"
#include "viscolortab.h"
#include "visisosurface.h"
#include "visvolorthoslice.h"
#include "visvolrenscalarfield.h"
#include "visvolren.h"
#include "vistransform.h"

#include "cubesampling.h"
#include "attribsel.h"
#include "attribdatacubes.h"
#include "arrayndimpl.h"
#include "survinfo.h"
#include "visselman.h"
#include "visdataman.h"
#include "sorting.h"
#include "iopar.h"
#include "vismaterial.h"
#include "colortab.h"
#include "zaxistransform.h"

mCreateFactoryEntry( visSurvey::VolumeDisplay );

visBase::FactoryEntry visSurvey::VolumeDisplay::oldnameentry(
			(visBase::FactPtr) visSurvey::VolumeDisplay::create,
			"VolumeRender::VolumeDisplay");

namespace visSurvey {

const char* VolumeDisplay::volumestr = "Cube ID";
const char* VolumeDisplay::volrenstr = "Volren";
const char* VolumeDisplay::inlinestr = "Inline";
const char* VolumeDisplay::crosslinestr = "Crossline";
const char* VolumeDisplay::timestr = "Time";

const char* VolumeDisplay::nrslicesstr = "Nr of slices";
const char* VolumeDisplay::slicestr = "SliceID ";
const char* VolumeDisplay::texturestr = "TextureID";



VolumeDisplay::VolumeDisplay()
    : VisualObjectImpl(true)
    , boxdragger_(visBase::BoxDragger::create())
    , scalarfield_(visBase::VolumeRenderScalarField::create())
    , volren_(0)
    , as_(*new Attrib::SelSpec)
    , cache_(0)
    , cacheid_(-1)
    , slicemoving(this)
    , voltrans_( visBase::Transformation::create() )
{
    boxdragger_->ref();
    addChild( boxdragger_->getInventorNode() );
    boxdragger_->finished.notify( mCB(this,VolumeDisplay,manipMotionFinishCB) );
    getMaterial()->setColor( Color::White );
    getMaterial()->setAmbience( 0.8 );
    getMaterial()->setDiffIntensity( 0.8 );
    voltrans_->ref();
    addChild( voltrans_->getInventorNode() );
    voltrans_->setRotation( Coord3( 0, 1, 0 ), M_PI_2 );
    scalarfield_->ref();
    addChild( scalarfield_->getInventorNode() );

    CubeSampling cs(false); CubeSampling sics = SI().sampling(true);
    cs.hrg.start.inl = (5*sics.hrg.start.inl+3*sics.hrg.stop.inl)/8;
    cs.hrg.start.crl = (5*sics.hrg.start.crl+3*sics.hrg.stop.crl)/8;
    cs.hrg.stop.inl = (3*sics.hrg.start.inl+5*sics.hrg.stop.inl)/8;
    cs.hrg.stop.crl = (3*sics.hrg.start.crl+5*sics.hrg.stop.crl)/8;
    cs.zrg.start = ( 5*sics.zrg.start + 3*sics.zrg.stop ) / 8;
    cs.zrg.stop = ( 3*sics.zrg.start + 5*sics.zrg.stop ) / 8;
    SI().snap( cs.hrg.start, BinID(0,0) );
    SI().snap( cs.hrg.stop, BinID(0,0) );
    float z0 = SI().zRange(true).snap( cs.zrg.start ); cs.zrg.start = z0;
    float z1 = SI().zRange(true).snap( cs.zrg.stop ); cs.zrg.stop = z1;
    
    setCubeSampling( cs );
    addSlice( cInLine() ); addSlice( cCrossLine() ); addSlice( cTimeSlice() );
    showVolRen( true ); showVolRen( false );

    setColorTab( getColorTab() );
    showManipulator( true );
    scalarfield_->turnOn( true );
}


VolumeDisplay::~VolumeDisplay()
{
    delete &as_;
    DPM( DataPackMgr::CubeID ).release( cacheid_ );

    if ( cache_ ) cache_->unRef();

    TypeSet<int> children;
    getChildren( children );
    for ( int idx=0; idx<children.size(); idx++ )
	removeChild( children[idx] );

    scalarfield_->unRef();

    boxdragger_->finished.remove( mCB(this,VolumeDisplay,manipMotionFinishCB) );
    boxdragger_->unRef();

    voltrans_->unRef();
}


void VolumeDisplay::setUpConnections()
{
    if ( !scene_ || !scene_->getDataTransform() ) return;

    ZAxisTransform* datatransform = scene_->getDataTransform();
    Interval<float> zrg = datatransform->getZInterval( false );
    CubeSampling cs = getCubeSampling( 0 );
    cs.zrg.start = zrg.start;
    cs.zrg.stop = zrg.stop;
    setCubeSampling( cs );
}


void VolumeDisplay::getChildren( TypeSet<int>&res ) const
{
    res.erase();
    for ( int idx=0; idx<slices_.size(); idx++ )
	res += slices_[idx]->id();
    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
	res += isosurfaces_[idx]->id();
    if ( volren_ ) res += volren_->id();
}


void VolumeDisplay::showManipulator( bool yn )
{ boxdragger_->turnOn( yn ); }


bool VolumeDisplay::isManipulatorShown() const
{ return boxdragger_->isOn(); }


bool VolumeDisplay::isManipulated() const
{
    return getCubeSampling(true,0) != getCubeSampling(false,0);
}


bool VolumeDisplay::canResetManipulation() const
{ return true; }


void VolumeDisplay::resetManipulation()
{
    const Coord3 center = voltrans_->getTranslation();
    const Coord3 width = voltrans_->getScale();
    boxdragger_->setCenter( center );
    boxdragger_->setWidth( Coord3(width.z, width.y, -width.x) );
}


void VolumeDisplay::acceptManipulation()
{
    setCubeSampling( getCubeSampling(true,0) );
}


int VolumeDisplay::addSlice( int dim )
{
    visBase::OrthogonalSlice* slice = visBase::OrthogonalSlice::create();
    slice->ref();
    slice->setMaterial(0);
    slice->setDim(dim);
    slice->motion.notify( mCB(this,VolumeDisplay,sliceMoving) );
    slices_ += slice;

    slice->setName( !dim ? timestr : (dim==1 ? crosslinestr : inlinestr) );

    addChild( slice->getInventorNode() );
    const CubeSampling cs = getCubeSampling( 0 );
    const Interval<float> defintv(-0.5,0.5);
    slice->setSpaceLimits( defintv, defintv, defintv );
    if ( cache_ )
    {
	const Array3D<float>& arr = cache_->getCube(0);
	slice->setVolumeDataSize( arr.info().getSize(2),
				    arr.info().getSize(1),
				    arr.info().getSize(0) );
    }

    return slice->id();
}


void VolumeDisplay::removeChild( int displayid )
{
    if ( volren_ && displayid==volren_->id() )
    {
	VisualObjectImpl::removeChild( volren_->getInventorNode() );
	volren_->unRef();
	volren_ = 0;
	return;
    }

    for ( int idx=0; idx<slices_.size(); idx++ )
    {
	if ( slices_[idx]->id()==displayid )
	{
	    VisualObjectImpl::removeChild( slices_[idx]->getInventorNode() );
	    slices_[idx]->motion.remove( mCB(this,VolumeDisplay,sliceMoving) );
	    slices_[idx]->unRef();
	    slices_.removeFast(idx);
	    return;
	}
    }

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
    {
	if ( isosurfaces_[idx]->id()==displayid )
	{
	    VisualObjectImpl::removeChild(isosurfaces_[idx]->getInventorNode());
	    isosurfaces_[idx]->unRef();
	    isosurfaces_.removeFast(idx);
	    return;
	}
    }
}


void VolumeDisplay::showVolRen( bool yn )
{
    if ( yn && !volren_ )
    {
	volren_ = visBase::VolrenDisplay::create();
	volren_->ref();
	volren_->setMaterial(0);
	addChild( volren_->getInventorNode() );
	volren_->setName( volrenstr );
    }

    if ( volren_ ) volren_->turnOn( yn );
}


bool VolumeDisplay::isVolRenShown() const
{ return volren_ && volren_->isOn(); }


int VolumeDisplay::addIsoSurface()
{
    visBase::IsoSurface* isosurface = visBase::IsoSurface::create();
    isosurface->ref();
    RefMan<IsoSurface> surface = new IsoSurface();
    isosurface->setSurface( *surface );
    if ( cache_ )
    {
	surface->setAxisScales( cache_->inlsampling, cache_->crlsampling,
				SamplingData<float>( cache_->z0*cache_->zstep,
				    		     cache_->zstep ) );
	surface->setVolumeData( &cache_->getCube(0),
		scalarfield_->getColorTab().getInterval().center() );
	isosurface->touch();
    }

    isosurface->setName( "Iso surface" );

    insertChild( 0, isosurface->getInventorNode() );
    isosurfaces_ += isosurface;
    return isosurface->id();
}


int VolumeDisplay::volRenID() const
{ return volren_ ? volren_->id() : -1; }

    
void VolumeDisplay::setCubeSampling( const CubeSampling& cs )
{
    const Interval<float> xintv( cs.hrg.start.inl, cs.hrg.stop.inl );
    const Interval<float> yintv( cs.hrg.start.crl, cs.hrg.stop.crl );
    const Interval<float> zintv( cs.zrg.start, cs.zrg.stop );
    voltrans_->setTranslation( 
	    	Coord3(xintv.center(),yintv.center(),zintv.center()) );
    voltrans_->setScale( Coord3(-zintv.width(),yintv.width(),xintv.width()) );
    scalarfield_->setVolumeSize( Interval<float>(-0.5,0.5),
	    		    Interval<float>(-0.5,0.5),
			    Interval<float>(-0.5,0.5) );

    for ( int idx=0; idx<slices_.size(); idx++ )
	slices_[idx]->setSpaceLimits( Interval<float>(-0.5,0.5), 
				     Interval<float>(-0.5,0.5),
				     Interval<float>(-0.5,0.5) );

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
    {
	isosurfaces_[idx]->getSurface()->setVolumeData( 0 );
	isosurfaces_[idx]->touch();
    }

    scalarfield_->turnOn( false );

    resetManipulation();
}


float VolumeDisplay::getValue( const Coord3& pos_ ) const
{
    if ( !cache_ ) return mUdf(float);
    const BinIDValue bidv( SI().transform(pos_), pos_.z );
    float val;
    if ( !cache_->getValue(0,bidv,&val,false) )
	return mUdf(float);

    return val;
}


void VolumeDisplay::manipMotionFinishCB( CallBacker* )
{
    if ( scene_ && scene_->getDataTransform() )
	return;

    CubeSampling cs = getCubeSampling( true, 0 );
    SI().snap( cs.hrg.start, BinID(0,0) );
    SI().snap( cs.hrg.stop, BinID(0,0) );
    float z0 = SI().zRange(true).snap( cs.zrg.start ); cs.zrg.start = z0;
    float z1 = SI().zRange(true).snap( cs.zrg.stop ); cs.zrg.stop = z1;

    Interval<int> inlrg( cs.hrg.start.inl, cs.hrg.stop.inl );
    Interval<int> crlrg( cs.hrg.start.crl, cs.hrg.stop.crl );
    Interval<float> zrg( cs.zrg.start, cs.zrg.stop );
    SI().checkInlRange( inlrg, true );
    SI().checkCrlRange( crlrg, true );
    SI().checkZRange( zrg, true );
    if ( inlrg.start == inlrg.stop ||
	 crlrg.start == crlrg.stop ||
	 mIsEqual(zrg.start,zrg.stop,1e-8) )
    {
	resetManipulation();
	return;
    }
    else
    {
	cs.hrg.start.inl = inlrg.start; cs.hrg.stop.inl = inlrg.stop;
	cs.hrg.start.crl = crlrg.start; cs.hrg.stop.crl = crlrg.stop;
	cs.zrg.start = zrg.start; cs.zrg.stop = zrg.stop;
    }

    const Coord3 newwidth( cs.hrg.stop.inl - cs.hrg.start.inl,
			   cs.hrg.stop.crl - cs.hrg.start.crl,
			   cs.zrg.stop - cs.zrg.start );
    boxdragger_->setWidth( newwidth );
    const Coord3 newcenter( (cs.hrg.stop.inl + cs.hrg.start.inl) / 2,
			    (cs.hrg.stop.crl + cs.hrg.start.crl) / 2,
			    (cs.zrg.stop + cs.zrg.start) / 2 );
    boxdragger_->setCenter( newcenter );
}


BufferString VolumeDisplay::getManipulationString() const
{
    BufferString str = slicename; str += ": "; str += sliceposition;
    return str;
}


void VolumeDisplay::sliceMoving( CallBacker* cb )
{
    mDynamicCastGet( visBase::OrthogonalSlice*, slice, cb );
    if ( !slice ) return;

    slicename = slice->name();
    sliceposition = slicePosition( slice );
    slicemoving.trigger();
}


float VolumeDisplay::slicePosition( visBase::OrthogonalSlice* slice ) const
{
    if ( !slice ) return 0;
    const int dim = slice->getDim();
    float slicepositionf = slice->getPosition();
    slicepositionf *= -voltrans_->getScale()[dim];

    float pos;    
    if ( dim == 2 )
    {
	slicepositionf += voltrans_->getTranslation()[0];
	pos = SI().inlRange(true).snap(slicepositionf);
    }
    else if ( dim == 1 )
    {
	slicepositionf += voltrans_->getTranslation()[1];
	pos = SI().crlRange(true).snap(slicepositionf);
    }
    else
    {
	slicepositionf += voltrans_->getTranslation()[2];
	pos = mNINT(slicepositionf*1000);
    }

    return pos;
}


void VolumeDisplay::setColorTab( visBase::VisColorTab& ctab )
{
    scalarfield_->setColorTab( ctab );
}


int VolumeDisplay::getColTabID( int attrib ) const
{
    return attrib ? -1 : getColorTab().id();
}


const visBase::VisColorTab& VolumeDisplay::getColorTab() const
{ return scalarfield_->getColorTab(); }


visBase::VisColorTab& VolumeDisplay::getColorTab()
{ return scalarfield_->getColorTab(); }


const TypeSet<float>* VolumeDisplay::getHistogram( int attrib ) const
{ return attrib ? 0 : &scalarfield_->getHistogram(); }


visSurvey::SurveyObject::AttribFormat VolumeDisplay::getAttributeFormat() const
{ return visSurvey::SurveyObject::Cube; }


const Attrib::SelSpec* VolumeDisplay::getSelSpec( int attrib ) const
{ return attrib ? 0 : &as_; }


void VolumeDisplay::setSelSpec( int attrib, const Attrib::SelSpec& as )
{
    if ( attrib || as_==as ) return;
    as_ = as;
    if ( cache_ ) cache_->unRef();
    cache_ = 0;
    scalarfield_->turnOn( false );

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
    {
	isosurfaces_[idx]->getSurface()->setVolumeData( 0 );
	isosurfaces_[idx]->touch();
    }
}


CubeSampling VolumeDisplay::getCubeSampling( int attrib ) const
{ return getCubeSampling(true,attrib); }


bool VolumeDisplay::setDataPackID( int attrib, DataPack::ID dpid )
{
    if ( attrib>0 ) return false;

    DataPackMgr& dpman = DPM( DataPackMgr::CubeID );
    const DataPack* datapack = dpman.obtain( dpid );
    mDynamicCastGet(const Attrib::CubeDataPack*,cdp,datapack);
    const bool res = setDataVolume( attrib, cdp ? &cdp->cube() : 0 );
    if ( !res )
    {
	dpman.release( dpid );
	return false;
    }

    const DataPack::ID oldid = cacheid_;
    cacheid_ = dpid;

    dpman.release( oldid );
    return true;
}


bool VolumeDisplay::setDataVolume( int attrib,
				   const Attrib::DataCubes* attribdata )
{
    if ( attrib || !attribdata )
	return false;

    const Array3D<float>& arr = attribdata->getCube(0);
    scalarfield_->setScalarField( &arr );

    setCubeSampling( attribdata->cubeSampling() );

    for ( int idx=0; idx<slices_.size(); idx++ )
	slices_[idx]->setVolumeDataSize( arr.info().getSize(2),
					arr.info().getSize(1),
					arr.info().getSize(0) );

    scalarfield_->turnOn( true );

    if ( cache_ ) cache_->unRef();
    cache_ = attribdata;
    cache_->ref();

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
    {
	isosurfaces_[idx]->getSurface()->setAxisScales(
		cache_->inlsampling, cache_->crlsampling,
		SamplingData<float>( cache_->z0*cache_->zstep, cache_->zstep ));
	isosurfaces_[idx]->getSurface()->setVolumeData( &cache_->getCube(0) );
	isosurfaces_[idx]->touch();
    }

    return true;
}


const Attrib::DataCubes* VolumeDisplay::getCacheVolume( int attrib ) const
{ return attrib ? 0 : cache_; }


DataPack::ID VolumeDisplay::getDataPackID( int attrib ) const
{ return attrib ? -1 : cacheid_; }


void VolumeDisplay::getMousePosInfo( const visBase::EventInfo&,
				     const Coord3& pos, BufferString& val,
				     BufferString& info ) const
{
    info = "";
    val = "undef";
    if ( !isManipulatorShown() )
	val = getValue( pos );
}


CubeSampling VolumeDisplay::getCubeSampling( bool manippos, int attrib ) const
{
    CubeSampling res;
    if ( manippos )
    {
	Coord3 center_ = boxdragger_->center();
	Coord3 width_ = boxdragger_->width();

	res.hrg.start = BinID( mNINT( center_.x - width_.x / 2 ),
			      mNINT( center_.y - width_.y / 2 ) );

	res.hrg.stop = BinID( mNINT( center_.x + width_.x / 2 ),
			     mNINT( center_.y + width_.y / 2 ) );

	res.hrg.step = BinID( SI().inlStep(), SI().crlStep() );

	res.zrg.start = center_.z - width_.z / 2;
	res.zrg.stop = center_.z + width_.z / 2;
    }
    else
    {
	const Coord3 transl = voltrans_->getTranslation();
	Coord3 scale = voltrans_->getScale();
	double dummy = scale.x; scale.x=scale.z; scale.z = dummy;

	res.hrg.start = BinID( mNINT(transl.x+scale.x/2),
			       mNINT(transl.y+scale.y/2) );
	res.hrg.stop = BinID( mNINT(transl.x-scale.x/2),
			       mNINT(transl.y-scale.y/2) );
	res.hrg.step = BinID( SI().inlStep(), SI().crlStep() );

	res.zrg.start = transl.z+scale.z/2;
	res.zrg.stop = transl.z-scale.z/2;
    }
    

    return res;
}


bool VolumeDisplay::allowPicks() const
{
    return !isVolRenShown();
}


visSurvey::SurveyObject* VolumeDisplay::duplicate() const
{
    VolumeDisplay* vd = create();

//  const char* ctnm = getColorTab().colorSeq().colors().name();
//  vd->getColorTab().colorSeq().loadFromStorage( ctnm );

    TypeSet<int> children;
    vd->getChildren( children );
    for ( int idx=0; idx<children.size(); idx++ )
	vd->removeChild( children[idx] );

    for ( int idx=0; idx<slices_.size(); idx++ )
    {
	const int sliceid = vd->addSlice( slices_[idx]->getDim() );
	mDynamicCastGet(visBase::OrthogonalSlice*,slice,
			visBase::DM().getObject(sliceid));
	slice->setSliceNr( slices_[idx]->getSliceNr() );
    }

    vd->setCubeSampling( getCubeSampling(false,0) );

    return vd;
}


void VolumeDisplay::fillPar( IOPar& par, TypeSet<int>& saveids) const
{
    visBase::VisualObject::fillPar( par, saveids );
    const CubeSampling cs = getCubeSampling(false,0);
    cs.fillPar( par );

    if ( volren_ )
    {
	int volid = volren_->id();
	par.set( volumestr, volid );
	if ( saveids.indexOf( volid )==-1 ) saveids += volid;
    }

    const int textureid = scalarfield_->id();
    par.set( texturestr, textureid );
    if ( saveids.indexOf(textureid) == -1 ) saveids += textureid;

    const int nrslices = slices_.size();
    par.set( nrslicesstr, nrslices );
    for ( int idx=0; idx<nrslices; idx++ )
    {
	BufferString str( slicestr ); str += idx;
	const int sliceid = slices_[idx]->id();
	par.set( str, sliceid );
	if ( saveids.indexOf(sliceid) == -1 ) saveids += sliceid;
    }

    as_.fillPar(par);
}


int VolumeDisplay::usePar( const IOPar& par )
{
    int res =  visBase::VisualObject::usePar( par );
    if ( res!=1 ) return res;

    CubeSampling cs;
    if ( cs.usePar(par) )
	setCubeSampling( cs );

    if ( !as_.usePar(par) ) return -1;

    int textureid;
    if ( !par.get(texturestr,textureid) ) return false;

    visBase::DataObject* dataobj = visBase::DM().getObject( textureid );
    if ( !dataobj ) return 0;
    mDynamicCastGet(visBase::VolumeRenderScalarField*,vt,dataobj)
    if ( !vt ) return -1;
    if ( scalarfield_ )
    {
	if ( childIndex(scalarfield_->getInventorNode()) !=-1 )
	    VisualObjectImpl::removeChild(scalarfield_->getInventorNode());
	scalarfield_->unRef();
    }

    scalarfield_ = vt;
    scalarfield_->ref();
    insertChild( 0, scalarfield_->getInventorNode() );

    int volid;
    if ( !par.get(volumestr,volid) ) return -1;
    dataobj = visBase::DM().getObject( volid );
    if ( !dataobj ) return 0;
    mDynamicCastGet(visBase::VolrenDisplay*,vr,dataobj)
    if ( !vr ) return -1;
    if ( volren_ )
    {
	if ( childIndex(volren_->getInventorNode())!=-1 )
	    VisualObjectImpl::removeChild(volren_->getInventorNode());
	volren_->unRef();
    }
    volren_ = vr;
    volren_->ref();
    addChild( volren_->getInventorNode() );

    while ( slices_.size() )
	removeChild( slices_[0]->id() );

    int nrslices = 0;
    par.get( nrslicesstr, nrslices );
    for ( int idx=0; idx<nrslices; idx++ )
    {
	BufferString str( slicestr ); str += idx;
	int sliceid;
	par.get( str, sliceid );
	dataobj = visBase::DM().getObject( sliceid );
	if ( !dataobj ) return 0;
	mDynamicCastGet(visBase::OrthogonalSlice*,os,dataobj)
	if ( !os ) return -1;
	os->ref();
	os->motion.notify( mCB(this,VolumeDisplay,sliceMoving) );
	slices_ += os;
	addChild( os->getInventorNode() );
    }

    setColorTab( getColorTab() );

    return 1;
}


}; // namespace VolumeRender
