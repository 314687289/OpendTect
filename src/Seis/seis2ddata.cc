/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Salil Agarwal
 * DATE     : Dec 2012
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "seis2ddata.h"

#include "seis2dlineio.h"
#include "seisbuf.h"
#include "survgeom2d.h"
#include "linesetposinfo.h"
#include "safefileio.h"
#include "ascstream.h"
#include "survinfo.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "od_ostream.h"



Seis2DDataSet::Seis2DDataSet( const IOObj& ioobj )
    : NamedObject("")
{
    init( ioobj.fullUserExpr(true) );
}


Seis2DDataSet::~Seis2DDataSet()
{
    deepErase( pars_ );
}


Seis2DDataSet& Seis2DDataSet::operator =( const Seis2DDataSet& dset )
{
    if ( &dset == this ) return *this;
    fname_ = dset.fname_;
    readFile( false );
    return *this;
}


void Seis2DDataSet::init( const char* fnm )
{
    readonly_ = false;
    fname_ = fnm;
    BufferString typestr = "CBVS";
    readFile( false, &typestr );

    liop_ = 0;
    const ObjectSet<Seis2DLineIOProvider>& liops = S2DLIOPs();
    for ( int idx=0; idx<liops.size(); idx++ )
    {
	const Seis2DLineIOProvider* liop = liops[idx];
	if ( typestr == liop->type() )
	    { liop_ = const_cast<Seis2DLineIOProvider*>(liop); break; }
    }
}


const char* Seis2DDataSet::type() const
{
    return liop_ ? liop_->type() : "CBVS";
}


const char* Seis2DDataSet::lineName( int idx ) const
{
    return pars_.validIdx(idx) ? Survey::GM().getName( geomID(idx) ) : "";
}


Pos::GeomID Seis2DDataSet::geomID( int idx ) const
{
    if ( !pars_.validIdx(idx) )
	return -1;

    int ret = -1;
    pars_[idx]->get( sKey::GeomID(), ret );
    return ret;
}


int Seis2DDataSet::indexOf( const char* linename ) const
{
    const int geomid = Survey::GM().getGeomID( linename );
    return indexOf( geomid );
}


int Seis2DDataSet::indexOf( Pos::GeomID geomid ) const
{
    for ( int idx=0; idx<pars_.size(); idx++ )
	if ( geomID(idx) == geomid )
	    return idx;

    return -1;
}


static const char* sKeyFileType = "2D Data Set";


void Seis2DDataSet::readFile( bool mklock, BufferString* typestr )
{
    deepErase( pars_ );

    SafeFileIO sfio( fname_, true );
    if ( !sfio.open(true) )
    {
	if ( name().isEmpty() )
	{
	    FilePath fp( fname_ );
	    fp.setExtension( 0 );
	    setName( fp.fileName() );
	}
	return;
    }

    getFrom( sfio.istrm(), typestr );
    sfio.closeSuccess( mklock );
}


void Seis2DDataSet::getFrom( od_istream& strm, BufferString* typestr )
{
    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(sKeyFileType) )
	return;
    //TODO: review and add Nr of lines info.
    if ( !atEndOfSection(astrm.next()) )
    {
	if ( astrm.hasKeyword(sKey::Name()) )
	    setName( astrm.value() );
	if ( astrm.hasKeyword(sKey::Type()) && typestr )
	    *typestr = astrm.value();
    }

    while ( astrm.type() != ascistream::EndOfFile )
    {
	IOPar* newpar = new IOPar;
	while ( !atEndOfSection(astrm.next()) )
	{
	    if ( astrm.hasKeyword(sKey::GeomID()) )
		newpar->set( sKey::GeomID(), astrm.value() );
	    else if ( !astrm.hasValue("") )
		newpar->set( astrm.keyWord(), astrm.value() );
	}
	if ( newpar->size() < 1 )
	    delete newpar;
	else
	    pars_ += newpar;
    }

    return;
}


void Seis2DDataSet::writeFile() const
{
    if ( !readonly_ )
    {
	SafeFileIO sfio( fname_, true );
	if ( sfio.open(false,true) )
	{
	    putTo( sfio.ostrm() );
	    sfio.closeSuccess();
	}
    }
}


void Seis2DDataSet::putTo( od_ostream& strm ) const
{
    ascostream astrm( strm );
    if ( !astrm.putHeader(sKeyFileType) )
	return;

    astrm.put( sKeyNoOfLines, pars_.size() );
    astrm.newParagraph();

    for ( int ipar=0; ipar<pars_.size(); ipar++ )
    {
	IOPar iopar = *pars_[ipar];
	iopar.setName("");
	iopar.putTo( astrm );
    }
}


bool Seis2DDataSet::getGeometry( int ipar, PosInfo::Line2DData& geom ) const
{
    if ( !liop_ )
    {
	ErrMsg("No suitable 2D line information object found");
	return 0;
    }

    if ( ipar < 0 || ipar >= pars_.size() )
    {
	ErrMsg("Line number requested not found in Line Set");
	return 0;
    }

    return liop_->getGeometry( *pars_[ipar], geom );
}


