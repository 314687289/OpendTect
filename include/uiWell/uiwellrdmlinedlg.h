#ifndef uiwellrdmlinedlg_h
#define uiwellrdmlinedlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          October 2005
 RCS:           $Id: uiwellrdmlinedlg.h,v 1.7 2009-01-08 09:16:21 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "bufstringset.h"

class MultiID;
class IOObj;
class CtxtIOObj;
class Coord;
class uiCheckBox;
class uiIOObjSel;
class uiTable;
class uiGenInput;
class uiListBox;
class uiToolButton;
class uiPushButton;
class uiWellPartServer;

/*! \brief: setup a dialog where the user can select throught which wells (s)he
  wants to make a random line path.
  Get coordinates from selected wells and send an event to allow the display of
  a fake random line on top of the survey; user can still modify choices, apply
  again and finally close the dlg.

*/

mClass uiWell2RandomLineDlg : public uiDialog
{
public:
    			uiWell2RandomLineDlg(uiParent*,uiWellPartServer*);
    			~uiWell2RandomLineDlg();

    void 		getCoordinates(TypeSet<Coord>&);
    const char*		getRandLineID() const;
    bool		dispOnCreation();

protected:
    void		fillListBox();
    void		setSelectedWells();
    
    const TypeSet<MultiID>&	getSelWells() const { return selwellsids_; }

    void		createSelectButtons(uiGroup*);
    void                createMoveButtons(uiGroup*);
    void		createFields(uiGroup*);
    void		attachFields(uiGroup*,uiGroup*,uiGroup*);
    void		selButPush(CallBacker*);
    void		moveButPush(CallBacker*);
    void		previewPush(CallBacker*);
    int			getFirstEmptyRow();
    void		ptsSel(CallBacker*);
    bool		acceptOK(CallBacker*);

    BufferStringSet	allwellsnames_;
    TypeSet<MultiID>	allwellsids_;
    TypeSet<MultiID>    selwellsids_;
    TypeSet<int>	selwellstypes_;

    uiListBox*		wellsbox_;
    uiTable*		selwellsbox_;
    uiGenInput*		onlytopfld_;
    uiIOObjSel* 	outfld_;
    uiCheckBox* 	dispfld_;

    CtxtIOObj&		outctio_;

    uiToolButton*	toselect_;
    uiToolButton*	fromselect_;

    uiToolButton*	moveupward_;
    uiToolButton*	movedownward_;
    
    uiPushButton*	previewbutton_;

    uiWellPartServer*	wellserv_;
};

#endif
