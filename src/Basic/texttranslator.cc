/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Feb 2014
________________________________________________________________________

-*/

#include "texttranslator.h"

#include "dirlist.h"
#include "filepath.h"
#include "oddirs.h"
#include "ptrman.h"
#include "settings.h"
#include "uistrings.h"

#ifndef OD_NO_QT
# include <QTranslator>
# include <QLocale>
#endif

static const char* sLocalizationKey() { return "Language"; }


void TextTranslateMgr::GetLocalizationDir( File::Path& res )
{
    res = File::Path( mGetSWDirDataDir(), "localizations" );
}


namespace OD
{

mGlobal(Basic) void loadLocalization();
void loadLocalization()
{
    File::Path basedir;
    TextTranslateMgr::GetLocalizationDir( basedir );
    const DirList dl( basedir.fullPath(), File::FilesInDir, "*.qm" );

    const char* accepted_languages[] = { "en-us", "cn-cn", 0 };
    //This should really be done on build-level, buy as od6 is released,
    //the installer will not remove those qm-files.

    for( int idx=0; idx<dl.size(); idx++ )
    {
	const File::Path path = dl.fullPath( idx );
	const BufferString filename = path.baseName();
	const char* applicationend =
		filename.find( TextTranslateMgr::cApplicationEnd() );
	if ( !applicationend )
	    continue;

	const BufferString locale = applicationend+1;
	if ( getIndexInStringArrCI(locale.buf(),accepted_languages,0,0,-1)==-1 )
	    continue;

	RefMan<TextTranslatorLanguage> trans =
		new TextTranslatorLanguage( locale );
	TrMgr().addLanguage( trans );
    }

    Settings& setts = Settings::common();
    BufferString locale;
    if ( setts.get(sLocalizationKey(),locale) )
	TrMgr().setLanguageByLocaleKey( locale );
}

} // namespace OD


TextTranslatorLanguage::TextTranslatorLanguage( const char* localekey )
    : loaded_( false )
#ifndef OD_NO_QT
    , locale_( new QLocale(localekey) )
    , languagename_( new QString )
    , localekey_( localekey )
#endif
{
    translators_.setNullAllowed( true );
    const BufferString filename = BufferString()
	.add( uiString::sODLocalizationApplication() )
	.add( TextTranslateMgr::cApplicationEnd() )
	.add( localekey )
	.add( ".qm" );

    File::Path locdir;
    TextTranslateMgr::GetLocalizationDir( locdir );
    const File::Path odfile( locdir, filename );

#ifndef OD_NO_QT
    QTranslator maintrans;
    if ( !maintrans.load(odfile.fullPath().buf()) )
	*languagename_ = localekey;
    else
    {
	uiString name = tr("Language Name",0,1);
	name.addLegacyVersion( uiString(name.getOriginalString(),
			       "TextTranslateMgr",
			       uiString::sODLocalizationApplication(), 0, 1 ));
	name.translate( maintrans, *languagename_ );
	//Force be a part of the plural setup
    }

    locale_->setNumberOptions( QLocale::OmitGroupSeparator );
#endif
}


TextTranslatorLanguage::~TextTranslatorLanguage()
{
#ifndef OD_NO_QT
    deepErase( translators_ );
    delete locale_;
    delete languagename_;
#endif
}

const QTranslator*
TextTranslatorLanguage::getTranslator( const char* appl ) const
{
    const int idx = applications_.indexOf( appl );
    return translators_.validIdx(idx) ? translators_[idx] : 0;
}


BufferString TextTranslatorLanguage::getLocaleKey() const
{
    return localekey_;
}


const mQtclass(QString)& TextTranslatorLanguage::getLanguageName() const
{
    return *languagename_;
}


const mQtclass(QLocale)& TextTranslatorLanguage::getLanguageLocale() const
{
    return *locale_;
}


bool TextTranslatorLanguage::load()
{
    if ( loaded_ )
	return true;

    loaded_ = true;

#ifdef OD_NO_QT
    return false;
#else
    const BufferString filenamesearch = BufferString("*")
	.add( TextTranslateMgr::cApplicationEnd() )
	.add( localekey_ )
	.add( ".qm" );

    File::Path basedir;
    TextTranslateMgr::GetLocalizationDir(basedir);
    DirList dl( basedir.fullPath(), File::FilesInDir, filenamesearch.buf() );

    for( int idx=0; idx<dl.size(); idx++ )
    {
	const File::Path filepath = dl.fullPath( idx );
	BufferString filename = filepath.baseName();

	BufferString application;
	char* applicationend =
		filename.find(TextTranslateMgr::cApplicationEnd());
	if ( !applicationend )
	    continue;

	application = filename;
	*application.find(TextTranslateMgr::cApplicationEnd()) = 0;

	BufferString language = applicationend+1;

	QTranslator* trans = new QTranslator;
	if ( !trans->load(filepath.fullPath().buf()) )
	{
	    delete trans;
	    continue;
	}

	translators_ += trans;
	applications_.add( application );
    }

    return true;
#endif
}


