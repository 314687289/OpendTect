/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 3-8-1994
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "ioman.h"

#include "ascstream.h"
#include "ctxtioobj.h"
#include "file.h"
#include "filepath.h"
#include "iodir.h"
#include "iopar.h"
#include "iostrm.h"
#include "iosubdir.h"
#include "oddirs.h"
#include "separstr.h"
#include "settings.h"
#include "staticstring.h"
#include "strmprov.h"
#include "survinfo.h"
#include "timefun.h"
#include "transl.h"
#include "msgh.h"

#include <stdlib.h>

IOMan* IOMan::theinst_	= 0;
static const MultiID emptykey( "" );


IOMan& IOM()
{
    if ( !IOMan::theinst_ )
	{ IOMan::theinst_ = new IOMan; IOMan::theinst_->init(); }
    return *IOMan::theinst_;
}

IOMan::IOMan( const char* rd )
	: NamedObject("IO Manager")
	, dirptr_(0)
	, survchgblocked_(false)
	, state_(IOMan::NeedInit)
	, newIODir(this)
	, entryRemoved(this)
	, surveyToBeChanged(this)
	, surveyChanged(this)
	, afterSurveyChange(this)
	, applicationClosing(this)
{
    rootdir_ = rd && *rd ? rd : GetDataDir();
    if ( !File::isDirectory(rootdir_) )
	rootdir_ = GetBaseDataDir();
}


void IOMan::init()
{
    state_ = Bad;
    if ( !to( emptykey, true ) )
    {
        FilePath surveyfp( GetDataDir(), ".omf" );
        if ( File::exists(surveyfp.fullPath().buf()) )
        {
            msg_ = "Warning: Invalid '.omf' found in:\n";
	    msg_ += rootdir_;
            msg_ += ".\nThis survey is corrupt.";
	    return;
        }

        FilePath basicfp( mGetSetupFileName(SurveyInfo::sKeyBasicSurveyName()),
			  ".omf" );
        File::copy( basicfp.fullPath(),surveyfp.fullPath() );
        if ( !to( emptykey, true ) )
        {
            msg_ = "Warning: Invalid or no '.omf' found in:\n";
	    msg_ += rootdir_;
            msg_ += ".\nThis survey is corrupt.";
	    return;
        }
    }

    state_ = Good;
    curlvl_ = 0;

    if ( dirPtr()->key() == MultiID("-1") ) return;

    int nrstddirdds = IOObjContext::totalNrStdDirs();
    const IOObjContext::StdDirData* prevdd = 0;
    const bool needsurvtype = !SI().survdatatypeknown_;
    bool needwrite = false;
    FilePath rootfp( rootdir_, "X" );
    for ( int idx=0; idx<nrstddirdds; idx++ )
    {
	IOObjContext::StdSelType stdseltyp = (IOObjContext::StdSelType)idx;
	const IOObjContext::StdDirData* dd
			    = IOObjContext::getStdDirData( stdseltyp );
	const IOObj* dirioobj = (*dirPtr())[MultiID(dd->id)];
	if ( dirioobj )
	{
	    if ( needsurvtype && stdseltyp == IOObjContext::Seis )
	    {
		IODir seisiodir( dirioobj->key() );
		bool has2d = false, has3d = false;
		const BufferString seisstr( "Seismic Data" );
		const BufferString tr2dstr( "2D" );
		const BufferString trsegystr( "SEG-Y" );
		for ( int iobj=0; iobj<seisiodir.size(); iobj++ )
		{
		    const IOObj& subioobj = *seisiodir[iobj];
		    if ( seisstr != subioobj.group() ||
			 trsegystr == subioobj.translator() ) continue;

		    const bool is2d = tr2dstr == subioobj.translator();
		    if ( is2d ) has2d = true;
		    else	has3d = true;
		    if ( has2d && has3d ) break;
		}
		SurveyInfo& si( const_cast<SurveyInfo&>(SI()) );
		si.survdatatypeknown_ = true;
		si.survdatatype_ = !has2d ? SurveyInfo::No2D
						// thus also if nothing found
				 : (has3d ? SurveyInfo::Both2DAnd3D
					  : SurveyInfo::Only2D);
		si.write();
	    }

	    prevdd = dd; continue;
	}

	// Oops, a data directory required is missing
	// We'll try to recover by using the 'Basic Survey' in the app
	FilePath basicfp( mGetSetupFileName(SurveyInfo::sKeyBasicSurveyName()),
			  "X" );
	basicfp.setFileName( dd->dirnm );
	BufferString basicdirnm = basicfp.fullPath();
	if ( !File::exists(basicdirnm) )
	    // Oh? So this is removed from the Basic Survey
	    // Let's hope they know what they're doing
	    { prevdd = dd; continue; }

	rootfp.setFileName( dd->dirnm );
	BufferString dirnm = rootfp.fullPath();

#define mErrMsgRet(s) ErrMsg(s); msg_ = s; state_ = Bad; return
	if ( !File::exists(dirnm) )
	{
	    // This directory should have been in the survey.
	    // It is not. If it is the seismic directory, we do not want to
	    // continue. Otherwise, we want to copy the Basic Survey directory.
	    if ( stdseltyp == IOObjContext::Seis )
	    {
		BufferString msg( "Corrupt survey: missing directory: " );
		msg += dirnm; mErrMsgRet( msg );
	    }
	    else if ( !File::copy(basicdirnm,dirnm) )
	    {
		BufferString msg( "Cannot create directory: " ); msg += dirnm;
		msg += ". You probably do not have write permissions in ";
		msg += rootfp.pathOnly(); mErrMsgRet( msg );
	    }
	}

	// So, we have copied the directory.
	// Now create an entry in the root omf
	IOSubDir* iosd = new IOSubDir( dd->dirnm );
	iosd->key_ = dd->id;
	iosd->dirnm_ = rootdir_;
	const IOObj* previoobj = prevdd ? (*dirPtr())[prevdd->id]
					: dirPtr()->main();
	int idxof = dirPtr()->objs_.indexOf( (IOObj*)previoobj );
	dirPtr()->objs_.insertAfter( iosd, idxof );

	prevdd = dd;
	needwrite = true;
    }

    if ( needwrite )
    {
	dirPtr()->doWrite();
	to( emptykey, true );
    }
}


