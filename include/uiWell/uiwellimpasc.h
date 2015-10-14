#ifndef uiwellimpasc_h
#define uiwellimpasc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2003
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uidialog.h"
#include "multiid.h"

class uiCheckBox;
class uiD2TModelGroup;
class uiFileInput;
class uiGenInput;
class uiLabel;
class uiTableImpDataSel;
class uiWellSel;
class UnitOfMeasure;

namespace Table { class FormatDesc; }
namespace Well { class Data; }


/*! \brief Dialog for well import from Ascii */

mExpClass(uiWell) uiWellImportAsc : public uiDialog
{ mODTextTranslationClass(uiWellImportAsc);
public:
			uiWellImportAsc(uiParent*);
			~uiWellImportAsc();

    MultiID		getWellID() const;
    Notifier<uiWellImportAsc> importReady;

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
    bool		zinft_;
    const UnitOfMeasure* zun_;

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


