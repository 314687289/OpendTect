/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 24-1-2001
 * FUNCTION : CBVS Seismic data translator
-*/

static const char* rcsID = "$Id: seiscbvs.cc,v 1.62 2004-12-30 17:29:35 bert Exp $";

#include "seiscbvs.h"
#include "seisinfo.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "cbvsreadmgr.h"
#include "cbvswritemgr.h"
#include "iostrm.h"
#include "cubesampling.h"
#include "iopar.h"
#include "binidselimpl.h"
#include "survinfo.h"
#include "strmprov.h"
#include "separstr.h"
#include "filegen.h"

const char* CBVSSeisTrcTranslator::sKeyDataStorage = "Data storage";
const char* CBVSSeisTrcTranslator::sKeyDefExtension = "cbvs";
const IOPar& CBVSSeisTrcTranslator::datatypeparspec = *new IOPar("CBVS option");


CBVSSeisTrcTranslator::CBVSSeisTrcTranslator( const char* nm, const char* unm )
	: SeisTrcTranslator(nm,unm)
	, headerdone(false)
	, donext(false)
	, forread(true)
	, storinterps(0)
	, userawdata(0)
	, blockbufs(0)
	, targetptrs(0)
	, preseldatatype(0)
	, rdmgr(0)
	, wrmgr(0)
	, nrdone(0)
    	, minimalhdrs(false)
    	, brickspec(*new VBrickSpec)
    	, single_file(false)
    	, is2d(false)
    	, coordpol((int)CBVSIO::NotStored)
{
}


CBVSSeisTrcTranslator::~CBVSSeisTrcTranslator()
{
    cleanUp();
    delete &brickspec;
}


CBVSSeisTrcTranslator* CBVSSeisTrcTranslator::make( const char* fnm,
			bool infoonly, bool is2d, BufferString* msg )
{
    if ( !fnm || !*fnm )
	{ if ( msg ) *msg = "Empty file name"; return 0; }

    CBVSSeisTrcTranslator* tr = CBVSSeisTrcTranslator::getInstance();
    tr->setSingleFile( is2d ); tr->set2D( is2d );
    tr->needHeaderInfoOnly( infoonly );
    if ( msg ) *msg = "";
    if ( !tr->initRead(new StreamConn(fnm,Conn::Read)) )
    {
	if ( msg ) *msg = tr->errMsg();
	delete tr; tr = 0;
    }
    return tr;
}



void CBVSSeisTrcTranslator::cleanUp()
{
    const int nrcomps = nrSelComps();
    SeisTrcTranslator::cleanUp();
    headerdone = false;
    donext =false;
    nrdone = 0;
    destroyVars( nrcomps );
}


void CBVSSeisTrcTranslator::destroyVars( int nrcomps )
{
    delete rdmgr; rdmgr = 0;
    delete wrmgr; wrmgr = 0;
    if ( !blockbufs ) return;

    for ( int idx=0; idx<nrcomps; idx++ )
    {
	delete [] blockbufs[idx];
	delete storinterps[idx];
    }

    delete [] blockbufs; blockbufs = 0;
    delete [] storinterps; storinterps = 0;
    delete [] compsel; compsel = 0;
    delete [] userawdata; userawdata = 0;
    delete [] targetptrs; targetptrs = 0;
}


void CBVSSeisTrcTranslator::setCoordPol( bool dowrite, bool intrailer )
{
    if ( !dowrite )
	coordpol = (int)CBVSIO::NotStored;
    else if ( intrailer )
	coordpol = (int)CBVSIO::InTrailer;
    else
	coordpol = (int)CBVSIO::InAux;
}


bool CBVSSeisTrcTranslator::getFileName( BufferString& fnm )
{
    if ( !conn || !conn->ioobj )
    {
	if ( !conn )
	    { errmsg = "Cannot reconstruct file name"; return false; }

	mDynamicCastGet(StreamConn*,strmconn,conn)
	if ( !strmconn )
	    { errmsg = "Wrong connection from Object Manager"; return false; }
	fnm = strmconn->fileName();
	return true;
    }

    mDynamicCastGet(IOStream*,iostrm,conn->ioobj)
    if ( !iostrm )
	{ errmsg = "Object manager provides wrong type"; return false; }

    // Catch the 'stdin' pretty name (currently "Std-IO")
    StreamProvider sp;
    fnm = iostrm->getExpandedName(true);
    if ( fnm == sp.fullName() )
	fnm = StreamProvider::sStdIO;

    conn->close();
    return true;
}


