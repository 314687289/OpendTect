#ifndef uisrchprocfiles_h
#define uisrchprocfiles_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Feb 2006
 RCS:           $Id: uisrchprocfiles.h,v 1.4 2012-08-03 13:01:01 cvskris Exp $
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"

class uiGenInput;
class uiIOObjSel;
class uiFileInput;
class CtxtIOObj;

/*!\brief Finds created objects in processing specification files */

mClass(uiIo) uiSrchProcFiles : public uiDialog
{
public:
			uiSrchProcFiles(uiParent*,CtxtIOObj&,
					const char* iopar_key);

    const char*		fileName() const;

protected:

    uiFileInput*	dirfld;
    uiGenInput*		maskfld;
    uiGenInput*		fnamefld;
    uiIOObjSel*		objfld;

    BufferString	iopkey_;
    CtxtIOObj&		ctio_;

    void		srchDir(CallBacker*);
    bool		acceptOK(CallBacker*);
};


#endif

