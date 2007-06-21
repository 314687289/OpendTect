/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: vismpeeditor.cc,v 1.22 2007-06-21 19:35:21 cvskris Exp $";

#include "vismpeeditor.h"

#include "errh.h"
#include "geeditor.h"
#include "emeditor.h"
#include "emsurface.h"
#include "emsurfaceedgeline.h"
#include "emsurfacegeometry.h"
#include "math2.h"
#include "vismarker.h"
#include "visdatagroup.h"
#include "visdragger.h"
#include "visevent.h"
#include "vishingeline.h"
#include "vismaterial.h"
#include "visshapescale.h"
#include "vistransform.h"

mCreateFactoryEntry( visSurvey::MPEEditor );


namespace visSurvey
{

MPEEditor::MPEEditor()
    : geeditor( 0 )
    , noderightclick( this )
    , emeditor( 0 )
    , eventcatcher( 0 )
    , transformation( 0 )
    , markersize( 3 )
    , interactionlinedisplay( 0 )
    , interactionlinerightclick( this )
    , activedragger( EM::PosID::udf() )
    , activenodematerial( 0 )
    , nodematerial( 0 )
    , isdragging( false )
{
    nodematerial = visBase::Material::create();
    nodematerial->ref();
    nodematerial->setColor( Color(255,255,0) );

    activenodematerial = visBase::Material::create();
    activenodematerial->ref();
    activenodematerial->setColor( Color(255,0,0) );
}


MPEEditor::~MPEEditor()
{
    if ( interactionlinedisplay )
    {
	if ( interactionlinedisplay->rightClicked() )
	    interactionlinedisplay->rightClicked()->remove(
		mCB( this,MPEEditor,interactionLineRightClickCB));
	interactionlinedisplay->unRef();
    }


    setEditor( (Geometry::ElementEditor*) 0 );
    setEditor( (MPE::ObjectEditor*) 0 );
    setSceneEventCatcher( 0 );
    setDisplayTransformation( 0 );

    while ( draggers.size() )
	removeDragger( 0 );

    if ( activenodematerial ) activenodematerial->unRef();
    if ( nodematerial ) nodematerial->unRef();
}


void MPEEditor::setEditor( Geometry::ElementEditor* ge )
{
    if ( ge ) setEditor( (MPE::ObjectEditor*) 0 );
    CallBack movementcb( mCB(this,MPEEditor,nodeMovement) );
    CallBack numnodescb( mCB(this,MPEEditor,changeNumNodes) );
    if ( geeditor )
    {
	geeditor->getElement().movementnotifier.remove( movementcb );
	geeditor->editpositionchange.remove( numnodescb );
    }

    geeditor = ge;

    if ( geeditor )
    {
	geeditor->getElement().movementnotifier.notify( movementcb );
	geeditor->editpositionchange.notify( numnodescb );
	changeNumNodes(0);
    }
}


void MPEEditor::setEditor( MPE::ObjectEditor* eme )
{
    if ( eme ) setEditor( (Geometry::ElementEditor*) 0 );
    CallBack movementcb( mCB(this,MPEEditor,nodeMovement) );
    CallBack numnodescb( mCB(this,MPEEditor,changeNumNodes) );

    if ( emeditor )
    {
	EM::EMObject& emobj = const_cast<EM::EMObject&>(emeditor->emObject());
	emobj.change.remove( movementcb );
	emobj.unRef();
	emeditor->editpositionchange.remove( numnodescb );
    }

    emeditor = eme;

    if ( emeditor )
    {
	EM::EMObject& emobj = const_cast<EM::EMObject&>(emeditor->emObject());
	emobj.ref();
	emobj.change.notify( movementcb );
	emeditor->editpositionchange.notify( numnodescb );
	changeNumNodes(0);
    }
}


void MPEEditor::setSceneEventCatcher( visBase::EventCatcher* nev )
{
    if ( eventcatcher )
	eventcatcher->unRef();

    eventcatcher = nev;

    if ( eventcatcher )
	eventcatcher->ref();
}


void MPEEditor::setDisplayTransformation( visBase::Transformation* nt )
{
    if ( transformation )
	transformation->unRef();

    transformation = nt;
    if ( transformation )
	transformation->ref();

    for ( int idx=0; idx<draggers.size(); idx++ )
	draggers[idx]->setDisplayTransformation( transformation );
}


void MPEEditor::setMarkerSize(float nsz)
{
    for ( int idx=0; idx<draggers.size(); idx++ )
	draggermarkers[idx]->setScreenSize( nsz );

    markersize = nsz;
}


EM::PosID MPEEditor::getNodePosID(int idx) const
{ return idx>=0&&idx<posids.size() ? posids[idx] : EM::PosID::udf(); }


void MPEEditor::mouseClick( const EM::PosID& pid,
			    bool shift, bool alt, bool ctrl )
{
    if ( !shift && !alt && !ctrl && emeditor )
    {
	TypeSet<EM::PosID> pids;
	emeditor->getEditIDs(pids);
	for ( int idx=0; idx<pids.size(); idx++ )
	    emeditor->removeEditID(pids[idx]);

	if ( emeditor->addEditID(pid) )
	    setActiveDragger( pid );
    }
    else if ( shift && !alt && !ctrl && activedragger.objectID()!=-1 )
	extendInteractionLine( pid );
}


void MPEEditor::changeNumNodes( CallBacker* )
{
    TypeSet<EM::PosID> editnodes;
    if ( emeditor )
	emeditor->getEditIDs( editnodes );
    else if ( geeditor )
    {
	posids.erase();
	TypeSet<GeomPosID> gpids;
	geeditor->getEditIDs( gpids );
	for ( int idy=0; idy<gpids.size(); idy++ )
	    editnodes += EM::PosID( -1, -1, gpids[idy] );
    }
    else
    {
	posids.erase();
    }

    TypeSet<EM::PosID> nodestoremove( posids );
    nodestoremove.createDifference( editnodes );

    if ( nodestoremove.indexOf(activedragger)!=-1 )
	setActiveDragger( EM::PosID::udf() );

    for ( int idx=0; idx<nodestoremove.size(); idx++ )
	removeDragger( posids.indexOf(nodestoremove[idx]) );

    TypeSet<EM::PosID> nodestoadd( editnodes );
    nodestoadd.createDifference( posids );

    for ( int idx=0; idx<nodestoadd.size(); idx++ )
	addDragger( nodestoadd[idx] );
}


void MPEEditor::removeDragger( int idx )
{
    draggers[idx]->started.remove(mCB(this,MPEEditor,dragStart));
    draggers[idx]->motion.remove(mCB(this,MPEEditor,dragMotion));
    draggers[idx]->finished.remove(mCB(this,MPEEditor,dragStop));
    removeChild( draggers[idx]->getInventorNode() );
    draggers[idx]->unRef();
    draggersshapesep[idx]->unRef();
    draggers.removeFast(idx);
    posids.removeFast(idx);
    draggersshapesep.removeFast(idx);
    draggermarkers.removeFast(idx);
}


void MPEEditor::addDragger( const EM::PosID& pid )
{
    bool translate1D = false, translate2D = false, translate3D = false;
    if ( emeditor )
    {
	translate1D = emeditor->mayTranslate1D(pid);
	translate2D = emeditor->mayTranslate2D(pid);
	translate3D = emeditor->mayTranslate3D(pid);
    }
    else if ( geeditor )
    {
	translate1D = geeditor->mayTranslate1D(pid.subID());
	translate2D = geeditor->mayTranslate2D(pid.subID());
	translate3D = geeditor->mayTranslate3D(pid.subID());
    }
    else
	return;

    if ( !translate1D && !translate2D && !translate3D )
	return;
	         
    visBase::Dragger* dragger = visBase::Dragger::create();
    dragger->ref();
    dragger->setDisplayTransformation( transformation );

    dragger->started.notify(mCB(this,MPEEditor,dragStart));
    dragger->motion.notify(mCB(this,MPEEditor,dragMotion));
    dragger->finished.notify(mCB(this,MPEEditor,dragStop));

    if ( translate3D )
    {
	dragger->setDraggerType( visBase::Dragger::Translate3D );
	dragger->setDefaultRotation();
    }
    else if ( translate2D )
    {
	dragger->setDraggerType( visBase::Dragger::Translate2D );
	const Coord3 defnormal( 0, 0, 1 );
	const Coord3 desnormal = emeditor 
	    ? emeditor->translation2DNormal( pid ).normalize()
	    : geeditor->translation2DNormal( pid.subID() )
							    .normalize();
	const float angle = ACos( defnormal.dot(desnormal) );
	const Coord3 axis = defnormal.cross(desnormal);
	dragger->setRotation( axis, angle );
    }
    else 
    {
	dragger->setDraggerType( visBase::Dragger::Translate1D );
	const Coord3 defori( 1, 0, 0 );
	const Coord3 desori = emeditor 
	    ? emeditor->translation1DDirection( pid ).normalize()
	    : geeditor->translation1DDirection( pid.subID() )
								.normalize();
	const float angle = ACos( defori.dot(desori) );
	const Coord3 axis = defori.cross(desori);
	dragger->setRotation( axis, angle );
    }

    visBase::Marker* marker = visBase::Marker::create();
    marker->setMaterial( nodematerial );
    marker->setScreenSize( markersize );
    visBase::DataObjectGroup* separator = visBase::DataObjectGroup::create();
    separator->setSeparate( true );
    separator->addObject( marker );
    separator->ref();
    dragger->setOwnShape( separator, "translator" );

    visBase::ShapeScale* shapescale = visBase::ShapeScale::create();
    shapescale->ref();
    shapescale->restoreProportions(true);
    shapescale->setScreenSize(20);
    shapescale->setMinScale( 10 );
    shapescale->setMaxScale( 150 );
    shapescale->setShape( dragger->getShape("translatorActive"));
    dragger->setOwnShape( shapescale, "translatorActive" );
    shapescale->unRef();

    dragger->setPos( emeditor
	    ? emeditor->getPosition(pid)
	    : geeditor->getPosition(pid.subID()) );

    addChild( dragger->getInventorNode() );

    draggers += dragger;
    draggermarkers += marker;
    draggersshapesep += separator;
    posids += pid;
}



void MPEEditor::nodeMovement(CallBacker* cb)
{
    if ( emeditor )
    {
	mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);
	if ( cbdata.event!=EM::EMObjectCallbackData::PositionChange )
	    return;

	const int idx = posids.indexOf( cbdata.pid0 );
	if ( idx==-1 ) return;

	const Coord3 pos = emeditor->getPosition( cbdata.pid0 );
	updateNodePos( idx,  emeditor->getPosition(cbdata.pid0) );
    }
    else if ( geeditor )
    {
	TypeSet<GeomPosID> pids;
	mCBCapsuleUnpack(const TypeSet<GeomPosID>*,gpids,cb);
	if ( gpids ) pids = *gpids;
	else geeditor->getEditIDs( pids );

	for ( int idx=0; idx<pids.size(); idx++ )
	{
	    for ( int idy=0; idy<posids.size(); idy++ )
	    {
		if ( posids[idy].subID()==pids[idx] )
		    updateNodePos( idy, geeditor->getPosition(pids[idx]));
	    }
	}
    }
}


