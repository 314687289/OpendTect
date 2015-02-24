/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "visfaultdisplay.h"

#include "arrayndimpl.h"
#include "binidsurface.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "emeditor.h"
#include "emfault3d.h"
#include "emfaultauxdata.h"
#include "emmanager.h"
#include "executor.h"
#include "explplaneintersection.h"
#include "faultsticksurface.h"
#include "faulteditor.h"
#include "faulthorintersect.h"
#include "iopar.h"
#include "keystrs.h"
#include "mouseevent.h"
#include "mpeengine.h"
#include "posvecdataset.h"
#include "settings.h"
#include "survinfo.h"
#include "undo.h"
#include "viscoord.h"
#include "visdrawstyle.h"
#include "visevent.h"
#include "visgeomindexedshape.h"
#include "vishorizondisplay.h"
#include "vishorizonsection.h"
#include "vismaterial.h"
#include "vismarkerset.h"
#include "vismpeeditor.h"
#include "visplanedatadisplay.h"
#include "vispolyline.h"
#include "vispolygonselection.h"
#include "visseis2ddisplay.h"
#include "vistransform.h"
#include "vistexturechannels.h"
#include "zaxistransform.h"


namespace visSurvey
{

#define mSceneIdx (scene_ ? scene_->fixedIdx() : -1)

const char* FaultDisplay::sKeyEarthModelID()	{ return "EM ID"; }
const char* FaultDisplay::sKeyTriProjection()	{ return "TriangulateProj"; }
const char* FaultDisplay::sKeyDisplayPanels()	{ return "Display panels"; }
const char* FaultDisplay::sKeyDisplaySticks()	{ return "Display sticks"; }
const char* FaultDisplay::sKeyDisplayIntersections()
				{ return "Display intersections"; }
const char* FaultDisplay::sKeyDisplayHorIntersections()
				{ return "Display horizon intersections"; }
const char* FaultDisplay::sKeyUseTexture()	{ return "Use texture"; }
const char* FaultDisplay::sKeyLineStyle()	{ return "Linestyle"; }
const char* FaultDisplay::sKeyZValues()		{ return "Z values"; }

FaultDisplay::FaultDisplay()
    : MultiTextureSurveyObject()
    , emfault_( 0 )
    , activestickmarker_( visBase::PolyLine3D::create() )
    , validtexture_( false )
    , paneldisplay_( 0 )
    , stickdisplay_( 0 )
    , intersectiondisplay_( 0 )
    , viseditor_( 0 )
    , faulteditor_( 0 )
    , eventcatcher_( 0 )
    , explicitpanels_( 0 )
    , explicitsticks_( 0 )
    , explicitintersections_( 0 )
    , displaytransform_( 0 )
    , zaxistransform_( 0 )
    , voiid_( -1 )
    , activestick_( mUdf(int) )
    , showmanipulator_( false )
    , colorchange( this )
    , displaymodechange( this )
    , usestexture_( true )
    , displaysticks_( false )
    , displaypanels_( true )
    , stickselectmode_( false )
    , displayintersections_( false )
    , displayhorintersections_( false )
    , drawstyle_( new visBase::DrawStyle )
{
    activestickmarker_->ref();
    activestickmarker_->setPickable( false, false );

    if ( !activestickmarker_->getMaterial() )
	activestickmarker_->setMaterial( new visBase::Material );

    addChild( activestickmarker_->osgNode() );

    getMaterial()->setAmbience( 0.8 );

    for ( int idx=0; idx<3; idx++ )
    {
	visBase::MarkerSet* markerset =visBase::MarkerSet::create();
	markerset->ref();
	addChild( markerset->osgNode() );
	knotmarkersets_ += markerset;
	markerset->setMarkersSingleColor(
	    idx ? Color(0,255,0) : Color(255,0,255) );
    }

    drawstyle_->ref();
    addNodeState( drawstyle_ );
    drawstyle_->setLineStyle( LineStyle(LineStyle::Solid,2) );
    texuredatas_.allowNull( true );

    if ( getMaterial() )
	mAttachCB( getMaterial()->change, FaultDisplay::matChangeCB );

    init();
}


FaultDisplay::~FaultDisplay()
{
    detachAllNotifiers();
    setSceneEventCatcher( 0 );
    showManipulator( false );

    if ( viseditor_ ) viseditor_->unRef();

    if ( faulteditor_ ) faulteditor_->unRef();
    if ( emfault_ ) MPE::engine().removeEditor( emfault_->id() );
    faulteditor_ = 0;

    if ( paneldisplay_ )
	paneldisplay_->unRef();

    if ( stickdisplay_ )
	stickdisplay_->unRef();

    if ( intersectiondisplay_ )
	intersectiondisplay_->unRef();

    while ( horintersections_.size() )
    {
	horintersections_.removeSingle(0)->unRef();
    }

    deepErase( horshapes_ );
    horintersectids_.erase();
    delete explicitpanels_;
    delete explicitsticks_;
    delete explicitintersections_;

    if ( emfault_ )
    {
	emfault_->unRef();
	emfault_ = 0;
    }

    if ( displaytransform_ ) displaytransform_->unRef();

    activestickmarker_->unRef();
    for ( int idx=knotmarkersets_.size()-1; idx>=0; idx-- )
    {
	removeChild( knotmarkersets_[idx]->osgNode() );
	knotmarkersets_[idx]->unRef();
    }

    deepErase( stickintersectpoints_ );

    drawstyle_->unRef(); drawstyle_ = 0;

    DataPackMgr& dpman = DPM( DataPackMgr::SurfID() );
    for ( int idx=0; idx<datapackids_.size(); idx++ )
	dpman.release( datapackids_[idx] );

    deepErase( texuredatas_ );
}


void FaultDisplay::setSceneEventCatcher( visBase::EventCatcher* vec )
{
    if ( eventcatcher_ )
    {
	mDetachCB( eventcatcher_->eventhappened,FaultDisplay::mouseCB );
	eventcatcher_->unRef();
    }

    eventcatcher_ = vec;

    if ( eventcatcher_ )
    {
	eventcatcher_->ref();
	mAttachCB( eventcatcher_->eventhappened,FaultDisplay::mouseCB );
    }

    if ( viseditor_ ) viseditor_->setSceneEventCatcher( eventcatcher_ );
}


EM::ObjectID FaultDisplay::getEMID() const
{ return emfault_ ? emfault_->id() : -1; }


#define mSetStickIntersectPointColor( color ) \
     knotmarkersets_[2]->setMarkersSingleColor( color ) ;

#define mErrRet(s) { errmsg_ = s; return false; }

bool FaultDisplay::setEMID( const EM::ObjectID& emid )
{
    if ( emfault_ )
    {
	mDetachCB( emfault_->change,FaultDisplay::emChangeCB );
	emfault_->unRef();
    }

    emfault_ = 0;
    if ( faulteditor_ ) faulteditor_->unRef();
    faulteditor_ = 0;
    if ( viseditor_ ) viseditor_->setEditor( (MPE::ObjectEditor*) 0 );

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::Fault3D*,emfault,emobject.ptr());
    if ( !emfault )
    {
	if ( paneldisplay_ ) paneldisplay_->turnOn( false );
	if ( intersectiondisplay_ ) intersectiondisplay_->turnOn( false );
	for ( int idx=0; idx<horintersections_.size(); idx++ )
	    horintersections_[idx]->turnOn( false );

	if ( stickdisplay_ ) stickdisplay_->turnOn( false );
	return false;
    }

    emfault_ = emfault;
    mAttachCB( emfault_->change,FaultDisplay::emChangeCB );
    emfault_->ref();


    if ( !emfault_->name().isEmpty() )
	setName( emfault_->name() );

