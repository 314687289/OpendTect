/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : June 2005
-*/

static const char* rcsID = "$Id: seisioobjinfo.cc,v 1.34 2010-08-06 10:44:32 cvsbert Exp $";

#include "seisioobjinfo.h"
#include "seis2dline.h"
#include "seiscbvs.h"
#include "seiscbvs2d.h"
#include "seiscbvs2dlinegetter.h"
#include "seispsioprov.h"
#include "seisread.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "seisselection.h"
#include "seistrctr.h"
#include "ptrman.h"
#include "iostrm.h"
#include "ioman.h"
#include "iodir.h"
#include "iopar.h"
#include "conn.h"
#include "linekey.h"
#include "survinfo.h"
#include "cubesampling.h"
#include "bufstringset.h"
#include "linekey.h"
#include "keystrs.h"
#include "zdomain.h"
#include "errh.h"

#define mGoToSeisDir() \
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id) )


SeisIOObjInfo::SeisIOObjInfo( const IOObj* ioobj )
    	: ioobj_(ioobj ? ioobj->clone() : 0)		{ setType(); }
SeisIOObjInfo::SeisIOObjInfo( const IOObj& ioobj )
    	: ioobj_(ioobj.clone())				{ setType(); }
SeisIOObjInfo::SeisIOObjInfo( const MultiID& id )
    	: ioobj_(IOM().get(id))				{ setType(); }


SeisIOObjInfo::SeisIOObjInfo( const char* ioobjnm )
    	: ioobj_(0)
{
    mGoToSeisDir();
    ioobj_ = IOM().getLocal( ioobjnm );
    setType();
}


SeisIOObjInfo::SeisIOObjInfo( const SeisIOObjInfo& sii )
    	: geomtype_(sii.geomtype_)
	, bad_(sii.bad_)
{
    ioobj_ = sii.ioobj_ ? sii.ioobj_->clone() : 0;
}


SeisIOObjInfo::~SeisIOObjInfo()
{
    delete ioobj_;
}


SeisIOObjInfo& SeisIOObjInfo::operator =( const SeisIOObjInfo& sii )
{
    if ( &sii != this )
    {
	delete ioobj_;
	ioobj_ = sii.ioobj_ ? sii.ioobj_->clone() : 0;
    	geomtype_ = sii.geomtype_;
    	bad_ = sii.bad_;
    }
    return *this;
}


void SeisIOObjInfo::setType()
{
    bad_ = !ioobj_;
    if ( bad_ ) return;

    const BufferString trgrpnm( ioobj_->group() );
    bool isps = false;
    if ( SeisTrcTranslator::isPS(*ioobj_) )
	isps = true;
    ioobj_->pars().getYN( SeisTrcTranslator::sKeyIsPS(), isps );

    if ( !isps && strcmp(ioobj_->group(),mTranslGroupName(SeisTrc)) )
	{ bad_ = true; return; }

    const bool is2d = SeisTrcTranslator::is2D( *ioobj_, false );
    geomtype_ = isps ? (is2d ? Seis::LinePS : Seis::VolPS)
		     : (is2d ? Seis::Line : Seis::Vol);
}


SeisIOObjInfo::SpaceInfo::SpaceInfo( int ns, int ntr, int bps )
	: expectednrsamps(ns)
	, expectednrtrcs(ntr)
	, maxbytespsamp(bps)
{
    if ( expectednrsamps < 0 )
	expectednrsamps = SI().zRange(false).nrSteps() + 1;
    if ( expectednrtrcs < 0 )
	expectednrtrcs = SI().sampling(false).hrg.totalNr();
}


#define mChk(ret) if ( bad_ ) return ret

bool SeisIOObjInfo::getDefSpaceInfo( SpaceInfo& spinf ) const
{
    mChk(false);

    if ( Seis::isPS(geomtype_) )
	{ pErrMsg( "No space info for PS" ); return false; }

    if ( Seis::is2D(geomtype_) )
	{ pErrMsg( "TODO: space info for 2D" ); return false; }

    CubeSampling cs;
    if ( !getRanges(cs) )
	return false;

    spinf.expectednrsamps = cs.zrg.nrSteps() + 1;
    spinf.expectednrtrcs = cs.hrg.totalNr();
    getBPS( spinf.maxbytespsamp, -1 );
    return true;
}


