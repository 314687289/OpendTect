/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R. K. Singh
 * DATE     : March 2008
-*/

static const char* rcsID = "$Id: madstream.cc,v 1.22 2009-06-18 11:51:30 cvsnanne Exp $";

#include "madstream.h"
#include "cubesampling.h"
#include "envvars.h"
#include "filegen.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "posinfo.h"
#include "ptrman.h"
#include "seis2dline.h"
#include "seisioobjinfo.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "seispswrite.h"
#include "seisbuf.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seispacketinfo.h"
#include "seiswrite.h"
#include "strmprov.h"
#include "survinfo.h"


using namespace ODMad;

static const char* sKeyRSFEndOfHeader = "\014\014\004";
static const char* sKeyMadagascar = "Madagascar";
static const char* sKeyInput = "Input";
static const char* sKeyOutput = "Output";
static const char* sKeyProc = "Proc";
static const char* sKeyWrite = "Write";
static const char* sKeyIn = "in";
static const char* sKeyStdIn = "stdin";
static const char* sKeyPosFileName = "Pos File Name";
static const char* sKeyScons = "Scons";

#undef mErrRet
#define mErrRet(s) { errmsg_ = s; return; }
#define mErrBoolRet(s) { errmsg_ = s; return false; }

#ifdef __win__
    #define mReadRSFTrc(arr) \
        for ( int idx=0; idx<nrsamps; idx++ ) \
	    *istrm_ >> arr[idx];
#else
    #define mReadRSFTrc(arr) \
        istrm_->read( (char*)arr, nrsamps*sizeof(float) );
#endif

MadStream::MadStream( IOPar& par )
    : pars_(par)
    , is2d_(false),isps_(false)
    , istrm_(0),ostrm_(0)
    , seisrdr_(0),seiswrr_(0)
    , psrdr_(0),pswrr_(0)
    , trcbuf_(0),curtrcidx_(-1)
    , stortrcbuf_(0)
    , stortrcbufismine_(true)
    , iter_(0),l2ddata_(0)
    , headerpars_(0)
    , errmsg_(*new BufferString(""))
    , iswrite_(false)
{
    par.getYN( sKeyWrite, iswrite_ );
    if ( iswrite_ )
    {
	PtrMan<IOPar> outpar = par.subselect( sKeyOutput );
	if ( !outpar ) mErrRet( "Output parameters missing" );

	initWrite( outpar );
    }
    else
    {
	PtrMan<IOPar> inpar = par.subselect( sKeyInput );
	if ( !inpar ) mErrRet( "Input parameters missing" );

	initRead( inpar );
    }
}


MadStream::~MadStream()
{
    if ( istrm_ && istrm_ != &std::cin ) delete istrm_;
    if ( ostrm_ )
    {
	ostrm_->flush();
	if ( ostrm_ != &std::cout && ostrm_ != &std::cerr ) delete ostrm_;
    }

    delete seisrdr_; delete seiswrr_;
    delete psrdr_; delete pswrr_;
    delete trcbuf_; delete iter_; delete l2ddata_;
    delete errmsg_;
    if ( headerpars_ ) delete headerpars_;
    if ( stortrcbuf_ && stortrcbufismine_ ) delete  stortrcbuf_;
}


static bool getScriptForScons( BufferString& str )
{
    FilePath fp( str.buf() );
    if ( fp.fileName() != "SConstruct" )
	return false;

    const char* rsfroot = GetEnvVar("RSFROOT");
    FilePath sconsfp(rsfroot);
    sconsfp.add( "bin" ).add( "scons" );
    if ( !File_exists(sconsfp.fullPath()) )
	return false;

#ifdef __win__
    BufferString scriptfile = FilePath::getTempName( "bat" );
    StreamData sd = StreamProvider(scriptfile).makeOStream();
    *sd.ostrm << "@echo off" << std::endl;
    *sd.ostrm << "Pushd " << fp.pathOnly() << std::endl;
    *sd.ostrm << sconsfp.fullPath() << std::endl;
    *sd.ostrm << "Popd" << std::endl;
#else
    BufferString scriptfile = FilePath::getTempName();
    StreamData sd = StreamProvider(scriptfile).makeOStream();
    *sd.ostrm << "#!/bin/csh -f" << std::endl;
    *sd.ostrm << "pushd " << fp.pathOnly() << std::endl;
    *sd.ostrm << sconsfp.fullPath() << std::endl;
    *sd.ostrm << "popd" << std::endl;
#endif
    sd.close();
    File_setPermissions( scriptfile, "744", 0 );

    str = "@";
    str += scriptfile;
    return true;
}


