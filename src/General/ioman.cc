/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 3-8-1994
-*/

static const char* rcsID = "$Id: ioman.cc,v 1.97 2009-03-16 12:43:46 cvsbert Exp $";

#include "ioman.h"
#include "iodir.h"
#include "oddirs.h"
#include "iopar.h"
#include "iolink.h"
#include "iostrm.h"
#include "transl.h"
#include "ctxtioobj.h"
#include "filegen.h"
#include "filepath.h"
#include "errh.h"
#include "strmprov.h"
#include "survinfo.h"
#include "ascstream.h"
#include "timefun.h"
#include "oddatadirmanip.h"
#include "envvars.h"
#include "settings.h"

#include <stdlib.h>
#include <sstream>

IOMan*	IOMan::theinst_	= 0;
extern "C" void SetSurveyName(const char*);
extern "C" const char* GetSurveyName();
extern "C" void SetSurveyNameDirty();

static bool survchg_triggers = false;


IOMan& IOM()
{
    if ( !IOMan::theinst_ )
	{ IOMan::theinst_ = new IOMan; IOMan::theinst_->init(); }
    return *IOMan::theinst_;
}

IOMan::IOMan( const char* rd )
	: NamedObject("IO Manager")
	, dirptr(0)
	, state_(IOMan::NeedInit)
    	, newIODir(this)
    	, entryRemoved(this)
    	, surveyToBeChanged(this)
    	, surveyChanged(this)
    	, afterSurveyChange(this)
    	, applicationClosing(this)
{
    rootdir = rd && *rd ? rd : GetDataDir();
    if ( !File_isDirectory(rootdir) )
	rootdir = GetBaseDataDir();
}


void IOMan::init()
{
    state_ = Bad;
    if ( !to( prevkey ) ) return;

    state_ = Good;
    curlvl = 0;

    if ( dirPtr()->key() == MultiID("-1") ) return;

    int nrstddirdds = IOObjContext::totalNrStdDirs();
    const IOObjContext::StdDirData* prevdd = 0;
    const bool needsurvtype = SI().isValid() && !SI().survdatatypeknown_;
    bool needwrite = false;
    FilePath basicfp = FilePath( mGetSetupFileName("BasicSurvey") );
    FilePath rootfp = FilePath( rootdir );
    basicfp.add( "X" ); rootfp.add( "X" );
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
	// We'll try to recover by using the 'BasicSurvey' in the app
	basicfp.setFileName( dd->dirnm );
	BufferString basicdirnm = basicfp.fullPath();
	if ( !File_exists(basicdirnm) )
	    // Oh? So this is removed from the BasicSurvey
	    // Let's hope they know what they're doing
	    { prevdd = dd; continue; }

	rootfp.setFileName( dd->dirnm );
	BufferString dirnm = rootfp.fullPath();
	if ( !File_exists(dirnm) )
	{
	    // This directory should have been in the survey.
	    // It is not. If it is the seismic directory, we do not want to
	    // continue. Otherwise, we want to copy the directory.
	    if ( stdseltyp == IOObjContext::Seis )
	    {
		BufferString msg( "Corrupt survey: missing directory: " );
		msg += dirnm; ErrMsg( msg ); state_ = Bad; return;
	    }
	    else if ( !File_copy(basicdirnm,dirnm,mFile_Recursive) )
	    {
		BufferString msg( "Cannot create directory: " );
		msg += dirnm; ErrMsg( msg ); state_ = Bad; return;
	    }
	}

	// So, we have copied the directory.
	// Now create an entry in the root omf
	MultiID ky( dd->id );
	ky += "1";
	IOObj* iostrm = new IOStream(dd->dirnm,ky);
	iostrm->setGroup( dd->desc );
	iostrm->setTranslator( "dGB" );
	IOLink* iol = new IOLink( iostrm );
	iol->key_ = dd->id;
	iol->dirname = iostrm->name();
	const IOObj* previoobj = prevdd ? (*dirPtr())[prevdd->id]
					: dirPtr()->main();
	int idxof = dirPtr()->objs_.indexOf( (IOObj*)previoobj );
	dirPtr()->objs_.insertAfter( iol, idxof );

	prevdd = dd;
	needwrite = true;
    }

    if ( needwrite )
    {
	dirPtr()->doWrite();
	to( prevkey );
    }
}