bool SeisIOObjInfo::isTime() const
{
    const bool siistime = SI().zIsTime();
    mChk(siistime);
    return ZDomain::isTime( ioobj_->pars() );
}


bool SeisIOObjInfo::isDepth() const
{
    const bool siisdepth = !SI().zIsTime();
    mChk(siisdepth);
    return ZDomain::isDepth( ioobj_->pars() );
}


const ZDomain::Def& SeisIOObjInfo::zDomainDef() const
{
    mChk(ZDomain::SI());
    return ZDomain::Def::get( ioobj_->pars() );
}


int SeisIOObjInfo::expectedMBs( const SpaceInfo& si ) const
{
    mChk(-1);

    if ( isPS() )
    {
	pErrMsg("TODO: no space estimate for PS");
	return -1;
    }

    Translator* tr = ioobj_->getTranslator();
    mDynamicCastGet(SeisTrcTranslator*,sttr,tr)
    if ( !sttr )
	{ pErrMsg("No Translator!"); return -1; }

    if ( si.expectednrtrcs < 0 || mIsUdf(si.expectednrtrcs) )
	return -1;

    int overhead = sttr->bytesOverheadPerTrace();
    delete tr;
    double sz = si.expectednrsamps;
    sz *= si.maxbytespsamp;
    sz = (sz + overhead) * si.expectednrtrcs;

    static const double bytes2mb = 9.53674e-7;
    return (int)((sz * bytes2mb) + .5);
}


bool SeisIOObjInfo::getRanges( CubeSampling& cs ) const
{
    mChk(false);
    mDynamicCastGet(IOStream*,iostrm,ioobj_)
    if ( iostrm && iostrm->isMulti() )
	{ cs.init( true ); return false; }
    return SeisTrcTranslator::getRanges( *ioobj_, cs );
}


bool SeisIOObjInfo::getBPS( int& bps, int icomp ) const
{
    mChk(false);
    if ( isPS() )
    {
	pErrMsg("TODO: no BPS for PS");
	return -1;
    }

    Translator* tr = ioobj_->getTranslator();
    mDynamicCastGet(SeisTrcTranslator*,sttr,tr)
    if ( !sttr )
	{ pErrMsg("No Translator!"); return false; }

    Conn* conn = ioobj_->getConn( Conn::Read );
    bool isgood = sttr->initRead(conn);
    bps = 0;
    if ( isgood )
    {
	ObjectSet<SeisTrcTranslator::TargetComponentData>& comps
	    	= sttr->componentInfo();
	for ( int idx=0; idx<comps.size(); idx++ )
	{
	    int thisbps = (int)comps[idx]->datachar.nrBytes();
	    if ( icomp < 0 )
		bps += thisbps;
	    else if ( icomp == idx )
		bps = thisbps;
	}
    }

    if ( bps == 0 ) bps = 4;
    return isgood;
}


void SeisIOObjInfo::getDefKeys( BufferStringSet& bss, bool add ) const
{
    if ( !add ) bss.erase();
    if ( !isOK() ) return;

    BufferString key( ioobj_->key().buf() );
    if ( !is2D() )
	{ bss.add( key.buf() ); bss.sort(); return; }
    else if ( isPS() )
	{ pErrMsg("2D PS not supported getting def keys"); return; }

    PtrMan<Seis2DLineSet> lset
	= new Seis2DLineSet( ioobj_->fullUserExpr(true) );
    if ( lset->nrLines() == 0 )
	return;

    BufferStringSet attrnms;
    lset->getAvailableAttributes( attrnms );
    for ( int idx=0; idx<attrnms.size(); idx++ )
	bss.add( LineKey(key.buf(),attrnms[idx]->buf()) );

    bss.sort();
}


#define mGetLineSet \
    if ( !add ) bss.erase(); \
    if ( !isOK() || !is2D() || isPS() ) return; \
 \
    PtrMan<Seis2DLineSet> lset \
	= new Seis2DLineSet( ioobj_->fullUserExpr(true) ); \
    if ( lset->nrLines() == 0 ) \
	return


