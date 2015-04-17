/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2008
-*/
static const char* rcsID mUsedVar = "$Id$";

#include "segydirectdef.h"

#include "ascstream.h"
#include "datainterp.h"
#include "datachar.h"
#include "file.h"
#include "filepath.h"
#include "idxable.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "offsetazimuth.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "segyfiledata.h"
#include "segyscanner.h"
#include "seispsioprov.h"
#include "seisposindexer.h"
#include "strmoper.h"
#include "strmprov.h"
#include "od_iostream.h"
#include "survinfo.h"

namespace SEGY
{

const char* DirectDef::sKeyDirectDef()  { return "DirectSEG-Y"; }
const char* DirectDef::sKeyFileType()   { return "SEG-Y Direct Definition"; }
const char* DirectDef::sKeyNrFiles()	{ return "Number of files"; }
const char* DirectDef::sKeyFloatDataChar()  { return "Float datachar"; }
const char* DirectDef::sKeyInt32DataChar()  { return "Int32 datachar"; }
const char* DirectDef::sKeyInt64DataChar()  { return "Int64 datachar"; }

class PosKeyList : public Seis::PosKeyList
{
public:

PosKeyList()
{
    setFDS( 0 );
}

void setFDS( const FileDataSet* fds )
{
    fds_ = fds;
}

od_int64 size() const { return fds_->size(); }

bool key( od_int64 nr, Seis::PosKey& pk ) const
{
    bool usable;
    if ( !fds_ || !fds_->getDetails( nr, pk, usable ) )
	return false;

    if ( !usable )
    {
	pk = Seis::PosKey::undef();
	return true;
    }

    const Seis::GeomType geom = fds_->geomType();
    if ( !Seis::is2D( geom ) )
    {
	const BinID bid = pk.binID();
	if ( bid.inl()<=0 || bid.crl()<=0 )
	{
	    pk = Seis::PosKey::undef();
	}
    }
    else
    {
	const int trcnr = pk.trcNr();
	if ( trcnr<0 )
	{
	    pk = Seis::PosKey::undef();
	}
    }

    if ( Seis::isPS( geom ) && pk.offset()<0 )
    {
	pk = Seis::PosKey::undef();
    }

    return true;
}


FileDataSet::TrcIdx find( const Seis::PosKey& pk,
			  const Seis::PosIndexer& idxer,
			  bool chkoffs ) const
{
    const od_int64 nr = idxer.findFirst( pk, chkoffs );
    return fds_->getFileIndex( nr );
}


FileDataSet::TrcIdx findOcc( const Seis::PosKey& pk,
			     const Seis::PosIndexer& idxer,
			     int occ ) const
{
    const od_int64 nr = idxer.findOcc( pk, occ );
    return fds_->getFileIndex( nr );
}

    od_int64		totsz_;
    const FileDataSet*	fds_;
};

}


#define mDefMembInitList \
    : fds_(0) \
    , myfds_(0) \
    , keylist_( 0 ) \
    , cubedata_( *new PosInfo::CubeData ) \
    , linedata_( *new PosInfo::Line2DData ) \
    , indexer_( 0 ) \
    , outstream_( 0 )

SEGY::DirectDef::DirectDef()
    mDefMembInitList
{
}


SEGY::DirectDef::DirectDef( const char* fnm )
    mDefMembInitList
{
    readFromFile( fnm );
}


SEGY::DirectDef::~DirectDef()
{
    delete &cubedata_;
    delete &linedata_;
    delete myfds_;
    delete keylist_;
    delete indexer_;
    if ( outstream_ )
	delete outstream_;
}


bool SEGY::DirectDef::isEmpty() const
{
    return !fds_ || fds_->isEmpty();
}


void SEGY::DirectDef::setData( FileDataSet& fds )
{
    if ( &fds != myfds_ )
	delete myfds_;

    myfds_ = 0;

    fds_ = &fds;

    delete keylist_;
    delete indexer_;

    keylist_ = new SEGY::PosKeyList;
    keylist_->setFDS( fds_ );
    indexer_ = new Seis::PosIndexer( *keylist_, false, true );
    fds.setIndexer( indexer_ );
}