IOMan::~IOMan()
{
    delete dirptr;
}


bool IOMan::isReady() const
{
    return bad() || !dirptr ? false : dirptr->key() != MultiID("-1");
}


#define mDestroyInst(dotrigger) \
    if ( dotrigger && survchg_triggers ) \
	IOM().surveyToBeChanged.trigger(); \
    StreamProvider::unLoadAll(); \
    CallBackSet s2bccbs = IOM().surveyToBeChanged.cbs_; \
    CallBackSet sccbs = IOM().surveyChanged.cbs_; \
    CallBackSet asccbs = IOM().afterSurveyChange.cbs_; \
    CallBackSet rmcbs = IOM().entryRemoved.cbs_; \
    CallBackSet dccbs = IOM().newIODir.cbs_; \
    CallBackSet apccbs = IOM().applicationClosing.cbs_; \
    delete IOMan::theinst_; \
    IOMan::theinst_ = 0; \
    clearSelHists()

#define mFinishNewInst(dotrigger) \
    IOM().surveyToBeChanged.cbs_ = s2bccbs; \
    IOM().surveyChanged.cbs_ = sccbs; \
    IOM().afterSurveyChange.cbs_ = asccbs; \
    IOM().entryRemoved.cbs_ = rmcbs; \
    IOM().newIODir.cbs_ = dccbs; \
    IOM().applicationClosing.cbs_ = apccbs; \
    if ( dotrigger ) \
    { \
	setupCustomDataDirs(-1); \
	if ( dotrigger && survchg_triggers ) \
	{ \
	    IOM().surveyChanged.trigger(); \
	    IOM().afterSurveyChange.trigger(); \
	} \
    }


static void clearSelHists()
{
    const ObjectSet<TranslatorGroup>& grps = TranslatorGroup::groups();
    const int sz = grps.size();
    for ( int idx=0; idx<grps.size(); idx++ )
	const_cast<TranslatorGroup*>(grps[idx])->clearSelHist();
}


bool IOMan::newSurvey()
{
    mDestroyInst( true );

    SetSurveyNameDirty();
    mFinishNewInst( true );
    return !IOM().bad();
}


void IOMan::setSurvey( const char* survname )
{
    mDestroyInst( true );

    SurveyInfo::deleteInstance();
    SetSurveyName( survname );
    mFinishNewInst( true );
}


const char* IOMan::surveyName() const
{
    return GetSurveyName();
}


static bool validOmf( const char* dir )
{
    FilePath fp( dir ); fp.add( ".omf" );
    BufferString fname = fp.fullPath();
    if ( File_isEmpty(fname) )
    {
	fp.setFileName( ".omb" );
	if ( File_isEmpty(fp.fullPath()) )
	    return false;
	else
	    File_copy( fname, fp.fullPath(), mFile_NotRecursive );
    }
    return true;
}


extern "C" const char* GetSurveyFileName();

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
    else if ( !File_exists(basedatadir) )
	mErrRetNotODDir(0)
    else if ( !validOmf(basedatadir) )
	mErrRetNotODDir(".omf")

    const BufferString projdir = GetDataDir();
    if ( projdir != basedatadir && File_isDirectory(projdir) )
    {
	const bool noomf = !validOmf( projdir );
	const bool nosurv = File_isEmpty(
				FilePath(projdir).add(".survey").fullPath() );

	if ( !noomf && !nosurv )
	{
	    if ( !IOM().bad() )
		return true; // This is normal

	    // But what's wrong here? In any case - survey is not good.
	}

	else
	{
	    BufferString msg;
	    if ( nosurv && noomf )
		msg = "Warning: Essential data files not found in ";
	    else if ( nosurv )
		msg = "Warning: Invalid or no '.survey' found in ";
	    else if ( noomf )
		msg = "Warning: Invalid or no '.omf' found in ";
	    msg += projdir; msg += ".\nThis survey is corrupt.";
	    UsrMsg( msg );
	}
    }

    // Survey in ~/.od/survey[.$DTECT_USER] is invalid. Remove it if necessary
    BufferString survfname = GetSurveyFileName();
    if ( File_exists(survfname) && !File_remove(survfname,mFile_NotRecursive) )
    {
	errmsg = "The file "; errmsg += survfname;
	errmsg += " contains an invalid survey.\n";
	errmsg += "Please remove this file";
	return false;
    }

    mDestroyInst( false );
    mFinishNewInst( false );
    return true;
}


