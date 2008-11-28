/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID = "$Id: emhorizon3d.cc,v 1.109 2008-11-28 13:05:05 cvsjaap Exp $";

#include "emhorizon3d.h"

#include "array2dinterpol.h"
#include "arrayndimpl.h"
#include "binidsurface.h"
#include "binidvalset.h"
#include "emmanager.h"
#include "emrowcoliterator.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceedgeline.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "ioobj.h"
#include "pickset.h"
#include "ptrman.h"
#include "scaler.h"
#include "survinfo.h"
#include "tabledef.h"
#include "trigonometry.h"
#include "unitofmeasure.h"
#include "zaxistransform.h"

#include <math.h>

namespace EM {

class AuxDataImporter : public Executor
{
public:

AuxDataImporter( Horizon3D& hor, const ObjectSet<BinIDValueSet>& sects,
		 const BufferStringSet& attribnames, const int start,
       		 HorSampling hs	)
    : Executor("Data Import")
    , horizon(hor)
    , sections(sects)
    , startidx(start)
    , totalnr(0)
    , nrdone(0)
    , hs_(hs)
{
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	const BinIDValueSet& bvs = *sections[idx];
	totalnr += bvs.nrInls();
    }

    const int nrattribs = attribnames.size();
    for ( int attribidx=0; attribidx<nrattribs; attribidx++ )
    {
	BufferString nm = attribnames.get( attribidx );
	if ( nm.isEmpty() )
	    { nm = "Imported attribute "; nm += attribidx; }

	attrindexes += horizon.auxdata.addAuxData( nm.buf() );
    }

    sectionidx = -1;
}


int nextStep()
{
    if ( sectionidx == -1 || inl > inlrange.stop )
    {
	sectionidx++;
	if ( sectionidx >= horizon.geometry().nrSections() )
	    return Finished;

	SectionID sectionid = horizon.geometry().sectionID(sectionidx);
	inlrange = horizon.geometry().rowRange(sectionid);
	crlrange = horizon.geometry().colRange(sectionid);
	inl = inlrange.start;
    }

    PosID posid( horizon.id(), horizon.geometry().sectionID(sectionidx) );
    const BinIDValueSet& bvs = *sections[sectionidx];
    const int nrattribs = attrindexes.size();
    for ( int crl=crlrange.start; crl<=crlrange.stop; crl+=crlrange.step )
    {
	const BinID bid(inl,crl);
	if ( !hs_.includes(bid) ) continue;

	BinIDValueSet::Pos pos = bvs.findFirst( bid );
	if ( !pos.valid() ) continue;

	const float* vals = bvs.getVals( pos );
	if ( !vals ) continue;
	posid.setSubID( bid.getSerialized() );
	int idx = 0;
	for ( int attridx=0; attridx<nrattribs; attridx++ )
	{
	    const float val = vals[attridx+startidx];
	    if ( !mIsUdf(val) )
		horizon.auxdata.setAuxDataVal( attrindexes[idx++], posid, val );
	}
    }

    inl += inlrange.step;
    nrdone++;
    return MoreToDo;
}


const char*	message() const		{ return "Importing aux data"; }
od_int64	totalNr() const		{ return totalnr; }
od_int64	nrDone() const		{ return nrdone; }
const char*	nrDoneText() const	{ return "Nr positions imported"; }

protected:

    const ObjectSet<BinIDValueSet>&	sections;
    Horizon3D&			horizon;
    StepInterval<int>		inlrange;
    StepInterval<int>		crlrange;

    int				inl;
    int				sectionidx;
    int				startidx;
    TypeSet<int>		attrindexes;

    HorSampling			hs_;
    int				totalnr;
    int				nrdone;
};


class HorizonImporter : public Executor
{
public:

HorizonImporter( Horizon3D& hor, const ObjectSet<BinIDValueSet>& sects, 
		 const HorSampling& hs )
    : Executor("Horizon Import")
    , horizon_(hor)
    , sections_(sects)
    , totalnr_(0)
    , nrdone_(0)
    , hs_(hs)

{
    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	const BinIDValueSet& bvs = *sections_[idx];
	totalnr_ += bvs.totalSize();
	const RowCol step( hs_.step.inl, hs_.step.crl );
	horizon_.geometry().setStep( step, step );
	horizon_.geometry().addSection( 0, false );
    }

    horizon_.enableGeometryChecks(false);
    sectionidx_ = 0;
}


