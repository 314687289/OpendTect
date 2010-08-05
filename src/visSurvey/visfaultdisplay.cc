/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: visfaultdisplay.cc,v 1.60 2010-08-05 14:21:08 cvsjaap Exp $";

#include "visfaultdisplay.h"

#include "arrayndimpl.h"
#include "binidsurface.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "emeditor.h"
#include "emfault3d.h"
#include "emmanager.h"
#include "executor.h"
#include "explfaultsticksurface.h"
#include "explplaneintersection.h"
#include "faultsticksurface.h"
#include "faulteditor.h"
#include "faulthorintersect.h"
#include "iopar.h"
#include "mpeengine.h"
#include "posvecdataset.h"
#include "survinfo.h"
#include "undo.h"
#include "viscoord.h"
#include "visdragger.h"
#include "visevent.h"
#include "viscolortab.h"
#include "visgeomindexedshape.h"
#include "vishorizondisplay.h"
#include "vishorizonsection.h"
#include "vismaterial.h"
#include "vismarker.h"
#include "vismultitexture2.h"
#include "vismpeeditor.h"
#include "vispickstyle.h"
#include "visplanedatadisplay.h"
#include "vispolyline.h"
#include "vispolygonselection.h"
#include "visshapehints.h"
#include "visseis2ddisplay.h"
#include "vistransform.h"

mCreateFactoryEntry( visSurvey::FaultDisplay );

namespace visSurvey
{

FaultDisplay::FaultDisplay()
    : MultiTextureSurveyObject( false )
    , emfault_( 0 )
    , activestickmarker_( visBase::IndexedPolyLine3D::create() )
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
    , activestick_( mUdf(int) )
    , shapehints_( visBase::ShapeHints::create() )
    , activestickmarkerpickstyle_( visBase::PickStyle::create() )
    , showmanipulator_( false )
    , colorchange( this )
    , displaymodechange( this )
    , usestexture_( false )
    , displaysticks_( false )
    , displaypanels_( true )
    , stickselectmode_( false )
    , displayintersections_( false )
    , displayhorintersections_( false )
{
    activestickmarkerpickstyle_->ref();
    activestickmarkerpickstyle_->setStyle( visBase::PickStyle::Unpickable );

    activestickmarker_->ref();
    activestickmarker_->setRadius( 1.2, true );
    if ( !activestickmarker_->getMaterial() )
	activestickmarker_->setMaterial( visBase::Material::create() );
    activestickmarker_->insertNode(
	    activestickmarkerpickstyle_->getInventorNode() );
    insertChild( childIndex(texture_->getInventorNode() ),
		 activestickmarker_->getInventorNode() );

    getMaterial()->setAmbience( 0.2 );
    shapehints_->ref();
    addChild( shapehints_->getInventorNode() );
    shapehints_->setVertexOrder( visBase::ShapeHints::CounterClockWise );

    for ( int idx=0; idx<2; idx++ )
    {
	visBase::DataObjectGroup* group=visBase::DataObjectGroup::create();
	group->ref();
	addChild( group->getInventorNode() );
	knotmarkers_ += group;
	visBase::Material* knotmat = visBase::Material::create();
	group->addObject( knotmat );
	if ( idx )
	    knotmat->setColor( Color(0,255,0) );
	else
	    knotmat->setColor( Color(255,255,255) );
    }
}


FaultDisplay::~FaultDisplay()
{
    setSceneEventCatcher( 0 );
    if ( viseditor_ ) viseditor_->unRef();

    if ( emfault_ ) MPE::engine().removeEditor( emfault_->id() );
    if ( faulteditor_ ) faulteditor_->unRef();
    faulteditor_ = 0;

    if ( paneldisplay_ )
	paneldisplay_->unRef();

    if ( stickdisplay_ )
	stickdisplay_->unRef();

    if ( intersectiondisplay_ )
	intersectiondisplay_->unRef();

    while ( horintersections_.size() )
    {
	horintersections_[0]->unRef();
	horintersections_.remove(0);
    }

    deepErase( horshapes_ );
    delete explicitpanels_;
    delete explicitsticks_;
    delete explicitintersections_;

    if ( emfault_ )
    {
	emfault_->change.remove( mCB(this,FaultDisplay,emChangeCB) );
       	emfault_->unRef();
	emfault_ = 0;
    }

    if ( displaytransform_ ) displaytransform_->unRef();
    shapehints_->unRef();

    activestickmarker_->unRef();
    activestickmarkerpickstyle_->unRef();

    for ( int idx=knotmarkers_.size()-1; idx>=0; idx-- )
    {
	removeChild( knotmarkers_[idx]->getInventorNode() );
	knotmarkers_[idx]->unRef();
	knotmarkers_.remove( idx );
    }
}


void FaultDisplay::setSceneEventCatcher( visBase::EventCatcher* vec )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove( mCB(this,FaultDisplay,mouseCB) );
	eventcatcher_->unRef();
    }

    eventcatcher_ = vec;
    
    if ( eventcatcher_ )
    {
	eventcatcher_->ref();
	eventcatcher_->eventhappened.notify( mCB(this,FaultDisplay,mouseCB) );
    }

    if ( viseditor_ ) viseditor_->setSceneEventCatcher( eventcatcher_ );
}