bool IOMan::setRootDir( const char* dirnm )
{
    if ( !dirnm || !strcmp(rootdir,dirnm) ) return true;
    if ( !File_isDirectory(dirnm) ) return false;
    rootdir = dirnm;
    return setDir( rootdir );
}


bool IOMan::to( const IOLink* link )
{
    if ( bad() )
	return link ? to( link->link()->key() ) : to( prevkey );
    else if ( !link && curlvl == 0 )
	return false;
    else if ( dirptr && link && link->key() == dirptr->key() )
	return true;

    FilePath fp( curDir() );
    fp.add( link ? (const char*)link->dirname : ".." );
    BufferString fulldir = fp.fullPath();
    if ( !File_isDirectory(fulldir) ) return false;

    prevkey = dirptr->key();
    return setDir( fulldir );
}


bool IOMan::to( const MultiID& ky )
{
    if ( dirptr && ky == dirptr->key() )
	return true;

    MultiID currentkey;
    IOObj* refioobj = IODir::getObj( ky );
    if ( !refioobj )
    {
	currentkey = ky.upLevel();
	refioobj = IODir::getObj( currentkey );
	if ( !refioobj )		currentkey = "";
    }
    else if ( !refioobj->isLink() )	currentkey = ky.upLevel();
    else				currentkey = ky;
    delete refioobj;

    IODir* newdir = currentkey.isEmpty()
	? new IODir( rootdir ) : new IODir( currentkey );
    if ( !newdir || newdir->bad() ) return false;

    bool needtrigger = dirptr;
    if ( dirptr )
    {
	prevkey = dirptr->key();
	delete dirptr;
    }
    dirptr = newdir;
    curlvl = levelOf( curDir() );
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

    if ( dirptr )
    {
	const IOObj* ioobj = (*dirptr)[ky];
	if ( ioobj ) return ioobj->clone();
    }

    return IODir::getObj( ky );
}


IOObj* IOMan::getOfGroup( const char* tgname, bool first,
			  bool onlyifsingle ) const
{
    if ( bad() || !tgname ) return 0;

    const IOObj* ioobj = 0;
    for ( int idx=0; idx<dirptr->size(); idx++ )
    {
	if ( !strcmp((*dirptr)[idx]->group(),tgname) )
	{
	    if ( onlyifsingle && ioobj ) return 0;

	    ioobj = (*dirptr)[idx];
	    if ( first && !onlyifsingle ) break;
	}
    }

    return ioobj ? ioobj->clone() : 0;
}


IOObj* IOMan::getLocal( const char* objname ) const
{
    if ( dirptr )
    {
	const IOObj* ioobj = (*dirptr)[objname];
	if ( ioobj ) return ioobj->clone();
    }

    if ( IOObj::isKey(objname) )
	return get( MultiID(objname) );

    return 0;
}


IOObj* IOMan::getByName( const char* objname,
			 const char* pardirname, const char* parname )
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

    MultiID startky = dirptr->key();
    to( MultiID("") );

    bool havepar = pardirname && *pardirname;
    bool parprov = parname && *parname;
    const ObjectSet<IOObj>& ioobjs = dirptr->getObjs();
    ObjectSet<MultiID> kys;
    const IOObj* ioobj;
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	ioobj = ioobjs[idx];
	if ( !havepar )
	{
	    if ( !strcmp(ioobj->name(),objname) )
		kys += new MultiID(ioobj->key());
	}
	else
	{
	    if ( !strncmp(ioobj->group(),pardirname,3) )
	    {
		if ( !parprov || ioobj->name() == parname )
		{
		    kys += new MultiID(ioobj->key());
		    if ( parprov ) break;
		}
	    }
	}
    }

    ioobj = 0;
    for ( int idx=0; idx<kys.size(); idx++ )
    {
	if ( havepar && !to( *kys[idx] ) )
	{
	    BufferString msg( "Survey is corrupt. Cannot go to dir with ID: " );
	    msg += (const char*)(*kys[idx]);
	    ErrMsg( msg );
	    deepErase( kys );
	    return 0;
	}

	ioobj = (*dirptr)[objname];
	if ( ioobj ) break;
    }

    if ( ioobj ) ioobj = ioobj->clone();
    to( startky );
    deepErase( kys );
    return (IOObj*)ioobj;
}


