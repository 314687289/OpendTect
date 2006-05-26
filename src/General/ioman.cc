/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 3-8-1994
-*/

static const char* rcsID = "$Id: ioman.cc,v 1.61 2006-05-26 07:26:13 cvsnanne Exp $";

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
#include <stdlib.h>

IOMan*	IOMan::theinst_	= 0;
void	IOMan::stop()	{ delete theinst_; theinst_ = 0; }
extern "C" void SetSurveyName(const char*);
extern "C" const char* GetSurveyName();
extern "C" void SetSurveyNameDirty();
extern "C" const char* GetBaseDataDir();


static void clearSelHists()
{
    const ObjectSet<TranslatorGroup>& grps = TranslatorGroup::groups();
    const int sz = grps.size();
    for ( int idx=0; idx<grps.size(); idx++ )
	grps[idx]->clearSelHist();
}

#define mReset() \
{ \
    delete IOMan::theinst_; \
    IOMan::theinst_ = 0; \
}


bool IOMan::newSurvey()
{
    CallBackList cbs = IOM().surveyChanged.cbs;
    mReset();
    clearSelHists();
    SetSurveyNameDirty();
    IOM().surveyChanged.cbs = cbs;
    IOM().surveyChanged.trigger();

    return !IOM().bad();
}


void IOMan::setSurvey( const char* survname )
{
    CallBackList cbs = IOM().surveyChanged.cbs;
    mReset();
    clearSelHists();

    delete SurveyInfo::theinst_;
    SurveyInfo::theinst_ = 0;
    SetSurveyName( survname );
    IOM().surveyChanged.cbs = cbs;
    IOM().surveyChanged.trigger();
}


const char* IOMan::surveyName() const
{
    return GetSurveyName();
}


IOMan::IOMan( const char* rd )
	: UserIDObject("IO Manager")
	, dirptr(0)
	, state_(IOMan::NeedInit)
    	, newIODir(this)
    	, entryRemoved(this)
    	, surveyChanged(this)
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
    bool needwrite = false;
    FilePath basicfp = FilePath( GetDataFileName("BasicSurvey") );
    FilePath rootfp = FilePath( rootdir );
    basicfp.add( "X" ); rootfp.add( "X" );
    for ( int idx=0; idx<nrstddirdds; idx++ )
    {
	const IOObjContext::StdDirData* dd
		= IOObjContext::getStdDirData( (IOObjContext::StdSelType)idx );
	if ( (*dirPtr())[MultiID(dd->id)] ) { prevdd = dd; continue; }

	basicfp.setFileName( dd->dirnm );
	BufferString basicdirnm = basicfp.fullPath();
	if ( !File_exists(basicdirnm) )
	    // Apparently, the application doesn't need such a directory
	    { prevdd = dd; continue; }

	rootfp.setFileName( dd->dirnm );
	BufferString dirnm = rootfp.fullPath();
	if ( !File_exists(dirnm) )
	{
	    // Apparently, this directory should have been in the survey
	    // It is not. If it is a basic directory, we do not want to
	    // continue. Otherwise, we want to create the directory.
	    if ( !idx ) // 0=Seis, if this is missing, why go on?
	    {
		BufferString msg( "Corrupt survey: missing directory: " );
		msg += dirnm; ErrMsg( msg ); state_ = Bad; return;
	    }
	    else if ( !File_copy(basicdirnm,dirnm,YES) )
	    {
		BufferString msg( "Cannot create directory: " );
		msg += dirnm; ErrMsg( msg ); state_ = Bad; return;
	    }
	}

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
	    File_copy( fname, fp.fullPath(), NO );
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
    if ( basedatadir == "" )
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
    if ( File_exists(survfname) && !File_remove( survfname, NO ) )
    {
	errmsg = "The file "; errmsg += survfname;
	errmsg += " contains an invalid survey.\n";
	errmsg += "Please remove this file";
	return false;
    }

    CallBackList cbs = IOM().surveyChanged.cbs;
    mReset();
    IOM().surveyChanged.cbs = cbs;
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
    if ( !link && curlvl == 0 )
	return false;
    if ( bad() ) return link ? to( link->link()->key() ) : to( prevkey );

    FilePath fp( curDir() );
    fp.add( link ? (const char*)link->dirname : ".." );
    BufferString fulldir = fp.fullPath();
    if ( !File_isDirectory(fulldir) ) return false;

    prevkey = dirptr->key();
    return setDir( fulldir );
}


bool IOMan::to( const MultiID& ky )
{
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

    IODir* newdir = currentkey == ""
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


IOObj* IOMan::getFirst( const IOObjContext& ctxt ) const
{
    if ( !ctxt.trgroup ) return 0;

    IOM().to( ctxt.stdSelKey() );

    const ObjectSet<IOObj>& ioobjs = dirptr->getObjs();
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj* ioobj = ioobjs[idx];
	if ( ctxt.validIOObj(*ioobj) )
	    return ioobj->clone();
    }

    return 0;
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
    if ( !bad() ) return dirptr->dirName();
    return rootdir;
}


MultiID IOMan::key() const
{
    if ( !bad() ) return dirptr->key();
    return MultiID( "" );
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


MultiID IOMan::newKey() const
{
    if ( bad() ) return MultiID( "" );
    return dirptr->newKey();
}


void IOMan::getEntry( CtxtIOObj& ctio, MultiID parentkey )
{
    ctio.setObj( 0 );
    if ( ctio.ctxt.name() == "" ) return;

    if ( parentkey == "" )
	parentkey = ctio.ctxt.parentkey;
    if ( parentkey != "" )
	to( parentkey );
    else if ( ctio.ctxt.hasStdSelType()
	   && ( ctio.ctxt.newonlevel != 2
	     || dirPtr()->main()->key().nrKeys() < 3 ) )
	to( ctio.ctxt.stdSelKey() );

    const IOObj* ioobj = (*dirPtr())[ ctio.ctxt.name() ];
    ctio.ctxt.fillTrGroup();
    if ( ioobj && ctio.ctxt.trgroup->userName() != ioobj->group() )
	ioobj = 0;

    if ( !ioobj )
    {
	IOStream* iostrm = new IOStream( ctio.ctxt.name(), newKey(), NO );
	dirPtr()->mkUniqueName( iostrm );
	iostrm->setParentKey( parentkey );
	iostrm->setGroup( ctio.ctxt.trgroup->userName() );
	const Translator* tr = ctio.ctxt.trgroup->templates().size() ?
	    			ctio.ctxt.trgroup->templates()[0] : 0;
	BufferString trnm( ctio.ctxt.deftransl != ""
			 ? ctio.ctxt.deftransl.buf()
			 : (tr ? tr->userName().buf() : "") );
	if ( trnm == "" )
	    trnm = ctio.ctxt.stdseltype == IOObjContext::Seis ? "CBVS" : "dGB";
	iostrm->setTranslator( trnm );

	// Generate the right filename
	Translator* tmptr = ctio.ctxt.trgroup->make( trnm );
	const char* fnm = generateFileName( tmptr, ctio.ctxt.name() );
	iostrm->setFileName( fnm );
	delete tmptr;

	ioobj = iostrm;
	if ( ctio.ctxt.crlink )
	    ioobj = new IOLink( ioobj->name(), (IOObj*)ioobj );
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
    cleanupString( ptr, NO, *ptr == *FilePath::dirSep(FilePath::Local), YES );
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
