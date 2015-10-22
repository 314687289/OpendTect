/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistring.h"

#include "bufstring.h"
#include "envvars.h"
#include "keystrs.h"
#include "od_iostream.h"
#include "perthreadrepos.h"
#include "ptrman.h"
#include "refcount.h"
#include "texttranslator.h"
#include "typeset.h"

#ifndef OD_NO_QT
# include <QString>
# include <QTranslator>
#endif

#define mDirty (-1)


class uiStringData
{ mRefCountImplNoDestructor(uiStringData)
  friend class uiString;
public:
    uiStringData( const char* originalstring, const char* context,
		  const char* application,
		  const char* disambiguation, int pluralnr )
	: originalstring_( originalstring )
	, translationcontext_( context )
	, translationpluralnumber_( pluralnr )
	, translationdisambiguation_( disambiguation )
	, application_( application )
	, contentlock_( true )
	, dirtycount_( mDirty )
    {
    }

    void setFrom( const uiStringData& d )
    {
	Threads::Locker contentlocker( contentlock_ );
	originalstring_ = d.originalstring_;
	translationcontext_ = d.translationcontext_;
	translationpluralnumber_ = d.translationpluralnumber_;
	translationdisambiguation_ = d.translationdisambiguation_;
	arguments_ = d.arguments_;
	application_ = d.application_;
	dirtycount_ = mDirty;
    }

    void addLegacyVersion( const uiString& legacy )
    {
	Threads::Locker contentlocker( contentlock_ );
        legacyversions_.add( legacy );
	dirtycount_ = mDirty;
    }

    void setFrom( const QString& qstr )
    {
#ifndef OD_NO_QT
	Threads::Locker contentlocker( contentlock_ );
	set( 0 );
	qstring_ = qstr;
#endif
    }

    void getFullString( BufferString& ) const;

    void set(const char* orig);
    bool fillQString(QString&,const QTranslator* translator=0) const;

    TypeSet<uiString>		arguments_;

#ifndef OD_NO_QT
    QString			qstring_;
#endif

    BufferString		originalstring_;
    TypeSet<uiString>		legacyversions_;
    const char*			translationcontext_;
    const char*			application_;
    const char*			translationdisambiguation_;
    int				translationpluralnumber_;

    int				dirtycount_;

    mutable Threads::Lock	contentlock_;
};


void uiStringData::set( const char* orig )
{
    Threads::Locker contentlocker( contentlock_ );
    originalstring_ = orig;
    arguments_.erase();
    legacyversions_.erase();
    translationcontext_ = 0;
    application_ = 0;
    translationdisambiguation_ = 0;
    translationpluralnumber_ = -1;
    dirtycount_ = mDirty;
#ifndef OD_NO_QT
    qstring_ = sKey::EmptyString().buf();
#endif
}


void uiStringData::getFullString( BufferString& ret ) const
{
    Threads::Locker contentlocker( contentlock_ );
    if ( !arguments_.size() )
    {
	ret = originalstring_;
	return;
    }

#ifndef OD_NO_QT
    QString qres;
    fillQString( qres, 0 );
    ret = qres;
#else
    ret = originalstring_;
#endif
}


bool uiStringData::fillQString( QString& res,
				const QTranslator* translator ) const
{
#ifndef OD_NO_QT
    Threads::Locker contentlocker( contentlock_ );
    if ( !originalstring_ || !*originalstring_ )
	return false;

    bool translationres = false;

    if ( translator )
    {
	res = translator->translate( translationcontext_, originalstring_,
				     translationdisambiguation_,
				     translationpluralnumber_ );

        if ( QString(originalstring_.buf())!=res )
            translationres = true;

        if ( legacyversions_.size() && !translationres )
        {
            for ( int idx=0; idx<legacyversions_.size(); idx++ )
            {
                QString legacytrans;
                if ( legacyversions_[idx].translate( *translator, legacytrans) )
                {
                    res = legacytrans;
                    translationres = true;
                    break;
                }
            }
        }
	mDefineStaticLocalObject(bool,dbgtransl,
				 = GetEnvVarYN("OD_DEBUG_TRANSLATION"));
	if ( dbgtransl )
	{
	    BufferString info( translationcontext_, " : " );
	    info.add( originalstring_ ).add( " : " )
		.add( translationdisambiguation_ ).add( " : " )
		.add( translationpluralnumber_ ).add( " : " )
		.add( res );
	    od_cout() << info << od_endl;
	}
    }

    if ( !translator || res.isEmpty() )
    {
	res = originalstring_;
    }

    for ( int idx=0; idx<arguments_.size(); idx++ )
	res = res.arg( arguments_[idx].getQtString() );

    return translationres;
#else
    return true;
#endif
}


