#ifndef uiwelltiemgrdlg_h
#define uiwelltiemgrdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jan 2009
 RCS:           $Id$
________________________________________________________________________

-*/


#include "uiwellattribmod.h"
#include "uidialog.h"

namespace Well { class Data; }

class MultiID;
class CtxtIOObj;

class uiIOObjSel;
class uiComboBox;
class uiLabeledComboBox;
class uiCheckBox;
class uiGenInput;
class uiPushButton;
class uiSeisSel;
class uiSeis2DLineNameSel;
class uiSeisWaveletSel;
class uiWaveletExtraction;
class uiWellElasticPropSel;


namespace WellTie
{
    class Setup;
    class uiTieWin;

mExpClass(uiWellAttrib) uiTieWinMGRDlg : public uiDialog
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
    uiWellElasticPropSel* logsfld_;

    Well::Data*		wd_;

    uiIOObjSel*         wellfld_;
    uiSeisWaveletSel* 	wvltfld_;
    uiGenInput*		typefld_;
    uiGenInput*		seisextractfld_;
    uiSeisSel* 		seis2dfld_;
    uiSeisSel* 		seis3dfld_;
    uiSeis2DLineNameSel* seislinefld_;
    uiCheckBox*		used2tmbox_;
    uiLabeledComboBox*	cscorrfld_;
    uiWaveletExtraction* extractwvltdlg_;

    bool		getSetup( const char* wllnm );
    bool		getSeismicInSetup();
    bool		getVelLogInSetup() const;
    bool		getDenLogInSetup() const;
    bool		initSetup();
    void		saveWellTieSetup(const MultiID&,
	    				const WellTie::Setup&) const;
    
    bool		acceptOK(CallBacker*);
    void		extrWvlt(CallBacker*);
    void                extractWvltDone(CallBacker*);
    void		seisSelChg(CallBacker*);
    void		d2TSelChg(CallBacker*);
    void		wellSelChg(CallBacker*);
    void 		wellTieDlgClosed(CallBacker*);
    void		set3DSeis() const;
    void		set2DSeis() const;
    void		setLine() const;
    void		setTypeFld();
    bool		seisIDIs3D(MultiID) const;
};

}; //namespace WellTie
#endif

