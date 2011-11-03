/*+
* (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
* AUTHOR   : A.H. Bril
* DATE     : 28-1-1998
-*/
static const char* rcsID = "$Id: seiswrite.cc,v 1.68 2011-11-03 14:27:39 cvskris Exp $";

#include "seiswrite.h"
#include "keystrs.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "seisselection.h"
#include "seis2dline.h"
#include "seis2dlineio.h"
#include "seispsioprov.h"
#include "seispswrite.h"
#include "seispscubetr.h"
#include "executor.h"
#include "iostrm.h"
#include "iox.h"
#include "ioman.h"
#include "separstr.h"
#include "surv2dgeom.h"
#include "threadwork.h"
#include "iopar.h"


#define mCurLineKey (lkp ? lkp->lineKey() : (seldata ? seldata->lineKey() : ""))
const char* SeisTrcWriter::sKeyWriteBluntly() { return "Write bluntly"; }


SeisTrcWriter::SeisTrcWriter( const IOObj* ioob, const LineKeyProvider* l )
	: SeisStoreAccess(ioob)
    	, lineauxiopar(*new IOPar)
	, lkp(l)
	, worktrc(*new SeisTrc)
	, makewrready(true)
{
    init();
}


SeisTrcWriter::SeisTrcWriter( const char* fnm, bool is_2d, bool isps )
	: SeisStoreAccess(fnm,is_2d,isps)
    	, lineauxiopar(*new IOPar)
	, lkp(0)
	, worktrc(*new SeisTrc)
	, makewrready(true)
{
    init();
}


void SeisTrcWriter::init()
{
    putter = 0; pswriter = 0;
    nrtrcs = nrwritten = 0;
    prepared = false;
}


SeisTrcWriter::~SeisTrcWriter()
{
    close();
    delete &lineauxiopar;
    delete &worktrc;
}


bool SeisTrcWriter::close()
{
    bool ret = true;
    if ( putter )
	{ ret = putter->close(); if ( !ret ) errmsg_ = putter->errMsg(); }

    if ( is2D() )
    {
	LineKey lk = mCurLineKey;
	const int lineidx = lset ? lset->indexOf(lk) : -1;
	const BufferString lnm = lk.lineName();
	if ( lineidx>=0 && !lnm.isEmpty() )
	{
	    const bool hasgeom = S2DPOS().hasLine( lnm, lset->name() );
	    if ( !hasgeom )
	    {
		S2DPOS().setCurLineSet( lset->name() );
		geom_.setLineName( lnm );
		PosInfo::POS2DAdmin().setGeometry( geom_ );
	    }
	}
    }

    delete putter; putter = 0;
    delete pswriter; pswriter = 0;
    psioprov = 0;
    ret &= SeisStoreAccess::close();

    return ret;
}


bool SeisTrcWriter::prepareWork( const SeisTrc& trc )
{
    if ( !ioobj )
    {
	errmsg_ = "Info for output seismic data not found in Object Manager";
	return false;
    }

    if ( !psioprov && ((is2d && !lset) || (!is2d && !trl)) )
    {
	errmsg_ = "No data storer available for '";
	errmsg_ += ioobj->name(); errmsg_ += "'";
	return false;
    }
    if ( is2d && !lkp && ( !seldata || seldata->lineKey().isEmpty() ) )
    {
	errmsg_ = "Internal: 2D seismic can only be stored if line key known";
	return false;
    }

    if ( is2d && !psioprov )
    {
	if ( !next2DLine() )
	    return false;

	SamplingData<float> sd = trc.info().sampling;
	StepInterval<float> zrg( sd.start, 0, sd.step );
	zrg.stop = sd.start + sd.step * (trc.size()-1);
	geom_.setZRange( zrg );
    }
    else if ( psioprov )
    {
	const char* psstorkey = ioobj->fullUserExpr(Conn::Write);
	pswriter = is2d ? psioprov->make2DWriter( psstorkey, mCurLineKey )
	    		: psioprov->make3DWriter( psstorkey );
	if ( !pswriter )
	{
	    errmsg_ = "Cannot open Pre-Stack data store for write";
	    return false;
	}
	pswriter->usePar( ioobj->pars() );
	if ( !is2d )
	    SPSIOPF().mk3DPostStackProxy( *ioobj );
    }
    else
    {
	mDynamicCastGet(const IOStream*,strm,ioobj)
	if ( !strm || !strm->isMulti() )
	    fullImplRemove( *ioobj );

	if ( !ensureRightConn(trc,true) )
	    return false;
    }

    return (prepared = true);
}