void MadStream::initRead( IOPar* par )
{
    BufferString inptyp( par->find(sKey::Type) );
    if ( inptyp == "None" || inptyp == "SU" )
	return;

    if ( inptyp == "Madagascar" )
    {
	const char* filenm = par->find( sKey::FileName ).buf();
	if ( !filenm || !*filenm )
	    mErrRet("No entry for 'Input file' in parameter file")

	BufferString inpstr( filenm );
	bool scons = false;
	par->getYN( sKeyScons, scons );
	if ( scons && !getScriptForScons(inpstr) )
	    return;
#ifdef __win__
	FilePath fp( GetEnvVar("RSFROOT") );
	if ( scons )
	    inpstr += " | ";
	else
	    inpstr = "@";
	inpstr += fp.add("bin").add("sfdd").fullPath();
	inpstr += " < \""; inpstr += filenm;
	inpstr += "\" form=ascii_float out=stdout";
#endif
	istrm_ = StreamProvider(inpstr).makeIStream().istrm;

	fillHeaderParsFromStream();
	if ( !headerpars_ ) mErrRet( "Error reading RSF header" );;

	BufferString insrc( headerpars_->find(sKeyIn) );
	if ( insrc == "" || insrc == sKeyStdIn ) return;

	StreamData sd = StreamProvider(insrc).makeIStream();
	if ( !sd.usable() ) mErrRet( "Cannot read RSF data file" );;

	if ( istrm_ && istrm_ != &std::cin ) delete istrm_;

	istrm_ = sd.istrm;
	headerpars_->set( sKeyIn, sKeyStdIn );
	return;
    }

    Seis::GeomType gt = Seis::geomTypeOf( inptyp );
    is2d_ = gt == Seis::Line || gt == Seis::LinePS;
    isps_ = gt == Seis::VolPS || gt == Seis::LinePS;
    MultiID inpid;
    if ( !par->get(sKey::ID,inpid) ) mErrRet( "Input ID missing" );

    PtrMan<IOObj> ioobj = IOM().get( inpid );
    if ( !ioobj ) mErrRet( "Cannot find input data" );

    PtrMan<IOPar> subpar = par->subselect( sKey::Subsel );
    Seis::SelData* seldata = Seis::SelData::get( *subpar );
    if ( !isps_ )
    {
	seisrdr_ = new SeisTrcReader( ioobj );
	seisrdr_->setSelData( seldata );
	seisrdr_->prepareWork();
	fillHeaderParsFromSeis();
    }
    else
    {
	const LineKey lk = seldata->lineKey();
	if ( is2d_ )
	    psrdr_ = SPSIOPF().get2DReader( *ioobj, lk.lineName() );
	else
	    psrdr_ = SPSIOPF().get3DReader( *ioobj );

	if ( !psrdr_ ) mErrRet( "Cannot read input data" );

	fillHeaderParsFromPS( seldata );
    }
}
 

