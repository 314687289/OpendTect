/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : March 2007
-*/

static const char* rcsID = "$Id: prestackeventio.cc,v 1.3 2008-09-29 13:23:48 cvsbert Exp $";

#include "prestackeventio.h"

#include "ascstream.h"
#include "binidvalset.h"
#include "color.h"
#include "datachar.h"
#include "datainterp.h"
#include "dirlist.h"
#include "filegen.h"
#include "filepath.h"
#include "iopar.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "prestackevents.h"
#include "prestackeventtransl.h"
#include "rowcol.h"
#include "samplingdata.h"
#include "separstr.h"
#include "survinfo.h"
#include "streamconn.h"
#include "offsetazimuth.h"


#define mHeaderEventSize (sizeof(int)+sizeof(int)+sizeof(int))


namespace PreStack
{

//Internal classes

class EventPatchFileHeader
{
public:
			EventPatchFileHeader();
			~EventPatchFileHeader();

    void		setInterpreters(const DataInterpreter<int>*,
					const DataInterpreter<short>*);

    bool		fromStream(std::istream&);
    bool		toStream(std::ostream&, bool binary );

    int			nrEvents() const;
    void		setNrEvents(int);

    BinID		getBinID(int) const;
    void		setBinID(int,const BinID&);

    int			getOffset(int) const;
    void		setOffset(int,int);

protected:

    int					nrevents_;
    char*				buffptr_;

    const DataInterpreter<int>*		int32interpreter_;
    const DataInterpreter<short>*	int16interpreter_;
};


class EventPatchReader : public SequentialTask
{
public:
			EventPatchReader(Conn*,EventManager*);
			~EventPatchReader();

    void		getPositions(BinIDValueSet& bidset) const;
    const HorSampling&	getRange() const 	{ return filebbox_; }

    void		setSelection(const BinIDValueSet*);
    void		setSelection(const HorSampling*);

    bool		hasDataInRange() const;
    const char*		errMsg() const;

    int			nextStep();

protected:

    bool	isWanted(const BinID&) const;
    int		readInt16(std::istream&) const;
    int		readInt32(std::istream&) const;
    int		readUInt8(std::istream&) const;
    float	readFloat(std::istream&) const;

    DataInterpreter<int>*	int32interpreter_;
    DataInterpreter<int>*	int16interpreter_;
    DataInterpreter<float>*	floatinterpreter_;

    EventPatchFileHeader	fileheader_;
    int				headeridx_;

    EventManager*		eventmanager_;
    Conn*			conn_;
    HorSampling			filebbox_;
    const HorSampling*		horsel_;
    const BinIDValueSet*	bidvalsel_;
    BufferString		errmsg_;

    bool			readeventtype_;
    bool			readdip_;
};


class EventPatchWriter : public SequentialTask
{
public:
		EventPatchWriter(const char* filename, EventManager&);
		~EventPatchWriter();

    void	setReader(EventPatchReader*);

    void	setSelection(const HorSampling&);
    void	setBinary(bool yn)			{ binary_ = yn; }
    const char*	errMsg() const;

    int		nextStep();

protected:

    bool	writeInt16(std::ostream&,int val,const char*) const;
    bool	writeInt32(std::ostream&,int val,const char*) const;
    bool	writeUInt8(std::ostream&,int val,const char*) const;
    bool	writeFloat(std::ostream&,float val,const char*) const;

    EventPatchReader*		reader_;
    EventPatchFileHeader	fileheader_;
    int				headeridx_;

