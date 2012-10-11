/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jan 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visscene.h"

#include "settings.h"
#include "iopar.h"
#include "mouseevent.h"
#include "visobject.h"
#include "visdataman.h"
#include "visselman.h"
#include "visevent.h"
#include "vismarker.h"
#include "vispolygonoffset.h"
#include "vislight.h"
#include "SoOD.h"

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/nodes/SoEnvironment.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTextureMatrixTransform.h>
#include <Inventor/nodes/SoCallback.h>

#include <osg/Group>
#include <osg/Light>

#define mDefaultFactor	1
#define mDefaultUnits	200


mCreateFactoryEntry( visBase::Scene );

namespace visBase
{

Scene::Scene()
    : selroot_( new SoGroup )
    , environment_( new SoEnvironment )
    , polygonoffset_( PolygonOffset::create() )
    , directionallight_( DirectionalLight::create() )  
    , events_( *EventCatcher::create() )
    , mousedownid_( -1 )
    , blockmousesel_( false )
    , nameChanged(this)
    , callback_( 0 )
    , osgsceneroot_( 0 )
{
    directionallight_->ref();
    directionallight_->turnOn( false );
    osg::ref_ptr<osg::StateSet> stateset = osggroup_->getOrCreateStateSet();
    stateset->setAttributeAndModes(directionallight_->osgLight() );
    
    selroot_->ref();

    if ( !SoOD::getAllParams() )
    {
	callback_ = new SoCallback;
	selroot_->addChild( callback_ );
	callback_->setCallback( firstRender, this );
    }

    polygonoffset_->ref();
    polygonoffset_->setStyle( PolygonOffset::Filled );

    float units = mDefaultUnits;
    float factor = mDefaultFactor;

    PtrMan<IOPar> settings = Settings::common().subselect( sKeyOffset() );
    if ( settings )
    {
	settings->get( sKeyFactor(), factor );
	settings->get( sKeyUnits(), units );
    }

    polygonoffset_->setFactor( factor );
    polygonoffset_->setUnits( units );

    events_.ref();
    events_.nothandled.notify( mCB(this,Scene,mousePickCB) );


    osgsceneroot_ = new osg::Group;
    osgsceneroot_->addChild( DataObjectGroup::gtOsgNode() );
    osgsceneroot_->addChild( events_.osgNode() );
    osgsceneroot_->ref();
}


bool Scene::saveCurrentOffsetAsDefault() const
{
    fillOffsetPar( Settings::common() );
    return Settings::common().write( true );
}


Scene::~Scene()
{
    if ( osgsceneroot_ )
	osgsceneroot_->unref();

    removeAll();
    events_.nothandled.remove( mCB(this,Scene,mousePickCB) );
    events_.unRef();

    polygonoffset_->unRef();
    selroot_->unref();
    directionallight_->unRef();
}


void Scene::addObject( DataObject* dataobj )
{
    removeCallback();
    mDynamicCastGet( VisualObject*, vo, dataobj );
    if ( vo ) vo->setSceneEventCatcher( &events_ );
    DataObjectGroup::addObject( dataobj );
}


void Scene::setAmbientLight( float n )
{
    environment_->ambientIntensity.setValue( n );
}


float Scene::ambientLight() const
{
    return environment_->ambientIntensity.getValue();
}

 
void Scene::setDirectionalLight( const DirectionalLight& dl )
{
    directionallight_->setIntensity( dl.intensity() );
    directionallight_->setDirection( dl.direction( 0 ), dl.direction( 1 ),
	    dl.direction( 2 ) );
    directionallight_->turnOn( dl.isOn() );
}
 

DirectionalLight* Scene::getDirectionalLight() const
{
    return directionallight_;
}


void Scene::setName( const char* newname )
{
    DataObjectGroup::setName( newname );
    nameChanged.trigger();
}


bool Scene::blockMouseSelection( bool yn )
{
    const bool res = blockmousesel_;
    blockmousesel_ = yn;
    return res;
}


SoNode* Scene::gtInvntrNode()
{
    return selroot_;
}


osg::Node* Scene::gtOsgNode()
{
    return osgsceneroot_;
}


EventCatcher& Scene::eventCatcher() { return events_; }


void Scene::mousePickCB( CallBacker* cb )
{
    if ( blockmousesel_ )
	return;

    mCBCapsuleUnpack(const EventInfo&,eventinfo,cb);
    if ( events_.isHandled() )
    {
	if ( eventinfo.type==MouseClick )
	    mousedownid_ = -1;
	return;
    }

    if ( eventinfo.dragging )
    {
	const TabletInfo* ti = TabletInfo::currentState();
	if ( ti && ti->maxPostPressDist()<5 )
	    events_.setHandled();
	else
	    mousedownid_ = -1;

	return;
    }

    if ( eventinfo.type!=MouseClick ) return;
    if ( OD::middleMouseButton(eventinfo.buttonstate_) ) return;

    if ( eventinfo.pressed )
    {
	mousedownid_ = -1;

	const int sz = eventinfo.pickedobjids.size();
	for ( int idx=0; idx<sz; idx++ )
	{
	    const DataObject* dataobj =
			    DM().getObject(eventinfo.pickedobjids[idx]);
	    if ( !dataobj )
		continue;

	    if ( dataobj->selectable() || dataobj->rightClickable() )
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
	    if ( !OD::shiftKeyboardButton(eventinfo.buttonstate_) &&
		 !OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
		 !OD::altKeyboardButton(eventinfo.buttonstate_) )
	    {
		DM().selMan().deSelectAll();
		events_.setHandled();
	    }
	}

	bool markerclicked = false;

	for ( int idx=0; idx<sz; idx++ )
	{
	    DataObject* dataobj = DM().getObject(eventinfo.pickedobjids[idx]);
	    const bool idisok = markerclicked || mousedownid_==dataobj->id();

	    if ( idisok )
	    {
		if ( dataobj->rightClickable() &&
		     OD::rightMouseButton(eventinfo.buttonstate_) &&
		     dataobj->rightClicked() )
		{
		    mDynamicCastGet( visBase::Marker*, emod, dataobj );
		    if ( emod ) 
		    {
			markerclicked = true;
			continue;
		    }
		    dataobj->triggerRightClick(&eventinfo);
		    events_.setHandled();
		}
		else if ( dataobj->selectable() )
		{
		    if ( OD::shiftKeyboardButton(eventinfo.buttonstate_) &&
			  !OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
			  !OD::altKeyboardButton(eventinfo.buttonstate_) )
		    {
			DM().selMan().select( mousedownid_, true );
			events_.setHandled();
		    }
		    else if ( !OD::shiftKeyboardButton(eventinfo.buttonstate_)&&
			  !OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
			  !OD::altKeyboardButton(eventinfo.buttonstate_) )
		    {
			DM().selMan().select( mousedownid_, false );
			events_.setHandled();
		    }
		}

		break;
	    }
	}
    }

    //Note: 
    //Don't call setHandled, since that will block all other event-catchers
    //(Does not apply for OSG. Every scene has only one EventCatcher. JCG)
}


void Scene::fillPar( IOPar& par, TypeSet<int>& additionalsaves ) const
{
    DataObjectGroup::fillPar( par, additionalsaves );
    fillOffsetPar( par );
}


void Scene::fillOffsetPar( IOPar& par ) const
{
    IOPar offsetpar;
    offsetpar.set( sKeyFactor(), polygonoffset_->getFactor() );
    offsetpar.set( sKeyUnits(), polygonoffset_->getUnits() );
    par.mergeComp( offsetpar, sKeyOffset() );
}


int Scene::usePar( const IOPar& par )
{
    int res = DataObjectGroup::usePar( par );
    if ( res!=1 )
	return res;

    PtrMan<IOPar> settings = par.subselect( sKeyOffset() );
    if ( settings )
    {
	float units, factor;
	if ( settings->get( sKeyFactor(), factor ) &&
	     settings->get( sKeyUnits(), units ) )
	{
	    polygonoffset_->setFactor( factor );
	    polygonoffset_->setUnits( units );
	}
    }

    return 1;
}


void Scene::firstRender( void*, SoAction* action )
{
    if ( action->isOfType( SoGLRenderAction::getClassTypeId()) )
	SoOD::getAllParams();
}


void Scene::removeCallback()
{
    if ( !callback_ )
	return;

    if ( SoOD::getAllParams() )
    {
	selroot_->removeChild( callback_ );
	callback_ = 0;
    }
}




}; // namespace visBase
