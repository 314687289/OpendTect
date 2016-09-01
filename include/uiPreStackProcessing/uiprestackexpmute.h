
#ifndef uiprestackexpmute_h
#define uiprestackexpmute_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		June 2008
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"

class CtxtIOObj;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;

namespace PreStack
{

mExpClass(uiPreStackProcessing) uiExportMute : public uiDialog
{ mODTextTranslationClass(uiExportMute);
public:
			uiExportMute(uiParent*);
		    	~uiExportMute();

protected:

    uiIOObjSel*		infld_;
    uiGenInput*		coordfld_;
    uiFileInput*	outfld_;

    CtxtIOObj&		ctio_;

    virtual bool	acceptOK();
    bool		writeAscii();    
};

} //namespace PreStack


#endif