void MadStream::initWrite( IOPar* par )
{
    BufferString outptyp( par->find(sKey::Type) );
    Seis::GeomType gt = Seis::geomTypeOf( outptyp );

    is2d_ = gt == Seis::Line || gt == Seis::LinePS;
    isps_ = gt == Seis::VolPS || gt == Seis::LinePS;
    istrm_ = &std::cin;
    MultiID outpid;
    if ( !par->get(sKey::ID,outpid) ) mErrRet( "Output data ID missing" );

    PtrMan<IOObj> ioobj = IOM().get( outpid );
    if ( !ioobj ) mErrRet( "Cannot find output object" );

    PtrMan<IOPar> subpar = par->subselect( sKey::Subsel );
    Seis::SelData* seldata = subpar ? Seis::SelData::get(*subpar) : 0;
    if ( !isps_ )
    {
	seiswrr_ = new SeisTrcWriter( ioobj );
	if ( !seiswrr_ ) mErrRet( "Cannot write to output object" );
    }
    else
    {
	const LineKey lk = seldata ? seldata->lineKey() : 0;
	pswrr_ = is2d_ ? SPSIOPF().get2DWriter(*ioobj,lk.lineName())
	    	       : SPSIOPF().get3DWriter(*ioobj);
	if ( !pswrr_ ) mErrRet( "Cannot write to output object" );
    }
    
    if ( is2d_ && !isps_ )
    {
	const char* attrnm = par->find( sKey::Attribute ).buf();
	if ( attrnm && *attrnm && seldata )
	    seldata->lineKey().setAttrName( attrnm );

	seiswrr_->setSelData(seldata);
    }	

    fillHeaderParsFromStream();
}


BufferString MadStream::getPosFileName( bool forread ) const
{
    BufferString posfnm;
    if ( forread )
    {
	posfnm = pars_.find( sKeyPosFileName ).buf();
	if ( !posfnm.isEmpty() && File_exists(posfnm) )
	    return posfnm;
	else posfnm.setEmpty();
    }

    BufferString typ = 
	pars_.find( IOPar::compKey( forread ? sKeyInput : sKeyOutput,
		    		    sKey::Type) ).buf();
    if ( typ == sKeyMadagascar )
    {
	BufferString outfnm =
	    pars_.find( IOPar::compKey( forread ? sKeyInput : sKeyOutput,
		    			sKey::FileName) ).buf();
	FilePath fp( outfnm );
	fp.setExtension( "pos" );
	if ( !forread || File_exists(fp.fullPath()) )
	    posfnm = fp.fullPath();
    }
    else if ( !forread )
	posfnm = FilePath::getTempName( "pos" );

    return posfnm;
}


#define mWriteToPosFile( obj ) \
    StreamData sd = StreamProvider( posfnm ).makeOStream(); \
    if ( !sd.usable() ) mErrRet( "Cannot create Pos file" ); \
    if ( !obj.write(*sd.ostrm,false) ) \
    { sd.close(); mErrRet( "Cannot write to Pos file" ); } \
    sd.close(); \
    pars_.set( sKeyPosFileName, posfnm );


