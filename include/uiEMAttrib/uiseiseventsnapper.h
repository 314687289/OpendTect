#ifndef uiseiseventsnapper_h
#define uiseiseventsnapper_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: uiseiseventsnapper.h,v 1.10 2012-08-03 13:00:55 cvskris Exp $
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "uidialog.h"

namespace EM { class Horizon; }

class CtxtIOObj;
class IOObj;
class uiGenInput;
class uiHorSaveFieldGrp;
class uiIOObjSel;
class uiSeisSel;

/*! \brief Part Server for Wells */

mClass(uiEMAttrib) uiSeisEventSnapper : public uiDialog
{
public:
			uiSeisEventSnapper(uiParent*,const IOObj*,
					   bool is2d=false);
			~uiSeisEventSnapper();
    uiHorSaveFieldGrp*	saveFldGrp() const { return savefldgrp_; }	

protected:

    uiHorSaveFieldGrp*	savefldgrp_;
    uiIOObjSel*		horinfld_;
    uiSeisSel*		seisfld_;
    uiGenInput*		eventfld_;
    uiGenInput*		gatefld_;
    bool		is2d_;

    virtual bool	acceptOK(CallBacker*);
    bool		readHorizon();

    EM::Horizon*	horizon_;
    CtxtIOObj&		seisctio_;
};

#endif