bool CBVSSeisTrcTranslator::initRead_( bool pinfo_only )
{
    forread = true;
    BufferString fnm; if ( !getFileName(fnm) ) return false;

    rdmgr = new CBVSReadMgr( fnm, 0, single_file, pinfo_only );
    if ( rdmgr->failed() )
	{ errmsg = rdmgr->errMsg(); return false; }

    minimalhdrs = !rdmgr->hasAuxInfo();
    const int nrcomp = rdmgr->nrComponents();
    const CBVSInfo& info = rdmgr->info();
    insd = info.sd;
    innrsamples = info.nrsamples;
    for ( int idx=0; idx<nrcomp; idx++ )
    {
	BasicComponentInfo& cinf = *info.compinfo[idx];
	addComp( cinf.datachar, cinf.name(), cinf.datatype );
    }
    pinfo->usrinfo = info.usertext;
    pinfo->stdinfo = info.stdtext;
    pinfo->nr = info.seqnr;
    pinfo->inlrg.start = info.geom.start.inl;
    pinfo->inlrg.stop = info.geom.stop.inl;
    pinfo->inlrg.step = info.geom.step.inl;
    pinfo->crlrg.start = info.geom.start.crl;
    pinfo->crlrg.stop = info.geom.stop.crl;
    pinfo->crlrg.step = info.geom.step.crl;
    rdmgr->getIsRev( pinfo->inlrev, pinfo->crlrev );
    return true;
}


bool CBVSSeisTrcTranslator::initWrite_( const SeisTrc& trc )
{
    if ( !trc.data().nrComponents() ) return false;
    forread = false;

    for ( int idx=0; idx<trc.data().nrComponents(); idx++ )
    {
	DataCharacteristics dc(trc.data().getInterpreter(idx)->dataChar());
	BufferString nm( "Component " );
	nm += idx+1;
	addComp( dc, nm );
	if ( preseldatatype )
	    tarcds[idx]->datachar = DataCharacteristics(
			(DataCharacteristics::UserType)preseldatatype );
    }

    return true;
}


bool CBVSSeisTrcTranslator::commitSelections_()
{
    if ( forread && !is2d && seldata && seldata->isPositioned() )
    {
	CubeSampling cs;
	Interval<int> inlrg = seldata->inlRange();
	Interval<int> crlrg = seldata->crlRange();
	cs.hrg.start.inl = inlrg.start; cs.hrg.start.crl = crlrg.start;
	cs.hrg.stop.inl = inlrg.stop; cs.hrg.stop.crl = crlrg.stop;
	cs.zrg.start = outsd.start; cs.zrg.step = outsd.step;
	cs.zrg.stop = outsd.start + (outnrsamples-1) * outsd.step;

	if ( !rdmgr->pruneReaders( cs ) )
	    { errmsg = "Input contains no relevant data"; return false; }
    }

    const int nrcomps = nrSelComps();
    userawdata = new bool [nrcomps];
    storinterps = new TraceDataInterpreter* [nrcomps];
    targetptrs = new unsigned char* [nrcomps];
    for ( int idx=0; idx<nrcomps; idx++ )
    {
	userawdata[idx] = inpcds[idx]->datachar == outcds[idx]->datachar;
	storinterps[idx] = new TraceDataInterpreter(
                  forread ? inpcds[idx]->datachar : outcds[idx]->datachar );
    }

    blockbufs = new unsigned char* [nrcomps];
    int bufsz = innrsamples + 1;
    for ( int iselc=0; iselc<nrcomps; iselc++ )
    {
	int nbts = inpcds[iselc]->datachar.nrBytes();
	if ( outcds[iselc]->datachar.nrBytes() > nbts )
	    nbts = outcds[iselc]->datachar.nrBytes();

	blockbufs[iselc] = new unsigned char [ nbts * bufsz ];
	if ( !blockbufs[iselc] ) { errmsg = "Out of memory"; return false; }
    }

    compsel = new bool [tarcds.size()];
    for ( int idx=0; idx<tarcds.size(); idx++ )
	compsel[idx] = tarcds[idx]->destidx >= 0;

    if ( !forread )
	return startWrite();

    if ( is2d && seldata && seldata->type_ == SeisSelData::Range )
    {
	// For 2D, inline is just an index number
	SeisSelData& sd = *const_cast<SeisSelData*>( seldata );
	sd.inlrg_.start = sd.inlrg_.stop = rdmgr->binID().inl;
    }

    if ( selRes(rdmgr->binID()) )
	return toNext();

    return true;
}


bool CBVSSeisTrcTranslator::inactiveSelData() const
{
    return isEmpty( seldata ) || seldata->type_ == SeisSelData::TrcNrs;
}


