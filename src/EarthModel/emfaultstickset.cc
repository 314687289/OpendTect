/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          November 2008
________________________________________________________________________

-*/

#include "emfaultstickset.h"

#include "emmanager.h"
#include "emrowcoliterator.h"
#include "emsurfacetr.h"
#include "dbman.h"
#include "ioobj.h"
#include "iopar.h"
#include "posfilter.h"
#include "undo.h"


namespace EM {

mImplementEMObjFuncs(FaultStickSet,EMFaultStickSetTranslatorGroup::sGroupName())

FaultStickSet::FaultStickSet( EMManager& em )
    : Fault(em)
    , geometry_( *this )
{
    geometry_.addSection( "", false );
}

mImplMonitorableAssignment( FaultStickSet, SharedObject )


FaultStickSet::FaultStickSet( const FaultStickSet& oth )
    : Fault(oth)
    , geometry_(oth.geometry_)
{
    copyClassData( oth );
}


FaultStickSet::~FaultStickSet()
{}


void FaultStickSet::copyClassData( const FaultStickSet& oth )
{
    id_ = oth.id_;
    preferredcolor_ = oth.preferredcolor_;
    changed_ = oth.changed_;
    fullyloaded_ = oth.fullyloaded_;
    locked_ = oth.locked_;
    burstalertcount_ = oth.fullyloaded_;
    selremoving_ = oth.selremoving_;
    preferredlinestyle_ = oth.preferredlinestyle_;
    preferredmarkerstyle_ = oth.preferredmarkerstyle_;
    selectioncolor_ = oth.selectioncolor_;
    haslockednodes_ = oth.haslockednodes_;
}



uiString FaultStickSet::getUserTypeStr() const
{ return EMFaultStickSetTranslatorGroup::sTypeName(); }


void FaultStickSet::apply( const Pos::Filter& pf )
{
    for ( int idx=0; idx<nrSections(); idx++ )
    {
	mDynamicCastGet( Geometry::FaultStickSet*, fssg,
			 sectionGeometry(sectionID(idx)) );
	if ( !fssg ) continue;

	const StepInterval<int> rowrg = fssg->rowRange();
	if ( rowrg.isUdf() ) continue;

	RowCol rc;
	for ( rc.row()=rowrg.stop; rc.row()>=rowrg.start; rc.row()-=rowrg.step )
	{
	    const StepInterval<int> colrg = fssg->colRange( rc.row() );
	    if ( colrg.isUdf() ) continue;

	    for ( rc.col()=colrg.stop; rc.col()>=colrg.start;
							rc.col()-=colrg.step )
	    {
		const Coord3 pos = fssg->getKnot( rc );
		if ( !pf.includes( pos.getXY(), (float) pos.z_) )
		    fssg->removeKnot( rc );
	    }
	}
    }

    // TODO: Handle case in which fault sticks become fragmented.
}


FaultStickSetGeometry& FaultStickSet::geometry()
{ return geometry_; }


const FaultStickSetGeometry& FaultStickSet::geometry() const
{ return geometry_; }


const IOObjContext& FaultStickSet::getIOObjContext() const
{ return EMFaultStickSetTranslatorGroup::ioContext(); }


bool FaultStickSet::pickedOnPlane( int row  ) const
{
    return geometry().pickedOnPlane( 0, row );
}


bool FaultStickSet::pickedOn2DLine( int row  ) const
{
    return geometry().pickedOn2DLine( 0, row );
}


Pos::GeomID FaultStickSet::pickedGeomID( int row  ) const
{
    return geometry().pickedGeomID( 0, row );
}


const DBKey* FaultStickSet::pickedDBKey( int sticknr ) const
{
    return geometry().pickedDBKey( 0, sticknr );
}


const char* FaultStickSet::pickedName( int sticknr ) const
{
    return geometry().pickedName( 0, sticknr );
}


// ***** FaultStickSetGeometry *****


FaultStickSetGeometry::StickInfo::StickInfo()
    : pickedgeomid(Survey::GeometryManager::cUndefGeomID())
{}


FaultStickSetGeometry::FaultStickSetGeometry( Surface& surf )
    : FaultGeometry(surf)
{}


FaultStickSetGeometry::~FaultStickSetGeometry()
{}


Geometry::FaultStickSet*
FaultStickSetGeometry::sectionGeometry( const SectionID& sid )
{
    Geometry::Element* res = SurfaceGeometry::sectionGeometry( sid );
    return (Geometry::FaultStickSet*) res;
}


const Geometry::FaultStickSet*
FaultStickSetGeometry::sectionGeometry( const SectionID& sid ) const
{
    const Geometry::Element* res = SurfaceGeometry::sectionGeometry( sid );
    return (const Geometry::FaultStickSet*) res;
}


Geometry::FaultStickSet* FaultStickSetGeometry::createSectionGeometry() const
{ return new Geometry::FaultStickSet; }


EMObjectIterator* FaultStickSetGeometry::createIterator( const SectionID& sid,
					const TrcKeyZSampling* cs ) const
{ return new RowColIterator( surface_, sid, cs ); }


int FaultStickSetGeometry::nrSticks( const SectionID& sid ) const
{
    const Geometry::FaultStickSet* fss = sectionGeometry( sid );
    return fss ? fss->nrSticks(): 0;
}


int FaultStickSetGeometry::nrKnots( const SectionID& sid, int sticknr ) const
{
    const Geometry::FaultStickSet* fss = sectionGeometry( sid );
    return fss ? fss->nrKnots(sticknr) : 0;
}


#define mTriggerSurfaceChange( surf ) \
    surf.setChangedFlag(); \
    EMObjectCallbackData cbdata; \
    cbdata.event = EMObjectCallbackData::BurstAlert; \
    surf.change.trigger( cbdata );


bool FaultStickSetGeometry::insertStick( const SectionID& sid, int sticknr,
					 int firstcol, const Coord3& pos,
					 const Coord3& editnormal,
					 bool addtohistory )
{
    return insertStick( sid, sticknr, firstcol, pos, editnormal, 0, 0,
			addtohistory );
}


bool FaultStickSetGeometry::insertStick( const SectionID& sid, int sticknr,
					 int firstcol, const Coord3& pos,
					 const Coord3& editnormal,
					 const DBKey* pickeddbkey,
					 const char* pickednm,
					 bool addtohistory )
{
    Geometry::FaultStickSet* fss = sectionGeometry( sid );

    if ( !fss )
	return false;

    const bool firstrowchange = sticknr < fss->rowRange().start;

    if ( !fss->insertStick(pos,editnormal,sticknr,firstcol) )
	return false;

    for ( int idx=0; !firstrowchange && idx<stickinfo_.size(); idx++ )
    {
	if ( stickinfo_[idx]->sid==sid && stickinfo_[idx]->sticknr>=sticknr )
	    stickinfo_[idx]->sticknr++;
    }

    stickinfo_.insertAt( new StickInfo, 0 );
    stickinfo_[0]->sid = sid;
    stickinfo_[0]->sticknr = sticknr;
    stickinfo_[0]->pickeddbkey = pickeddbkey ? *pickeddbkey
					     : DBKey::getInvalid();
    stickinfo_[0]->pickednm = pickednm;
    if ( addtohistory )
    {
	const PosID posid( surface_.id(),sid,RowCol(sticknr,0).toInt64());
	UndoEvent* undo = new FaultStickUndoEvent( posid );
	FSSMan().undo().addEvent( undo, 0 );
    }

    mTriggerSurfaceChange( surface_ );
    return true;
}


bool FaultStickSetGeometry::insertStick( const SectionID& sid, int sticknr,
					 int firstcol, const Coord3& pos,
					 const Coord3& editnormal,
					 Pos::GeomID pickedgeomid,
					 bool addtohistory )
{
    Geometry::FaultStickSet* fss = sectionGeometry( sid );

    if ( !fss )
	return false;

    const bool firstrowchange = sticknr < fss->rowRange().start;

    if ( !fss->insertStick(pos,editnormal,sticknr,firstcol) )
	return false;

    for ( int idx=0; !firstrowchange && idx<stickinfo_.size(); idx++ )
    {
	if ( stickinfo_[idx]->sid==sid && stickinfo_[idx]->sticknr>=sticknr )
	    stickinfo_[idx]->sticknr++;
    }

    stickinfo_.insertAt( new StickInfo, 0 );
    stickinfo_[0]->sid = sid;
    stickinfo_[0]->sticknr = sticknr;
    stickinfo_[0]->pickedgeomid = pickedgeomid;
    if ( addtohistory )
    {
	const PosID posid( surface_.id(),sid,RowCol(sticknr,0).toInt64());
	UndoEvent* undo = new FaultStickUndoEvent( posid );
	FSSMan().undo().addEvent( undo, 0 );
    }

    mTriggerSurfaceChange( surface_ );
    return true;
}


bool FaultStickSetGeometry::removeStick( const SectionID& sid, int sticknr,
					 bool addtohistory )
{
    Geometry::FaultStickSet* fss = sectionGeometry( sid );
    if ( !fss )
	return false;

    const StepInterval<int> colrg = fss->colRange( sticknr );
    if ( colrg.isUdf() || colrg.width() )
	return false;

    const RowCol rc( sticknr, colrg.start );

    const Coord3 pos = fss->getKnot( rc );
    const Coord3 normal = getEditPlaneNormal( sid, sticknr );
    if ( !normal.isDefined() || !pos.isDefined() )
	return false;

    if ( !fss->removeStick(sticknr) )
	return false;

    for ( int idx=stickinfo_.size()-1; idx>=0; idx-- )
    {
	if ( stickinfo_[idx]->sid != sid )
	    continue;

	if ( stickinfo_[idx]->sticknr == sticknr )
	{
	    delete stickinfo_.removeSingle( idx );
	    continue;
	}

	if ( sticknr>=fss->rowRange().start && stickinfo_[idx]->sticknr>sticknr)
	    stickinfo_[idx]->sticknr--;
    }

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), sid, rc.toInt64() );
	UndoEvent* undo = new FaultStickUndoEvent( posid, pos, normal );
	FSSMan().undo().addEvent( undo, 0 );
    }

    mTriggerSurfaceChange( surface_ );
    return true;
}


