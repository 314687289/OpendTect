/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID mUsedVar = "$Id$";

#include "attribstorprovider.h"

#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attriblinebuffer.h"
#include "attribdataholder.h"
#include "attribdatacubes.h"
#include "datainpspec.h"
#include "datapack.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "linekey.h"
#include "linesetposinfo.h"
#include "multiid.h"
#include "ptrman.h"
#include "seis2ddata.h"
#include "seisbounds.h"
#include "seisbufadapters.h"
#include "seiscbvs.h"
#include "seiscubeprov.h"
#include "seisioobjinfo.h"
#include "seispacketinfo.h"
#include "seisread.h"
#include "seistrc.h"
#include "seisselectionimpl.h"
#include "seistrctr.h"
#include "simpnumer.h"
#include "statruncalc.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "posinfo2dsurv.h"
#include "threadwork.h"
#include "task.h"
#include <math.h>


namespace Attrib
{

mAttrDefCreateInstance(StorageProvider)

void StorageProvider::initClass()
{
    mAttrStartInitClassWithUpdate

    desc->addParam( new SeisStorageRefParam(keyStr()) );
    desc->addOutputDataType( Seis::UnknowData );

    mAttrEndInitClass
}


void StorageProvider::updateDesc( Desc& desc )
{
    updateDescAndGetCompNms( desc, 0 );
}


void StorageProvider::updateDescAndGetCompNms( Desc& desc,
					       BufferStringSet* compnms )
{
    const LineKey lk( desc.getValParam(keyStr())->getStringValue(0) );

    BufferString linenm = lk.lineName();
    if (  linenm.firstChar() == '#' )
    {
	DataPack::FullID fid( linenm.buf()+1 );
	if ( !DPM(fid).haveID( fid ) )
	    desc.setErrMsg( "Cannot find data in memory" );
	return;
    }

    const MultiID key( linenm );
    PtrMan<IOObj> ioobj = IOM().get( key );
    SeisTrcReader rdr( ioobj );
    if ( !rdr.ioObj() || !rdr.prepareWork(Seis::PreScan) || rdr.psIOProv() )
    {
//	TODO desc.setErrMsg( rdr.errMsg() );
	return;
    }

    if ( compnms )
	compnms->erase();

    if ( rdr.is2D() )
    {
	if ( !rdr.dataSet() )
	{
	    BufferString errmsg = "No dataset available for '";
	    errmsg += ioobj->name(); errmsg += "'";
//	    desc.setErrMsg( errmsg );
	    return;
	}

	FixedString datatype = rdr.dataSet()->dataType();
	if ( datatype != sKey::Steering() )
	{
	    SeisTrcTranslator* transl = rdr.seisTranslator();
	    if ( !transl )
	       desc.setNrOutputs( Seis::UnknowData, 1 );//why only 1 ?
	    else if ( transl->componentInfo().isEmpty() )
	    {
		BufferStringSet complist;
		SeisIOObjInfo::getCompNames(key, complist);
		desc.setNrOutputs( Seis::UnknowData, complist.size() );
		if ( compnms )
		    compnms->operator =( complist );
	    }
	    else
	    {
		for ( int idx=0; idx<transl->componentInfo().size(); idx++ )
		    desc.addOutputDataType( (Seis::DataType)
					transl->componentInfo()[0]->datatype );
	    }
	}
	else
	{
	    desc.setNrOutputs( Seis::Dip, 2 );
	    if ( compnms )
		compnms->add( rdr.dataSet()->name() );
	}
    }
    else
    {
	SeisTrcTranslator* transl = rdr.seisTranslator();
	if ( !transl )
	{
	    BufferString errmsg = "No data interpreter available for '";
	    errmsg += ioobj->name(); errmsg += "'";
//	    desc.setErrMsg ( errmsg );
	    return;
	}

	BufferString type;
	ioobj->pars().get( sKey::Type(), type );

	const int nrattribs = transl->componentInfo().size();
	if ( type == sKey::Steering() )
	    desc.setNrOutputs( Seis::Dip, nrattribs );
	else
	{
	    for ( int idx=1; idx<=nrattribs; idx++ )
		if ( desc.nrOutputs() < idx )
		    desc.addOutputDataType( (Seis::DataType)
				    transl->componentInfo()[idx-1]->datatype);
	}

	if ( compnms )
	    transl->getComponentNames( *compnms );
    }
}


StorageProvider::StorageProvider( Desc& desc )
    : Provider( desc )
    , mscprov_(0)
    , status_( Nada )
    , stepoutstep_(-1,0)
    , isondisc_(true)
    , useintertrcdist_(false)
    , ls2ddata_(0)
{
    const LineKey lk( desc.getValParam(keyStr())->getStringValue(0) );
    BufferString bstring = lk.lineName();
    const char* linenm = bstring.buf();
    if ( linenm && *linenm == '#' )
    {
	DataPack::FullID fid( linenm+1 );
	isondisc_ =  !DPM(fid).haveID( fid );
    }
}


StorageProvider::~StorageProvider()
{
    if ( mscprov_ ) delete mscprov_;
    if ( ls2ddata_ ) delete ls2ddata_;
}


#undef mErrRet
#define mErrRet(s) { errmsg_ = s; delete mscprov_; mscprov_= 0; return false; }

bool StorageProvider::checkInpAndParsAtStart()
{
    if ( status_!=Nada ) return false;

    if ( !isondisc_ )
    {
	storedvolume_.zrg.start = 0;	//cover up for synthetics
	DataPack::FullID fid( getDPID() );
	DataPackRef<SeisTrcBufDataPack> stbdtp =
				DPM(fid).obtain( DataPack::getID(fid) );
	if ( !stbdtp ) return false;
	SeisPacketInfo si;
	stbdtp->trcBuf().fill( si );
	storedvolume_.hrg.setInlRange( si.inlrg );
	storedvolume_.hrg.setCrlRange( si.crlrg );
	storedvolume_.zrg = si.zrg;
	return true;
    }

    const LineKey lk( desc_.getValParam(keyStr())->getStringValue(0) );
    const MultiID mid( lk.lineName() );
    if ( !isOK() ) return false;
    mscprov_ = new SeisMSCProvider( mid );

    if ( !initMSCProvider() )
	mErrRet( mscprov_->errMsg() )

    const bool is2d = mscprov_->is2D();
    desc_.set2D( is2d );
    if ( !is2d )
	SeisTrcTranslator::getRanges( mid, storedvolume_, lk );
    else
    {
	Seis2DDataSet* dset = mscprov_->reader().dataSet();
	if ( !dset )
	    mErrRet( "2D seismic data/No data set found" );

	int lineidx = dset->indexOf( lk.buf() );
	if ( lineidx == -1 )
	{
	    storedvolume_.hrg.start.inl() = 0;
	    storedvolume_.hrg.stop.inl() = 1;
	    storedvolume_.hrg.include( BinID( 0,0 ) );
	    storedvolume_.hrg.include( BinID( 0,SI().maxNrTraces(true) ) );
	    storedvolume_.hrg.step.crl() = 1; // what else?
	    BufferStringSet candidatelines;
	    dset->getLineNames( candidatelines );
	    bool foundone = false;
	    for ( int idx=0; idx<candidatelines.size(); idx++ )
	    {
		lineidx = dset->indexOf( candidatelines.get(idx).buf() );
		if ( lineidx> -1 )
		{
		    StepInterval<int> trcrg; StepInterval<float> zrg;
		    if ( dset->getRanges( lineidx, trcrg, zrg ) )
		    {
			if ( foundone )
			{
			    storedvolume_.hrg.include( BinID(0,trcrg.start) );
			    storedvolume_.hrg.include( BinID(0,trcrg.stop) );
			    storedvolume_.zrg.include( zrg );
			}
			else
			{
			    storedvolume_.hrg.start.crl() = trcrg.start;
			    storedvolume_.hrg.stop.crl() = trcrg.stop;
			    storedvolume_.zrg = zrg;
			}
			foundone = true;
		    }
		}
	    }
	}
	else
	{
	    storedvolume_.hrg.start.inl() = storedvolume_.hrg.stop.inl()
					  = lineidx;
	    StepInterval<int> trcrg; StepInterval<float> zrg;
	    if ( !dset->getRanges( lineidx, trcrg, zrg ) )
		mErrRet("Cannot get needed trace range from 2D line set")
	    else
	    {
		storedvolume_.hrg.start.crl() = trcrg.start;
		storedvolume_.hrg.stop.crl() = trcrg.stop;
		storedvolume_.zrg = zrg;
	    }
	}
    }

    status_ = StorageOpened;
    return true;
}


int StorageProvider::moveToNextTrace( BinID startpos, bool firstcheck )
{
    if ( alreadymoved_ )
	return 1;

    if ( status_==Nada && isondisc_ )
	return -1;

    if ( status_==StorageOpened )
    {
	if ( !setMSCProvSelData() )
	    return -1;

	status_ = Ready;
    }

    if ( !useshortcuts_ )
    {
	if ( getDesc().is2D() )
	    prevtrcnr_ = currentbid_.crl();

	bool validstartpos = startpos != BinID(-1,-1);
	if ( validstartpos && curtrcinfo_ && curtrcinfo_->binid == startpos )
	{
	    alreadymoved_ = true;
	    return 1;
	}
    }

    bool advancefurther = true;
    while ( advancefurther )
    {
	if ( isondisc_ )
	{
	    SeisMSCProvider::AdvanceState res = mscprov_ ? mscprov_->advance()
						 : SeisMSCProvider::EndReached;
	    switch ( res )
	    {
		case SeisMSCProvider::Error:	{ errmsg_ = mscprov_->errMsg();
						      return -1; }
		case SeisMSCProvider::EndReached:	return 0;
		case SeisMSCProvider::Buffering:	continue;
						//TODO return 'no new position'

		case SeisMSCProvider::NewPosition:
		{
		    if ( useshortcuts_ )
			{ advancefurther = false; continue; }

		    SeisTrc* trc = mscprov_->get( 0, 0 );
		    if ( !trc ) continue; // should not happen

		    registerNewPosInfo( trc, startpos, firstcheck,
					advancefurther );
		}
	    }
	}
	else
	{
	    SeisTrc* trc = getTrcFromPack( BinID(0,0), 1 );
	    if ( !trc ) return 0;

	    status_ = Ready;
	    registerNewPosInfo( trc, startpos, firstcheck, advancefurther );
	}
    }

    setCurrentPosition( currentbid_ );
    alreadymoved_ = true;
    return 1;
}


void StorageProvider::registerNewPosInfo( SeisTrc* trc, const BinID& startpos,
					  bool firstcheck, bool& advancefurther)
{
    for ( int idx=0; idx<trc->nrComponents(); idx++ )
    {
	if ( datachar_.size()<=idx )
	    datachar_ +=
		trc->data().getInterpreter()->dataChar();
	else
	    datachar_[idx] =
		trc->data().getInterpreter()->dataChar();
    }

    curtrcinfo_ = 0;
    const SeisTrcInfo& newti = trc->info();
    currentbid_ = desc_.is2D()? BinID( 0, newti.nr ) : newti.binid;
    trcinfobid_ = newti.binid;
    if ( firstcheck || startpos == BinID(-1,-1) || currentbid_ == startpos
	    || newti.binid == startpos )
    {
	advancefurther = false;
	curtrcinfo_ = &trc->info();
    }
}


#define mAdjustToAvailStep( dir )\
{\
    if ( res.hrg.step.dir>1 )\
    {\
	float remain = ( possiblevolume_->hrg.start.dir - res.hrg.start.dir ) %\
			res.hrg.step.dir;\
	if ( !mIsZero( remain, 1e-3 ) )\
	    res.hrg.start.dir = possiblevolume_->hrg.start.dir + \
				mNINT32(remain +0.5) *res.hrg.step.dir;\
    }\
}

bool StorageProvider::getPossibleVolume( int, CubeSampling& globpv )
{
    if ( !possiblevolume_ )
	possiblevolume_ = new CubeSampling;

    *possiblevolume_ = storedvolume_;
    globpv.limitToWithUdf( *possiblevolume_ );

    if ( mscprov_ && mscprov_->is2D() )
    {
	globpv.hrg.stop.inl() = globpv.hrg.start.inl() = 0;
	globpv.hrg.setCrlRange( storedvolume_.hrg.crlRange() );
	return globpv.nrCrl() > 0;
    }
    else
	globpv.limitToWithUdf( *possiblevolume_ );

    const bool haveinls = globpv.hrg.inlRange().width(false) >= 0;
    const bool havecrls = globpv.hrg.crlRange().width(false) >= 0;
    return haveinls && havecrls;
}


bool StorageProvider::initMSCProvider()
{
    if ( !mscprov_ || !mscprov_->prepareWork() )
	return false;

    updateStorageReqs();
    return true;
}


#define setBufStepout( prefix ) \
{ \
    if ( ns.inl() <= prefix##bufferstepout_.inl() \
	    && ns.crl() <= prefix##bufferstepout_.crl() ) \
	return; \
\
    if ( ns.inl() > prefix##bufferstepout_.inl() ) \
	prefix##bufferstepout_.inl() = ns.inl(); \
    if ( ns.crl() > prefix##bufferstepout_.crl() ) \
	prefix##bufferstepout_.crl() = ns.crl();\
}


void StorageProvider::setReqBufStepout( const BinID& ns, bool wait )
{
    setBufStepout(req);
    if ( !wait )
	updateStorageReqs();
}


void StorageProvider::setDesBufStepout( const BinID& ns, bool wait )
{
    setBufStepout(des);
    if ( !wait )
	updateStorageReqs();
}


void StorageProvider::updateStorageReqs( bool )
{
    if ( !mscprov_ ) return;

    mscprov_->setStepout( desbufferstepout_.inl(), desbufferstepout_.crl(),
			  false );
    mscprov_->setStepout( reqbufferstepout_.inl(), reqbufferstepout_.crl(),
			  true );
}


SeisMSCProvider* StorageProvider::getMSCProvider( bool& needmscprov) const
{
    needmscprov = isondisc_;
    return mscprov_;
}


bool StorageProvider::setMSCProvSelData()
{
    if ( !mscprov_ ) return false;

    SeisTrcReader& reader = mscprov_->reader();
    if ( reader.psIOProv() ) return false;

    const bool is2d = reader.is2D();
    const bool haveseldata = seldata_ && !seldata_->isAll();

    if ( haveseldata && seldata_->type() == Seis::Table )
	return setTableSelData();

    if ( is2d )
	return set2DRangeSelData();

    if ( !desiredvolume_ )
    {
	for ( int idp=0; idp<parents_.size(); idp++ )
	{
	    if ( !parents_[idp] ) continue;

	    if ( parents_[idp]->getDesiredVolume() )
	    {
		setDesiredVolume( *parents_[idp]->getDesiredVolume() );
		break;
	    }
	}
	if ( !desiredvolume_ )
	    return true;
    }

    if ( !checkDesiredVolumeOK() )
	return false;

    CubeSampling cs;
    cs.hrg.start.inl() =
	    desiredvolume_->hrg.start.inl() < storedvolume_.hrg.start.inl() ?
	    storedvolume_.hrg.start.inl() : desiredvolume_->hrg.start.inl();
    cs.hrg.stop.inl() =
	    desiredvolume_->hrg.stop.inl() > storedvolume_.hrg.stop.inl() ?
	    storedvolume_.hrg.stop.inl() : desiredvolume_->hrg.stop.inl();
    cs.hrg.stop.crl() =
	    desiredvolume_->hrg.stop.crl() > storedvolume_.hrg.stop.crl() ?
	    storedvolume_.hrg.stop.crl() : desiredvolume_->hrg.stop.crl();
    cs.hrg.start.crl() =
	    desiredvolume_->hrg.start.crl() < storedvolume_.hrg.start.crl() ?
	    storedvolume_.hrg.start.crl() : desiredvolume_->hrg.start.crl();
    cs.zrg.start = desiredvolume_->zrg.start < storedvolume_.zrg.start ?
		    storedvolume_.zrg.start : desiredvolume_->zrg.start;
    cs.zrg.stop = desiredvolume_->zrg.stop > storedvolume_.zrg.stop ?
		     storedvolume_.zrg.stop : desiredvolume_->zrg.stop;

    reader.setSelData( haveseldata ? seldata_->clone()
				   : new Seis::RangeSelData(cs) );

    SeisTrcTranslator* transl = reader.seisTranslator();
    for ( int idx=0; idx<outputinterest_.size(); idx++ )
    {
	if ( !outputinterest_[idx] )
	    transl->componentInfo()[idx]->destidx = -1;
    }

    return true;
}


bool StorageProvider::setTableSelData()
{
    if ( !isondisc_ ) return false;	//in this case we might not use a table

    Seis::SelData* seldata = seldata_->clone();
    seldata->extendZ( extraz_ );
    SeisTrcReader& reader = mscprov_->reader();
    if ( reader.is2D() )
	seldata->setGeomID( Survey::GM().cUndefGeomID() );

    reader.setSelData( seldata );
    SeisTrcTranslator* transl = reader.seisTranslator();
    if ( !transl ) return false;
    for ( int idx=0; idx<outputinterest_.size(); idx++ )
    {
	if ( !outputinterest_[idx] && transl->componentInfo().size()>idx )
	    transl->componentInfo()[idx]->destidx = -1;
    }
    return true;
}


bool StorageProvider::set2DRangeSelData()
{
    if ( !isondisc_ ) return false;

    mDynamicCastGet(const Seis::RangeSelData*,rsd,seldata_)
    Seis::RangeSelData* seldata = rsd ? (Seis::RangeSelData*)rsd->clone()
				      : new Seis::RangeSelData( false );
    SeisTrcReader& reader = mscprov_->reader();
    Seis2DDataSet* dset = reader.dataSet();
    if ( !dset )
    {
	if ( !rsd && seldata ) delete seldata;
	return false;
    }

    if ( geomid_ != Survey::GeometryManager::cUndefGeomID() )
    {
	seldata->setGeomID( geomid_ );
	int idx = dset->indexOf( geomid_ );
	StepInterval<float> lsetzrg; StepInterval<int> trcrg;
	if ( idx >= 0 && dset->getRanges(idx,trcrg,lsetzrg) )
	{
	    if ( !checkDesiredTrcRgOK(trcrg,lsetzrg) )
		return false;
	    Interval<int> rg( 0, 0 );
	    seldata->setInlRange( rg );
	    rg.start = desiredvolume_->hrg.start.crl() < trcrg.start?
			trcrg.start : desiredvolume_->hrg.start.crl();
	    rg.stop = desiredvolume_->hrg.stop.crl() > trcrg.stop ?
			trcrg.stop : desiredvolume_->hrg.stop.crl();
	    seldata->setCrlRange( rg );
	    Interval<float> zrg;
	    zrg.start = desiredvolume_->zrg.start < lsetzrg.start ?
			lsetzrg.start : desiredvolume_->zrg.start;
	    zrg.stop = desiredvolume_->zrg.stop > lsetzrg.stop ?
			lsetzrg.stop : desiredvolume_->zrg.stop;
	    seldata->setZRange( zrg );
	}
	reader.setSelData( seldata );
    }
    else if ( !rsd && seldata )
	delete seldata;

    return true;
}


#define mInitErrMsg() \
    errmsg_ = uiString("'%1' contains no data in selected area:\n") \
		  .arg( desc_.userRef() ); \

#define mAdd2ErrMsg(varwrong,s,start,stop) \
    if ( varwrong ) \
	errmsg_.append( uiString( "%1 range is: %2-%3\n") \
		.arg( s ).arg( start ).arg( stop ) ); \

bool StorageProvider::checkDesiredVolumeOK()
{
    if ( !desiredvolume_ )
	return true;

    const bool inlwrong =
	desiredvolume_->hrg.start.inl() > storedvolume_.hrg.stop.inl()
     || desiredvolume_->hrg.stop.inl() < storedvolume_.hrg.start.inl();
    const bool crlwrong =
	desiredvolume_->hrg.start.crl() > storedvolume_.hrg.stop.crl()
     || desiredvolume_->hrg.stop.crl() < storedvolume_.hrg.start.crl();
    const bool zwrong =
	desiredvolume_->zrg.start > storedvolume_.zrg.stop
     || desiredvolume_->zrg.stop < storedvolume_.zrg.start;
    const float zstepratio = desiredvolume_->zrg.step / storedvolume_.zrg.step;
    const bool zstepwrong = zstepratio > 100 || zstepratio < 0.01;

    if ( !inlwrong && !crlwrong && !zwrong && !zstepwrong )
	return true;

    mInitErrMsg();
    mAdd2ErrMsg(inlwrong,"Inline",
		storedvolume_.hrg.start.inl(),storedvolume_.hrg.stop.inl())
    mAdd2ErrMsg(crlwrong,"Crossline",
		storedvolume_.hrg.start.crl(),storedvolume_.hrg.stop.crl())
    mAdd2ErrMsg(zwrong,"Z",storedvolume_.zrg.start,storedvolume_.zrg.stop)
    if ( zstepwrong )
	errmsg_ = "Z-Step is not correct. The maximum resampling allowed is a"
		  " factor 100. Probably the data belongs to a different"
		  " Z-Domain";
    return false;
}


bool StorageProvider::checkDesiredTrcRgOK( StepInterval<int> trcrg,
				   StepInterval<float>zrg )
{
    if ( !desiredvolume_ )
    {
	errmsg_ = uiString("internal error, '%1' "" has no desired volume\n")
		      .arg( desc_.userRef() );
	return false;
    }

    const bool trcrgwrong =
	desiredvolume_->hrg.start.crl() > trcrg.stop
     || desiredvolume_->hrg.stop.crl() < trcrg.start;
    const bool zwrong =
	desiredvolume_->zrg.start > zrg.stop
     || desiredvolume_->zrg.stop < zrg.start;

    if ( !trcrgwrong && !zwrong )
	return true;

    mInitErrMsg();
    mAdd2ErrMsg(trcrgwrong,"Trace",trcrg.start,trcrg.stop)
    mAdd2ErrMsg(zwrong,"Z",zrg.start,zrg.stop)
    return false;
}


bool StorageProvider::computeData( const DataHolder& output,
				   const BinID& relpos,
				   int z0, int nrsamples, int threadid ) const
{
    const BinID bidstep = getStepoutStep();
    const SeisTrc* trc = 0;

    if ( isondisc_)
	trc = mscprov_->get( relpos.inl()/bidstep.inl(),
			     relpos.crl()/bidstep.crl() );
    else
	trc = getTrcFromPack( relpos, 0 );

    if ( !trc || !trc->size() )
	return false;

    if ( desc_.is2D() && seldata_ && seldata_->type() == Seis::Table )
    {
	Interval<float> deszrg = desiredvolume_->zrg;
	Interval<float> poszrg = possiblevolume_->zrg;
	const float desonlyzrgstart = deszrg.start - poszrg.start;
	const float desonlyzrgstop = deszrg.stop - poszrg.stop;
	Interval<float> trcrange = trc->info().sampling.interval(trc->size());
	const float diffstart = z0*refstep_ - trcrange.start;
	const float diffstop = (z0+nrsamples-1)*refstep_ - trcrange.stop;
	bool isdiffacceptable =
	    ( (mIsEqual(diffstart,0,refstep_/100) || diffstart>0)
	      || diffstart >= desonlyzrgstart )
	 && ( (mIsEqual(diffstop,0,refstep_/100) || diffstop<0 )
	      || diffstop<=desonlyzrgstop );
	if ( !isdiffacceptable )
	    return false;
    }

    return fillDataHolderWithTrc( trc, output );
}


DataPack::FullID StorageProvider::getDPID() const
{
    const LineKey lk( desc_.getValParam(keyStr())->getStringValue(0) );
    BufferString bstring = lk.lineName();
    const char* linenm = bstring.buf();
    if ( !linenm || *linenm != '#' )
	return 0;

    DataPack::FullID fid( linenm+1 );
    return fid;
}


SeisTrc* StorageProvider::getTrcFromPack( const BinID& relpos, int relidx) const
{
    DataPack::FullID fid( getDPID() );
    DataPackRef<SeisTrcBufDataPack> stbdtp =
				    DPM( fid ).obtain( DataPack::getID(fid) );

    if ( !stbdtp )
	return 0;

    int trcidx = stbdtp->trcBuf().find(currentbid_+relpos, desc_.is2D());
    if ( trcidx+relidx >= stbdtp->trcBuf().size() || trcidx+relidx<0 )
	return 0;

    return stbdtp->trcBuf().get( trcidx + relidx );
}


bool StorageProvider::fillDataHolderWithTrc( const SeisTrc* trc,
					     const DataHolder& data ) const
{
    if ( !trc || data.isEmpty() )
	return false;

    for ( int idx=0; idx<outputinterest_.size(); idx++ )
    {
        if ( outputinterest_[idx] && !data.series(idx) )
	    return false;
    }

    const int z0 = data.z0_;
    float extrazfromsamppos = 0;
    BoolTypeSet isclass( outputinterest_.size(), true );
    if ( needinterp_ )
    {
	checkClassType( trc, isclass );
	extrazfromsamppos = getExtraZFromSampInterval( z0, data.nrsamples_ );
	const_cast<DataHolder&>(data).extrazfromsamppos_ = extrazfromsamppos;
    }

    Interval<float> trcrange = trc->info().sampling.interval(trc->size());
    trcrange.widen( 0.001f * trc->info().sampling.step );
    for ( int idx=0; idx<data.nrsamples_; idx++ )
    {
	const float curt = (float)(z0+idx)*refstep_ + extrazfromsamppos;
	int compidx = -1;
	for ( int idy=0; idy<outputinterest_.size(); idy++ )
	{
	    if ( outputinterest_[idy] )
	    {
		compidx++;
		const int compnr = desc_.is2D() ? idy : compidx;
		const float val = trcrange.includes(curt,false) ?
		   ( isclass[idy] ? trc->get(trc->nearestSample(curt), compnr)
				  : trc->getValue(curt, compnr) )
		   : mUdf(float);

		const_cast<DataHolder&>(data).series(idy)->setValue(idx,val);
	    }
	}
    }

    return true;
}


BinDataDesc StorageProvider::getOutputFormat( int output ) const
{
    if ( output>=datachar_.size() )
	return Provider::getOutputFormat( output );

    return datachar_[output];
}


BinID StorageProvider::getStepoutStep() const
{
    if ( stepoutstep_.inl() >= 0 )
	return stepoutstep_;

    BinID& sos = const_cast<StorageProvider*>(this)->stepoutstep_;
    if ( mscprov_ )
    {
	PtrMan<Seis::Bounds> bds = mscprov_->reader().getBounds();
	if ( bds )
	{
	    sos.inl() = bds->step( true );
	    sos.crl() = bds->step( false );
	}
    }
    else
	sos.inl() = sos.crl() = 1;

    return stepoutstep_;
}


void StorageProvider::adjust2DLineStoredVolume()
{
    if ( !isondisc_ || !mscprov_ ) return;

    const SeisTrcReader& reader = mscprov_->reader();
    if ( !reader.is2D() ) return;

    const Seis2DDataSet* dset = reader.dataSet();
    const int idx = dset->indexOf( geomid_ );
    StepInterval<int> trcrg;
    StepInterval<float> zrg;
    if ( idx >= 0 && dset->getRanges(idx,trcrg,zrg) )
    {
	storedvolume_.hrg.start.crl() = trcrg.start;
	storedvolume_.hrg.stop.crl() = trcrg.stop;
	storedvolume_.zrg.start = zrg.start;
	storedvolume_.zrg.stop = zrg.stop;
	storedvolume_.zrg.step = zrg.step;
    }
}


Pos::GeomID StorageProvider::getGeomID() const
{ return geomid_; }


void StorageProvider::fillDataCubesWithTrc( DataCubes* dc ) const
{
    if ( !mscprov_ ) return;
    const SeisTrc* trc = mscprov_->get(0,0);
    if ( !trc ) return;

    Interval<float> trcrange = trc->info().sampling.interval(trc->size());
    trcrange.widen( 0.001f * trc->info().sampling.step );
    const BinID bid = trc->info().binid;
    if ( !dc->includes(bid) )
	return;

    const int inlidx = dc->inlsampling_.nearestIndex( bid.inl() );
    const int crlidx = dc->crlsampling_.nearestIndex( bid.crl() );
    for ( int idz=0; idz<dc->getZSz(); idz++ )
    {
	const float curt = (float) ((dc->z0_+idz) * dc->zstep_);
	int cubeidx = -1;
	for ( int idx=0; idx<outputinterest_.size(); idx++ )
	{
	    if ( !outputinterest_[idx] )
		continue;

	    cubeidx++;
	    if ( cubeidx >= dc->nrCubes() && !dc->addCube(mUdf(float)) )
		continue;

	    if ( !trcrange.includes(curt,false) )
		continue;

	    //the component index inthe trace is depending on outputinterest_,
	    //thus is the same as cubeidx
	    const float val = trc->getValue( curt, cubeidx );
	    dc->setValue( cubeidx, inlidx, crlidx, idz, val );
	}
    }
}


void StorageProvider::checkClassType( const SeisTrc* trc,
				      BoolTypeSet& isclass ) const
{
    int idx = 0;
    bool foundneed = true;
    while ( idx<trc->size() && foundneed )
    {
	foundneed = false;
	int compidx = -1;
	for ( int ido=0; ido<outputinterest_.size(); ido++ )
	{
	    if ( outputinterest_[ido] )
	    {
		compidx++;
		if ( isclass[ido] )
		{
		    foundneed = true;
		    const float val  = trc->get( idx, compidx );
		    if ( !holdsClassValue( val) )
			isclass[ido] = false;
		}
	    }
	}
	idx++;
    }
}


bool StorageProvider::compDistBetwTrcsStats( bool force )
{
    if ( !mscprov_ ) return false;
    if ( ls2ddata_ && ls2ddata_->areStatsComputed() ) return true;

    const SeisTrcReader& reader = mscprov_->reader();
    if ( !reader.is2D() ) return false;

    const Seis2DDataSet* dset = reader.dataSet();
    if ( !dset ) return false;

    if ( ls2ddata_ ) delete ls2ddata_;
    ls2ddata_ = new PosInfo::LineSet2DData();
    for ( int idx=0; idx<dset->nrLines(); idx++ )
    {
	PosInfo::Line2DData& linegeom = ls2ddata_->addLine(dset->lineName(idx));
	const Survey::Geometry* geom =
		Survey::GM().getGeometry( dset->geomID(idx) );
	mDynamicCastGet( const Survey::Geometry2D*, geom2d, geom );
	if ( !geom2d ) continue;

	linegeom = geom2d->data();
	if ( linegeom.positions().isEmpty() )
	{
	    ls2ddata_->removeLine( dset->lineName(idx) );
	    return false;
	}
    }

    ls2ddata_->compDistBetwTrcsStats();
    return true;
}


void StorageProvider::getCompNames( BufferStringSet& nms ) const
{
    updateDescAndGetCompNms( desc_, &nms );
}


bool StorageProvider::useInterTrcDist() const
{
    if ( useintertrcdist_ )
	return true;

    if ( getDesc().is2D() )
    {
	const LineKey lk( desc_.getValParam(keyStr())->getStringValue(0) );
	const BufferString attrnm = lk.attrName();
	const MultiID key( lk.lineName() );
	PtrMan<IOObj> ioobj = IOM().get( key );
	SeisTrcReader rdr( ioobj );
	if ( rdr.ioObj() && rdr.dataSet() )
	{
	    int lineidx = rdr.dataSet()->indexOf( geomid_ );
	    if ( lineidx<0 ) return false;
	    IOPar linepars = rdr.dataSet()->getInfo( lineidx );
	    FixedString fname = linepars.find( sKey::FileName() );
	    FilePath fp( fname );
	    if ( !fp.isAbsolute() )
	        fp.setPath( IOObjContext::getDataDirName( IOObjContext::Seis) );
	    PtrMan<CBVSSeisTrcTranslator> tmptransl =
		    CBVSSeisTrcTranslator::make( fp.fullPath(), true, true, 0 );
	    if ( !tmptransl )
	        return false;

	    BufferStringSet compnms;
	    tmptransl->getComponentNames( compnms );
	    if ( compnms.size()>=2
		    && compnms.get(1)== BufferString(Desc::sKeyLineDipComp()) )
	    {
		const_cast<Attrib::StorageProvider*>(this)
						      ->useintertrcdist_ = true;
		return useintertrcdist_;
	    }
	}
    }

    return false;
}


float StorageProvider::getDistBetwTrcs( bool ismax, const char* linenm ) const
{
    if ( !ls2ddata_ )
	const_cast<StorageProvider*>(this)->compDistBetwTrcsStats( true );

    return ls2ddata_ ? ls2ddata_->getDistBetwTrcs( ismax, linenm )
		     : mUdf(float);
}

}; // namespace Attrib