void MadStream::fillHeaderParsFromSeis()
{
    if ( !seisrdr_ ) mErrRet( "Cannot read input data" );

    if ( headerpars_ ) delete headerpars_; headerpars_ = 0;

    headerpars_ = new IOPar;
    bool needposfile = true;
    StepInterval<float> zrg;
    StepInterval<int> trcrg;
    int nrtrcs = 0;
    BufferString posfnm = getPosFileName( false );
    if ( is2d_ )
    {
	const IOObj* ioobj = seisrdr_->ioObj();
	if ( !ioobj ) mErrRet( "No nput object" );

	Seis2DLineSet lset( *ioobj );
	const Seis::SelData* seldata = seisrdr_->selData();
	if ( !seldata ) mErrRet( "Invalid data subselection" );

	const int lidx = lset.indexOf( seldata->lineKey() );
	if ( lidx < 0 ) mErrRet( "2D Line not found" );

	PosInfo::Line2DData geom;
	if ( !lset.getGeometry(lidx,geom) )
	    mErrRet( "Line geometry not available" );

	if ( !seldata->isAll() )
	{
	    geom.limitTo( seldata->crlRange() );
	    geom.zrg_.limitTo( seldata->zRange() );
	}

	nrtrcs = geom.posns_.size();
	zrg = geom.zrg_;
	mWriteToPosFile( geom )
    }
    else
    {
	SeisPacketInfo& pinfo = seisrdr_->seisTranslator()->packetInfo();
	zrg = pinfo.zrg;
	trcrg = pinfo.crlrg;
	
	mDynamicCastGet(const Seis::RangeSelData*,rangesel,seisrdr_->selData())
	if ( rangesel && !rangesel->isAll() )
	{
	    zrg.limitTo( rangesel->zRange() );
	    trcrg.limitTo( rangesel->crlRange() );
	}

	if ( pinfo.fullyrectandreg )
	    needposfile = false;
	else
	{
	    if ( !pinfo.cubedata ) mErrRet( "Incomplete Geometry Information" );

	    PosInfo::CubeData newcd( *pinfo.cubedata );
	    if ( rangesel && !rangesel->isAll() )
		newcd.limitTo( rangesel->cubeSampling().hrg );

	    needposfile = !newcd.isFullyRectAndReg();
	    if ( needposfile )
	    {
		nrtrcs = newcd.totalSize();
		mWriteToPosFile( newcd )
	    }
	}
	
	if ( !needposfile )
	{
	    StepInterval<int> inlrg = rangesel ?
			rangesel->cubeSampling().hrg.inlRange() : pinfo.inlrg;
	    headerpars_->set( "o3", inlrg.start );
	    headerpars_->set( "n3", inlrg.nrSteps()+1 );
	    headerpars_->set( "d3", inlrg.step );
	    if ( File_exists(posfnm) )
		StreamProvider(posfnm).remove(); // While overwriting rsf
	}
    }

    if ( nrtrcs )
	headerpars_->set( "n2", nrtrcs );
    else
    {
	headerpars_->set( "o2", trcrg.start );
	headerpars_->set( "n2", trcrg.nrSteps()+1 );
	headerpars_->set( "d2", trcrg.step );
    }

    headerpars_->set( "o1", zrg.start );
    headerpars_->set( "n1", zrg.nrSteps()+1 );
    headerpars_->set( "d1", zrg.step );
}


void MadStream::fillHeaderParsFromPS( const Seis::SelData* seldata )
{
    if ( !psrdr_ ) mErrRet( "Cannot read input data" );

    if ( headerpars_ ) delete headerpars_; headerpars_ = 0;

    headerpars_ = new IOPar;
    bool needposfile = true;
    StepInterval<float> zrg;
    int nrbids = 0;
    BufferString posfnm = getPosFileName( false );
    BinID firstbid;
    SeisTrcBuf trcbuf( true );
    if ( is2d_ )
    {
	mDynamicCastGet(SeisPS2DReader*,rdr,psrdr_);
	l2ddata_ = new PosInfo::Line2DData( rdr->posData() );
	if ( seldata && !seldata->isAll() )
	{
	    l2ddata_->limitTo( seldata->crlRange() );
	    l2ddata_->zrg_.limitTo( seldata->zRange() );
	}

	nrbids = l2ddata_->posns_.size();
	if ( !nrbids ) mErrRet( "No data available in the given range" );

	mWriteToPosFile( (*l2ddata_) );
	firstbid.crl = l2ddata_->posns_[0].nr_;

	if ( !rdr->getGather(firstbid,trcbuf) )
	    mErrRet( "No data to read" );
    }
    else
    {
	mDynamicCastGet(SeisPS3DReader*,rdr,psrdr_);
	PosInfo::CubeData newcd( rdr->posData() );
	if ( seldata )
	{
	    HorSampling hs;
	    hs.set( seldata->inlRange(), seldata->crlRange() );
	    newcd.limitTo( hs );
	}

	nrbids = newcd.totalSize();
	if ( !nrbids ) mErrRet( "No data available in the given range" );
	mWriteToPosFile( newcd );

	int idx=0;
	while (idx<newcd.size() && newcd[idx] && !newcd[idx]->segments_.size())
	    idx++;

	iter_ = new PosInfo::CubeDataIterator( newcd );
	firstbid.inl = newcd[idx]->linenr_;
	firstbid.crl = newcd[idx]->segments_[0].start;

	if ( !rdr->getGather(firstbid,trcbuf) )
	    mErrRet( "No data to read" );
    }

    nroffsets_ = trcbuf.size();
    SeisTrc* firsttrc = trcbuf.get(0);
    SeisTrc* nexttrc = trcbuf.get(1);
    if ( !firsttrc || !nexttrc || !firsttrc->size() || !nexttrc->size() )
	mErrRet( "No data to read" );
    
    headerpars_->set( "n3", nrbids );

    headerpars_->set( "o2", firsttrc->info().offset );
    headerpars_->set( "d2", nexttrc->info().offset - firsttrc->info().offset );
    headerpars_->set( "n2", nroffsets_ );

    headerpars_->set( "o1", firsttrc->info().sampling.start );
    headerpars_->set( "d1", firsttrc->info().sampling.step );
    headerpars_->set( "n1", firsttrc->size() );
}


