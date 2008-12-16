/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		August 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: gmtcontour.cc,v 1.7 2008-12-16 06:26:43 cvsraman Exp $";

#include "gmtcontour.h"

#include "coltabsequence.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "filepath.h"
#include "ioobj.h"
#include "keystrs.h"
#include "pickset.h"
#include "strmdata.h"
#include "strmprov.h"
#include "survinfo.h"



int GMTContour::factoryid_ = -1;

void GMTContour::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Contour", GMTContour::createInstance );
}

GMTPar* GMTContour::createInstance( const IOPar& iop )
{
    return new GMTContour( iop );
}


const char* GMTContour::userRef() const
{
    BufferString* str = new BufferString( "Contour: " );
    const char* nm = find( sKey::Name );
    *str += nm;
    return str->buf();
}


bool GMTContour::fillLegendPar( IOPar& par ) const
{
    BufferString str = find( sKey::Name );
    par.set( sKey::Name, str );
    bool drawcontour = false;
    getYN( ODGMT::sKeyDrawContour, drawcontour );
    if ( drawcontour )
    {
	par.set( ODGMT::sKeyShape, "Line" );
	par.set( sKey::Size, 1 );
	str = find( ODGMT::sKeyLineStyle );
	par.set( ODGMT::sKeyLineStyle, str );
    }

    bool dofill = false;
    getYN( ODGMT::sKeyFill, dofill );
    if ( dofill )
    {
	par.set( ODGMT::sKeyPostColorBar, true );
	str = find( ODGMT::sKeyDataRange );
	par.set( ODGMT::sKeyDataRange, str );
    }

    return true;
}


bool GMTContour::execute( std::ostream& strm, const char* fnm )
{
    MultiID id;
    get( sKey::ID, id );
    bool closeps, drawcontour, dofill;
    getYN( ODGMT::sKeyClosePS, closeps );
    getYN( ODGMT::sKeyDrawContour, drawcontour );
    getYN( ODGMT::sKeyFill, dofill );

    const char* hornm = find( sKey::Name );
    strm << "Loading horizon " << hornm << " ...  ";
    strm.flush();
    EM::SurfaceIOData sd;
    PtrMan<IOPar> subpar = subselect( sKey::Selection );
    if ( !subpar )
	mErrStrmRet("Missing subselection")

    sd.rg.usePar( *subpar );
    PtrMan<EM::SurfaceIODataSelection> sel = new EM::SurfaceIODataSelection(sd);
    PtrMan<Executor> exec = EM::EMM().objectLoader( id, sel );
    if ( !exec || !exec->execute() )
	mErrStrmRet("Cannot load horizon")

    EM::ObjectID objid = EM::EMM().getObjectID( id );
    EM::EMObject* obj = EM::EMM().getObject( objid );
    if ( !obj )
	mErrStrmRet("Failed");

    strm << "Done" << std::endl;
    obj->ref();

    FilePath fp( fnm );
    fp.setExtension( "cpt" );
    BufferString cptfnm = fp.fullPath();
    strm << "Creating color pallette file ...  ";
    if ( !makeCPT(cptfnm.buf()) )
	mErrStrmRet("Failed")

    strm << "Done" << std::endl;
    strm << "Creating grid 100 X 100 ...  ";
    Coord spt1 = SI().transform( BinID(sd.rg.start.inl,sd.rg.start.crl) );
    Coord spt2 = SI().transform( BinID(sd.rg.start.inl,sd.rg.stop.crl) );
    Coord spt3 = SI().transform( BinID(sd.rg.stop.inl,sd.rg.start.crl) );
    Coord spt4 = SI().transform( BinID(sd.rg.stop.inl,sd.rg.stop.crl) );
    Coord botleft( mMIN( mMIN( spt1.x, spt2.x ), mMIN( spt3.x, spt4.x ) ),
	    	   mMIN( mMIN( spt1.y, spt2.y ), mMIN( spt3.y, spt4.y ) ) );
    Coord topright( mMAX( mMAX( spt1.x, spt2.x ), mMAX( spt3.x, spt4.x ) ),
	    	    mMAX( mMAX( spt1.y, spt2.y ), mMAX( spt3.y, spt4.y ) ) );
    fp.setExtension( "gd1" );
    BufferString grd100fnm = fileName( fp.fullPath() );
    BufferString rstr = "-R";
    rstr += botleft.x; rstr += "/"; rstr += topright.x; rstr += "/";
    rstr += botleft.y; rstr += "/"; rstr += topright.y;
    BufferString comm = "@blockmean "; comm += rstr;
    comm += " -I100 | surface "; comm += rstr; comm += " -I100 -T0.7 -N250 -G";
    comm += grd100fnm;
    StreamData sdata = StreamProvider(comm).makeOStream();
    if ( !sdata.usable() ) mErrStrmRet("Failed")

    HorSamplingIterator iter( sd.rg );
    BinID bid;
    EM::SectionID sid = obj->sectionID( 0 );
    const float fac = SI().zFactor();
    while ( iter.next(bid) )
    {
	EM::SubID subid = bid.getSerialized();
	Coord3 pos = obj->getPos( sid, subid );
	if ( !pos.isDefined() )
	    continue;

	*sdata.ostrm << pos.x << " " << pos.y << " " << fac*pos.z << std::endl;
    }

    obj->unRef();
    sdata.close();
    strm << "Done" << std::endl;
    strm << "Regridding 25 X 25 ...  ";
    comm = "grdsample ";
    comm += grd100fnm; comm += " -I25 -G";
    fp.setExtension( "gd2" );
    comm += fileName( fp.fullPath() );
    if ( system(comm) )
	mErrStrmRet("Failed")

    strm << "Done" << std::endl;

    mDynamicCastGet(EM::Horizon3D*,hor,obj)
    Pick::Set ps;
    BufferString finalgrd = fileName( fp.fullPath() );
    if ( hor->geometry().getBoundingPolygon(hor->sectionID(0),ps) )
    {
	strm << "Clipping the grid ... ";
	fp.setExtension( "gd3" );
	BufferString maskgrd = fileName( fp.fullPath() );
	comm = "@grdmask -R -NNaN/NaN/1 -I25 -G";
	comm += maskgrd;
	sdata = StreamProvider(comm).makeOStream();
	if ( !sdata.usable() )
	    mErrStrmRet("Failed")

	for ( int pdx=0; pdx<ps.size(); pdx++ )
	    *sdata.ostrm << ps[pdx].pos.x << " " << ps[pdx].pos.y << std::endl;

	sdata.close();
	fp.setExtension( "gd2" );
	comm = "grdmath ";
	comm += fileName( fp.fullPath() );
	comm += " "; comm += maskgrd;
	comm += " OR = ";
	fp.setExtension( "gd4" );
	comm += fileName( fp.fullPath() );
	if ( system(comm) )
	    mErrStrmRet("Failed")

	strm << "Done" << std::endl;
	StreamProvider( maskgrd ).remove();
	StreamProvider( finalgrd ).remove();
	finalgrd = fileName( fp.fullPath() );
    }

    BufferString mapprojstr;
    mGetRangeProjString( mapprojstr, "X" );
    if ( dofill )
    {
	strm << "Filling colors ...  ";
	comm = "grdimage "; comm += finalgrd;
	comm += " "; comm += mapprojstr;
	comm += " -O -Q -C"; comm += fileName( cptfnm );
	if ( !closeps || drawcontour )
	    comm += " -K";

	comm += " >> "; comm += fileName( fnm );
	if ( system(comm) )
	    mErrStrmRet("Failed")

	strm << "Done" << std::endl;
    }

    if ( drawcontour )
    {
	strm << "Drawing contours ...  ";
	BufferString lskey = find( ODGMT::sKeyLineStyle );
	LineStyle ls; ls.fromString( lskey.buf() );
	BufferString lsstr;
	mGetLineStyleString( ls, lsstr );
	comm = "grdcontour "; comm += finalgrd;
	comm += " "; comm += mapprojstr;
	comm += " -O -C"; comm += fileName( cptfnm );
	BufferString colstr; mGetColorString( ls.color_, colstr );
	comm += " -A+k"; comm += colstr;
	comm += " -W"; comm += lsstr;
	if ( !closeps )
	    comm += " -K";

	comm += " >> "; comm += fileName( fnm );
	if ( system(comm) )
	    mErrStrmRet("Failed")

	strm << "Done" << std::endl;
    }

    strm << "Removing temporary grid files ...  ";
    StreamProvider( grd100fnm ).remove();
    StreamProvider( finalgrd ).remove();
    strm << "Done" << std::endl;
    return true;
}