SEGY::FileDataSet::TrcIdx SEGY::DirectDef::find( const Seis::PosKey& pk,
						 bool chkoffs ) const
{
    if ( !keylist_ || !indexer_ )
    {
	SEGY::FileDataSet::TrcIdx res;
	return res;
    }

    return keylist_->find( pk, *indexer_, chkoffs );
}


SEGY::FileDataSet::TrcIdx SEGY::DirectDef::findOcc( const Seis::PosKey& pk,
						    int occ ) const
{
    if ( !keylist_ || !indexer_ )
    {
	SEGY::FileDataSet::TrcIdx res;
	return res;
    }

    return keylist_->findOcc( pk, *indexer_, occ );
}


#define mErrRet(s) { errmsg_ = s; return false; }
#define mGetInterp( str, type, interp ) \
    PtrMan<DataInterpreter<type> > interp = 0; \
    if ( iop1.get(str,dc ) ) \
    { \
	DataCharacteristics writtentype; \
	writtentype.set( dc.buf() ); \
	type dummy; \
	DataCharacteristics owntype(dummy); \
	if ( owntype!=writtentype ) \
	    interp = new DataInterpreter<type> ( writtentype ); \
    }

bool SEGY::DirectDef::readFromFile( const char* fnm )
{
    od_istream strm( fnm );
    if ( !strm.isOK() )
    {
	BufferString msg("Cannot open '", fnm, "':\n");
	msg.add(strm.errMsg().getFullString());
	mErrRet( msg )
    }

    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(sKeyFileType()) )
	mErrRet(BufferString("Input file '", fnm, "' has wrong file type"))

    IOPar iop1; iop1.getFrom( astrm );
    int version = 1;
    iop1.get( sKey::Version(), version );
    if ( version<1 || version>2 )
    {
	mErrRet(BufferString("Input file '", fnm,
			     "' is written by a later version of OpendTect"));
    }
    if ( version==1 )
    {
	delete myfds_;
	fds_ = myfds_ = new FileDataSet( iop1, astrm );

	keylist_ = new SEGY::PosKeyList();
	keylist_->setFDS( fds_ );

	indexer_ = new Seis::PosIndexer( *keylist_, true, true );
	getPosData( cubedata_ );
	getPosData( linedata_ );
    }
    else
    {
	const char* readerror = "Cannot read file";
	BufferString dc;

	PtrMan<DataInterpreter<float> > floatinterp =
	    DataInterpreter<float>::create(iop1,sKeyFloatDataChar(),false );
	PtrMan<DataInterpreter<od_int64> > int64interp =
	    DataInterpreter<od_int64>::create(iop1,sKeyInt64DataChar(),false );
	PtrMan<DataInterpreter<od_int32> > int32interp =
	    DataInterpreter<od_int32>::create(iop1,sKeyInt32DataChar(),false );

	IOPar segypars;
	segypars.getFrom( astrm );

	const od_stream::Pos datastart =
	    DataInterpreter<od_int64>::get(int64interp,strm);
	const od_stream::Pos textpars =
	    DataInterpreter<od_int64>::get(int64interp,strm);
	const od_stream::Pos cubedatastart =
	    DataInterpreter<od_int64>::get(int64interp,strm);
	const od_stream::Pos indexstart =
	    DataInterpreter<od_int64>::get(int64interp,strm);
	if ( !strm.isOK() )
	    mErrRet( readerror );

	strm.setPosition( textpars );
	ascistream astrm2( strm, false );

	IOPar iop2;
	iop2.getFrom( astrm2 );
	if ( !strm.isOK() )
	    mErrRet( readerror );

	FixedString int32typestr = iop1.find( sKeyInt32DataChar() );
	DataCharacteristics int32type;
	int32type.set( int32typestr );
	FileDataSet* fds = new FileDataSet(segypars,fnm,datastart,int32type);
	if ( !fds->usePar(iop2) )
	{
	    delete fds;
	    mErrRet( readerror );
	}

	const od_stream::Pos curpos = strm.position();
	if ( curpos!=cubedatastart )
	    strm.setPosition( cubedatastart );

	if ( !cubedata_.read(strm,false) || !linedata_.read(strm,false) )
	    { delete fds; mErrRet( readerror ); }

	delete keylist_;
	delete indexer_;

	delete myfds_;
	fds_ = myfds_ = fds;

	keylist_ = new SEGY::PosKeyList;
	keylist_->setFDS( fds_ );

	indexer_ = new Seis::PosIndexer( *keylist_, false, true );

	if ( !indexer_->readFrom( fnm, indexstart, false, int32interp,
				  int64interp, floatinterp ) )
	    mErrRet( readerror );
    }

    return true;
}