void MadStream::fillHeaderParsFromStream()
{
    delete headerpars_; headerpars_ = 0;

    headerpars_ = new IOPar;
    char linebuf[256], tag[4], *ptr;
    if ( !istrm_ ) return;

    while ( *istrm_ )
    {
	int idx = 0, nullcount = 0;
	while( *istrm_ && nullcount<3 )
	{
	    if ( idx >= 255 )
		mErrRet("Error reading RSF header")

	    char c;
	    istrm_->get( c );
	    if ( c == '\n' ) { linebuf[idx] = '\0'; break; }
	    linebuf[idx++] = c;
	    if ( c==sKeyRSFEndOfHeader[nullcount] )
		nullcount++;
	}

	if ( nullcount > 2 ) break;

	ptr = linebuf + 1;
	if ( *(ptr+2)!='=' )continue;

	char* valptr = ptr + 3;
	*(ptr+2) = '\0';
	if ( !strcmp(ptr,"in") )
	{
	    valptr++;
	    const int sz = strlen( valptr );
	    *( valptr + sz - 1 ) = '\0';
	}

	headerpars_->set( ptr, valptr );
    }

    if ( !headerpars_->size() )
	mErrRet("Empty or corrupt RSF Header")
}


int MadStream::getNrSamples() const
{
    int nrsamps = 0;
    if ( headerpars_ ) headerpars_->get( "n1", nrsamps );

    return nrsamps;
}


bool MadStream::isOK() const
{
    if ( errMsg() ) return false;

    return true;
}


const char* MadStream::errMsg() const
{
    if ( errmsg_ == "" ) return 0;
    else 
	return errmsg_.buf();
}


bool MadStream::putHeader( std::ostream& strm )
{
    if ( !headerpars_ ) mErrBoolRet( "Header parameters not found" );

    for ( int idx=0; idx<headerpars_->size(); idx++ )
    {
	strm << "\t" << headerpars_->getKey(idx);
	strm << "=" << headerpars_->getValue(idx) << std::endl;
    }

#ifdef __win__
    strm << "\t" << "data_format=\"ascii_float\"" << std::endl;
#else
    strm << "\t" << "data_format=\"native_float\"" << std::endl;
#endif
    strm << "\t" << "in=\"stdout\"" << std::endl;
    strm << "\t" << "in=\"stdin\"" << std::endl;

    strm << sKeyRSFEndOfHeader;
    return true;
}


bool MadStream::getNextPos( BinID& bid )
{
    if ( !is2d_ )
	return iter_ && iter_->next( bid );

    if ( !l2ddata_ || !l2ddata_->posns_.size() ) return false;

    if ( !bid.crl )
    { bid.crl = l2ddata_->posns_[0].nr_; return true; }
    
    int idx = 0;
    while ( idx<l2ddata_->posns_.size()
	  && bid.crl != l2ddata_->posns_[idx].nr_ )
	idx++;

    if ( idx+1 >= l2ddata_->posns_.size() )
	return false;

    bid.crl = l2ddata_->posns_[idx+1].nr_;
    return true;
}