    if ( !paneldisplay_ )
    {
	paneldisplay_ = visBase::GeomIndexedShape::create();
	paneldisplay_->ref();
	paneldisplay_->setDisplayTransformation( displaytransform_ );
	paneldisplay_->setMaterial( 0 );
	paneldisplay_->setSelectable( false );
	paneldisplay_->setGeometryShapeType(
	    visBase::GeomIndexedShape::Triangle );
	paneldisplay_->useOsgNormal( true );
	paneldisplay_->setRenderMode( visBase::RenderBothSides );
	paneldisplay_->setTextureChannels( channels_ );

	addChild( paneldisplay_->osgNode() );
    }

    if ( !intersectiondisplay_ )
    {
	intersectiondisplay_ = visBase::GeomIndexedShape::create();
	intersectiondisplay_->ref();
	intersectiondisplay_->setDisplayTransformation( displaytransform_ );
	intersectiondisplay_->setMaterial( 0 );
	intersectiondisplay_->setSelectable( false );
	intersectiondisplay_->setGeometryShapeType(
		visBase::GeomIndexedShape::PolyLine3D,
		Geometry::PrimitiveSet::Lines );
	addChild( intersectiondisplay_->osgNode() );
	intersectiondisplay_->turnOn( false );
    }

    if ( !stickdisplay_ )
    {
	stickdisplay_ = visBase::GeomIndexedShape::create();
	stickdisplay_->ref();
	stickdisplay_->setDisplayTransformation( displaytransform_ );
	if ( !stickdisplay_->getMaterial() )
	    stickdisplay_->setMaterial( new visBase::Material );
	stickdisplay_->setSelectable( false );
	stickdisplay_->setGeometryShapeType(
	    visBase::GeomIndexedShape::PolyLine3D,
	    Geometry::PrimitiveSet::LineStrips );
	addChild( stickdisplay_->osgNode() );
    }

    if ( !explicitpanels_ )
    {
	const float zscale = scene_
	    ? scene_->getZScale() *scene_->getFixedZStretch()
	    : s3dgeom_->zScale();

	mTryAlloc( explicitpanels_,Geometry::ExplFaultStickSurface(0,zscale));
	explicitpanels_->display( false, true );
	explicitpanels_->setTexturePowerOfTwo( true );
	explicitpanels_->setTextureSampling(
		BinIDValue( BinID(s3dgeom_->inlRange().step,
				  s3dgeom_->crlRange().step),
				  s3dgeom_->zStep() ) );

	mTryAlloc( explicitsticks_,Geometry::ExplFaultStickSurface(0,zscale) );
	explicitsticks_->display( true, false );
	explicitsticks_->setTexturePowerOfTwo( true );
	explicitsticks_->setTextureSampling(
		BinIDValue( BinID(s3dgeom_->inlRange().step,
				  s3dgeom_->crlRange().step),
				  s3dgeom_->zStep() ) );

	mTryAlloc( explicitintersections_, Geometry::ExplPlaneIntersection );
    }

    mDynamicCastGet( Geometry::FaultStickSurface*, fss,
		     emfault_->sectionGeometry( emfault_->sectionID(0)) );

    paneldisplay_->setSurface( explicitpanels_ );
    explicitpanels_->setSurface( fss );
    paneldisplay_->touch( true );

    explicitintersections_->setShape( *explicitpanels_ );
    intersectiondisplay_->setSurface( explicitintersections_ );

    stickdisplay_->setSurface( explicitsticks_ );
    explicitsticks_->setSurface( fss );
    stickdisplay_->touch( true );

    if ( !viseditor_ )
    {
	viseditor_ = visSurvey::MPEEditor::create();
	viseditor_->ref();
	viseditor_->setSceneEventCatcher( eventcatcher_ );
	viseditor_->setDisplayTransformation( displaytransform_ );
	viseditor_->sower().alternateSowingOrder();
	viseditor_->sower().setIfDragInvertMask();
	addChild( viseditor_->osgNode() );
    }

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, true );
    mDynamicCastGet( MPE::FaultEditor*, fe, editor.ptr() );
    faulteditor_ = fe;
    if ( faulteditor_ ) faulteditor_->ref();

    viseditor_->setEditor( faulteditor_ );

    displaysticks_ = emfault_->isEmpty();

    mSetStickIntersectPointColor( emfault_->preferredColor() );
    nontexturecol_ = emfault_->preferredColor();
    updateSingleColor();
    updateDisplay();
    updateManipulator();

    return true;
}


void FaultDisplay::removeSelection( const Selector<Coord3>& selector,
				    TaskRunner* taskr )
{
    if ( faulteditor_ )
	faulteditor_->removeSelection( selector );
}


MultiID FaultDisplay::getMultiID() const
{
    return emfault_ ? emfault_->multiID() : MultiID();
}


void FaultDisplay::setColor( Color nc )
{
    if ( emfault_ )
	emfault_->setPreferredColor( nc );
    else
    {
	nontexturecol_ = nc;
	updateSingleColor();
    }
}


void FaultDisplay::updateSingleColor()
{
    const bool usesinglecolor = !showsTexture();

    activestickmarker_->getMaterial()->setColor( nontexturecol_ );

    channels_->turnOn( !usesinglecolor );

    const Color prevcol = getMaterial()->getColor();
    const Color newcol = usesinglecolor ? nontexturecol_.darker(0.3)
					: Color::White();
    if ( newcol != prevcol )
    {
	getMaterial()->setColor( newcol );
	colorchange.trigger();
    }
    else if ( !usesinglecolor )			// To update color column in
	getMaterial()->change.trigger();	// tree if texture is shown

    for ( int idx = 0; idx<horintersections_.size(); idx++ )
	horintersections_[idx]->getMaterial()->setColor( nontexturecol_ );

    if ( intersectiondisplay_ && intersectiondisplay_->getMaterial() )
	intersectiondisplay_->updateMaterialFrom( getMaterial() );

    if ( stickdisplay_ && stickdisplay_->getMaterial() )
	stickdisplay_->updateMaterialFrom( getMaterial() );
}


bool FaultDisplay::usesColor() const
{
    return !showsTexture() || areSticksDisplayed() ||
	   areIntersectionsDisplayed() || areHorizonIntersectionsDisplayed();
}