uiString::uiString( const char* str )
    : data_( new uiStringData( 0, 0, 0, 0, -1 ) )
{
    data_->ref();
    *this = str;
}


uiString::uiString( const char* originaltext, const char* context,
		    const char* application,
		    const char* disambiguation, int pluralnr )
    : data_( new uiStringData(originaltext, context, application,
			      disambiguation, pluralnr ))
{
    data_->ref();
}



uiString::uiString( const uiString& str )
    : data_( str.data_ )
    , datalock_( true )
{
    data_->ref();
}


uiString::uiString( const OD::String& str )
    : data_( new uiStringData( 0, 0, 0, 0, -1 ) )
    , datalock_( true )
{
    data_->ref();
    *this = str;
}


uiString::~uiString()
{
    data_->unRef();
}


void uiString::addLegacyVersion( const uiString& legacy )
{
    Threads::Locker datalocker( datalock_ );
    makeIndependent();
    data_->addLegacyVersion( legacy );
}


bool uiString::isEmpty() const
{
    Threads::Locker datalocker( datalock_ );
    Threads::Locker contentlocker( data_->contentlock_ );
    return data_->originalstring_.isEmpty();
}


void uiString::setEmpty()
{
    *this = sKey::EmptyString();
}


static uiString emptystring;


const uiString& uiString::emptyString()
{
    return emptystring;
}


const char* uiString::getOriginalString() const
{
    Threads::Locker datalocker( datalock_ );
    Threads::Locker contentlocker( data_->contentlock_ );
    return data_->originalstring_;
}


const OD::String& uiString::getFullString( BufferString* res ) const
{
    Threads::Locker datalocker( datalock_ );
    data_->getFullString( *res );
    return *res;
}


const OD::String& uiString::getFullString() const
{
    mDeclStaticString( res );
    return getFullString( &res );
}


const QString& uiString::getQtString() const
{
#ifndef OD_NO_QT
    Threads::Locker datalocker( datalock_ );
    Threads::Locker contentlocker( data_->contentlock_ );
    if ( data_->dirtycount_!=TrMgr().dirtyCount() )
    {
	data_->fillQString( data_->qstring_,
			TrMgr().getQTranslator(data_->application_) );
	data_->dirtycount_ = TrMgr().dirtyCount();
    }

    return data_->qstring_;
#else
    QString* ptr = 0;
    return *ptr;
#endif
}


wchar_t* uiString::createWCharString() const
{
#ifndef OD_NO_QT
    Threads::Locker datalocker( datalock_ );
    QString qstr;
    Threads::Locker contentlocker( data_->contentlock_ );
    data_->fillQString( qstr, TrMgr().getQTranslator(data_->application_) );
    if ( !qstr.size() )
	return 0;

    mDeclareAndTryAlloc( wchar_t*, res, wchar_t[qstr.size()+1] );

    if ( !res )
	return 0;

    const int nrchars = qstr.toWCharArray( res );
    res[nrchars] = 0;

    return res;
#else
    return 0;
#endif
}


uiString& uiString::operator=( const uiString& str )
{
    Threads::Locker datalocker( datalock_ );
    str.data_->ref();
    data_->unRef();
    data_ = str.data_;
    return *this;
}