EM::ObjectID FaultDisplay::getEMID() const
{ return emfault_ ? emfault_->id() : -1; }


#define mErrRet(s) { errmsg_ = s; return false; }

bool FaultDisplay::setEMID( const EM::ObjectID& emid )
{
    if ( emfault_ )
    {
	emfault_->change.remove( mCB(this,FaultDisplay,emChangeCB) );
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
    emfault_->change.notify( mCB(this,FaultDisplay,emChangeCB) );
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
	paneldisplay_->setRightHandSystem( righthandsystem_ );
	addChild( paneldisplay_->getInventorNode() );
    }

    if ( !intersectiondisplay_ )
    {
	intersectiondisplay_ = visBase::GeomIndexedShape::create();
	intersectiondisplay_->ref();
	intersectiondisplay_->setDisplayTransformation( displaytransform_ );
	intersectiondisplay_->setMaterial( 0 );
	intersectiondisplay_->setSelectable( false );
	intersectiondisplay_->setRightHandSystem( righthandsystem_ );
	insertChild( childIndex(texture_->getInventorNode() ),
		     intersectiondisplay_->getInventorNode() );
	intersectiondisplay_->turnOn( false );
    }

    if ( !stickdisplay_ )
    {
	stickdisplay_ = visBase::GeomIndexedShape::create();
	stickdisplay_->ref();
	stickdisplay_->setDisplayTransformation( displaytransform_ );
	if ( !stickdisplay_->getMaterial() )
	    stickdisplay_->setMaterial( visBase::Material::create() );
	stickdisplay_->setSelectable( false );
	stickdisplay_->setRightHandSystem( righthandsystem_ );
	insertChild( childIndex(texture_->getInventorNode() ),
		     stickdisplay_->getInventorNode() );
    }

    if ( !explicitpanels_ )
    {
	const float zscale = scene_
	    ? scene_->getZScale() *scene_->getZStretch()
	    : SI().zScale();

	mTryAlloc( explicitpanels_,Geometry::ExplFaultStickSurface(0,zscale));
	explicitpanels_->display( false, true );
	explicitpanels_->setMaximumTextureSize( texture_->getMaxTextureSize() );
	explicitpanels_->setTexturePowerOfTwo( true );
	explicitpanels_->setTextureSampling(
		BinIDValue( BinID(SI().inlRange(true).step,
				  SI().crlRange(true).step),
				  SI().zStep() ) );

	mTryAlloc( explicitsticks_,Geometry::ExplFaultStickSurface(0,zscale) );
	explicitsticks_->display( true, false );
	explicitsticks_->setMaximumTextureSize( texture_->getMaxTextureSize() );
	explicitsticks_->setTexturePowerOfTwo( true );
	explicitsticks_->setTextureSampling(
		BinIDValue( BinID(SI().inlRange(true).step,
				  SI().crlRange(true).step),
				  SI().zStep() ) );

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
	insertChild( childIndex(texture_->getInventorNode() ),
		viseditor_->getInventorNode() );
    }

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, true );
    mDynamicCastGet( MPE::FaultEditor*, fe, editor.ptr() );
    faulteditor_ = fe;
    if ( faulteditor_ ) faulteditor_->ref();

    viseditor_->setEditor( faulteditor_ );
    
    displaysticks_ = emfault_->isEmpty();

    nontexturecol_ = emfault_->preferredColor();
    updateSingleColor();
    updateDisplay();
    updateManipulator();

    return true;
}


