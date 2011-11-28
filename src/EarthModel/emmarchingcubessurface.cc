/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Aug 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: emmarchingcubessurface.cc,v 1.26 2011-11-28 21:51:02 cvsyuancheng Exp $";

#include "emmarchingcubessurface.h"

#include "arrayndimpl.h"
#include "ascstream.h"
#include "datainterp.h"
#include "datachar.h"
#include "executor.h"
#include "embodytr.h"
#include "embodyoperator.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "marchingcubes.h"
#include "randcolor.h"
#include "streamconn.h"
#include "separstr.h"

#include "emmanager.h"

namespace EM 
{

class MarchingCubesSurfaceReader : public Executor
{
public:
    ~MarchingCubesSurfaceReader()
    {
	delete conn_;
	delete int32interpreter_;
	delete exec_;
    }

    MarchingCubesSurfaceReader( MarchingCubesSurface& surface, Conn* conn )
	: Executor( "Surface Loader" )
	, conn_( conn )
	, int32interpreter_( 0 )
	, exec_( 0 )
	, surface_( surface )
    {
	if ( !conn_ || !conn_->forRead() )
	{
	    errmsg_ = "Cannot open connection";
	    return;
	}

	std::istream& strm = ((StreamConn*)conn_)->iStream();
	ascistream astream( strm );
	if ( !astream.isOfFileType( sFileType() ) &&
	     !astream.isOfFileType( sOldFileType() ) )
	{
	    errmsg_ = "Invalid filetype";
	    return;
	}

	IOPar par( astream );

	int32interpreter_ =
	    DataInterpreter<int>::create( par, sKeyInt32DataChar(), true );

	SamplingData<int> inlsampling;
	SamplingData<int> crlsampling;
	SamplingData<float> zsampling;
	if ( !par.get( sKeyInlSampling(),inlsampling.start,inlsampling.step) ||
	     !par.get( sKeyCrlSampling(),crlsampling.start,crlsampling.step) ||
	     !par.get( sKeyZSampling(),zsampling.start,zsampling.step ) )
	{
	    errmsg_ = "Invalid filetype";
	    return;
	}

	surface.setInlSampling( inlsampling );
	surface.setCrlSampling( crlsampling );
	surface.setZSampling( zsampling );

	Color col;
	if ( par.get( sKey::Color, col ) )
	    surface.setPreferredColor( col );

	exec_ = surface.surface().readFrom(strm,int32interpreter_);
    }

    static const char* sKeyInt32DataChar()	{ return "Int32 format"; }
    static const char* sKeyInlSampling()	{ return "Inline sampling"; }
    static const char* sKeyCrlSampling()	{ return "Crossline sampling"; }
    static const char* sKeyZSampling()		{ return "Z sampling"; }
    static const char* sFileType()
    { return MarchingCubesSurface::typeStr(); }

    static const char* sOldFileType() { return "MarchingCubesSurface"; }

    int	nextStep()
    {
	if ( !exec_ ) return ErrorOccurred();
	const int res = exec_->doStep();
	if ( res==Finished() )
	{
	    surface_.setFullyLoaded( true );
	    surface_.resetChangedFlag();
	}
	return res;
    }

    od_int64	totalNr() const { return exec_ ? exec_->totalNr() : -1; }
    od_int64	nrDone() const { return exec_ ? exec_->nrDone() : -1; }
    const char*	nrDoneText() const { return exec_ ? exec_->nrDoneText() : 0; }
    const char*	message() const
    {
	return errmsg_.isEmpty()
	    ? "Loading body"
	    : errmsg_.str();
     }

protected:

