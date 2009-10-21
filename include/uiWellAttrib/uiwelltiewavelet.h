#ifndef uiwelltiewavelet_h
#define uiwelltiewavelet_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          January 2009
RCS:           $Id: uiwelltiewavelet.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uigroup.h"

class CtxtIOObj;
class FFT;
class Wavelet;

class uiFlatViewer;
class uiFunctionDisplay;
class uiGenInput;
class uiToolButton;
class uiIOObjSel;
class uiTextEdit;
class uiWaveletDispPropDlg;

namespace WellTie
{

class GeoCalculator;
class DataHolder;
class Setup;
class uiWavelet;

mClass uiWaveletView : public uiGroup
{
public:

	    uiWaveletView(uiParent*,DataHolder*); 
	    ~uiWaveletView();

    void 			redrawWavelets();

    Notifier<uiWaveletView> 	activeWvltChged;
    void 			activeWvltChanged(CallBacker*);

protected:

    WellTie::DataHolder*	dataholder_;
    CtxtIOObj&          	wvltctio_;

    uiGenInput*			activewvltfld_;
    ObjectSet<WellTie::uiWavelet> uiwvlts_;

    void 			createWaveletFields(uiGroup*);	   
};


class uiWavelet : public uiGroup
{

public: 
    				uiWavelet(uiParent*,Wavelet*,bool);
				~uiWavelet();
    
    Notifier<uiWavelet> 	wvltChged;
    void			drawWavelet();
    void			setAsActive(bool);

protected:				    

    bool			isactive_;
  
    Wavelet*			wvlt_; 	
    ObjectSet<uiToolButton>     wvltbuts_;
    uiFlatViewer*               viewer_;
    uiWaveletDispPropDlg*  	wvltpropdlg_;

    void			initWaveletViewer();

    void 			dispProperties(CallBacker*);
    void			rotatePhase(CallBacker*);
    void			taper(CallBacker*);
    void 			wvltChanged(CallBacker*);
};				


}; //namespace WellTie
#endif

