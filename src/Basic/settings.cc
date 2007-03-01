/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 4-11-1995
 * FUNCTION : Default user settings
-*/
 
static const char* rcsID = "$Id: settings.cc,v 1.34 2007-03-01 10:49:47 cvsbert Exp $";

#include "settings.h"
#include "filegen.h"
#include "filepath.h"
#include "ascstream.h"
#include "safefileio.h"
#include "oddirs.h"
#include "errh.h"
#include <filegen.h>

static const char* sKeyDeflt = "Default settings";
static const char* sKeyCommon = "Common";
#define mGetKey(key) (key && *key ? key : sKeyCommon)
#define mIsCommon(key) (!key || !*key || !strcmp(key,sKeyCommon))

static ObjectSet<Settings>& getSetts()
{
    static ObjectSet<Settings>* theinst_ = 0;
    if ( !theinst_ )
	theinst_ = new ObjectSet<Settings>;
    return *theinst_;
}


static BufferString getFileName( const char* key, const char* dtectusr,
				 const char* dirnm )
{
    FilePath fp( dirnm ? dirnm : GetSettingsDir() );
    fp.add( "settings" );
    BufferString fname = fp.fullPath();
    if ( !mIsCommon(key) )
	{ fname += "_"; fname += key; }
    if ( dtectusr && *dtectusr )
	{ fname += "."; fname += dtectusr; }
    return fname;
}


Settings& Settings::fetch( const char* key )
{
    const char* settkey = mGetKey( key );
    ObjectSet<Settings>& settlist = getSetts();
    for ( int idx=0; idx<settlist.size(); idx++ )
	if ( settlist[idx]->name() == settkey )
	    return *settlist[idx];

    Settings* newsett = doFetch( key, GetSoftwareUser(), GetSettingsDir(),
	    			 false );
    if ( !newsett )
    {
	if ( mIsCommon(key) )
	    ErrMsg( "Cannot find valid settings file in .od directory" );
	newsett = new Settings( settkey );
    }

    newsett->setName( settkey );
    newsett->fname = getFileName( key, GetSoftwareUser(), GetSettingsDir() );
    settlist += newsett;
    return *newsett;
}


Settings* Settings::fetchExternal( const char* key, const char* dtectusr,
				   const char* dirnm )
{
    return doFetch( key, dtectusr, dirnm, true );
}


Settings* Settings::doFetch( const char* key, const char* dtectusr,
			     const char* dirnm, bool ext )
{
    BufferString fname( getFileName(key,dtectusr,dirnm) );

    Settings* ret = new Settings( fname );
    ret->setName( mGetKey(key) );
    if ( !ret->doRead(ext) )
	{ delete ret; ret = 0; }

    return ret;
}


bool Settings::doRead( bool ext )
{
    const bool empty_initially = File_isEmpty(fname);
    const bool iscommon = name() == sKeyCommon;

    SafeFileIO sfio( fname, false );
    if ( empty_initially || !sfio.open(true) )
    {
	if ( ext ) return false;

	BufferString tmplfname( iscommon ? "od" : name().buf() );
	tmplfname += "Settings";
	tmplfname = GetDataFileName(tmplfname);
	bool okaftercopy = false;
	if ( File_exists(tmplfname) )
	{
	    File_copy( tmplfname, fname, NO );
	    if ( sfio.open(true) )
		okaftercopy = true;
	}
	if ( !okaftercopy )
	{
	    if ( iscommon )
		ErrMsg( sfio.errMsg() );
	    return false;
	}
    }

    ascistream stream( sfio.istrm(), true );
    if ( !stream.isOfFileType( sKeyDeflt ) )
    {
	BufferString emsg = "User settings file '";
	emsg += fname;
	emsg += "' seems to be corrupted.";
	sfio.closeFail();
	return false;
    }

    clear();
    getFrom( stream );
    sfio.closeSuccess();

    if ( empty_initially )
	write( false );
    else if ( iscommon && stream.majorVersion() < 3 )
    {
	IOPar* coltabpar = subselect( "Color table" );
	bool havewrittencoltabs = false;
	if ( coltabpar && coltabpar->size() )
	{
	    Settings& coltabsetts( fetch("coltabs") );
	    coltabsetts.merge( *coltabpar );
	    coltabsetts.write( false );
	    /* For 3.2 or later:
	    if ( coltabsetts.write(false) )
		{ removeWithKey( "Color table.*" ); write( false ); }
		*/
	}
	delete coltabpar;
    }
    return true;
}


bool Settings::write( bool do_merge ) const
{
    if ( do_merge )
    {
	IOPar dup( *this );
	Settings* me = const_cast<Settings*>( this );
	me->reRead();
	me->merge( dup );
    }

    SafeFileIO sfio( fname );
    if ( !sfio.open(false) )
    {
	BufferString msg( "Cannot open user settings file for write" );
	if ( *sfio.errMsg() )
	{ msg += "\n\t"; msg += sfio.errMsg(); }
	    ErrMsg( msg );
	return false;
    }

    ascostream stream( sfio.ostrm() );
    stream.putHeader( sKeyDeflt );
    putTo( stream );
    if ( !sfio.closeSuccess() )
    {
	BufferString msg( "Error closing user settings file:\n" );
	msg += sfio.errMsg();
	ErrMsg( msg );
	return false;
    }

    return true;
}


extern "C" const char* GetSettingsDataDir()
{
    static BufferString dirnm;
    Settings::common().get( "Default DATA directory", dirnm );
    return dirnm.buf();
}
