/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 28-1-1998
 * FUNCTION : Seismic data reader
-*/

static const char* rcsID = "$Id: seisread.cc,v 1.68 2007-04-11 10:10:19 cvsbert Exp $";

#include "seisread.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "executor.h"
#include "iostrm.h"
#include "streamconn.h"
#include "survinfo.h"
#include "binidselimpl.h"
#include "keystrs.h"
#include "segposinfo.h"
#include "errh.h"
#include "iopar.h"


#define mUndefPtr(clss) ((clss*)0xdeadbeef) // Like on AIX. Nothing special.


SeisTrcReader::SeisTrcReader( const IOObj* ioob )
	: SeisStoreAccess(ioob)
    	, outer(mUndefPtr(BinIDRange))
    	, fetcher(0)
    	, tbuf(0)
{
    init();
    if ( ioobj )
	entryis2d = SeisTrcTranslator::is2D( *ioob, false );
}



SeisTrcReader::SeisTrcReader( const char* fname )
	: SeisStoreAccess(fname)
    	, outer(mUndefPtr(BinIDRange))
    	, fetcher(0)
    	, tbuf(0)
{
    init();
}


#define mDelOuter if ( outer != mUndefPtr(BinIDRange) ) delete outer

SeisTrcReader::~SeisTrcReader()
{
    mDelOuter;
    delete tbuf;
    delete fetcher;
}


void SeisTrcReader::init()
{
    foundvalidinl = foundvalidcrl = entryis2d =
    new_packet = inforead = needskip = prepared = forcefloats = false;
    prev_inl = mUdf(int);
    readmode = Seis::Prod;
    if ( tbuf ) tbuf->deepErase();
    mDelOuter; outer = mUndefPtr(BinIDRange);
    delete fetcher; fetcher = 0;
    nrfetchers = 0; curlineidx = -1;
}


bool SeisTrcReader::prepareWork( Seis::ReadMode rm )
{
    if ( !ioobj )
    {
	errmsg = "Info for input seismic data not found in Object Manager";
	return false;
    }
    else if ( psioprov )
    {
	pErrMsg( "Cannot access Pre-Stack data through SeisTrcReader (yet)" );
	errmsg = "'"; errmsg += ioobj->name();
	errmsg += "' is a Pre-Stack data store";
	return false;
    }
    if ( (is2d && !lset) || (!is2d && !trl) )
    {
	errmsg = "No data interpreter available for '";
	errmsg += ioobj->name(); errmsg += "'";
	return false;
    }

    readmode = rm;
    Conn* conn = 0;
    if ( !is2d )
    {
	conn = openFirst();
	if ( !conn )
	{
	    errmsg = "Cannot open data files for '";
	    errmsg += ioobj->name(); errmsg += "'";
	    return false;
	}
    }

    if ( !initRead(conn) )
    {
	delete conn;
	return false;
    }

    return (prepared = true);
}



void SeisTrcReader::startWork()
{
    if ( psioprov || !trl ) return;

    outer = 0;
    if ( is2d )
    {
	tbuf = new SeisTrcBuf( true );
	return;
    }

    SeisTrcTranslator& sttrl = *strl();
    if ( forcefloats )
    {
	for ( int idx=0; idx<sttrl.componentInfo().size(); idx++ )
	    sttrl.componentInfo()[idx]->datachar = DataCharacteristics();
    }

    sttrl.setSelData( seldata );
    if ( sttrl.inlCrlSorted() && seldata )
    {
	outer = new BinIDRange;
	if ( !seldata->fill(*outer) )
	    { delete outer; outer = 0; }
    }
}


bool SeisTrcReader::isMultiConn() const
{
    return !psioprov && !is2d && !entryis2d
	&& ioobj && ioobj->hasConnType(StreamConn::sType)
	&& ((IOStream*)ioobj)->multiConn();
}


Conn* SeisTrcReader::openFirst()
{
    mDynamicCastGet(IOStream*,iostrm,ioobj)
    if ( iostrm )
	iostrm->setConnNr( iostrm->fileNumbers().start );

    trySkipConns();

    Conn* conn = ioobj->getConn( Conn::Read );
    if ( !conn || conn->bad() )
    {
	delete conn; conn = 0;
	if ( iostrm && isMultiConn() )
	{
	    while ( !conn || conn->bad() )
	    {
		delete conn; conn = 0;
		if ( !iostrm->toNextConnNr() ) break;

		conn = ioobj->getConn( Conn::Read );
	    }
	}
    }
    return conn;
}


