/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : J.C. Glas
 * DATE     : December 2010
-*/

static const char* rcsID = "$Id: vissower.cc,v 1.10 2011-12-16 15:57:21 cvskris Exp $";


#include "vissower.h"

#include "bendpointfinder.h"
#include "cubesampling.h"
#include "mousecursor.h"
#include "mouseevent.h"
#include "settings.h"
#include "survinfo.h"
#include "timefun.h"
#include "visevent.h"
#include "vislocationdisplay.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "vismpeeditor.h"
#include "vispolygonselection.h"
#include "vispolyline.h"
#include "vissurvscene.h"
#include "vistransform.h"
#include "vistransmgr.h"


namespace visSurvey
{


Sower::Sower( const visBase::VisualObjectImpl* editobj )
    : visBase::VisualObjectImpl( false )
    , editobject_( editobj )
    , eventcatcher_( 0 )
    , mode_( Idle )
    , sowingline_( visBase::PolyLine::create() )
    , linelost_( false )
    , singleseeded_( true )
    , curpid_( EM::PosID::udf() )
    , curpidstamp_( mUdf(int) )
    , workrange_( 0 )
{
    sowingline_->ref();
    addChild( sowingline_->getInventorNode() );
    sowingline_->setMaterial( visBase::Material::create() );
    reInitSettings();
}


Sower::~Sower()
{
    removeChild( sowingline_->getInventorNode() );
    sowingline_->unRef();
    deepErase( eventlist_ );

    if ( workrange_ )
    {
	delete workrange_;
	workrange_ = 0;
    }
}


void Sower::reInitSettings()
{
    reversesowingorder_ = false;
    alternatesowingorder_ = false;
    intersow_ = false;

    setIfDragInvertMask( false );
    setSequentSowMask();
    setLaserMask();
    setEraserMask();
}


void Sower::reverseSowingOrder( bool yn )
{ reversesowingorder_ = yn; }


void Sower::alternateSowingOrder( bool yn )
{ alternatesowingorder_ = yn; }


void Sower::intersow( bool yn )
{ intersow_ = yn; }


void Sower::setDisplayTransformation( const mVisTrans* transformation )
{ sowingline_->setDisplayTransformation( transformation ); }


void Sower::setEventCatcher( visBase::EventCatcher* eventcatcher )
{ eventcatcher_ = eventcatcher; }


#define mReturnHandled( yn ) \
{ \
    if ( yn && eventcatcher_ ) eventcatcher_->setHandled(); \
    return yn; \
}


bool Sower::activate( const Color& color, const visBase::EventInfo& eventinfo,
		      int underlyingobjid )
{ return activate( color, eventinfo, underlyingobjid, 0 ); }


bool Sower::activate( const Color& color, const visBase::EventInfo& eventinfo,
		      const HorSampling* workrange )
{ return activate( color, eventinfo, -1, workrange ); }


bool Sower::activate( const Color& color, const visBase::EventInfo& eventinfo,
		      int underlyingobjid, const HorSampling* workrg )
{
    if ( mode_ != Idle )
	mReturnHandled( false );

    Scene* scene = STM().currentScene();
    if ( scene && scene->getPolySelection()->getSelectionType() !=
	    					visBase::PolygonSelection::Off )
	mReturnHandled( false );

    if ( eventinfo.type!=visBase::MouseClick || !eventinfo.pressed )
	mReturnHandled( false );

    if ( eventinfo.buttonstate_ & OD::RightButton )
	mReturnHandled( false );

    mode_ = Furrowing;
    furrowstamp_ = Time::getMilliSeconds();

    underlyingobjid_ = underlyingobjid;
    if ( workrange_ ) delete workrange_;
    workrange_ = workrg ? new HorSampling(*workrg) : 0;

    if ( !accept(eventinfo) )
    {
	mode_ = Idle;
	mReturnHandled( false );
    }

    sowingline_->getMaterial()->setColor( color );
    sowingline_->turnOn( true );

    mReturnHandled( true );
}


Coord3 Sower::pivotPos() const
{
    if ( mode_<FirstSowing || eventlist_.isEmpty() )
	return Coord3::udf();

    Coord3 sum = eventlist_[0]->displaypickedpos;
    sum += eventlist_[eventlist_.size()-1]->displaypickedpos;
    return 0.5*sum;
}


bool Sower::moreToSow() const
{ return mode_>=FirstSowing && bendpoints_.size()>1; }


void Sower::stopSowing()
{ bendpoints_.erase(); }


bool Sower::accept( const visBase::EventInfo& eventinfo )
{
    if ( eventinfo.tabletinfo )
	return acceptTablet( eventinfo );

    return acceptMouse( eventinfo );
}


bool Sower::isInWorkRange( const visBase::EventInfo& eventinfo ) const
{
    if ( !workrange_ || !workrange_->isDefined() )
	return false;

    const BinID eventbid = SI().transform( eventinfo.worldpickedpos );
    return workrange_->includes(eventbid);
}


void Sower::tieToWorkRange( const visBase::EventInfo& eventinfo )
{
    if ( !workrange_ || !workrange_->isDefined() || !eventlist_.size() )
	return;

    if ( workrange_->nrInl()!=1 && workrange_->nrCrl()!=1 )
	return;

    if ( isInWorkRange(eventinfo) )
    {
	if ( eventinfo.type!=visBase::MouseMovement || eventlist_.size()!=1 )
	    return;
    }
    else if ( eventinfo.type!=visBase::MouseClick || eventinfo.pressed )
	return; 

    Coord3& lastpos = eventlist_[eventlist_.size()-1]->worldpickedpos;
    const BinID lastbid = SI().transform( lastpos );
    const BinID start = workrange_->start;
    const BinID stop = workrange_->stop;

    const int min2 = mMIN( lastbid.sqDistTo(start), lastbid.sqDistTo(stop) );
    const int step = workrange_->nrCrl()==1 ? workrange_->step.inl
					    : workrange_->step.crl;
    if ( min2 > 100*step*step )
	return;

    lastpos.coord() = lastbid.sqDistTo(start) < lastbid.sqDistTo(stop) ?
		      SI().transform(start) : SI().transform(stop);
}


bool Sower::acceptMouse( const visBase::EventInfo& eventinfo )
{
    if ( mode_==Idle &&
	 eventinfo.type==visBase::MouseClick && !eventinfo.pressed )
    {
	const EM::PosID pid = getMarkerID( eventinfo );
	if ( pid.isUdf() )
	    mReturnHandled( true );
    }

    if ( mode_ != Furrowing )
	mReturnHandled( false );

    if ( eventinfo.type == visBase::Keyboard )
	mReturnHandled( true );

    if ( eventinfo.type==visBase::MouseClick && eventinfo.pressed )
	pressedbutstate_ = eventinfo.buttonstate_;

    const int sz = eventlist_.size();
    if ( eventinfo.type==visBase::MouseMovement || eventinfo.pressed )
    {
	if ( sz && eventinfo.mousepos==eventlist_[sz-1]->mousepos )
	    mReturnHandled( true );

	bool isvalidpos = !sz ||
			eventinfo.pickedobjids==eventlist_[0]->pickedobjids ||
			eventinfo.pickedobjids.isPresent(underlyingobjid_);

	if ( workrange_ )
	{
	    isvalidpos = isInWorkRange( eventinfo ) &&
			 !eventinfo.pickedobjids.isPresent(sowingline_->id());
	}

	if ( !isvalidpos )
	{
	    if ( eventinfo.worldpickedpos.isDefined() && !linelost_ )
		sowingline_->addPoint( eventinfo.worldpickedpos );
	    else
		linelost_ = true;

	    mReturnHandled( true );
	}

	linelost_ = false;
	sowingline_->addPoint( eventinfo.worldpickedpos );

	if ( !sz )
	    singleseeded_ = true;
	else if ( eventinfo.mousepos.distTo(eventlist_[0]->mousepos) > 5 )
	    singleseeded_ = false;

	eventlist_ += new visBase::EventInfo( eventinfo );
	mousecoords_ += eventinfo.mousepos;

	tieToWorkRange( eventinfo );
	mReturnHandled( true );
    }
    else
	tieToWorkRange( eventinfo );

    if ( !sz || !sowingline_->isOn() )
    {
	reset();
	mReturnHandled( true );
    }

    MouseCursorChanger mousecursorchanger( MouseCursor::Wait );

    if ( Time::passedSince(furrowstamp_) < 200 )
	singleseeded_ = true;

    int butstate = pressedbutstate_;
    if ( !singleseeded_ )
	butstate ^= ifdraginvertmask_;

    eventlist_[0]->type = visBase::MouseClick;
    eventlist_[0]->buttonstate_ = (OD::ButtonState) butstate;
    butstate &= sequentsowmask_;

    for ( int idx=sz-1; idx>0; idx--)
    {
	if ( singleseeded_ )
	{
	    eventlist_.remove( idx );
	    mousecoords_.remove( idx );
	}
	else
	{
	    eventlist_[idx]->type = visBase::MouseClick;
	    eventlist_[idx]->buttonstate_ = (OD::ButtonState) butstate;
	}
    }

    const bool intersowing = intersow_ && eventlist_.size()>1;

    float bendthreshold = 2.0;
    mSettUse( get, "dTect.Seed dragging", "Bend threshold", bendthreshold );
    if ( intersowing )
	bendthreshold *= 2;
    if ( bendthreshold < 0.1 )
	bendthreshold = 0.1;

    BendPointFinder2D bpfinder( mousecoords_, bendthreshold );
    bpfinder.execute( true );

    bendpoints_.erase();

    const int last = intersowing ? eventlist_.size()-1
				 : bpfinder.bendPoints().size()-1;

    for ( int idx=0; idx<=last; idx++ )
    {
	int eventidx = idx;
	if ( alternatesowingorder_ )
	    eventidx = idx%2 ? last-idx/2 : idx/2;

	bendpoints_ += intersowing ? eventidx : bpfinder.bendPoints()[eventidx];
	if ( intersowing && bpfinder.bendPoints().indexOf(eventidx)>=0 )
	    bendpoints_ += eventidx;
    }

    if ( reversesowingorder_ )
	bendpoints_.reverse();

    if ( intersowing )
	bendpoints_[0] = bendpoints_[bendpoints_.size()-1];

    mode_ = FirstSowing;
    int count = 0;
    while ( bendpoints_.size() )
    {
	int eventidx = bendpoints_[0];
	for ( int yn=1; yn>=0; yn-- )
	{
	    eventlist_[eventidx]->pressed = yn;
	    if ( eventcatcher_ )
		eventcatcher_->reHandle( *eventlist_[eventidx] );
	}

	bendpoints_.remove( 0 );

	if ( !intersowing || count++ )
	    mode_ = SequentSowing;
    }

    reset();
    mReturnHandled( true );
}


void Sower::reset()
{
    sowingline_->turnOn( false );
    for ( int idx=sowingline_->size()-1; idx>=0; idx-- )
	sowingline_->removePoint( idx );

    deepErase( eventlist_ );
    mousecoords_.erase();

    mode_ = Idle;
}


bool Sower::acceptTablet( const visBase::EventInfo& eventinfo )
{
    if ( !eventinfo.tabletinfo )
	mReturnHandled( false );

    const EM::PosID pid = getMarkerID( eventinfo );
    if ( pid != curpid_ )
    {
	curpidstamp_ = Time::getMilliSeconds();
	curpid_ = pid;
    }

    if ( eventinfo.tabletinfo->pointertype_ == TabletInfo::Eraser )
    {
	if ( !pid.isUdf() )
	    return acceptEraser( eventinfo );

	for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
	{
	    const int visid = eventinfo.pickedobjids[idx];
	    visBase::DataObject* dataobj = visBase::DM().getObject( visid );
	    mDynamicCastGet( const visBase::Marker*, marker, dataobj );
	    if ( marker )
		return acceptEraser( eventinfo );
	}

	mReturnHandled( true );
    }

    if ( mode_==Idle && eventinfo.type==visBase::MouseMovement &&
	 !pid.isUdf() && !mIsUdf(curpidstamp_) &&
	 Time::passedSince(curpidstamp_) > 300 )
    {
	curpidstamp_ = mUdf(int);
	return acceptLaser( eventinfo );
    }

    if ( !pid.isUdf() && mode_==Furrowing && singleseeded_ )
	sowingline_->turnOn( false );

    return acceptMouse( eventinfo );
}


bool Sower::acceptLaser( const visBase::EventInfo& eventinfo )
{
    if ( mode_ != Idle )
	mReturnHandled( false );

    mode_ = Lasering;

    visBase::EventInfo newevent( eventinfo );
    newevent.type = visBase::MouseClick;

    int butstate = newevent.buttonstate_ | lasermask_;
    newevent.buttonstate_ = (OD::ButtonState) butstate;

    for ( int yn=1; yn>=0; yn-- )
    {
	newevent.pressed = yn;
	if ( eventcatcher_ )
	    eventcatcher_->reHandle( newevent );
    }

    mode_ = Idle;
    mReturnHandled( true );
}


bool Sower::acceptEraser( const visBase::EventInfo& eventinfo )
{
    if ( mode_ != Idle )
	mReturnHandled( false );

    if ( eventinfo.type==visBase::MouseMovement &&
	 !eventinfo.tabletinfo->pressure_ )
	mReturnHandled( false );

    mode_ = Erasing;

    visBase::EventInfo newevent( eventinfo );
    newevent.type = visBase::MouseClick;

    int butstate = newevent.buttonstate_ | erasermask_;
    newevent.buttonstate_ = (OD::ButtonState) butstate;

    for ( int yn=1; yn>=0; yn-- )
    {
	newevent.pressed = yn;
	if ( eventcatcher_ )
	    eventcatcher_->reHandle( newevent );
    }

    mode_ = Idle;
    mReturnHandled( true );
}


void Sower::setSequentSowMask( bool yn, OD::ButtonState mask )
{ sequentsowmask_ = yn ? mask : OD::ButtonState(~OD::NoButton); }


void Sower::setIfDragInvertMask( bool yn, OD::ButtonState mask )
{ ifdraginvertmask_ = yn ? mask : OD::NoButton; }


void Sower::setLaserMask( bool yn, OD::ButtonState mask )
{ lasermask_ = yn ? mask : OD::NoButton; }


void Sower::setEraserMask( bool yn, OD::ButtonState mask )
{ erasermask_ = yn ? mask : OD::NoButton; }


EM::PosID Sower::getMarkerID( const visBase::EventInfo& eventinfo ) const
{
    if ( !editobject_ ) return EM::PosID::udf();

    mDynamicCastGet( const MPEEditor*, mpeeditor, editobject_ );
    if ( mpeeditor )
	return mpeeditor->mouseClickDragger( eventinfo.pickedobjids );

    mDynamicCastGet( const LocationDisplay*, locdisp, editobject_ );
    if ( locdisp )
    {
	const int knotid = locdisp->isMarkerClick( eventinfo.pickedobjids );
	return knotid<0 ? EM::PosID::udf() : EM::PosID( 0, 0, knotid );
    }

    return EM::PosID::udf();
}


}; //namespace
