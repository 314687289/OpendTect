#ifndef uisetdatadir_h
#define uisetdatadir_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          June 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"

class uiFileInput;

mExpClass(uiIo) uiSetDataDir : public uiDialog
{
public:
			uiSetDataDir(uiParent*);

    const char*		selectedDir() const	{ return seldir_; }
    static bool		isOK(const char* dirnm=0); // if null, std data dir
    static bool		setRootDataDir(uiParent*,const char*);

protected:

    BufferString	seldir_;
    uiFileInput*	basedirfld_;
    const BufferString	curdatadir_;


    bool		acceptOK(CallBacker*);

    static void		offerUnzipSurv(uiParent*,const char*);

};

#endif