void MPEEditor::updateNodePos( int idx, const Coord3& pos )
{
    if ( isdragging && posids[idx]==activedragger )
	return;

    draggers[idx]->setPos( pos );
}


void MPEEditor::interactionLineRightClickCB( CallBacker* )
{ interactionlinerightclick.trigger(); }

	

bool MPEEditor::clickCB( CallBacker* cb )
{
    if ( eventcatcher->isEventHandled() || !isOn() )
	return true;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb );

    if ( eventinfo.type!=visBase::MouseClick || !eventinfo.pressed )
	return true;

    int nodeidx = -1;
    for ( int idx=0; idx<draggers.size(); idx++ )
    {
	if ( eventinfo.pickedobjids.indexOf(draggers[idx]->id()) != -1 )
	{
	    nodeidx = idx;
	    break;
	}
    }

    if ( nodeidx==-1 )
	return true;

    if ( eventinfo.mousebutton==visBase::EventInfo::rightMouseButton() )
    {
	rightclicknode = nodeidx;
	noderightclick.trigger();
	eventcatcher->eventIsHandled();
	return false;
    }

    if ( eventinfo.mousebutton!=visBase::EventInfo::leftMouseButton() )
	return true;

    if ( activedragger!=-1 && eventinfo.shift && !eventinfo.ctrl &&
	 !eventinfo.alt )
    {
	extendInteractionLine( posids[nodeidx] );
	eventcatcher->eventIsHandled();
	return false;
    }

    return false;
}