#define mPrintCol( col, endchar ) \
    *sd.ostrm << (int)col.r() << "\t"; \
    *sd.ostrm << (int)col.g() << "\t"; \
    *sd.ostrm << (int)col.b() << endchar;

bool GMTContour::makeCPT( const char* cptfnm ) const
{
    StepInterval<float> rg;
    get( ODGMT::sKeyDataRange, rg );
    const char* seqname = find( ODGMT::sKeyColSeq );
    if ( !seqname || !*seqname ) return false;

    ColTab::Sequence seq;
    if ( !ColTab::SM().get(seqname,seq) ) return false;

    bool doflip = false;
    getYN( ODGMT::sKeyFlipColTab, doflip );
    StreamData sd = StreamProvider(cptfnm).makeOStream();
    if ( !sd.usable() ) return false;

    *sd.ostrm << "#COLOR_MODEL = RGB" << std::endl;
    const int nrsteps = rg.nrSteps();
    for ( int idx=0; idx<=nrsteps; idx++ )
    {
	const float val = rg.start + rg.step * idx;
	const float frac = (float)idx / (float)nrsteps;
	const Color col = seq.color( doflip ? 1 - frac : frac );
	if ( idx )
	{
	    *sd.ostrm << val << "\t";
	    mPrintCol( col, std::endl );
	}

	if ( idx < nrsteps )
	{
	    *sd.ostrm << val << "\t";
	    mPrintCol( col, "\t" );
	}
    }

    const Color bgcol = seq.color( 0 );
    const Color fgcol = seq.color( 1 );
    *sd.ostrm << "B" << "\t";  mPrintCol( bgcol, std::endl );
    *sd.ostrm << "F" << "\t";  mPrintCol( fgcol, std::endl );
    sd.close();
    return true;
}

