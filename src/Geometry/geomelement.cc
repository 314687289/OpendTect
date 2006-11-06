/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2004
-*/

static const char* rcsID = "$Id: geomelement.cc,v 1.7 2006-11-06 10:46:28 cvsjaap Exp $";

#include "geomelement.h"
#include "survinfo.h"

namespace Geometry
{
Element::Element()
    : nrpositionnotifier( this )
    , movementnotifier( this )
    , ischanged_( false )
    , errmsg_( 0 )
    , blockcbs_( false )
{ }


Element::~Element()
{ delete errmsg_; }


IntervalND<float> Element::boundingBox(bool) const
{
    TypeSet<GeomPosID> ids;
    getPosIDs( ids );

    IntervalND<float> bbox(3);
    Coord3 pos;
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	pos = getPosition(ids[idx]);
	//if ( !pos.isDefined() )
	    //continue;

	if ( !bbox.isSet() ) bbox.setRange(pos);
	else bbox.include(pos);
    }

    return bbox;
}



const char* Element::errMsg() const
{ return errmsg_ && errmsg_->size() ? (const char*) (*errmsg_) : 0; }


BufferString& Element::errmsg()
{
    if ( !errmsg_ ) errmsg_ = new BufferString;
    return *errmsg_;
}


void Element::blockCallBacks( bool yn, bool flush )
{
    if ( flush )
    {
	blockcbs_ = false;
	triggerNrPosCh( nrposchbuffer_ );
	triggerMovement( movementbuffer_ );
    }

    blockcbs_ = yn;

    if ( blockcbs_ && !flush )
	return;
    
    nrposchbuffer_.erase();
    movementbuffer_.erase();
}


void Element::triggerMovement( const TypeSet<GeomPosID>& gpids )
{
    if ( !gpids.size() ) return;

    if ( blockcbs_ )
	movementbuffer_.append( gpids );
    else
	movementnotifier.trigger( &gpids, this );

    ischanged_ = true;
}


void Element::triggerMovement( const GeomPosID& gpid )
{
    TypeSet<GeomPosID> gpids( 1, gpid );
    triggerMovement( gpids );
}


void Element::triggerMovement()
{
    if ( blockcbs_ )
	getPosIDs( movementbuffer_, true );
    else
	movementnotifier.trigger( 0, this );

    ischanged_ = true;
}


void Element::triggerNrPosCh( const TypeSet<GeomPosID>& gpids )
{
    if ( !gpids.size() ) return;

    if ( blockcbs_ )
	nrposchbuffer_.append( gpids );
    else
	nrpositionnotifier.trigger( &gpids, this );

    ischanged_ = true;
}


void Element::triggerNrPosCh( const GeomPosID& gpid )
{
    TypeSet<GeomPosID> gpids( 1, gpid );
    triggerNrPosCh( gpids );
}


void Element::triggerNrPosCh()
{
    if ( blockcbs_ )
	getPosIDs( nrposchbuffer_, true );
    else
	nrpositionnotifier.trigger( 0, this );
    ischanged_ = true;
}

}; //Namespace