void uiString::setFrom( const QString& qstr )
{
    Threads::Locker datalocker( datalock_ );
    makeIndependent();
    Threads::Locker contentlocker( data_->contentlock_ );
    data_->setFrom( qstr );
}


uiString& uiString::setFrom( const uiString& str )
{
    Threads::Locker datalocker( datalock_ );
    makeIndependent();
    Threads::Locker contentlocker( data_->contentlock_ );
    data_->setFrom( *str.data_ );
    return *this;
}


uiString& uiString::operator=( const OD::String& str )
{
    return operator=( str.str() );
}


uiString& uiString::operator=( const char* str )
{
    Threads::Locker datalocker( datalock_ );
    makeIndependent();
    data_->set( str );
    return *this;
}


bool uiString::operator>(const uiString& b ) const
{
#ifndef OD_NO_QT
    const QString& aqstr = getQtString();
    const QString& bqstr = b.getQtString();
    return aqstr > bqstr;
#else
    return true;
#endif
}


bool uiString::operator<(const uiString& b ) const
{
#ifndef OD_NO_QT
    const QString& aqstr = getQtString();
    const QString& bqstr = b.getQtString();
    return aqstr < bqstr;
#else
    return true;
#endif
}


uiString& uiString::arg( const uiString& newarg )
{
    Threads::Locker datalocker( datalock_ );
    makeIndependent();
    Threads::Locker contentlocker( data_->contentlock_ );
    data_->arguments_ += newarg;
    data_->dirtycount_ = mDirty;
    return *this;
}


uiString& uiString::append( const uiString& txt, bool withnewline )
{
    Threads::Locker datalocker( datalock_ );
    uiString self( *this );
    self.makeIndependent();
    RefMan<uiStringData> tmpptr = data_;

    Threads::Locker contentlocker( tmpptr->contentlock_ );

    if ( isEmpty() )
	withnewline = false;

    *this = uiString( withnewline ? "%1\n%2" : "%1%2").arg( self ).arg( txt );
    return *this;
}


uiString& uiString::append( const OD::String& a, bool withnewline )
{ return append( a.str(), withnewline ); }


uiString& uiString::append( const char* newarg, bool withnewline )
{
    return append( uiString(newarg), withnewline );
}


bool uiString::translate( const QTranslator& tr , QString& res ) const
{
    Threads::Locker datalocker( datalock_ );
    return data_->fillQString( res, &tr );
}


void uiString::makeIndependent()
{
    Threads::Locker datalocker( datalock_ );
    if ( data_->refcount_.count()==1 )
	return;

    RefMan<uiStringData> olddata = data_;
    data_->unRef();

    data_ = new uiStringData( 0, 0, 0, 0, -1 );
    data_->ref();

    data_->setFrom( *olddata );
}


uiString uiStringSet::createOptionString( bool use_and, char space ) const
{
    BufferString glue;


    const char* percentage = "%";
    uiStringSet arguments;
    const char spacestring[] = { space, 0 };

    arguments += spacestring;
    bool firsttime = true;

    for ( int idx=0; idx<size(); idx++ )
    {
	if ( (*this)[idx].isEmpty() )
	    continue;

	if ( firsttime )
	{
	    arguments += (*this)[idx];
	    glue.add( percentage );
	    glue.add( arguments.size() );

	    firsttime = false;

	    continue;
	}

	if ( idx==size()-1 )
	{
	    if ( size()==2 )
		glue.add( use_and ? " and%1%" : " or%1%" );
	    else
		glue.add( use_and ? ", and%1%" : ", or%1%");
	}
	else
	    glue.add(",%1%");

	arguments += (*this)[idx];
	glue.add( arguments.size() );
    }

    if ( glue.isEmpty() )
	return uiString();

    uiString res = glue;

    for ( int idx=0; idx<arguments.size(); idx++ )
	res.arg( arguments[idx] );

    return res;
}


