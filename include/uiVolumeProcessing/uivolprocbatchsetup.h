#ifndef uivolprocbatchsetup_h
#define uivolprocbatchsetup_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uivolprocbatchsetup.h,v 1.5 2009-05-14 21:18:16 cvskris Exp $
________________________________________________________________________

-*/

#include "uibatchlaunch.h"
class IOObj;
class IOPar;
class CtxtIOObj;
class uiIOObjSel;
class uiPosSubSel;
class uiGenInput;
class uiVelocityDesc;


namespace VolProc 
{

class Chain;

mClass uiBatchSetup : public uiFullBatchDialog
{

public:
                        uiBatchSetup(uiParent*, const IOObj* setupsel = 0 );
                        ~uiBatchSetup();

protected:

    bool		prepareProcessing();
    bool		fillPar(IOPar&);

    uiIOObjSel*		setupsel_;
    uiPushButton*	editsetup_;
    uiPosSubSel*	possubsel_;
    uiIOObjSel*		outputsel_;
    Chain*		chain_;

    void		setupSelCB(CallBacker*);
    void		editPushCB(CallBacker*);
};

}; //namespace

#endif
