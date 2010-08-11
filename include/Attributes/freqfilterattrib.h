#ifndef freqfilterattrib_h
#define freqfilterattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          February 2003
 RCS:           $Id: freqfilterattrib.h,v 1.16 2010-08-11 16:55:33 cvsyuancheng Exp $
________________________________________________________________________

-*/


#include "attribprovider.h"
#include "arrayndutils.h"
#include "arrayndimpl.h"
#include "fourier.h"
#include <complex>


/*!\brief Frequency filtering Attribute

  FreqFilter type=LowPass,HighPass,BandPass minfreq= maxfreq= nrpoles=
             isfftfilter= window=

Input:                                  ||
0       Real data                       ||0     Real Data
                                        ||1     Imaginary Data
Output:                                 ||
0       Frequency filtered data         ||0     Frequency filtered data
          (Butterworth Filter)          ||           (FFT Filter)

*/

namespace Attrib
{

mClass FreqFilter: public Provider
{
public:
    static void		initClass();
			FreqFilter(Desc&);

    static const char*  attribName()		{ return "FreqFilter"; }
    static const char*  filtertypeStr()		{ return "type"; }
    static const char*  minfreqStr()    	{ return "minfreq"; }
    static const char*  maxfreqStr()            { return "maxfreq"; }
    static const char*  nrpolesStr()            { return "nrpoles"; }
    static const char*  isfftfilterStr()        { return "isfftfilter"; }
    static const char*  isfreqtaperStr()        { return "isfreqtaper"; }
    static const char*  windowStr()             { return "window"; }
    static const char*  fwindowStr()             { return "fwindow"; }
    static const char*  paramvalStr()           { return "paramval"; }
    static const char*  highfreqparamvalStr()   { return "highfreqparamval"; }
    static const char*  lowfreqparamvalStr()    { return "lowfreqparamval"; }
    static const char*  filterTypeNamesStr(int);

protected:
    			~FreqFilter();
    static Provider*    createInstance(Desc&);
    static void         updateDesc(Desc&);

    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&, int idx);
    bool		computeData(const DataHolder&,const BinID& relpos,
				    int t0,int nrsamples,int threadid) const;
    void		butterWorthFilter(const DataHolder&, int, int);
    void		fftFilter(const DataHolder&, int, int);

    void 		setSz(int sz);
    
    const Interval<int>*	desZSampMargin(int input,int output) const;

    int				filtertype_;
    float 			minfreq_;
    float                       maxfreq_;
    int				nrpoles_;
    bool			isfftfilter_;
    Fourier::CC*		fft_;
    Fourier::CC*		fftinv_;
    int                         fftsz_;

    ArrayNDWindow*              window_;
    BufferString                windowtype_;
    float                       variable_;
    float                       highfreqvariable_;
    float                       lowfreqvariable_;

    Interval<int>		zmargin_;

    Array1DImpl<float_complex>  signal_;
    Array1DImpl<float_complex>  timedomain_;
    Array1DImpl<float_complex>  freqdomain_;
    Array1DImpl<float_complex>  tmpfreqdomain_;
    Array1DImpl<float_complex>  timecplxoutp_;
    
    const DataHolder*		redata_;
    const DataHolder*		imdata_;

    int				realidx_;
    int				imagidx_;
};

}; // namespace Attrib

#endif