void FaultDisplay::useTexture( bool yn, bool trigger )
{
    if ( yn && !validtexture_ )
    {
	for ( int idx=0; idx<nrAttribs(); idx++ )
	{
	    if ( getSelSpec(idx) &&
		 getSelSpec(idx)->id()==Attrib::SelSpec::cNoAttrib() )
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
	colorchange.trigger();
}


void FaultDisplay::setDepthAsAttrib( int attrib )
{
    const bool attribwasdepth = getSelSpec(attrib) &&
		    FixedString(getSelSpec(attrib)->userRef())==sKeyZValues();

    const Attrib::SelSpec as( sKeyZValues(), Attrib::SelSpec::cNoAttrib(),
			      false, "" );
    setSelSpec( attrib, as );

    DataPointSet* data = new DataPointSet( false, true );
    DPM( DataPackMgr::PointID() ).addAndObtain( data );
    getRandomPos( *data, 0 );
    DataColDef* zvalsdef = new DataColDef( sKeyZValues() );
    data->dataSet().add( zvalsdef );
    BinIDValueSet& bivs = data->bivSet();
    if ( data->size() && bivs.nrVals()==4 )
    {
	int zcol = data->dataSet().findColDef( *zvalsdef,
					       PosVecDataSet::NameExact );
	if ( zcol==-1 ) zcol = 3;

	BinIDValueSet::SPos pos;
	while ( bivs.next(pos) )
	{
	    float* vals = bivs.getVals(pos);
	    if ( zaxistransform_ )
	    {
		vals[zcol] = zaxistransform_->transform(
				    BinIDValue(bivs.getBinID(pos), vals[0]) );
	    }
	    else
		vals[zcol] = vals[0];
	}

	setRandomPosData( attrib, data, 0 );
    }

    DPM( DataPackMgr::PointID() ).release( data->id() );

    if ( !attribwasdepth )
    {
	BufferString seqnm;
	Settings::common().get( "dTect.Horizon.Color table", seqnm );
	ColTab::Sequence seq( seqnm );
	setColTabSequence( attrib, seq, 0 );
	setColTabMapperSetup( attrib, ColTab::MapperSetup(), 0 );
    }
}


bool FaultDisplay::usesTexture() const
{ return usestexture_; }


bool FaultDisplay::showsTexture() const
{ return canShowTexture() && usesTexture(); }


bool FaultDisplay::canShowTexture() const
{ return validtexture_ && isAnyAttribEnabled() && arePanelsDisplayedInFull(); }


NotifierAccess* FaultDisplay::materialChange()
{ return &getMaterial()->change; }


Color FaultDisplay::getColor() const
{ return nontexturecol_; }


void FaultDisplay::updatePanelDisplay()
{
    if ( paneldisplay_ )
    {
	const bool dodisplay = arePanelsDisplayed() &&
			       !areIntersectionsDisplayed() &&
			       !areHorizonIntersectionsDisplayed();
	if ( dodisplay )
	    paneldisplay_->touch( false,false );

	paneldisplay_->turnOn( dodisplay );
    }
}


void FaultDisplay::updateStickDisplay()
{
    if ( stickdisplay_ )
    {
	setLineRadius( stickdisplay_ );

	bool dodisplay = areSticksDisplayed();
	if ( arePanelsDisplayedInFull() && emfault_ && emfault_->nrSections() )
	{
	    const EM::SectionID sid = emfault_->sectionID( 0 );
	    if ( emfault_->geometry().nrSticks(sid) == 1 )
		dodisplay = true;
	}

	if ( dodisplay )
	    stickdisplay_->touch( false, false );

	stickdisplay_->turnOn( dodisplay );
    }

    updateKnotMarkers();
    updateEditorMarkers();
}


void FaultDisplay::updateIntersectionDisplay()
{
    if ( intersectiondisplay_ )
    {
	setLineRadius( intersectiondisplay_ );

	const bool dodisplay = areIntersectionsDisplayed() &&
			       arePanelsDisplayed();
	if ( dodisplay )
	    intersectiondisplay_->touch( false );

	intersectiondisplay_->turnOn( dodisplay );
    }

    updateStickHiding();
}


void FaultDisplay::updateHorizonIntersectionDisplay()
{
    for ( int idx=0; idx<horintersections_.size(); idx++ )
    {
	setLineRadius( horintersections_[idx] );

	const bool dodisplay = areHorizonIntersectionsDisplayed() &&
			       arePanelsDisplayed();
	if ( dodisplay )
	    horintersections_[idx]->touch( false );

	horintersections_[idx]->turnOn( dodisplay );
    }
}


void FaultDisplay::updateDisplay()
{
    updateIntersectionDisplay();
    updateHorizonIntersectionDisplay();
    updatePanelDisplay();
    updateStickDisplay();
}


void FaultDisplay::triangulateAlg( mFltTriProj projplane )
{
    if ( !explicitpanels_ || projplane==explicitpanels_->triangulateAlg() )
	return;

    explicitpanels_->triangulateAlg( projplane );
    paneldisplay_->touch( true );
    updateIntersectionDisplay();
}


mFltTriProj FaultDisplay::triangulateAlg() const
{
    return explicitpanels_ ? explicitpanels_->triangulateAlg()
			   : Geometry::ExplFaultStickSurface::None;
}


void FaultDisplay::display( bool sticks, bool panels )
{
    displaysticks_ = sticks;
    displaypanels_ = panels;

    updateDisplay();
    updateManipulator();
    displaymodechange.trigger();
}


bool FaultDisplay::areSticksDisplayed() const
{ return displaysticks_; }


bool FaultDisplay::arePanelsDisplayed() const
{ return displaypanels_; }


bool FaultDisplay::arePanelsDisplayedInFull() const
{
    return paneldisplay_ ? paneldisplay_->isOn() : false;
}


void FaultDisplay::fillPar( IOPar& par ) const
{
    visSurvey::MultiTextureSurveyObject::fillPar( par );

    par.set( sKeyEarthModelID(), getMultiID() );
    par.set( sKeyTriProjection(), triangulateAlg() );

    par.setYN( sKeyDisplayPanels(), displaypanels_ );
    par.setYN( sKeyDisplaySticks(), displaysticks_ );
    par.setYN( sKeyDisplayIntersections(), displayintersections_ );
    par.setYN( sKeyDisplayHorIntersections(), displayhorintersections_ );
    par.setYN( sKeyUseTexture(), usestexture_ );
    par.set( sKey::Color(), (int) getColor().rgb() );

    if ( lineStyle() )
    {
	BufferString str;
	lineStyle()->toString( str );
	par.set( sKeyLineStyle(), str );
    }
}


bool FaultDisplay::usePar( const IOPar& par )
{
    if ( !visSurvey::MultiTextureSurveyObject::usePar( par ) )
	return false;

    MultiID newmid;
    if ( par.get(sKeyEarthModelID(),newmid) )
    {
	EM::ObjectID emid = EM::EMM().getObjectID( newmid );
	RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
	if ( !emobject )
	{
	    PtrMan<Executor> loader = EM::EMM().objectLoader( newmid );
	    if ( loader ) loader->execute();
	    emid = EM::EMM().getObjectID( newmid );
	    emobject = EM::EMM().getObject( emid );
	}

	if ( emobject ) setEMID( emobject->id() );
    }

    int tp;
    par.get( sKeyTriProjection(), tp );
    triangulateAlg( (Geometry::ExplFaultStickSurface::TriProjection)tp );

    par.getYN( sKeyDisplayPanels(), displaypanels_ );
    par.getYN( sKeyDisplaySticks(), displaysticks_ );
    par.getYN( sKeyDisplayIntersections(), displayintersections_ );
    par.getYN( sKeyDisplayHorIntersections(), displayhorintersections_ );
    par.getYN( sKeyUseTexture(), usestexture_ );

    par.get( sKey::Color(), (int&) nontexturecol_.rgb() );
    updateSingleColor();

    BufferString linestyle;
    if ( par.get(sKeyLineStyle(),linestyle) )
    {
	LineStyle ls;
	ls.fromString( linestyle );
	setLineStyle( ls );
    }

    return true;
}


void FaultDisplay::setDisplayTransformation( const mVisTrans* nt )
{
    if ( paneldisplay_ ) paneldisplay_->setDisplayTransformation( nt );
    if ( stickdisplay_ ) stickdisplay_->setDisplayTransformation( nt );
    if ( intersectiondisplay_ )
	intersectiondisplay_->setDisplayTransformation( nt );
    for ( int idx=0; idx<horintersections_.size(); idx++ )
	horintersections_[idx]->setDisplayTransformation( nt );

    if ( viseditor_ ) viseditor_->setDisplayTransformation( nt );
    activestickmarker_->setDisplayTransformation( nt );

    for ( int idx=0; idx<knotmarkersets_.size(); idx++ )
	knotmarkersets_[idx]->setDisplayTransformation( nt );

    if ( displaytransform_ ) displaytransform_->unRef();
    displaytransform_ = nt;
    if ( displaytransform_ ) displaytransform_->ref();
}


const mVisTrans* FaultDisplay::getDisplayTransformation() const
{ return displaytransform_; }


bool FaultDisplay::setZAxisTransform( ZAxisTransform* zat, TaskRunner* )
{
    if ( zaxistransform_ )
    {
	if ( zaxistransform_->changeNotifier() )
	    zaxistransform_->changeNotifier()->remove(
		mCB(this,FaultDisplay,dataTransformCB) );
	if ( voiid_>0 )
	{
	    zaxistransform_->removeVolumeOfInterest( voiid_ );
	    voiid_ = -1;
	}

	zaxistransform_->unRef();
	zaxistransform_ = 0;
    }

    zaxistransform_ = zat;
    if ( zaxistransform_ )
    {
	zaxistransform_->ref();
	if ( zaxistransform_->changeNotifier() )
	    zaxistransform_->changeNotifier()->notify(
		    mCB(this,FaultDisplay,dataTransformCB) );
    }

    return true;
}


const ZAxisTransform* FaultDisplay::getZAxisTransform() const
{ return zaxistransform_; }


void FaultDisplay::dataTransformCB( CallBacker* )
{
    // TODO: implement
}


Coord3 FaultDisplay::disp2world( const Coord3& displaypos ) const
{
    Coord3 pos = displaypos;
    if ( pos.isDefined() )
    {
	if ( scene_ )
	    scene_->getTempZStretchTransform()->transformBack( pos );
	if ( displaytransform_ )
	    displaytransform_->transformBack( pos );
    }
    return pos;
}


#define mZScale() \
    ( scene_ ? scene_->getZScale()*scene_->getFixedZStretch() \
	     : s3dgeom_->zScale() )

void FaultDisplay::mouseCB( CallBacker* cb )
{
    if ( stickselectmode_ )
	return stickSelectCB( cb );

    if ( !emfault_ || !faulteditor_ || !isOn() || eventcatcher_->isHandled() ||
	 !isSelected() || !viseditor_ || !viseditor_->isOn() )
	return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);

    faulteditor_->setSowingPivot( disp2world(viseditor_->sower().pivotPos()) );
    if ( viseditor_->sower().accept(eventinfo) )
	return;

    TrcKeyZSampling mouseplanecs;
    mouseplanecs.setEmpty();
    Coord3 editnormal = Coord3::udf();

    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	const int visid = eventinfo.pickedobjids[idx];
	visBase::DataObject* dataobj = visBase::DM().getObject( visid );

	mDynamicCastGet( visSurvey::PlaneDataDisplay*, plane, dataobj );
	if ( plane )
	{
	    mouseplanecs = plane->getTrcKeyZSampling();
	    editnormal = plane->getNormal( Coord3::udf() );
	    break;
	}
    }

    Coord3 pos = disp2world( eventinfo.displaypickedpos );

    const EM::PosID pid = viseditor_ ?
       viseditor_->mouseClickDragger(eventinfo.pickedobjids) : EM::PosID::udf();

    if ( pid.isUdf() && mouseplanecs.isEmpty() )
    {
	setActiveStick( EM::PosID::udf() );
	return;
    }

    bool makenewstick = eventinfo.type==visBase::MouseClick &&
			!OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
			!OD::altKeyboardButton(eventinfo.buttonstate_) &&
			OD::shiftKeyboardButton(eventinfo.buttonstate_) &&
			OD::leftMouseButton(eventinfo.buttonstate_);

    EM::PosID insertpid;
    faulteditor_->setZScale( mZScale() );
    faulteditor_->getInteractionInfo(makenewstick, insertpid, pos, &editnormal);

    if ( pid.isUdf() && !viseditor_->isDragging() )
	setActiveStick( makenewstick ? EM::PosID::udf() : insertpid );

    if ( locked_ || !pos.isDefined() || viseditor_->isDragging() )
	return;

    if ( !pid.isUdf() )
    {
	if ( eventinfo.type == visBase::MouseClick )
	{
	    faulteditor_->setLastClicked( pid );
	    setActiveStick( pid );
	}

	if ( eventinfo.type==visBase::MouseClick &&
	     OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
	     !OD::altKeyboardButton(eventinfo.buttonstate_) &&
	     !OD::shiftKeyboardButton(eventinfo.buttonstate_) &&
	     OD::leftMouseButton(eventinfo.buttonstate_) )
	{
	    eventcatcher_->setHandled();
	    if ( !eventinfo.pressed )
	    {
		bool res;
		const int rmstick = pid.getRowCol().row();

		EM::Fault3DGeometry& f3dg = emfault_->geometry();
		if ( f3dg.nrKnots(pid.sectionID(),rmstick)==1 )
		    res = f3dg.removeStick( pid.sectionID(), rmstick, true );
		else
		    res = f3dg.removeKnot( pid.sectionID(), pid.subID(), true );

		if ( res && !viseditor_->sower().moreToSow() )
		{
		    EM::EMM().undo().setUserInteractionEnd(
			    EM::EMM().undo().currentEventID() );
		    updateDisplay();
		}
	    }
	}

	return;
    }

    if ( eventinfo.type!=visBase::MouseClick ||
	 OD::altKeyboardButton(eventinfo.buttonstate_) ||
	 OD::ctrlKeyboardButton(eventinfo.buttonstate_) ||
	 !OD::leftMouseButton(eventinfo.buttonstate_) )
	return;

    if ( viseditor_->sower().activate(emfault_->preferredColor(), eventinfo) )
	return;

    if ( eventinfo.pressed || insertpid.isUdf() )
	return;

    if ( makenewstick )
    {
	//Add Stick
	if ( mouseplanecs.isEmpty() )
	    return;

	const int insertstick = insertpid.isUdf()
	    ? mUdf(int)
	    : insertpid.getRowCol().row();

	if ( emfault_->geometry().insertStick( insertpid.sectionID(),
	       insertstick, 0, pos, editnormal, true ) )
	{
	    faulteditor_->setLastClicked( insertpid );
	    if ( !viseditor_->sower().moreToSow() )
	    {
		EM::EMM().undo().setUserInteractionEnd(
		    EM::EMM().undo().currentEventID() );

		updateDisplay();
		setActiveStick( insertpid );
		faulteditor_->editpositionchange.trigger();
	    }
	}
    }
    else
    {
	if ( emfault_->geometry().insertKnot( insertpid.sectionID(),
		insertpid.subID(), pos, true ) )
	{
	    faulteditor_->setLastClicked( insertpid );
	    if ( !viseditor_->sower().moreToSow() )
	    {
		EM::EMM().undo().setUserInteractionEnd(
		    EM::EMM().undo().currentEventID() );
		faulteditor_->editpositionchange.trigger();
	    }
	}
    }

    eventcatcher_->setHandled();
}