void SeisIOObjInfo::getNms( BufferStringSet& bss, bool add, bool attr,
				const BinIDValueSet* bvs,
       				const char* datatype,
				bool allowcnstabsent, bool incl ) const
{
    if ( !isOK() )
	return;

    if ( isPS() )
    {
	SPSIOPF().getLineNames( *ioobj_, bss );
	return;
    }

    mGetLineSet;

    BufferStringSet rejected;
    for ( int idx=0; idx<lset->nrLines(); idx++ )
    {
	const char* nm = attr ? lset->attribute(idx) : lset->lineName(idx);
	if ( bss.indexOf(nm) >= 0 )
	    continue;
	else if ( bvs )
	{
	    if ( rejected.indexOf(nm) >= 0 )
		continue;
	    if ( !lset->haveMatch(idx,*bvs) )
	    {
		rejected.add( nm );
		continue;
	    }
	}

	if ( attr && datatype )
	{
	    const char* founddatatype = lset->datatype(idx);

	    if ( !founddatatype)
	    {
		if ( allowcnstabsent )
		{
		    if ( strcmp(datatype,nm) )  	bss.add( nm );
		}
		else
		{
		    if ( !strcmp(datatype,nm) )		bss.add( nm );
		}
	    }
	    else
	    {
		if ( !strcmp(datatype,founddatatype) && incl )
		    bss.add( nm );
	    }
	}
	else
	bss.add( nm );
    }

    bss.sort();
}


void SeisIOObjInfo::getNmsSubSel( const char* nm, BufferStringSet& bss,
				    bool add, bool l4a,
       				    const char* datatype,
				    bool allowcnstabsent, bool incl ) const
{
    mGetLineSet;
    if ( !nm || !*nm ) return;

    const BufferString target( nm );
    for ( int idx=0; idx<lset->nrLines(); idx++ )
    {
	const char* lnm = lset->lineName( idx );
	const char* anm = lset->attribute( idx );
	const char* requested = l4a ? anm : lnm;
	const char* listadd = l4a ? lnm : anm;

	if ( target == requested )
	{
	    if ( !l4a && datatype )
	    {
		const char* founddatatype = lset->datatype(idx);

		if ( !founddatatype)
		{
		    if ( allowcnstabsent )
		    {
			if ( strcmp(datatype,listadd) )
		    	    bss.addIfNew( listadd );
		    }
		    else
		    {
			if ( !strcmp(datatype,listadd) )
			    bss.addIfNew( listadd );
		    }
		}
		else
		{
		    if ( !strcmp(datatype,founddatatype) && incl )
			bss.addIfNew( listadd );
		}
	    }
	    else
	    bss.addIfNew( listadd );
	}
    }

    bss.sort();
}


bool SeisIOObjInfo::getRanges( const LineKey& lk, StepInterval<int>& trcrg,
			       StepInterval<float>& zrg ) const
{
    PtrMan<Seis2DLineSet> lset = new Seis2DLineSet(ioobj_->fullUserExpr(true));
    if ( lset->nrLines() == 0 )
	return false;

    const int lidx = lset->indexOf( lk );
    if ( lidx < 0 )
	return false;

    return lset->getRanges( lidx, trcrg, zrg );
}

	
BufferString SeisIOObjInfo::defKey2DispName( const char* defkey,
					     const char* ioobjnm )
{
    return LineKey::defKey2DispName( defkey, ioobjnm );
}


static BufferStringSet& getTypes()
{
    static BufferStringSet* types = 0;
    if ( !types )
	types = new BufferStringSet;
    return *types;
}

static TypeSet<MultiID>& getIDs()
{
    static TypeSet<MultiID>* ids = 0;
    if ( !ids )
	ids = new TypeSet<MultiID>;
    return *ids;
}


void SeisIOObjInfo::initDefault( const char* typ )
{
    BufferStringSet& typs = getTypes();
    const int typidx = typs.indexOf( typ );
    if ( typidx > -1 )
	return;

    IOObjContext ctxt( SeisTrcTranslatorGroup::ioContext() );
    ctxt.parconstraints.set( sKey::Type, typ );
    ctxt.includeconstraints = true;
    ctxt.allowcnstrsabsent = typ && *typ ? false : true;
    ctxt.trglobexpr = "CBVS";
    int nrpresent = 0;
    PtrMan<IOObj> ioobj = IOM().getFirst( ctxt, &nrpresent );
    if ( !ioobj || nrpresent > 1 )
	return;

    typs.add( typ );
    getIDs() += ioobj->key();
}