const char*	message() const		{ return "Importing ..."; }
od_int64	totalNr() const		{ return totalnr_; }
od_int64	nrDone() const		{ return nrdone_; }
const char*	nrDoneText() const	{ return "Nr positions imported"; }

int nextStep()
{	
    if ( sectionidx_ >= sections_.size() )
    {
	horizon_.enableGeometryChecks( true );
	return Finished;
    }

    const BinIDValueSet& bvs = *sections_[sectionidx_];
    bool haspos = bvs.next( pos_ );
    if ( !haspos )
    {
	sectionidx_++;
	pos_.i = pos_.j = -1;
	return MoreToDo;
    }

    const int nrvals = bvs.nrVals();
    TypeSet<float> vals( nrvals, mUdf(float) );
    BinID bid;
    bvs.get( pos_, bid, vals );
    if ( !hs_.includes(bid) ) return MoreToDo;

    PosID posid( horizon_.id(), horizon_.sectionID(sectionidx_),
		 bid.getSerialized() );
    bool res = horizon_.setPos( posid, Coord3(0,0,vals[0]), false );
    if ( res )
	nrdone_++;

    return MoreToDo;
}

protected:

    const ObjectSet<BinIDValueSet>&	sections_;
    Horizon3D&		horizon_;
    BinIDValueSet::Pos	pos_;
    HorSampling		hs_;

    int			sectionidx_;
    int			totalnr_;
    int			nrdone_;
};


Horizon3D::Horizon3D( EMManager& man )
    : Horizon(man)
    , geometry_( *this )
    , auxdata( *new SurfaceAuxData(*this) )
    , edgelinesets( *new EdgeLineManager(*this) )
{
    geometry_.addSection( "", false );
}


Horizon3D::~Horizon3D()
{
    delete &auxdata;
    delete &edgelinesets;
}


void Horizon3D::removeAll()
{
    Surface::removeAll();
    geometry_.removeAll();
    auxdata.removeAll();
    edgelinesets.removeAll();
}


void Horizon3D::fillPar( IOPar& par ) const
{
    Surface::fillPar( par );
    Horizon::fillPar( par );
    auxdata.fillPar( par );
    edgelinesets.fillPar( par );
}


bool Horizon3D::usePar( const IOPar& par )
{
    return Surface::usePar( par ) &&
	auxdata.usePar( par ) &&
	edgelinesets.usePar( par ) &&
	Horizon::usePar( par );
}


Horizon3DGeometry& Horizon3D::geometry()
{ return geometry_; }


const Horizon3DGeometry& Horizon3D::geometry() const
{ return geometry_; }


mImplementEMObjFuncs( Horizon3D, EMHorizon3DTranslatorGroup::keyword );


Array2D<float>* Horizon3D::createArray2D( 
		    SectionID sid, const ZAxisTransform* zaxistransform ) const
{
    if ( geometry().sectionGeometry( sid )->isEmpty() )
	return 0;

    const StepInterval<int> rowrg = geometry_.rowRange( sid );
    const StepInterval<int> colrg = geometry_.colRange( sid );

    Array2DImpl<float>* arr =
	    new Array2DImpl<float>( rowrg.nrSteps()+1, colrg.nrSteps()+1 );
    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
    {
	for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
	{
	    Coord3 pos = getPos( sid, RowCol(row,col).getSerialized() );
	    if ( zaxistransform )
		pos.z = zaxistransform->transform( pos );

	    arr->set( rowrg.getIndex(row), colrg.getIndex(col), pos.z );
	}
    }

    return arr;
}


bool Horizon3D::setArray2D( const Array2D<float>& arr, SectionID sid, 
			    bool onlyfillundefs )
{
    if ( geometry().sectionGeometry( sid )->isEmpty() )
	return 0;

    const StepInterval<int> rowrg = geometry_.rowRange( sid );
    const StepInterval<int> colrg = geometry_.colRange( sid );

    const RowCol startrc( rowrg.start, colrg.start );
    const RowCol stoprc( rowrg.stop, colrg.stop );
    geometry().sectionGeometry( sid )->expandWithUdf( startrc, stoprc );
    
    int poscount = 0;
    geometry().sectionGeometry( sid )->blockCallBacks( true, false );
    const bool didcheck = geometry().enableChecks( false );
    setBurstAlert( true );

    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
    {
	for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
	{
	    const RowCol rc( row, col );
	    Coord3 pos = getPos( sid, rc.getSerialized() );
	    if ( pos.isDefined() && onlyfillundefs )
		continue;

	    const double val = arr.get(rowrg.getIndex(row),colrg.getIndex(col));
	    if ( pos.z == val )
		continue;

	    pos.z = val;
	    setPos( sid, rc.getSerialized(), pos, false );

	    if ( ++poscount >= 10000 ) 
	    {
		geometry().sectionGeometry( sid )->blockCallBacks( true, true );
		poscount = 0;
	    }
	}
    }

    setBurstAlert(false);
    geometry().sectionGeometry( sid )->blockCallBacks( false, true );
    geometry().enableChecks( didcheck );
    geometry().sectionGeometry( sid )->trimUndefParts();
    return true;
}