IOObj* IOMan::getFirst( const IOObjContext& ctxt, int* nrfound ) const
{
    if ( !ctxt.trgroup ) return 0;

    IOM().to( ctxt.getSelKey() );

    const ObjectSet<IOObj>& ioobjs = dirptr->getObjs();
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


const char* IOMan::nameOf( const char* id, bool full ) const
{
    static FileNameString ret;
    ret = "";
    if ( !id || !*id ) return ret;

    MultiID ky( id );
    IOObj* ioobj = get( ky );
    if ( !ioobj )
	{ ret = "ID=<"; ret += id; ret += ">"; }
    else
    {
	do { 
	    ret += ioobj->name();
	    if ( !full ) { delete ioobj; break; }
	    IOObj* parioobj = ioobj->getParent();
	    delete ioobj;
	    ioobj = parioobj;
	    if ( ioobj ) ret += " <- ";
	} while ( ioobj );
    }

    return ret;
}


void IOMan::back()
{
    to( prevkey );
}


const char* IOMan::curDir() const
{
    return dirptr ? dirptr->dirName() : (const char*)rootdir;
}


MultiID IOMan::key() const
{
    return MultiID(dirptr ? dirptr->key() : "");
}


MultiID IOMan::newKey() const
{
    return dirptr ? dirptr->newKey() : MultiID( "" );
}


bool IOMan::setDir( const char* dirname )
{
    if ( !dirname ) dirname = rootdir;

    IODir* newdirptr = new IODir( dirname );
    if ( !newdirptr ) return false;
    if ( newdirptr->bad() )
    {
	delete newdirptr;
	return false;
    }

    prevkey = key();
    bool needtrigger = dirptr;
    delete dirptr;
    dirptr = newdirptr;
    curlvl = levelOf( curDir() );
    if ( needtrigger )
	newIODir.trigger();
    return true;
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
	MultiID newkey( mktmp ? ctio.ctxt.getSelKey() : newKey() );
	if ( mktmp )
	    newkey.add( IOObj::tmpID() );
	IOStream* iostrm = new IOStream( ctio.ctxt.name(), newkey, false );
	dirPtr()->mkUniqueName( iostrm );
	iostrm->setGroup( ctio.ctxt.trgroup->userName() );
	const Translator* tr = ctio.ctxt.trgroup->templates().size() ?
	    			ctio.ctxt.trgroup->templates()[0] : 0;
	BufferString trnm( ctio.ctxt.deftransl.isEmpty()
			 ? (tr ? tr->userName().buf() : "")
			 : ctio.ctxt.deftransl.buf() );
	if ( trnm.isEmpty() )
	    trnm = ctio.ctxt.stdseltype == IOObjContext::Seis ? "CBVS" : "dGB";
	iostrm->setTranslator( trnm );

	// Generate the right filename
	Translator* tmptr = ctio.ctxt.trgroup->make( trnm );
	BufferString fnm = generateFileName( tmptr, iostrm->name() );
	int ifnm = 1;
	while ( File_exists(fnm) )
	{
	    BufferString altfnm( iostrm->name() );
	    altfnm += ifnm; fnm = generateFileName( tmptr, altfnm );
	    ifnm++;
	}
	iostrm->setFileName( fnm );
	delete tmptr;

	ioobj = iostrm;
	if ( ctio.ctxt.includeconstraints && !ctio.ctxt.allowcnstrsabsent )
	    ioobj->pars().merge( ctio.ctxt.parconstraints );

	dirPtr()->addObj( (IOObj*)ioobj );
    }

    ctio.setObj( ioobj->clone() );
}


