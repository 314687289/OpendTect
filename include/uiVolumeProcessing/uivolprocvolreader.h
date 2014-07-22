#ifndef uivolprocvolreader_h
#define uivolprocvolreader_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		November 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"
#include "uivolprocchain.h"

#include "volprocvolreader.h"

class uiSeisSel;


namespace VolProc
{

mExpClass(uiVolumeProcessing) uiVolumeReader : public uiStepDialog
{ mODTextTranslationClass(uiVolumeReader);
public:
    mDefaultFactoryInstanciationBase(
	    VolProc::VolumeReader::sFactoryKeyword(),
	    VolProc::VolumeReader::sFactoryDisplayName())
    mDefaultFactoryInitClassImpl( uiStepDialog, createInstance );



protected:
				uiVolumeReader(uiParent*,VolumeReader*);
				~uiVolumeReader();
    static uiStepDialog*	createInstance(uiParent*, Step*);

    void			volSel(CallBacker*);
    void			updateFlds(CallBacker*);
    bool			acceptOK(CallBacker*);

    VolumeReader*		volumereader_;

    uiSeisSel*			seissel_;

};

}; //namespace

#endif

