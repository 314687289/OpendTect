/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2002
-*/

static const char* rcsID = "$Id: vispicksetdisplay.cc,v 1.77 2006-04-14 14:43:14 cvsnanne Exp $";

#include "vispicksetdisplay.h"

#include "color.h"
#include "iopar.h"
#include "pickset.h"
#include "survinfo.h"
#include "visevent.h"
#include "visdataman.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "visdatagroup.h"
#include "vistransform.h"
#include "vistransmgr.h"
#include "separstr.h"
#include "trigonometry.h"


mCreateFactoryEntry( visSurvey::PickSetDisplay );

namespace visSurvey {

const char* PickSetDisplay::nopickstr = "No Picks";
const char* PickSetDisplay::pickprefixstr = "Pick ";
const char* PickSetDisplay::showallstr = "Show all";
const char* PickSetDisplay::shapestr = "Shape";
const char* PickSetDisplay::sizestr = "Size";

PickSetDisplay::PickSetDisplay()
    : VisualObjectImpl(true)
    , group( visBase::DataObjectGroup::create() )
    , eventcatcher( 0 )
    , initsz(3)
    , picktype(3)
    , changed(this)
    , showall(true)
    , haschanged(false)
    , transformation( 0 )
{
    group->ref();
    addChild( group->getInventorNode() );
    setScreenSize(initsz);

    fillMarkerSet();
}
    

PickSetDisplay::~PickSetDisplay()
{
    setSceneEventCatcher( 0 );
    removeChild( group->getInventorNode() );
    group->unRef();

    if ( transformation ) transformation->unRef();
}


bool PickSetDisplay::isPicking() const
{
    return isSelected() && !isLocked();
}


void PickSetDisplay::copyFromPickSet( const PickSet& pickset )
{
    setColor( pickset.color_ );
    setName( pickset.name() );
    setScreenSize( pickset.size_ );
    removeAll();
    bool hasdir = false;
    const int nrpicks = pickset.nrPicks();
    for ( int idx=0; idx<nrpicks; idx++ )
    {
	const PickLocation& loc = pickset[idx];
	addPick( Coord3(loc.pos,loc.z), loc.dir );
	if ( loc.hasDir() ) hasdir = true;
    }

    if ( hasdir ) //show Arrows
       setType( markertypes.size()-2 );

    haschanged = false;
}


void PickSetDisplay::copyToPickSet( PickSet& pickset ) const
{
    pickset.setName( name() );
    pickset.color_ = getMaterial()->getColor();
    pickset.color_.setTransparency( 0 );
    pickset.size_ = picksz;
    for ( int idx=0; idx<nrPicks(); idx++ )
	pickset += PickLocation( getPick(idx), getDirection(idx) );
}


void PickSetDisplay::addPick( const Coord3& pos, const Sphere& dir )
{
    visBase::Marker* marker = visBase::Marker::create();
    group->addObject( marker );

    marker->setDisplayTransformation( transformation );
    marker->setCenterPos( pos );
    marker->setDirection( dir );
    marker->setScreenSize( picksz );
    marker->setType( (MarkerStyle3D::Type)picktype );
    marker->setMaterial( 0 );

    haschanged = true;
    changed.trigger();
}


void PickSetDisplay::addPick( const Coord3& pos )
{
    addPick( pos, Sphere(0,0,0) );
}


BufferString PickSetDisplay::getManipulationString() const
{
    BufferString str = "Nr. of picks: ";
    str += nrPicks();
    return str;
}


int PickSetDisplay::nrPicks() const
{
    return group->size();
}


Coord3 PickSetDisplay::getPick( int idx ) const
{
    mDynamicCastGet(visBase::Marker*,marker,group->getObject(idx))
    return marker ? marker->centerPos() : Coord3::udf();
}


Coord3 PickSetDisplay::getDirection( int idx ) const
{
    mDynamicCastGet(visBase::Marker*,marker,group->getObject(idx))
    Sphere dir = marker ? marker->getDirection() : Sphere(0,0,0);
    return Coord3(dir.radius,dir.theta,dir.phi);
}


void PickSetDisplay::removePick( const Coord3& pos )
{
    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*, marker, group->getObject( idx ) );
	if ( !marker ) continue;

	if ( marker->centerPos() == pos )
	{
	    group->removeObject( idx );
	    haschanged = true;
	    changed.trigger();
	    return;
	}
    }
}


void PickSetDisplay::removeAll()
{
    group->removeAll();
}


void PickSetDisplay::showAll( bool yn )
{
    showall = yn;
    if ( !showall ) return;

    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*, marker, group->getObject( idx ) );
	if ( !marker ) continue;

	marker->turnOn( true );
    }
}


void PickSetDisplay::otherObjectsMoved(
			const ObjectSet<const SurveyObject>& objs, int )
{
    if ( showall ) return;
    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*,marker,group->getObject(idx))
	if ( !marker ) continue;

	Coord3 pos = marker->centerPos(true);
	marker->turnOn( false );
	for ( int idy=0; idy<objs.size(); idy++ )
	{
	    const float dist = objs[idy]->calcDist(pos);
	    if ( dist < objs[idy]->maxDist() )
	    {
		marker->turnOn(true);
		break;
	    }
	}
    }
}


void PickSetDisplay::setScreenSize( float newsize )
{
    picksz = newsize;
    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*, marker, group->getObject( idx ) );
	if ( !marker ) continue;

	marker->setScreenSize( picksz );
    }
}


void PickSetDisplay::setColor( Color col )
{
    (this)->getMaterial()->setColor( col );
}


Color PickSetDisplay::getColor() const
{
    return (this)->getMaterial()->getColor();
}


