/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : October 2011
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uibodyregiondlg.h"

#include "arrayndimpl.h"
#include "embodytr.h"
#include "emfault3d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emmarchingcubessurface.h"
#include "emsurfacetr.h"
#include "explfaultsticksurface.h"
#include "explplaneintersection.h"
#include "executor.h"
#include "ioman.h"
#include "marchingcubes.h"
#include "polygon.h"
#include "polyposprovider.h"
#include "positionlist.h"
#include "sorting.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uiioobjseldlg.h"
#include "uimsg.h"
#include "uispinbox.h"
#include "uipossubsel.h"
#include "uistepoutsel.h"
#include "uitable.h"
#include "uitaskrunner.h"
#include "varlenarray.h"
#include "od_helpids.h"


#define mBelow 0
#define mAbove 1
#define mToMinInline 0
#define mToMaxInline 1
#define mToMinCrossline 2
#define mToMaxCrossline 3

#define cNameCol 0
#define cSideCol 1
#define cRelLayerCol 2
#define cHorShiftUpCol 2
#define cHorShiftDownCol 3

class BodyExtractorFromHorizons : public ParallelTask
{
public:
BodyExtractorFromHorizons( const TypeSet<MultiID>& hlist,
	const TypeSet<char>& sides, const TypeSet<float>& horshift,
	const CubeSampling& cs, Array3D<float>& res, const ODPolygon<float>& p )
    : res_(res)
    , cs_(cs)
    , plg_(p)
{
    res_.setAll( 1 );
    for ( int idx=0; idx<hlist.size(); idx++ )
    {
	EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded(hlist[idx]);
	mDynamicCastGet(EM::Horizon3D*,hor,emobj);
	if ( !hor ) continue;

	hor->ref();
	hors_ += hor;
	hsides_ += sides[idx];
	horshift_ += horshift[idx];
    }
}


~BodyExtractorFromHorizons()	{ deepUnRef( hors_ ); }
od_int64 nrIterations() const   { return cs_.nrInl()*cs_.nrCrl(); }
const char* message() const	{ return "Extracting body from horizons"; }

bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    const int crlsz = cs_.nrCrl();
    if ( !crlsz ) return true;

    const int zsz = cs_.nrZ();
    const int horsz = hors_.size();
    const bool usepolygon = !plg_.isEmpty();

    for ( int idx=mCast(int,start); idx<=stop && shouldContinue();
						    idx++, addToNrDone(1) )
    {
	const int inlidx = idx/crlsz;
	const int crlidx = idx%crlsz;
	const BinID bid = cs_.hrg.atIndex(inlidx,crlidx);
	if ( bid.inl()==cs_.hrg.start.inl() || bid.inl()==cs_.hrg.stop.inl() ||
	     bid.crl()==cs_.hrg.start.crl() || bid.crl()==cs_.hrg.stop.crl() )
	    continue;/*Extended one layer*/

	if ( usepolygon )
	{
	    Geom::Point2D<float> pt( mCast(float,bid.inl()),
				     mCast(float,bid.crl()) );
	    if ( !plg_.isInside(pt,true,0.01) )
		continue;
	}

	for ( int idz=1; idz<zsz-1; idz++ ) /*Extended one layer*/
	{
	    const float curz = cs_.zrg.atIndex( idz );
	    bool curzinrange = true;
	    float mindist = -1;
	    for ( int idy=0; idy<horsz; idy++ )
	    {
		const float hz = hors_[idy]->getZ(bid) + horshift_[idy];
		if ( mIsUdf(hz) ) continue;

		const float dist = hsides_[idy]==mBelow ? curz-hz : hz-curz;
		if ( dist<0 )
		{
		    curzinrange = false;
		    break;
		}

		if ( mindist<0 || mindist>dist )
		    mindist = dist;
	    }

	    if ( curzinrange )
		res_.set( inlidx, crlidx, idz, -mindist );
	}
    }

    return true;
}

Array3D<float>&					res_;
const CubeSampling&				cs_;
ObjectSet<EM::Horizon3D>			hors_;
TypeSet<char>					hsides_;
TypeSet<float>					horshift_;
const ODPolygon<float>&				plg_;
};


