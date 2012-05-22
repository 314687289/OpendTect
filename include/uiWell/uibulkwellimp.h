#ifndef uibulkwellimp_h
#define uibulkwellimp_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : May 2012
 * ID       : $Id: uibulkwellimp.h,v 1.1 2012-05-22 21:52:18 cvsnanne Exp $
-*/

#include "uidialog.h"

class uiFileInput;
class uiTableImpDataSel;

namespace Table { class FormatDesc; }
namespace Well { class Data; }


mClass uiBulkTrackImport : public uiDialog
{
public:
			uiBulkTrackImport(uiParent*);
			~uiBulkTrackImport();

protected:

    bool		acceptOK(CallBacker*);

    uiFileInput*	inpfld_;
    uiTableImpDataSel*	dataselfld_;

    ObjectSet<Well::Data>   wells_;
    Table::FormatDesc*	    fd_;
};


mClass uiBulkLogImport : public uiDialog
{
public:
			uiBulkLogImport(uiParent*);
			~uiBulkLogImport();

protected:

    bool		acceptOK(CallBacker*);

    uiFileInput*	inpfld_;
};


mClass uiBulkMarkerImport : public uiDialog
{
public:
			uiBulkMarkerImport(uiParent*);
			~uiBulkMarkerImport();
protected:

    bool		acceptOK(CallBacker*);

    uiFileInput*	inpfld_;
};

#endif