const char* IOMan::generateFileName( Translator* tr, const char* fname )
{
    BufferString cleanname( fname );
    char* ptr = cleanname.buf();
    cleanupString( ptr, mC_False, *ptr == *FilePath::dirSep(FilePath::Local),
	    	   mC_True );
    static BufferString fnm;
    for ( int subnr=0; ; subnr++ )
    {
	fnm = cleanname;
	if ( subnr ) fnm += subnr;
	if ( tr && tr->defExtension() )
	    { fnm += "."; fnm += tr->defExtension(); }
	if ( !File_exists(fnm) ) break;
    }

    return fnm;
}


bool IOMan::setFileName( MultiID newkey, const char* fname )
{
    IOObj* ioobj = get( newkey );
    if ( !ioobj ) return false;
    const FileNameString fulloldname = ioobj->fullUserExpr( true );
    ioobj->setName( fname );
    mDynamicCastGet(IOStream*,iostrm,ioobj)
    if ( !iostrm ) return false;

    Translator* tr = ioobj->getTranslator();
    BufferString fnm = generateFileName( tr, fname );
    FilePath fp( fulloldname );
    if ( fp.isAbsolute() )
	fp.setFileName( fnm );
    else
	fp.set( fnm );
    iostrm->setFileName( fp.fullPath() );
  
    const FileNameString fullnewname = iostrm->fullUserExpr(true); 
    int ret = File_rename( fulloldname, fullnewname );
    if ( !ret || !commitChanges( *ioobj ) )
	return false;

    return true;
}


