#ifndef volproctrans_h
#define volproctrans_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		March 2007
 RCS:		$Id: volproctrans.h,v 1.2 2009-01-06 10:16:09 cvsranojay Exp $
________________________________________________________________________

-*/
 
#include "transl.h"

namespace VolProc { class Chain; }

/*! Translator implementation for Volume Processing Setups. */

mClass VolProcessingTranslatorGroup : public TranslatorGroup
{				      isTranslatorGroup(VolProcessing)
public:
    			mDefEmptyTranslatorGroupConstructor(VolProcessing)

    const char*		defExtension() const	{ return "vpsetup"; }
    static const char*	sKeyIsVolProcSetup()	{ return "VolProcSetup"; }

    			//For process_volume par-files
    static const char*	sKeyChainID()		{ return "Chain ID"; }
    static const char*	sKeyOutputID()		{ return "Output ID"; }
};


mClass VolProcessingTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(VolProcessing)

    virtual const char*	read(VolProc::Chain&,Conn&)		= 0;
			//!< returns err msg or null on success
    virtual const char*	write(const VolProc::Chain&,Conn&)	= 0;
			//!< returns err msg or null on success

    static bool		retrieve(VolProc::Chain&,const IOObj*,
	    			 BufferString&);
    static bool		store(const VolProc::Chain&,const IOObj*,
	    		      BufferString&);
};


mClass dgbVolProcessingTranslator : public VolProcessingTranslator
{			     isTranslator(dgb,VolProcessing)
public:

    			mDefEmptyTranslatorConstructor(dgb,VolProcessing)

    const char*		read(VolProc::Chain&,Conn&);
    const char*		write(const VolProc::Chain&,Conn&);

};


#endif
