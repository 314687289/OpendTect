#ifndef uiwelldlgs_h
#define uiwelldlgs_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id: uiwelldlgs.h,v 1.30 2009-02-23 16:06:42 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uiselsimple.h"
#include "multiid.h"
#include "ranges.h"

class uiButtonGroup;
class uiCheckBox;
class uiD2TModelGroup;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiLabel;
class uiLabeledListBox;
class uiTable;
class Coord3;
class CtxtIOObj;
class StreamData;
class uiTableImpDataSel;

namespace Table { class FormatDesc; }
namespace Well { class Data; class Track; class D2TModel; };


/*! \brief Dialog for D2T Model editing. */

mClass uiWellTrackDlg : public uiDialog
{
public:
				uiWellTrackDlg(uiParent*,Well::Data&);
				~uiWellTrackDlg();

protected:

    Well::Data&			wd;
    Well::Track&		track;
    Well::Track*		orgtrack;

    Table::FormatDesc&		fd;
    uiTable*			table;
    uiGenInput*			unitfld;

    void			fillTable();
    void			updNow(CallBacker*);
    void			readNew(CallBacker*);
    bool			rejectOK(CallBacker*);
    bool			acceptOK(CallBacker*);
};


mClass uiD2TModelDlg : public uiDialog
{
public:
				uiD2TModelDlg(uiParent*,Well::Data&);
				~uiD2TModelDlg();

protected:

    Well::Data&			wd;
    Well::D2TModel&		d2t;
    Well::D2TModel*		orgd2t;

    uiTable*			table;
    uiGenInput*			unitfld;

    void			fillTable();
    void			updNow(CallBacker*);
    void			readNew(CallBacker*);
    bool			rejectOK(CallBacker*);
    bool			acceptOK(CallBacker*);
};



/*! \brief
Dialog for loading logs from las file
*/

mClass uiLoadLogsDlg : public uiDialog
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



mClass uiExportLogs : public uiDialog
{
public:
    				uiExportLogs(uiParent*,const Well::Data&,
					    const BoolTypeSet&);

protected:
    const Well::Data&		wd;
    const BoolTypeSet&		logsel;

    uiGenInput*			typefld;
    uiButtonGroup*		zunitgrp;
    uiGenInput*			zrangefld;
    uiFileInput*		outfld;

    void			setDefaultRange(bool);
    void			typeSel(CallBacker*);
    virtual bool		acceptOK(CallBacker*);
    void			writeHeader(StreamData&);
    void			writeLogs(StreamData&);
};

/*! \brief Dialog for storing edited or home-made wells */

mClass uiStoreWellDlg : public uiDialog
{
public:
    				uiStoreWellDlg(uiParent*,const BufferString&);
				~uiStoreWellDlg();

    void			setWellCoords(const TypeSet<Coord3>&);
    MultiID			getMultiID() const;

protected:

    uiD2TModelGroup*		d2tgrp;
    uiGenInput*			usemodelfld;
    uiGenInput*			constvelfld;
    uiIOObjSel*			outfld;

    virtual bool       		acceptOK(CallBacker*);
    bool			checkInpFlds();
    void			modelSel(CallBacker*);
    bool			storeWell();
    bool			setWellTrack(Well::Data*);

    CtxtIOObj&			ctio_;
    TypeSet<Coord3>		wellcoords_;

};


/*! \brief Dialog for user made wells */

class uiColorInput;

mClass uiNewWellDlg : public uiGetObjectName
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

#endif