IOMan::~IOMan()
{
    delete dirptr_;
}


bool IOMan::isReady() const
{
    return isBad() || !dirptr_ ? false : dirptr_->key() != MultiID("-1");
}


#define mDestroyInst(dotrigger) \
    if ( dotrigger && !IOM().isBad() ) \
	IOM().surveyToBeChanged.trigger(); \
    if ( IOM().changeSurveyBlocked() ) \
    { \
	IOM().setChangeSurveyBlocked(false); \
	return false; \
    } \
    StreamProvider::unLoadAll(); \
    CallBackSet s2bccbs = IOM().surveyToBeChanged.cbs_; \
    CallBackSet sccbs = IOM().surveyChanged.cbs_; \
    CallBackSet asccbs = IOM().afterSurveyChange.cbs_; \
    CallBackSet rmcbs = IOM().entryRemoved.cbs_; \
    CallBackSet dccbs = IOM().newIODir.cbs_; \
    CallBackSet apccbs = IOM().applicationClosing.cbs_; \
    delete IOMan::theinst_; \
    IOMan::theinst_ = 0; \
    clearSelHists();

#define mFinishNewInst(dotrigger) \
    IOM().surveyToBeChanged.cbs_ = s2bccbs; \
    IOM().surveyChanged.cbs_ = sccbs; \
    IOM().afterSurveyChange.cbs_ = asccbs; \
    IOM().entryRemoved.cbs_ = rmcbs; \
    IOM().newIODir.cbs_ = dccbs; \
    IOM().applicationClosing.cbs_ = apccbs; \
    if ( !IOM().isBad() ) \
    { \
	SurveyInfo::setSurveyName( SI().getDirName() ); \
	setupCustomDataDirs(-1); \
	if ( dotrigger ) \
	{ \
	    IOM().surveyChanged.trigger(); \
	    IOM().afterSurveyChange.trigger(); \
	} \
    }


static void clearSelHists()
{
    const ObjectSet<TranslatorGroup>& grps = TranslatorGroup::groups();
    for ( int idx=0; idx<grps.size(); idx++ )
	const_cast<TranslatorGroup*>(grps[idx])->clearSelHist();
}