int CBVSSeisTrcTranslator::selRes( const BinID& bid ) const
{
    if ( inactiveSelData() )
	return 0;

    // Table for 2D: can't select because inl/crl in file is not 'true'
    if ( is2d && seldata->type_ == SeisSelData::Table )
	return 0;

    return seldata->selRes(bid);
}


bool CBVSSeisTrcTranslator::toNext()
{
    if ( inactiveSelData() )
	return rdmgr->toNext();

    const CBVSInfo& info = rdmgr->info();
    if ( info.nrtrcsperposn > 1 )
    {
	if ( !rdmgr->toNext() )
	    return false;
	else if ( !selRes(rdmgr->binID()) )
	    return true;
    }

    BinID nextbid = rdmgr->nextBinID();
    if ( nextbid == BinID(0,0) )
	return false;

    if ( !selRes(nextbid) )
	return rdmgr->toNext();

    // find next requested BinID
    while ( true )
    {
	while ( true )
	{
	    int res = selRes( nextbid );
	    if ( !res ) break;

	    if ( res%256 == 2 )
		{ if ( !info.geom.toNextInline(nextbid) ) return false; }
	    else if ( !info.geom.toNextBinID(nextbid) )
		return false;
	}

	if ( goTo(nextbid) )
	    break;
	else if ( !info.geom.toNextBinID(nextbid) )
	    return false;
    }

    return true;
}


bool CBVSSeisTrcTranslator::goTo( const BinID& bid )
{
    if ( rdmgr->goTo(bid) )
    {
	headerdone = false;
	donext = false;
	return true;
    }
    return false;
}


bool CBVSSeisTrcTranslator::readInfo( SeisTrcInfo& ti )
{
    if ( !storinterps && !commitSelections() ) return false;
    if ( headerdone ) return true;

    donext = donext || selRes( rdmgr->binID() );

    if ( donext && !toNext() ) return false;
    donext = true;

    if ( !rdmgr->getAuxInfo(auxinf) )
	return false;

    ti.getFrom( auxinf );
    ti.sampling.start = outsd.start;
    ti.sampling.step = outsd.step;
    ti.nr = ++nrdone;

    if ( ti.binid.inl == 0 && ti.binid.crl == 0 )
	ti.binid = SI().transform( ti.coord );

    return (headerdone = true);
}


bool CBVSSeisTrcTranslator::read( SeisTrc& trc )
{
    if ( !headerdone && !readInfo(trc.info()) )
	return false;

    prepareComponents( trc, outnrsamples );
    const CBVSInfo& info = rdmgr->info();
    const int nselc = nrSelComps();
    for ( int iselc=0; iselc<nselc; iselc++ )
    {
	const BasicComponentInfo& ci = *info.compinfo[ selComp(iselc) ];
	unsigned char* trcdata = trc.data().getComponent( iselc )->data();
	targetptrs[iselc] = userawdata[iselc] ? trcdata : blockbufs[iselc];
    }

    if ( !rdmgr->fetch( (void**)targetptrs, compsel, &samps ) )
    {
	errmsg = rdmgr->errMsg();
	return false;
    }

    for ( int iselc=0; iselc<nselc; iselc++ )
    {
	if ( !userawdata[iselc] )
	{
	    // Convert data into other format
	    for ( int isamp=0; isamp<outnrsamples; isamp++ )
		trc.set( isamp,
			 storinterps[iselc]->get( targetptrs[iselc], isamp ),
			 iselc );
	}
    }

    headerdone = false;
    return true;
}


bool CBVSSeisTrcTranslator::skip( int )
{
    if ( !rdmgr->skip(true) ) return false;
    donext = false;
    headerdone = false;
    return true;
}


BinID2Coord CBVSSeisTrcTranslator::getTransform() const
{
    if ( !rdmgr || !rdmgr->nrReaders() )
	return SI().binID2Coord();
    return rdmgr->info().geom.b2c;
}


bool CBVSSeisTrcTranslator::startWrite()
{
    BufferString fnm; if ( !getFileName(fnm) ) return false;

    CBVSInfo info;
    info.auxinfosel.setAll( !minimalhdrs );
    info.geom.fullyrectandreg = false;
    info.geom.b2c = SI().binID2Coord();
    info.stdtext = pinfo->stdinfo;
    info.usertext = pinfo->usrinfo;
    for ( int idx=0; idx<nrSelComps(); idx++ )
	info.compinfo += new BasicComponentInfo(*outcds[idx]);
    info.sd = insd;
    info.nrsamples = innrsamples;

    wrmgr = new CBVSWriteMgr( fnm, info, &auxinf, &brickspec, single_file,
	    			(CBVSIO::CoordPol)coordpol );
    if ( wrmgr->failed() )
	{ errmsg = wrmgr->errMsg(); return false; }

    return true;
}