void FaultDisplay::removeSelection( const Selector<Coord3>& selector,
	TaskRunner* tr )
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
    if ( emfault_ ) emfault_->setPreferredColor(nc);
    else
    {
	nontexturecol_ = nc;
	updateSingleColor();
    }
}


void FaultDisplay::updateSingleColor()
{
    const bool usesinglecolor = !showingTexture();

    const Color prevcol = getMaterial()->getColor();
    const Color newcol = usesinglecolor ? nontexturecol_*0.8 : Color::White();
    if ( newcol==prevcol )
	return;

    getMaterial()->setColor( newcol );
    activestickmarker_->getMaterial()->setColor( nontexturecol_ );

    for ( int idx=0; idx<horintersections_.size(); idx++ )
	horintersections_[idx]->getMaterial()->setColor( nontexturecol_ );

    if ( stickdisplay_ )
	stickdisplay_->getMaterial()->setColor( nontexturecol_ );

    texture_->turnOn( !usesinglecolor );
    colorchange.trigger();
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
    const Attrib::SelSpec as( "", Attrib::SelSpec::cNoAttrib(), false, "" );
    setSelSpec( attrib, as );

    texture_->getColorTab( attrib ).setAutoScale( true );
    texture_->getColorTab( attrib ).setClipRate( 0 );
    texture_->getColorTab( attrib ).setSymMidval( mUdf(float) );

    TypeSet<DataPointSet::DataRow> pts; 
    BufferStringSet nms; 
    DataPointSet positions( pts, nms, false, true ); 
    getRandomPos( positions, 0 ); 

    if ( !positions.size() ) return;

    setRandomPosData( attrib, &positions, 0 );
}


bool FaultDisplay::usesTexture() const
{ return usestexture_; }


bool FaultDisplay::showingTexture() const
{ return validtexture_ && usestexture_; }


NotifierAccess* FaultDisplay::materialChange()
{ return &getMaterial()->change; }


Color FaultDisplay::getColor() const
{ return getMaterial()->getColor(); }


void FaultDisplay::updatePanelDisplay()
{
    if ( paneldisplay_ )
    {
	const bool dodisplay = arePanelsDisplayed() &&
			       !areIntersectionsDisplayed() &&
			       !areHorizonIntersectionsDisplayed();
	if ( dodisplay )
	    paneldisplay_->touch( false );

	paneldisplay_->turnOn( dodisplay );
    }
}


void FaultDisplay::updateStickDisplay()
{
    if ( stickdisplay_ )
    {
	bool dodisplay = areSticksDisplayed();
	if ( arePanelsDisplayedInFull() && emfault_->nrSections() )
	{
	    const EM::SectionID sid = emfault_->sectionID( 0 );
	    if ( emfault_->geometry().nrSticks(sid) == 1 )
		dodisplay = true;
	}

	if ( dodisplay )
	    stickdisplay_->touch( false );

	stickdisplay_->turnOn( dodisplay );
    }

    updateKnotMarkers();
    updateEditorMarkers();	
}


void FaultDisplay::updateIntersectionDisplay()
{
    if ( intersectiondisplay_ )
    {
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
	const bool dodisplay = areHorizonIntersectionsDisplayed() &&
			       arePanelsDisplayed(); 
	if ( dodisplay )
	    horintersections_[idx]->touch( false );

	horintersections_[idx]->turnOn( dodisplay );
    }
}