bool IOMan::newSurvey( SurveyInfo* newsi )
{
    mDestroyInst( true );

    SurveyInfo::deleteInstance();
    if ( !newsi )
	SurveyInfo::setSurveyName( "" );
    else
    {
	SurveyInfo::setSurveyName( newsi->getDirName() );
	SurveyInfo::pushSI( newsi );
    }

    mFinishNewInst( true );
    return !IOM().isBad();
}


bool IOMan::setSurvey( const char* survname )
{
    mDestroyInst( true );

    SurveyInfo::deleteInstance();
    SurveyInfo::setSurveyName( survname );

    mFinishNewInst( true );
    return !IOM().isBad();
}


void IOMan::surveyParsChanged()
{
    IOM().surveyToBeChanged.trigger();
    if ( IOM().changeSurveyBlocked() )
	{ IOM().setChangeSurveyBlocked(false); return; }
    IOM().surveyChanged.trigger();
    IOM().afterSurveyChange.trigger();
}


const char* IOMan::surveyName() const
{
    return GetSurveyName();
}


static bool validOmf( const char* dir )
{
    FilePath fp( dir ); fp.add( ".omf" );
    BufferString fname = fp.fullPath();
    if ( File::isEmpty(fname) )
    {
	fp.setFileName( ".omb" );
	if ( File::isEmpty(fp.fullPath()) )
	    return false;
	else
	    File::copy( fname, fp.fullPath() );
    }
    return true;
}


#define mErrRet(str) \
    { errmsg = str; return false; }

#define mErrRetNotODDir(fname) \
    { \
        errmsg = "$DTECT_DATA="; errmsg += GetBaseDataDir(); \
        errmsg += "\nThis is not a valid OpendTect data storage directory."; \
	if ( fname ) \
	    { errmsg += "\n[Cannot find: "; errmsg += fname; errmsg += "]"; } \
        return false; \
    }

bool IOMan::validSurveySetup( BufferString& errmsg )
{
    errmsg = "";
    const BufferString basedatadir( GetBaseDataDir() );
    if ( basedatadir.isEmpty() )
	mErrRet("Please set the environment variable DTECT_DATA.")
    else if ( !File::exists(basedatadir) )
	mErrRetNotODDir(0)
    else if ( !validOmf(basedatadir) )
	mErrRetNotODDir(".omf")

    const BufferString projdir = GetDataDir();
    if ( projdir != basedatadir && File::isDirectory(projdir) )
    {
	const bool noomf = !validOmf( projdir );
	const bool nosurv = File::isEmpty( FilePath(projdir).
					   add(SurveyInfo::sKeySetupFileName()).
					   fullPath() );
	if ( !noomf && !nosurv )
	{
	    if ( !IOM().isBad() )
		return true; // This is normal

	    // But what's wrong here? In any case - survey is not good.
	}

	else
	{
	    BufferString msg;
	    if ( nosurv && noomf )
		msg = "Warning: Essential data files not found in ";
	    else if ( nosurv )
	    {
		msg = BufferString( "Warning: Invalid or no '",
				    SurveyInfo::sKeySetupFileName(),
				    "' found in " );
	    }
	    else if ( noomf )
		msg = "Warning: Invalid or no '.omf' found in ";
	    msg += projdir; msg += ".\nThis survey is corrupt.";
	    UsrMsg( msg );
	}
    }

    // Survey name in ~/.od/survey is invalid or absent. If there, lose it ...
    const BufferString survfname = SurveyInfo::surveyFileName();
    if ( File::exists(survfname) && !File::remove(survfname) )
    {
	errmsg.set( "The file:\n" ).add( survfname )
	    .add( "\ncontains an invalid survey.\n\nPlease remove this file" );
	return false;
    }

    SurveyInfo::setSurveyName( "" ); // force user-set of survey

    mDestroyInst( false );
    mFinishNewInst( false );
    return true;
}


bool IOMan::setRootDir( const char* dirnm )
{
    if ( !dirnm || rootdir_==dirnm ) return true;
    if ( !File::isDirectory(dirnm) ) return false;
    rootdir_ = dirnm;
    return setDir( rootdir_ );
}