bool MadStream::getNextTrace( float* arr )
{
    if ( istrm_ && *istrm_ )
    {
	const int nrsamps = getNrSamples();
	mReadRSFTrc( arr );
	return *istrm_;
    }
    else if ( seisrdr_ )
    {
	SeisTrc trc;
	const int rv = seisrdr_->get( trc.info() );
	if ( rv < 0 ) mErrBoolRet( "Cannot read traces" )
	else if ( rv == 0 ) return false;

	if ( !seisrdr_->get(trc) ) mErrBoolRet( "Cannot read traces" );
	for ( int idx=0; idx<trc.size(); idx++ )
	    arr[idx] = trc.get( idx, 0 );

	return true;
    }
    else if ( psrdr_ )
    {
	if ( !trcbuf_ ) trcbuf_ = new SeisTrcBuf( true );

	if ( curtrcidx_ < 0 || curtrcidx_ >= nroffsets_ )
	{
	    if ( !getNextPos(curbid_) || !psrdr_->getGather(curbid_,*trcbuf_) )
		return false;

	    curtrcidx_ = 0;
	}

	const int nrsamps = getNrSamples();
	SeisTrc* trc = 0;
	if ( curtrcidx_ < trcbuf_->size() )
	    trc = trcbuf_->get( curtrcidx_ );

	for ( int idx=0; idx<nrsamps; idx++ )
	    arr[idx] = trc ? trc->get(idx,0) : 0;

	curtrcidx_++;
	return true;
    }

    mErrBoolRet( "No data source found" );
}


#define mReadFromPosFile( obj ) \
    BufferString posfnm = getPosFileName( true ); \
    if ( !posfnm.isEmpty() ) \
    { \
	haspos = true; \
	StreamProvider sp( posfnm ); \
	StreamData possd = sp.makeIStream(); \
	if ( !possd.usable() ) mErrBoolRet("Cannot Open Pos File"); \
       	if ( !obj.read(*possd.istrm,false) ) \
	    mErrBoolRet("Cannot Read Pos File"); \
	possd.close(); \
	FilePath fp( posfnm ); \
	if ( fp.pathOnly() == FilePath::getTempDir() ) \
	    sp.remove(); \
    }

