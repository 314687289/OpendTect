#ifndef uidpsselgrpdlg_h
#define uidpsselgrpdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          June 2011
 RCS:           $Id: uidpsselgrpdlg.cc,: 
________________________________________________________________________

-*/

#include "uidialog.h"

class uiDataPointSetCrossPlotter;
class uiTable;
class SelectionGrp;

class uiDPSSelGrpDlg : public uiDialog
{
public:

			uiDPSSelGrpDlg(uiDataPointSetCrossPlotter&,
				       const BufferStringSet&);
protected:

    int					curselgrp_;
    uiTable*				tbl_;
    uiDataPointSetCrossPlotter&		plotter_;
    ObjectSet<SelectionGrp>&		selgrps_;

    void				setCurSelGrp(CallBacker*);
    void				changeSelGrbNm(CallBacker*);
    void				addSelGrp(CallBacker*);
    void				importSelectionGrps(CallBacker*);
    void				exportSelectionGrps(CallBacker*);
    void				remSelGrp(CallBacker*);
    void				changeColCB(CallBacker*);
    void				mapLikeliness(CallBacker*);
};

# endif
