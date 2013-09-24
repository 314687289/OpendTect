#ifndef uiwelldlgs_h
#define uiwelldlgs_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uiselsimple.h"
#include "multiid.h"
#include "ranges.h"

class uiButtonGroup;
class uiCheckBox;
class uiComboBox;
class uiD2TModelGroup;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiLabel;
class uiLabeledListBox;
class uiTable;
class BufferStringSet;
class Coord3;
class od_ostream;
class CtxtIOObj;
class UnitOfMeasure;
class uiTableImpDataSel;

namespace Table { class FormatDesc; }
namespace Well { class Data; class Track; class D2TModel; class Log; }


/*! \brief Dialog for D2T Model editing. */

mExpClass(uiWell) uiWellTrackDlg : public uiDialog
{
public:
				uiWellTrackDlg(uiParent*,Well::Data&);
				~uiWellTrackDlg();

protected:

    Well::Data&			wd_;
    Well::Track&		track_;
    Well::Track*		orgtrack_;

    Table::FormatDesc&		fd_;
    uiTable*			tbl_;
    uiCheckBox*			zinftfld_;

    bool			fillTable(CallBacker* cb=0);
    bool			updNow(CallBacker*);
    void			readNew(CallBacker*);
    bool			rejectOK(CallBacker*);
    bool			acceptOK(CallBacker*);
    void			exportCB(CallBacker*);
};


mExpClass(uiWell) uiD2TModelDlg : public uiDialog
{
public:
				uiD2TModelDlg(uiParent*,Well::Data&,bool chksh);
				~uiD2TModelDlg();

protected:

    Well::Data&			wd_;
    bool			cksh_;
    Well::D2TModel*		orgd2t_; // Must be declared *below* others

    uiTable*			tbl_;
    uiCheckBox*			unitfld_;
    uiCheckBox*			timefld_;

    void			fillTable(CallBacker*);
    bool			getFromScreen();
    void			updNow(CallBacker*);
    void			dtpointChangedCB(CallBacker*);
    void			dtpointRemovedCB(CallBacker*);
    bool			updateDtpointDepth(int row);
    bool			updateDtpointTime(int row);
    bool			updateDtpoint(int row,float oldval);
    void			readNew(CallBacker*);
    bool			rejectOK(CallBacker*);
    bool			acceptOK(CallBacker*);
    void			expData(CallBacker*);
    void			getModel(Well::D2TModel&);

    void			getColLabels(BufferStringSet&) const;
    int				getTVDGLCol() const;
    int				getTVDSDCol() const;
    int				getTVDSSCol() const;
    int 			getTimeCol() const;
    int				getVintCol() const;
    bool			rowIsIncomplete(int row) const;
    int				getPreviousCompleteRowIdx(int row) const;
    int				getNextCompleteRowIdx(int row) const;
};



/*! \brief
Dialog for loading logs from las file
*/

mExpClass(uiWell) uiLoadLogsDlg : public uiDialog
{
public:
    				uiLoadLogsDlg(uiParent*,Well::Data&);

protected:

    uiFileInput*		lasfld;
    uiGenInput*			intvfld;
    uiGenInput*			intvunfld;
    uiGenInput*			istvdfld;
    uiGenInput*			udffld;
    uiLabel*			unitlbl;
    uiLabeledListBox*		logsfld;

    Well::Data&			wd;

    bool			acceptOK(CallBacker*);
    void			lasSel(CallBacker*);
};



mExpClass(uiWell) uiExportLogs : public uiDialog
{
public:
    				uiExportLogs(uiParent*,
					const ObjectSet<Well::Data>&,
					const BufferStringSet&);
protected:
    const ObjectSet<Well::Data>& wds_;
    const BufferStringSet&	logsel_;

    uiGenInput*			typefld_;
    uiButtonGroup*		zunitgrp_;
    uiGenInput*			zrangefld_;
    uiFileInput*		outfld_;
    uiGenInput*			multiwellsnamefld_;

    void			setDefaultRange(bool);
    void			writeHeader(od_ostream&,const Well::Data&);
    void			writeLogs(od_ostream&,const Well::Data&);

    void			typeSel(CallBacker*);
    virtual bool		acceptOK(CallBacker*);
};


/*! \brief Dialog for user made wells */

class uiColorInput;

mExpClass(uiWell) uiNewWellDlg : public uiGetObjectName
{
public:
    				uiNewWellDlg(uiParent*);
    				~uiNewWellDlg();

    const Color&		getWellColor();
    const char* 		getName() const		{ return name_; }
				
protected:

    uiColorInput*		colsel_;
    BufferString		name_;
    BufferStringSet*		nms_;

    virtual bool        	acceptOK(CallBacker*);
    const BufferStringSet&	mkWellNms();
};


/* brief some editable uom for the logs */
mExpClass(uiWell) uiWellLogUOMDlg : public uiDialog
{
public:
				uiWellLogUOMDlg(uiParent*,Well::Log&);

protected:

    uiComboBox*                 unfld_;
    Well::Log&			log_;

    bool			acceptOK(CallBacker*);
};


#endif

