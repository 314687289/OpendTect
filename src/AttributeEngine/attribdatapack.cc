/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          January 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "attribdatapack.h"

#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "attribdatacubes.h"
#include "attribdataholder.h"
#include "attribdataholderarray.h"
#include "bufstringset.h"
#include "flatposdata.h"
#include "iopar.h"
#include "keystrs.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "survinfo.h"

#define mStepIntvD( rg ) \
    StepInterval<double>( rg.start, rg.stop, rg.step )

namespace Attrib
{

// DataPackCommon

static const FixedString sAttribute2D()	{ return "Attribute2D"; }

const char* DataPackCommon::categoryStr( bool vertical, bool is2d )
{
    mDeclStaticString( vret );
    vret = IOPar::compKey( is2d ? sAttribute2D() : sKey::Attribute(), "V" );
    return vertical ? vret.str() : (is2d ? sAttribute2D().str()
					 : sKey::Attribute().str());
}


void DataPackCommon::dumpInfo( IOPar& iop ) const
{
    iop.set( "Source type", sourceType() );
    iop.set( "Attribute.ID", descID().asInt() );
    iop.set( IOPar::compKey(sKey::Attribute(),sKey::Stored()),
	     descID().isStored() );
    iop.set( "Vertical", isVertical() );
}


// Flat3DDataPack
Flat3DDataPack::Flat3DDataPack( DescID did, const DataCubes& dc, int cubeidx )
    : ::FlatDataPack(categoryStr(true))
    , DataPackCommon(did)
    , cube_(dc)
    , cubeidx_( cubeidx )
    , arr2dsl_(0)
    , arr2dsource_(0)
    , usemultcubes_( cubeidx == -1 )
{
    cube_.ref();
    if ( cube_.getInlSz() < 2 )
	dir_ = TrcKeyZSampling::Inl;
    else if ( cube_.getCrlSz() < 2 )
	dir_ = TrcKeyZSampling::Crl;
    else
	dir_ = TrcKeyZSampling::Z;

    if ( usemultcubes_ )
    {
	createA2DSFromMultCubes();
	cubeidx_ = -1;
    }
    else
	createA2DSFromSingCube( cubeidx );

    arr2dsl_->init();

    setPosData();
}


Flat3DDataPack::~Flat3DDataPack()
{
    if ( arr2dsource_ ) delete arr2dsource_;
    delete arr2dsl_;
    cube_.unRef();
}


void Flat3DDataPack::createA2DSFromSingCube( int cubeidx )
{
    int unuseddim, dim0, dim1;
    if ( dir_==TrcKeyZSampling::Inl )
    {
	unuseddim = DataCubes::cInlDim();
	dim0 = DataCubes::cCrlDim();
	dim1 = DataCubes::cZDim();
    }
    else if ( dir_==TrcKeyZSampling::Crl )
    {
	unuseddim = DataCubes::cCrlDim();
	dim0 = DataCubes::cInlDim();
	dim1 = DataCubes::cZDim();
    }
    else
    {
	dir_ = TrcKeyZSampling::Z;
	unuseddim = DataCubes::cZDim();
	dim0 = DataCubes::cInlDim();
	dim1 = DataCubes::cCrlDim();
	setCategory( categoryStr(false) );
    }

    if ( !arr2dsl_ )
	arr2dsl_ = new Array2DSlice<float>( cube_.getCube(cubeidx) );

    arr2dsl_->setPos( unuseddim, 0 );
    arr2dsl_->setDimMap( 0, dim0 );
    arr2dsl_->setDimMap( 1, dim1 );
}


void Flat3DDataPack::createA2DSFromMultCubes()
{
    const TrcKeyZSampling cs = cube_.cubeSampling();
    bool isvert = cs.nrZ() > 1;
    int dim0sz = isvert ? cube_.nrCubes()
			: dir_==TrcKeyZSampling::Inl ? cube_.getCrlSz()
						  : cube_.getInlSz();
    int dim1sz = isvert ? cube_.getZSz() : cube_.nrCubes();

    arr2dsource_ = new Array2DImpl<float>( dim0sz, dim1sz );
    for ( int id0=0; id0<dim0sz; id0++ )
    {
	for ( int id1=0; id1<dim1sz; id1++ )
	{
	    float val = isvert ? cube_.getCube(id0).get( 0, 0, id1 )
			       : dir_==TrcKeyZSampling::Inl
					? cube_.getCube(id1).get( 0, id0, 0 )
					: cube_.getCube(id1).get( id0, 0, 0 );
	    arr2dsource_->set( id0, id1, val );
	}
    }

    arr2dsl_ = new Array2DSlice<float>( *arr2dsource_ );
    arr2dsl_->setDimMap( 0, 0 );
    arr2dsl_->setDimMap( 1, 1 );
}


bool Flat3DDataPack::setDataDir( TrcKeyZSampling::Dir dir )
{
    if ( arr2dsource_ )
    {
	pErrMsg("Not supported");
	return false;
    }

    dir_ = dir;
    createA2DSFromSingCube( cubeidx_ );
    return arr2dsl_->init();
}

#define mGetDim( unuseddim ) \
    if ( dir_==TrcKeyZSampling::Inl ) \
	unuseddim = DataCubes::cInlDim(); \
    else if ( dir_==TrcKeyZSampling::Crl ) \
	unuseddim = DataCubes::cCrlDim(); \
    else \
	unuseddim = DataCubes::cZDim()

int Flat3DDataPack::nrSlices() const
{
    if ( arr2dsource_ ) return 1;

    int unuseddim;
    mGetDim( unuseddim );

    return arr2dsl_->getDimSize( unuseddim );
}


int Flat3DDataPack::getDataSlice() const
{
    if ( arr2dsource_ ) return 0;

    int unuseddim;
    mGetDim( unuseddim );

    return arr2dsl_->getPos( unuseddim );
}


bool Flat3DDataPack::setDataSlice( int pos )
{
    if ( arr2dsource_ )
    {
	pErrMsg("Not supported");
	return false;
    }

    int unuseddim;
    mGetDim( unuseddim );

    if ( pos<0 || pos>=arr2dsl_->getDimSize( unuseddim ) )
	return false;

    arr2dsl_->setPos( unuseddim, pos );
    return true;
}


Array2D<float>& Flat3DDataPack::data()
{
    return *arr2dsl_;
}


void Flat3DDataPack::setPosData()
{
    const TrcKeyZSampling cs = cube_.cubeSampling();
    if ( usemultcubes_ )
    {
	StepInterval<int> cubeintv( 0, cube_.nrCubes(), 1 );
	bool isvert = cs.nrZ() > 1;
	posdata_.setRange( true,
	    isvert ? mStepIntvD(cubeintv)
		   : dir_==TrcKeyZSampling::Inl ? mStepIntvD(cs.hrg.crlRange())
					     : mStepIntvD(cs.hrg.inlRange()) );
	posdata_.setRange(false, isvert ? mStepIntvD(cs.zrg)
					: mStepIntvD(cubeintv) );
    }
    else
    {
	posdata_.setRange( true, dir_==TrcKeyZSampling::Inl
	    ? mStepIntvD(cs.hrg.crlRange()) : mStepIntvD(cs.hrg.inlRange()) );
	posdata_.setRange( false, dir_==TrcKeyZSampling::Z
	    ? mStepIntvD(cs.hrg.crlRange()) : mStepIntvD(cs.zrg) );
    }
}


void Flat3DDataPack::dumpInfo( IOPar& iop ) const
{
    ::FlatDataPack::dumpInfo( iop );
    DataPackCommon::dumpInfo( iop );
}


Coord3 Flat3DDataPack::getCoord( int i0, int i1 ) const
{
    const TrcKeyZSampling& cs = cube_.cubeSampling();
    int inlidx = i0; int crlidx = 0; int zidx = i1;
    if ( usemultcubes_ )
    {
	if ( cs.nrZ() > 1 )
	    inlidx = 0;
	else if ( dir_==TrcKeyZSampling::Inl )
	    { inlidx = 0; crlidx = i0; zidx = 0; }
	else
	    { zidx = 0; }
    }
    else
    {
	if ( dir_ == TrcKeyZSampling::Inl )
	    { inlidx = 0; crlidx = i0; }
	else if ( dir_ == TrcKeyZSampling::Z )
	    { crlidx = i1; zidx = 0; }
    }

    const Coord c = SI().transform( cs.hrg.atIndex(inlidx,crlidx) );
    return Coord3(c.x,c.y,cs.zsamp_.atIndex(zidx));
}


#define mKeyInl SeisTrcInfo::getFldString(SeisTrcInfo::BinIDInl)
#define mKeyCrl SeisTrcInfo::getFldString(SeisTrcInfo::BinIDCrl)
#define mKeyX SeisTrcInfo::getFldString(SeisTrcInfo::CoordX)
#define mKeyY SeisTrcInfo::getFldString(SeisTrcInfo::CoordY)
#define mKeyCube "Series"
//TODO : find a way to get a better name than "Series"

const char* Flat3DDataPack::dimName( bool dim0 ) const
{
    if ( usemultcubes_ )
    {
	if ( cube_.cubeSampling().nrZ() > 1 )
	    return dim0 ? mKeyCube : "Z";
	else if ( dir_ == TrcKeyZSampling::Inl )
	    return dim0 ? mKeyCrl : mKeyCube;
	else
	    return dim0 ? mKeyInl : mKeyCube;
    }

    return dim0 ? (dir_==TrcKeyZSampling::Inl ? mKeyCrl : mKeyInl)
		: (dir_==TrcKeyZSampling::Z ? mKeyCrl : "Z");
}


bool Flat3DDataPack::isAltDim0InInt( const char* keystr ) const
{
    FixedString key( keystr );
    return key == mKeyInl || key == mKeyCrl;
}


void Flat3DDataPack::getAltDim0Keys( BufferStringSet& bss ) const
{
    if ( usemultcubes_ && cube_.cubeSampling().nrZ() > 1 )
	bss.add( mKeyCube );
    if ( dir_== TrcKeyZSampling::Crl )
	bss.add( mKeyInl );
    else
	bss.add( mKeyCrl );
    bss.add( mKeyX );
    bss.add( mKeyY );
}


double Flat3DDataPack::getAltDim0Value( int ikey, int i0 ) const
{
    if ( ikey < 1 || ikey > 3 )
	 return FlatDataPack::getAltDim0Value( ikey, i0 );

    if ( usemultcubes_ && cube_.cubeSampling().nrZ() > 1 )
	return i0;	//TODO: now returning cube idx, what else can we do?

    const Coord3 c( getCoord(i0,0) );
    return ikey == 1 ? c.x : c.y;
}


void Flat3DDataPack::getAuxInfo( int i0, int i1, IOPar& iop ) const
{
    const Coord3 c( getCoord(i0,i1) );

    BinID bid = SI().transform( c );
    iop.set( mKeyX, c.x );
    iop.set( mKeyY, c.y );
    iop.set( "Inline", bid.inl() );
    iop.set( "Crossline", bid.crl() );
    iop.set( "Z", c.z*SI().zDomain().userFactor() );

    if ( usemultcubes_ )
	iop.set( mKeyCube, cube_.cubeSampling().nrZ() > 1 ? i0 : i1 );
}


// Flat2DDataPack
Flat2DDataPack::Flat2DDataPack( DescID did )
    : ::FlatDataPack(categoryStr(true,true))
    , DataPackCommon(did)
{
    SeisTrcInfo::getAxisCandidates( Seis::Line, tiflds_ );
}


void Flat2DDataPack::dumpInfo( IOPar& iop ) const
{
    ::FlatDataPack::dumpInfo( iop );
    DataPackCommon::dumpInfo( iop );
}


bool Flat2DDataPack::isAltDim0InInt( const char* keystr ) const
{
    FixedString key( keystr );
    return key == SeisTrcInfo::getFldString(SeisTrcInfo::TrcNr) ||
	   key == SeisTrcInfo::getFldString(SeisTrcInfo::BinIDInl) ||
	   key == SeisTrcInfo::getFldString(SeisTrcInfo::BinIDCrl);
}


void Flat2DDataPack::getAltDim0Keys( BufferStringSet& bss ) const
{
    for ( int idx=0; idx<tiflds_.size(); idx++ )
	bss.add( SeisTrcInfo::getFldString(tiflds_[idx]) );
}


const char* Flat2DDataPack::dimName( bool dim0 ) const
{
    return dim0 ? "Distance" : "Z";
}


// Flat2DDHDataPack
Flat2DDHDataPack::Flat2DDHDataPack( DescID did, const Data2DHolder& dh,
				    const Pos::GeomID& geomid,
				    bool usesingtrc, int component )
    : Flat2DDataPack( did )
    , geomid_(geomid)
    , usesingtrc_( usesingtrc )
    , tracerange_(0,0,1)
    , dataholderarr_( 0 )
{
    if ( !dh.trcinfoset_.isEmpty() )
    {
	tracerange_ = StepInterval<int>( dh.trcinfoset_[0]->nr,
			dh.trcinfoset_[dh.trcinfoset_.size()-1]->nr, 1 );
	samplingdata_ = dh.trcinfoset_[0]->sampling;
    }

    ConstRefMan<Data2DHolder> dataref( &dh );
    mTryAlloc( dataholderarr_, Data2DArray( dh ) );
    if ( !dataholderarr_ )
	return;

    dataholderarr_->ref();

    if ( !dataholderarr_->isOK() )
    {
	dataholderarr_->unRef();
	dataholderarr_ = 0;
	return;
    }

    Array2DSlice<float>* arr2dslice;
    mTryAlloc( arr2dslice, Array2DSlice<float>(*dataholderarr_->dataset_) );
    if ( !arr2dslice )
	return;

    if ( usesingtrc )
    {
	arr2dslice->setPos( 1, 0 );
	arr2dslice->setDimMap( 0, 0 );
    }
    else
    {
	const int nrseries = dataholderarr_->dataset_->info().getSize( 0 );
	if ( nrseries==1 )
	    component = 0;
	arr2dslice->setPos( 0, component );
	arr2dslice->setDimMap( 0, 1 );
    }

    arr2dslice->setDimMap( 1, 2 );
    arr2dslice->init();
    arr2d_ = arr2dslice;

    setPosData();
}


Flat2DDHDataPack::Flat2DDHDataPack( DescID did, const Array2D<float>& arr2d,
						const Pos::GeomID& geomid,
						const SamplingData<float>& sd,
						const StepInterval<int>& trcrg )
    : Flat2DDataPack(did)
    , geomid_(geomid)
    , tracerange_(trcrg)
    , samplingdata_(sd)
    , usesingtrc_(false)
    , dataholderarr_(0)
{
    arr2d_ = new Array2DImpl<float>( arr2d );
    setPosData();
}


Flat2DDHDataPack::~Flat2DDHDataPack()
{
    if ( dataholderarr_ )
	dataholderarr_->unRef();
}


TrcKeyZSampling Flat2DDHDataPack::getTrcKeyZSampling() const
{
    // TODO: Get rid of this function.
    TrcKeyZSampling cs;
    cs.hrg.setInlRange( StepInterval<int>(0,0,1) );
    cs.hrg.setCrlRange( tracerange_ );
    cs.zsamp_ = samplingdata_.interval( arr2d_->info().getSize(1) );
    return cs;
}


void Flat2DDHDataPack::getPosDataTable( TypeSet<int>& trcnrs,
					TypeSet<float>& dist ) const
{
    trcnrs.erase(); dist.erase();
    const int nrtrcs = tracerange_.nrSteps()+1;
    trcnrs.setSize( nrtrcs, -1 );
    dist.setSize( nrtrcs, -1 );
    for ( int idx=0; idx<nrtrcs; idx++ )
    {
	trcnrs[idx] = tracerange_.atIndex( idx );
	if ( posdata_.width(true)/posdata_.range(true).step > idx )
	    dist[idx] = (float) posdata_.position( true, idx );
	else
	    trcnrs[idx] = -1;
    }
}


void Flat2DDHDataPack::getCoordDataTable( const TypeSet<int>& trcnrs,
					  TypeSet<Coord>& coords ) const
{
    if ( trcnrs.size() > 0 )
	coords.setSize( tracerange_.nrSteps()+1, Coord::udf() );

    const Survey::Geometry* geometry = Survey::GM().getGeometry( geomid_ );
    if ( !geometry ) return;

    for ( int idx=0; idx<trcnrs.size(); idx++ )
    {
	if ( tracerange_.includes(trcnrs[idx],true) )
	    coords[idx] = geometry->toCoord( geomid_, trcnrs[idx] );
    }
}


#define mNrTrcDim 1

void Flat2DDHDataPack::setPosData()
{
    const int nrpos = !dataholderarr_ ? tracerange_.nrSteps()+1 :
	    usesingtrc_ ? dataholderarr_->dataset_->info().getSize(0)
			: dataholderarr_->dataset_->info().getSize(mNrTrcDim);
    if ( nrpos < 1 ) return;

    if ( usesingtrc_ )
	posdata_.setRange( true, StepInterval<double>( 0, nrpos-1, 1 ) );
    else
    {
	float* pos = new float[nrpos];
	pos[0] = 0;
	const Survey::Geometry* geometry = Survey::GM().getGeometry( geomid_ );
	if ( !geometry ) return;

	Coord prevcrd = geometry->toCoord( geomid_, tracerange_.atIndex(0) );
	for ( int idx=1; idx<nrpos; idx++ )
	{
	    Coord crd = geometry->toCoord( geomid_, tracerange_.atIndex(idx) );
	    if ( crd.isUdf() )
		pos[idx] = mCast(float,(pos[idx-1]));
	    else
	    {
		pos[idx] = mCast(float,(pos[idx-1] + prevcrd.distTo(crd)));
		prevcrd = crd;
	    }
	}
	posdata_.setX1Pos( pos, nrpos, 0 );
    }

    posdata_.setRange( false,
	    mStepIntvD(samplingdata_.interval(arr2d_->info().getSize(1))) );
}


double Flat2DDHDataPack::getAltDim0Value( int ikey, int i0 ) const
{
    const int nrpos = !dataholderarr_ ? tracerange_.nrSteps()+1 :
	    usesingtrc_ ? dataholderarr_->dataset_->info().getSize(0)
			: dataholderarr_->dataset_->info().getSize(mNrTrcDim);
    if ( i0<0 || i0>=nrpos || !tiflds_.validIdx(ikey) )
	return FlatDataPack::getAltDim0Value( ikey, i0 );

    if ( usesingtrc_ )
	return i0;	//what else can we do?

    if ( dataholderarr_ )
	return dataholderarr_->trcinfoset_[i0]->getValue( tiflds_[ikey] );

    switch ( tiflds_[ikey] )
    {
	case SeisTrcInfo::TrcNr:	 return tracerange_.atIndex(i0);
	case SeisTrcInfo::CoordX:	 return getCoord(i0,0).x;
	case SeisTrcInfo::CoordY:	 return getCoord(i0,0).y;
	default:		return FlatDataPack::getAltDim0Value(ikey,i0);
    }
}


void Flat2DDHDataPack::getAuxInfo( int i0, int i1, IOPar& iop ) const
{
    int trcinfoidx = usesingtrc_ ? 0 : i0;
    if ( !dataholderarr_ )
    {
	const Coord3 crd = getCoord( i0, i1 );
	iop.set( "X-coordinate", crd.x );
	iop.set( "Y-coordinate", crd.y );
	iop.set( "Z-Coord", crd.z*SI().zDomain().userFactor() );
	iop.set( sKey::TraceNr(), tracerange_.atIndex(i0) );
	return;
    }

    if (  !dataholderarr_->trcinfoset_.validIdx(trcinfoidx) )
	return;

    const SeisTrcInfo& ti = *dataholderarr_->trcinfoset_[ trcinfoidx ];
    ti.getInterestingFlds( Seis::Line, iop );
    iop.set( "Z-Coord", ti.sampling.atIndex(i1)*SI().zDomain().userFactor() );
}


Coord3 Flat2DDHDataPack::getCoord( int i0, int i1 ) const
{
    if ( i0 < 0 || usesingtrc_ ) i0 = 0;
    if ( i0 > tracerange_.nrSteps() ) i0 = tracerange_.nrSteps();
    const Survey::Geometry* geometry = Survey::GM().getGeometry( geomid_ );
    if ( !geometry ) return Coord3::udf();
    return Coord3( geometry->toCoord(geomid_,tracerange_.atIndex(i0)),
		   samplingdata_.atIndex(i1) );
}


// FlatRdmTrcsDataPack
FlatRdmTrcsDataPack::FlatRdmTrcsDataPack( DescID did, const SeisTrcBuf& sb,
					  const TypeSet<BinID>* path )
    : Flat2DDataPack(did)
    , path_(0)
{
    if ( path )
	path_ = new TypeSet<BinID>(*path);

    const SeisTrc* firsttrc = sb.get( 0 );
    if ( firsttrc )
	samplingdata_.set( firsttrc->info().sampling );

    seisbuf_ = new SeisTrcBuf( true );
    sb.copyInto(*seisbuf_);

    const int nrtrcs = seisbuf_->size();
    const int arrsz0 = path ? path->size() : nrtrcs;
    const int nrsamp = nrtrcs ? seisbuf_->get(0)->size() : 0;
    arr2d_ = new Array2DImpl<float>( arrsz0, nrsamp );
    fill2DArray( path );
    setPosData( path );
}


FlatRdmTrcsDataPack::FlatRdmTrcsDataPack( DescID did,
		const Array2D<float>& arr2d, const SamplingData<float>& sd,
		const TypeSet<BinID>* path )
    : Flat2DDataPack(did)
    , samplingdata_(sd)
    , seisbuf_(0)
    , path_(0)
{
    if ( path )
	path_ = new TypeSet<BinID>(*path);

    arr2d_ = new Array2DImpl<float>( arr2d );
    setPosData( path );
}


FlatRdmTrcsDataPack::~FlatRdmTrcsDataPack()
{
    if ( seisbuf_ ) seisbuf_->erase();
    delete path_;
}


void FlatRdmTrcsDataPack::setPosData( const TypeSet<BinID>* path )
{
    const int nrtrcs = seisbuf_ ? seisbuf_->size() : arr2d_->info().getSize(0);
    if ( nrtrcs<1 || ( path && path->size()<nrtrcs ) ) return;

    const int nrpos = path ? path->size() : nrtrcs;
    float* pos = new float[nrpos]; pos[0] = 0;
    Coord prevcrd; int x0arridx = -1;
    for ( int idx=0; idx<nrpos; idx++ )
    {
	x0arridx++;
	const Coord crd = path ? SI().transform( (*path)[idx] )
			 : seisbuf_->get(idx)->info().coord;
	if ( x0arridx > 0 )
	{
	    float distnnm1 = (float) prevcrd.distTo(crd);
	    pos[x0arridx] = pos[x0arridx-1] + fabs( distnnm1 );
	}
	prevcrd = crd;
    }

    posdata_.setX1Pos( pos, nrpos, 0 );
    posdata_.setRange( false,
	    mStepIntvD(samplingdata_.interval(arr2d_->info().getSize(1))) );
}


double FlatRdmTrcsDataPack::getAltDim0Value( int ikey, int i0 ) const
{
    if ( !tiflds_.validIdx(ikey) )
	return FlatDataPack::getAltDim0Value( ikey, i0 );

    const bool useseisbuf = seisbuf_ && (!path_ ||
			tiflds_[ikey]==SeisTrcInfo::RefNr);
    //!< Using path is preferred as path_ can be longer than seisbuf_.
    const int nrpos = useseisbuf ? seisbuf_->size() : path_->size();
    if ( i0 < 0 ) i0 = 0;
    if ( i0 >= nrpos ) i0 = nrpos-1;

    if ( useseisbuf )
	seisbuf_->get(i0)->info().getValue( tiflds_[ikey] );

    switch ( tiflds_[ikey] )
    {
	case SeisTrcInfo::TrcNr:	return TrcKey((*path_)[i0]).trcNr();
	case SeisTrcInfo::CoordX:	return getCoord(i0,0).x;
	case SeisTrcInfo::CoordY:	return getCoord(i0,0).y;
	default:		return FlatDataPack::getAltDim0Value(ikey,i0);
    }
}


void FlatRdmTrcsDataPack::getAuxInfo( int i0, int i1, IOPar& iop ) const
{
    if ( !seisbuf_ )
    {
	if ( path_ && path_->validIdx(i0) )
	{
	    iop.set( "In-line", (*path_)[i0].lineNr() );
	    iop.set( "Cross-line", (*path_)[i0].trcNr() );
	}

	const Coord3 crd = getCoord( i0, i1 );
	iop.set( "X-coordinate", crd.x );
	iop.set( "Y-coordinate", crd.y );
	iop.set( "Z-Coord", crd.z*SI().zDomain().userFactor() );
	return;
    }

    if ( !seisbuf_->validIdx(i0) )
	return;

    const SeisTrcInfo& ti = seisbuf_->get(i0)->info();
    ti.getInterestingFlds( Seis::Line, iop );
    iop.set( "Z-Coord", ti.samplePos(i1)*SI().zDomain().userFactor() );
}


Coord3 FlatRdmTrcsDataPack::getCoord( int i0, int i1 ) const
{
    const int nrpos = path_ ? path_->size() : seisbuf_->size();
    if ( i0 < 0 ) i0 = 0;
    if ( i0 >= nrpos ) i0 = nrpos-1;

    if ( path_ )
	return Coord3( SI().transform((*path_)[i0]),samplingdata_.atIndex(i1) );

    if ( seisbuf_ && !seisbuf_->isEmpty() )
    {
	const SeisTrcInfo& ti = seisbuf_->get(i0)->info();
	return Coord3( ti.coord, ti.sampling.atIndex(i1) );
    }

    return Coord3();
}


void FlatRdmTrcsDataPack::fill2DArray( const TypeSet<BinID>* path )
{
    if ( !seisbuf_ || seisbuf_->isEmpty() || !arr2d_ ) return;

    const int nrtrcs = seisbuf_->size();
    if ( path && path->size()<nrtrcs ) return;

    const int nrsamp = seisbuf_->get(0)->size();
    const int arrsz0 = path ? path->size() : nrtrcs;
    int x0arridx = -1;

    for ( int idt=0; idt<arrsz0; idt++ )
    {
	const int trcidx = path ? seisbuf_->find( (*path)[idt] ) : idt;
	x0arridx++;
	const SeisTrc* trc = trcidx<0 ? 0 : seisbuf_->get( trcidx );
	for ( int idz=0; idz<nrsamp; idz++ )
	    arr2d_->set( x0arridx, idz, !trc ? mUdf(float) : trc->get(idz,0) );
    //rem: assuming that interesting data is at component 0;
    //always true if coming from the engine, from where else?
    }
}


} // namespace Attrib

