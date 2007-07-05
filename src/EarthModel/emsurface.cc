/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: emsurface.cc,v 1.87 2007-07-05 17:27:24 cvskris Exp $
________________________________________________________________________

-*/

#include "emsurface.h"

#include "cubesampling.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emrowcoliterator.h"
#include "emsurfacegeometry.h"
#include "emsurfaceiodata.h"
#include "emsurfaceauxdata.h"
#include "emsurfacerelations.h"
#include "iopar.h"


static const char* sDbInfo = "DB Info";
static const char* sRange = "Range";
static const char* sValnms = "Value Names";
static const char* sSections = "Patches";


using namespace EM;


void SurfaceIOData::clear()
{
    rg.init(false);
    dbinfo = "";
    deepErase(valnames);
    deepErase(sections);
}


void SurfaceIOData::use( const Surface& surf )
{
    clear();

    mDynamicCastGet( const RowColSurfaceGeometry*, rcsg, &surf.geometry() );
    if ( rcsg )
    {
	StepInterval<int> hrg = rcsg->rowRange();
	rg.start.inl = hrg.start; rg.stop.inl = hrg.stop;
	rg.step.inl = hrg.step;
	hrg = rcsg->colRange();
	rg.start.crl = hrg.start; rg.stop.crl = hrg.stop;
	rg.step.crl = hrg.step;
    }

    for ( int idx=0; idx<surf.nrSections(); idx++ )
	sections += new BufferString( surf.sectionName( surf.sectionID(idx) ) );

    mDynamicCastGet(const Horizon3D*,horizon,&surf);
    if ( horizon )
    {
	for ( int idx=0; idx<horizon->auxdata.nrAuxData(); idx++ )
	    valnames += new BufferString( horizon->auxdata.auxDataName(idx) );
    }
}


void SurfaceIOData::fillPar( IOPar& iopar ) const
{
    iopar.set( sDbInfo, dbinfo );

    IOPar bidpar;
    rg.fillPar( bidpar );
    iopar.mergeComp( bidpar, sRange );

    IOPar valnmspar;
    valnames.fillPar( valnmspar );
    iopar.mergeComp( valnmspar, sValnms );

    IOPar sectionpar;
    sections.fillPar( sectionpar );
    iopar.mergeComp( sectionpar, sSections );
}


void SurfaceIOData::usePar( const IOPar& iopar )
{
    iopar.get( sDbInfo, dbinfo );

    IOPar* bidpar = iopar.subselect(sRange);
    if ( bidpar ) rg.usePar( *bidpar );

    IOPar* valnmspar = iopar.subselect(sValnms);
    if ( valnmspar ) valnames.usePar( *valnmspar );

    IOPar* sectionpar = iopar.subselect(sSections);
    if ( sectionpar ) sections.usePar( *sectionpar );
}


void SurfaceIODataSelection::setDefault()
{
    rg = sd.rg;
    selvalues.erase(); selsections.erase();
    for ( int idx=0; idx<sd.valnames.size(); idx++ )
	selvalues += idx;
    for ( int idx=0; idx<sd.sections.size(); idx++ )
	selsections += idx;
}


Surface::Surface( EMManager& man)
    : EMObject( man )
    , relations( *new SurfaceRelations(*this ) )
{
}


Surface::~Surface()
{ delete &relations; }

void Surface::removeAll()
{ relations.removeAll(); }

int Surface::nrSections() const
{ return geometry().nrSections(); }

SectionID Surface::sectionID( int idx ) const
{ return geometry().sectionID(idx); }

BufferString Surface::sectionName( const SectionID& sid ) const
{ return geometry().sectionName(sid); }

bool Surface::canSetSectionName() const { return true; }

bool Surface::setSectionName( const SectionID& sid, const char* nm, bool hist )
{ return geometry().setSectionName(sid,nm,hist); }

bool Surface::removeSection( SectionID sid, bool hist )
{ return geometry().removeSection( sid, hist ); }

bool Surface::isAtEdge( const PosID& posid ) const
{ return geometry().isAtEdge(posid); }

bool Surface::isLoaded() const
{ return geometry().isLoaded(); }

Executor* Surface::saver() { return geometry().saver(); }

Executor* Surface::loader() { return geometry().loader(); }

Geometry::Element* Surface::sectionGeometryInternal( const SectionID& sid )
{ return geometry().sectionGeometry(sid); }

EMObjectIterator* Surface::createIterator( const SectionID& sid, 
					   const CubeSampling* cs ) const
{ return geometry().createIterator( sid, cs ); }

bool Surface::enableGeometryChecks( bool nv )
{ return geometry().enableChecks( nv ); }

bool Surface::isGeometryChecksEnabled() const
{ return geometry().isChecksEnabled(); }

const SurfaceGeometry& Surface::geometry() const
{ return const_cast<Surface*>(this)->geometry(); }


bool Surface::usePar( const IOPar& par )
{
    return EMObject::usePar(par) && geometry().usePar(par) &&
	   relations.usePar(par);
}


void Surface::fillPar( IOPar& par ) const
{
    EMObject::fillPar( par );
    relations.fillPar( par );
    geometry().fillPar( par );
}
