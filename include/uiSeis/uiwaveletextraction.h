#ifndef uiwaveletextraction_h
#define uiwaveletextraction_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          April 2009
 RCS:		$Id: uiwaveletextraction.h,v 1.13 2009-12-02 05:29:30 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiGenInput;
class uiIOObjSel;
class uiIOObjSelGrp;
class uiPosProvGroup;
class uiSeisSel;
class uiSelection2DParSel;
class uiSeis3DSubSel;
class uiSelZRange;
class CtxtIOObj;
class IOPar;
class MultiID;
namespace Seis { class SelData; class TableSelData; }

mClass uiWaveletExtraction : public uiDialog
{
public:
				uiWaveletExtraction(uiParent*,bool is2d);
				~uiWaveletExtraction();
    MultiID			storeKey() const;

    Notifier<uiWaveletExtraction>	extractionDone;

protected:

    void			createCommonUIFlds();
    bool			checkWaveletSize();
    bool			check2DFlds();
    bool			acceptOK(CallBacker*);
    void			choiceSelCB(CallBacker*);
    void			inputSelCB(CallBacker*);
    bool			doProcess(const IOPar&,const IOPar&);
    bool                        getSelData(const IOPar&,const IOPar&);
    bool			fillHorizonSelData(const IOPar&,const IOPar&,
						   Seis::TableSelData&);

    CtxtIOObj&			seisctio_;
    CtxtIOObj&			wvltctio_;
    uiGenInput*			zextraction_;
    uiGenInput*			wtlengthfld_;
    uiGenInput*			wvltphasefld_;
    uiGenInput*			taperfld_;
    uiIOObjSel*			outputwvltfld_;
    uiPosProvGroup* 		surfacesel_;
    uiSeisSel*			seissel3dfld_;
    uiSelection2DParSel*	linesel2dfld_;
    uiSeis3DSubSel*		subselfld3d_;
    uiSelZRange*		zrangefld_;
    Seis::SelData*		sd_;

    int				wvltsize_;
};

#endif