const MultiID& SeisIOObjInfo::getDefault( const char* typ )
{
    static const MultiID noid( "" );
    const int typidx = getTypes().indexOf( typ );
    return typidx < 0 ? noid : getIDs()[typidx];
}


void SeisIOObjInfo::setDefault( const MultiID& id, const char* typ )
{
    BufferStringSet& typs = getTypes();
    TypeSet<MultiID>& ids = getIDs();

    const int typidx = typs.indexOf( typ );
    if ( typidx > -1 )
	ids[typidx] = id;
    else
    {
	typs.add( typ );
	ids += id;
    }
}


int SeisIOObjInfo::nrComponents( LineKey lk ) const
{
    return getComponentInfo( lk, 0 );
}


void SeisIOObjInfo::getComponentNames( BufferStringSet& nms, LineKey lk ) const
{
    getComponentInfo( lk, &nms );
}


void SeisIOObjInfo::getCompNames( const LineKey& lk, BufferStringSet& nms )
{
    SeisIOObjInfo ioobjinf( MultiID(lk.lineName()) );
    LineKey tmplk( "",lk.attrName(), true );
    ioobjinf.getComponentNames( nms, tmplk );
}


int SeisIOObjInfo::getComponentInfo( LineKey lk, BufferStringSet* nms ) const
{
    int ret = 0;
    mChk(ret);

    if ( !is2D() )
    {
	Translator* tr = ioobj_->getTranslator();
	mDynamicCastGet(SeisTrcTranslator*,sttr,tr)
	if ( !sttr )
	    { pErrMsg("No Translator!"); return 0; }
	Conn* conn = ioobj_->getConn( Conn::Read );
	if ( sttr->initRead(conn) )
	{
	    ret = sttr->componentInfo().size();
	    if ( nms )
	    {
		for ( int icomp=0; icomp<ret; icomp++ )
		    nms->add( sttr->componentInfo()[icomp]->name() );
	    }
	}
	delete tr;
    }
    else
    {
	PtrMan<Seis2DLineSet> lset = new Seis2DLineSet(
						ioobj_->fullUserExpr(true));
	if ( !lset || lset->nrLines() == 0 )
	    return 0;
	int lidx = 0;
	const bool haveline = !lk.lineName().isEmpty();
	const BufferString attrnm( lk.attrName() );
	if ( haveline )
	    lidx = lset->indexOf( lk );
	else
	{
	    for ( int idx=0; idx<lset->nrLines(); idx++ )
		if ( attrnm == lset->attribute(idx) )
		    { lidx = idx; break; }
	}
	if ( lidx < 0 ) lidx = 0;
	SeisTrcBuf tbuf( true );
	Executor* ex = lset->lineFetcher( lidx, tbuf, 1 );
	if ( ex ) ex->doStep();
	ret = tbuf.isEmpty() ? 0 : tbuf.get(0)->nrComponents();
	if ( nms )
	{
	    mDynamicCastGet(SeisCBVS2DLineGetter*,lg,ex)
	    if ( !lg )
	    {
		for ( int icomp=0; icomp<ret; icomp++ )
		    nms->add( BufferString("[",icomp+1,"]") );
	    }
	    else
	    {
		ret = lg->tr ? lg->tr->componentInfo().size() : 0;
		if ( nms )
		{
		    for ( int icomp=0; icomp<ret; icomp++ )
			nms->add( lg->tr->componentInfo()[icomp]->name() );
		}
	    }
	}
	delete ex;
    }

    return ret;
}


void SeisIOObjInfo::get2DLineInfo( BufferStringSet& linesets,
				   TypeSet<MultiID>* setids,
				   TypeSet<BufferStringSet>* linenames )
{
    mGoToSeisDir();
    ObjectSet<IOObj> ioobjs = IOM().dirPtr()->getObjs();
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	if ( !SeisTrcTranslator::is2D(ioobj,true)
	  || SeisTrcTranslator::isPS(ioobj) ) continue;

	linesets.add( ioobj.name() );
	if ( setids )
	    *setids += ioobj.key();

	if ( linenames )
	{
	    SeisIOObjInfo oinf( ioobj );
	    BufferStringSet lnms;
	    oinf.getLineNames( lnms );
	    *linenames += lnms;
	}
    }
}
