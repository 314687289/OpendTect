/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: vispolygonbodydisplay.cc,v 1.10 2010-06-24 12:46:40 cvsjaap Exp $";

#include "vispolygonbodydisplay.h"

#include "empolygonbody.h"
#include "emmanager.h"
#include "executor.h"
#include "explpolygonsurface.h"
#include "explplaneintersection.h"
#include "polygonsurfeditor.h"
#include "iopar.h"
#include "mpeengine.h"
#include "survinfo.h"
#include "undo.h"
#include "viscoord.h"
#include "visevent.h"
#include "visgeomindexedshape.h"
#include "vismaterial.h"
#include "vismpeeditor.h"
#include "vispickstyle.h"
#include "visplanedatadisplay.h"
#include "vispolyline.h"
#include "visshapehints.h"
#include "vistransform.h"

mCreateFactoryEntry( visSurvey::PolygonBodyDisplay );

namespace visSurvey
{

PolygonBodyDisplay::PolygonBodyDisplay()
    : visBase::VisualObjectImpl(true)
    , empolygonsurf_( 0 )
    , bodydisplay_( 0 )
    , explicitbody_( 0 )
    , polygondisplay_( 0 )
    , explicitpolygons_( 0 )
    , intersectiondisplay_( 0 )
    , explicitintersections_( 0 )
    , viseditor_( 0 )
    , polygonsurfeditor_( 0 )
    , eventcatcher_( 0 )
    , displaytransform_( 0 )
    , nearestpolygon_( mUdf(int) )
    , nearestpolygonmarker_( visBase::IndexedPolyLine3D::create() )
    , nearestpolygonmarkerpickstyle_( visBase::PickStyle::create() )
    , shapehints_( visBase::ShapeHints::create() )
    , showmanipulator_( false )
    , displaypolygons_( false )
{
    nearestpolygonmarkerpickstyle_->ref();
    nearestpolygonmarkerpickstyle_->setStyle( visBase::PickStyle::Unpickable );

    nearestpolygonmarker_->ref();
    nearestpolygonmarker_->setRadius( 1, true );
    if ( !nearestpolygonmarker_->getMaterial() )
	nearestpolygonmarker_->setMaterial( visBase::Material::create() );
    nearestpolygonmarker_->insertNode(
	    nearestpolygonmarkerpickstyle_->getInventorNode() );
    addChild( nearestpolygonmarker_->getInventorNode() );

    getMaterial()->setAmbience( 0.2 );
    shapehints_->ref();
    addChild( shapehints_->getInventorNode() );
    shapehints_->setVertexOrder( visBase::ShapeHints::CounterClockWise );
}


PolygonBodyDisplay::~PolygonBodyDisplay()
{
    setSceneEventCatcher( 0 );
   
    if ( viseditor_ ) 
	viseditor_->unRef();

    if ( polygonsurfeditor_ ) 
	polygonsurfeditor_->unRef();
    
    polygonsurfeditor_ = 0;

    if ( empolygonsurf_ )
    {
	MPE::engine().removeEditor( empolygonsurf_->id() );
	empolygonsurf_->change.remove(mCB(this,PolygonBodyDisplay,emChangeCB));
       	empolygonsurf_->unRef();
    }

    if ( bodydisplay_ )
	bodydisplay_->unRef();

    if ( polygondisplay_ )
	polygondisplay_->unRef();

    if ( intersectiondisplay_ )
	intersectiondisplay_->unRef();

    if ( displaytransform_ ) 
	displaytransform_->unRef();

    delete explicitbody_;
    delete explicitpolygons_;
    delete explicitintersections_;

    shapehints_->unRef();
    nearestpolygonmarker_->unRef();
    nearestpolygonmarkerpickstyle_->unRef();
}


void PolygonBodyDisplay::setSceneEventCatcher( visBase::EventCatcher* vec )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(
		mCB(this,PolygonBodyDisplay,mouseCB) );
	eventcatcher_->unRef();
    }

    eventcatcher_ = vec;
    if ( eventcatcher_ )
    {
    	eventcatcher_->ref();
    	eventcatcher_->eventhappened.notify(
		mCB(this,PolygonBodyDisplay,mouseCB) );
    }

    if ( viseditor_ ) 
	viseditor_->setSceneEventCatcher( eventcatcher_ );
}