bool FaultStickSetGeometry::insertKnot( const SectionID& sid,
					const SubID& subid, const Coord3& pos,
					bool addtohistory )
{
    Geometry::FaultStickSet* fss = sectionGeometry( sid );
    RowCol rc = RowCol::fromInt64( subid );
    if ( !fss || !fss->insertKnot(rc,pos) )
	return false;

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), sid, subid );
	UndoEvent* undo = new FaultKnotUndoEvent( posid );
	FSSMan().undo().addEvent( undo, 0 );
    }

    mTriggerSurfaceChange( surface_ );
    return true;
}


bool FaultStickSetGeometry::removeKnot( const SectionID& sid,
					const SubID& subid, bool addtohistory )
{
    Geometry::FaultStickSet* fss = sectionGeometry( sid );
    if ( !fss ) return false;

    RowCol rc = RowCol::fromInt64( subid );
    const Coord3 pos = fss->getKnot( rc );

    if ( !pos.isDefined() || !fss->removeKnot(rc) )
	return false;

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), sid, subid );
	UndoEvent* undo = new FaultKnotUndoEvent( posid, pos );
	FSSMan().undo().addEvent( undo, 0 );
    }

    mTriggerSurfaceChange( surface_ );
    return true;
}


bool FaultStickSetGeometry::pickedOnPlane( const SectionID& sid,
					   int sticknr ) const
{
    if ( pickedDBKey(sid,sticknr) || pickedOn2DLine(sid,sticknr) )
	return false;

    const Coord3& editnorm = getEditPlaneNormal( sid, sticknr );
    return editnorm.isDefined();
}


