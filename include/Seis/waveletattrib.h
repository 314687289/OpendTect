#ifndef waveletattrib_h
#define waveletattrib_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Nov 2009
 RCS:           $Id: waveletattrib.h,v 1.11 2010-10-27 06:56:31 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "commondefs.h"

class ArrayNDWindow;
class Wavelet;
template <class T> class Array1DImpl;

mClass WaveletAttrib
{
public:
    			WaveletAttrib(const Wavelet&);
			~WaveletAttrib();

    void		setNewWavelet(const Wavelet&);
    void		getHilbert(Array1DImpl<float>&) const;
    void		getPhase(Array1DImpl<float>&,bool degree=false) const;
    void		getFrequency(Array1DImpl<float>&,int padfac=1);
    			//frequency array will be resized to padfac*array size )
    void		applyFreqWindow(const ArrayNDWindow&, int padfac,
					Array1DImpl<float>&);

    static void		unwrapPhase(int nrsamples,float wrapparam,float* phase);
    static void		muteZeroFrequency(Array1DImpl<float>&);

protected:

    Array1DImpl<float>* wvltarr_;
    int			wvltsz_;
};

#endif