EM::ObjectID PolygonBodyDisplay::getEMID() const
{ 
    return empolygonsurf_ ? empolygonsurf_->id() : -1; 
}


#define mErrRet(s) { errmsg_ = s; return false; }

bool PolygonBodyDisplay::setEMID( const EM::ObjectID& emid )
{
    if ( empolygonsurf_ )
    {
	empolygonsurf_->change.remove( 
		mCB(this,PolygonBodyDisplay,emChangeCB) );
	empolygonsurf_->unRef();
    }

    if ( polygonsurfeditor_ ) polygonsurfeditor_->unRef();
    if ( viseditor_ ) viseditor_->setEditor( (MPE::ObjectEditor*) 0 );
    empolygonsurf_ = 0;
    polygonsurfeditor_ = 0;

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet( EM::PolygonBody*, emplys, emobject.ptr() );
    if ( !emplys )
    {
	if ( bodydisplay_ ) bodydisplay_->turnOn( false );
	if ( intersectiondisplay_ ) intersectiondisplay_->turnOn( false );
	if ( polygondisplay_ ) polygondisplay_->turnOn( false );
	return false;
    }

    empolygonsurf_ = emplys;
    empolygonsurf_->change.notify( mCB(this,PolygonBodyDisplay,emChangeCB) );
    empolygonsurf_->ref();

    if ( !empolygonsurf_->name().isEmpty() )
	setName( empolygonsurf_->name() );

    if ( !bodydisplay_ )
    {
	bodydisplay_ = visBase::GeomIndexedShape::create();
	bodydisplay_->ref();
	bodydisplay_->setDisplayTransformation( displaytransform_ );
	bodydisplay_->setMaterial( 0 );
	bodydisplay_->setSelectable( false );
	bodydisplay_->setRightHandSystem( righthandsystem_ );
	addChild( bodydisplay_->getInventorNode() );
    }

    if ( !intersectiondisplay_ )
    {
	intersectiondisplay_ = visBase::GeomIndexedShape::create();
	intersectiondisplay_->ref();
	intersectiondisplay_->setDisplayTransformation( displaytransform_ );
	intersectiondisplay_->setMaterial( 0 );
	intersectiondisplay_->setSelectable( false );
	intersectiondisplay_->setRightHandSystem( righthandsystem_ );
	addChild( intersectiondisplay_->getInventorNode() );
	intersectiondisplay_->turnOn( false );
    }

    if ( !polygondisplay_ )
    {
	polygondisplay_ = visBase::GeomIndexedShape::create();
	polygondisplay_->ref();
	polygondisplay_->setDisplayTransformation( displaytransform_ );
	if ( !polygondisplay_->getMaterial() )
	    polygondisplay_->setMaterial( visBase::Material::create() );
	polygondisplay_->setSelectable( false );
	polygondisplay_->setRightHandSystem( righthandsystem_ );
	addChild( polygondisplay_->getInventorNode() );
    }
    
    const float zscale = scene_
	? scene_->getZScale() *scene_->getZStretch()
	: SI().zScale();

    if ( !explicitbody_ )
    {
	mTryAlloc( explicitbody_,Geometry::ExplPolygonSurface(0,zscale));
	explicitbody_->display( false, true );
    }

    if ( !explicitpolygons_ )
    {
	mTryAlloc( explicitpolygons_,Geometry::ExplPolygonSurface(0,zscale) );
	explicitpolygons_->display( true, false );
    }

    if ( !explicitintersections_ )
	mTryAlloc( explicitintersections_, Geometry::ExplPlaneIntersection );

    mDynamicCastGet( Geometry::PolygonSurface*, plgs,
	    empolygonsurf_->sectionGeometry(empolygonsurf_->sectionID(0)) );

    explicitbody_->setSurface( plgs ); 
    bodydisplay_->setSurface( explicitbody_ );

    explicitintersections_->setShape( *explicitbody_ );
    intersectiondisplay_->setSurface( explicitintersections_ );

    explicitpolygons_->setSurface( plgs ); 
    polygondisplay_->setSurface( explicitpolygons_ );

    if ( !viseditor_ )
    {
	viseditor_ = visSurvey::MPEEditor::create();
	viseditor_->ref();
	viseditor_->setSceneEventCatcher( eventcatcher_ );
	viseditor_->setDisplayTransformation( displaytransform_ );
	viseditor_->sower().alternateSowingOrder();
	addChild( viseditor_->getInventorNode() );
    }

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, true );
    mDynamicCastGet( MPE::PolygonBodyEditor*, pe, editor.ptr() );
    polygonsurfeditor_ =  pe;
    if ( polygonsurfeditor_ ) polygonsurfeditor_->ref();

    viseditor_->setEditor( polygonsurfeditor_ );
    bodydisplay_->turnOn( true );
    displaypolygons_ = false;

    nontexturecol_ = empolygonsurf_->preferredColor();
    updateSingleColor();
    updatePolygonDisplay();

    return true;
}