    bool			binary_;
    EventManager&		eventmanager_;
    BufferString		filename_;
    StreamConn*			conn_;
    HorSampling			horsel_;
    int				fileheaderoffset_;
    BufferString		errmsg_;
};


//EventReader
EventReader::EventReader( IOObj* ioobj, EventManager* events )
    : Executor( "Reading Pre-stack events" )
    , eventmanager_( events )
    , ioobj_( ioobj )
    , bidsel_( 0 )
    , horsel_( 0 )
{}


EventReader::~EventReader()
{
    delete ioobj_;
    deepErase( patchreaders_ );
}


bool EventReader::getPositions( BinIDValueSet& bidset ) const
{
    for ( int idx=patchreaders_.size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( const EventPatchReader*, reader, patchreaders_[idx] );
	reader->getPositions( bidset );
    }

    return true;
}


bool EventReader::getBoundingBox( Interval<int>& inlrg,
				  Interval<int>& crlrg ) const
{
    for ( int idx=0; idx<patchreaders_.size(); idx++ )
    {
	mDynamicCastGet( const EventPatchReader*, reader, patchreaders_[idx] );
	const HorSampling& hrg = reader->getRange();
	if ( !idx )
	{
	    inlrg.start = hrg.start.inl;
	    inlrg.stop = hrg.stop.inl;
	    crlrg.start = hrg.start.crl;
	    crlrg.stop = hrg.stop.crl;
	}
	else
	{
	    inlrg.include( hrg.start.inl );
	    inlrg.include( hrg.stop.inl );
	    crlrg.include( hrg.start.crl );
	    crlrg.include( hrg.stop.crl );
	}
    }

    return patchreaders_.size();
}


int EventReader::encodeEventType( VSEvent::Type et )
{
    if ( et==VSEvent::Min )
	return 1;
    if ( et==VSEvent::Max )
	return 2;

    return 0;
}


VSEvent::Type EventReader::decodeEventType( int et )
{
    if ( et==1 )
	return VSEvent::Min;
    if ( et==2 )
	return VSEvent::Max;

    return VSEvent::None;
}


void EventReader::setSelection( const BinIDValueSet* bivs )
{ bidsel_ = bivs; }


void EventReader::setSelection( const HorSampling* hs )
{ horsel_ = hs; }


int EventReader::nextStep()
{
    if ( !eventmanager_ ) return Finished;

    if ( !patchreaders_.size() )
    {
	if ( !prepareWork() )
	    return ErrorOccurred;

	const BufferString fnm( ioobj_->fullUserExpr(true) );
	if ( !readHorizonIDs( fnm.buf() ) )
	    return false;

	return patchreaders_.size() ? MoreToDo : Finished;
    }

    const int res = patchreaders_[0]->doStep();
    if ( res<0 ) return ErrorOccurred;
    if ( !res )
    {
	delete patchreaders_.remove( 0 );
	if ( patchreaders_.size() )
	    return MoreToDo;
        
	return	Finished;
    }

    return MoreToDo;
}


bool EventReader::prepareWork()
{
    const BufferString fnm( ioobj_->fullUserExpr(true) );
    if ( !File_isDirectory(fnm.buf()) )
	return false;

    BufferString mask = "*.";
    mask += PSEventTranslatorGroup::sDefExtension();

    const DirList dirlist( fnm.buf(), DirList::FilesOnly, mask.buf() );
    for ( int idx=0; idx<dirlist.size(); idx++ )
    {
	bool usefile = true;
	if ( bidsel_ || horsel_ )
	{
	    usefile = false;
	    SeparString sepstr( dirlist[idx]->buf(), '_' );
	    HorSampling filehrg;
	    if ( getFromString( filehrg.start.inl, sepstr[0], -1 ) &&
		 getFromString( filehrg.stop.inl, sepstr[1], -1 ) &&
		 getFromString( filehrg.start.crl, sepstr[2], -1 ) &&
		 getFromString( filehrg.stop.crl, sepstr[3], -1 ) )
	    {
		HorSampling dummy;
		if ( horsel_ && horsel_->getInterSection(filehrg,dummy))
		    usefile = true;

		if ( !usefile && bidsel_ )
		{
		    BinIDValueSet::Pos pos;
		    while ( bidsel_->next(pos,true) )
		    {
			if ( filehrg.includes( bidsel_->getBinID(pos) ))
			{
			    usefile = true;
			    break;
			}
		    }
		}
	    }
	}

	if ( !usefile )
	    continue;

	addReader( dirlist.fullPath(idx) );
    }

    return true;
}



void EventReader::addReader( const char* fnm )
{
    Conn* conn = new StreamConn( fnm, Conn::Read );

    EventPatchReader* reader = new EventPatchReader( conn, eventmanager_ );

    if ( reader->errMsg() )
    {
	delete reader;
	return;
    }

    reader->setSelection( bidsel_ );
    reader->setSelection( horsel_ );

    if ( !reader->hasDataInRange() )
    {
	delete reader;
    }
    else
    {
	patchreaders_ += reader;
    }
}


bool EventReader::readHorizonIDs(const char* fnm)
{
    if ( !eventmanager_ )
	return false;

    if ( eventmanager_->getHorizonIDs().size() )
	return true; //The version in mem is newer - keep it

    FilePath horidfnm;
    horidfnm.setPath( fnm );
    horidfnm.setFileName( EventReader::sHorizonFileName() );

    IOPar par;
    if ( !par.read( horidfnm.fullPath().buf(),
		    EventReader::sHorizonFileType(), true ) )
	return false;

    int nrhors, nexthor;
    if ( !par.get( EventReader::sKeyNrHorizons(), nrhors ) ||
	 !par.get( EventReader::sKeyNextHorizonID(), nexthor ) )
	return false;

    NotifyStopper stopper( eventmanager_->change );
    for ( int idx=0; idx<nrhors; idx++ )
    {
	BufferString key = idx;
	PtrMan<IOPar> horpar = par.subselect( key.buf() );
	if ( !horpar )
	    return false;

	int id;
	if ( !horpar->get( EventReader::sKeyHorizonID(), id ) )
	    return false;

	eventmanager_->addHorizon( id );

	MultiID emref;
	if ( horpar->get( EventReader::sKeyHorizonRef(), emref ) )
	    eventmanager_->setHorizonEMReference( id, emref );
    }

    Color col = Color(255,0,0); //Todo: Make mandatory to have color
    par.get( sKey::Color, col );
    eventmanager_->setColor( col );

    eventmanager_->setNextHorizonID( nexthor );
    eventmanager_->resetChangedFlag( true );

    EventManager::DipSource primarydipsource;
    EventManager::DipSource secondarydipsource;
    BufferString dipsourcestr;
    if ( par.get( sKeyPrimaryDipSource(), dipsourcestr ) )
    {
	if ( primarydipsource.use( dipsourcestr.buf() ) )
	    return false;

	if ( !par.get( sKeySecondaryDipSource(), dipsourcestr ) ||
	     !secondarydipsource.use( dipsourcestr.buf() ) )
	    return false;
    }

    eventmanager_->setDipSource( primarydipsource, true );
    eventmanager_->setDipSource( secondarydipsource, false );

    return true;
}


//EventWriter
EventWriter::EventWriter( IOObj* ioobj, EventManager& events )
    : Executor( "Writing Pre-stack events" )
    , eventmanager_( events )
    , ioobj_( ioobj )
{}


EventWriter::~EventWriter()
{
    delete ioobj_;
    deepErase( patchwriters_ );
}


int EventWriter::nextStep()
{
    if ( !patchwriters_.size() )
    {
	SamplingData<int> inlsampling;
	SamplingData<int> crlsampling;
	IOPar& pars = ioobj_->pars();
	if ( !pars.get(sKeyISamp(), inlsampling.start, inlsampling.step )
	  || !pars.get(sKeyCSamp(), crlsampling.start, crlsampling.step ) )
	{
	    const StepInterval<int> inlrg = SI().inlRange(false);
	    const StepInterval<int> crlrg = SI().crlRange(false);
	    inlsampling.start = inlrg.start; inlsampling.step = inlrg.step*25;
	    crlsampling.start = crlrg.start; crlsampling.step = crlrg.step*25;
	    pars.set( sKeyISamp(), inlsampling.start, inlsampling.step );
	    pars.set( sKeyCSamp(), crlsampling.start, crlsampling.step );

	    if ( !IOM().commitChanges( *ioobj_ ) )
	    {
		errmsg_ = "Cannot write to object database";
		return ErrorOccurred;
	    }
	}

	const BufferString fnm( ioobj_->fullUserExpr(true) );
	if ( !File_exists( fnm.buf() ) || !File_isDirectory( fnm.buf() ) )
	{
	    if ( !File_isDirectory( fnm.buf() ) )
	    {
		if ( !File_remove( fnm.buf(), mFile_NotRecursive ) )
		{
		    errmsg_ = "Cannot remove ";
		    errmsg_ += fnm;
		    return ErrorOccurred;
		}
	    }

	    if ( !File_createDir(fnm.buf(), 0) )
	    {
		errmsg_ = "Cannot create directory ";
		errmsg_ += fnm;
		return ErrorOccurred;
	    }
	}

	eventmanager_.cleanUp( true );

	if ( !writeHorizonIDs( fnm.buf() ) )
	    return ErrorOccurred;

	const MultiDimStorage<EventSet*>& evstor = eventmanager_.getStorage();
	int pos[] = { -1, -1 };
	TypeSet<RowCol> rcols;
	while ( evstor.next( pos, true ) )
	{
	    BinID bid;
	    evstor.getPos( pos, bid );
	    EventSet* ge = evstor.getRef( pos, 0 );

	    if ( !ge->ischanged_ )
		continue;

	    const RowCol rc( (bid.inl-inlsampling.start)/inlsampling.step,
		    	     (bid.crl-crlsampling.start)/crlsampling.step );

	    if ( rcols.indexOf( rc )==-1 )
		rcols += rc;
	}

	if ( !rcols.size() )
	    return Finished;

	HorSampling hrg( true );

	for ( int idx=0; idx<rcols.size(); idx++ )
	{
	    const RowCol& rc( rcols[idx] );
	    hrg.start.inl = inlsampling.atIndex( rc.row );
	    hrg.stop.inl = inlsampling.atIndex( rc.row+1 ) - hrg.step.inl;
	    hrg.start.crl = crlsampling.atIndex( rc.col );
	    hrg.stop.crl = crlsampling.atIndex( rc.col+1 ) - hrg.step.crl;

	    SeparString filenamebase( 0, '_' );
	    filenamebase += hrg.start.inl;
	    filenamebase += hrg.stop.inl;
	    filenamebase += hrg.start.crl;
	    filenamebase += hrg.stop.crl;


	    FilePath filename;
	    filename.setPath( fnm.buf() );
	    filename.setFileName( filenamebase.buf() );
	    filename.setExtension( PSEventTranslatorGroup::sDefExtension() );

	    EventPatchWriter* writer =
		new EventPatchWriter( filename.fullPath().buf(), eventmanager_);

	    StreamConn* conn = new StreamConn( filename.fullPath().buf(),
		    			       Conn::Read );
	    if ( conn && conn->forRead() )
	    {
		EventPatchReader* reader =
		    new EventPatchReader( conn, &eventmanager_ );

		if ( reader->errMsg() )
		{
		    delete reader;
		    return ErrorOccurred;
		}

		writer->setReader( reader );
	    }
	    else
	    {
		delete conn;
	    }

	    writer->setSelection( hrg );
	    patchwriters_ += writer;
	}

	return patchwriters_.size() ? MoreToDo : Finished;
    }

    const int res = patchwriters_[0]->doStep();
    if ( res<0 ) return ErrorOccurred;
    if ( !res )
    {
	delete patchwriters_.remove( 0 );
	if ( patchwriters_.size() )
	    return MoreToDo;
        
	eventmanager_.resetChangedFlag( true );
	return	Finished;
    }

    return MoreToDo;
}


const char* EventWriter::errMsg() const
{ return errmsg_[0] ? errmsg_.buf() : 0; }


bool EventWriter::writeHorizonIDs( const char* fnm ) const
{
    IOPar par;
    par.set( EventReader::sKeyNrHorizons(),
	     eventmanager_.getHorizonIDs().size());
    par.set( EventReader::sKeyNextHorizonID(),
	     eventmanager_.nextHorizonID(false) );

    for ( int idx=eventmanager_.getHorizonIDs().size()-1; idx>=0; idx-- )
    {
	BufferString key = idx;
	IOPar horpar;
	const int id =  eventmanager_.getHorizonIDs()[idx];
	horpar.set( EventReader::sKeyHorizonID(), id );

	par.mergeComp( horpar, key.buf() );
    }

    BufferString dipsource;
    eventmanager_.getDipSource( true ).fill( dipsource );
    if ( !dipsource.isEmpty() )
	par.set( EventReader::sKeyPrimaryDipSource(), dipsource.buf() );

    eventmanager_.getDipSource( false ).fill( dipsource );
    if ( !dipsource.isEmpty() )
	par.set( EventReader::sKeySecondaryDipSource(), dipsource.buf() );

    FilePath horidfnm;
    horidfnm.setPath( fnm );
    horidfnm.setFileName( EventReader::sHorizonFileName() );

    par.set( sKey::Color, eventmanager_.getColor() );

    return par.write( horidfnm.fullPath().buf(),
	              EventReader::sHorizonFileType() );
}


//Duplicator


EventDuplicator::EventDuplicator( IOObj* from, IOObj* to )
    : Executor( "Copying Prestack events" )
    , from_( from )
    , to_( to )
    , totalnr_( -1 )
{
    const BufferString fromnm( from_->fullUserExpr(true) );
    if ( !File_isDirectory(fromnm.buf()) )
    {
	errmsg_ = "Input file ";
	errmsg_ += fromnm.buf();
	errmsg_ += " is not a directory.";
	return;
    }

    const BufferString tonm( to_->fullUserExpr(true) );
    if ( File_exists( tonm.buf() ) )
    {
	File_remove( tonm.buf(), mFile_Recursive );
	if ( File_exists( tonm.buf() ) )
	{
	    errmsg_ = "Cannot overwrite ";
	    errmsg_ += tonm.buf();
	    errmsg_ += ".";
	    return;
	}
    }

    if ( !File_createDir( tonm.buf(), 0 ) )
    {
	errmsg_ = "Cannot create ";
	errmsg_ += tonm.buf();
	errmsg_ += ".";
	return;
    }

    const DirList dirlist( fromnm.buf(), DirList::FilesOnly, "*" );
    for ( int idx=0; idx<dirlist.size(); idx++ )
	filestocopy_.add( dirlist.fullPath( idx ) );

    totalnr_ = filestocopy_.size();

    return;
}



EventDuplicator::~EventDuplicator()
{
    if ( filestocopy_.size() ) //premature exit
	errorCleanup();

    delete from_;
    delete to_;
}


int EventDuplicator::nextStep()
{
    if ( errMsg() ) //Catch error in prepareWork
	return ErrorOccurred;	

    if ( !filestocopy_.size() )
	return Finished;

    const int idx = filestocopy_.size()-1;

    const BufferString tonm( to_->fullUserExpr(true) );
    if ( !File_isDirectory(tonm.buf()) )
	return ErrorOccurred;

    FilePath targetfile( filestocopy_[idx]->buf() );
    targetfile.setPath( tonm.buf() );

    message_ = "Copying ";
    message_ += targetfile.fileName();
    message_ += ".";

    if ( !File_copy( filestocopy_[idx]->buf(), targetfile.fullPath().buf(),
			mFile_NotRecursive) )
    {
	errmsg_ = "Cannot copy ";
	errmsg_ = filestocopy_[idx]->buf();
	errmsg_ += " to ";
	errmsg_ += targetfile.fullPath().buf();
	errorCleanup();
	return ErrorOccurred;
    }

    filestocopy_.remove( idx );
    return MoreToDo;
}


void EventDuplicator::errorCleanup()
{

    const BufferString tonm( to_->fullUserExpr(true) );
    if ( File_exists( tonm.buf() ) )
	File_remove( tonm.buf(), mFile_Recursive );
}


//EventPatchFileHeader


EventPatchFileHeader::EventPatchFileHeader()
    : buffptr_( 0 )
    , int32interpreter_( 0 )
    , int16interpreter_( 0 )
    , nrevents_( 0 )
{}


EventPatchFileHeader::~EventPatchFileHeader()
{ delete [] buffptr_; }


bool EventPatchFileHeader::fromStream( std::istream& strm )
{
    int nrevents;
    if ( int16interpreter_ )
    {
	const int sz = int32interpreter_->nrBytes();
	ArrPtrMan<char> buf = new char [sz];
	if ( !strm.read(buf,sz) )
	    return false;

	nrevents = int16interpreter_->get(buf,0);
    }
    else
    {
	strm >> nrevents;
	if ( !strm ) return false;
    }

    setNrEvents( nrevents );

    if ( int16interpreter_ )
    {
	const int chunksize = nrevents_*mHeaderEventSize;
	if ( !strm.read( buffptr_, chunksize ) )
	{
	    nrevents_ = 0;
	    delete [] buffptr_;
	    buffptr_ = 0;
	    return false;
	}

	return true;
    }

    for ( int idx=0; idx<nrevents_; idx++ )
    {
	BinID bid;
	int offset;
	strm >> bid.inl;
	strm >> bid.crl;
	strm >> offset;
	if ( !strm )
	{
	    nrevents_ = 0;
	    delete [] buffptr_;
	    buffptr_ = 0;
	    return false;
	}

	setBinID( idx, bid );
	setOffset( idx, offset );
    }

    return true;
}


#define mWriteFixedCharVal( val, post ) \
{ \
    unsigned int uval = abs(val); \
    const BufferString valstr ( getStringFromInt64(uval) ); \
    const int len = valstr.size(); \
    int totlen = 11; \
    if ( val<0 ) \
    { \
	strm << '-'; \
	totlen = 10; \
    } \
		 \
    for ( int idy=totlen; idy>len; idy-- ) \
	strm << '0'; \
    strm << valstr.buf() << post; \
}


bool EventPatchFileHeader::toStream( std::ostream& strm, bool binary )
{
    if ( binary )
    {
	strm.write( (const char*) &nrevents_, sizeof(nrevents_) );
	strm.write( (const char*) buffptr_, nrevents_*mHeaderEventSize );
    }
    else
    {
	mWriteFixedCharVal( nrevents_, '\n' );
	for ( int idx=0; idx<nrevents_; idx++ )
	{
	    mWriteFixedCharVal( getBinID(idx).inl, '\t' );
	    mWriteFixedCharVal( getBinID(idx).crl, '\t' );
	    mWriteFixedCharVal( getOffset(idx), '\n' );
	}
    }

    return strm;
}


int EventPatchFileHeader::nrEvents() const
{ return nrevents_; }


void EventPatchFileHeader::setNrEvents( int nr )
{
    delete [] buffptr_;
    nrevents_ = nr;
    buffptr_ = 0;

    const int chunksize = nrevents_*mHeaderEventSize;
    buffptr_ = new char[chunksize];
    memset( buffptr_, 0, chunksize );
}


BinID EventPatchFileHeader::getBinID( int idx ) const
{
    const int offset = idx * mHeaderEventSize;
    const char* baseptr = buffptr_+offset;
    if ( int32interpreter_ )
	return BinID( int32interpreter_->get( baseptr, 0 ),
	       	      int32interpreter_->get( baseptr, 1 ) );

    return BinID( ((int*) baseptr)[0], ((int*) baseptr)[1] );
}


void EventPatchFileHeader::setBinID( int idx, const BinID& bid )
{
    const int offset = idx * mHeaderEventSize;
    char* baseptr = buffptr_+offset;
    if ( int32interpreter_ )
    {
	int32interpreter_->put( baseptr, 0,  bid.inl );
	int32interpreter_->put( baseptr, 1,  bid.crl );
    }
    else
    {
	((int*) baseptr)[0] = bid.inl;
	((int*) baseptr)[1] = bid.crl;
    }
}


int EventPatchFileHeader::getOffset( int idx ) const
{
    const int offset = idx * mHeaderEventSize;
    const char* baseptr = buffptr_+offset+sizeof(int)*2;
    return int32interpreter_ ? int32interpreter_->get( baseptr, 0 )
    			     : *((int*) baseptr);
}


void EventPatchFileHeader::setOffset( int idx, int offsetval )
{
    const int offset = idx * mHeaderEventSize;
    char* baseptr = buffptr_+offset+sizeof(int)*2;
    if ( int32interpreter_ )
	int32interpreter_->put( baseptr, 0,  offsetval );
    else
	*((int*) baseptr) = offsetval;
}


EventPatchReader::EventPatchReader( Conn* conn,
	EventManager* events )
    : conn_( conn )
    , eventmanager_( events )
    , horsel_( 0 )
    , bidvalsel_( 0 )
    , headeridx_( 0 )
    , int32interpreter_( 0 )
    , int16interpreter_( 0 )
    , floatinterpreter_( 0 )
    , readeventtype_( false )
    , readdip_( true )
{
    if ( !conn_ || !conn_->forRead() )
    {
	errmsg_ = "Cannot open connection";
	return;
    }

    std::istream& strm = ((StreamConn*)conn_)->iStream();
    ascistream astream( strm );
    if ( !astream.isOfFileType( EventReader::sFileType() ) )
    {
	errmsg_ = "Invalid filetype";
	return;
    }

    astream.next();
    IOPar par( astream );

    BufferString dcs;
    if ( par.get( EventReader::sKeyInt16DataChar(), dcs ) )
    {
	DataCharacteristics dc; dc.set( dcs.buf() );
	int16interpreter_ = new DataInterpreter<int>( dc );
    }

    if ( par.get( EventReader::sKeyInt32DataChar(), dcs ) )
    {
	DataCharacteristics dc; dc.set( dcs.buf() );
	int32interpreter_ = new DataInterpreter<int>( dc );
    }

    if ( par.get( EventReader::sKeyFloatDataChar(), dcs ) )
    {
	DataCharacteristics dc; dc.set( dcs.buf() );
	floatinterpreter_ = new DataInterpreter<float>( dc );
    }

    filebbox_.usePar( par );

    int version = 1;
    par.get( EventReader::sHorizonFileVersion(), version );
    if ( version==2 )
    {
	readeventtype_ = true;
	readdip_ = false;
    }

    if ( !fileheader_.fromStream( strm ) )
	errmsg_ = "Could not read file header";
}


EventPatchReader::~EventPatchReader()
{ delete conn_; }


void EventPatchReader::setSelection(
	const BinIDValueSet* bidvalset )
{ bidvalsel_ = bidvalset; }



void EventPatchReader::setSelection( const HorSampling* hs )
{ horsel_ = hs; }


bool EventPatchReader::hasDataInRange() const
{
    if ( !horsel_ && !bidvalsel_ )
	return true;

    if ( horsel_ )
    {
	HorSampling dummy;
	if ( horsel_->getInterSection( filebbox_, dummy ) )
	    return true;
    }

    if ( bidvalsel_ )
    {
	for ( int idx=fileheader_.nrEvents()-1; idx>=0; idx-- )
	{
	    if ( bidvalsel_->includes( fileheader_.getBinID(idx) ) )
		return true;
	}
    }

    return false;
}


void EventPatchReader::getPositions( BinIDValueSet& bidset ) const
{
    for ( int idx=fileheader_.nrEvents()-1; idx>=0; idx-- )
	bidset.add( fileheader_.getBinID(idx) );
}


const char* EventPatchReader::errMsg() const
{ return errmsg_[0] ? errmsg_.buf() : 0; }


int EventPatchReader::nextStep()
{
    if ( !eventmanager_ ) return cFinished();

    std::istream& strm = ((StreamConn*)conn_)->iStream();

    BinID curbid;
    while ( headeridx_<fileheader_.nrEvents() )
    {
	curbid = fileheader_.getBinID(headeridx_);
	if ( !isWanted(curbid) )
	{
	    headeridx_++;
	    continue;
	}

	RefMan<EventSet> ge =
	    eventmanager_->getEvents( curbid, false, false );
	if ( ge && ge->ischanged_ )
	{
	    headeridx_++;
	    continue;
	}

	break;
    }

    if ( headeridx_>=fileheader_.nrEvents() )
	return cFinished();

    strm.seekg( fileheader_.getOffset( headeridx_ ), std::ios::beg );
    const int nrevents = readInt16( strm );
    if ( !strm ) return cErrorOccurred();

    EventSet* ge = eventmanager_->getEvents( curbid, false, true );
    ge->ref();
    deepErase( ge->events_ );

    for ( int idx=0; idx<nrevents; idx++ )
    {
	const int nrpicks = readUInt8( strm );
	if ( !strm )
	{
	    ge->unRef();
	    return cErrorOccurred();
	}

	Event* pse = new Event( nrpicks, true );

	pse->horid_ = readInt32( strm );
	pse->quality_ = readUInt8( strm );
	if ( readeventtype_ )
	{
	    const int eventtype = readUInt8( strm );
	    pse->eventtype_ = EventReader::decodeEventType(eventtype);
	}
	else pse->eventtype_ = VSEvent::None;

	if ( readdip_ )
	{
	    readFloat( strm );
	    readFloat( strm );
	}

	for ( int idy=0; idy<nrpicks; idy++ )
	{
	    pse->pick_[idy] = readFloat( strm );
	    pse->pickquality_[idy] = readUInt8( strm );
	    pse->offsetazimuth_[idy].setFrom( readInt32( strm ) );
	}

	ge->events_ += pse;
    }

    if ( !strm )
    {
	deepErase( ge->events_ );
	ge->unRef();
	return cErrorOccurred();
    }

    ge->unRefNoDelete();
    eventmanager_->reportChange( curbid );

    headeridx_++;
    return cMoreToDo();
}


bool EventPatchReader::isWanted( const BinID& bid ) const
{
    if ( !filebbox_.includes( bid ) )
	return false;

    if ( !horsel_ && !bidvalsel_ )
	return true;

    if ( horsel_ && horsel_->includes( bid ) )
	return true;

    return bidvalsel_->includes( bid );
}


int EventPatchReader::readInt16( std::istream& strm ) const
{
    if ( int16interpreter_ )
    {
	const int sz = int16interpreter_->nrBytes();
	ArrPtrMan<char> buf = new char [sz];
	strm.read(buf,sz);
	return int16interpreter_->get(buf,0);
    }

    int res;
    strm >> res;
    return res;
}


int EventPatchReader::readUInt8( std::istream& strm ) const
{
    if ( int16interpreter_ )
    {
	char res;
	strm.read(&res,1);
	return (unsigned char) res;
    }

    int res;
    strm >> res;
    return res;
}


int EventPatchReader::readInt32( std::istream& strm ) const
{
    if ( int32interpreter_ )
    {
	const int sz = int16interpreter_->nrBytes();
	ArrPtrMan<char> buf = new char [sz];
	strm.read(buf,sz);
	return int32interpreter_->get(buf,0);
    }

    int res;
    strm >> res;
    return res;
}


float EventPatchReader::readFloat( std::istream& strm ) const
{
    if ( floatinterpreter_ )
    {
	const int sz = floatinterpreter_->nrBytes();
	ArrPtrMan<char> buf = new char [sz];
	strm.read(buf,sz);
	return floatinterpreter_->get(buf,0);
    }

    float res;
    strm >> res;
    return res;
}



EventPatchWriter::EventPatchWriter( const char* filename, EventManager& ev )
    : conn_( 0 )
    , filename_( filename )
    , eventmanager_( ev )
    , headeridx_( 0 )
    , horsel_( true )
    , reader_( 0 )
    , binary_( false )
{
}


EventPatchWriter::~EventPatchWriter()
{
    delete conn_;
    delete reader_;
}


void EventPatchWriter::setReader( EventPatchReader* reader )
{
    delete reader_;
    reader_ = reader;
    reader_->setSelection( &horsel_ );
}


void EventPatchWriter::setSelection( const HorSampling& hrg )
{ horsel_ = hrg; }


const char* EventPatchWriter::errMsg() const
{
    return errmsg_.size() ? errmsg_.buf() : 0;
}

#define mSetDc( type, string ) \
{ \
    type dummy; \
    DataCharacteristics(dummy).toString( dc.buf() ); \
}\
    par.set( string, dc.buf() )



int EventPatchWriter::nextStep()
{
    if ( reader_ )
    {
	const int res = reader_->nextStep();
	if ( !res )
	{
	    delete reader_;
	    reader_ = 0;
	    return cMoreToDo();
	}

	return res;
    }

    if ( !headeridx_ )
    {
	const MultiDimStorage<EventSet*>& evstor = eventmanager_.getStorage();
	int pos[] = { -1, -1 };

	TypeSet<BinID> bids;
	HorSampling hrg( true );
	while ( evstor.next( pos, true ) )
	{
	    BinID bid;
	    evstor.getPos( pos, bid );
	    if ( !horsel_.includes( bid ) )
		continue;

	    RefMan<EventSet> pses = eventmanager_.getEvents(bid,false,false);
	    
	    bool isempty = true;
	    for ( int idx=pses->events_.size()-1; idx>=0; idx-- )
	    {
		if ( pses->events_[idx]->sz_ )
		{
		    isempty = false;
		    break;
		}
	    }

	    if ( isempty )
		continue;

	    if ( bids.size() ) hrg.include( bid );
	    else { hrg.start = hrg.stop = bid; }
	    bids += bid;
	}

	if ( !bids.size() )
	{
	    if ( File_exists( filename_.buf() ) &&
		 File_isWritable( filename_.buf() ) )
	    {
		return File_remove( filename_.buf(), mFile_NotRecursive )
		    ? cFinished()
		    : cErrorOccurred();
	    }
	}

	fileheader_.setNrEvents( bids.size() );
	for ( int idx=bids.size()-1; idx>=0; idx-- )
	    fileheader_.setBinID( idx, bids[idx] );

	IOPar par;
	if ( binary_ )
	{
	    BufferString dc;
	    mSetDc( od_int32, EventReader::sKeyInt32DataChar() );
	    mSetDc( short, EventReader::sKeyInt16DataChar() );
	    mSetDc( float, EventReader::sKeyFloatDataChar() );
	}

	par.set( EventReader::sHorizonFileVersion(), 2 );

	hrg.fillPar( par );

	if ( !conn_ )
	{
	    conn_ = new StreamConn( filename_.buf(), Conn::Write );
	    if ( !conn_->forWrite() )
	    {
		errmsg_ = "Cannot open connection";
		return cErrorOccurred();
	    }
	}

	std::ostream& strm = conn_->oStream();
	ascostream astream( strm );
	astream.putHeader( EventReader::sFileType() );
	par.putTo( astream );
	fileheaderoffset_ = strm.tellp();
	if ( !fileheader_.toStream( strm, binary_ ) )
	    return cErrorOccurred();
    }

    std::ostream& strm = conn_->oStream();
    if ( headeridx_>=fileheader_.nrEvents() )
    {
	strm.seekp( fileheaderoffset_, std::ios::beg );
	fileheader_.toStream( strm, binary_ );
	return cFinished();
    }

    int curoffset = strm.tellp();
    const BinID bid = fileheader_.getBinID( headeridx_ );
    fileheader_.setOffset( headeridx_, curoffset );

    RefMan<EventSet> pses = eventmanager_.getEvents( bid, false, false);

    TypeSet<int> eventnrtosave;
    for ( int idx=pses->events_.size()-1; idx>=0; idx-- )
    {
	if ( pses->events_[idx]->sz_ )
	    eventnrtosave += idx;
    }

    const int nrevents = eventnrtosave.size();
    writeInt16( strm, nrevents, "\t" );
    for ( int idx=0; idx<eventnrtosave.size(); idx++ )
    {
	const Event* pse = pses->events_[eventnrtosave[idx]];
	const int nrpicks = pse->sz_;
	writeInt16( strm, nrpicks, "\t" );
	writeInt32( strm, pse->horid_, "\t" );
	writeUInt8( strm, pse->quality_, "\t" );
	writeUInt8( strm, EventReader::encodeEventType(pse->eventtype_),
		    "\n\t\t" );

	for ( int idy=0; idy<nrpicks; idy++ )
	{
	    writeFloat( strm, pse->pick_[idy], "\t" );
	    writeUInt8( strm, pse->pickquality_
			      ? pse->pickquality_[idy] : 255, "\t" );
	    writeInt32( strm , pse->offsetazimuth_[idy].asInt(), "\n" );
	    if ( !binary_ )
	    {
		if ( idy!=nrpicks-1 )
		    strm << "\t\t";
		else if ( idx!=nrevents-1 )
		    strm << '\t';
	    }
	}
    }

    if ( !strm )
	return cErrorOccurred();

    pses->ischanged_ = false;

    headeridx_++;
    return cMoreToDo();
}


bool EventPatchWriter::writeInt16( std::ostream& strm, int val,
						 const char* post ) const
{
    if ( binary_ )
	strm.write((const char*)&val,sizeof(val));
    else
	strm << val << post;

    return strm;
}


bool EventPatchWriter::writeInt32( std::ostream& strm, int val,
						 const char* post ) const
{
    if ( binary_ )
	strm.write((const char*)&val,sizeof(val));
    else
	strm << val << post;

    return strm;
}


bool EventPatchWriter::writeUInt8( std::ostream& strm, int val,
						 const char* post ) const
{
    if ( binary_ )
	strm.write((const char*)&val,sizeof(val));
    else
	strm << val << post;

    return strm;
}


bool EventPatchWriter::writeFloat( std::ostream& strm, float val,
						 const char* post ) const
{
    if ( binary_ )
	strm.write((const char*)&val,sizeof(val));
    else
	strm << val << post;

    return strm;
}


}; //namespace
