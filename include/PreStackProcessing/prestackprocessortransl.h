#ifndef prestackprocessortransl_h
#define prestackprocessortransl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Oct 2008
 RCS:		$Id: prestackprocessortransl.h,v 1.5 2012-08-03 13:00:34 cvskris Exp $
________________________________________________________________________


-*/
 
#include "prestackprocessingmod.h"
#include "transl.h"
namespace PreStack { class ProcessManager; }


mClass(PreStackProcessing) PreStackProcTranslatorGroup : public TranslatorGroup
{				      isTranslatorGroup(PreStackProc)
public:
    			mDefEmptyTranslatorGroupConstructor(PreStackProc)

    const char*		defExtension() const		{ return "psp"; }
};


mClass(PreStackProcessing) PreStackProcTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(PreStackProc)

    virtual const char*	read(PreStack::ProcessManager&,Conn&)		= 0;
			//!< returns err msg or null on success
    virtual const char*	write(const PreStack::ProcessManager&,Conn&)	= 0;
			//!< returns err msg or null on success

    static bool		retrieve(PreStack::ProcessManager&,const IOObj*,
	    			 BufferString&);
    static bool		store(const PreStack::ProcessManager&,const IOObj*,
	    		      BufferString&);
};


mClass(PreStackProcessing) dgbPreStackProcTranslator : public PreStackProcTranslator
{			     isTranslator(dgb,PreStackProc)
public:

    			mDefEmptyTranslatorConstructor(dgb,PreStackProc)

    const char*		read(PreStack::ProcessManager&,Conn&);
    const char*		write(const PreStack::ProcessManager&,Conn&);

};


#endif