class ImplicitBodyRegionExtractor : public ParallelTask
{
public:
ImplicitBodyRegionExtractor( const TypeSet<MultiID>& surflist,
	const TypeSet<char>& sides, const TypeSet<float>& horshift,
	const CubeSampling& cs, Array3D<float>& res, const ODPolygon<float>& p )
    : res_(res)
    , cs_(cs)
    , plg_(p)
    , bidinplg_(0)
{
    res_.setAll( 1 );

    c_[0] = Geom::Point2D<float>( mCast(float,cs_.hrg.start.inl()),
	                          mCast(float,cs_.hrg.start.crl()) );
    c_[1] = Geom::Point2D<float>( mCast(float,cs_.hrg.stop.inl()),
				  mCast(float,cs_.hrg.start.crl()) );
    c_[2] = Geom::Point2D<float>( mCast(float,cs_.hrg.stop.inl()),
				  mCast(float,cs_.hrg.stop.crl()) );
    c_[3] = Geom::Point2D<float>( mCast(float,cs_.hrg.start.inl()),
				  mCast(float,cs_.hrg.stop.crl()) );

    for ( int idx=0; idx<surflist.size(); idx++ )
    {
	EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded( surflist[idx] );
	mDynamicCastGet( EM::Horizon3D*, hor, emobj );
	if ( hor )
	{
	    hor->ref();
	    hors_ += hor;
	    hsides_ += sides[idx];
	    horshift_ += horshift[idx];
	}
	else
	{
	    mDynamicCastGet( EM::Fault3D*, emflt, emobj );
	    Geometry::FaultStickSurface* flt =
		emflt ? emflt->geometry().sectionGeometry(0) : 0;
	    if ( !flt ) continue;

	    emflt->ref();
	    Geometry::ExplFaultStickSurface* efs =
		new Geometry::ExplFaultStickSurface(0,SI().zScale());
	    efs->setCoordList( new Coord3ListImpl, new Coord3ListImpl );
	    efs->setSurface( flt );
	    efs->update( true, 0 );
	    expflts_ += efs;
	    fsides_ += sides[idx];
	    flts_ += emflt;

	    computeFltOuterRange( *flt, sides[idx] );
	}
    }

    computeHorOuterRange();
    if ( !plg_.isEmpty() )
    {
	bidinplg_ = new Array2DImpl<unsigned char>(cs_.nrInl(),cs_.nrCrl());

	HorSamplingIterator iter( cs_.hrg );
	BinID bid;
	while( iter.next(bid) )
	{
	    const int inlidx = cs_.hrg.inlIdx(bid.inl());
	    const int crlidx = cs_.hrg.crlIdx(bid.crl());
	    bidinplg_->set( inlidx, crlidx, plg_.isInside(
		    Geom::Point2D<float>( mCast(float,bid.inl()),
					 mCast(float,bid.crl()) ),true,0.01 ) );
	}
    }
}


~ImplicitBodyRegionExtractor()
{
    delete bidinplg_;
    deepUnRef( hors_ );
    deepErase( expflts_ );
    deepUnRef( flts_ );
}

od_int64 nrIterations() const	{ return cs_.nrZ(); }
const char* message() const		{ return "Extracting implicit body"; }

bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    const int lastinlidx = cs_.nrInl()-1;
    const int lastcrlidx = cs_.nrCrl()-1;
    const int lastzidx = cs_.nrZ()-1;
    const int horsz = hors_.size();
    const int fltsz = fsides_.size();
    const bool usepolygon = !plg_.isEmpty();

    ObjectSet<Geometry::ExplPlaneIntersection> intersects;
    for ( int idx=0; idx<fltsz; idx++ )
    {
	intersects += new Geometry::ExplPlaneIntersection();
	intersects[idx]->setCoordList( new Coord3ListImpl, new Coord3ListImpl );
	intersects[idx]->setShape( *expflts_[idx] );
    }

    TypeSet<Coord3> corners;
    if ( fltsz )
    {
	corners += Coord3( SI().transform(cs_.hrg.start), 0 );
	const BinID cbid0( cs_.hrg.start.inl(), cs_.hrg.stop.crl() );
	corners += Coord3( SI().transform(cbid0), 0 );
	corners += Coord3( SI().transform(cs_.hrg.stop), 0 );
	const BinID cbid1( cs_.hrg.stop.inl(), cs_.hrg.start.crl() );
	corners += Coord3( SI().transform(cbid1), 0 );
    }
    const int cornersz = corners.size();

