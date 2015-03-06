/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "basemaprandomline.h"

#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "randomlinegeom.h"
#include "randomlinetr.h"
#include "survinfo.h"

namespace Basemap
{

RandomLineObject::RandomLineObject( const MultiID& mid )
    : BaseMapObject(0)
    , mid_(mid)
    , rdlset_(*new Geometry::RandomLineSet)
{
    setMultiID( mid );
}


RandomLineObject::~RandomLineObject()
{
    delete &rdlset_;
}


void RandomLineObject::setMultiID( const MultiID& mid )
{
    mid_ = mid;
    PtrMan<IOObj> ioobj = IOM().get( mid_ );
    if ( !ioobj ) return;

    setName( ioobj->name() );

    BufferString msg;
    rdlset_.setEmpty();
    const bool res = RandomLineSetTranslator::retrieve( rdlset_, ioobj, msg );
    if ( !res ) return;
}


void RandomLineObject::setLineStyle( const LineStyle& ls )
{
    ls_ = ls;
}


void RandomLineObject::setLineStyle( int shapeidx, const LineStyle& ls )
{
    setLineStyle( ls );
}


void RandomLineObject::updateGeometry()
{ changed.trigger(); }


int RandomLineObject::nrShapes() const
{ return rdlset_.isEmpty() ? 0 : 1; }


const char* RandomLineObject::getShapeName( int idx ) const
{ return name().buf(); }


void RandomLineObject::getPoints( int shapeidx, TypeSet<Coord>& pts ) const
{
    const Geometry::RandomLine* rdl = rdlset_.lines()[0];
    const int nrnodes = rdl->nrNodes();
    for ( int idx=0; idx<nrnodes; idx++ )
	pts += SI().transform( rdl->nodePosition(idx) );
}


const MarkerStyle2D* RandomLineObject::getMarkerStyle( int ) const
{ return 0; }


void RandomLineObject::setMarkerStyle( int, const MarkerStyle2D& ms )
{ ms_ = ms; }

} // namespace Basemap
