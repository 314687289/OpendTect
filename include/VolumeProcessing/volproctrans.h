#ifndef volproctrans_h
#define volproctrans_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		March 2007
________________________________________________________________________

-*/
 
#include "volumeprocessingmod.h"
#include "transl.h"

/*!\brief Volume Processing*/

namespace VolProc { class Chain; }

/*!
\brief Translator implementation for Volume Processing Setups.
*/

mExpClass(VolumeProcessing) VolProcessingTranslatorGroup
				: public TranslatorGroup
{   isTranslatorGroup(VolProcessing);
    mODTextTranslationClass(VolProcessingTranslatorGroup);
public:
    			mDefEmptyTranslatorGroupConstructor(VolProcessing)

    const char*		defExtension() const	{ return "vpsetup"; }
    static const char*	sKeyIsVolProcSetup()	{ return "VolProcSetup"; }

    			//For od_process_volume par-files
    static const char*	sKeyChainID()		{ return "Chain ID"; }
    static const char*	sKeyOutputID()		{ return "Output ID"; }
};


/*!
\brief Volume Processing Translator
*/

mExpClass(VolumeProcessing) VolProcessingTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(VolProcessing)

    virtual const char*	read(VolProc::Chain&,Conn&)		= 0;
			//!< returns err msg or null on success
    virtual const char*	write(const VolProc::Chain&,Conn&)	= 0;
			//!< returns err msg or null on success

    static bool		retrieve(VolProc::Chain&,const IOObj*,
				 uiString&);
    static bool		store(const VolProc::Chain&,const IOObj*,
			      uiString&);
};


/*!
\brief dgb Volume Processing Translator
*/

mExpClass(VolumeProcessing) dgbVolProcessingTranslator
					: public VolProcessingTranslator
{			     isTranslator(dgb,VolProcessing)
public:

    			mDefEmptyTranslatorConstructor(dgb,VolProcessing)

    const char*		read(VolProc::Chain&,Conn&);
    const char*		write(const VolProc::Chain&,Conn&);

};


#endif