    for ( int idz=mCast(int,start); idz<=stop && shouldContinue();
	    idz++, addToNrDone(1) )
    {
	if ( !idz || idz==lastzidx )
	    continue;

	const double curz = cs_.zrg.atIndex( idz );
	bool outsidehorrg = false;
	for ( int hidx=0; hidx<horoutrgs_.size(); hidx++ )
	{
	    if ( horoutrgs_[hidx].includes(curz,false) )
	    {
		outsidehorrg = true;
		break;
	    }
	}

	if ( outsidehorrg )
	    continue;

	if ( fltsz )
	{
	    for ( int cidx=0; cidx<cornersz; cidx++ )
		corners[cidx].z = curz;

	    for ( int fidx=0; fidx<fltsz; fidx++ )
	    {
		if ( intersects[fidx]->nrPlanes() )
		    intersects[fidx]->setPlane( 0, Coord3(0,0,1), corners );
		else
		    intersects[fidx]->addPlane( Coord3(0,0,1), corners );

		intersects[fidx]->update( false, 0 );
	    }
	}

	HorSamplingIterator iter( cs_.hrg );
	BinID bid;
	ObjectSet<ODPolygon<float> > polygons;
	for ( int idx=0; idx<fltsz; idx++ )
	{
	    polygons += new ODPolygon<float>;
	    getPolygon( idx, intersects[idx], *polygons[idx] );
	}

	while( iter.next(bid) )
	{
	    const int inlidx = cs_.hrg.inlIdx(bid.inl());
	    const int crlidx = cs_.hrg.crlIdx(bid.crl());
	    if (!inlidx || !crlidx || inlidx==lastinlidx || crlidx==lastcrlidx)
		continue;

	    if ( usepolygon && !bidinplg_->get(inlidx,crlidx) )
		continue;

	    bool infltrg = true;
	    for ( int idy=0; idy<fltsz; idy++ )
	    {
		infltrg = inFaultRange(bid,idy,*polygons[idy]);
		if ( !infltrg ) break;
	    }
	    if ( !infltrg )
		continue;

	    if ( !horsz )
	    {
		res_.set( inlidx, crlidx, idz, -1 );
		continue;
	    }

	    float minz = mUdf(float);
	    float maxz = mUdf(float);
	    for ( int idy=0; idy<horsz; idy++ )
	    {
		const float hz = hors_[idy]->getZ(bid) + horshift_[idy];
		if ( mIsUdf(hz) ) continue;

		if ( hsides_[idy]==mBelow )
		{
		    if ( mIsUdf(minz) || minz<hz )
			minz = hz;
		}
		else if ( mIsUdf(maxz) || maxz>hz )
		    maxz = hz;
	    }

	    if ( mIsUdf(minz) && mIsUdf(maxz) )
		continue;

	    if ( mIsUdf(minz) ) minz = cs_.zrg.start;
	    if ( mIsUdf(maxz) ) maxz = cs_.zrg.stop;
	    if ( minz>=maxz )
		continue;

	    double val = curz < minz ? minz - curz :
		( curz > maxz ? curz - maxz : -mMIN(curz-minz, maxz-curz) );
	    res_.set( inlidx, crlidx, idz, (float) val );
	}

	deepErase( polygons );
    }

    deepErase( intersects );
    return true;
}