bool FaultStickSetGeometry::pickedOnHorizon( const SectionID& sid,
					     int sticknr ) const
{
    const Coord3& editnorm = getEditPlaneNormal( sid, sticknr );
    return !pickedOnPlane(sid,sticknr) &&
	   editnorm.isDefined() && fabs(editnorm.z_)>0.5;
}


bool FaultStickSetGeometry::pickedOn2DLine( const SectionID& sid,
					    int sticknr ) const
{
    return pickedGeomID(sid,sticknr) != Survey::GeometryManager::cUndefGeomID();
}


const DBKey* FaultStickSetGeometry::pickedDBKey( const SectionID& sid,
						     int sticknr) const
{
    int idx = indexOf(sid,sticknr);
    if ( idx >= 0 )
    {
	const DBKey& pickeddbkey = stickinfo_[idx]->pickeddbkey;
	return pickeddbkey==DBKey::getInvalid() ? 0 : &pickeddbkey;
    }

    return 0;
}


const char* FaultStickSetGeometry::pickedName( const SectionID& sid,
					       int sticknr) const
{
    int idx = indexOf(sid,sticknr);
    if ( idx >= 0 )
    {
        const char* pickednm = stickinfo_[idx]->pickednm.buf();
        return *pickednm ? pickednm : 0;
    }

    return 0;
}