void PolygonBodyDisplay::touchAll( bool yn, bool updatemarker ) 
{
    if ( bodydisplay_ )
    	bodydisplay_->touch( yn );

    if ( polygondisplay_ )
    	polygondisplay_->touch( yn );

    if ( intersectiondisplay_ )
	intersectiondisplay_->touch( yn );

    if ( updatemarker )
	updateNearestPolygonMarker();
}


void PolygonBodyDisplay::removeSelection( const Selector<Coord3>& selector,
	TaskRunner* tr )
{
    if ( polygonsurfeditor_ )
	polygonsurfeditor_->removeSelection( selector );
}


MultiID PolygonBodyDisplay::getMultiID() const
{
    return empolygonsurf_ ? empolygonsurf_->multiID() : MultiID();
}


void PolygonBodyDisplay::setColor( Color nc )
{
    if ( empolygonsurf_ ) 
	empolygonsurf_->setPreferredColor(nc);
    else
    {
	nontexturecol_ = nc;
	updateSingleColor();
    }
}


void PolygonBodyDisplay::updateSingleColor()
{
    const Color prevcol = getMaterial()->getColor();
    const Color newcol = nontexturecol_*0.8;
    if ( newcol==prevcol )
	return;

    getMaterial()->setColor( newcol );
    nearestpolygonmarker_->getMaterial()->setColor( nontexturecol_ );
    if ( polygondisplay_ )
	polygondisplay_->getMaterial()->setColor( nontexturecol_ );
}


NotifierAccess* PolygonBodyDisplay::materialChange()
{ return &getMaterial()->change; }


Color PolygonBodyDisplay::getColor() const
{ return getMaterial()->getColor(); }


void PolygonBodyDisplay::updatePolygonDisplay()
{
    if ( !polygondisplay_ )
	return;

    const EM::SectionID sid = empolygonsurf_->sectionID( 0 );
    const bool dodisplay = displaypolygons_ ||
 	(isBodyDisplayed() && isManipulatorShown()) ||
	(isBodyDisplayed() && empolygonsurf_->geometry().nrPolygons(sid)==1);
    
    polygondisplay_->turnOn( dodisplay );
}


void PolygonBodyDisplay::display( bool polygons, bool body )
{
    displaypolygons_ = polygons;

    if ( nearestpolygonmarker_ )
	nearestpolygonmarker_->turnOn( polygons || body );

    viseditor_->turnOn( (polygons || body) && showmanipulator_ );

    if ( bodydisplay_ )
	bodydisplay_->turnOn( body );

    updatePolygonDisplay();
}


bool PolygonBodyDisplay::arePolygonsDisplayed() const 
{ return displaypolygons_; }


bool PolygonBodyDisplay::isBodyDisplayed() const
{ return bodydisplay_ ? bodydisplay_->isOn() : false; }


void PolygonBodyDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );
    par.set( sKeyEMPolygonSurfID(), getMultiID() );
}