void getPolygon( int curidx, Geometry::ExplPlaneIntersection* epi,
                 ODPolygon<float>& poly )
{
   if (  !epi || !epi->getPlaneIntersections().size() )
	return;

    const char side = fsides_[curidx];
    const TypeSet<Coord3>& crds = epi->getPlaneIntersections()[0].knots_;
    const TypeSet<int>& conns = epi->getPlaneIntersections()[0].conns_;
    const int sz = crds.size();
    if ( sz<2 )
	return;

    TypeSet<int> edgeids;
    for ( int idx=0; idx<conns.size(); idx++ )
    {
	const int index = conns[idx];
	if ( index == -1 )
	    continue;

	int count = 0;
	for ( int cidx=0; cidx<conns.size(); cidx++ )
	{
	    const int cindex = conns[cidx];
	    if ( cindex == -1 )
		continue;

	    if ( index == cindex )
		count++;
	}

	if ( count == 1 )
	    edgeids += index;
    }

    if ( edgeids.isEmpty() )
	return;

    TypeSet<int> crdids;
    crdids += edgeids[0];
    edgeids -= edgeids[0];

    while ( !edgeids.isEmpty() && crdids.size() < sz )
    {
	bool foundmatch = false;
	for ( int idx=0; idx<conns.size()-1; idx++ )
	{
	    const int index = conns[idx];
	    const int nextindex = conns[idx+1];
	    if ( index == -1 || nextindex == -1 )
		continue;

	    const int lastidx = crdids.last();
	    if ( index == lastidx && !crdids.isPresent(nextindex) )
	    {
		crdids += nextindex;
		foundmatch = true;
		break;
	    }
	    else if ( nextindex == lastidx && !crdids.isPresent(index) )
	    {
		crdids += index;
		foundmatch = true;
		break;
	    }
	}

	if ( foundmatch == false  )
	{
	    if ( !crdids.isPresent(edgeids[0]) )
		crdids += edgeids[0];

	    edgeids -= edgeids[0];
	}
    }


    mAllocVarLenArr(int,ids,sz);
    TypeSet< Geom::Point2D<float> > bidpos;
    for ( int idx=0; idx<sz; idx++ )
    {
	ids[idx] = crdids[idx];
	BinID bid = SI().transform( crds[idx] );
	bidpos += Geom::Point2D<float>( mCast(float,bid.inl()),
					mCast(float,bid.crl()) );
    }

    poly.setClosed( true );
    for ( int idx=0; idx<sz; idx++ )
	poly.add( bidpos[ids[idx]] );

    const bool ascending = poly.data()[sz-1].y > poly.data()[0].y;

    if ( (side==mToMinInline && ascending) ||
	 (side==mToMaxCrossline && ascending) )
    {
	poly.add( c_[2] );
	poly.add( c_[3] );
	poly.add( c_[0] );
    }
    else if ( (side==mToMinInline && !ascending) ||
	      (side==mToMinCrossline && !ascending) )
    {
	poly.add( c_[1] );
	poly.add( c_[0] );
	poly.add( c_[3] );
    }
    else if ( (side==mToMaxInline && ascending) ||
	      (side==mToMinCrossline && ascending) )
    {
	poly.add( c_[2] );
	poly.add( c_[1] );
	poly.add( c_[0] );
    }
    else
    {
	poly.add( c_[1] );
	poly.add( c_[2] );
	poly.add( c_[3] );
    }
}


bool inFaultRange( const BinID& pos, int curidx, ODPolygon<float>& poly )
{
    const char side = fsides_[curidx];
    const int ic = side==mToMinInline || side==mToMaxInline
	? pos.inl() : pos.crl();
    if ( outsidergs_[curidx].includes(ic,false) )
	return false;

    if ( insidergs_[curidx].includes(ic,false) )
	return true;

    if ( poly.isEmpty() )
	return true;

    return poly.isInside(
	Geom::Point2D<float>( mCast(float,pos.inl()),
			      mCast(float,pos.crl())), true, 0 );
}