bool SeisTrcReader::initRead( Conn* conn )
{
    if ( psioprov || is2d )
	return true;
    else
    {
	if ( !trl )
	    { pErrMsg("Should be a translator there"); return false; }
	mDynamicCastGet(SeisTrcTranslator*,sttrl,trl)
	if ( !sttrl )
	{
	    errmsg = trl->userName();
	    errmsg +=  "found where seismic cube was expected";
	    cleanUp(); return false;
	}
    }

    SeisTrcTranslator& sttrl = *strl();
    if ( !sttrl.initRead(conn,readmode) )
    {
	errmsg = sttrl.errMsg();
	cleanUp(); return false;
    }
    const int nrcomp = sttrl.componentInfo().size();
    if ( nrcomp < 1 )
    {
	// Why didn't the translator return false?
	errmsg = "Internal: no data components found";
	cleanUp(); return false;
    }

    // Make sure the right component(s) are selected.
    // If all else fails, take component 0.
    bool foundone = false;
    for ( int idx=0; idx<nrcomp; idx++ )
    {
	if ( sttrl.componentInfo()[idx]->destidx >= 0 )
	    { foundone = true; break; }
    }
    if ( !foundone )
    {
	for ( int idx=0; idx<nrcomp; idx++ )
	{
	    if ( selcomp == -1 )
		sttrl.componentInfo()[idx]->destidx = idx;
	    else
		sttrl.componentInfo()[idx]->destidx = selcomp == idx ? 0 : 1;
	    if ( sttrl.componentInfo()[idx]->destidx >= 0 )
		foundone = true;
	}
	if ( !foundone )
	    sttrl.componentInfo()[0]->destidx = 0;
    }

    needskip = false;
    return true;
}


int SeisTrcReader::get( SeisTrcInfo& ti )
{
    if ( !prepared && !prepareWork(readmode) )
	return -1;
    else if ( outer == mUndefPtr(BinIDRange) )
	startWork();

    if ( is2d )
	return get2D(ti);

    SeisTrcTranslator& sttrl = *strl();
    bool needsk = needskip; needskip = false;
    if ( needsk && !sttrl.skip() )
	return nextConn( ti );

    if ( !sttrl.readInfo(ti) )
	return nextConn( ti );

    ti.new_packet = false;

    if ( mIsUdf(prev_inl) )
	prev_inl = ti.binid.inl;
    else if ( prev_inl != ti.binid.inl )
    {
	foundvalidcrl = false;
	prev_inl = ti.binid.inl;
	if ( !entryis2d )
	    ti.new_packet = true;
    }

    int selres = 0;
    if ( seldata )
    {
	if ( !entryis2d )
	    selres = seldata->selRes(ti.binid);
	else
	{
	    BinID bid( seldata->inlrg_.start, ti.nr );
	    selres = seldata->selRes( bid );
	}
    }

    if ( selres / 256 == 0 )
	foundvalidcrl = true;
    if ( selres % 256 == 0 )
	foundvalidinl = true;

    if ( selres )
    {
	if ( !entryis2d && sttrl.inlCrlSorted() )
	{
	    int outerres = outer ? outer->excludes(ti.binid) : 0;
	    if ( foundvalidinl && outerres % 256 == 2 )
		return false;

	    bool neednewinl =  selres % 256 == 2
			    || foundvalidcrl && outerres / 256 == 2;
	    if ( neednewinl )
	    {
		mDynamicCastGet(IOStream*,iostrm,ioobj)
		if ( iostrm && iostrm->directNumberMultiConn() )
		    return nextConn(ti);
	    }
	}

	return sttrl.skip() ? 2 : nextConn( ti );
    }

    nrtrcs++;
    if ( !isEmpty(seldata) )
    {
	const int res = seldata->selRes( nrtrcs );
	if ( res > 1 )
	    return 0;
	if ( res == 1 )
	    return sttrl.skip() ? 2 : nextConn( ti );
    }

    // Hey, the trace is (believe it or not) actually selected!
    if ( new_packet )
    {
	ti.new_packet = true;
	new_packet = false;
    }
    needskip = true;
    return 1;
}


