/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Nov 2008
-*/
static const char* rcsID = "$Id: seispreload.cc,v 1.7 2009-02-18 17:12:19 cvsbert Exp $";

#include "seispreload.h"
#include "seistrctr.h"
#include "seispsioprov.h"
#include "seiscbvs2d.h"
#include "seiscbvsps.h"
#include "seis2dline.h"
#include "seisioobjinfo.h"
#include "filepath.h"
#include "filegen.h"
#include "keystrs.h"
#include "ioman.h"
#include "strmprov.h"
#include "cbvsio.h"

const char* Seis::PreLoader::sKeyLines()	{ return "Lines"; }
const char* Seis::PreLoader::sKeyAttrs()	{ return "Attributes"; }


IOObj* Seis::PreLoader::getIOObj() const
{
    IOObj* ret = IOM().get( id_ );
    if ( !ret )
	errmsg_ = "Cannot find ID in object manager";
    return ret;
}


Interval<int> Seis::PreLoader::inlRange() const
{
    Interval<int> ret( -mUdf(int), mUdf(int) );
    BufferStringSet fnms;
    StreamProvider::getPreLoadedFileNames( id_.buf(), fnms );
    for ( int idx=0; idx<fnms.size(); idx++ )
	ret.include( SeisCBVSPSIO::getInlNr( fnms.get(idx) ) );

    if ( mIsUdf(ret.stop) ) ret.start = ret.stop;
    return ret;
}


void Seis::PreLoader::getLineKeys( BufferStringSet& lks ) const
{
    lks.erase(); PtrMan<IOObj> ioobj = getIOObj();
    if ( !ioobj ) return;

    BufferStringSet fnms;
    StreamProvider::getPreLoadedFileNames( id_.buf(), fnms );
    if ( fnms.isEmpty() ) return;
    BufferStringSet nms;
    for ( int idx=0; idx<fnms.size(); idx++ )
    {
	FilePath fp( fnms.get(idx) );
	nms.add( fp.fileName() );
    }

    Seis2DLineSet ls( *ioobj );
    const int nrlns = ls.nrLines();
    for ( int iln=0; iln<nrlns; iln++ )
    {
	const IOPar& iop = ls.getInfo( iln );
	const char* fnm = iop.find( sKey::FileName );
	if ( !fnm ) continue;

	const int idxof = nms.indexOf( fnm );
	if ( idxof >= 0 )
	    lks.add( ls.lineKey(iln).buf() );
    }
}


#define mPrepIOObj(is2dln) \
    PtrMan<IOObj> ioobj = getIOObj(); \
    if ( !ioobj ) \
	return false; \
    if ( !is2dln && strcmp(ioobj->translator(),"CBVS") ) \
	{ errmsg_ = "Cannot pre-load other than CBVS data"; return false; } \
    TaskRunner& trunnr = getTr()


bool Seis::PreLoader::loadVol() const
{
    mPrepIOObj( false );

    BufferStringSet fnms;
    const BufferString basefnm = CBVSIOMgr::baseFileName(
				 ioobj->fullUserExpr(true) );
    for ( int idx=0; true; idx++ )
    {
	const BufferString fnm( CBVSIOMgr::getFileName(basefnm,idx) );
	if ( File_exists(fnm) )
	    fnms.add( fnm );
	else
	    break;
    }

    return StreamProvider::preLoad( fnms, trunnr, id_.buf() );
}


bool Seis::PreLoader::loadLines( const BufferStringSet& lnms,
				 const BufferStringSet& attrnms ) const
{
    if ( lnms.isEmpty() || attrnms.isEmpty() )
	{ errmsg_ = "Internal: No line or no attr requested"; return false; }
    mPrepIOObj( true );

    Seis2DLineSet ls( *ioobj );
    const int nrlns = ls.nrLines();
    if ( nrlns < 1 )
    {
	errmsg_ = "Line Set '"; errmsg_ += ioobj->name();
	errmsg_ += "' contains no data";
	return false;
    }

    BufferStringSet fnms;
    for ( int iln=0; iln<nrlns; iln++ )
    {
	const char* lnm = ls.lineName( iln );
	const char* attrnm = ls.attribute( iln );
	if ( lnms.indexOf(lnm) < 0 || attrnms.indexOf(attrnm) < 0 )
	    continue;

	fnms.add( SeisCBVS2DLineIOProvider::getFileName(ls.getInfo(iln)) );
    }

    return fnms.isEmpty() ? true
	 : StreamProvider::preLoad( fnms, trunnr, id_.buf() );
}