void computeHorOuterRange()
{
    Interval<double> hortopoutrg(mUdf(float),mUdf(float));
    Interval<double> horbotoutrg(mUdf(float),mUdf(float));
    for ( int idx=0; idx<hors_.size(); idx++ )
    {
	const Geometry::BinIDSurface* surf =
	    hors_[idx]->geometry().sectionGeometry(hors_[idx]->sectionID(0));
	const Array2D<float>* depth = surf ? surf->getArray() : 0;
	const int sz = depth ? mCast( int,depth->info().getTotalSz() ) : 0;
	if ( !sz ) continue;

	const float* data = depth->getData();
	Interval<float> zrg;
	bool defined = false;
	for ( int idz=0; idz<sz; idz++ )
	{
	    if ( mIsUdf(data[idz]) )
		continue;

	    if ( !defined )
	    {
		zrg.start = zrg.stop = data[idz];
		defined = true;
	    }
	    else
		zrg.include( data[idz], false );
	}

	if ( !defined )
	    continue;

	if ( hsides_[idx]==mBelow )
	{
	    if ( hortopoutrg.isUdf() )
		hortopoutrg.set(cs_.zrg.start,zrg.start);
	    else if ( hortopoutrg.stop>zrg.start )
		hortopoutrg.stop = zrg.start;
	}
	else
	{
	    if ( horbotoutrg.isUdf() )
		horbotoutrg.set(zrg.stop,cs_.zrg.stop);
	    else if ( horbotoutrg.start<zrg.stop )
		hortopoutrg.start = zrg.stop;
	}
    }

    if ( !hortopoutrg.isUdf() && hortopoutrg.start<hortopoutrg.stop )
	horoutrgs_ += hortopoutrg;
    if ( !horbotoutrg.isUdf() && horbotoutrg.start<horbotoutrg.stop )
	horoutrgs_ += horbotoutrg;
}


void computeFltOuterRange( const Geometry::FaultStickSurface& flt, char side )
{
    HorSampling hrg(false);
    for ( int idx=0; idx<flt.nrSticks(); idx++ )
    {
	const TypeSet<Coord3>* stick = flt.getStick(idx);
	if ( !stick ) continue;

	for ( int idy=0; idy<stick->size(); idy++ )
	    hrg.include( SI().transform((*stick)[idy]) );
    }

    Interval<int> insiderg;
    Interval<int> outsiderg;

    if ( side==mToMinInline )
    {
	insiderg.set( cs_.hrg.start.inl(), hrg.start.inl() );
	outsiderg.set( hrg.stop.inl(), cs_.hrg.stop.inl() );
    }
    else if ( side==mToMaxInline )
    {
	insiderg.set( hrg.stop.inl(), cs_.hrg.stop.inl() );
	outsiderg.set( cs_.hrg.start.inl(), hrg.start.inl() );
    }
    else if ( side==mToMinCrossline )
    {
	insiderg.set( cs_.hrg.start.crl(), hrg.start.crl() );
	outsiderg.set( hrg.stop.crl(), cs_.hrg.stop.crl() );
    }
    else
    {
	insiderg.set( hrg.stop.crl(), cs_.hrg.stop.crl() );
	outsiderg.set( cs_.hrg.start.crl(), hrg.start.crl() );
    }

    insidergs_ += insiderg;
    outsidergs_ += outsiderg;
}

Array3D<float>&					res_;
const CubeSampling&				cs_;
Geom::Point2D<float>				c_[4];

ObjectSet<EM::Horizon3D>			hors_;
TypeSet<char>					hsides_;
TypeSet<float>					horshift_;

ObjectSet<EM::Fault3D>				flts_;
TypeSet<char>					fsides_;

ObjectSet<Geometry::ExplFaultStickSurface>	expflts_;
TypeSet< Interval<int> >			insidergs_;
TypeSet< Interval<int> >			outsidergs_;
TypeSet< Interval<double> >			horoutrgs_;
const ODPolygon<float>&				plg_;
Array2D<unsigned char>*				bidinplg_;
};