Executor* Seis2DDataSet::lineFetcher( int ipar, SeisTrcBuf& tbuf, int ntps,
				      const Seis::SelData* sd ) const
{
    if ( !liop_ )
    {
	ErrMsg("No suitable 2D line extraction object found");
	return 0;
    }

    if ( ipar < 0 || ipar >= pars_.size() )
    {
	ErrMsg("Line number requested not found in Line Set");
	return 0;
    }

    return liop_->getFetcher( *pars_[ipar], tbuf, ntps, sd );
}


Seis2DLinePutter* Seis2DDataSet::linePutter( IOPar* newiop )
{
    if ( !newiop )
    {
	ErrMsg("No data for line add provided");
	return 0;
    }
    else if ( !liop_ )
    {
	ErrMsg("No suitable 2D line creation object found");
	return 0;
    }

    int newgeomid;
    if ( !newiop->get(sKey::GeomID(),newgeomid) )
	return 0; // TODO: add error message

    readFile( true );

    Seis2DLinePutter* res = 0;
    int paridx = indexOf( newgeomid );
    if ( paridx >= 0 )
    {
	pars_[paridx]->merge( *newiop );
	*newiop = *pars_[paridx];
	delete pars_.replace( paridx, newiop );
	res = liop_->getReplacer( *pars_[paridx] );
    }
    else if ( !readonly_ )
    {
	const IOPar* previop = pars_.size() ? pars_[pars_.size()-1] : 0;
	pars_ += newiop;
	res = liop_->getAdder( *newiop, previop, name() );
    }
    else
    {
	BufferString msg( "Read-only Data set chg req: " );
	msg += Survey::GM().getName(newgeomid); msg += " not yet in set ";
	msg += name();
	pErrMsg( msg );
    }

    if ( res )
	writeFile();

    // Phew! Made it.
    return res;
}


bool Seis2DDataSet::isEmpty( int ipar ) const
{
    return liop_ ? liop_->isEmpty( *pars_[ipar] ) : true;
}


bool Seis2DDataSet::remove( Pos::GeomID geomid )
{
    if ( readonly_ ) return false;

    readFile( true );
    int ipar = indexOf( geomid );
    if ( ipar < 0 )
	{ return true; }

    IOPar* iop = pars_[ipar];
    if ( liop_ )
	liop_->removeImpl(*iop);
    pars_ -= iop;
    delete iop;

    writeFile();
    return true;
}


bool Seis2DDataSet::renameFiles( const char* newnm )
{
    BufferString cleannm( newnm );
    cleannm.clean();
    if ( fname_.isEmpty() )
	return false;

    setName( cleannm.buf() );
    BufferString oldnm;
    if ( !pars_.size() )
	return false;

    pars_[0]->get( sKey::FileName(), oldnm );
    int index = 0;
    while ( true )
    {
	if ( oldnm[index] == '^' || oldnm[index] == '.' )
	    break;
	index++;
    }

    oldnm[index] = '\0';

    FilePath fp( fname_ );
    for ( int idx=0; idx<nrLines(); idx++ )
    {
	BufferString filenm, oldfilenm;
	pars_[idx]->get( sKey::FileName(), filenm );
	oldfilenm = filenm;
	filenm.replace( oldnm.buf(), cleannm.buf() );
	FilePath newfp( fp.pathOnly(), filenm );
	FilePath oldfp( fp.pathOnly(), oldfilenm );
	if ( oldfp.isEmpty() || newfp.isEmpty() || oldfp == newfp )
	    continue;

	if ( !File::rename(oldfp.fullPath(),newfp.fullPath()) )
	{
	    renameFiles( oldnm );
	    return false;
	}

	newfp.setExtension( "par" );
	oldfp.setExtension( "par" );
	File::rename( oldfp.fullPath(), newfp.fullPath() );

	pars_[idx]->set( sKey::FileName(), filenm );
    }

    writeFile();
    return true;
}


bool Seis2DDataSet::getTxtInfo( int ipar, BufferString& uinf,
				  BufferString& stdinf ) const
{
    return liop_ ? liop_->getTxtInfo(*pars_[ipar],uinf,stdinf) : false;
}


bool Seis2DDataSet::getRanges( int ipar, StepInterval<int>& sii,
				 StepInterval<float>& sif ) const
{
    return liop_ ? liop_->getRanges(*pars_[ipar],sii,sif) : false;
}


