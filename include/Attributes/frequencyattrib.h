#ifndef frequencyattrib_h
#define frequencyattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: frequencyattrib.h,v 1.7 2006-01-12 13:15:30 cvshelene Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"
#include "bufstringset.h"
#include "fft.h"

#include <complex>

/*!\brief Frequency Attribute

  Frequency gate=[-4,4] [normalize=No] [window=CosTaper5] [dumptofile=No]

  Calculates a number of attributes (se below) from the frequency domain
  in a gate. The gate can be windowed with the window specified in
  the window parameter prior to the Fourier Transform. If normalize is enabled,
  the frequency spectra is normalized with regard to its area. This will make
  it possible to attribare the attribs from areas with high and low energy.

  Dumptofile dumps the spectrogram at all locations to a file. This feature is
  only for experimental dGB use - don't present it in any manuals. If used wrong
  (i.e. with volume output) you will end up with a file of several Gb. The
  file is stored as /tmp/frequency.dump
  
Input:
0       Real data
1       Imag data

Output:
0       Dominant frequency (DFQ)
1       Average frequency  (AFQ)
2       Median frequency (MFQ)
3       Average frequency Squared (AFS)
4       Maximum spectral amplitude (MSA)
5       Spectral Area beyond dominant frequency (SADF)
6       Frequency Slope Fall (FSF)
7       Absorption Quality Factor (AQF)

*/


class ArrayNDWindow;
class BinID;
template<class T> class Array1DImpl;

namespace Attrib
{

class DataHolder;

class Frequency : public Provider
{
public:
    static void			initClass();
				Frequency(Desc&);

    static const char*		attribName()		{ return "Frequency"; }
    static const char*		gateStr()		{ return "gate"; }
    static const char*		normalizeStr()		{ return "normalize"; }
    static const char*		windowStr()		{ return "window"; }
    static const char*		dumptofileStr()		{ return "dumptofile"; }

protected:
    				~Frequency();
    static Provider*		createInstance(Desc&);

    bool			allowParallelComputation() const
    				{ return true; }
    bool			getInputOutput(int input,
	    				       TypeSet<int>& res) const;
    bool			getInputData(const BinID&,int idx);
    bool			computeData(const DataHolder&,const BinID& rel,
					    int z0,int nrsamples) const;

    const Interval<float>*	reqZMargin(int input,int output) const;

    Interval<float>		gate;
    Interval<int>		samplegate;
    bool			dumptofile;
    int				fftsz;
    FFT				fft;
    ArrayNDWindow*		window;
    int				windowtype;
    float			df;

    bool			normalize;

    const DataHolder*		redata;
    const DataHolder*           imdata;
    int				realidx_;
    int				imagidx_;

    BufferStringSet		dumpset;
    bool			fftisinit;

    Array1DImpl<float_complex>*	signal;
    Array1DImpl<float_complex>*	timedomain;
    Array1DImpl<float_complex>*	freqdomain;
    float*			freqdomainpower;
};

}; // namespace Attrib

#endif
