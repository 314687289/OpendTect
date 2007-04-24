/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2007
-*/

static const char* rcsID = "$Id: seiscubeprov.cc,v 1.10 2007-04-24 16:38:21 cvsbert Exp $";

#include "seismscprov.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "seisread.h"
#include "seisbuf.h"
#include "survinfo.h"
#include "cubesampling.h"
#include "ioobj.h"
#include "ioman.h"
#include "errh.h"


static const IOObj* nullioobj = 0;

SeisMSCProvider::SeisMSCProvider( const MultiID& id )
	: rdr_(*new SeisTrcReader(nullioobj))
{
    IOObj* ioobj = IOM().get( id );
    rdr_.setIOObj( ioobj );
    delete ioobj;
    init();
}


SeisMSCProvider::SeisMSCProvider( const IOObj& ioobj )
	: rdr_(*new SeisTrcReader(&ioobj))
{
    init();
}


SeisMSCProvider::SeisMSCProvider( const char* fnm )
	: rdr_(*new SeisTrcReader(fnm))
{
    init();
}


void SeisMSCProvider::init()
{
    readstate_ = NeedStart;
//  seldata_ = 0;
    intofloats_ = workstarted_ = false;
    errmsg_ = "";
    estnrtrcs_ = -2;
    reqmask_ = 0;
    bufidx_ = -1;
    trcidx_ = -1;
}


SeisMSCProvider::~SeisMSCProvider()
{
    rdr_.close();
    deepErase( tbufs_ );
//  delete seldata_;
    delete &rdr_;
    delete reqmask_;
}


bool SeisMSCProvider::prepareWork()
{
    const bool prepared = rdr_.isPrepared() ? true : rdr_.prepareWork();
    if ( !prepared )
	errmsg_ = rdr_.errMsg();

    return prepared;
}


bool SeisMSCProvider::is2D() const
{
    return rdr_.is2D();
}


void SeisMSCProvider::setStepout( int i, int c, bool req )
{
    if ( req )
    { 
	reqstepout_.r() = is2D() ? 0 : i; 
	reqstepout_.c() = c; 
	delete reqmask_;
	reqmask_ = 0;
    }
    else
    { 
	desstepout_.r() = is2D() ? 0 : i; 
	desstepout_.c() = c; 
    }
}


void SeisMSCProvider::setStepout( Array2D<bool>* mask )
{
    if ( !mask ) return;
    
    setStepout( (mask->info().getSize(0)-1)/2, 
		(mask->info().getSize(1)-1)/2, true );
    reqmask_ = mask;
}


void SeisMSCProvider::setSelData( SeisSelData* sd )
{
    rdr_.setSelData( sd );
}


/* Strategy:
   1) try going to next in already buffered traces: doAdvance()
   2) if not possible, read new trace.
   3) if !doAdvance() now, we're buffering
   */

SeisMSCProvider::AdvanceState SeisMSCProvider::advance()
{
    if ( !workstarted_ && !startWork() )
	{ errmsg_ = rdr_.errMsg(); return Error; }

    if ( doAdvance() )
	return NewPosition;
    else if ( readstate_ == ReadErr )
	return Error;
    else if ( readstate_ == ReadAtEnd )
	return EndReached;

    SeisTrc* trc = new SeisTrc;
    int res = readTrace( *trc );
    if ( res < 1 )
	delete trc;
    if ( res <= 0 )
    {
	readstate_ = res==0 ? ReadAtEnd : ReadErr;
	return advance();
    }

    trc->data().handleDataSwapping();

    SeisTrcBuf* addbuf = tbufs_.isEmpty() ? 0 : tbufs_[ tbufs_.size()-1 ];
    if ( is2D() && trc->info().new_packet )
	addbuf = 0;
    if ( !is2D() && addbuf && 
	 addbuf->get(0)->info().binid.inl != trc->info().binid.inl )
	addbuf = 0;	

    if ( !addbuf )
    {
	addbuf = new SeisTrcBuf;
	tbufs_ += addbuf;
    }

    addbuf->add( trc );

    return doAdvance() ? NewPosition : Buffering;
}


