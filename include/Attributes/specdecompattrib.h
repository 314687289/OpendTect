#ifndef specdecompattrib_h
#define specdecompattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          Jan 2004
 RCS:           $Id: specdecompattrib.h,v 1.15 2010-08-11 16:55:33 cvsyuancheng Exp $
________________________________________________________________________
-*/

#include "attribprovider.h"
#include "arrayndutils.h"
#include "wavelettrans.h"
#include "fourier.h"


/*!\brief Spectral Decomposition Attribute

SpecDecomp gate=[-12,12] window=[Box]|Hamming|Hanning|Barlett|Blackman|CosTaper5

Calculates the frequency spectrum of a trace
Input:
0       Real data
1       Imag data

Output:
0       Spectrum 0 freq
1       freq 1
2       freq 2
|       etc
|       etc
N

*/

namespace Attrib
{

class DataHolder;

mClass SpecDecomp : public Provider
{
public:
    static void		initClass();
			SpecDecomp(Desc&);

    static const char*	attribName()		{ return "SpecDecomp"; }
    static const char*  transformTypeStr()	{ return "transformtype"; }
    static const char*	windowStr()		{ return "window"; }
    static const char*	gateStr()		{ return "gate"; }
    static const char*	deltafreqStr()		{ return "deltafreq"; }
    static const char*	dwtwaveletStr()		{ return "dwtwavelet"; }
    static const char*	cwtwaveletStr()		{ return "cwtwavelet"; }
    static const char*	transTypeNamesStr(int);

    void                getCompNames(BufferStringSet&) const;

protected:
    			~SpecDecomp();
    static Provider*	createInstance(Desc&);
    static void		updateDesc(Desc&);

    bool		allowParallelComputation() const
    			{ return false; }

    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&,int idx);
    bool		computeData(const DataHolder&,const BinID& relpos,
	    			    int t0,int nrsamples,int threadid) const;
    bool		calcDFT(const DataHolder&,int t0,int nrsamples) const;
    bool		calcDWT(const DataHolder&,int t0,int nrsamples) const;
    bool		calcCWT(const DataHolder&,int t0,int nrsamples) const;

    const Interval<float>*	reqZMargin(int input, int output) const;
    const Interval<int>*	desZSampMargin(int input, int output) const;

    int					transformtype_;
    ArrayNDWindow::WindowType		windowtype_;
    Interval<float>			gate_;
    float                       	deltafreq_;
    WaveletTransform::WaveletType	dwtwavelet_;
    CWT::WaveletType			cwtwavelet_;

    Interval<int>               	samplegate_;

    ArrayNDWindow*              	window_;
    Fourier::CC*                       	fft_;
    CWT                         	cwt_;
    int                         	scalelen_;

    float                       	df_;
    int                         	fftsz_;
    int                         	sz_;

    bool				fftisinit_;
    Interval<int>			desgate_;

    int					realidx_;
    int					imagidx_;

    Array1DImpl<float_complex>*     	timedomain_;
    Array1DImpl<float_complex>*     	freqdomain_;
    Array1DImpl<float_complex>*     	signal_;

    const DataHolder*		    	redata_;
    const DataHolder*               	imdata_;
};

}; // namespace Attrib


#endif
