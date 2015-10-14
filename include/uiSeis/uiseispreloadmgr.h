#ifndef uiseispreloadmgr_h
#define uiseispreloadmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "multiid.h"

class BufferStringSet;
class uiListBox;
class uiTextEdit;


/*!\brief Manager for pre-loading Seismic Data */

mExpClass(uiSeis) uiSeisPreLoadMgr : public uiDialog
{ mODTextTranslationClass(uiSeisPreLoadMgr)
public:
			uiSeisPreLoadMgr(uiParent*);

protected:

    uiListBox*		listfld_;
    uiTextEdit*		infofld_;

    void		fillList();
    void		fullUpd(CallBacker*);
    void		selChg(CallBacker*);
    void		cubeLoadPush(CallBacker*);
    void		linesLoadPush(CallBacker*);
    void		ps3DPush(CallBacker*);
    void		ps2DPush(CallBacker*);
    void		unloadPush(CallBacker*);
    void		openPush(CallBacker*);
    void		savePush(CallBacker*);
};


#endif