void FaultDisplay::updateDisplay()
{
    updatePanelDisplay();
    updateStickDisplay();
    updateIntersectionDisplay();
    updateHorizonIntersectionDisplay();
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


void FaultDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visSurvey::MultiTextureSurveyObject::fillPar( par, saveids );
    par.set( sKeyEarthModelID(), getMultiID() );
}


int FaultDisplay::usePar( const IOPar& par )
{
    int res = visSurvey::MultiTextureSurveyObject::usePar( par );
    if ( res!=1 ) return res;

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
    return 1;
}


void FaultDisplay::setDisplayTransformation(visBase::Transformation* nt)
{
    if ( paneldisplay_ ) paneldisplay_->setDisplayTransformation( nt );
    if ( stickdisplay_ ) stickdisplay_->setDisplayTransformation( nt );
    if ( intersectiondisplay_ )
	intersectiondisplay_->setDisplayTransformation( nt );
    for ( int idx=0; idx<horintersections_.size(); idx++ )
	horintersections_[idx]->setDisplayTransformation( nt );

    if ( viseditor_ ) viseditor_->setDisplayTransformation( nt );
    activestickmarker_->setDisplayTransformation( nt );

    for ( int idx=0; idx<knotmarkers_.size(); idx++ )
	knotmarkers_[idx]->setDisplayTransformation( nt );

    if ( displaytransform_ ) displaytransform_->unRef();
    displaytransform_ = nt;
    if ( displaytransform_ ) displaytransform_->ref();
}


void FaultDisplay::setRightHandSystem(bool yn)
{
    visBase::VisualObjectImpl::setRightHandSystem( yn );
    if ( paneldisplay_ ) paneldisplay_->setRightHandSystem( yn );
    if ( stickdisplay_ ) stickdisplay_->setRightHandSystem( yn );
    if ( intersectiondisplay_ ) intersectiondisplay_->setRightHandSystem( yn );
    for ( int idx=0; idx<horintersections_.size(); idx++ )
	horintersections_[idx]->setRightHandSystem( yn );
}


visBase::Transformation* FaultDisplay::getDisplayTransformation()
{ return displaytransform_; }


Coord3 FaultDisplay::disp2world( const Coord3& displaypos ) const
{
    Coord3 pos = displaypos;
    if ( pos.isDefined() )
    {
	if ( scene_ )
	    pos = scene_->getZScaleTransform()->transformBack( pos );
	if ( displaytransform_ )
	    pos = displaytransform_->transformBack( pos );
    }
    return pos;
}


#define mZScale() \
    ( scene_ ? scene_->getZScale()*scene_->getZStretch() : SI().zScale() )

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
   
    CubeSampling mouseplanecs; 
    mouseplanecs.setEmpty();
    Coord3 editnormal = Coord3::udf();

    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	const int visid = eventinfo.pickedobjids[idx];
	visBase::DataObject* dataobj = visBase::DM().getObject( visid );

	mDynamicCastGet( visSurvey::PlaneDataDisplay*, plane, dataobj );
	if ( plane )
	{
	    mouseplanecs = plane->getCubeSampling();
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
    faulteditor_->getInteractionInfo( makenewstick, insertpid, pos,
				      mZScale(), &editnormal );

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
		const int rmstick = RowCol(pid.subID()).row;

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
	    : RowCol(insertpid.subID()).row;

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


void FaultDisplay::stickSelectCB( CallBacker* cb )
{
    if ( !emfault_ || !isOn() || eventcatcher_->isHandled() || !isSelected() )
	return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);

    ctrldown_ = OD::ctrlKeyboardButton(eventinfo.buttonstate_);

    if ( eventinfo.type!=visBase::MouseClick ||
	 !OD::leftMouseButton(eventinfo.buttonstate_) )
	return;

    EM::PosID pid = EM::PosID::udf();

    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	const int visid = eventinfo.pickedobjids[idx];
	visBase::DataObject* dataobj = visBase::DM().getObject( visid );
	mDynamicCastGet( visBase::Marker*, marker, dataobj );
	if ( marker )
	{
	    PtrMan<EM::EMObjectIterator> iter =
		emfault_->geometry().createIterator(-1);
	    while ( true )
	    {
		pid = iter->next();
		if ( pid.objectID() == -1 )
		    return;

		const Coord3 diff = emfault_->getPos(pid) - marker->centerPos();
		if ( diff.abs() < 1e-4 )
		    break;
	    }
	}
    }

    if ( pid.isUdf() )
	return;

    const int sticknr = RowCol( pid.subID() ).row;
    const bool isselected = !OD::ctrlKeyboardButton( eventinfo.buttonstate_ );

    const int sid = emfault_->sectionID(0);
    Geometry::FaultStickSet* fss = emfault_->geometry().sectionGeometry( sid );

    fss->selectStick( sticknr, isselected );
    updateKnotMarkers();
    eventcatcher_->setHandled();
}