uiBodyRegionDlg::uiBodyRegionDlg( uiParent* p )
    : uiDialog( p, Setup("Region constructor","Boundary settings",
                        mODHelpKey(mBodyRegionDlgHelpID) ) )
    , singlehoradded_(false)
{
    setCtrlStyle( RunAndClose );

    subvolfld_ =  new uiPosSubSel( this,  uiPosSubSel::Setup( !SI().has3D(),
		true).choicetype(uiPosSubSel::Setup::RangewithPolygon).
		seltxt("Geometry boundary").withstep(false) );

    singlehorfld_ = new uiGenInput( this, "Apply", BoolInpSpec(false,
		"Single horizon wrapping", "Multiple horizon layers") );
    singlehorfld_->attach( alignedBelow, subvolfld_ );
    singlehorfld_->valuechanged.notify(mCB(this,uiBodyRegionDlg,horModChg));

    uiTable::Setup tsu( 4, 4 );
    table_ = new uiTable( this, tsu.rowdesc("Boundary").defrowlbl(true), "Sf" );
    BufferStringSet lbls; lbls.add("Name").add("Region location").add(
	    "Relative horizon shift").add(" ");
    table_->attach( alignedBelow, singlehorfld_ );
    table_->setColumnLabels( lbls );
    table_->setPrefWidth( 600 );
    table_->setColumnResizeMode( uiTable::ResizeToContents );
    table_->resizeColumnsToContents();
    table_->setTableReadOnly( true );

    addhorbutton_ = new uiPushButton( this, "&Add horizon",
	    mCB(this,uiBodyRegionDlg,addSurfaceCB), false );
    addhorbutton_->attach( rightOf, table_ );

    addfltbutton_ = new uiPushButton( this, "&Add fault",
	    mCB(this,uiBodyRegionDlg,addSurfaceCB), false );
    addfltbutton_->attach( alignedBelow, addhorbutton_ );

    removebutton_ = new uiPushButton( this, "&Remove",
	    mCB(this,uiBodyRegionDlg,removeSurfaceCB), false );
    removebutton_->attach( alignedBelow, addfltbutton_ );
    removebutton_->setSensitive( false );

    outputfld_ = new uiIOObjSel( this, mWriteIOObjContext(EMBody) );
    outputfld_->attach( alignedBelow, table_ );
}


uiBodyRegionDlg::~uiBodyRegionDlg()
{}


MultiID uiBodyRegionDlg::getBodyMid() const
{
    return outputfld_->key();
}


void uiBodyRegionDlg::horModChg( CallBacker* cb )
{
    table_->clearTable();
    table_->selectRow( 0 );
    surfacelist_.erase();

    const bool singlehormod = singlehorfld_->getBoolValue();
    BufferStringSet lbls;
    lbls.add("Name").add("Region location");
    if ( singlehormod )
	lbls.add("Relative shift up").add("Relative shift down");
    else
	lbls.add("Relative horizon shift").add("  ");

    table_->setColumnLabels( lbls );
    table_->resizeColumnsToContents();
    singlehoradded_ = false;
    removebutton_->setSensitive( false );
}


void uiBodyRegionDlg::addSurfaceCB( CallBacker* cb )
{
    const bool isflt = addfltbutton_==cb;
    if ( !isflt && addhorbutton_!=cb )
	return;

    PtrMan<CtxtIOObj> objio =  isflt ? mMkCtxtIOObj(EMFault3D)
				     : mMkCtxtIOObj(EMHorizon3D);
    uiIOObjSelDlg::Setup sdsu; sdsu.multisel( true );
    PtrMan<uiIOObjSelDlg> dlg = new uiIOObjSelDlg( this, sdsu, *objio );
    if ( !dlg->go() )
	return;

    const bool singlehormod = singlehorfld_->getBoolValue();
    const int nrsel = dlg->nrChosen();
    for ( int idx=0; idx<nrsel; idx++ )
    {
	const MultiID& mid = dlg->chosenID( idx );
	if ( isflt )
	{
	    if ( surfacelist_.isPresent(mid) )
		continue;
	}
	else
	{
	    int count = 0;
	    for ( int idy=0; idy<surfacelist_.size(); idy++ )
	    {
		if ( surfacelist_[idy]==mid )
		    count++;
	    }
	    if ( count>1 )
		continue;
	}

	PtrMan<IOObj> ioobj = IOM().get( mid );
	if ( isflt || !singlehormod )
	    addSurfaceTableEntry( *ioobj, isflt, 0 );
	else
	{
	    if ( singlehoradded_ )
	    {
		uiMSG().message("You already picked a horizon");
		return;
	    }

	    singlehoradded_ = true;
	    addSurfaceTableEntry( *ioobj, isflt, 0 );
	}
    }
}