    MarchingCubesSurface&	surface_;
    Executor*			exec_;
    DataInterpreter<od_int32>*	int32interpreter_;
    Conn*			conn_;
    FixedString			errmsg_;
};



class MarchingCubesSurfaceWriter : public Executor
{
public:
~MarchingCubesSurfaceWriter()
{
    delete conn_;
    delete exec_;
}

MarchingCubesSurfaceWriter( MarchingCubesSurface& surface,
			    Conn* conn, bool binary )
    : Executor( "Surface Writer" )
    , conn_( conn )
    , exec_( 0 )
    , surface_( surface )
{
    if ( !conn_ || !conn_->forWrite() )
    {
	errmsg_ = "Cannot open connection";
	return;
    }

    std::ostream& strm = ((StreamConn*)conn_)->oStream();
    ascostream astream( strm );
    astream.putHeader( MarchingCubesSurfaceReader::sFileType());

    IOPar par;
    if ( binary )
    {
	BufferString dcs;
	od_int32 dummy;
	DataCharacteristics(dummy).toString( dcs.buf() );
	par.set(MarchingCubesSurfaceReader::sKeyInt32DataChar(),
		dcs.buf() );
    }

    par.set( MarchingCubesSurfaceReader::sKeyInlSampling(),
	     surface.inlSampling().start,surface.inlSampling().step);
    par.set( MarchingCubesSurfaceReader::sKeyCrlSampling(),
	     surface.crlSampling().start,surface.crlSampling().step);
    par.set( MarchingCubesSurfaceReader::sKeyZSampling(),
	     surface.zSampling().start,surface.zSampling().step );

    par.set( sKey::Color, surface.preferredColor() );

    par.putTo( astream );

    exec_ = surface.surface().writeTo( strm, binary );
}

int nextStep()
{
    if ( !exec_ ) return ErrorOccurred();
    const int res = exec_->doStep();
    if ( !res )
	surface_.resetChangedFlag();

    return res;
}

od_int64 totalNr() const { return exec_ ? exec_->totalNr() : -1; }
od_int64 nrDone() const { return exec_ ? exec_->nrDone() : -1; }
const char* nrDoneText() const { return exec_ ? exec_->nrDoneText() : 0; }
const char*       message() const
{
    return errmsg_.isEmpty()
	? "Loading body"
	: errmsg_.str();
}


protected:

