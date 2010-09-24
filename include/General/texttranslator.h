#ifndef texttranslator_h
#define texttranslator_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id: texttranslator.h,v 1.1 2010-09-24 11:46:40 cvsnanne Exp $
________________________________________________________________________

-*/


#include "callback.h"


mClass TextTranslator : public CallBacker
{
public:

    virtual void		enable()			= 0;
    virtual void		disable()			= 0;

    virtual int			nrSupportedLanguages() const	= 0;
    virtual const wchar_t*	getLanguageUserName(int) const	= 0;
    virtual const char*		getLanguageName(int) const	= 0;

    virtual void		setToLanguage(const char*)	= 0;
    virtual const char*		getToLanguage() const		= 0;

    virtual int			translate(const char*)		= 0;
    virtual const wchar_t*	get() const			= 0;

    CNotifier<TextTranslator,int>	ready;
    Notifier<TextTranslator>		message;

protected:

				TextTranslator()
				    : ready(this), message(this)	{}

};


mClass TextTranslateMgr
{
public:
				TextTranslateMgr()
				    : translator_(0)	{}
				~TextTranslateMgr()	{ delete translator_; }

    void			setTranslator( TextTranslator* ttr )
				{ translator_ = ttr; }
    TextTranslator*		tr()			{ return translator_; }

protected:

    TextTranslator*			translator_;

};


mGlobal TextTranslateMgr& TrMgr();

#endif
