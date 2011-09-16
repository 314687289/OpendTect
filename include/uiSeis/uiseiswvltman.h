#ifndef uiseiswvltman_h
#define uiseiswvltman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2006
 RCS:           $Id: uiseiswvltman.h,v 1.21 2011-09-16 10:01:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"
#include "datapack.h"

class uiFlatViewer;
class uiWaveletExtraction;
class uiWaveletDispPropDlg;
class Wavelet;
template <class T> class Array2D;


mClass uiSeisWvltMan : public uiObjFileMan
{
public:
			uiSeisWvltMan(uiParent*);
			~uiSeisWvltMan();

    mDeclInstanceCreatedNotifierAccess(uiSeisWvltMan);

protected:

    uiGroup*			butgrp_;
    uiFlatViewer*		wvltfld_;
    DataPack::ID		curid_;
    uiWaveletExtraction*  	wvltext_;
    uiWaveletDispPropDlg*	wvltpropdlg_;

    void			mkFileInfo();
    void			setViewerData(const Wavelet*);
    
    void                	closeDlg(CallBacker*);
    void			crPush(CallBacker*);
    void			dispProperties(CallBacker*);
    void			impPush(CallBacker*);
    void			mrgPush(CallBacker*);
    void			extractPush(CallBacker*);
    void			getFromOtherSurvey(CallBacker*);
    void			reversePolarity(CallBacker*);
    void			rotatePhase(CallBacker*);
    void			taper(CallBacker*);
    void                	updateCB(CallBacker*);
    void 			updateViewer(CallBacker*);
};


#endif