const char* Seis2DDataSet::getCubeSampling( CubeSampling& cs, int lnr ) const
{
    cs.hrg.step.inl() = cs.hrg.step.crl() = 1;
    cs.hrg.start.inl() = 0; cs.hrg.stop.inl() = nrLines()-1;
    cs.hrg.start.crl() = 0; cs.hrg.stop.crl() = mUdf(int);
    cs.zrg = SI().zRange(false);
    const int nrlines = nrLines();
    if ( nrlines < 1 )
	return "No lines in Data Set";

    bool havelinesel = lnr >= 0;
    if ( !havelinesel )
	lnr = 0;
    else
	cs.hrg.start.inl() = cs.hrg.stop.inl() = lnr;

    StepInterval<int> trg; StepInterval<float> zrg;
    bool foundone = false;

    if ( getRanges(lnr,trg,zrg) )
	foundone = true;

    if ( !havelinesel )
    {
	StepInterval<int> newtrg; StepInterval<float> newzrg;
	for ( int iln=1; iln<nrlines; iln++ )
	{
	    if ( getRanges(iln,newtrg,newzrg) )
	    {
		foundone = true;
		if ( newtrg.start < trg.start ) trg.start = newtrg.start;
		if ( newtrg.stop > trg.stop ) trg.stop = newtrg.stop;
		if ( newtrg.step < trg.step ) trg.step = newtrg.step;
		if ( newzrg.start < zrg.start ) zrg.start = newzrg.start;
		if ( newzrg.stop > zrg.stop ) zrg.stop = newzrg.stop;
		if ( newzrg.step < zrg.step ) zrg.step = newzrg.step;
	    }
	}
    }

    if ( !foundone )
	return "No range info present";

    cs.hrg.start.crl() = trg.start; cs.hrg.stop.crl() = trg.stop;
    cs.hrg.step.crl() = trg.step;
    cs.zrg = zrg;
    return 0;
}


class Seis2DGeomDumper : public Executor
{
public:

Seis2DGeomDumper( const Seis2DDataSet& l, od_ostream& o, bool inr, float z,
		  const char* lk )
	: Executor("Geometry extraction")
	, ds(l)
	, strm(o)
	, donr(inr)
	, curidx(-1)
	, totalnr(-1)
	, lnshandled(0)
	, ptswritten(0)
	, zval(z)
	, incz(!mIsUdf(z))
	, dolnm(true)
{
    lastidx = ds.nrLines() - 1;
    if ( lastidx < 0 )
    {
	curmsg = "No lines in data set";
	return;
    }

    curidx = 0;
    if ( lk && *lk )
    {
	curidx = ds.indexOf( lk );
	if ( curidx < 0 )
	{
	    curmsg = "Could not find the line  '";
	    curmsg += lk; curmsg += "' in data set";
	    return;
	}

	lastidx = curidx;
	dolnm = false;
    }

    totalnr = lastidx - curidx + 1;
    curmsg = "Extracting geometry";
}

const char* message() const	{ return curmsg.buf(); }
const char* nrDoneText() const	{ return "Lines handled"; }
od_int64 nrDone() const		{ return lnshandled; }
od_int64 totalNr() const	{ return totalnr; }


int nextStep()
{
    if ( curidx < 0 )
	return ErrorOccurred();
    if ( !strm.isOK() )
    {
	curmsg = "Cannot write to file";
	return ErrorOccurred();
    }

    if ( curidx > lastidx )
    {
	if ( ptswritten == 0 )
	{
	    curmsg = "No output created";
	    return ErrorOccurred();
	}
	return Finished();
    }

    Survey::Geometry2D* geom2d = (Survey::Geometry2D*)Survey::GM().getGeometry
							( ds.geomID(curidx) );
    /*S2DPOS().setCurLineSet( ds.name() );
    PosInfo::Line2DData geom( ds.lineKey(curidx).lineName() );*/
    if ( geom2d->data().isEmpty()/*!S2DPOS().getGeometry(geom)*/ )
    {
	curmsg = "Couldn't get geometry for '";
	curmsg += ds.lineName( curidx );
	curmsg += "'";
	return totalnr == 1		? ErrorOccurred()
	     : (curidx == lastidx	? Finished()
					: WarningAvailable());
    }

    BufferString outstr;
    const float zfac = mCast( float, SI().zDomain().userFactor() );
    const TypeSet<PosInfo::Line2DPos>& posns = geom2d->data().positions();
    for ( int idx=0; idx<posns.size(); idx++ )
    {
	const PosInfo::Line2DPos& pos = posns[idx];
	outstr = "";
	if ( dolnm )
	    { outstr += ds.lineName(curidx); outstr += "\t"; }
	if ( donr )
	    { outstr += pos.nr_; outstr += "\t"; }
	outstr += pos.coord_.x; outstr += "\t";
	outstr += pos.coord_.y;
	if ( incz )
	    { outstr += "\t"; outstr += zval * zfac; }
	strm << outstr << '\n';
	ptswritten++;
    }
    strm.flush();

    if ( !strm.isOK() )
    {
	curmsg = "Error during write to file";
	return ErrorOccurred();
    }

    lnshandled++;
    curidx++;
    return curidx > lastidx ? Finished() : MoreToDo();
}

    const Seis2DDataSet&	ds;
    od_ostream&			strm;
    BufferString		attrnm;
    const bool			incz;
    const float			zval;
    bool			donr;
    bool			dolnm;
    int				curidx;
    int				lastidx;
    int				totalnr;
    int				lnshandled;
    int				ptswritten;
    BufferString		curmsg;

};


Executor* Seis2DDataSet::geometryDumper( od_ostream& strm, bool incnr,
					 float z, const char* lk ) const
{
    return new Seis2DGeomDumper( *this, strm, incnr, z, lk );
}