Conn* SeisTrcWriter::crConn( int inl, bool first )
{
    if ( !ioobj )
	{ errmsg_ = "No data from object manager"; return 0; }

    if ( isMultiConn() )
    {
	mDynamicCastGet(IOStream*,iostrm,ioobj)
	iostrm->setConnNr( inl );
    }

    return ioobj->getConn( Conn::Write );
}


bool SeisTrcWriter::start3DWrite( Conn* conn, const SeisTrc& trc )
{
    if ( !conn || conn->bad() || !trl )
    {
	errmsg_ = "Cannot write to ";
	errmsg_ += ioobj->fullUserExpr(false);
	delete conn;
	return false;
    }

    strl()->cleanUp();
    if ( !strl()->initWrite(conn,trc) )
    {
	errmsg_ = strl()->errMsg();
	return false;
    }

    return true;
}


bool SeisTrcWriter::ensureRightConn( const SeisTrc& trc, bool first )
{
    bool neednewconn = !curConn3D();

    if ( !neednewconn && isMultiConn() )
    {
	mDynamicCastGet(IOStream*,iostrm,ioobj)
	neednewconn = trc.info().new_packet
		   || (iostrm->isMulti() &&
			iostrm->connNr() != trc.info().binid.inl);
    }

    if ( neednewconn )
    {
	Conn* conn = crConn( trc.info().binid.inl, first );
	if ( !conn || !start3DWrite(conn,trc) )
	    return false;
    }

    return true;
}


bool SeisTrcWriter::next2DLine()
{
    LineKey lk = mCurLineKey;
    if ( !attrib.isEmpty() )
	lk.setAttrName( attrib );
    BufferString lnm = lk.lineName();
    if ( lnm.isEmpty() )
    {
	errmsg_ = "Cannot write to empty line name";
	return false;
    }

    prevlk = lk;
    delete putter;

    IOPar* lineiopar = new IOPar;
    lk.fillPar( *lineiopar, true );

    if ( !datatype.isEmpty() )
	lineiopar->set( sKey::DataType, datatype.buf() );

    lineiopar->merge( lineauxiopar );
    putter = lset->linePutter( lineiopar );
    if ( !putter )
    {
	errmsg_ = "Cannot create 2D line writer";
	return false;
    }

    return true;
}


bool SeisTrcWriter::put2D( const SeisTrc& trc )
{
    if ( !putter ) return false;

    if ( mCurLineKey != prevlk )
    {
	if ( !next2DLine() )
	    return false;
    }

    bool res = putter->put( trc );
    if ( !res )
	errmsg_ = putter->errMsg();

    PosInfo::Line2DPos pos( trc.info().nr );
    pos.coord_ = trc.info().coord;
    geom_.add( pos );

    return res;
}



bool SeisTrcWriter::put( const SeisTrc& intrc )
{
    const SeisTrc* trc = &intrc;
    if ( makewrready && !intrc.isWriteReady() )
    {
	intrc.getWriteReady( worktrc );
	trc = &worktrc;
    }
    if ( !prepared ) prepareWork(*trc);

    nrtrcs++;
    if ( seldata && seldata->selRes( trc->info().binid ) )
	return true;

    if ( psioprov )
    {
	if ( !pswriter )
	    return false;
	if ( !pswriter->put(*trc) )
	{
	    errmsg_ = pswriter->errMsg();
	    return false;
	}
    }
    else if ( is2d )
    {
	if ( !put2D(*trc) )
	    return false;
    }
    else
    {
	if ( !ensureRightConn(*trc,false) )
	    return false;
	else if ( !strl()->write(*trc) )
	{
	    errmsg_ = strl()->errMsg();
	    strl()->close(); delete trl; trl = 0;
	    return false;
	}
    }

    nrwritten++;
    return true;
}


void SeisTrcWriter::usePar( const IOPar& iop )
{
    SeisStoreAccess::usePar( iop );
    bool wrbl = !makewrready;
    iop.getYN( sKeyWriteBluntly(), wrbl );
    makewrready = !wrbl;
}


void SeisTrcWriter::fillPar( IOPar& iop ) const
{
    SeisStoreAccess::fillPar( iop );
    iop.setYN( sKeyWriteBluntly(), !makewrready );
}


bool SeisTrcWriter::isMultiConn() const
{
    mDynamicCastGet(IOStream*,iostrm,ioobj)
    return iostrm && iostrm->isMulti();
}


SeisSequentialWriter::SeisSequentialWriter( SeisTrcWriter* writer,
					    int buffsize )
    : writer_( writer )
    , maxbuffersize_( buffsize<1 ? Threads::getNrProcessors()*2 : buffsize )
    , latestbid_( -1, -1 )
{
    queueid_ = Threads::WorkManager::twm().addQueue(
				    Threads::WorkManager::SingleThread );
}


