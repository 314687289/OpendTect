#ifndef uiwelltiemgrdlg_h
#define uiwelltiemgrdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jan 2009
 RCS:           $Id: uiwelltiemgrdlg.h,v 1.14 2011-01-20 10:21:38 cvsbruno Exp $
________________________________________________________________________

-*/


#include "uidialog.h"

namespace Well { class Data; }

class MultiID;
class CtxtIOObj;

class uiIOObjSel;
class uiComboBox;
class uiCheckBox;
class uiGenInput;
class uiSeisSel;
class uiSeis2DLineNameSel;
class uiSeisWaveletSel;
class uiWaveletExtraction;

namespace WellTie
{
    class Setup;
    class uiTieWin;

mClass uiTieWinMGRDlg : public uiDialog
{

public:    
			uiTieWinMGRDlg(uiParent*,WellTie::Setup&);
			~uiTieWinMGRDlg();

    void		delWins(); 

protected:

    WellTie::Setup& 	wtsetup_;
    CtxtIOObj&          wllctio_;
    CtxtIOObj&          wvltctio_;
    CtxtIOObj&          seisctio2d_;
    CtxtIOObj&          seisctio3d_;
    bool		savedefaut_;
    bool		is2d_;
    ObjectSet<uiTieWin> welltiedlgset_;
    ObjectSet<uiTieWin> welltiedlgsetcpy_;

    Well::Data*		wd_;

    uiIOObjSel*         wellfld_;
    uiSeisWaveletSel* 	wvltfld_;
    uiGenInput*		typefld_;
    uiGenInput*		seisextractfld_;
    uiSeisSel* 		seis2dfld_;
    uiSeisSel* 		seis3dfld_;
    uiSeis2DLineNameSel* seislinefld_;
    uiComboBox*		vellogfld_;
    uiComboBox*		denlogfld_;
    uiCheckBox*		isvelbox_;
    uiCheckBox*		used2tmbox_;
    uiWaveletExtraction* extractwvltdlg_;

    bool		getDefaults();
    void		saveWellTieSetup(const MultiID&,const WellTie::Setup&);
    
    bool		acceptOK(CallBacker*);
    void		extrWvlt(CallBacker*);
    void                extractWvltDone(CallBacker*);
    void		isSonicSel(CallBacker*);
    void		selChg( CallBacker* );
    void		wellSel(CallBacker*);
    void 		wellTieDlgClosed(CallBacker*);
};

}; //namespace WellTie
#endif