bool IOMan::to( const IOSubDir* sd, bool forcereread )
{
    if ( isBad() )
    {
	if ( !to("0",true) || isBad() ) return false;
	return to( sd, true );
    }
    else if ( !forcereread )
    {
	if ( !sd && curlvl_ == 0 )
	    return true;
	else if ( dirptr_ && sd && sd->key() == dirptr_->key() )
	    return true;
    }

    const char* dirnm = sd ? sd->dirName() : rootdir_.buf();
    if ( !File::isDirectory(dirnm) )
	return false;

    return setDir( dirnm );
}


bool IOMan::to( const MultiID& ky, bool forcereread )
{
    const bool issamedir = dirptr_ && ky == dirptr_->key();
    if ( !forcereread && issamedir )
	return true;

    MultiID dirkey;
    IOObj* refioobj = IODir::getObj( ky );
    if ( refioobj )
	dirkey = refioobj->isSubdir() ? ky : MultiID(ky.upLevel());
    else
    {
	dirkey = ky.upLevel();
	refioobj = IODir::getObj( dirkey );
	if ( !refioobj )
	    dirkey = "";
    }
    delete refioobj;

    IODir* newdir = dirkey.isEmpty() ? new IODir(rootdir_) : new IODir(dirkey);
    if ( !newdir || newdir->isBad() )
	return false;

    bool needtrigger = dirptr_;
    if ( dirptr_ )
	delete dirptr_;
    dirptr_ = newdir;
    curlvl_ = levelOf( curDirName() );
    if ( needtrigger )
	newIODir.trigger();

    return true;
}


IOObj* IOMan::get( const MultiID& k ) const
{
    if ( !IOObj::isKey(k) )
	return 0;

    MultiID ky( k );
    char* ptr = strchr( ky.buf(), '|' );
    if ( ptr ) *ptr = '\0';
    ptr = strchr( ky.buf(), ' ' );
    if ( ptr ) *ptr = '\0';

    if ( dirptr_ )
    {
	const IOObj* ioobj = (*dirptr_)[ky];
	if ( ioobj ) return ioobj->clone();
    }

    return IODir::getObj( ky );
}


IOObj* IOMan::getOfGroup( const char* tgname, bool first,
			  bool onlyifsingle ) const
{
    if ( isBad() || !tgname ) return 0;

    const IOObj* ioobj = 0;
    for ( int idx=0; idx<dirptr_->size(); idx++ )
    {
	if ( (*dirptr_)[idx]->group()==tgname )
	{
	    if ( onlyifsingle && ioobj ) return 0;

	    ioobj = (*dirptr_)[idx];
	    if ( first && !onlyifsingle ) break;
	}
    }

    return ioobj ? ioobj->clone() : 0;
}


IOObj* IOMan::getLocal( const char* objname ) const
{
    if ( !objname || !*objname )
	return 0;

    if ( matchString("ID=<",objname) )
    {
	BufferString oky( objname+4 );
	char* ptr = strchr( oky.buf(), '>' );
	if ( ptr ) *ptr = '\0';
	return get( MultiID((const char*)oky) );
    }

    if ( dirptr_ )
    {
	const IOObj* ioobj = (*dirptr_)[objname];
	if ( ioobj ) return ioobj->clone();
    }

    if ( IOObj::isKey(objname) )
	return get( MultiID(objname) );

    return 0;
}


IOObj* IOMan::getFirst( const IOObjContext& ctxt, int* nrfound ) const
{
    if ( !ctxt.trgroup ) return 0;

    IOM().to( ctxt.getSelKey() );

    const ObjectSet<IOObj>& ioobjs = dirptr_->getObjs();
    IOObj* ret = 0; if ( nrfound ) *nrfound = 0;
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj* ioobj = ioobjs[idx];
	if ( ctxt.validIOObj(*ioobj) )
	{
	    if ( !ret )
		ret = ioobj->clone();
	    if ( nrfound )
		(*nrfound)++;
	    else
		break;
	}
    }

    return ret;
}