void uiBodyRegionDlg::addSurfaceTableEntry( const IOObj& ioobj,	bool isfault,
	char side )
{
    const int row = surfacelist_.size();
    if ( row==table_->nrRows() )
	table_->insertRows( row, 1 );

    const bool singlehormod = singlehorfld_->getBoolValue();

    BufferStringSet sidenms;
    if ( isfault )
    {
	sidenms.add("Between min In-line and fault");
	sidenms.add("Between max In-line and fault");
	sidenms.add("Between min Cross-line and fault");
	sidenms.add("Between max Cross-line and fault");
    }
    else if ( !singlehormod )
    {
	sidenms.add("Below horizon+shift (Z increase side)");
	sidenms.add("Above horizon+shift (Z decrease side)");
    }

    if ( isfault || (!isfault && !singlehormod) )
    {
	uiComboBox* sidesel = new uiComboBox( 0, sidenms, 0 );
	sidesel->setCurrentItem( side==-1 ? 0 : 1 );
	table_->setCellObject( RowCol(row,cSideCol), sidesel );
    }

    table_->setText( RowCol(row,cNameCol), ioobj.name() );
    table_->setCellReadOnly( RowCol(row,cNameCol), true );

    if ( !isfault )
    {
	if ( singlehormod )
	{
	    BufferString unt( " ", SI().getZUnitString() );

	    uiSpinBox* shiftupfld = new uiSpinBox( 0, 0, "Shift up" );
	    shiftupfld->setSuffix( unt.buf() );
	    table_->setCellObject( RowCol(row,cHorShiftUpCol), shiftupfld );

	    uiSpinBox* shiftdownfld = new uiSpinBox( 0, 0, "Shift down" );
	    shiftdownfld->setSuffix( unt.buf() );
	    table_->setCellObject( RowCol(row,cHorShiftDownCol), shiftdownfld );
	}
	else
	{
	    uiSpinBox* shiftfld = new uiSpinBox( 0, 0, "Shift" );
	    BufferString unt( " ", SI().getZUnitString() );
	    shiftfld->setSuffix( unt.buf() );
	    shiftfld->setMinValue( -INT_MAX );
	    table_->setCellObject( RowCol(row,cRelLayerCol), shiftfld );
	}
    }

    surfacelist_ += ioobj.key();
    removebutton_->setSensitive( surfacelist_.size() );
}


void uiBodyRegionDlg::removeSurfaceCB( CallBacker* )
{
    const int currow = table_->currentRow();
    if ( currow==-1 ) return;

    if ( currow<surfacelist_.size() )
    {
	if ( singlehorfld_->getBoolValue() )
	{
	    const FixedString objtype
			= EM::EMM().objectType( surfacelist_[currow] );
	    if ( objtype == sKey::Horizon() )
		singlehoradded_ = false;
	}

	surfacelist_.removeSingle( currow );
    }

    table_->removeRow( currow );
    if ( table_->nrRows() < 4 )
	table_->setNrRows( 4 );

    removebutton_->setSensitive( surfacelist_.size() );
}


#define mRetErr(msg)  { uiMSG().error( msg ); return false; }
#define mRetErrDelHoridx(msg)  \
{  \
    if ( duplicatehoridx>=0 ) \
	surfacelist_.removeSingle( duplicatehoridx ); \
    mRetErr(msg) \
}


bool uiBodyRegionDlg::acceptOK( CallBacker* cb )
{
    if ( !surfacelist_.size() )
	mRetErr("Please select at least one boundary");

    if ( outputfld_->isEmpty() )
	mRetErr("Please select choose a name for the output");

    if ( !outputfld_->commitInput() )
	return false;

    if ( createImplicitBody() )
    {
	BufferString msg = "The body ";
	msg += outputfld_->getInput();
	msg += " created successfully";
	uiMSG().message( msg.buf() );
    }

    return false; //Make the dialog stay.
}