void FaultDisplay::setActiveStick( const EM::PosID& pid )
{
    const int sticknr = pid.isUdf() ? mUdf(int) : RowCol(pid.subID()).row;
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
	    if ( RowCol(cbdata.pid0.subID()).row==activestick_ )
		updateActiveStickMarker();
	}
	else
	    updateActiveStickMarker();
    }
    else if ( cbdata.event==EM::EMObjectCallbackData::PrefColorChange )
    {
	nontexturecol_ = emfault_->preferredColor();
	updateSingleColor();
    }

    updateDisplay();
}


void FaultDisplay::updateActiveStickMarker()
{
    activestickmarker_->removeCoordIndexAfter(-1);
    activestickmarker_->getCoordinates()->removeAfter(-1);

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
    if ( rowrg.isUdf() || !rowrg.includes(activestick_) )
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

    int idx = 0;
    RowCol rc( activestick_, 0 );
    for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col += colrg.step )
    {
	const Coord3 pos = fss->getKnot( rc );
	const int ci = activestickmarker_->getCoordinates()->addPos( pos );
	activestickmarker_->setCoordIndex( idx++, ci );
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


void FaultDisplay::getRandomPos( DataPointSet& dpset, TaskRunner* tr ) const
{
    if ( explicitpanels_ )
    {
	explicitpanels_->getTexturePositions( dpset, tr );
	paneldisplay_->touch( true );
    }
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
    const DataPointSet* dpset, int column, TaskRunner* )
{
    if ( attrib>=nrAttribs() || !dpset || dpset->nrCols()<3 ||
	 !explicitpanels_ )
    {
	validtexture_ = false;
	updateSingleColor();
	return;
    }

    const BinIDValueSet& bidvset = dpset->bivSet();
    RowCol sz = explicitpanels_->getTextureSize();
    mDeclareAndTryAlloc( PtrMan<Array2D<float> >, texturedata,
	    		 Array2DImpl<float>(sz.col,sz.row) );

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
    BinIDValueSet::Pos pos;
    while ( vals.next( pos ) )
    {
	const float* ptr = vals.getVals( pos );
	const float i = ptr[columni];
	const float j = ptr[columnj];
	texturedata->set( mNINT(j), mNINT(i), ptr[column] );
    }

    texture_->setData( attrib, 0, texturedata, true );
    validtexture_ = true;
    usestexture_ = true;
    updateSingleColor();
}


void FaultDisplay::setResolution( int res, TaskRunner* tr )
{
    if ( texture_->canUseShading() )
	return;

    if ( res==resolution_ )
	return;

    resolution_ = res;
    texture_->clearAll();
}


bool FaultDisplay::getCacheValue( int attrib, int version, const Coord3& crd,
	                          float& value ) const
{ return true; }


void FaultDisplay::addCache()
{}

void FaultDisplay::removeCache( int attrib )
{}

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


#define mCreateNewHorIntersection() \
    visBase::GeomIndexedShape* line = visBase::GeomIndexedShape::create();\
    line->ref(); \
    if ( !line->getMaterial() )	\
	line->setMaterial(visBase::Material::create()); \
    if ( idy!=-1 ) \
	line->getMaterial()->setColor(  \
		horintersections_[0]->getMaterial()->getColor() ); \
    line->setDisplayTransformation( displaytransform_ ); \
    line->setSelectable( false ); \
    line->setRightHandSystem( righthandsystem_ ); \
    insertChild( childIndex(texture_->getInventorNode() ), \
		 line->getInventorNode() ); \
    line->turnOn( false ); \
    Geometry::ExplFaultStickSurface* shape = 0; \
    mTryAlloc( shape, Geometry::ExplFaultStickSurface(0,mZScale()) ); \
    shape->display( false, false ); \
    line->setSurface( shape ); \
    shape->setSurface( fss ); \
    Geometry::FaultBinIDSurfaceIntersector it( 0, *surf,  \
	    *emfault_->geometry().sectionGeometry( \
		emfault_->sectionID(0)), *shape->coordList() ); \
    it.setShape( *shape ); \
    it.compute(); \


void FaultDisplay::updateHorizonIntersections( int whichobj, 
	const ObjectSet<const SurveyObject>& objs )
{
    if ( !emfault_ )
	return;
   
    mDynamicCastGet( Geometry::FaultStickSurface*, fss,
		     emfault_->sectionGeometry( emfault_->sectionID(0)) );
    
    ObjectSet<const SurveyObject> usedhors;
    for ( int idx=0; idx<objs.size(); idx++ )
    {
	mDynamicCastGet( const HorizonDisplay*, hor, objs[idx] );
	if ( !hor || !hor->isOn() ) continue;
	usedhors += objs[idx];
    }

    const bool removehor = horobjs_.size() && usedhors.size()<horobjs_.size();
    
    TypeSet<int> usedids;
    for ( int idx=0; idx<usedhors.size(); idx++ )
    {
	mDynamicCastGet( const HorizonDisplay*, hor, usedhors[idx] );

	visSurvey::HorizonDisplay* hd = 
	    const_cast<visSurvey::HorizonDisplay*>( hor ); 
	if ( !hd->getSectionIDs().size() ) continue;
	EM::SectionID sid = hd->getSectionIDs()[0];
	const Geometry::BinIDSurface* surf =
	    hd->getHorizonSection(sid)->getSurface();
	
	const int idy = horobjs_.indexOf( objs[idx] );
	
	if ( idy==-1 )
	{
	    mCreateNewHorIntersection();
	    horintersections_ += line;
	    horshapes_ += shape;
	}
	else 
	{
	    usedids += idy;
	    if( horintersections_.isEmpty() ) continue;
	    horintersections_[idy]->turnOn( false );
	    if ( !removehor )
	    {		
    		mCreateNewHorIntersection();
		horintersections_[idy]->unRef();
    		horintersections_.replace( idy, line );
    		delete horshapes_.replace( idy, shape );
	    }
	}
    }

    if ( removehor )
    {
	ObjectSet<visBase::GeomIndexedShape> usdhorintersections;
	ObjectSet<Geometry::ExplFaultStickSurface> usedhorshapes;
	for ( int idx=0; idx<usedids.size(); idx++ )
	{
	    usdhorintersections += horintersections_[usedids[idx]];
	    usedhorshapes += horshapes_[usedids[idx]];
	}
	
	for ( int idx=0; idx<horintersections_.size(); idx++ )
	{
	    if ( usedids.indexOf(idx)==-1 )
	    {
		horintersections_[idx]->turnOn( false );
		horintersections_[idx]->unRef();
		horintersections_.remove( idx );
		delete horshapes_.remove( idx );
	    }
	}

	horintersections_ = usdhorintersections;
	horshapes_ = usedhorshapes;
    }
	
    for ( int idx=usedids.size()-1; idx>=0; idx-- )
	horobjs_.remove( usedids[idx] );
    horobjs_ = usedhors;
    
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

	const CubeSampling cs = plane->getCubeSampling(true,true,-1);
	const BinID b00 = cs.hrg.start, b11 = cs.hrg.stop;
	BinID b01, b10;

	if ( plane->getOrientation()==PlaneDataDisplay::Timeslice )
	{
	    b01 = BinID( cs.hrg.start.inl, cs.hrg.stop.crl );
	    b10 = BinID( cs.hrg.stop.inl, cs.hrg.start.crl );
	}
	else
	{
	    b01 = b00;
	    b10 = b11;
	}

	const Coord3 c00( SI().transform(b00),cs.zrg.start );
	const Coord3 c01( SI().transform(b01),cs.zrg.stop );
	const Coord3 c11( SI().transform(b11),cs.zrg.stop );
	const Coord3 c10( SI().transform(b10),cs.zrg.start );

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

	    intersectionobjs_.remove( idy );
	    planeids_.remove( idy );
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

    const CallBack cb = mCB( this, FaultDisplay, polygonFinishedCB );
    if ( yn )
	scene_->getPolySelection()->polygonFinished()->notify( cb );
    else
	scene_->getPolySelection()->polygonFinished()->remove( cb );
}


void FaultDisplay::polygonFinishedCB( CallBacker* cb )
{
    if ( !stickselectmode_ || !emfault_ || !scene_ || !displaysticks_ )
	return;

    PtrMan<EM::EMObjectIterator> iter = emfault_->geometry().createIterator(-1);
    while ( true )
    {
	EM::PosID pid = iter->next();
	if ( pid.objectID() == -1 )
	    break;

	if ( !scene_->getPolySelection()->isInside(emfault_->getPos(pid) ) )
	    continue;

	const int sticknr = RowCol( pid.subID() ).row;
	const EM::SectionID sid = pid.sectionID();
	Geometry::FaultStickSet* fss =
	    			 emfault_->geometry().sectionGeometry( sid );
	fss->selectStick( sticknr, !ctrldown_ );
    }

    updateKnotMarkers();
}


bool FaultDisplay::isInStickSelectMode() const
{ return stickselectmode_; }


void FaultDisplay::updateEditorMarkers()
{
    if ( !emfault_ || !viseditor_ || !viseditor_->isOn() )
	return;
    
    PtrMan<EM::EMObjectIterator> iter = emfault_->geometry().createIterator(-1);    while ( true )
    {
	const EM::PosID pid = iter->next();
	if ( pid.objectID() == -1 )
	    break;

	const int sid = pid.sectionID();
	const int sticknr = RowCol( pid.subID() ).row;
	Geometry::FaultStickSet* fs = emfault_->geometry().sectionGeometry(sid);
	viseditor_->turnOnMarker( pid, !fs->isStickHidden(sticknr) );
    }
}


void FaultDisplay::updateKnotMarkers()
{
    for ( int idx=0; idx<knotmarkers_.size(); idx++ )
    {
	while ( knotmarkers_[idx]->size() > 1 )
	    knotmarkers_[idx]->removeObject( 1 );
    }

    if ( !showmanipulator_ || !stickselectmode_ || !areSticksDisplayed() )
	return;

    PtrMan<EM::EMObjectIterator> iter = emfault_->geometry().createIterator(-1);
    while ( true )
    {
	const EM::PosID pid = iter->next();
	if ( pid.objectID() == -1 )
	    break;

	const int sid = pid.sectionID();
	const int sticknr = RowCol( pid.subID() ).row;
	Geometry::FaultStickSet* fs = emfault_->geometry().sectionGeometry(sid);
	if ( fs->isStickHidden(sticknr) )
	    continue;

	const Coord3 pos = emfault_->getPos( pid );

	visBase::Marker* marker = visBase::Marker::create();
	marker->setMarkerStyle( emfault_->getPosAttrMarkerStyle(0) );
	marker->setMaterial(0);
	marker->setDisplayTransformation( displaytransform_ );
	marker->setCenterPos(pos);
	marker->setScreenSize(3);
	const int groupidx = fs->isStickSelected(sticknr) ? 1 : 0;
	knotmarkers_[groupidx]->addObject( marker );
    }
}


bool FaultDisplay::coincidesWith2DLine( const Geometry::FaultStickSurface& fss,
					int sticknr ) const
{
    RowCol rc( sticknr, 0 );
    const StepInterval<int> rowrg = fss.rowRange();
    if ( !scene_ || !rowrg.includes(sticknr) || rowrg.snap(sticknr)!=sticknr )
	return false;

    for ( int idx=0; idx<scene_->size(); idx++ )
    {
	visBase::DataObject* dataobj = scene_->getObject( idx );
	mDynamicCastGet( Seis2DDisplay*, s2dd, dataobj );
	if ( !s2dd || !s2dd->isOn() )
	    continue;

	const float onestepdist = SI().oneStepDistance(Coord3(0,0,1),mZScale());

	const StepInterval<int> colrg = fss.colRange( rc.row );
	for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col+=colrg.step )
	{
	    Coord3 pos = fss.getKnot(rc);
	    if ( displaytransform_ )
		pos = displaytransform_->transform( pos ); 

	    if ( s2dd->calcDist( pos ) <= 0.5*onestepdist )
		return true;
	}
    }
    return false;
}