int IOMan::levelOf( const char* dirnm ) const
{
    if ( !dirnm ) return 0;

    int lendir = strlen(dirnm);
    int lenrootdir = strlen(rootdir);
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


bool IOMan::haveEntries( const MultiID& id,
			 const char* trgrpnm, const char* trnm ) const
{
    IODir iodir( id );
    const bool chkgrp = trgrpnm && *trgrpnm;
    const bool chktr = trnm && *trnm;
    const int sz = iodir.size();
    for ( int idx=1; idx<sz; idx++ )
    {
	const IOObj& ioobj = *iodir[idx];
	if ( chkgrp && strcmp(ioobj.group(),trgrpnm) )
	    continue;
	if ( chktr && strcmp(ioobj.translator(),trnm) )
	    continue;
	return true;
    }
    return false;
}


bool IOMan::commitChanges( const IOObj& ioobj )
{
    IOObj* clone = ioobj.clone();
    to( clone->key() );
    bool rv = dirPtr() ? dirPtr()->commitChanges( clone ) : false;
    delete clone;
    return rv;
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
    bool		createRootEntry();

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
    cleanupString( dd->dirname_.buf(), mC_False, mC_False, mC_False );

    int nr = dd->selkey_.ID( 0 );
    if ( nr <= 200000 )
	mErrRet("Invalid selection key passed for '",dd->desc_,"'")

    dd->selkey_ = "";
    dd->selkey_.setID( 0, nr );

    return true;
}


bool SurveyDataTreePreparer::prepSurv()
{
    if ( IOM().bad() ) { errmsg_ = "Bad directory"; return false; }

    PtrMan<IOObj> ioobj = IOM().get( dirdata_.selkey_ );
    if ( ioobj ) return true;

    IOM().to( 0 );
    if ( IOM().bad() ) { errmsg_ = "Can't go to root of survey"; return false; }
    if ( !createDataTree() )
	return false;

    // Maybe the parent entry is already there, then this would succeeed now:
    ioobj = IOM().get( dirdata_.selkey_ );
    if ( ioobj ) return true;

    return createRootEntry();
}


bool SurveyDataTreePreparer::createDataTree()
{
    if ( !IOM().dirPtr() ) { errmsg_ = "Invalid current survey"; return false; }

    FilePath fp( IOM().dirPtr()->dirName() );
    fp.add( dirdata_.dirname_ );
    const BufferString thedirnm( fp.fullPath() );
    bool dircreated = false;
    if ( !File_exists(thedirnm) )
    {
	if ( !File_createDir(thedirnm,0) )
	    mErrRet( "Cannot create '", dirdata_.dirname_,
		     "' directory in survey");
	dircreated = true;
    }

    fp.add( ".omf" );
    const BufferString omffnm( fp.fullPath() );
    if ( File_exists(omffnm) )
	return true;

    StreamData sd = StreamProvider( fp.fullPath() ).makeOStream();
    if ( !sd.usable() )
    {
	if ( dircreated )
	    File_remove( thedirnm, mFile_Recursive );
	mErrRet( "Could not create '.omf' file in ", dirdata_.dirname_,
		 " directory" );
    }

    *sd.ostrm << GetProjectVersionName();
    *sd.ostrm << "\nObject Management file\n";
    *sd.ostrm << Time_getFullDateString();
    *sd.ostrm << "\n!\nID: " << dirdata_.selkey_ << "\n!\n"
	      << dirdata_.desc_ << ": 1\n"
	      << dirdata_.desc_ << " directory: Gen`Stream\n"
	     	"$Name: Main\n!"
	      << std::endl;
    sd.close();
    return true;
}


bool SurveyDataTreePreparer::createRootEntry()
{
    BufferString parentry( "@" ); parentry += dirdata_.selkey_;
    parentry += ": "; parentry += dirdata_.dirname_;
    parentry += "\n!\n";
    std::string parentrystr( parentry.buf() );
    std::istringstream parstrm( parentrystr );
    ascistream ascstrm( parstrm, false ); ascstrm.next();
    IOLink* iol = IOLink::get( ascstrm, 0 );
    if ( !IOM().dirPtr()->addObj(iol,true) )
	mErrRet( "Couldn't add ", dirdata_.dirname_, " directory to root .omf" )
    return true;
}


TypeSet<IOMan::CustomDirData>& getCDDs()
{
    static TypeSet<IOMan::CustomDirData>* cdds = 0;
    if ( !cdds )
	cdds = new TypeSet<IOMan::CustomDirData>;
    return *cdds;
}


const MultiID& IOMan::addCustomDataDir( const IOMan::CustomDirData& dd )
{
    SurveyDataTreePreparer sdtp( dd );
    if ( !sdtp.prepDirData() )
    {
	ErrMsg( sdtp.errmsg_ );
	static MultiID none( "" );
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


bool OD_isValidRootDataDir( const char* d )
{
    FilePath fp( d ? d : GetBaseDataDir() );
    if ( !File_isDirectory( fp.fullPath() ) ) return false;

    fp.add( ".omf" );
    if ( !File_exists( fp.fullPath() ) ) return false;

    fp.setFileName( ".survey" );
    if ( File_exists( fp.fullPath() ) )
	return false;

    return true;
}

/* Hidden function, not to be used lightly. Basically, changes DTECT_DATA */
const char* OD_SetRootDataDir( const char* inpdatadir )
{
    BufferString datadir = inpdatadir;

    if ( !OD_isValidRootDataDir(datadir) )
	return "Provided directory name is not a valid OpendTect root data dir";

    const bool haveenv = GetEnvVar("DTECT_DATA") || GetEnvVar("dGB_DATA")
		      || GetEnvVar("DTECT_WINDATA") || GetEnvVar("dGB_WINDATA");
    if ( haveenv )
    {
#ifdef __win__
	FilePath dtectdatafp( datadir.buf() );
	
	SetEnvVar( "DTECT_WINDATA", dtectdatafp.fullPath(FilePath::Windows) );

	if ( GetOSEnvVar( "DTECT_DATA" ) )
	    SetEnvVar( "DTECT_DATA", dtectdatafp.fullPath(FilePath::Unix) );
#else
	SetEnvVar( "DTECT_DATA", datadir.buf() );
#endif
    }

    Settings::common().set( "Default DATA directory", datadir );
    if ( !Settings::common().write() )
    {
	if ( !haveenv )
	    return "Cannot write the user settings defining the "
		    "OpendTect root data dir";
    }

    IOMan::newSurvey();
    return 0;
}


void IOMan::enableSurveyChangeTriggers( bool yn )
{
    survchg_triggers = yn;
}