bool SeisTrcReader::get( SeisTrc& trc )
{
    needskip = false;
    if ( !prepared && !prepareWork(readmode) )
	return false;
    else if ( outer == mUndefPtr(BinIDRange) )
	startWork();
    if ( is2d )
	return get2D(trc);

    if ( !strl()->read(trc) )
    {
	errmsg = strl()->errMsg();
	strl()->skip();
	return false;
    }
    return true;
}


LineKey SeisTrcReader::lineKey() const
{
    if ( lset )
    {
	if ( curlineidx >= 0 && lset->nrLines() > curlineidx )
	    return lset->lineKey( curlineidx );
    }
    else if ( seldata )
	return seldata->linekey_;
    else if ( ioobj )
	return LineKey(ioobj->name(),ioobj->pars().find(sKey::Attribute));

    return LineKey(0,0);
}


class SeisTrcReaderLKProv : public LineKeyProvider
{
public:
    SeisTrcReaderLKProv( const SeisTrcReader& r )
				: rdr(r) {}
    LineKey lineKey() const	{ return rdr.lineKey(); }
    const SeisTrcReader&	rdr;
};


LineKeyProvider* SeisTrcReader::lineKeyProvider() const
{
    return new SeisTrcReaderLKProv( *this );
}


bool SeisTrcReader::ensureCurLineAttribOK( const BufferString& attrnm )
{
    const int nrlines = lset->nrLines();
    while ( curlineidx < nrlines )
    {
	if ( lset->lineKey(curlineidx).attrName() == attrnm )
	    break;
	curlineidx++;
    }

    bool ret = curlineidx < nrlines;
    if ( !ret && nrfetchers < 1 )
	errmsg = "No line found matching selection";
    return ret;
}


bool SeisTrcReader::mkNextFetcher()
{
    curlineidx++; tbuf->deepErase();
    LineKey lk( seldata ? seldata->linekey_ : "" );
    const BufferString attrnm = lk.attrName();
    const bool islinesel = !lk.lineName().isEmpty();
    const bool istable = seldata && seldata->type_ == Seis::Table;
    const int nrlines = lset->nrLines();

    if ( !islinesel )
    {
	if ( !ensureCurLineAttribOK(attrnm) )
	    return false;

	if ( istable )
	{
	    // Chances are we do not need to go through this line at all
	    while ( !lset->haveMatch(curlineidx,seldata->table_) )
	    {
	    	curlineidx++;
		if ( !ensureCurLineAttribOK(attrnm) )
		    return false;
	    }
	}
    }
    else
    {
	if ( nrfetchers > 0 )
	{ errmsg = ""; return false; }

	bool found = false;
	for ( ; curlineidx<nrlines; curlineidx++ )
	{
	    if ( lk == lset->lineKey(curlineidx) )
		{ found = true; break; }
	}
	if ( !found )
	{
	    errmsg = "Line key not found in line set: ";
	    errmsg += seldata->linekey_;
	    return false;
	}
    }

    StepInterval<float> zrg;
    lset->getRanges( curlineidx, curtrcnrrg, zrg );
    if ( seldata && !seldata->all_ && seldata->type_ == Seis::Range )
    {
	if ( seldata->crlrg_.start > curtrcnrrg.start )
	    curtrcnrrg.start = seldata->crlrg_.start;
	if ( seldata->crlrg_.stop < curtrcnrrg.stop )
	    curtrcnrrg.stop = seldata->crlrg_.stop;
    }

    prev_inl = mUdf(int);
    fetcher = lset->lineFetcher( curlineidx, *tbuf, 1, seldata );
    nrfetchers++;
    return fetcher;
}


bool SeisTrcReader::readNext2D()
{
    if ( tbuf->size() )
	tbuf->deepErase();

    int res = fetcher->doStep();
    if ( res == Executor::ErrorOccurred )
    {
	errmsg = fetcher->message();
	return false;
    }
    else if ( res == 0 )
    {
	if ( !mkNextFetcher() )
	    return false;
	return readNext2D();
    }

    return tbuf->size();
}


#define mNeedNextFetcher() (tbuf->size() == 0 && !fetcher)