int MPEEditor::getRightClickNode() const { return rightclicknode; }


void MPEEditor::dragStart( CallBacker* cb )
{
    const int idx = draggers.indexOf((visBase::Dragger*) cb );
    setActiveDragger( posids[idx] );
    if ( emeditor ) emeditor->startEdit(activedragger);
}


void MPEEditor::dragMotion( CallBacker* cb )
{
    const int idx = draggers.indexOf( (visBase::Dragger*) cb );
    const Coord3 np = draggers[idx]->getPos();
    isdragging = true;
    if ( emeditor ) emeditor->setPosition( np );
    if ( geeditor ) geeditor->setPosition( posids[idx].subID(), np );
    isdragging = false;
}


void MPEEditor::dragStop( CallBacker* cb )
{
    if ( emeditor ) 
    { 
	const int idx = draggers.indexOf( (visBase::Dragger*) cb );
        NotifyStopper cbstop( draggers[idx]->motion );
	draggers[idx]->setPos( emeditor->getPosition( activedragger ) );
	emeditor->finishEdit();
    }
}


void MPEEditor::setActiveDragger( const EM::PosID& pid )
{
    if ( emeditor ) emeditor->restartInteractionLine(pid);

    if ( activedragger==pid )
	return;

/*
    int idx = posids.indexOf(activedragger);
    if ( idx!=-1 )
	draggermarkers[idx]->setMaterial( nodematerial );
	*/

    activedragger = pid;
/*
    idx = posids.indexOf(activedragger);
    if ( idx!=-1 )
	draggermarkers[idx]->setMaterial( activenodematerial );
	*/
}


void MPEEditor::setupInteractionLineDisplay()
{
    const EM::EdgeLineSet* edgelineset = emeditor
	? emeditor->getInteractionLine() : 0;

    if ( !edgelineset )
	return;

    if ( !interactionlinedisplay )
    {
	interactionlinedisplay = EdgeLineSetDisplay::create();
	interactionlinedisplay->setDisplayTransformation( transformation );
	interactionlinedisplay->ref();
	interactionlinedisplay->setConnect(true);
	interactionlinedisplay->setShowDefault(true);
	interactionlinedisplay->getMaterial()->setColor(Color(255,255,0),0);

	addChild( interactionlinedisplay->getInventorNode() );
	if ( interactionlinedisplay->rightClicked() )
	    interactionlinedisplay->rightClicked()->notify(
		mCB( this,MPEEditor,interactionLineRightClickCB));
    }

    interactionlinedisplay->setEdgeLineSet(edgelineset);
} 


void MPEEditor::extendInteractionLine( const EM::PosID& pid )
{
    setupInteractionLineDisplay();
    if ( !interactionlinedisplay || activedragger.subID()==-1 )
	return;

    emeditor->interactionLineInteraction(pid);
}
    
}; //namespce