int SeisMSCProvider::comparePos( const SeisMSCProvider& mscp ) const
{
    if ( &mscp == this )
	return 0;

    if ( is2D() && mscp.is2D() )
    {
	const int mynr = getTrcNr();
	const int mscpsnr = mscp.getTrcNr();
	if ( mynr == mscpsnr )
	    return 0;
	return mynr > mscpsnr ? 1 : -1;
    }

    const BinID mybid = getPos();
    const BinID mscpsbid = mscp.getPos();
    if ( mybid == mscpsbid )
	return 0;

    if ( mybid.inl != mscpsbid.inl )
	return mybid.inl > mscpsbid.inl ? 1 : -1;

    return mybid.crl > mscpsbid.crl ? 1 : -1;
}


int SeisMSCProvider::estimatedNrTraces() const
{
    if ( estnrtrcs_ != -2 ) return estnrtrcs_;
    estnrtrcs_ = -1;
    if ( !rdr_.selData() )
	return is2D() ? estnrtrcs_ : SI().sampling(false).hrg.totalNr();

    estnrtrcs_ = rdr_.selData()->expectedNrTraces( is2D() );
    return estnrtrcs_;
}


bool SeisMSCProvider::startWork()
{
    if ( !prepareWork() ) return false;

    workstarted_ = true;
    rdr_.getSteps( stepoutstep_.r(), stepoutstep_.c() );
    rdr_.forceFloatData( intofloats_ );
    if ( reqstepout_.r() > desstepout_.r() ) desstepout_.r() = reqstepout_.r();
    if ( reqstepout_.c() > desstepout_.c() ) desstepout_.c() = reqstepout_.c();

//  delete seldata_; seldata_ = 0;

    if ( rdr_.selData() && !rdr_.selData()->all_ )
    {
//	seldata_ = new SeisSelData( *rdr_.selData() );
	SeisSelData* newseldata = new SeisSelData( *rdr_.selData() );
	BinID so( desstepout_.r(), desstepout_.c() );
	bool doextend = so.inl > 0 || so.crl > 0;
	if ( is2D() )
	{
	    so.inl = 0;
	    doextend = doextend && newseldata->type_ == Seis::Range;
	    if ( so.crl && newseldata->type_ == Seis::Table )
	    newseldata->all_ = true;
	}

	if ( doextend )
	{
	    BinID bid( stepoutstep_.r(), stepoutstep_.c() );
	    newseldata->extend( so, &bid );
	}

	rdr_.setSelData( newseldata );
    }

    SeisTrc* trc = new SeisTrc;
    int rv = readTrace( *trc );
    while ( rv > 1 )
	rv = readTrace( *trc );

    if ( rv < 0 )
	{ errmsg_ = rdr_.errMsg(); return false; }
    else if ( rv == 0 )
	{ errmsg_ = "No valid/selected trace found"; return false; }

    SeisTrcBuf* newbuf = new SeisTrcBuf;
    tbufs_ += newbuf;
    newbuf->add( trc );
    
    pivotidx_ = 0; pivotidy_ = 0;
    readstate_ = ReadOK;
    return true;
}



int SeisMSCProvider::readTrace( SeisTrc& trc )
{
    while ( true )
    {
	const int rv = rdr_.get( trc.info() );

	switch ( rv )
	{
	case 1:		break;
	case -1:	errmsg_ = rdr_.errMsg();	return -1;
	case 0:						return 0;
	case 2:
	default:					return 2;
	}

	if ( rdr_.get(trc) )
	    return 1;
	else
	{
	    BufferString msg( "Trace " );
	    if ( is2D() )
		msg += trc.info().nr;
	    else
		trc.info().binid.fill( msg.buf() + 6 );
	    msg += ": "; msg += rdr_.errMsg();
	    ErrMsg( msg );
	}
    }
}


BinID SeisMSCProvider::getPos() const
{
    return bufidx_==-1 ? BinID(-1,-1) :
			 tbufs_[bufidx_]->get(trcidx_)->info().binid; 
}


int SeisMSCProvider::getTrcNr() const
{
    return !is2D() || bufidx_==-1 ? -1 :
				    tbufs_[bufidx_]->get(trcidx_)->info().nr;
}