    Executor*			exec_;
    Conn*			conn_;
    FixedString			errmsg_;
    MarchingCubesSurface&	surface_;
};


void MarchingCubesSurface::initClass()
{
    SeparString sep( 0, FactoryBase::cSeparator() );
    sep += typeStr();
    sep += MarchingCubesSurfaceReader::sOldFileType();
    EMOF().addCreator( create, sep.buf() );
}


EMObject* MarchingCubesSurface::create( EMManager& emm ) \
{
    EMObject* obj = new MarchingCubesSurface( emm );
    if ( !obj ) return 0;
    obj->ref();
    emm.addObject( obj );
    obj->unRefNoDelete();
    return obj;
}


const char* MarchingCubesSurface::typeStr()
{ return mcEMBodyTranslator::sKeyUserName(); }


const char* MarchingCubesSurface::getTypeStr() const
{ return typeStr(); }


void MarchingCubesSurface::setNewName()
{
    static int objnr = 1;
    BufferString nm( "<New ", typeStr(), " " );
    nm.add( objnr++ ).add( ">" );
    setName( nm );
}


MarchingCubesSurface::MarchingCubesSurface( EMManager& emm )
    : EMObject( emm )
    , mcsurface_( new ::MarchingCubesSurface )
    , operator_( 0 )					      
{
    mcsurface_->ref();
    setPreferredColor( getRandomColor( false ) );
}


MarchingCubesSurface::~MarchingCubesSurface()
{
    mcsurface_->unRef();
    delete operator_;
}


void MarchingCubesSurface::refBody()
{
    EMObject::ref();
}


void MarchingCubesSurface::unRefBody()
{
    EMObject::unRef();
}


Executor* MarchingCubesSurface::loader()
{
    PtrMan<IOObj> ioobj = IOM().get( multiID() );
    if ( !ioobj )
	return 0;

    Conn* conn = ioobj->getConn( Conn::Read );
    if ( !conn )
	return 0;

    return new MarchingCubesSurfaceReader( *this, conn );
}


Executor* MarchingCubesSurface::saver()
{
    return saver( 0 );
}


Executor* MarchingCubesSurface::saver( IOObj* inpioobj )
{
    PtrMan<IOObj> myioobj = 0;
    IOObj* ioobj = 0;
    if ( inpioobj )
	ioobj = inpioobj;
    else
    {
	myioobj = IOM().get( multiID() );
	ioobj = myioobj;
    }

    if ( !ioobj )
	return 0;
    
    Conn* conn = ioobj->getConn( Conn::Write );
    if ( !conn )
	return 0;

    return new MarchingCubesSurfaceWriter( *this, conn, true );
}


bool MarchingCubesSurface::isEmpty() const
{ return mcsurface_->isEmpty(); }


const IOObjContext& MarchingCubesSurface::getIOObjContext() const
{
    static IOObjContext* res = 0;
    if ( !res )
    {
	res = new IOObjContext(EMBodyTranslatorGroup::ioContext() );
	res->deftransl = mcEMBodyTranslator::sKeyUserName();
	res->toselect.allowtransls_ = mcEMBodyTranslator::sKeyUserName();
    }

    return *res; 
}


bool MarchingCubesSurface::useBodyPar( const IOPar& par )
{ return EMObject::usePar( par ); }


void MarchingCubesSurface::fillBodyPar( IOPar& par ) const
{ EMObject::fillPar( par ); }


void MarchingCubesSurface::setBodyOperator( BodyOperator* op )
{
    if ( operator_ ) delete operator_;
    operator_ = op;
}


void MarchingCubesSurface::createBodyOperator()
{
    if ( operator_ ) delete operator_;
    operator_ = new BodyOperator();
}


bool MarchingCubesSurface::regenerateMCBody( TaskRunner* tr )
{
    if ( !operator_ || !mcsurface_ ) 
	return false;

    ImplicitBody* body = 0;
    if ( !operator_->createImplicitBody( body, tr ) || !body )
	return false;

    setInlSampling( body->inlsampling_ );
    setCrlSampling( body->crlsampling_ );
    setZSampling( body->zsampling_ );

    return mcsurface_->setVolumeData( 0, 0, 0, *body->arr_, body->threshold_ );
}


bool MarchingCubesSurface::getBodyRange( CubeSampling& cs )
{
    Interval<int> inlrg, crlrg, zrg;
    if ( !mcsurface_->models_.getRange( 0, inlrg ) ||
	 !mcsurface_->models_.getRange( 1, crlrg ) ||
	 !mcsurface_->models_.getRange( 2, zrg ) )
	return false;

    cs.hrg.start = BinID( inlsampling_.atIndex(inlrg.start), 
	    	           crlsampling_.atIndex(crlrg.start) );
    cs.hrg.stop = BinID( inlsampling_.atIndex(inlrg.stop),
	    		 crlsampling_.atIndex(crlrg.stop) );
    cs.hrg.step = BinID( inlsampling_.step, crlsampling_.step );
    cs.zrg.start = zsampling_.atIndex( zrg.start );
    cs.zrg.step = zsampling_.step;
    cs.zrg.stop = zsampling_.atIndex( zrg.stop );

    return true;
}


ImplicitBody* MarchingCubesSurface::createImplicitBody( TaskRunner* t,
							bool smooth ) const
{
    if ( !mcsurface_ )
    {
	if ( operator_ )
    	{
    	    ImplicitBody* body = 0;
    	    if ( operator_->createImplicitBody(body, t) && body )
    		return body;
    	}

	return 0;
    }

    mcsurface_->modelslock_.readLock();
    Interval<int> inlrg, crlrg, zrg;
    if ( !mcsurface_->models_.getRange( 0, inlrg ) ||
	!mcsurface_->models_.getRange( 1, crlrg ) ||
	!mcsurface_->models_.getRange( 2, zrg ) )
    {
	mcsurface_->modelslock_.readUnLock();
	return 0;
    }

    const int inlsz = inlrg.width()+1;
    const int crlsz = crlrg.width()+1;
    const int zsz = zrg.width()+1;


    mDeclareAndTryAlloc(Array3D<int>*,intarr,Array3DImpl<int>(inlsz,crlsz,zsz));
    if ( !intarr )
    {
	mcsurface_->modelslock_.readUnLock();
	return 0;
    }

    mDeclareAndTryAlloc( ImplicitBody*, res, ImplicitBody );
    if ( !res )
    {
	mcsurface_->modelslock_.readUnLock();
	delete intarr;
	return 0;
    }

    Array3D<float>* arr = new Array3DConv<float,int>(intarr);
    if ( !arr )
    {
	mcsurface_->modelslock_.readUnLock();
	delete intarr;
	delete res;
	return 0;
    }

    res->arr_ = arr;
    res->inlsampling_.start = inlsampling_.atIndex( inlrg.start );
    res->inlsampling_.step = inlsampling_.step;

    res->crlsampling_.start = crlsampling_.atIndex( crlrg.start );
    res->crlsampling_.step = crlsampling_.step;

    res->zsampling_.start = zsampling_.atIndex( zrg.start );
    res->zsampling_.step = zsampling_.step;

    MarchingCubes2Implicit m2i( *mcsurface_, *intarr,
				inlrg.start, crlrg.start, zrg.start, !smooth );
    if ( (t && !t->execute( m2i ) ) || (!t && !m2i.execute() ) )
    {
	delete res;
	mcsurface_->modelslock_.readUnLock();
	return 0;
    }

    res->threshold_ = m2i.threshold();
    mcsurface_->modelslock_.readUnLock();

    return res;
}


bool MarchingCubesSurface::setSurface( ::MarchingCubesSurface* ns )
{
    if ( !ns )
	return false;

    mcsurface_->unRef();
    mcsurface_ = ns;
    mcsurface_->ref();

    return true;
}


void MarchingCubesSurface::setInlSampling( const SamplingData<int>& s )
{ inlsampling_ = s; }


void MarchingCubesSurface::setCrlSampling( const SamplingData<int>& s )
{ crlsampling_ = s; }


void MarchingCubesSurface::setZSampling( const SamplingData<float>& s )
{ zsampling_ = s; }


}; // namespace