#undef mErrRet
#define mErrRet(s) { errmsg_ = s; return false; }

FixedString SEGY::DirectDef::fileName( int idx ) const
{
    if ( !fds_ )
	return sKey::EmptyString();

    return fds_->fileName( idx );
}


const IOPar* SEGY::DirectDef::segyPars() const
{
    return fds_ ? &fds_->segyPars() : 0;
}

#define mSetDc( par, type, string ) \
{ \
    type dummy; \
    DataCharacteristics(dummy).toString( dc ); \
}\
    par.set( string, dc )

#define mWriteOffset(var) strm.addBin( var )
#define mWriteOffsets \
    mWriteOffset(datastart_); mWriteOffset(textparstart_); \
    mWriteOffset(cubedatastart_); mWriteOffset(indexstart_)


bool SEGY::DirectDef::writeHeadersToFile( const char* fnm )
{
    delete outstream_;
    outstream_ = new od_ostream( fnm );
    if ( !outstream_->isOK() )
    {
	BufferString msg("Cannot open '", fnm, "' for write");
	outstream_->addErrMsgTo( msg );
	delete outstream_; outstream_ = 0;
	mErrRet( msg );
    }

    od_ostream& strm = *outstream_;
    ascostream astrm( strm );
    astrm.putHeader( sKeyFileType() );

    IOPar iop1;
    iop1.set( sKey::Version(), 2 );
    BufferString dc;
    mSetDc( iop1, od_int64, sKeyInt64DataChar() );
    mSetDc( iop1, od_int32, sKeyInt32DataChar() );
    mSetDc( iop1, float, sKeyFloatDataChar() );
    iop1.putTo( astrm );
    fds_->segyPars().putTo( astrm );

    offsetstart_ = strm.position();

    //Reserve space for offsets, which are written at the end
    datastart_ = textparstart_ = cubedatastart_ = indexstart_ = 0;

    mWriteOffsets;

    //Write the data
    datastart_ = strm.position();

    return strm.isOK();
}


bool SEGY::DirectDef::readFooter( const char* fnm, IOPar& pars,
				  od_stream_Pos& offset )
{
    od_istream istrm( fnm );
    if ( !istrm.isOK() )
	return false;

    ascistream astrm( istrm, true );
    if ( !astrm.isOfFileType(sKeyFileType()) )
	return false;

    IOPar iop1; iop1.getFrom( astrm );
    int version = 1;
    iop1.get( sKey::Version(), version );
    if ( version<=1 || version>2 )
	return false;

    IOPar segypars;
    segypars.getFrom( astrm );

    PtrMan<DataInterpreter<od_int64> > int64interp =
	DataInterpreter<od_int64>::create(iop1,sKeyInt64DataChar(),false );

    const od_stream::Pos datastart mUnusedVar =
	DataInterpreter<od_int64>::get(int64interp,istrm);
    offset = DataInterpreter<od_int64>::get(int64interp,istrm);

    istrm.setPosition( offset );
    ascistream astrm2( istrm, false );
    pars.getFrom( astrm2 );
    return istrm.isOK();
}


bool SEGY::DirectDef::updateFooter( const char* fnm, const IOPar& pars,
				    od_stream_Pos offset )
{
    od_ostream ostrm( fnm, true );
    if ( !ostrm.isOK() )
	return false;

    ostrm.setPosition( offset );
    ascostream astrm2( ostrm );
    pars.putTo( astrm2 );
    od_stream_Pos endpos = ostrm.endPosition();
    od_stream_Pos usedsize = ostrm.position();
    int nrcharstopadup = endpos - usedsize;
    for ( int idx=0; idx<nrcharstopadup-1; idx++ )
	ostrm.add( '#' );

    ostrm.add( '\n' );
    if ( !ostrm.isOK() )
	return false;

    ostrm.close();
    File::resize( fnm, usedsize );
    return true;
}