int PolygonBodyDisplay::usePar( const IOPar& par )
{
    int res = visBase::VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    MultiID newmid;
    if ( par.get(sKeyEMPolygonSurfID(),newmid) )
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


void PolygonBodyDisplay::setDisplayTransformation(visBase::Transformation* nt)
{
    if ( bodydisplay_ ) 
	bodydisplay_->setDisplayTransformation( nt );

    if ( polygondisplay_ ) 
	polygondisplay_->setDisplayTransformation( nt );

    if ( intersectiondisplay_ )
	intersectiondisplay_->setDisplayTransformation( nt );

    if ( viseditor_ ) 
	viseditor_->setDisplayTransformation( nt );

    nearestpolygonmarker_->setDisplayTransformation( nt );

    if ( displaytransform_ ) displaytransform_->unRef();
    displaytransform_ = nt;
    if ( displaytransform_ ) displaytransform_->ref();
}


void PolygonBodyDisplay::setRightHandSystem(bool yn)
{
    visBase::VisualObjectImpl::setRightHandSystem( yn );
    if ( bodydisplay_ ) bodydisplay_->setRightHandSystem( yn );
    if ( polygondisplay_ ) polygondisplay_->setRightHandSystem( yn );
    if ( intersectiondisplay_ ) intersectiondisplay_->setRightHandSystem( yn );
}


visBase::Transformation* PolygonBodyDisplay::getDisplayTransformation()
{ return displaytransform_; }


Coord3 PolygonBodyDisplay::disp2world( const Coord3& displaypos ) const
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


void PolygonBodyDisplay::mouseCB( CallBacker* cb )
{
    if ( !empolygonsurf_ || !polygonsurfeditor_ || !viseditor_ || !isOn() || 
	 eventcatcher_->isHandled() || !isSelected() )
	return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);

    polygonsurfeditor_->setSowingPivot(
	    			disp2world(viseditor_->sower().pivotPos()) );
    if ( viseditor_->sower().accept(eventinfo) )
	return;
   
    CubeSampling mouseplanecs; 
    mouseplanecs.setEmpty();
    EM::PosID mousepid( EM::PosID::udf() );
    bool mouseondragger = false;

    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	const int visid = eventinfo.pickedobjids[idx];
	visBase::DataObject* dataobj = visBase::DM().getObject( visid );

	mDynamicCastGet( visSurvey::PlaneDataDisplay*, plane, dataobj );
	if ( plane )
	{
	    mouseplanecs = plane->getCubeSampling();
	    break;
	}
    }

    if ( locked_ )
	return;

    Coord3 pos = disp2world( eventinfo.displaypickedpos );

    EM::PosID nearestpid0, nearestpid1, insertpid;
    const float zscale = scene_
	? scene_->getZScale() *scene_->getZStretch()
	: SI().zScale();

    polygonsurfeditor_->getInteractionInfo( nearestpid0, nearestpid1, insertpid,					    pos, zscale );

    const int nearestpolygon = 
	nearestpid0.isUdf() ? mUdf(int) : RowCol(insertpid.subID()).row;

    if ( nearestpolygon_!=nearestpolygon )
    {
	nearestpolygon_ = nearestpolygon;
	updateNearestPolygonMarker();
    }

    if ( !pos.isDefined() )
	return;

    if ( eventinfo.type!=visBase::MouseClick )
	return;

    const EM::PosID pid = viseditor_->mouseClickDragger(eventinfo.pickedobjids);
    if ( !pid.isUdf() )
    {
	if ( OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
	     !OD::altKeyboardButton(eventinfo.buttonstate_) &&
	     !OD::shiftKeyboardButton(eventinfo.buttonstate_) &&
	     OD::leftMouseButton(eventinfo.buttonstate_) )
	{
	    eventcatcher_->setHandled();
	    if ( !eventinfo.pressed )
	    {
		const int removepolygon = RowCol(pid.subID()).row;
		const bool res = empolygonsurf_->geometry().nrKnots( 
			pid.sectionID(),removepolygon)==1  ? 
		    empolygonsurf_->geometry().removePolygon( 
			    pid.sectionID(), removepolygon, true )
		    : empolygonsurf_->geometry().removeKnot( 
			    pid.sectionID(), pid.subID(), true );

		polygonsurfeditor_->setLastClicked( EM::PosID::udf() );

		if ( res && !viseditor_->sower().moreToSow() )
		{
		    EM::EMM().undo().setUserInteractionEnd(
			    EM::EMM().undo().currentEventID() );
		    touchAll( false );
		}
	    }
	}

	return;
    }

    if ( OD::altKeyboardButton(eventinfo.buttonstate_) ||
	 !OD::leftMouseButton(eventinfo.buttonstate_) )
	return;

    const Color& prefcol = empolygonsurf_->preferredColor();
    if ( viseditor_->sower().activate(prefcol, eventinfo) )
	return;

    if ( eventinfo.pressed || insertpid.isUdf() )
	return;

    if ( nearestpid0.isUdf() )
    {
	Coord3 editnormal(0,0,1);
	if ( mouseplanecs.isEmpty() )
	    return;

	if ( mouseplanecs.defaultDir()==CubeSampling::Inl )
	    editnormal = Coord3( SI().transform(BinID(1,0))-
		    SI().transform(BinID(0,0)), 0 );
	else if ( mouseplanecs.defaultDir()==CubeSampling::Crl ) 
    	    editnormal = Coord3( SI().transform(BinID(0,1))-
		    SI().transform(BinID(0,0)), 0 );

	if ( empolygonsurf_->geometry().insertPolygon( insertpid.sectionID(),
	       RowCol(insertpid.subID()).row, 0, pos, editnormal, true ) )
	{
	    polygonsurfeditor_->setLastClicked( insertpid );
	    if ( !viseditor_->sower().moreToSow() )
	    {
		EM::EMM().undo().setUserInteractionEnd(
		    EM::EMM().undo().currentEventID() );

		touchAll( false );
		polygonsurfeditor_->editpositionchange.trigger();
	    }
	}
    }
    else
    {
	if ( empolygonsurf_->geometry().insertKnot( insertpid.sectionID(),
		insertpid.subID(), pos, true ) )
	{
	    polygonsurfeditor_->setLastClicked( insertpid );
	    if ( !viseditor_->sower().moreToSow() )
	    {
		EM::EMM().undo().setUserInteractionEnd( 
		    EM::EMM().undo().currentEventID() );
		polygonsurfeditor_->editpositionchange.trigger();
	    }
	}
    }

    eventcatcher_->setHandled();
}


