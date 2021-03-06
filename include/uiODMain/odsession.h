#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "sets.h"
#include "transl.h"
#include "iopar.h"
#include "uistrings.h"

/*!\brief dTect session save/restore */

mExpClass(uiODMain) ODSession
{ mODTextTranslationClass(ODSession);
public:
			ODSession();
    virtual		~ODSession()		{}

    IOPar&		empars()		{ return empars_; }
    IOPar&		seispars()		{ return seispars_; }
    IOPar&		vispars()		{ return vispars_; }
    IOPar&		attrpars(bool for2d);
    IOPar&		nlapars()		{ return nlapars_; }
    IOPar&		mpepars()		{ return mpepars_; }
    IOPar&		scenepars()		{ return scenepars_; }
    IOPar&		vwr2dpars()		{ return vwr2dpars_; }
    IOPar&		pluginpars()		{ return pluginpars_; }

    void		clear();
    ODSession&		operator =(const ODSession&);
    bool		operator ==(const ODSession&) const;

    bool		usePar(const IOPar&);
    void		fillPar(IOPar& par) const;

    static void		getStartupData(bool& douse,DBKey&);
    static void		setStartupData(bool,const DBKey&);

protected:

    IOPar		empars_;
    IOPar		seispars_;
    IOPar		vispars_;
    IOPar		scenepars_;
    IOPar		attrpars2d_;
    IOPar		attrpars3d_;
    IOPar		nlapars_;
    IOPar		mpepars_;
    IOPar		pluginpars_;
    IOPar		vwr2dpars_;

    static const char*	emprefix();
    static const char*	seisprefix();
    static const char*	visprefix();
    static const char*	sceneprefix();
    static const char*	attrprefix();
    static const char*	attr2dprefix();
    static const char*	attr3dprefix();
    static const char*	nlaprefix();
    static const char*	trackprefix();
    static const char*	vwr2dprefix();
    static const char*	pluginprefix();

    static const char*	sKeyUseStartup();
    static const char*	sKeyStartupID();
};


mExpClass(uiODMain) ODSessionTranslatorGroup : public TranslatorGroup
{   isTranslatorGroup(ODSession);
    mODTextTranslationClass(ODSessionTranslatorGroup);
public:
			mDefEmptyTranslatorGroupConstructor(ODSession)
    const char*		defExtension() const		{ return "sess"; }
};


mExpClass(uiODMain) ODSessionTranslator : public Translator
{ mODTextTranslationClass(ODSessionTranslator);
public:
			mDefEmptyTranslatorBaseConstructor(ODSession)

    virtual const char*	read(ODSession&,Conn&)		= 0;
			//!< returns err msg or null on success
    virtual const char*	write(const ODSession&,Conn&)	= 0;
			//!< returns err msg or null on success
    virtual const char*	warningMsg() const	{ return ""; }
    virtual const uiString warningUiMsg() const	{ return uiString::empty(); }

    static bool		retrieve(ODSession&,const IOObj*,uiString&);
			//!< BufferString has errmsg, if any
			//!< If true returned, errmsg contains warnings
    static bool		store(const ODSession&,const IOObj*,uiString&);
			//!< BufferString has errmsg, if any
			//!< If true returned, errmsg contains warnings
};


mExpClass(uiODMain) dgbODSessionTranslator : public ODSessionTranslator
{				  isTranslator(dgb,ODSession)
public:
			mDefEmptyTranslatorConstructor(dgb,ODSession)

    const char*		read(ODSession&,Conn&);
			//!< returns err msg or null on success
    const char*		write( const ODSession&,Conn&);
			//!< returns err msg or null on success
    const char*		warningMsg() const	{ return warningmsg; }

    BufferString	warningmsg;

};


#include "uiobjfileman.h"


/*! \brief
Session manager
*/

mExpClass(uiODMain) uiSessionMan : public uiObjFileMan
{ mODTextTranslationClass(ODSessionTranslator);
public:

			uiSessionMan(uiParent*);
			~uiSessionMan();

    mDeclInstanceCreatedNotifierAccess(uiSessionMan);

protected:

    virtual bool	gtItemInfo(const IOObj&,uiPhraseSet&) const;

};
