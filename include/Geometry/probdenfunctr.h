#ifndef probdenfunctr_h
#define probdenfunctr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id: probdenfunctr.h,v 1.7 2010-02-09 16:04:07 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "transl.h"
#include <iosfwd>

class IOObj;
class ProbDenFunc;
class BufferString;


mClass ProbDenFuncTranslatorGroup : public TranslatorGroup
{				    isTranslatorGroup(ProbDenFunc)
public:
    			mDefEmptyTranslatorGroupConstructor(ProbDenFunc)

    const char*		defExtension() const		{ return "prdf"; }

};


mClass ProbDenFuncTranslator : public Translator
{
public:
    			ProbDenFuncTranslator(const char* nm,const char* unm);

    static const char*	key();

    static ProbDenFunc* read(const IOObj&,BufferString* emsg=0);
    static bool		write(const ProbDenFunc&,const IOObj&,
	    		      BufferString* emsg=0);

    virtual ProbDenFunc* read(std::istream&)			= 0;
    virtual bool	 write(const ProbDenFunc&,std::ostream&) = 0;;

    bool		binary_;	//!< default: false

};


mClass odProbDenFuncTranslator : public ProbDenFuncTranslator
{				 isTranslator(od,ProbDenFunc)
public:
    			mDefEmptyTranslatorConstructor(od,ProbDenFunc)

    ProbDenFunc*	read(std::istream&);
    bool		write(const ProbDenFunc&,std::ostream&);

};


#endif