// TextTranslateMgr
mGlobal(Basic) TextTranslateMgr& TrMgr()
{
    mDefineStaticLocalObject( PtrMan<TextTranslateMgr>, trmgr, = 0 );
    if ( !trmgr ) trmgr = new TextTranslateMgr();
    return *trmgr;
}


TextTranslateMgr::TextTranslateMgr()
    : dirtycount_(0)
    , currentlanguageidx_(-1)
    , languageChange(this)
{
    loadUSEnglish();
    setLanguage( 0 );
}


TextTranslateMgr::~TextTranslateMgr()
{
    deepUnRef( languages_ );
}


int TextTranslateMgr::nrSupportedLanguages() const
{
    return languages_.size();
}


bool TextTranslateMgr::addLanguage( TextTranslatorLanguage* language )
{
    if ( !language )
	return false;

    language->ref();
    const BufferString newkey = language->getLocaleKey();
    for ( int idx=0; idx<languages_.size(); idx++ )
    {
	if ( languages_[idx]->getLocaleKey() == newkey )
	    { language->unRef(); return false; }
    }

    languages_ += language;
    languageChange.trigger();
    return true;
}


uiString TextTranslateMgr::getLanguageUserName( int idx ) const
{
    if ( languages_.validIdx(idx) )
    {
	uiString ret;
#ifdef OD_NO_QT
	ret = toUiString(languages_[idx]->getLocaleKey());
#else
	ret.setFrom( languages_[idx]->getLanguageName() );
#endif
	return ret;
    }

    return toUiString("Unknown language");
}


BufferString TextTranslateMgr::getLocaleKey( int idx ) const
{
    if ( languages_.validIdx(idx) )
	return languages_[idx]->getLocaleKey();

    return "en_us";
}


void TextTranslateMgr::storeToUserSettings()
{
    Settings& setts = Settings::common();
    const BufferString lky = getLocaleKey( currentLanguage() );
    BufferString curlky;
    if ( setts.get(sLocalizationKey(),curlky) && curlky==lky )
	return;

    setts.set( sLocalizationKey(), lky );
    setts.write();
}


uiRetVal TextTranslateMgr::setLanguage( int idx )
{
    if ( idx < 0 )
	idx = 0;
    if ( idx==currentlanguageidx_ )
	return uiRetVal::OK();

    if ( !languages_.validIdx(idx) )
	return uiRetVal( uiStrings::phrInternalError("Bad language index") );

    if ( !languages_[idx]->load() )
	return uiRetVal( tr("Cannot load %1").arg( getLanguageUserName(idx) ) );

    currentlanguageidx_ = idx;
    dirtycount_++;
    languageChange.trigger();
    return uiRetVal::OK();
}


uiRetVal TextTranslateMgr::setLanguageByLocaleKey( const char* lky )
{
    for ( int idx=0; idx<TrMgr().nrSupportedLanguages(); idx++ )
    {
	if ( getLocaleKey(idx) == lky )
	    { setLanguage( idx ); return uiRetVal::OK(); }
    }
    return uiRetVal( tr("Cannot find locale '%1'").arg( lky ) );
}


int TextTranslateMgr::currentLanguage() const
{
    return currentlanguageidx_;
}


const QTranslator*
    TextTranslateMgr::getQTranslator( const char* application ) const
{
    return languages_.validIdx(currentlanguageidx_)
	? languages_[currentlanguageidx_]->getTranslator( application )
	: 0;
}


const QLocale* TextTranslateMgr::getQLocale() const
{
#ifndef OD_NO_QT
    return languages_.validIdx(currentlanguageidx_)
	? &languages_[currentlanguageidx_]->getLanguageLocale()
        : 0;
#else
    return 0;
#endif
}


void TextTranslateMgr::loadUSEnglish()
{
    RefMan<TextTranslatorLanguage> english =
				new TextTranslatorLanguage("en-us");
    addLanguage( english );
}