#define mMatchMarker( sid, sticknr, pos1, pos2,eps ) \
    if ( pos1.isSameAs(pos2,eps) ) \
    { \
	Geometry::FaultStickSet* fss = \
				emfault_->geometry().sectionGeometry( sid ); \
	if ( fss ) \
	{ \
	    fss->selectStick( sticknr, !ctrldown_ ); \
	    updateKnotMarkers(); \
	    eventcatcher_->setHandled(); \
	    return; \
	} \
    }

void FaultDisplay::stickSelectCB( CallBacker* cb )
{
    if ( !emfault_ || !isOn() || eventcatcher_->isHandled() || !isSelected() )
	return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);

    bool leftmousebutton = OD::leftMouseButton( eventinfo.buttonstate_ );
    ctrldown_ = OD::ctrlKeyboardButton( eventinfo.buttonstate_ );

    if ( eventinfo.tabletinfo &&
	 eventinfo.tabletinfo->pointertype_==TabletInfo::Eraser )
    {
	leftmousebutton = true;
	ctrldown_ = true;
    }

    if ( eventinfo.type!=visBase::MouseClick || !leftmousebutton )
	return;

    const double epsxy = get3DSurvGeom()->inlDistance()*0.1f;
    const double epsz = 0.01 * get3DSurvGeom()->zStep();
    const Coord3 eps( epsxy,epsxy,epsz );
    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	const Coord3 selectpos = eventinfo.worldpickedpos;
	const int visid = eventinfo.pickedobjids[idx];
	visBase::DataObject* dataobj = visBase::DM().getObject( visid );
	mDynamicCastGet( visBase::MarkerSet*, markerset, dataobj );

	if ( markerset )
	{
	    const int markeridx = markerset->findClosestMarker( selectpos );
	    if( markeridx ==  -1 ) continue;

	    const Coord3 markerpos =
		markerset->getCoordinates()->getPos( markeridx );

	    for ( int sipidx=0; sipidx<stickintersectpoints_.size(); sipidx++ )
	    {
		const StickIntersectPoint* sip = stickintersectpoints_[sipidx];
		mMatchMarker(sip->sid_,sip->sticknr_, markerpos,sip->pos_,eps );
	    }

	    PtrMan<EM::EMObjectIterator> iter =
					emfault_->geometry().createIterator(-1);
	    while ( true )
	    {
		const EM::PosID pid = iter->next();
		if ( pid.objectID() == -1 )
		    return;

		const int sticknr = pid.getRowCol().row();
		mMatchMarker( pid.sectionID(), sticknr,
			      markerpos, emfault_->getPos(pid),eps );
	    }
	}
    }
}