IOObj* IOMan::getFromPar( const IOPar& par, const char* bky,
			  const IOObjContext& ctxt,
			  bool mknew, BufferString& errmsg ) const
{
    BufferString basekey( bky );
    if ( !basekey.isEmpty() ) basekey.add( "." );
    BufferString iopkey( basekey );
    iopkey.add( sKey::ID() );
    BufferString res = par.find( iopkey ).str();
    if ( res.isEmpty() )
    {
	iopkey = basekey; iopkey.add( sKey::Name() );
	res = par.find( iopkey ).str();
	if ( res.isEmpty() )
	{
	    errmsg = BufferString( "Please specify '", iopkey, "'" );
	    return 0;
	}

	if ( !IOObj::isKey(res.buf()) )
	{
	    CtxtIOObj ctio( ctxt );
	    IOM().to( ctio.ctxt.getSelKey() );
	    const IOObj* ioob = (*(const IODir*)(dirPtr()))[res.buf()];
	    if ( ioob )
		res = ioob->key();
	    else if ( mknew )
	    {
		ctio.setName( res );
		IOM().getEntry( ctio );
		if ( ctio.ioobj )
		{
		    IOM().commitChanges( *ctio.ioobj );
		    return ctio.ioobj;
		}
	    }
	}
    }

    IOObj* ioobj = get( MultiID(res.buf()) );
    if ( !ioobj )
	errmsg = BufferString( "Value for ", iopkey, " is invalid." );

    return ioobj;
}


bool IOMan::isKey( const char* ky ) const
{
    if ( !ky || !*ky || !isdigit(*ky) ) return false;

    bool digitseen = false;
    while ( *ky )
    {
	if ( isdigit(*ky) )
	    digitseen = true;
	else if ( *ky == '|' )
	    return digitseen;
	else if ( *ky != '.' )
	    return false;
	ky++;
    }

    return true;
}


const char* IOMan::nameOf( const char* id ) const
{
    mDeclStaticString( ret );
    if ( !id || !*id || !IOObj::isKey(id) )
	return id;

    MultiID ky( id );
    IOObj* ioobj = get( ky );
    ret.setEmpty();
    if ( !ioobj )
	{ ret = "ID=<"; ret += id; ret += ">"; }
    else
    {
	ret = ioobj->name();
	delete ioobj;
    }

    return ret.buf();
}


const char* IOMan::curDirName() const
{
    return dirptr_ ? dirptr_->dirName() : (const char*)rootdir_;
}


const MultiID& IOMan::key() const
{
    return dirptr_ ? dirptr_->key() : emptykey;
}


bool IOMan::setDir( const char* dirname )
{
    if ( !dirname ) dirname = rootdir_;

    IODir* newdirptr = new IODir( dirname );
    if ( !newdirptr ) return false;
    if ( newdirptr->isBad() )
    {
	delete newdirptr;
	return false;
    }

    bool needtrigger = dirptr_;
    delete dirptr_;
    dirptr_ = newdirptr;
    curlvl_ = levelOf( curDirName() );
    if ( needtrigger )
	newIODir.trigger();
    return true;
}


static const char* getTranslDirNm( const Translator* tr )
{
    const IOObjContext& ctxt = tr->group()->ioCtxt();
    const IOObjContext::StdDirData* sdd
		    = IOObjContext::getStdDirData( ctxt.stdseltype );
    return sdd ? sdd->dirnm : 0;
}


