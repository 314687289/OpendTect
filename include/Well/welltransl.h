#ifndef welltransl_h
#define welltransl_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: welltransl.h,v 1.4 2003-11-07 12:21:52 bert Exp $
________________________________________________________________________


-*/

#include "transl.h"


namespace Well { class Data; };


class WellTranslatorGroup : public TranslatorGroup
{			    isTranslatorGroup(Well)
public:
    			mDefEmptyTranslatorGroupConstructor(Well)
    const char*		defExtension() const { return "well"; }
};


class WellTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(Well)

    virtual bool	read(Well::Data&,const IOObj&)		= 0;
    virtual bool	write(const Well::Data&,const IOObj&)	= 0;

    virtual bool	implRemove(const IOObj*) const;
    virtual bool	implRename(const IOObj*,const char*,
	    			   const CallBack* cb=0) const;
    virtual bool	implSetReadOnly(const IOObj*,bool) const;

};


class dgbWellTranslator : public WellTranslator
{			  isTranslator(dgb,Well)
public:
    			mDefEmptyTranslatorConstructor(dgb,Well)

    virtual bool	read(Well::Data&,const IOObj&);
    virtual bool	write(const Well::Data&,const IOObj&);

};


#endif