bool SeisSequentialWriter::finishWrite()
{
    if ( outputs_.size() )
    {
	pErrMsg( "Buffer is not empty" );
	deepErase( outputs_ );
    }

    Threads::WorkManager::twm().emptyQueue( queueid_, true );

    return errmsg_.isEmpty();
}


SeisSequentialWriter::~SeisSequentialWriter()
{
    if ( outputs_.size() )
    {
	pErrMsg( "Buffer is not empty" );
	deepErase( outputs_ );
    }

    if ( Threads::WorkManager::twm().queueSize( queueid_ ) )
    {
	pErrMsg("finishWrite is not called");
    }

    Threads::WorkManager::twm().removeQueue( queueid_, false );
}


bool SeisSequentialWriter::announceTrace( const BinID& bid )
{
    Threads::MutexLocker lock( lock_ );
    if ( bid.inl<latestbid_.inl ||
	    (bid.inl==latestbid_.inl && bid.crl<latestbid_.crl ) )
    {
	errmsg_ = "Announced trace is out of sequence";
	return false;
    }

    announcedtraces_ += bid;
    return true;
}


class SeisSequentialWriterTask : public Task
{
public:
    SeisSequentialWriterTask( SeisSequentialWriter& imp, SeisTrcWriter& writer,
			      SeisTrc* trc )
	: ssw_( imp )
	, writer_( writer )
	, trc_( trc )
    {}

    bool execute()
    {
	BufferString errmsg;
	if ( !writer_.put( *trc_ ) )
	{
	    errmsg = writer_.errMsg();
	    if ( errmsg.isEmpty() )
	    {
		pErrMsg( "Need an error message from writer" );
		errmsg = "Cannot write trace";
	    }
	}

	delete trc_;

	ssw_.reportWrite( errmsg.str() );
	return !errmsg.str();
    }


protected:

    SeisSequentialWriter&       ssw_;
    SeisTrcWriter&     		writer_;
    SeisTrc*             	trc_;
};



bool SeisSequentialWriter::submitTrace( SeisTrc* inptrc, bool waitforbuffer )
{
    Threads::MutexLocker lock( lock_ );
    outputs_ += inptrc;
    return iterateBuffer( waitforbuffer );
}


bool SeisSequentialWriter::submitGather( ObjectSet<SeisTrc>& gather,
					 bool waitforbuffer )
{
    pErrMsg("Not implemented yet. Talk to Bruno" );
    return false;

    Threads::MutexLocker lock( lock_ );
    for ( int idx=0; idx<gather.size(); idx++ )
	outputs_ += gather[idx];

    return iterateBuffer( waitforbuffer );
}


bool SeisSequentialWriter::iterateBuffer( bool waitforbuffer )
{
    bool found = true;
    while ( found )
    {
	found = false;
	int idx = 0;

	while ( idx<announcedtraces_.size() )
	{
	    const BinID& bid = announcedtraces_[idx];

	    ObjectSet<SeisTrc> trcs;
	    for ( int idy=0; idy<outputs_.size(); idy++ )
	    {
		if ( outputs_[idy]->info().binid==bid )
		{
		    trcs += outputs_.remove( idy );
		    idy--;
		}
		else if ( trcs.size() )
		    break;
	    }

	    if ( !trcs.size() )
	    {
		idx--;
		break;
	    }

	    for ( int idy=0; idy<trcs.size(); idy++ )
	    {
		Task* task =
		    new SeisSequentialWriterTask( *this, *writer_, trcs[idy] );
		Threads::WorkManager::twm().addWork( Threads::Work(*task,true), 0,
						     queueid_, false );
	    }

	    found = true;

	    idx++;
	}

	if ( idx>=0 )
	    announcedtraces_.remove( 0, idx );
    }

    if ( found )
	lock_.signal(true);

    if ( waitforbuffer )
    {
	int bufsize = outputs_.size() +
	    Threads::WorkManager::twm().queueSize( queueid_ );
	while ( bufsize>=maxbuffersize_ && !errmsg_.str() )
	{
	    lock_.wait();
	    bufsize = outputs_.size() +
			Threads::WorkManager::twm().queueSize( queueid_ );
	}
    }

    return errmsg_.isEmpty();
}


void SeisSequentialWriter::reportWrite( const char* errmsg )
{
    Threads::MutexLocker lock( lock_ );
    if ( errmsg )
    {
	errmsg_ = errmsg;
	Threads::WorkManager::twm().emptyQueue( queueid_, false );
	lock_.signal( true );
	return;
    }

    const int bufsize = Threads::WorkManager::twm().queueSize( queueid_ ) + 
			outputs_.size();
    if ( bufsize<maxbuffersize_ )
	lock_.signal( true );
}