bool Seis::PreLoader::loadLines() const
{
    mPrepIOObj( true );
    Seis2DLineSet ls( *ioobj );
    const int nrlns = ls.nrLines();
    if ( nrlns < 1 )
    {
	errmsg_ = "Line Set '"; errmsg_ += ioobj->name();
	errmsg_ += "' contains no data";
	return false;
    }

    BufferStringSet fnms;
    for ( int iln=0; iln<nrlns; iln++ )
	fnms.add( SeisCBVS2DLineIOProvider::getFileName(ls.getInfo(iln)) );

    return fnms.isEmpty() ? true
	 : StreamProvider::preLoad( fnms, trunnr, id_.buf() );
}


bool Seis::PreLoader::loadPS3D( const Interval<int>* inlrg ) const
{
    mPrepIOObj( false );

    SeisCBVSPSIO psio( ioobj->fullUserExpr(true) );
    BufferStringSet fnms;
    if ( !psio.get3DFileNames(fnms,inlrg) )
	{ errmsg_ = psio.errMsg(); return false; }

    return fnms.isEmpty() ? true
	 : StreamProvider::preLoad( fnms, trunnr, id_.buf() );
}


bool Seis::PreLoader::loadPS2D( const char* lnm ) const
{
    mPrepIOObj( false );
    BufferStringSet lnms;
    if ( lnm && *lnm )
	lnms.add( lnm );
    else
	SPSIOPF().getLineNames( *ioobj, lnms );

    return loadPS2D( lnms );
}


bool Seis::PreLoader::loadPS2D( const BufferStringSet& lnms ) const
{
    if ( lnms.isEmpty() )
	return true;

    mPrepIOObj( false );

    BufferStringSet fnms;
    SeisCBVSPSIO psio( ioobj->fullUserExpr(true) );
    for ( int idx=0; idx<lnms.size(); idx++ )
	fnms.add( psio.get2DFileName(lnms.get(idx)) );

    return StreamProvider::preLoad( fnms, trunnr, id_.buf() );
}


void Seis::PreLoader::unLoad() const
{
    StreamProvider::unLoad( id_.buf(), true );
}


void Seis::PreLoader::load( const IOPar& iniop, TaskRunner* tr )
{
    static BufferString errmsg;

    PtrMan<IOPar> iop = iniop.subselect( "Seis" );
    if ( !iop || iop->isEmpty() ) return;

    for ( int ipar=0; ; ipar++ )
    {
	PtrMan<IOPar> objiop = iniop.subselect( ipar );
	if ( !iop || iop->isEmpty() )
	    { if ( ipar ) break; continue; }
	loadObj( *objiop, tr );
    }
}


void Seis::PreLoader::loadObj( const IOPar& iop, TaskRunner* tr )
{
    const char* id = iop.find( sKey::ID );
    if ( !id || !*id ) return;

    const MultiID ky( id );
    SeisIOObjInfo oinf( ky );
    if ( !oinf.isOK() ) return;

    Seis::PreLoader spl( ky, tr );
    const Seis::GeomType gt = oinf.geomType();
    switch ( gt )
    {
	case Seis::Vol:
	    spl.loadVol();
	break;
	case Seis::Line: {
	    BufferStringSet lnms, attrs;
	    iop.get( sKeyLines(), lnms ); iop.get( sKeyAttrs(), attrs );
	    if ( lnms.isEmpty() && attrs.isEmpty() )
		spl.loadLines();
	    else
	    {
		if ( lnms.isEmpty() ) oinf.getLineNames(lnms);
		if ( attrs.isEmpty() ) oinf.getAttribNames(attrs);
		spl.loadLines( lnms, attrs );
	    }
	 } break;
	case Seis::VolPS: {
	    Interval<int> nrrg; Interval<int>* toload = 0;
	    if ( iop.get(sKey::Range,nrrg) )
		toload = &nrrg;
	    spl.loadPS3D( toload );
	} break;
	case Seis::LinePS: {
	    BufferStringSet lnms; iop.get( sKeyLines(), lnms );
	    if ( lnms.isEmpty() )
		spl.loadPS2D();
	    else
		spl.loadPS2D( lnms );
	} break;
    }
}