void IOMan::getEntry( CtxtIOObj& ctio, bool mktmp )
{
    ctio.setObj( 0 );
    if ( ctio.ctxt.name().isEmpty() )
	return;
    to( ctio.ctxt.getSelKey() );

    const IOObj* ioobj = (*dirPtr())[ ctio.ctxt.name() ];
    ctio.ctxt.fillTrGroup();
    if ( ioobj && ctio.ctxt.trgroup->userName() != ioobj->group() )
	ioobj = 0;

    if ( !ioobj )
    {
	// Make new key and generate name
	MultiID newkey( mktmp ? ctio.ctxt.getSelKey() : dirptr_->newKey() );
	if ( mktmp )
	    newkey.add( IOObj::tmpID() );
	IOStream* iostrm = new IOStream( ctio.ctxt.name(), newkey, false );
	iostrm->setGroup( ctio.ctxt.trgroup->userName() );

	// Get default translator with dir
	const Translator* tr = 0;
	if ( !ctio.ctxt.trgroup->templates().isEmpty() )
	{
	    const int trnr = ctio.ctxt.trgroup->defTranslIdx();
	    tr = ctio.ctxt.trgroup->templates()[trnr];
	}
	BufferString trnm( ctio.ctxt.deftransl.isEmpty()
			 ? (tr ? tr->userName().buf() : "")
			 : ctio.ctxt.deftransl.buf() );
	if ( trnm.isEmpty() ) trnm = "OD"; // shouldn't happen
	iostrm->setTranslator( trnm );
	const char* dirnm = getTranslDirNm( tr );
	if ( dirnm )
	    iostrm->setDirName( dirnm );

	// Now generate the 'right' filename
	Translator* tmptr = ctio.ctxt.trgroup->make( trnm );
	if ( tmptr )
	{
	    iostrm->setExt( tmptr->defExtension() );
	    delete tmptr;
	}

	dirPtr()->mkUniqueName( iostrm );
	const BufferString uniqnm( iostrm->name() );
	int ifnm = 0;
	while ( true )
	{
	    iostrm->genDefaultImpl();
	    if ( !File::exists(iostrm->fileName()) )
		break;
	    ifnm++;
	    iostrm->setName( BufferString(uniqnm,ifnm) );
	    dirPtr()->mkUniqueName( iostrm );
	}

	iostrm->updateCreationPars();
	ioobj = iostrm;

	ioobj->pars().merge( ctio.ctxt.toselect.require_ );
	dirPtr()->addObj( (IOObj*)ioobj );
    }

    ctio.setObj( ioobj->clone() );
}


int IOMan::levelOf( const char* dirnm ) const
{
    if ( !dirnm ) return 0;

    int lendir = strlen(dirnm);
    int lenrootdir = strlen(rootdir_);
    if ( lendir <= lenrootdir ) return 0;

    int lvl = 0;
    const char* ptr = ((const char*)dirnm) + lenrootdir;
    while ( ptr )
    {
	ptr++; lvl++;
	ptr = strchr( ptr, *FilePath::dirSep(FilePath::Local) );
    }
    return lvl;
}


bool IOMan::commitChanges( const IOObj& ioobj )
{
    PtrMan<IOObj> clone = ioobj.clone();
    to( clone->key() );
    return dirPtr() ? dirPtr()->commitChanges( clone ) : false;
}


bool IOMan::permRemove( const MultiID& ky )
{
    if ( !dirPtr() || !dirPtr()->permRemove(ky) )
	return false;

    CBCapsule<MultiID> caps( ky, this );
    entryRemoved.trigger( &caps );
    return true;
}


class SurveyDataTreePreparer
{
public:
			SurveyDataTreePreparer( const IOMan::CustomDirData& dd )
			    : dirdata_(dd)		{}

    bool		prepDirData();
    bool		prepSurv();
    bool		createDataTree();

    const IOMan::CustomDirData&	dirdata_;
    BufferString	errmsg_;
};


#undef mErrRet
#define mErrRet(s1,s2,s3) \
	{ errmsg_ = s1; errmsg_ += s2; errmsg_ += s3; return false; }


bool SurveyDataTreePreparer::prepDirData()
{
    IOMan::CustomDirData* dd = const_cast<IOMan::CustomDirData*>( &dirdata_ );

    replaceCharacter( dd->desc_.buf(), ':', ';' );
    cleanupString( dd->dirname_.buf(), false, false, false );

    int nr = dd->selkey_.ID( 0 );
    if ( nr <= 200000 )
	mErrRet("Invalid selection key passed for '",dd->desc_,"'")

    dd->selkey_ = "";
    dd->selkey_.setID( 0, nr );

    return true;
}


bool SurveyDataTreePreparer::prepSurv()
{
    if ( IOM().isBad() ) { errmsg_ = "Bad directory"; return false; }

    PtrMan<IOObj> ioobj = IOM().get( dirdata_.selkey_ );
    if ( ioobj ) return true;

    IOM().toRoot();
    if ( IOM().isBad() )
	{ errmsg_ = "Can't go to root of survey"; return false; }
    IODir* topdir = IOM().dirPtr();
    if ( !topdir->main() || topdir->main()->name() == "Appl dir" )
	return true;

    if ( !createDataTree() )
	return false;

    // Maybe the parent entry is already there, then this would succeeed now:
    ioobj = IOM().get( dirdata_.selkey_ );
    if ( ioobj ) return true;

    if ( !IOM().dirPtr()->addObj(IOMan::getIOSubDir(dirdata_),true) )
	mErrRet( "Couldn't add ", dirdata_.dirname_, " directory to root .omf" )
    return true;
}


