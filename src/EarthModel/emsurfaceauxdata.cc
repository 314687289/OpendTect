/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: emsurfaceauxdata.cc,v 1.16 2008-02-12 12:40:49 cvsnanne Exp $
________________________________________________________________________

-*/

#include "emsurfaceauxdata.h"

#include "arrayndimpl.h"
#include "emhorizon3d.h"
#include "emsurfacegeometry.h"
#include "emsurfacetr.h"
#include "emsurfauxdataio.h"
#include "executor.h"
#include "filegen.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "iostrm.h"
#include "parametricsurface.h"
#include "ptrman.h"
#include "settings.h"
#include "strmprov.h"

namespace EM
{

SurfaceAuxData::SurfaceAuxData( Horizon3D& horizon )
    : horizon_( horizon )
    , changed( 0 )
{
    auxdatanames.allowNull(true);
    auxdatainfo.allowNull(true);
    auxdata.allowNull(true);
}


SurfaceAuxData::~SurfaceAuxData()
{
    removeAll();
}


void SurfaceAuxData::removeAll()
{
    deepErase( auxdatanames );
    deepErase( auxdatainfo );
    for ( int idx=0; idx<auxdata.size(); idx++ )
    {
	if ( !auxdata[idx] ) continue;
	deepErase( *auxdata[idx] );
    }

    deepErase( auxdata );
    changed = true;
}


int SurfaceAuxData::nrAuxData() const
{ return auxdatanames.size(); }


const char* SurfaceAuxData::auxDataName( int dataidx ) const
{
    if ( nrAuxData() && auxdatanames[dataidx] )
	return *auxdatanames[dataidx];

    return 0;
}


void SurfaceAuxData::setAuxDataName( int dataidx, const char* name )
{
    if ( auxdatanames[dataidx] )
	auxdatanames.replace( dataidx, new BufferString(name) );
}


int SurfaceAuxData::auxDataIndex( const char* nm ) const
{
    for ( int idx=0; idx<auxdatanames.size(); idx++ )
	if ( *auxdatanames[idx] == nm ) return idx;
    return -1;
}


int SurfaceAuxData::addAuxData( const char* name )
{
    auxdatanames += new BufferString( name );
    ObjectSet<TypeSet<float> >* newauxdata = new ObjectSet<TypeSet<float> >;
    auxdata += newauxdata;
    newauxdata->allowNull(true);

    for ( int idx=0; idx<horizon_.nrSections(); idx++ )
	(*newauxdata) += 0;

    changed = true;
    return auxdatanames.size()-1;
}


void SurfaceAuxData::removeAuxData( int dataidx )
{
    delete auxdatanames[dataidx];
    auxdatanames.replace( dataidx, 0 );

    deepEraseArr( *auxdata[dataidx] );
    delete auxdata[dataidx];
    auxdata.replace( dataidx, 0 );
    changed = true;
}


float SurfaceAuxData::getAuxDataVal( int dataidx, const PosID& posid ) const
{
    if ( !auxdata.validIdx(dataidx) || !auxdata[dataidx] )
	return mUdf(float);

    const int sectionidx = horizon_.sectionIndex( posid.sectionID() );
    if ( sectionidx==-1 ) return mUdf(float);

    const TypeSet<float>* sectionauxdata = sectionidx<auxdata[dataidx]->size()
	? (*auxdata[dataidx])[sectionidx] : 0;

    if ( !sectionauxdata ) return mUdf(float);

    const RowCol geomrc( posid.subID() );
    const int subidx =
	horizon_.geometry().sectionGeometry(posid.sectionID())->getKnotIndex(geomrc);
    if ( subidx==-1 ) return mUdf(float);
    return (*sectionauxdata)[subidx];
}


void SurfaceAuxData::setAuxDataVal( int dataidx, const PosID& posid, float val)
{
    if ( !auxdata[dataidx] ) return;

    const int sectionidx = horizon_.sectionIndex( posid.sectionID() );
    if ( sectionidx==-1 ) return;

    const RowCol geomrc( posid.subID() ); 
    const int subidx =
	horizon_.geometry().sectionGeometry(posid.sectionID())->getKnotIndex(geomrc);
    if ( subidx==-1 ) return;

    TypeSet<float>* sectionauxdata = sectionidx<auxdata[dataidx]->size()
	? (*auxdata[dataidx])[sectionidx] : 0;
    if ( !sectionauxdata )
    {
	for ( int idx=auxdata[dataidx]->size(); idx<=sectionidx; idx++ )
	    (*auxdata[dataidx]) += 0;

	const int sz =
	    horizon_.geometry().sectionGeometry( posid.sectionID() )->nrKnots();
	auxdata[dataidx]->replace( sectionidx,
				   new TypeSet<float>(sz,mUdf(float)) );
	sectionauxdata = (*auxdata[dataidx])[sectionidx];
    }

    (*sectionauxdata)[subidx] = val;
    changed = true;
}


bool SurfaceAuxData::isChanged(int idx) const
{ return changed; }


void SurfaceAuxData::resetChangedFlag()
{
    changed = false;
}


Executor* SurfaceAuxData::auxDataLoader( int selidx )
{
    PtrMan<IOObj> ioobj = IOM().get( horizon_.multiID() );
    if ( !ioobj )
	{ horizon_.errmsg_ = "Cannot find surface"; return 0; }

    PtrMan<EMSurfaceTranslator> tr = 
			(EMSurfaceTranslator*)ioobj->getTranslator();
    if ( !tr || !tr->startRead(*ioobj) )
    { horizon_.errmsg_ = tr ? tr->errMsg() : "Cannot find Translator";return 0;}

    SurfaceIODataSelection& sel = tr->selections();
    int nrauxdata = sel.sd.valnames.size();
    if ( !nrauxdata || selidx >= nrauxdata ) return 0;

    ExecutorGroup* grp = new ExecutorGroup( "Surface attributes reader" );
    for ( int validx=0; validx<sel.sd.valnames.size(); validx++ )
    {
	if ( selidx>=0 && selidx != validx ) continue;

	BufferString filenm = getAuxDataFileName( *ioobj, 
					sel.sd.valnames[validx]->buf() );
	if ( !*filenm ) continue;

	dgbSurfDataReader* rdr = new dgbSurfDataReader(filenm);
	rdr->setSurface( horizon_ );
	grp->add( rdr );
    }

    return grp;
}


Executor* SurfaceAuxData::auxDataSaver( int dataidx, bool overwrite )
{
    PtrMan<IOObj> ioobj = IOM().get( horizon_.multiID() );
    if ( !ioobj )
	{ horizon_.errmsg_ = "Cannot find surface"; return 0; }
    StreamConn* conn = dynamic_cast<StreamConn*>(ioobj->getConn(Conn::Read));
    if ( !conn ) return 0;

    bool binary = true;
    mSettUse(getYN,"dTect.Surface","Binary format",binary);

    BufferString fnm;
    if ( overwrite )
    {
	if ( dataidx<0 ) dataidx = 0;
	fnm = getAuxDataFileName( *ioobj, auxDataName(dataidx) );
	return new dgbSurfDataWriter(horizon_,dataidx,0,binary,fnm);
    }

    ExecutorGroup* grp = new ExecutorGroup( "Surface attributes saver" );
    grp->setNrDoneText( "Nr done" );
    for ( int selidx=0; selidx<nrAuxData(); selidx++ )
    {
	if ( dataidx >= 0 && dataidx != selidx ) continue;
	for ( int idx=0; ; idx++ )
	{
	    fnm = dgbSurfDataWriter::createHovName( conn->fileName(), idx );
	    if ( !File_exists(fnm) )
		break;
	}

	Executor* exec = new dgbSurfDataWriter(horizon_,selidx,0,binary,fnm);
	grp->add( exec );
    }

    return grp;
}


void SurfaceAuxData::removeSection( const SectionID& sectionid )
{
    const int sectionidx = horizon_.sectionIndex( sectionid );
    if ( sectionidx==-1 ) return;

    for ( int idy=0; idy<nrAuxData(); idy++ )
    {
	if ( !auxdata[idy] )
	    continue;

	delete (*auxdata[idy])[sectionidx];
	auxdata[idy]->replace( sectionidx, 0 );
    }
}


BufferString SurfaceAuxData::getAuxDataFileName( const IOObj& ioobj,
						const char* attrnm )
{
    mDynamicCastGet(const IOStream*,iostrm,&ioobj)
    if ( !iostrm ) return "";
    StreamProvider sp( iostrm->fileName() );
    sp.addPathIfNecessary( iostrm->dirName() );

    BufferString filenm;
    int gap = 0;
    for ( int idx=0; ; idx++ )
    {
	if ( gap > 100 ) return "";

	filenm = EM::dgbSurfDataWriter::createHovName(sp.fileName(),idx);
	if ( File_isEmpty(filenm) )
	{ gap++; continue; }

	EM::dgbSurfDataReader rdr( filenm );
	if ( !strcmp(rdr.dataName(),attrnm) )
	    break;
    }

    return filenm;
}
 

Array2D<float>* SurfaceAuxData::createArray2D( int dataidx, SectionID sid) const
{
    const StepInterval<int> rowrg = horizon_.geometry().rowRange( sid );
    const StepInterval<int> colrg = horizon_.geometry().colRange( sid );
    if ( rowrg.width(false)<1 || colrg.width(false)<1 )
	return 0;

    PosID posid( horizon_.id(), sid );
    Array2DImpl<float>* arr =
	new Array2DImpl<float>( rowrg.nrSteps()+1, colrg.nrSteps()+1 );
    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
    {
	for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
	{
	    posid.setSubID( RowCol(row,col).getSerialized() );
	    const float val = getAuxDataVal( dataidx, posid);
	    arr->set( rowrg.getIndex(row), colrg.getIndex(col), val );
	}
    }

    return arr;
}


void SurfaceAuxData::setArray2D( int dataidx, SectionID sid,
				 const Array2D<float>& arr2d )
{
    const StepInterval<int> rowrg = horizon_.geometry().rowRange( sid );
    const StepInterval<int> colrg = horizon_.geometry().colRange( sid );
    if ( rowrg.width(false)<1 || colrg.width(false)<1 )
	return;

    PosID posid( horizon_.id(), sid );

    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
    {
	for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
	{
	    posid.setSubID( RowCol(row,col).getSerialized() );
	    const float val = arr2d.get( rowrg.getIndex(row),
		    			 colrg.getIndex(col) );
	    setAuxDataVal( dataidx, posid, val );
	}
    }
}


bool SurfaceAuxData::usePar( const IOPar& par )
{ return true; }

void SurfaceAuxData::fillPar( IOPar& par ) const
{}

}; //namespace