bool FaultDisplay::coincidesWithPlane( const Geometry::FaultStickSurface& fss,
				       int sticknr ) const
{
    RowCol rc( sticknr, 0 );
    const StepInterval<int> rowrg = fss.rowRange();
    if ( !scene_ || !rowrg.includes(sticknr) || rowrg.snap(sticknr)!=sticknr )
	return false;

    for ( int idx=0; idx<scene_->size(); idx++ )
    {
	visBase::DataObject* dataobj = scene_->getObject( idx );
	mDynamicCastGet( PlaneDataDisplay*, plane, dataobj );
	if ( !plane || !plane->isOn() )
	    continue;

	const Coord3 vec1 = fss.getEditPlaneNormal(sticknr).normalize();
	const Coord3 vec2 = plane->getNormal(Coord3()).normalize();
	if ( fabs(vec1.dot(vec2)) < 0.5 )
	    continue;

	const Coord3 planenormal = plane->getNormal( Coord3::udf() );
	const float onestepdist = SI().oneStepDistance( planenormal,mZScale() );

	float prevdist;
	Coord3 prevpos;

	const StepInterval<int> colrg = fss.colRange( rc.row );
	for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col+=colrg.step )
	{
	    Coord3 curpos = fss.getKnot(rc);
	    if ( displaytransform_ )
		curpos = displaytransform_->transform( curpos );

	    const float curdist = plane->calcDist( curpos );
	    if ( curdist <= 0.5*onestepdist )
		return true;

	    if ( rc.col != colrg.start )
	    {
		const float frac = prevdist / (prevdist+curdist);
		Coord3 interpos = (1-frac)*prevpos + frac*curpos;
		if ( plane->calcDist(interpos) <= 0.5*onestepdist )
		    return true;
	    }

	    prevdist = curdist;
	    prevpos = curpos;
	}
    }
    return false;
}


void FaultDisplay::updateStickHiding()
{
    if ( !emfault_ )
	return;

    for ( int sidx=0; sidx<emfault_->nrSections(); sidx++ )
    {
	int sid = emfault_->sectionID( sidx );
	mDynamicCastGet( Geometry::FaultStickSurface*, fss,
			 emfault_->sectionGeometry( sid ) );
	if ( fss->isEmpty() )
	    continue;

	RowCol rc;
	const StepInterval<int> rowrg = fss->rowRange();

	for ( rc.row=rowrg.start; rc.row<=rowrg.stop; rc.row+=rowrg.step )
	{
	    fss->hideStick( rc.row, areIntersectionsDisplayed() &&
				    !coincidesWithPlane(*fss, rc.row) &&
				    !coincidesWith2DLine(*fss, rc.row) );
	}
    }
}


}; // namespace visSurvey