bool SurveyDataTreePreparer::createDataTree()
{
    if ( !IOM().dirPtr() ) { errmsg_ = "Invalid current survey"; return false; }

    FilePath fp( IOM().dirPtr()->dirName() );
    fp.add( dirdata_.dirname_ );
    const BufferString thedirnm( fp.fullPath() );
    bool dircreated = false;
    if ( !File::exists(thedirnm) )
    {
	if ( !File::createDir(thedirnm) )
	    mErrRet( "Cannot create '", dirdata_.dirname_,
		     "' directory in survey");
	dircreated = true;
    }

    fp.add( ".omf" );
    const BufferString omffnm( fp.fullPath() );
    if ( File::exists(omffnm) )
	return true;

    StreamData sd = StreamProvider( fp.fullPath() ).makeOStream();
    if ( !sd.usable() )
    {
	if ( dircreated )
	    File::remove( thedirnm );
	mErrRet( "Could not create '.omf' file in ", dirdata_.dirname_,
		 " directory" );
    }

    *sd.ostrm << GetProjectVersionName();
    *sd.ostrm << "\nObject Management file\n";
    *sd.ostrm << Time::getDateTimeString();
    *sd.ostrm << "\n!\nID: " << dirdata_.selkey_ << "\n!\n"
	      << dirdata_.desc_ << ": 1\n"
	      << dirdata_.desc_ << " directory: Gen`Stream\n"
		"$Name: Main\n!"
	      << std::endl;
    sd.close();
    return true;
}


static TypeSet<IOMan::CustomDirData>& getCDDs()
{
    mDefineStaticLocalObject( PtrMan<TypeSet<IOMan::CustomDirData> >, cdds,
			      = new TypeSet<IOMan::CustomDirData> );
    return *cdds;
}


const MultiID& IOMan::addCustomDataDir( const IOMan::CustomDirData& dd )
{
    SurveyDataTreePreparer sdtp( dd );
    if ( !sdtp.prepDirData() )
    {
	ErrMsg( sdtp.errmsg_ );
	mDefineStaticLocalObject( MultiID, none, ("") );
	return none;
    }

    TypeSet<IOMan::CustomDirData>& cdds = getCDDs();
    for ( int idx=0; idx<cdds.size(); idx++ )
    {
	const IOMan::CustomDirData& cdd = cdds[idx];
	if ( cdd.selkey_ == dd.selkey_ )
	    return cdd.selkey_;
    }

    cdds += dd;
    int idx = cdds.size() - 1;
    const char* survnm = IOM().surveyName();
    if ( survnm && *survnm )
	setupCustomDataDirs( idx );
    return cdds[idx].selkey_;
}


void IOMan::setupCustomDataDirs( int taridx )
{
    const TypeSet<IOMan::CustomDirData>& cdds = getCDDs();
    for ( int idx=0; idx<cdds.size(); idx++ )
    {
	if ( taridx >= 0 && idx != taridx )
	    continue;

	SurveyDataTreePreparer sdtp( cdds[idx] );
	if ( !sdtp.prepSurv() )
	    ErrMsg( sdtp.errmsg_ );
    }
}


IOSubDir* IOMan::getIOSubDir( const IOMan::CustomDirData& cdd )
{
    IOSubDir* sd = new IOSubDir( cdd.dirname_ );
    sd->setDirName( IOM().rootDir() );
    sd->setKey( cdd.selkey_ );
    sd->isbad_ = false;
    return sd;
}


bool IOMan::isValidDataRoot( const char* d )
{
    FilePath fp( d ? d : GetBaseDataDir() );
    const BufferString dirnm( fp.fullPath() );
    if ( !File::isDirectory(dirnm) || !File::isWritable(dirnm) )
	return false;

    fp.add( ".omf" );
    if ( !File::exists(fp.fullPath()) )
	return false;

    return true;
}
