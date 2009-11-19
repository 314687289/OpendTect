/*+
________________________________________________________________________
           
 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nageswara
 Date:          July 2009
 RCS:           $Id: waveletextractor.h,v 1.3 2009-11-19 10:21:20 cvsnageswara Exp $ 
 ________________________________________________________________________
                 
-*/   

#include "executor.h"

class FFT;
class IOObj;
namespace Seis { class SelData; }
class SeisTrc;
class SeisTrcReader;
class Wavelet;
template <class T> class Array1DImpl;

mClass WaveletExtractor : public Executor
{
public:
				WaveletExtractor(const IOObj&,int wvltsize);
				~WaveletExtractor();

    void			setSelData(const Seis::SelData&); // 3D
    void			setSelData(ObjectSet<Seis::SelData>&); // 2D
    void			setPhase(int phase);
    void			setCosTaperParamVal(float paramval);
    Wavelet			getWavelet();

protected:

    void			initFFT();
    void			initWavelet();
    void			init2D();
    void			init3D();
    bool			getSignalInfo(const SeisTrc&,
	    				      int& start,int& signalsz) const;
    bool			getNextLine(); //2D
    bool			processTrace(const SeisTrc&,
	    				     int start, int signalsz);
    void			normalisation(Array1DImpl<float>&);
    bool			finish(int nrusedtrcs);
    bool			doWaveletIFFT();
    bool			rotateWavelet();
    bool			taperWavelet();

    int				nextStep();
    od_int64			totalNr() const	{ return totalnr_ ; }
    od_int64			nrDone() const	{ return nrdone_; }
    const char*			nrDoneText() const;
    const char*			message() const;

    Wavelet&			wvlt_;
    const IOObj&		iobj_;
    const Seis::SelData*	sd_;
    ObjectSet<Seis::SelData>    sdset_;
    SeisTrcReader*		seisrdr_;
    FFT*			fft_;
    int				lineidx_;
    float			paramval_;
    int				wvltsize_;
    int				phase_;
    int				nrusedtrcs_;
    int				nrdone_;
    bool			isdouble_;
    od_int64			totalnr_;
    BufferString		msg_;
};