void PolygonBodyDisplay::emChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);

    if ( cbdata.event==EM::EMObjectCallbackData::BurstAlert ||
	 cbdata.event==EM::EMObjectCallbackData::SectionChange ||
	 cbdata.event==EM::EMObjectCallbackData::PositionChange )
    {
	updateSingleColor();
	if ( cbdata.event==EM::EMObjectCallbackData::PositionChange )
	{
	     if ( RowCol(cbdata.pid0.subID()).row==nearestpolygon_ )
		updateNearestPolygonMarker();
	}
	else
	    updateNearestPolygonMarker();

	touchAll( false );
    }
    else if ( cbdata.event==EM::EMObjectCallbackData::PrefColorChange )
    {
	nontexturecol_ = empolygonsurf_->preferredColor();
	updateSingleColor();
    }
}


void PolygonBodyDisplay::updateNearestPolygonMarker()
{
    if ( mIsUdf(nearestpolygon_) || !isManipulatorShown() )
	nearestpolygonmarker_->turnOn( false );
    else
    {
	mDynamicCastGet( Geometry::PolygonSurface*, plgs,
		empolygonsurf_->sectionGeometry(empolygonsurf_->sectionID(0)));

	const StepInterval<int> rowrg = plgs->rowRange();
	if ( rowrg.isUdf() || !rowrg.includes(nearestpolygon_) )
	{
	    nearestpolygonmarker_->turnOn( false );
	    return;
	}

	const StepInterval<int> colrg = plgs->colRange( nearestpolygon_ );
	if ( colrg.isUdf() || colrg.start==colrg.stop )
	{
	    nearestpolygonmarker_->turnOn( false );
	    return;
	}

	nearestpolygonmarker_->removeCoordIndexAfter(-1);
	nearestpolygonmarker_->getCoordinates()->removeAfter(-1);

	int markeridx = 0;
	TypeSet<Coord3> knots;
	const float zfactor = scene_ ? scene_->getZScale() : SI().zScale();
	plgs->getCubicBezierCurve( nearestpolygon_, knots, zfactor  );
	for ( int idy=0; idy<knots.size(); idy++ )
	{
	    const int ci = 
		nearestpolygonmarker_->getCoordinates()->addPos(knots[idy]);
	    nearestpolygonmarker_->setCoordIndex( markeridx++, ci );
	}

	if ( markeridx>2 )
	{
	    const int ci = 
		nearestpolygonmarker_->getCoordinates()->addPos(knots[0]);
	    nearestpolygonmarker_->setCoordIndex( markeridx++, ci );
	}
	
	nearestpolygonmarker_->turnOn( true );
    }
}


