#ifndef uimadbldcmd_h
#define uimadbldcmd_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : June 2007
 * ID       : $Id: uimadbldcmd.h,v 1.5 2007-07-04 09:44:54 cvsbert Exp $
-*/

#include "uidialog.h"
class uiLineEdit;
class uiSeparator;
class uiComboBox;
class uiListBox;
class uiTextEdit;
namespace ODMad { class ProgDef; }


class uiMadagascarBldCmd : public uiDialog
{
public:

			uiMadagascarBldCmd(uiParent*,const char* cmd,
					   bool modaldlg=true);
			~uiMadagascarBldCmd();

    const char*		command() const;

    Notifier<uiMadagascarBldCmd> applyReq;

protected:

    uiComboBox*		groupfld_;
    uiComboBox*		srchresfld_;
    uiListBox*		progfld_;
    uiLineEdit*		cmdfld_;
    uiLineEdit*		descfld_;
    uiLineEdit*		synopsfld_;
    uiLineEdit*		srchfld_;
    uiTextEdit*		commentfld_;

    uiSeparator*	createMainPart();

    void		onPopup(CallBacker*);
    void		groupChg(CallBacker*);
    void		progChg(CallBacker*);
    void		doAdd(CallBacker*);
    void		dClick(CallBacker*);
    void		doSearch(CallBacker*);
    void		searchBoxSel(CallBacker*);

    void		setProgName(const char*);
    void		setInput(const ODMad::ProgDef*);
    const ODMad::ProgDef* getDef(const char*);
    void		setGroupProgs(const BufferString*);
    bool		cmdOK();

    bool		acceptOK(CallBacker*);

};


#endif
