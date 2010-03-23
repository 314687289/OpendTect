#ifndef uiamplspectrum_h
#define uiamplspectrum_h

/*
________________________________________________________________________
            
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Satyaki Maitra
Date:          September 2007
RCS:           $Id: uiamplspectrum.h,v 1.9 2010-03-23 16:14:38 cvsbruno Exp $
______________________________________________________________________
                       
*/   

#include "uidialog.h"
#include "datapack.h"

#include <complex>
typedef std::complex<float> float_complex;

class uiFunctionDisplay;
class uiGenInput;
class uiLabeledSpinBox;
class FFT;
template <class T> class Array2D;
template <class T> class Array3D;
template <class T> class Array1DImpl;


mClass uiAmplSpectrum : public uiMainWin
{
public:
    				uiAmplSpectrum(uiParent*);
				~uiAmplSpectrum();

    void			setDataPackID(DataPack::ID,DataPackMgr::ID);
    void			setData(const Array2D<float>&);
    void			setData(const Array3D<float>&);

    void			getSpectrumData(Array1DImpl<float>&);
    Interval<float>		getPosRange() const { return posrange_; }

protected:

    uiFunctionDisplay*		disp_;
    uiGenInput*			rangefld_;
    uiLabeledSpinBox*		stepfld_;
    uiGroup*			dispparamgrp_;

    void			initFFT(int nrsamples);
    bool			compute(const Array3D<float>&);
    void			putDispData();
    
    Array1DImpl<float_complex>* timedomain_;
    Array1DImpl<float_complex>* freqdomain_;
    Array1DImpl<float_complex>* freqdomainsum_;
    Array1DImpl<float>* 	specvals_;

    Interval<float>		posrange_;

    FFT*			fft_;
    int				nrtrcs_;

    void			dispRangeChgd(CallBacker*);
};


#endif