Pos::GeomID FaultStickSetGeometry::pickedGeomID( const SectionID& sid,
						 int sticknr) const
{
    int idx = indexOf(sid,sticknr);
    if ( idx >= 0 )
	return stickinfo_[idx]->pickedgeomid;

    return Survey::GeometryManager::cUndefGeomID();
}


#define mDefStickInfoStr( prefixstr, stickinfostr, sid, sticknr ) \
    BufferString stickinfostr(prefixstr); stickinfostr += " of section "; \
    stickinfostr += sid; stickinfostr += " sticknr "; stickinfostr += sticknr;

#define mDefEditNormalStr( editnormalstr, sid, sticknr ) \
    mDefStickInfoStr( "Edit normal", editnormalstr, sid, sticknr )
#define mDefLineSetStr( linesetstr, sid, sticknr ) \
    mDefStickInfoStr( "Line set", linesetstr, sid, sticknr )
#define mDefLineNameStr( linenamestr, sid, sticknr ) \
    mDefStickInfoStr( "Line name", linenamestr, sid, sticknr )
#define mDefPickedDBKeyStr( pickeddbkeystr, sid, sticknr ) \
    mDefStickInfoStr( "Picked DBKey", pickeddbkeystr, sid, sticknr )
#define mDefPickedNameStr( pickednmstr, sid, sticknr ) \
    mDefStickInfoStr( "Picked name", pickednmstr, sid, sticknr )
#define mDefPickedGeomIDStr( pickedgeomidstr, sid, sticknr ) \
	mDefStickInfoStr( "Picked GeomID", pickedgeomidstr, sid, sticknr )
#define mDefL2DKeyStr( l2dkeystr, sid, sticknr ) \
	mDefStickInfoStr( "GeomID", l2dkeystr, sid, sticknr )



