#ifndef uituthortools_h
#define uituthortools_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh / Karthika
 * DATE     : Mar 2007
 * ID       : $Id: uituthortools.h,v 1.8 2010-02-09 05:15:28 cvsnanne Exp $
-*/

#include "uidialog.h"

class TaskRunner;
class uiIOObjSel;
class uiGenInput;

class uiTutHorTools : public uiDialog
{
public:

    			uiTutHorTools(uiParent*);
			~uiTutHorTools();

protected:

    void		choiceSel(CallBacker*);
    bool		acceptOK(CallBacker*);

    bool		checkAttribName() const;
    bool		doThicknessCalc();
    bool		doSmoother();

    uiGenInput*		taskfld_;
    uiIOObjSel*		inpfld_;
    uiIOObjSel*		inpfld2_;
    uiGenInput*		selfld_;
    uiGenInput*		attribnamefld_;
    uiIOObjSel*		outfld_;
    uiGenInput*		strengthfld_;
};


#endif
