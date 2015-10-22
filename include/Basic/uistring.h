#ifndef uistring_h
#define uistring_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "gendefs.h"
#include "basicmod.h"
#include "keystrs.h"
#include "threadlock.h"

#include "typeset.h"

class uiStringData;

mFDQtclass( QString );
mFDQtclass( QTranslator );


/*!
   String that is able to hold wide character strings for the user interface.
   These strings can be in different encodings and should only be used to pass
   text to the ui.

   The string may have an original string encoded in ASCI characters, but that
   should only be used for object names and similar.

   If the string holds %N arguments, these can be replaced by arguments:

 \code
   uiString string = uiString( "%1 plus %2 is %3")
			.arg( 4 )
			.arg( 5 )
			.arg( 4+5 );
 \endcode

   Will result in the string "4 plus 5 is 9"

 \note As multiple uiStrings may use the same underlying data (if they are
       constructed with the copy constructor or the equal operator, they are not
       suited for other types of string operations than passing messages to the
       user.

 The translation in OpendTect is done using Qt's subsystem for localization.
 A class that wishes to enable localization should:

  -# Declare the mTextTranslationClass(classname,application) in its class
     definition. The application is a string that identifies your application.
     OpendTect's internal classes use the "od" applicaiton string, and can for
     short use the mODTextTranslationClass macro.
  -# Use the tr() function for all translatable string. The tr() function
     returns a uiString() that can be passed to the ui.
  -# Use Qt's lupdate to scan your code for localization strings. This will
     generate a .ts file which can be editded with Qt's Linguist program to
     translate the strings.
  -# The updated .ts file should be converted to a binary .qm file using Qt's
     lrelease application.
  -# The .qm file should be placed in
     data/localizations/<application>_<lang>_<country>.ts in the release. For
     example, a localization of OpendTect to traditional Chinese/Taiwan would be
     saved as od_zh_TW.qm.
 */


mExpClass(Basic) uiString
{
public:
		uiString(const uiString&);
		/*!<\note Does not copy data, will use the same underlying data
			  structure (reference counted). */
		uiString(const char* original = 0);
		uiString(const OD::String&);
		~uiString();

    bool	isEmpty() const;
    void	setEmpty();
    bool	isSet() const		{ return !isEmpty(); }
    bool	operator!() const	{ return isEmpty(); }

    uiString&	operator=(const uiString&);
		/*!<\note Does not copy data, will use the same underlying data
			  structure (reference counted). */
    uiString&	operator=(const char*);
    uiString&	operator=(const OD::String&);
    uiString&	set( const char* s )	{ *this = s; return *this; }

    template <class T>
    uiString&	arg(const T&);
		/*!<Replaces the %N (e.g. %1) with the lowest N with the
		    provided string. */
    uiString&	arg(const uiString&);
		/*!<Replaces the %N (e.g. %1) with the lowest N with the
		    provided string. */

    uiString&	append(const char*,bool withnewline=false);
		/*!Appends string with provided string. In most cases, use arg
		   to allow translator to change ordering.
		   \param withnewline will add a newline character before the
			  appended string if current string is not empty.*/
    uiString&	append(const OD::String&,bool withnewline=false);
		/*!Appends string with provided string. In most cases, use arg
		   to allow translator to change ordering.
		   \param withnewline will add a newline character before the
			  appended string if current string is not empty.*/
    uiString&	append(const uiString&,bool withnewline=false);
		/*!Appends string with provided string. In most cases, use arg
		   to allow translator to change ordering.
		   \param withnewline will add a newline character before the
			  appended string if current string is not empty.*/

    const OD::String&		getFullString() const;
				/*!<Constructs the result from the original
				    string and the arguments,
				    without translation. \Note that
				    result is in a thread-safe static buffer, so
				    copy the result before calling again.*/
    const OD::String&		getFullString(BufferString*) const;
				/*!< Full string, *without* translation
				    result is put in the provided buffer. */
    const char*			getOriginalString() const;
    const mQtclass(QString&)	getQtString() const;
    wchar_t*			createWCharString() const;
				/*!<Result becomes owners and should be
				    deleted using the [] operator. */

    static const char*		sODLocalizationApplication() { return "od"; }
    static const uiString&	emptyString();

    bool	operator==(const uiString& b) const { return b.data_==data_; }
    bool	operator!=(const uiString& b) const { return b.data_!=data_; }
    bool	operator>(const uiString& b) const;
    bool	operator<(const uiString& b) const;

private:

    friend class		uiStringData;
    uiStringData*		data_;
    mutable Threads::Lock	datalock_;

public:

		//Only for expert users
    void	makeIndependent();
		//!<If data is shared, I'll get an own copy
		uiString(const char* original,
			 const char* context,
			 const char* application,
			 const char* disambiguation,
			 int pluralnr);
    uiString&	setFrom(const uiString&);
		//!<Will be gone in coming releases. Don't use.
    void	setFrom(const mQtclass(QString)&);
		/*!<Set the translated text no further
		    translation will be done. */
    void	addLegacyVersion(const uiString&);
		/*!<If this string was previously known by another origial
		    string, it can be added here. This is normally done with the
		    legacyTr function.
		    \code
			uiString str = tr("New version");
			str.addLegacyVersion( legacyTr("Old ver") );
		    \endcode
                */

    bool	translate(const mQtclass(QTranslator)&,
			  mQtclass(QString)&) const;
		//!<Returns true if the translation succeeded

};

mExpClass(Basic) uiStringSet : public TypeSet<uiString>
{
public:
		uiStringSet()				{}
		uiStringSet(const uiStringSet& sl) : TypeSet<uiString>(sl)  {}
		uiStringSet(const uiString& s)		{ add( s ); }

    uiString	createOptionString(bool use_and=true,char space = ' ') const;
		//!<Returns a string with "option1, option2, and/or option 3"
};


template <class T> inline
uiString& uiString::arg( const T& thearg )
{
    const uiString newarg( toString( thearg ) );
    return arg( newarg );
}


//!Don't use in new code. Use uiString directly.
typedef uiString uiStringCopy;


#define mTextTranslationClass(clss,application) \
private: \
 static inline uiString tr( const char* text, const char* disambiguation = 0,  \
 int pluralnr=-1 ) \
 { return uiString( text, #clss, application, disambiguation, pluralnr ); } \
 static inline uiString legacyTr( const char* text, \
				  const char* disambiguation = 0,  \
				  int pluralnr=-1 ) \
 { return uiString( text, #clss, application, disambiguation, pluralnr ); }

#define mODTextTranslationClass(clss) \
mTextTranslationClass( clss, uiString::sODLocalizationApplication() )



#endif