void FaultDisplay::setActiveStick( const EM::PosID& pid )
{
    const int sticknr = pid.isUdf() ? mUdf(int) : pid.getRowCol().row();
    if ( activestick_ != sticknr )
    {
	activestick_ = sticknr;
	updateActiveStickMarker();
    }
}


void FaultDisplay::emChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);

    if ( cbdata.event == EM::EMObjectCallbackData::SectionChange )
    {
	if ( emfault_ && emfault_->nrSections() )
	{
	    mDynamicCastGet( Geometry::FaultStickSurface*, fss,
			     emfault_->sectionGeometry(emfault_->sectionID(0)) )

	    if ( explicitpanels_ )
		explicitpanels_->setSurface( fss );
	    if ( explicitsticks_ )
		explicitsticks_->setSurface( fss );
	}
    }

    if ( cbdata.event==EM::EMObjectCallbackData::BurstAlert ||
	 cbdata.event==EM::EMObjectCallbackData::SectionChange ||
	 cbdata.event==EM::EMObjectCallbackData::PositionChange )
    {
	validtexture_ = false;
	updateSingleColor();
	if ( cbdata.event==EM::EMObjectCallbackData::PositionChange )
	{
	    if ( cbdata.pid0.getRowCol().row()==activestick_ )
		updateActiveStickMarker();
	}
	else
	    updateActiveStickMarker();
    }
    else if ( cbdata.event==EM::EMObjectCallbackData::PrefColorChange )
    {
	mSetStickIntersectPointColor( emfault_->preferredColor() );
	nontexturecol_ = emfault_->preferredColor();
	updateSingleColor();
    }

    if ( emfault_ && !emfault_->hasBurstAlert() )
	updateDisplay();
}


void FaultDisplay::updateActiveStickMarker()
{
    activestickmarker_->removeAllPrimitiveSets();
    activestickmarker_->getCoordinates()->setEmpty();

    if ( mIsUdf(activestick_) || !showmanipulator_ || !displaysticks_ )
	activestickmarker_->turnOn( false );

    if ( !emfault_->nrSections() )
    {
	activestickmarker_->turnOn( false );
	return;
    }

    mDynamicCastGet( Geometry::FaultStickSurface*, fss,
		     emfault_->sectionGeometry( emfault_->sectionID(0)) );

    const StepInterval<int> rowrg = fss->rowRange();
    if ( rowrg.isUdf() || !rowrg.includes(activestick_,false) )
    {
	activestickmarker_->turnOn( false );
	return;
    }

    const StepInterval<int> colrg = fss->colRange( activestick_ );
    if ( colrg.isUdf() || colrg.start==colrg.stop )
    {
	activestickmarker_->turnOn( false );
	return;
    }

    RowCol rc( activestick_, 0 );
    for ( rc.col()=colrg.start; rc.col()<=colrg.stop; rc.col() += colrg.step )
    {
	const Coord3 pos = fss->getKnot( rc );
	activestickmarker_->getCoordinates()->addPos( pos );
    }

    activestickmarker_->turnOn( true );
}


void FaultDisplay::showManipulator( bool yn )
{
    showmanipulator_ = yn;
    updateDisplay();
    updateManipulator();
    displaymodechange.trigger();
}


void FaultDisplay::updateManipulator()
{
    const bool show = showmanipulator_ && areSticksDisplayed();
    if ( viseditor_ )
	viseditor_->turnOn( show && !stickselectmode_ );

    if ( activestickmarker_ )
	activestickmarker_->turnOn( show && !stickselectmode_);
    if ( scene_ )
	scene_->blockMouseSelection( show );
}


bool  FaultDisplay::isManipulatorShown() const
{ return showmanipulator_; }


int FaultDisplay::nrResolutions() const
{ return 1; }


void FaultDisplay::getRandomPos( DataPointSet& dpset, TaskRunner* taskr ) const
{
    if ( explicitpanels_ )
    {
	MouseCursorChanger mousecursorchanger( MouseCursor::Wait );
	explicitpanels_->getTexturePositions( dpset, taskr );
	paneldisplay_->touch( false, false );
    }
}


void FaultDisplay::getRandomPosCache( int attrib, DataPointSet& data ) const
{
    if ( attrib<0 || attrib>=nrAttribs() )
	return;

    DataPack::ID dpid = getDataPackID( attrib );
    DataPackMgr& dpman = DPM( DataPackMgr::SurfID() );
    const DataPack* datapack = dpman.obtain( dpid );
    mDynamicCastGet( const DataPointSet*, dps, datapack );
    if ( dps )
	data  = *dps;

    dpman.release( dpid );
}


void FaultDisplay::setRandomPosData( int attrib, const DataPointSet* dpset,
				     TaskRunner* )
{
    const DataColDef texturej(Geometry::ExplFaultStickSurface::sKeyTextureJ());
    const int columnj =
	dpset->dataSet().findColDef(texturej,PosVecDataSet::NameExact);

    setRandomPosDataInternal( attrib, dpset, columnj+1, 0 );
}


void FaultDisplay::setRandomPosDataInternal( int attrib,
    const DataPointSet* dpset, int column, TaskRunner* taskr )
{
    if ( attrib>=nrAttribs() || !dpset || dpset->nrCols()<3 ||
	 !explicitpanels_ )
    {
	validtexture_ = false;
	updateSingleColor();
	return;
    }

    const RowCol sz = explicitpanels_->getTextureSize();

    while ( texuredatas_.size()-1 < attrib )
	texuredatas_ += 0;

    mDeclareAndTryAlloc( Array2D<float>*, texturedata,
			 Array2DImpl<float>(sz.col(),sz.row()) );

    float* texturedataptr = texturedata->getData();
    for ( int idy=0; idy<texturedata->info().getTotalSz(); idy++ )
	(*texturedataptr++) = mUdf(float);

    const DataColDef texturei(Geometry::ExplFaultStickSurface::sKeyTextureI());
    const DataColDef texturej(Geometry::ExplFaultStickSurface::sKeyTextureJ());
    const int columni =
	dpset->dataSet().findColDef(texturei,PosVecDataSet::NameExact);
    const int columnj =
	dpset->dataSet().findColDef(texturej,PosVecDataSet::NameExact);

    const BinIDValueSet& vals = dpset->bivSet();
    BinIDValueSet::SPos pos;
    while ( vals.next( pos ) )
    {
	const float* ptr = vals.getVals( pos );
	const float i = ptr[columni];
	const float j = ptr[columnj];
	texturedata->set( mNINT32(j), mNINT32(i), ptr[column] );
    }

    delete texuredatas_.replace( attrib, texturedata );
    channels_->setSize( attrib, 1, texturedata->info().getSize(0),
			texturedata->info().getSize(1) );
    channels_->setUnMappedData( attrib, 0, texturedata->getData(),
				OD::UsePtr, taskr );
    validtexture_ = true;
    updateSingleColor();
}


