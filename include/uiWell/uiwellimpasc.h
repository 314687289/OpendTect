#ifndef uiwellimpasc_h
#define uiwellimpasc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2003
 RCS:           $Id: uiwellimpasc.h,v 1.16 2012-03-27 20:16:21 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiCheckBox;
class uiD2TModelGroup;
class uiFileInput;
class uiGenInput;
class uiLabel;
class uiTableImpDataSel;
class uiWellSel;

namespace Table { class FormatDesc; }
namespace Well { class Data; }


/*! \brief Dialog for well import from Ascii */

mClass uiWellImportAsc : public uiDialog
{
public:
			uiWellImportAsc(uiParent*);
			~uiWellImportAsc();

protected:

    uiFileInput*	trckinpfld_;
    uiCheckBox*		havetrckbox_;
    uiGenInput*		coordfld_;
    uiGenInput*		kbelevfld_;
    uiGenInput*		tdfld_;
    uiLabel*		vertwelllbl_;

    Well::Data&		wd_;

    Table::FormatDesc&  fd_;
    uiTableImpDataSel*  dataselfld_;
    uiD2TModelGroup*	d2tgrp_;
    uiWellSel*		outfld_;

    virtual bool	acceptOK(CallBacker*);
    bool		checkInpFlds();
    bool		doWork();
    void		doAdvOpt(CallBacker*);
    void		trckFmtChg(CallBacker*);
    void		inputChgd(CallBacker*);
    void		haveTrckSel(CallBacker*);

    friend class	uiWellImportAscOptDlg;
};


#endif