int SeisTrcReader::get2D( SeisTrcInfo& ti )
{
    if ( !fetcher && !mkNextFetcher() )
	return errmsg.isEmpty() ? 0 : -1;

    if ( !readNext2D() )
	return errmsg.isEmpty() ? 0 : -1;

    inforead = true;
    SeisTrcInfo& trcti = tbuf->get( 0 )->info();
    trcti.new_packet = mIsUdf(prev_inl);
    ti = trcti;
    prev_inl = 0;

    bool isincl = true;
    if ( seldata )
    {
	if ( seldata->type_ == Seis::Table && !seldata->all_ )
	    // Not handled by fetcher
	    isincl = seldata->table_.includes(trcti.binid);
    }
    return isincl ? 1 : 2;
}


bool SeisTrcReader::get2D( SeisTrc& trc )
{
    if ( !inforead && !get2D(trc.info()) )
	return false;

    inforead = false;
    const SeisTrc* buftrc = tbuf->get( 0 );
    if ( !buftrc )
	{ pErrMsg("Huh"); return false; }
    trc.info() = buftrc->info();
    trc.copyDataFrom( *buftrc, -1, forcefloats );

    delete tbuf->remove(0);
    return true;
}


int SeisTrcReader::nextConn( SeisTrcInfo& ti )
{
    new_packet = false;
    if ( !isMultiConn() ) return 0;

    strl()->cleanUp();
    IOStream* iostrm = (IOStream*)ioobj;
    if ( !iostrm->toNextConnNr() )
	return 0;

    trySkipConns();
    Conn* conn = iostrm->getConn( Conn::Read );

    while ( !conn || conn->bad() )
    {
	delete conn; conn = 0;
	if ( !iostrm->toNextConnNr() ) return 0;

	trySkipConns();
	conn = iostrm->getConn( Conn::Read );
    }

    if ( !strl()->initRead(conn) )
    {
	errmsg = strl()->errMsg();
	return -1;
    }
    int rv = get(ti);
    if ( rv < 1 ) return rv;

    if ( seldata && seldata->isPositioned()
	         && iostrm->directNumberMultiConn() )
    {
	if ( !binidInConn(seldata->selRes(ti.binid)) )
	    return nextConn( ti );
    }

    if ( rv == 2 )	new_packet = true;
    else		ti.new_packet = true;

    return rv;
}


void SeisTrcReader::trySkipConns()
{
    if ( !isMultiConn() || !seldata || !seldata->isPositioned() )
	return;
    mDynamicCastGet(IOStream*,iostrm,ioobj)
    if ( !iostrm || !iostrm->directNumberMultiConn() ) return;

    BinID binid;

    if ( seldata->type_ == Seis::Range )
	binid.crl = seldata->crlrg_.start;
    else if ( seldata->table_.totalSize() == 0 )
	return;
    else
	binid.crl = seldata->table_.firstPos().crl;

    do
    {
	binid.inl = iostrm->connNr();
	if ( binidInConn(seldata->selRes(binid)) )
	    return;

    } while ( iostrm->toNextConnNr() );
}


bool SeisTrcReader::binidInConn( int r ) const
{
    return r == 0 || !strl()->inlCrlSorted() || r%256 != 2;
}


void SeisTrcReader::fillPar( IOPar& iopar ) const
{
    SeisStoreAccess::fillPar( iopar );
    if ( seldata )	seldata->fillPar( iopar );
    else		SeisSelData::removeFromPar( iopar );
}


void SeisTrcReader::getSteps( int& inl, int& crl ) const
{
    inl = crl = 1;
    if ( psioprov )
	return;

    if ( !is2d )
    {
	inl = trl ? strl()->packetInfo().inlrg.step : SI().inlStep();
	crl = trl ? strl()->packetInfo().crlrg.step : SI().crlStep();
	return;
    }

    if ( !lset || lset->nrLines() < 1 )
	return;
    PosInfo::Line2DData l2dd;
    if ( !lset->getGeometry(0,l2dd) || l2dd.posns.size() < 2 )
	return;

    crl = mUdf(int);
    int prevnr = l2dd.posns[0].nr_;
    for ( int idx=1; idx<l2dd.posns.size(); idx++ )
    {
	const int curnr = l2dd.posns[idx].nr_;
	const int step = abs( curnr - prevnr );
	if ( step > 0 && step < crl )
	{
	    crl = step;
	    if ( crl == 1 ) break;
	}
    }
}


void SeisTrcReader::getIsRev( bool& inl, bool& crl ) const
{
    inl = crl = false;
    if ( !trl || is2d || psioprov ) return;
    inl = strl()->packetInfo().inlrev;
    crl = strl()->packetInfo().crlrev;
}