void FaultDisplay::showSelectedSurfaceData()
{
    int lastattridx = nrAttribs()-1;
    if ( !emfault_ || lastattridx<0 )
	return;

    EM::FaultAuxData* auxdata = emfault_->auxData();
    if ( !auxdata )
	return;

    const TypeSet<int>& selindies = auxdata->selectedIndices();
    for ( int idx=0; idx<selindies.size(); idx++ )
    {
	const Array2D<float>* data = auxdata->loadIfNotLoaded(selindies[idx]);
	if ( !data )
	    continue;

	channels_->setSize( lastattridx, 1, data->info().getSize(0),
			       data->info().getSize(1) );
	channels_->setUnMappedData( lastattridx--, 0, data->getData(),
				    OD::UsePtr, 0 );
	if ( lastattridx<0 )
	    break;
    }

    validtexture_ = true;
    updateSingleColor();
}


const BufferStringSet* FaultDisplay::selectedSurfaceDataNames() const
{
    return emfault_ && emfault_->auxData()
	? &emfault_->auxData()->selectedNames() : 0;
}


const Array2D<float>* FaultDisplay::getTextureData( int attrib )
{ return texuredatas_.validIdx(attrib) ? texuredatas_[attrib] : 0; }


void FaultDisplay::setResolution( int res, TaskRunner* taskr )
{
    if ( res==resolution_ )
	return;

    resolution_ = res;
}


bool FaultDisplay::getCacheValue( int attrib, int version, const Coord3& crd,
	                          float& value ) const
{ return true; }


void FaultDisplay::addCache()
{
    datapackids_ += 0;
}

void FaultDisplay::removeCache( int attrib )
{
    if ( !datapackids_.validIdx(attrib) )
	return;

    DPM( DataPackMgr::SurfID() ).release( datapackids_[attrib] );
    datapackids_.removeSingle( attrib );
}

void FaultDisplay::swapCache( int attr0, int attr1 )
{}

void FaultDisplay::emptyCache( int attrib )
{}

bool FaultDisplay::hasCache( int attrib ) const
{ return false; }


void FaultDisplay::displayIntersections( bool yn )
{
    displayintersections_ = yn;
    updateDisplay();
    displaymodechange.trigger();
}


bool FaultDisplay::areIntersectionsDisplayed() const
{ return displayintersections_; }


bool FaultDisplay::canDisplayIntersections() const
{
    if ( scene_ )
    {
	for ( int idx=0; idx<scene_->size(); idx++ )
	{
	    visBase::DataObject* dataobj = scene_->getObject( idx );
	    mDynamicCastGet( PlaneDataDisplay*, plane, dataobj );
	    mDynamicCastGet( Seis2DDisplay*, s2dd, dataobj );
	    if ( (plane && plane->isOn()) || (s2dd && s2dd->isOn()) )
		return true;
	}
    }

    return displayintersections_;
}


void FaultDisplay::displayHorizonIntersections( bool yn )
{
    displayhorintersections_ = yn;
    updateDisplay();
    displaymodechange.trigger();
}


bool FaultDisplay::areHorizonIntersectionsDisplayed() const
{  return displayhorintersections_; }


bool FaultDisplay::canDisplayHorizonIntersections() const
{
    if ( scene_ )
    {
	for ( int idx=0; idx<scene_->size(); idx++ )
	{
	    visBase::DataObject* dataobj = scene_->getObject( idx );
	    mDynamicCastGet( HorizonDisplay*, hor, dataobj );
	    if ( hor && hor->isOn() )
		return true;
	}
    }

    return displayhorintersections_;
}


void FaultDisplay::updateHorizonIntersections( int whichobj,
	const ObjectSet<const SurveyObject>& objs )
{
    if ( !emfault_ )
	return;

    ObjectSet<HorizonDisplay> activehordisps;
    TypeSet<int> activehorids;
    for ( int idx=0; idx<objs.size(); idx++ )
    {
	mDynamicCastGet( const HorizonDisplay*, hor, objs[idx] );
	if ( !hor || !hor->isOn() || !hor->getSectionIDs().size() )
	    continue;
	if ( hor->getOnlyAtSectionsDisplay() )
	    continue;

	activehordisps += const_cast<HorizonDisplay*>( hor );
	activehorids += hor->id();
    }

    for ( int idx=horintersections_.size()-1; idx>=0; idx-- )
    {

	if ( whichobj>=0 && horintersectids_[idx]!=whichobj )
	    continue;
	if ( whichobj<0 && activehorids.isPresent(horintersectids_[idx]) )
	    continue;

	horintersections_[idx]->turnOn( false );
	horintersections_.removeSingle( idx )->unRef();
	delete horshapes_.removeSingle( idx );
	horintersectids_.removeSingle( idx );
    }

    mDynamicCastGet( Geometry::FaultStickSurface*, fss,
		     emfault_->sectionGeometry( emfault_->sectionID(0)) );

    for ( int idx=0; idx<activehordisps.size(); idx++ )
    {
	if ( horintersectids_.isPresent(activehorids[idx]) )
	    continue;

	EM::SectionID sid = activehordisps[idx]->getSectionIDs()[0];
	Geometry::BinIDSurface* surf =
	    activehordisps[idx]->getHorizonSection(sid)->getSurface();

	visBase::GeomIndexedShape* line = visBase::GeomIndexedShape::create();
	line->ref();
	if ( !line->getMaterial() )
	    line->setMaterial(new visBase::Material);

	line->getMaterial()->setColor( nontexturecol_ );
	line->setSelectable( false );
	line->setGeometryShapeType( visBase::GeomIndexedShape::PolyLine3D,
	    Geometry::PrimitiveSet::LineStrips );
	line->setDisplayTransformation( displaytransform_ );
	addChild( line->osgNode() );
	line->turnOn( false );
	Geometry::ExplFaultStickSurface* shape = 0;
	mTryAlloc( shape, Geometry::ExplFaultStickSurface(0,mZScale()) );
	line->setSurface( shape );
	shape->display( false, false );
	shape->setSurface( fss );
	const float zshift = (float) activehordisps[idx]->getTranslation().z;
	Geometry::FaultBinIDSurfaceIntersector it( zshift, *surf,
		*explicitpanels_, *shape->coordList() );
	it.setShape( *shape );
	it.compute();

	line->touch( true, false );
	horintersections_ += line;
	horshapes_ += shape;
	horintersectids_ += activehorids[idx];
    }

    updateHorizonIntersectionDisplay();
}