const IOObjContext& Horizon3D::getIOObjContext() const
{ return EMHorizon3DTranslatorGroup::ioContext(); }


Executor* Horizon3D::importer( const ObjectSet<BinIDValueSet>& sections, 
			       const HorSampling& hs )
{
    removeAll();
    return new HorizonImporter( *this, sections, hs );
}


Executor* Horizon3D::auxDataImporter( const ObjectSet<BinIDValueSet>& sections,
				      const BufferStringSet& attribnms,
       				      const int start, const HorSampling& hs )
{
    return new AuxDataImporter( *this, sections, attribnms, start, hs );
}



Horizon3DGeometry::Horizon3DGeometry( Surface& surf )
    : HorizonGeometry( surf ) 
    , step_( SI().inlStep(), SI().crlStep() )
    , loadedstep_( SI().inlStep(), SI().crlStep() )
    , shift_( 0 )
    , checksupport_( true )
{}


const Geometry::BinIDSurface*
Horizon3DGeometry::sectionGeometry( const SectionID& sid ) const
{
    return (const Geometry::BinIDSurface*) SurfaceGeometry::sectionGeometry(sid);
}


Geometry::BinIDSurface*
Horizon3DGeometry::sectionGeometry( const SectionID& sid )
{
    return (Geometry::BinIDSurface*) SurfaceGeometry::sectionGeometry(sid);
}


RowCol Horizon3DGeometry::loadedStep() const
{
    return loadedstep_;
}


RowCol Horizon3DGeometry::step() const
{
    return step_;
}


void Horizon3DGeometry::setStep( const RowCol& ns, const RowCol& loadedstep )
{
    if ( ns.row && ns.col )
    {
	step_.row = abs( ns.row ); step_.col = abs( ns.col );
    }

    if ( loadedstep.row && loadedstep.col )
    {
	loadedstep_.row = abs( loadedstep.row );
	loadedstep_.col = abs( loadedstep.col );
    }

    if ( nrSections() )
        pErrMsg("Hey, this can only be done without sections.");
}


bool Horizon3DGeometry::isFullResolution() const
{
    return loadedstep_ == step_;
}


bool Horizon3DGeometry::removeSection( const SectionID& sid, bool addtoundo )
{
    int idx=sids_.indexOf(sid);
    if ( idx==-1 ) return false;

    ((Horizon3D&) surface_).edgelinesets.removeSection( sid );
    ((Horizon3D&) surface_).auxdata.removeSection( sid );
    return SurfaceGeometry::removeSection( sid, addtoundo );
}


SectionID Horizon3DGeometry::cloneSection( const SectionID& sid )
{
    const SectionID res = SurfaceGeometry::cloneSection(sid);
    if ( res!=-1 )
	((Horizon3D&) surface_).edgelinesets.cloneEdgeLineSet( sid, res );

    return res;
}


void Horizon3DGeometry::setShift( float ns )
{ shift_ = ns; }


float Horizon3DGeometry::getShift() const
{ return shift_; }


bool Horizon3DGeometry::enableChecks( bool yn )
{
    const bool res = checksupport_;
    for ( int idx=0; idx<sections_.size(); idx++ )
	((Geometry::BinIDSurface*)sections_[idx])->checkSupport(yn);

    checksupport_ = yn;

    return res;
}


bool Horizon3DGeometry::isChecksEnabled() const
{
    return checksupport_;
}


bool Horizon3DGeometry::isNodeOK( const PosID& pid ) const
{
    const Geometry::BinIDSurface* surf = sectionGeometry( pid.sectionID() );
    return surf ? surf->hasSupport( RowCol(pid.subID()) ) : false;
}


