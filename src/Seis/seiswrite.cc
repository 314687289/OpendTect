/*+
* COPYRIGHT: (C) dGB Beheer B.V.
* AUTHOR   : A.H. Bril
* DATE     : 28-1-1998
* FUNCTION : Seismic data writer
-*/

#include "seiswrite.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "seis2dline.h"
#include "seispsioprov.h"
#include "seispswrite.h"
#include "executor.h"
#include "iostrm.h"
#include "separstr.h"
#include "iopar.h"

SeisTrcWriter::SeisTrcWriter( const IOObj* ioob, const LineKeyProvider* l )
	: SeisStoreAccess(ioob)
    	, lineauxiopar(*new IOPar)
	, lkp(l)
{
    init();
}


SeisTrcWriter::SeisTrcWriter( const char* fnm, bool isps )
	: SeisStoreAccess(fnm,isps)
    	, lineauxiopar(*new IOPar)
	, lkp(0)
{
    init();
}


void SeisTrcWriter::init()
{
    putter = 0; psioprov = 0; pswriter = 0;
    nrtrcs = nrwritten = 0;
    prepared = false;
}


SeisTrcWriter::~SeisTrcWriter()
{
    close();
    delete &lineauxiopar;
}


bool SeisTrcWriter::close()
{
    bool ret = true;
    if ( putter )
	{ ret = putter->close(); if ( !ret ) errmsg = putter->errMsg(); }

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
	errmsg = "Info for output seismic data not found in Object Manager";
	return false;
    }

    if ( !psioprov && ((is2d && !lset) || (!is2d && !trl)) )
    {
	errmsg = "No data storer available for '";
	errmsg += ioobj->name(); errmsg += "'";
	return false;
    }
    if ( is2d && !lkp && ( !seldata || seldata->linekey_.isEmpty() ) )
    {
	errmsg = "Internal: 2D seismic can only be stored if line key known";
	return false;
    }

    if ( is2d )
    {
	if ( !next2DLine() )
	    return false;
    }
    else if ( psioprov )
    {
	pswriter = psioprov->makeWriter( ioobj->fullUserExpr(Conn::Write) );
	if ( !pswriter )
	{
	    errmsg = "Cannot open Pre-Stack data store for write";
	    return false;
	}
	pswriter->usePar( ioobj->pars() );
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
	{ errmsg = "No data from object manager"; return 0; }

    if ( isMultiConn() )
    {
	mDynamicCastGet(IOStream*,iostrm,ioobj)
	if ( iostrm->directNumberMultiConn() )
	    iostrm->setConnNr( inl );
	else if ( !first )
	    iostrm->toNextConnNr();
    }

    return ioobj->getConn( Conn::Write );
}


bool SeisTrcWriter::start3DWrite( Conn* conn, const SeisTrc& trc )
{
    strl()->cleanUp();
    if ( !conn || conn->bad() )
    {
	errmsg = "Cannot write to ";
	errmsg += ioobj->fullUserExpr(false);
	delete conn;
	return false;
    }

    if ( !strl()->initWrite(conn,trc) )
    {
	errmsg = strl()->errMsg();
	delete conn;
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
		   || (iostrm->directNumberMultiConn() &&
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


#define mCurLineKey (lkp ? lkp->lineKey() : seldata->linekey_)


bool SeisTrcWriter::next2DLine()
{
    LineKey lk = lkp ? lkp->lineKey() : seldata->linekey_;
    if ( !attrib.isEmpty() )
	lk.setAttrName( attrib );
    BufferString lnm = lk.lineName();
    if ( lnm.isEmpty() )
    {
	errmsg = "Cannot write to empty line name";
	return false;
    }

    prevlk = lk;
    delete putter;

    IOPar* lineiopar = new IOPar;
    lk.fillPar( *lineiopar, true );
    lineiopar->merge( lineauxiopar );
    putter = lset->linePutter( lineiopar );
    if ( !putter )
    {
	errmsg = "Cannot create 2D line writer";
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
	errmsg = putter->errMsg();
    return res;
}



bool SeisTrcWriter::put( const SeisTrc& trc )
{
    if ( !prepared ) prepareWork(trc);

    nrtrcs++;
    if ( seldata )
    {
	if ( seldata->type_ == Seis::TrcNrs )
	{
	    int selres = seldata->selRes(nrtrcs);
	    if ( selres == 1 )
		return true;
	    else if ( selres > 1 )
	    {
		errmsg = "Selected number of traces reached";
		return false;
	    }
	}
	else if ( seldata->selRes( trc.info().binid ) )
	    return true;
    }

    if ( is2d )
    {
	if ( !put2D(trc) )
	    return false;
    }
    else if ( psioprov )
    {
	if ( !pswriter )
	    return false;
	if ( !pswriter->put(trc) )
	{
	    errmsg = pswriter->errMsg();
	    return false;
	}
    }
    else
    {
	if ( !ensureRightConn(trc,false) )
	    return false;
	else if ( !strl()->write(trc) )
	{
	    errmsg = strl()->errMsg();
	    strl()->close(); delete trl; trl = 0;
	    return false;
	}
    }

    nrwritten++;
    return true;
}


bool SeisTrcWriter::isMultiConn() const
{
    mDynamicCastGet(IOStream*,iostrm,ioobj)
    return iostrm && iostrm->isMulti();
}