bool SEGY::DirectDef::writeFootersToFile()
{
    if ( !outstream_ )
	return false;
    od_ostream& strm = *outstream_;

    strm << "\n!\n"; //Just for nice formatting

    cubedatastart_ = strm.position();

    getPosData( cubedata_ );
    getPosData( linedata_ );

    cubedata_.write( strm, false );
    linedata_.write( strm, false );

    indexstart_ = strm.position();

    indexer_->dumpTo( strm );

    //Put this at the end, so one can manipulate the filenames without
    //breaking the indexing
    textparstart_ = strm.position();
    IOPar iop2;
    fds_->fillPar( iop2 );

    ascostream astrm2( strm );
    iop2.putTo( astrm2 );

    const od_stream::Pos eofpos = strm.position();
    strm.setPosition( offsetstart_ );
    mWriteOffsets;

    strm.setPosition( eofpos );
    const bool res = strm.isOK();
    delete outstream_;
    outstream_ = 0;

    return res;
}


void SEGY::DirectDef::getPosData( PosInfo::CubeData& cd ) const
{
    if ( !indexer_ || Seis::is2D(indexer_->geomType()) ) return;

    Interval<int> crlrg( indexer_->crlRange() ); crlrg.sort();
    const BinID step( SI().inlStep(), SI().crlStep() );

    PosInfo::CubeDataFiller cdf( cd );
    const TypeSet<int>& inlines = indexer_->getInls();
    TypeSet<int> crls;

    for ( int idx=0; idx<inlines.size(); idx++ )
    {
	const int inl = inlines[idx];
	crls.erase();

	indexer_->getCrls( inl, crls );
	for ( int idy=0; idy<crls.size(); idy++ )
	{
	    const BinID bid( inl, crls[idy] );
	    const FileDataSet::TrcIdx tidx = keylist_->find( Seis::PosKey(bid),
							    *indexer_, false );
	    if ( tidx.isValid() )
		cdf.add( bid );
	}
    }

    cdf.finish();
}


void SEGY::DirectDef::getPosData( PosInfo::Line2DData& ld ) const
{
    if ( !fds_ || !indexer_ || fds_->isEmpty() ||
	 !Seis::is2D(indexer_->geomType()) )
	return;

    Interval<int> nrrg( indexer_->trcNrRange() );
    nrrg.sort();
    for ( int nr=nrrg.start; nr<=nrrg.stop; nr++ )
    {
	const od_int64 tidx = indexer_->findFirst( Seis::PosKey(nr), false );
	if ( tidx<0 )
	    continue;

	PosInfo::Line2DPos l2dpos( nr );
	l2dpos.coord_ = fds_->get2DCoord( nr );
	ld.add( l2dpos );
    }

    ld.setZRange( fds_->getSampling().interval(fds_->getTrcSz()) );
}


const char* SEGY::DirectDef::get2DFileName( const char* dirnm,
					    Pos::GeomID geomid )
{
    mDeclStaticString( ret );
    FilePath fp( dirnm );
    BufferString nm( fp.fileName(), "^", toString(geomid) );
    fp.add( nm );
    fp.setExtension( "sgydef" );
    ret = fp.fullPath();
    return ret.buf();
}


const char* SEGY::DirectDef::get2DFileName( const char* dirnm, const char* unm )
{
    Pos::GeomID geomid = Survey::GM().getGeomID( unm );
    return geomid == Survey::GM().cUndefGeomID() ? 0
				: get2DFileName( dirnm, geomid );
}