bool Horizon3DGeometry::isAtEdge( const PosID& pid ) const
{
    if ( !surface_.isDefined( pid ) )
	return false;

    return !surface_.isDefined(getNeighbor(pid,RowCol(0,1))) ||
	   !surface_.isDefined(getNeighbor(pid,RowCol(1,0))) ||
	   !surface_.isDefined(getNeighbor(pid,RowCol(0,-1))) ||
	   !surface_.isDefined(getNeighbor(pid,RowCol(-1,0)));
}


PosID Horizon3DGeometry::getNeighbor( const PosID& posid,
				      const RowCol& dir ) const
{
    const RowCol rc( posid.subID() );
    const SectionID sid = posid.sectionID();

    const StepInterval<int> rowrg = rowRange( sid );
    const StepInterval<int> colrg = colRange( sid, rc.row );

    RowCol diff(0,0);
    if ( dir.row>0 ) diff.row = rowrg.step;
    else if ( dir.row<0 ) diff.row = -rowrg.step;

    if ( dir.col>0 ) diff.col = colrg.step;
    else if ( dir.col<0 ) diff.col = -colrg.step;

    TypeSet<PosID> aliases;
    getLinkedPos( posid, aliases );
    aliases += posid;

    const int nraliases = aliases.size();
    for ( int idx=0; idx<nraliases; idx++ )
    {
	const RowCol ownrc(aliases[idx].subID());
	const RowCol neigborrc = ownrc+diff;
	if ( surface_.isDefined(aliases[idx].sectionID(),
	     neigborrc.getSerialized()) )
	{
	    return PosID( surface_.id(), aliases[idx].sectionID(),
			  neigborrc.getSerialized() );
	}
    }

    const RowCol ownrc(posid.subID());
    const RowCol neigborrc = ownrc+diff;

    return PosID( surface_.id(), posid.sectionID(), neigborrc.getSerialized());
}


int Horizon3DGeometry::getConnectedPos( const PosID& posid,
				        TypeSet<PosID>* res ) const
{
    int rescount = 0;
    const TypeSet<RowCol>& dirs = RowCol::clockWiseSequence();
    for ( int idx=dirs.size()-1; idx>=0; idx-- )
    {
	const PosID neighbor = getNeighbor( posid, dirs[idx] );
	if ( surface_.isDefined( neighbor ) )
	{
	    rescount++;
	    if ( res ) (*res) += neighbor;
	}
    }

    return rescount;
}


Geometry::BinIDSurface* Horizon3DGeometry::createSectionGeometry() const
{
    Geometry::BinIDSurface* res = new Geometry::BinIDSurface( loadedstep_ );
    res->checkSupport( checksupport_ );

    return res;
}


bool Horizon3DGeometry::getBoundingPolygon( const SectionID& sid,
					    Pick::Set& set ) const
{
    set.erase();
    const Geometry::BinIDSurface* surf = sectionGeometry( sid );
    if ( !surf ) return false;

    StepInterval<int> rowrg = rowRange( sid );
    StepInterval<int> colrg = colRange( sid, rowrg.start );
    SubID subid; PosID posid;
    bool nodefound = false;
    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
    {
	for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
	{
	    subid = RowCol( row, col ).getSerialized();
	    posid = PosID( surface_.id(), sid, subid );
	    if ( isNodeOK(posid) && isAtEdge(posid) )
	    {
		nodefound = true;
		break;
	    }
	}

	if ( nodefound ) break;
    }

    if ( !nodefound ) return false;

    const PosID firstposid = posid;
    while ( true )
    {
	Coord3 pos = surf->getKnot( RowCol(posid.subID()), false );
	set += Pick::Location( pos );

	nodefound = false;
	const TypeSet<RowCol>& dirs = RowCol::clockWiseSequence();
	if ( dirs.size() != 8 ) return false;

	for ( int idx=0; idx<dirs.size(); idx++ )
	{
	    PosID curposid, leftposid, rightposid;
	    curposid = leftposid = rightposid = posid;
	    RowCol rcol( curposid.subID() );
	    RowCol currcol = rcol + dirs[idx];
	    int leftidx = idx ? idx - 1 : 7;
	    int rightidx = idx < 7 ? idx + 1 : 0;
	    RowCol leftrcol = rcol + dirs[leftidx];
	    RowCol rightrcol = rcol + dirs[rightidx];
	    curposid.setSubID( currcol.getSerialized() );
	    leftposid.setSubID( leftrcol.getSerialized() );
	    rightposid.setSubID( rightrcol.getSerialized() );
	    if ( !isAtEdge(curposid) )
		continue;

	    if ( !surface_.isDefined(leftposid) || !isNodeOK(leftposid) )
		continue;

	    if ( !surface_.isDefined(rightposid) || !isNodeOK(rightposid) )
	    {
		posid = curposid;
		nodefound = true;
		break;
	    }
	}
	
	if ( posid == firstposid || !nodefound || set.size() > 100000 )
	    break;
    }

    set.disp_.connect_ = Pick::Set::Disp::Close;
    return set.size() ? true : false;
}