bool CBVSSeisTrcTranslator::writeTrc_( const SeisTrc& trc )
{
    for ( int iselc=0; iselc<nrSelComps(); iselc++ )
    {
	const unsigned char* trcdata
	    		= trc.data().getComponent( selComp(iselc) )->data();
	unsigned char* blockbuf = blockbufs[iselc];
	targetptrs[iselc] = userawdata[iselc]
	    		  ? const_cast<unsigned char*>( trcdata )
			  : blockbuf;
	if ( !userawdata[iselc] )
	{
	    // Convert data into other format
	    int icomp = selComp(iselc);
	    for ( int isamp=samps.start; isamp<=samps.stop; isamp++ )
		storinterps[iselc]->put( blockbuf, isamp-samps.start,
					 trc.get(isamp,icomp) );
	}
    }

    trc.info().putTo( auxinf );
    if ( !wrmgr->put( (void**)targetptrs ) )
	{ errmsg = wrmgr->errMsg(); return false; }

    return true;
}


void CBVSSeisTrcTranslator::blockDumped( int nrtrcs )
{
    if ( nrtrcs > 1 && wrmgr )
	wrmgr->ensureConsistent();
}


const IOPar* CBVSSeisTrcTranslator::parSpec( Conn::State ) const
{
    if ( !datatypeparspec.size() )
    {
	FileMultiString fms;
	const char* ptr = DataCharacteristics::UserTypeNames[0];
	for ( int idx=0; DataCharacteristics::UserTypeNames[idx]
		     && *DataCharacteristics::UserTypeNames[idx]; idx++ )
	    fms += DataCharacteristics::UserTypeNames[idx];
	IOPar& ps = const_cast<IOPar&>( datatypeparspec );
	ps.set( sKeyDataStorage, fms );
    }
    return &datatypeparspec;
}


void CBVSSeisTrcTranslator::usePar( const IOPar& iopar )
{
    SeisTrcTranslator::usePar( iopar );

    const char* res = iopar.find( sKeyDataStorage );
    if ( res && *res )
	preseldatatype = (DataCharacteristics::UserType)(*res-'0');

    res = iopar.find( "Optimized direction" );
    if ( res && *res )
    {
	brickspec.setStd( *res == 'H' );
	if ( *res == 'H' && *res && *(res+1) == '`' )
	{
	    FileMultiString fms( res + 2 );
	    const int sz = fms.size();
	    int tmp = atoi( fms[0] );
	    if ( tmp > 0 )
		brickspec.nrsamplesperslab = tmp < 100000 ? tmp : 100000;
	    if ( sz > 1 )
	    {
		tmp = atoi( fms[1] );
		if ( tmp > 0 )
		    brickspec.maxnrslabs = tmp;
	    }
	}
    }
}


#define mImplStart(fn) \
    if ( !ioobj || strcmp(ioobj->translator(),"CBVS") ) return false; \
    mDynamicCastGet(const IOStream*,iostrm,ioobj) \
    if ( !iostrm ) return false; \
    if ( iostrm->isMulti() ) \
	return const_cast<IOStream*>(iostrm)->fn; \
 \
    BufferString pathnm = iostrm->dirName(); \
    BufferString basenm = iostrm->fileName()

#define mImplLoopStart \
	StreamProvider sp( CBVSIOMgr::getFileName(basenm,nr) ); \
	sp.addPathIfNecessary( pathnm ); \
	if ( !sp.exists(true) ) \
	    return true;


bool CBVSSeisTrcTranslator::implRemove( const IOObj* ioobj ) const
{
    mImplStart(implRemove());

    for ( int nr=0; ; nr++ )
    {
	mImplLoopStart;

	if ( !sp.remove(false) )
	    return nr ? true : false;
    }
}


bool CBVSSeisTrcTranslator::implRename( const IOObj* ioobj, const char* newnm,
       					const CallBack* cb ) const
{
    mImplStart( implRename(newnm) );

    bool rv = true;
    for ( int nr=0; ; nr++ )
    {
	mImplLoopStart;

	StreamProvider spnew( CBVSIOMgr::getFileName(newnm,nr) );
	spnew.addPathIfNecessary( pathnm );
	if ( !sp.rename(spnew.fileName(),cb) )
	    rv = false;
    }
}


bool CBVSSeisTrcTranslator::implSetReadOnly( const IOObj* ioobj, bool yn ) const
{
    mImplStart( implSetReadOnly(yn) );

    bool rv = true;
    for ( int nr=0; ; nr++ )
    {
	mImplLoopStart;

	if ( !sp.setReadOnly(yn) )
	    rv = false;
    }
}