void FaultDisplay::otherObjectsMoved( const ObjectSet<const SurveyObject>& objs,
				      int whichobj )
{
    updateHorizonIntersections( whichobj, objs );

    if ( !explicitintersections_ ) return;

    ObjectSet<const SurveyObject> usedobjects;
    TypeSet<int> planeids;

    for ( int idx=0; idx<objs.size(); idx++ )
    {

	mDynamicCastGet( const PlaneDataDisplay*, plane, objs[idx] );
	if ( !plane || !plane->isOn() )
	    continue;

	const TrcKeyZSampling cs = plane->getTrcKeyZSampling(true,true,-1);
	const BinID b00 = cs.hrg.start, b11 = cs.hrg.stop;
	BinID b01, b10;

	if ( plane->getOrientation()==OD::ZSlice )
	{
	    b01 = BinID( cs.hrg.start.inl(), cs.hrg.stop.crl() );
	    b10 = BinID( cs.hrg.stop.inl(), cs.hrg.start.crl() );
	}
	else
	{
	    b01 = b00;
	    b10 = b11;
	}

	const Coord3 c00( s3dgeom_->transform(b00),cs.zsamp_.start );
	const Coord3 c01( s3dgeom_->transform(b01),cs.zsamp_.stop );
	const Coord3 c11( s3dgeom_->transform(b11),cs.zsamp_.stop );
	const Coord3 c10( s3dgeom_->transform(b10),cs.zsamp_.start );

	const Coord3 normal = (c01-c00).cross(c10-c00).normalize();

	TypeSet<Coord3> positions;
	positions += c00;
	positions += c01;
	positions += c11;
	positions += c10;

	const int idy = intersectionobjs_.indexOf( objs[idx] );
	if ( idy==-1 )
	{
	    usedobjects += plane;
	    planeids += explicitintersections_->addPlane(normal,positions);
	}
	else
	{
	    usedobjects += plane;
	    explicitintersections_->setPlane( planeids_[idy],
					      normal, positions );
	    planeids += planeids_[idy];

	    intersectionobjs_.removeSingle( idy );
	    planeids_.removeSingle( idy );
	}
    }

    for ( int idx=planeids_.size()-1; idx>=0; idx-- )
	explicitintersections_->removePlane( planeids_[idx] );

    intersectionobjs_ = usedobjects;
    planeids_ = planeids;
    updateIntersectionDisplay();
    updateStickDisplay();
}


void FaultDisplay::setStickSelectMode( bool yn )
{
    stickselectmode_ = yn;
    ctrldown_ = false;

    setActiveStick( EM::PosID::udf() );
    updateManipulator();
    updateKnotMarkers();

    if ( scene_ && scene_->getPolySelection() )
    {
	if ( yn )
	    mAttachCBIfNotAttached(
	    scene_->getPolySelection()->polygonFinished(),
	    FaultDisplay::polygonFinishedCB );
	else
	    mDetachCB(
	    scene_->getPolySelection()->polygonFinished(),
	    FaultDisplay::polygonFinishedCB );
    }
}


bool FaultDisplay::isSelectableMarkerInPolySel(
					const Coord3& markerworldpos ) const
{
    visBase::PolygonSelection* polysel = scene_->getPolySelection();
    if ( polysel )
	return polysel->isInside( markerworldpos );
    return false;
}


void FaultDisplay::polygonFinishedCB( CallBacker* cb )
{
    if ( !stickselectmode_ || !emfault_ || !scene_ || !displaysticks_ ||
	 !isOn() || !isSelected() )
	return;

    MouseCursorChanger mousecursorchanger( MouseCursor::Wait );

    for ( int idx=0; idx<stickintersectpoints_.size(); idx++ )
    {
	const StickIntersectPoint* sip = stickintersectpoints_[idx];
	Geometry::FaultStickSet* fss =
			emfault_->geometry().sectionGeometry( sip->sid_ );

	if ( !fss || fss->isStickSelected(sip->sticknr_)!=ctrldown_ )
	    continue;

	if ( !isSelectableMarkerInPolySel(sip->pos_) )
	    continue;

	fss->selectStick( sip->sticknr_, !ctrldown_ );
    }

    PtrMan<EM::EMObjectIterator> iter = emfault_->geometry().createIterator(-1);
    while ( true )
    {
	EM::PosID pid = iter->next();
	if ( pid.objectID() == -1 )
	    break;

	const int sticknr = pid.getRowCol().row();
	const EM::SectionID sid = pid.sectionID();
	Geometry::FaultStickSet* fss =
				 emfault_->geometry().sectionGeometry( sid );

	if ( fss->isStickSelected(sticknr) != ctrldown_ )
		continue;

	if ( !isSelectableMarkerInPolySel(emfault_->getPos(pid)) )
	    continue;

	fss->selectStick( sticknr, !ctrldown_ );
    }

    updateKnotMarkers();
    scene_->getPolySelection()->clear();
}


bool FaultDisplay::isInStickSelectMode() const
{ return stickselectmode_; }


void FaultDisplay::updateEditorMarkers()
{
    if ( !emfault_ || !viseditor_ )
	return;

    PtrMan<EM::EMObjectIterator> iter = emfault_->geometry().createIterator(-1);
    while ( true )
    {
	const EM::PosID pid = iter->next();
	if ( pid.objectID() == -1 )
	    break;

	const EM::SectionID sid = pid.sectionID();
	const int sticknr = pid.getRowCol().row();
	Geometry::FaultStickSet* fs = emfault_->geometry().sectionGeometry(sid);
	viseditor_->turnOnMarker( pid, !fs->isStickHidden(sticknr,mSceneIdx) );
    }
}


#define mForceDrawMarkerSet( knotersets )\
    for ( int idx = 0; idx < knotersets.size(); idx++ )\
	knotersets[ idx ]->forceRedraw( true );\

void FaultDisplay::updateKnotMarkers()
{
    if ( !emfault_ || (viseditor_ && viseditor_->sower().moreToSow()) )
	return;

    for ( int idx=0; idx<knotmarkersets_.size(); idx++ )
    {
	visBase::MarkerSet* markerset = knotmarkersets_[idx];
	markerset->clearMarkers();
	markerset->setMarkerStyle( MarkerStyle3D::Sphere );
	markerset->setMaterial(0);
	markerset->setDisplayTransformation( displaytransform_ );
	markerset->setScreenSize(3);
    }

    if ( !areSticksDisplayed() )
	return;

    int groupidx = !showmanipulator_ || !stickselectmode_  ? 2 : 0;

    knotmarkersets_[groupidx]->setType( MarkerStyle3D::Sphere );

    for ( int idx=0; idx<stickintersectpoints_.size(); idx++ )
    {
	const StickIntersectPoint* sip = stickintersectpoints_[idx];
	Geometry::FaultStickSet* fss =
			    emfault_->geometry().sectionGeometry( sip->sid_ );
	if ( !fss ) continue;

	if ( fss->isStickSelected( sip->sticknr_ ) )
	    groupidx = 1;

	knotmarkersets_[groupidx]->addPos( sip->pos_, false );
    }

    mForceDrawMarkerSet( knotmarkersets_ );

    if ( !showmanipulator_ || !stickselectmode_ )
	return;

    PtrMan<EM::EMObjectIterator> iter = emfault_->geometry().createIterator(-1);

    while ( true )
    {
	const EM::PosID pid = iter->next();
	if ( pid.objectID() == -1 )
	    break;

	const EM::SectionID sid = pid.sectionID();
	const int sticknr = pid.getRowCol().row();
	Geometry::FaultStickSet* fs = emfault_->geometry().sectionGeometry(sid);
	if ( !fs || fs->isStickHidden(sticknr,mSceneIdx) )
	    continue;

	groupidx = fs->isStickSelected( sticknr ) ? 1 : 0;
	const MarkerStyle3D& style = emfault_->getPosAttrMarkerStyle(0);
	knotmarkersets_[groupidx]->setMarkerStyle( style );
	knotmarkersets_[groupidx]->addPos( emfault_->getPos( pid ), false );
    }
    mForceDrawMarkerSet( knotmarkersets_ );
}


bool FaultDisplay::coincidesWith2DLine( const Geometry::FaultStickSurface& fss,
					int sticknr ) const
{
    RowCol rc( sticknr, 0 );
    const StepInterval<int> rowrg = fss.rowRange();
    if ( !scene_ || !rowrg.includes(sticknr,false) ||
	 rowrg.snap(sticknr)!=sticknr )
	return false;

    for ( int idx=0; idx<scene_->size(); idx++ )
    {
	visBase::DataObject* dataobj = scene_->getObject( idx );
	mDynamicCastGet( Seis2DDisplay*, s2dd, dataobj );
	if ( !s2dd || !s2dd->isOn() )
	    continue;

	const float onestepdist =
	    mCast( float, Coord3(1,1,mZScale()).dot(
		    s3dgeom_->oneStepTranslation(Coord3(0,0,1)) ) );

	const StepInterval<int> colrg = fss.colRange( rc.row() );
	for ( rc.col()=colrg.start; rc.col()<=colrg.stop; rc.col()+=colrg.step )
	{
	    Coord3 pos = fss.getKnot(rc);
	    if ( displaytransform_ )
		displaytransform_->transform( pos );

	    if ( s2dd->calcDist( pos ) <= 0.5*onestepdist )
		return true;
	}
    }
    return false;
}