void PolygonBodyDisplay::showManipulator( bool yn )
{
    if ( showmanipulator_==yn )
	return;

    showmanipulator_ = yn;
    updatePolygonDisplay();
    updateManipulator();
}


void PolygonBodyDisplay::updateManipulator()
{
    const bool show = showmanipulator_ &&
    		      (arePolygonsDisplayed() || isBodyDisplayed() );
    if ( viseditor_ ) 
	viseditor_->turnOn( show );

    if ( nearestpolygonmarker_ ) 
	nearestpolygonmarker_->turnOn( show );
}


bool  PolygonBodyDisplay::isManipulatorShown() const
{ return showmanipulator_; }


void PolygonBodyDisplay::displayIntersections( bool yn )
{
    if ( intersectiondisplay_ )
    {
	if ( yn ) intersectiondisplay_->touch( false );
	intersectiondisplay_->turnOn( yn );
    }
}


bool PolygonBodyDisplay::areIntersectionsDisplayed() const
{ return intersectiondisplay_ ? intersectiondisplay_->isOn() : false; }


void PolygonBodyDisplay::otherObjectsMoved( 
	const ObjectSet<const SurveyObject>& objs, int whichobj )
{
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
	    explicitintersections_->setPlane(planeids_[idy], normal, positions);
	    planeids += planeids_[idy];

	    intersectionobjs_.remove( idy );
	    planeids_.remove( idy );
	}
    }

    for ( int idx=planeids_.size()-1; idx>=0; idx-- )
	explicitintersections_->removePlane( planeids_[idx] );

    intersectionobjs_ = usedobjects;
    planeids_ = planeids;

    if ( areIntersectionsDisplayed() ) 
	intersectiondisplay_->touch( false );
}


void PolygonBodyDisplay::setNewIntersectingPolygon( const Coord3& normal, 
						    const Coord3& c00 )
{ 
    //To be used later.
    if ( !empolygonsurf_ )
	return;

    EM::PolygonBodyGeometry& geo = empolygonsurf_->geometry();
    const EM::SectionID sid = geo.sectionID( 0 );
    const int nrplgs = geo.nrPolygons(sid);
    if ( !nrplgs )
	return;

    const float zfactor = scene_ ? scene_->getZScale() : SI().zScale();
    const Coord3 scale( 1, 1, zfactor  );
    const Coord3 invscale( 1, 1, 1.0/zfactor  );
    const Plane3 newplane( normal, c00.scaleBy(scale), false );
    TypeSet<Coord3> intersections;

    for ( int plg=0; plg<nrplgs; plg++ )
    {
	TypeSet<Coord3> knots;
	if ( geo.sectionGeometry(sid) )
	    geo.sectionGeometry(sid)->getCubicBezierCurve(plg, knots, scale.z);
	
	const int nrknots = knots.size();
	if ( !nrknots )
	    continue;

	const Coord3 plgnormal = geo.getPolygonNormal( sid, plg ).normalize();
	const Plane3 plgplane( plgnormal, knots[0].scaleBy(scale), false );
	Line3 intersectionline;
	if ( !newplane.intersectWith(plgplane,intersectionline) )
	    continue;

	for ( int knot=0; knot<nrknots; knot++ )
	{
	    const Coord3 p0 = knots[knot].scaleBy(scale);
	    const Coord3 p1 = knots[knot<nrknots-1 ? knot+1 : 0].scaleBy(scale);
	    const Line3 segment(p0, p1-p0);
	    double t, segt;
	    if ( intersectionline.closestPoint(segment,t,segt) && segt<=1+1e-3 
		    					       && segt+1e-3>=0 )
		intersections += segment.getPoint(segt).scaleBy(invscale);
	}
    }
   
    for ( int pt=0; pt<intersections.size(); pt++ )
    {
	if ( pt==0 )
    	    geo.insertPolygon( sid, nrplgs, 0, intersections[0], normal, true );
        else
	    geo.insertKnot( sid, RowCol(nrplgs,pt).toInt64(), 
		    	    intersections[pt], true );
    }
}


}; // namespace visSurvey