void PickSetDisplay::setType( int tp )
{
    if ( tp < 0 ) tp = 0;
    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*,marker,group->getObject(idx))
	if ( !marker ) continue;
	marker->setType( (MarkerStyle3D::Type)tp );
    }

    picktype = tp;
}


int PickSetDisplay::getType() const
{
    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*,marker,group->getObject(idx))
	if ( !marker ) continue;
	return (int)marker->getType();
    }

    return -1;
}


void PickSetDisplay::fillMarkerSet()
{
    const char** names = MarkerStyle3D::TypeNames;
    while ( *names )
	markertypes.add( *names++ );
}


void PickSetDisplay::pickCB( CallBacker* cb )
{
    if ( !isOn() || !isSelected() || isLocked() ) return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
    if ( eventinfo.type != visBase::MouseClick ||
	 eventinfo.mousebutton != visBase::EventInfo::leftMouseButton() )
	return;

    int eventid = -1;
    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	visBase::DataObject* dataobj =
	    		visBase::DM().getObject(eventinfo.pickedobjids[idx]);
	if ( dataobj->selectable() )
	{
	    eventid = eventinfo.pickedobjids[idx];
	    break;
	}
    }

    if ( eventinfo.pressed )
    {
	mousepressid = eventid;
	mousepressposition = eventid==-1 ? Coord3::udf() : eventinfo.pickedpos;
	eventcatcher->eventIsHandled();
    }
    else 
    {
	if ( eventinfo.ctrl && !eventinfo.alt && !eventinfo.shift )
	{
	    if ( eventinfo.pickedobjids.size() &&
		 eventid==mousepressid )
	    {
		int removeidx = group->getFirstIdx(mousepressid);
		if ( removeidx != -1 )
		{
		    group->removeObject( removeidx );
		    changed.trigger();
		}
	    }

	    eventcatcher->eventIsHandled();
	}
	else if ( !eventinfo.ctrl && !eventinfo.alt && !eventinfo.shift )
	{
	    if ( eventinfo.pickedobjids.size() &&
		 eventid==mousepressid )
	    {
		const int sz = eventinfo.pickedobjids.size();
		bool validpicksurface = false;

		for ( int idx=0; idx<sz; idx++ )
		{
		    const DataObject* pickedobj =
			visBase::DM().getObject(eventinfo.pickedobjids[idx]);
		    mDynamicCastGet(const SurveyObject*,so,pickedobj)
		    if ( so && so->allowPicks() )
		    {
			validpicksurface = true;
			break;
		    }
		}

		if ( validpicksurface )
		{
		    Coord3 newpos = scene_->getZScaleTransform()->
			transformBack( eventinfo.pickedpos );
		    if ( transformation )
			newpos = transformation->transformBack(newpos);
		    mDynamicCastGet(SurveyObject*,so,
			    	    visBase::DM().getObject(eventid))
		    if ( so ) so->snapToTracePos( newpos );
		    addPick( newpos );
		}
	    }

	    eventcatcher->eventIsHandled();
	}
    }
}


void PickSetDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );

    const int nrpicks = group->size();
    par.set( nopickstr, nrpicks );

    for ( int idx=0; idx<nrpicks; idx++ )
    {
	const DataObject* so = group->getObject( idx );
        mDynamicCastGet(const visBase::Marker*, marker, so );
	BufferString key = pickprefixstr; key += idx;
	Coord3 pos = marker->centerPos();
	Sphere dir = marker->getDirection();
	FileMultiString str; str += pos.x; str += pos.y; str += pos.z;
	if ( dir.radius || dir.theta || dir.phi )
	    { str += dir.radius; str += dir.theta; str += dir.phi; }
	par.set( key, str.buf() );
    }

    par.setYN( showallstr, showall );

    int type = getType();
    par.set( shapestr, type );
    par.set( sizestr, picksz );
}


int PickSetDisplay::usePar( const IOPar& par )
{
    int res =  visBase::VisualObjectImpl::usePar( par );
    if ( res != 1 ) return res;

    picktype = 0;
    par.get( shapestr, picktype );

    picksz = 5;
    par.get( sizestr, picksz );

    bool shwallpicks = true;
    par.getYN( showallstr, shwallpicks );
    showAll( shwallpicks );

    group->removeAll();

    int nopicks = 0;
    par.get( nopickstr, nopicks );
    for ( int idx=0; idx<nopicks; idx++ )
    {
	BufferString str;
	BufferString key = pickprefixstr; key += idx;
	if ( !par.get(key,str) )
	    return -1;

	FileMultiString fms( str );
	Coord3 pos( atof(fms[0]), atof(fms[1]), atof(fms[2]) );
	Sphere dir;
	if ( fms.size() > 3 )
	    dir = Sphere( atof(fms[3]), atof(fms[4]), atof(fms[5]) );
	    
	addPick( pos, dir );
    }

    return 1;
}


void PickSetDisplay::setDisplayTransformation( visBase::Transformation* newtr )
{
    if ( transformation==newtr )
	return;

    if ( transformation )
	transformation->unRef();

    transformation = newtr;

    if ( transformation )
	transformation->ref();

    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet( visBase::Marker*, marker, group->getObject(idx));
	marker->setDisplayTransformation( transformation );
    }
}


visBase::Transformation* PickSetDisplay::getDisplayTransformation()
{
    return transformation;
}


void PickSetDisplay::setSceneEventCatcher( visBase::EventCatcher* nevc )
{
    if ( eventcatcher )
    {
	eventcatcher->eventhappened.remove(mCB(this,PickSetDisplay,pickCB));
	eventcatcher->unRef();
    }

    eventcatcher = nevc;

    if ( eventcatcher )
    {
	eventcatcher->eventhappened.notify(mCB(this,PickSetDisplay,pickCB));
	eventcatcher->ref();
    }

}

}; // namespace visSurvey