void Horizon3DGeometry::fillBinIDValueSet( const SectionID& sid,
					   BinIDValueSet& bivs ) const
{
    PtrMan<EMObjectIterator> it = createIterator( sid );
    if ( !it ) return;

    BinID bid;
    while ( true )
    {
	const PosID pid = it->next();
	if ( pid.objectID()==-1 )
	    break;

	const Coord3 crd = surface_.getPos( pid );
	if ( crd.isDefined() )
	{
	    bid.setSerialized( pid.subID() );
	    bivs.add( bid, crd.z );
	}
    }
}


EMObjectIterator* Horizon3DGeometry::createIterator(
			const SectionID& sid, const CubeSampling* cs) const
{
    if ( !cs )
        return new RowColIterator( surface_, sid, cs );

    const StepInterval<int> rowrg = cs->hrg.inlRange();
    const StepInterval<int> colrg = cs->hrg.crlRange();
    return new RowColIterator( surface_, sid, rowrg, colrg );
}


Table::FormatDesc* Horizon3DAscIO::getDesc() 
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Horizon3D" );
    fd->headerinfos_ += new Table::TargetInfo( "Undefined Value",
	    		StringInpSpec(sKey::FloatUdf), Table::Required );
    BufferStringSet attrnms;
    createDescBody( fd, attrnms );
    return fd;
}


void Horizon3DAscIO::createDescBody( Table::FormatDesc* fd,
				     const BufferStringSet& attrnms )
{
    Table::TargetInfo* posinfo = new Table::TargetInfo( "", FloatInpSpec(),
	   						Table::Required );
    Table::TargetInfo::Form* form = new Table::TargetInfo::Form( "Inl/Crl",
	    						FloatInpSpec() );
    form->add( FloatInpSpec() );
    posinfo->add( form );
    posinfo->form(0).setName( "X/Y");
    posinfo->form(0).add( FloatInpSpec() );
    fd->bodyinfos_ += posinfo;

    for ( int idx=0; idx<attrnms.size(); idx++ )
    {
	BufferString fldname = attrnms.get( idx );
	Table::TargetInfo* ti = new Table::TargetInfo( fldname.buf(),
		FloatInpSpec(), Table::Required );
	if ( fldname == "Z values" )
	{
	    ti->setPropertyType( PropertyRef::surveyZType() );
	    const char* un = SI().zIsTime() ? "Milliseconds"
					    : SI().zInFeet() ? "Feet" : "Meter";
	    ti->selection_.unit_ = UoMR().get( un );
	}

	fd->bodyinfos_ += ti;
    }
}


void Horizon3DAscIO::updateDesc( Table::FormatDesc& fd,
				 const BufferStringSet& attrnms )
{
    fd.bodyinfos_.erase();
    createDescBody( &fd, attrnms );
}


#define mErrRet(s) { if ( s ) errmsg_ = s; return 0; }

bool Horizon3DAscIO::isXY() const
{
    const Table::TargetInfo* xinfo = fd_.bodyinfos_[0];
    if ( !xinfo ) return false;

    const int sel = xinfo->selection_.form_;
    return !sel;
}


int Horizon3DAscIO::getNextLine( TypeSet<float>& data )
{
    data.erase();
    if ( !finishedreadingheader_ )
    {
	if ( !getHdrVals(strm_) )
	    return -1;
	
	udfval_ = getfValue( 0 );
	finishedreadingheader_ = true;
    }

    int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return ret;

    for ( int idx=0; idx<=fd_.bodyinfos_.size(); idx++ )
	data += getfValue( idx, udfval_ );

    return ret;
}


} // namespace EM