void FaultStickSetGeometry::fillPar( IOPar& par ) const
{
    for ( int idx=0; idx<nrSections(); idx++ )
    {
	const EM::SectionID sid = sectionID( idx );
	const Geometry::FaultStickSet* fss = sectionGeometry( sid );
	if ( !fss ) continue;

	StepInterval<int> stickrg = fss->rowRange();
	for ( int sticknr=stickrg.start; sticknr<=stickrg.stop; sticknr++ )
	{
	    mDefEditNormalStr( editnormalstr, sid, sticknr );
	    par.set( editnormalstr.buf(), fss->getEditPlaneNormal(sticknr) );
	    const DBKey* pickeddbkey = pickedDBKey( sid, sticknr );
	    if ( pickeddbkey )
	    {
		mDefPickedDBKeyStr( pickeddbkeystr, sid, sticknr );
		par.set( pickeddbkeystr.buf(), *pickeddbkey );
	    }

	    const char* pickednm = pickedName( sid, sticknr );
	    if ( pickednm )
	    {
		mDefPickedNameStr( pickednmstr, sid, sticknr );
		par.set( pickednmstr.buf(), pickednm );
	    }

	    Pos::GeomID geomid = pickedGeomID( sid, sticknr );
	    if ( geomid != Survey::GeometryManager::cUndefGeomID() )
	    {
		mDefPickedGeomIDStr( pickedgeomidstr, sid, sticknr );
		par.set( pickedgeomidstr.buf(), geomid );
	    }
	}
    }
}


bool FaultStickSetGeometry::usePar( const IOPar& par )
{
    for ( int idx=0; idx<nrSections(); idx++ )
    {
	const EM::SectionID sid = sectionID( idx );
	Geometry::FaultStickSet* fss = sectionGeometry( sid );
	if ( !fss ) return false;

	StepInterval<int> stickrg = fss->rowRange();
	for ( int sticknr=stickrg.start; sticknr<=stickrg.stop; sticknr++ )
	{
	    mDefEditNormalStr( editnormstr, sid, sticknr );
	    Coord3 editnormal( Coord3::udf() );
	    par.get( editnormstr.buf(), editnormal );
	    fss->setEditPlaneNormal( sticknr, editnormal );

	    stickinfo_.insertAt( new StickInfo, 0 );
	    stickinfo_[0]->sid = sid;
	    stickinfo_[0]->sticknr = sticknr;

	    mDefPickedGeomIDStr( pickedgeomidstr, sid, sticknr );
	    if ( par.get(pickedgeomidstr.buf(), stickinfo_[0]->pickedgeomid) )
		continue;

	    mDefL2DKeyStr( l2dkeystr, sid, sticknr );
	    BufferString keybuf;
	    if ( par.get(l2dkeystr.buf(),keybuf) )
	    {
		PosInfo::Line2DKey l2dkey;
		l2dkey.fromString( keybuf );
		if ( S2DPOS().curLineSetID() != l2dkey.lsID() )
		    S2DPOS().setCurLineSet( l2dkey.lsID() );

		stickinfo_[0]->pickedgeomid = Survey::GM().getGeomID(
				    S2DPOS().getLineSet(l2dkey.lsID()),
				    S2DPOS().getLineName(l2dkey.lineID()) );
		continue;
	    }

	    mDefPickedDBKeyStr( pickeddbkeystr, sid, sticknr );
	    mDefLineSetStr( linesetstr, sid, sticknr );
	    if ( !par.get(pickeddbkeystr.buf(), stickinfo_[0]->pickeddbkey))
		par.get( linesetstr.buf(), stickinfo_[0]->pickeddbkey );
	    mDefPickedNameStr( pickednmstr, sid, sticknr );
	    mDefLineNameStr( linenamestr, sid, sticknr );

	    if ( !par.get(pickednmstr.buf(), stickinfo_[0]->pickednm) )
		par.get( linenamestr.buf(), stickinfo_[0]->pickednm );

	    PtrMan<IOObj> pickedioobj = DBM().get( stickinfo_[0]->pickeddbkey );
	    if ( pickedioobj )
		stickinfo_[0]->pickedgeomid =
			Survey::GM().getGeomID( pickedioobj->name(),
						stickinfo_[0]->pickednm );
	}
    }

    return true;
}


int FaultStickSetGeometry::indexOf( const SectionID& sid, int sticknr ) const
{
    for ( int idx=0; idx<stickinfo_.size(); idx++ )
    {
	if ( stickinfo_[idx]->sid==sid && stickinfo_[idx]->sticknr==sticknr )
	    return idx;
    }

    return -1;
}


} // namespace EM
