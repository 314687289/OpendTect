/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Jan 2002
 RCS:           $Id: visscene.cc,v 1.32 2006-06-14 17:09:43 cvskris Exp $
________________________________________________________________________

-*/

#include "visscene.h"
#include "visobject.h"
#include "visdataman.h"
#include "visselman.h"
#include "visevent.h"
#include "vismarker.h"
#include "vispolygonoffset.h"

#include <Inventor/nodes/SoEnvironment.h>
#include <Inventor/nodes/SoSeparator.h>

mCreateFactoryEntry( visBase::Scene );

namespace visBase
{

Scene::Scene()
    : selroot_( new SoGroup )
    , environment_( new SoEnvironment )
    , polygonoffset_( PolygonOffset::create() )
    , events_( *EventCatcher::create() )
    , mousedownid_( -1 )
    , blockmousesel_( false )
{
    selroot_->ref();

    polygonoffset_->ref();
    polygonoffset_->setStyle( PolygonOffset::Filled );
    polygonoffset_->setFactor( 1 );
    polygonoffset_->setUnits( 2 );
    selroot_->addChild( polygonoffset_->getInventorNode() );

    selroot_->addChild( environment_ );
    events_.ref();
    selroot_->addChild( events_.getInventorNode() );
    selroot_->addChild( DataObjectGroup::getInventorNode() );
    events_.nothandled.notify( mCB(this,Scene,mousePickCB) );
}


Scene::~Scene()
{
    removeAll();
    events_.nothandled.remove( mCB(this,Scene,mousePickCB) );
    events_.unRef();

    polygonoffset_->unRef();
    selroot_->unref();
}


void Scene::addObject( DataObject* dataobj )
{
    mDynamicCastGet( VisualObject*, vo, dataobj );
    if ( vo ) vo->setSceneEventCatcher( &events_ );
    DataObjectGroup::addObject( dataobj );
}


void Scene::insertObject( int idx, DataObject* dataobj )
{
    mDynamicCastGet( VisualObject*, vo, dataobj );
    if ( vo ) vo->setSceneEventCatcher( &events_ );
    DataObjectGroup::insertObject( idx, dataobj );
}


void Scene::setAmbientLight( float n )
{
    environment_->ambientIntensity.setValue( n );
}


float Scene::ambientLight() const
{
    return environment_->ambientIntensity.getValue();
}


bool Scene::blockMouseSelection( bool yn )
{
    const bool res = blockmousesel_;
    blockmousesel_ = yn;
    return res;
}


SoNode* Scene::getInventorNode()
{
    return selroot_;
}


void Scene::mousePickCB( CallBacker* cb )
{
    if ( blockmousesel_ )
	return;

    mCBCapsuleUnpack(const EventInfo&,eventinfo,cb);
    if ( events_.isEventHandled() )
    {
	if ( eventinfo.type==MouseClick )
	    mousedownid_ = -1;
	return;
    }

    if ( eventinfo.type!=MouseClick ) return;
    if ( eventinfo.mousebutton==EventInfo::middleMouseButton() ) return;

    if ( eventinfo.pressed )
    {
	mousedownid_ = -1;

	const int sz = eventinfo.pickedobjids.size();
	for ( int idx=0; idx<sz; idx++ )
	{
	    const DataObject* dataobj =
			    DM().getObject(eventinfo.pickedobjids[idx]);
	    if ( dataobj->selectable() )
	    {
		mousedownid_ = eventinfo.pickedobjids[idx];
		break;
	    }
	}
    }
    else
    {
	const int sz = eventinfo.pickedobjids.size();
	if ( !sz && mousedownid_==-1 )
	{
	    if ( !eventinfo.shift && !eventinfo.alt && !eventinfo.ctrl )
		DM().selMan().deSelectAll();
	}

	bool markerclicked = false;

	for ( int idx=0; idx<sz; idx++ )
	{
	    DataObject* dataobj = DM().getObject(eventinfo.pickedobjids[idx]);
	    const bool idisok = markerclicked || mousedownid_==dataobj->id();

	    if ( dataobj->selectable() && idisok )
	    {
		if ( eventinfo.mousebutton==EventInfo::rightMouseButton() &&
			dataobj->rightClicked() )
		{
		    mDynamicCastGet( visBase::Marker*, emod, dataobj );
		    if ( emod ) 
		    {
			markerclicked = true;
			continue;
		    }
		    dataobj->triggerRightClick(&eventinfo);
		}
		else if ( eventinfo.shift && !eventinfo.alt && !eventinfo.ctrl )
		    DM().selMan().select( mousedownid_, true );
		else if ( !eventinfo.shift && !eventinfo.alt && !eventinfo.ctrl)
		    DM().selMan().select( mousedownid_, false );

		break;
	    }
	}
    }

    //Note:
    //Don't call setHandled, since that will block all other event-catchers
}


}; // namespace visBase