bool FaultDisplay::coincidesWithPlane(
			const Geometry::FaultStickSurface& fss, int sticknr,
			TypeSet<Coord3>& intersectpoints ) const
{
    bool res = false;
    RowCol rc( sticknr, 0 );
    const StepInterval<int> rowrg = fss.rowRange();
    if ( !scene_ || !rowrg.includes(sticknr,false) ||
	  rowrg.snap(sticknr)!=sticknr )
	return res;

    for ( int idx=0; idx<scene_->size(); idx++ )
    {
	visBase::DataObject* dataobj = scene_->getObject( idx );
	mDynamicCastGet( PlaneDataDisplay*, plane, dataobj );
	if ( !plane || !plane->isOn() )
	    continue;

	const Coord3 vec1 = fss.getEditPlaneNormal(sticknr).normalize();
	const Coord3 vec2 = plane->getNormal(Coord3()).normalize();
	const bool coincidemode = fabs(vec1.dot(vec2)) > 0.5;

	const Coord3 planenormal = plane->getNormal( Coord3::udf() );
	const float onestepdist =
	    mCast( float, Coord3(1,1,mZScale()).dot(
		    s3dgeom_->oneStepTranslation( planenormal ) ) );

	float prevdist=-1;
	Coord3 prevpos;

	const StepInterval<int> colrg = fss.colRange( rc.row() );
	for ( rc.col()=colrg.start; rc.col()<=colrg.stop; rc.col()+=colrg.step )
	{
	    Coord3 curpos = fss.getKnot(rc);
	    if ( displaytransform_ )
		displaytransform_->transform( curpos );

	    const float curdist = plane->calcDist( curpos );
	    if ( curdist <= 0.5*onestepdist )
	    {
		res = res || coincidemode;
		intersectpoints += curpos;
	    }
	    else if ( rc.col() != colrg.start )
	    {
		const float frac = prevdist / (prevdist+curdist);
		Coord3 interpos = (1-frac)*prevpos + frac*curpos;
		if ( plane->calcDist(interpos) <= 0.5*onestepdist )
		{
		    if ( prevdist <= 0.5*onestepdist )
			intersectpoints.removeSingle(intersectpoints.size()-1);

		    res = res || coincidemode;
		    intersectpoints += interpos;
		}
	    }

	    prevdist = curdist;
	    prevpos = curpos;
	}
    }
    return res;
}


void FaultDisplay::updateStickHiding()
{
    if ( !emfault_ || !faulteditor_ )
	return;

    NotifyStopper ns( faulteditor_->editpositionchange );
    deepErase( stickintersectpoints_ );

    for ( int sidx=0; sidx<emfault_->nrSections(); sidx++ )
    {
	EM::SectionID sid = emfault_->sectionID( sidx );
	mDynamicCastGet( Geometry::FaultStickSurface*, fss,
			 emfault_->sectionGeometry( sid ) );
	if ( !fss || fss->isEmpty() )
	    continue;

	RowCol rc;
	const StepInterval<int> rowrg = fss->rowRange();
	for ( rc.row()=rowrg.start; rc.row()<=rowrg.stop; rc.row()+=rowrg.step )
	{
	    TypeSet<Coord3> intersectpoints;
	    fss->hideStick( rc.row(), true, mSceneIdx );

	    if ( !areIntersectionsDisplayed() ||
		 coincidesWith2DLine(*fss, rc.row()) ||
		 coincidesWithPlane(*fss, rc.row(), intersectpoints) )
	    {
		fss->hideStick( rc.row(), false, mSceneIdx );
		continue;
	    }

	    for (  int idx=0; idx<intersectpoints.size(); idx++ )
	    {
		StickIntersectPoint* sip = new StickIntersectPoint();
		sip->sid_ = sid;
		sip->sticknr_ = rc.row();
		sip->pos_ = intersectpoints[idx];
		if ( displaytransform_ )
		    displaytransform_->transformBack( sip->pos_ );

		stickintersectpoints_ += sip;
	    }
	}
    }
}


const LineStyle* FaultDisplay::lineStyle() const
{ return &drawstyle_->lineStyle(); }


void FaultDisplay::setLineStyle( const LineStyle& lst )
{
    if ( lineStyle()->width_<0 || lst.width_<0 )
    {
	drawstyle_->setLineStyle( lst );
	scene_->objectMoved( 0 );
    }
    else
	drawstyle_->setLineStyle( lst );

    updateDisplay();
}


void FaultDisplay::setLineRadius( visBase::GeomIndexedShape* shape )
{
    const bool islinesolid = lineStyle()->type_ == LineStyle::Solid;
    const float linewidth = islinesolid ? 0.5f*lineStyle()->width_ : -1.0f;

    LineStyle lnstyle( *lineStyle() ) ;
    lnstyle.width_ = (int)( 2*linewidth );

    if ( shape )
	shape->setLineStyle( lnstyle );

    int width = (int)mMAX(linewidth+3.5f, 1.0f);
    activestickmarker_->setLineStyle(LineStyle(LineStyle::Solid, width) );
}


void FaultDisplay::getMousePosInfo( const visBase::EventInfo& eventinfo,
				    Coord3& pos, BufferString& val,
				    BufferString& info ) const
{
    info = ""; val = "";
    if ( !emfault_ ) return;

    info = "Fault: "; info.add( emfault_->name() );
}


int FaultDisplay::addDataPack( const DataPointSet& dpset ) const
{
    DataPackMgr& dpman = DPM( DataPackMgr::SurfID() );
    DataPointSet* newdpset = new DataPointSet( dpset );
    newdpset->setName( dpset.name() );
    dpman.add( newdpset );
    return newdpset->id();
}


bool FaultDisplay::setDataPackID( int attrib, DataPack::ID dpid,
				  TaskRunner* taskr )
{
    if ( !datapackids_.validIdx(attrib) )
	return false;

    DataPackMgr& dpman = DPM( DataPackMgr::SurfID() );
    const DataPack* datapack = dpman.obtain( dpid );
    if ( !datapack )
	return false;

    DataPack::ID oldid = datapackids_[attrib];
    datapackids_[attrib] = dpid;
    dpman.release( oldid );
    return true;
}


DataPack::ID FaultDisplay::getDataPackID( int attrib ) const
{
    return datapackids_[attrib];
}


void FaultDisplay::matChangeCB(CallBacker*)
{
    if ( paneldisplay_ )
	paneldisplay_->updateMaterialFrom( getMaterial() );
}


void FaultDisplay::setPixelDensity( float dpi )
{
    MultiTextureSurveyObject::setPixelDensity( dpi );

    if ( paneldisplay_ )
	paneldisplay_->setPixelDensity( dpi );
    if ( stickdisplay_ )
	stickdisplay_->setPixelDensity( dpi );

    if ( intersectiondisplay_ )
	intersectiondisplay_->setPixelDensity( dpi );

    if ( activestickmarker_ )
	activestickmarker_->setPixelDensity( dpi );

    for ( int idx=0; idx<knotmarkersets_.size(); idx++ )
	knotmarkersets_[idx]->setPixelDensity( dpi );
}


void FaultDisplay::enableAttrib( int attrib, bool yn )
{
    MultiTextureSurveyObject::enableAttrib( attrib, yn );
    updateSingleColor();
}


} // namespace visSurvey