SeisTrc* SeisMSCProvider::get( int deltainl, int deltacrl )
{
    if ( bufidx_==-1 )
	return 0;
    if ( abs(deltainl)>desstepout_.r() || abs(deltacrl)>desstepout_.c() )
	return 0;

    BinID bidtofind( deltainl*stepoutstep_.r(), deltacrl*stepoutstep_.c() );
    bidtofind += !is2D() ? tbufs_[bufidx_]->get(trcidx_)->info().binid :
		 BinID( bufidx_, tbufs_[bufidx_]->get(trcidx_)->info().nr );
    
    int idx = mMIN( mMAX(0,bufidx_+deltainl), tbufs_.size()-1 ); 
    while ( !is2D() )
    {
	const int inldif = tbufs_[idx]->get(0)->info().binid.inl-bidtofind.inl;
	if ( !inldif )
	    break;
	if ( deltainl*inldif < 0 )
	    return 0;
	idx += deltainl>0 ? -1 : 1;
    }

    const int idy = tbufs_[idx]->find( bidtofind, is2D() );
    return idy<0 ? 0 : tbufs_[idx]->get(idy);
}


SeisTrc* SeisMSCProvider::get( const BinID& bid )
{
    if ( bufidx_==-1 || !stepoutstep_.r() || !stepoutstep_.c() )
	return 0;

    RowCol biddif( bid ); 
    biddif -= tbufs_[bufidx_]->get(trcidx_)->info().binid;

    RowCol delta( biddif ); delta /= stepoutstep_; 
    RowCol check( delta  ); check *= stepoutstep_;

    if ( biddif != check )
	return 0;
    
    return get( delta.r(), delta.c() );
}


// Distances to box borders: 0 on border, >0 outside, <0 inside.
#define mCalcBoxDistances(idx,idy,stepout) \
    const BinID curbid = is2D() ? \
	    BinID( idx, tbufs_[idx]->get(idy)->info().nr ) : \
	    tbufs_[idx]->get(idy)->info().binid; \
    const BinID pivotbid = is2D() ? \
	    BinID( pivotidx_, tbufs_[pivotidx_]->get(pivotidy_)->info().nr ) : \
	    tbufs_[pivotidx_]->get(pivotidy_)->info().binid; \
    RowCol bidstepout( stepout ); bidstepout *= stepoutstep_; \
    const int bottomdist = pivotbid.inl - curbid.inl - bidstepout.r(); \
    const int topdist = curbid.inl - pivotbid.inl - bidstepout.r(); \
    const int leftdist = pivotbid.crl - curbid.crl - bidstepout.c(); \
    const int rightdist = curbid.crl - pivotbid.crl - bidstepout.c();
   

bool SeisMSCProvider::isReqBoxFilled() const
{
    for ( int idy=0; idy<=2*reqstepout_.c(); idy++ )
    { 
	for ( int idx=0; idx<=2*reqstepout_.r(); idx++ )
	{
	    if ( !reqmask_ || reqmask_->get(idx,idy) )
	    {
		if ( !get(idx-reqstepout_.r(), idy-reqstepout_.c()) )
		    return false;
	    }
	}
    }
    return true;
}


bool SeisMSCProvider::doAdvance()
{
    while ( true )
    {
	bufidx_=-1; trcidx_=-1;

	// Remove oldest traces no longer needed from buffer.
	while ( !tbufs_.isEmpty() ) 
	{
	    if ( pivotidx_ < tbufs_.size() )
	    {
		mCalcBoxDistances(0,0,desstepout_);
		
		if ( bottomdist<0 || bottomdist==0 && leftdist<=0 )
		    break;
	    }

	    delete tbufs_[0]->remove(0); 
	    if ( pivotidx_ == 0 )
		pivotidy_--;
	    if ( tbufs_[0]->isEmpty() )
	    {
		delete tbufs_.remove(0);
		pivotidx_--; 
	    }
	}

	// If no traces left in buffer (e.g. at 0-stepouts), ask next trace.
	if ( tbufs_.isEmpty() )
	    return false;
	
	// If last trace not beyond desired box, ask next trace if available.
	if ( readstate_!=ReadAtEnd && readstate_!=ReadErr )
	{
	    const int lastidx = tbufs_.size()-1;
	    const int lastidy = tbufs_[lastidx]->size()-1;
	    mCalcBoxDistances(lastidx,lastidy,desstepout_);

	    if ( topdist<0 || topdist==0 && rightdist<0 )
		return false;
	}

	// Store current pivot position for external reference.
	bufidx_=pivotidx_; trcidx_=pivotidy_;
	
	// Determine next pivot position to consider.
	pivotidy_++;
	if ( pivotidy_ == tbufs_[pivotidx_]->size() )
	{
	    pivotidx_++; pivotidy_ = 0;
	}

	// Report stored pivot position if required box valid.
	if ( isReqBoxFilled() )
	    return true;
    }
}