bool uiBodyRegionDlg::createImplicitBody()
{
    MouseCursorChanger mcc( MouseCursor::Wait );

    const bool singlehormod = singlehorfld_->getBoolValue();

    TypeSet<char> sides;
    TypeSet<float> horshift;
    bool hasfaults = false;
    int duplicatehoridx = -1;

    for ( int idx=0; idx<surfacelist_.size(); idx++ )
    {
	mDynamicCastGet(uiComboBox*, selbox,
		table_->getCellObject(RowCol(idx,cSideCol)) );
	if ( singlehormod && !selbox )
	{
	    mDynamicCastGet(uiSpinBox*, shiftupfld,
		    table_->getCellObject(RowCol(idx,cHorShiftUpCol)) );

	    mDynamicCastGet(uiSpinBox*, shiftdownfld,
		    table_->getCellObject(RowCol(idx,cHorShiftDownCol)) );
	    if ( !shiftupfld->getValue() && !shiftdownfld->getValue() )
		mRetErr("You did not choose any horizon shift");

	    duplicatehoridx = idx;
	    sides += mBelow;
	    horshift += -shiftupfld->getValue()/SI().zScale();
	    sides += mAbove;
	    horshift += shiftdownfld->getValue()/SI().zScale();
	}
	else
	{
	    sides += mCast(char,selbox->currentItem());
	    mDynamicCastGet(uiSpinBox*, shiftfld,
		    table_->getCellObject(RowCol(idx,cRelLayerCol)) );
	    horshift += shiftfld ? shiftfld->getValue()/SI().zScale() : 0;
	}

	if ( !hasfaults )
	{
	    RefMan<EM::EMObject> emobj =
		EM::EMM().loadIfNotFullyLoaded( surfacelist_[idx] );
	    mDynamicCastGet(EM::Fault3D*,emflt,emobj.ptr());
	    if ( emflt ) hasfaults = true;
	}
    }

    if ( duplicatehoridx>=0 )
	surfacelist_.insert( duplicatehoridx, surfacelist_[duplicatehoridx] );

    CubeSampling cs = subvolfld_->envelope();
    cs.zrg.start -= cs.zrg.step; cs.zrg.stop += cs.zrg.step;
    cs.hrg.start.inl() -= cs.hrg.step.inl();
    cs.hrg.stop.inl() += cs.hrg.step.inl();
    cs.hrg.start.crl() -= cs.hrg.step.crl();
    cs.hrg.stop.crl() += cs.hrg.step.crl();

    mDeclareAndTryAlloc( Array3DImpl<float>*, arr,
	    Array3DImpl<float> (cs.nrInl(),cs.nrCrl(),cs.nrZ()) );
    if ( !arr )
	mRetErrDelHoridx("Can not allocate disk space to create region.")

    uiTaskRunner taskrunner( this );
    ODPolygon<float> dummy;
    mDynamicCastGet(Pos::PolyProvider3D*,plgp,subvolfld_->curProvider());

    if ( hasfaults )
    {
	ImplicitBodyRegionExtractor ext( surfacelist_, sides, horshift, cs,
		*arr, plgp ? plgp->polygon() : dummy );

	if ( !TaskRunner::execute( &taskrunner, ext ) )
	    mRetErrDelHoridx("Extracting body region failed.")
    }
    else
    {
	BodyExtractorFromHorizons ext( surfacelist_, sides, horshift, cs, *arr,
		plgp ? plgp->polygon() : dummy );
	if ( !TaskRunner::execute( &taskrunner, ext ) )
	    mRetErrDelHoridx("Extracting body from horizons failed.")
    }

    RefMan<EM::MarchingCubesSurface> emcs =
	new EM::MarchingCubesSurface(EM::EMM());

    emcs->surface().setVolumeData( 0, 0, 0, *arr, 0, &taskrunner);
    emcs->setInlSampling(
	    SamplingData<int>(cs.hrg.start.inl(),cs.hrg.step.inl()));
    emcs->setCrlSampling(
	    SamplingData<int>(cs.hrg.start.crl(),cs.hrg.step.crl()));
    emcs->setZSampling(SamplingData<float>(cs.zrg.start,cs.zrg.step));

    emcs->setMultiID( outputfld_->key() );
    emcs->setName( outputfld_->getInput() );
    emcs->setFullyLoaded( true );
    emcs->setChangedFlag();

    EM::EMM().addObject( emcs );
    PtrMan<Executor> exec = emcs->saver();
    if ( !exec )
	mRetErrDelHoridx( "Body saving failed" )

    MultiID key = emcs->multiID();
    PtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj->pars().find( sKey::Type() ) )
    {
	ioobj->pars().set( sKey::Type(), emcs->getTypeStr() );
	if ( !IOM().commitChanges( *ioobj ) )
	    mRetErrDelHoridx( "Writing body to disk failed, no permision?" )
    }

    if ( !TaskRunner::execute( &taskrunner, *exec ) )
	mRetErrDelHoridx("Saving body failed");

    return true;
}