bool MadStream::writeTraces( bool writetofile )
{
    if ( writetofile && ( ( isps_ && !pswrr_ ) || ( !isps_ && !seiswrr_ ) ) )
	mErrBoolRet( "Cannot initialize writing" );

    if ( is2d_ )
	return write2DTraces( writetofile );

    int inlstart=0, crlstart=1, inlstep=1, crlstep=1, nrinl=1, nrcrl, nrsamps;
    int firstoffset=0, nrtrcsperbinid=1, nrbinids=0;
    SamplingData<float> sd;
    bool haspos = false;
    PosInfo::CubeData cubedata;
    mReadFromPosFile( cubedata );
    headerpars_->get( "o1", sd.start );
    headerpars_->get( "d1", sd.step );
    headerpars_->get( "n1", nrsamps );
    if ( !isps_ )
    {
	nroffsets_ = 1;
	headerpars_->get( "o2", crlstart );
	headerpars_->get( "o3", inlstart );
	headerpars_->get( "n2", nrcrl );
	headerpars_->get( "n3", nrinl );
	headerpars_->get( "d2", crlstep );
	headerpars_->get( "d3", inlstep );
    }
    else
    {
	if ( !haspos )
	    mErrBoolRet( "Geometry data missing" );

	headerpars_->set( "n2", nrtrcsperbinid );
	headerpars_->set( "n3", nrbinids );
    }

    if ( haspos )
	nrinl = cubedata.size();

    float* buf = new float[nrsamps];
    for ( int inlidx=0; inlidx<nrinl; inlidx++ )
    {
	const int inl = haspos ? cubedata[inlidx]->linenr_
	    			: inlstart + inlidx * inlstep;

	const int nrsegs = haspos ? cubedata[inlidx]->segments_.size() : 1;
	for ( int segidx=0; segidx<nrsegs; segidx++ )
	{
	    if ( haspos )
	    {
		StepInterval<int> seg = cubedata[inlidx]->segments_[segidx];
		crlstart = seg.start; crlstep = seg.step;
		nrcrl = seg.nrSteps() + 1;
	    }

	    for ( int crlidx=0; crlidx<nrcrl; crlidx++ )
	    {
		int crl = crlstart + crlidx * crlstep;
		for ( int trcidx=1; trcidx<=nrtrcsperbinid; trcidx++ )
		{
		    mReadRSFTrc( buf );
		    SeisTrc* trc = new SeisTrc( nrsamps );
		    trc->info().sampling = sd;
		    trc->info().binid = BinID( inl, crl );
		    if ( isps_ ) trc->info().nr = trcidx;

		    for ( int isamp=0; isamp<nrsamps; isamp++ )
			trc->set( isamp, buf[isamp], 0 );

		    if ( writetofile )
		    {
			if ( ( isps_ && !pswrr_->put (*trc) )
			    || ( !isps_ && !seiswrr_->put(*trc) ) )
			{ 
			    delete [] buf; delete trc;
			    mErrBoolRet("Cannot write trace");
			}
			delete trc;
		    }
		    else
		    {
			if ( !stortrcbuf_ )
			    stortrcbuf_ = new SeisTrcBuf( true );

			stortrcbuf_->add( trc );
		    }
		}
	    }
	}
    }

    delete [] buf;
    return true;
}


bool MadStream::write2DTraces( bool writetofile )
{
    PosInfo::Line2DData geom;
    bool haspos = false;
    mReadFromPosFile ( geom );
    if ( !haspos ) mErrBoolRet( "Position data not available" );

    int nrtrcs=0, nrsamps=0, nroffsets=1;
    SamplingData<float> sd;

    headerpars_->get( "o1", sd.start );
    headerpars_->get( "n1", nrsamps );
    headerpars_->get( "d1", sd.step );

    if ( !isps_ )
	headerpars_->get( "n2", nrtrcs );
    else
    {
	headerpars_->get( "n2", nroffsets );
	headerpars_->get( "n3", nrtrcs );
    }

    if ( nrtrcs != geom.posns_.size() )
	mErrBoolRet( "Line geometry doesn't match with data" );

    float* buf = new float[nrsamps];
    for ( int idx=0; idx<geom.posns_.size(); idx++ )
    {
	const int trcnr = geom.posns_[idx].nr_;
	for ( int offidx=1; offidx<=nroffsets; offidx++ )
	{
	    mReadRSFTrc( buf );
	    SeisTrc* trc = new SeisTrc( nrsamps );
	    trc->info().sampling = sd;
	    trc->info().coord = geom.posns_[idx].coord_;
	    trc->info().binid.crl = trcnr;
	    trc->info().nr = trcnr;

	    for ( int isamp=0; isamp<nrsamps; isamp++ )
		trc->set( isamp, buf[isamp], 0 );

	    if ( writetofile )
	    {
		if ( ( isps_ && !pswrr_->put (*trc) )
			|| ( !isps_ && !seiswrr_->put(*trc) ) )
		{
		    delete [] buf; delete trc;
		    mErrBoolRet("Error writing traces");
		}
		delete trc;
	    }
	    else
	    {
		if ( !stortrcbuf_ )
		    stortrcbuf_ = new SeisTrcBuf( true );

		stortrcbuf_->add( trc );
	    }
	}
    }

    delete [] buf;
    return true;
}
#undef mErrRet