SEGY::FileIndexer::FileIndexer( const MultiID& mid, bool isvol,
					const FileSpec& sgyfile, bool is2d,
				        const IOPar& segypar )
    : Executor( "SEGY Indexer" )
    , directdef_( 0 )
    , ioobj_( IOM().get( mid ) )
    , isvol_(isvol)
    , is2d_(is2d)
    , geomid_(mUdfGeomID)
    , scanner_(0)
{
    if ( !ioobj_ )
    { msg_ = tr("Cannot find output object"); return; }
    if ( is2d && !segypar.get(sKey::GeomID(),geomid_) )
    {
	const FixedString linename = segypar.find( sKey::LineName() );
	if ( linename )
	    geomid_ = Survey::GM().getGeomID( linename );
    }

    if ( is2d && geomid_ == mUdfGeomID )
    {
	delete ioobj_; ioobj_ = 0; msg_ = tr("2D Line ID not specified");
	return;
    }

    scanner_ = new SEGY::Scanner( sgyfile,
			is2d_ ? (isvol_ ? Seis::Line : Seis::LinePS) :
				(isvol_ ? Seis::Vol : Seis::VolPS), segypar );
}

SEGY::FileIndexer::~FileIndexer()
{
    delete ioobj_;
    delete directdef_;
    delete scanner_;
}


#undef mErrRet
#define mErrRet( s1, s2 ) \
{ \
    msg_ = s1; \
    msg_ += " "; \
    msg_ += s2; \
    return ErrorOccurred(); \
}


int SEGY::FileIndexer::nextStep()
{
    if ( !ioobj_ ) return ErrorOccurred();

    if ( !directdef_ )
    {
	BufferString outfile = ioobj_->fullUserExpr( false );
	if ( outfile.isEmpty() )
	{ msg_ = tr("Output filename empty"); return ErrorOccurred(); }

	if ( is2d_ )
	{
	    if ( !File::isDirectory(outfile) )
	    {
		File::createDir(outfile);
		if ( !File::isDirectory(outfile) )
		{
		    msg_ = tr("Cannot create directory for output:\n%1")
			      .arg(outfile);
		    return ErrorOccurred();
		}
	    }
	    if ( !File::isWritable(outfile) )
	    {
		msg_ = tr( "Output directory is not writable:\n%1")
			   .arg(outfile);
		return ErrorOccurred();
	    }

	    outfile = SEGY::DirectDef::get2DFileName( outfile, geomid_ );
	}

	if ( File::exists(outfile) && !File::isWritable(outfile) )
	{
	    msg_ = tr("Cannot overwrite output file:\n%1").arg(outfile);
	    return ErrorOccurred();
	}

	msg_ = tr("Setting up output indexing");
	directdef_ = new SEGY::DirectDef;
	directdef_->setData( scanner_->fileDataSet() );
	if ( is2d_ )
	    scanner_->fileDataSet().save2DCoords( true );

	if ( !directdef_->writeHeadersToFile( outfile ) )
	{
	    msg_ = tr( "Cannot write to file %1" ).arg( outfile );
	    return ErrorOccurred();
	}

	scanner_->fileDataSet().setOutputStream(*directdef_->getOutputStream());
	return MoreToDo();
    }

    msg_ = scanner_->uiMessage();
    const int res = scanner_->nextStep();
    if ( res == ErrorOccurred() )
	msg_ = scanner_->uiMessage();
    else if ( res==Finished() )
    {
	const SEGY::FileDataSet& fds = scanner_->fileDataSet();
	if ( !fds.nrFiles() )
	{
	    IOM().permRemove( ioobj_->key() );
	    msg_ = tr("No files scanned");
	    return ErrorOccurred();
	}

	if ( fds.isEmpty() )
	{
	    IOM().permRemove( ioobj_->key() );
	    msg_ = fds.nrFiles() > 1
		? tr("No traces found in any of the files")
		: tr("No traces found in file");

	    return ErrorOccurred();
	}

	if ( !directdef_->writeFootersToFile() )
	{
	    msg_ = tr("Cannot complete file output");
	    return ErrorOccurred();
	}

	if ( !is2d_ && !isvol_ )
	    SPSIOPF().mk3DPostStackProxy( *ioobj_ );
    }

    return res;
}


uiString SEGY::FileIndexer::uiMessage() const
{ return msg_; }


od_int64 SEGY::FileIndexer::nrDone() const
{ return scanner_ ? scanner_->nrDone() : 0; }


od_int64 SEGY::FileIndexer::totalNr() const
{ return scanner_ ? scanner_->totalNr() : 0; }


uiString SEGY::FileIndexer::uiNrDoneText() const
{ return tr("Traces scanned"); }
