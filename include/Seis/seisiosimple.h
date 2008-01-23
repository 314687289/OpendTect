#ifndef seisiosimple_h
#define seisiosimple_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Nov 2003
-*/

#include "samplingdata.h"
#include "multiid.h"
#include "position.h"
#include "executor.h"
#include "seistype.h"
class IOPar;
class Scaler;
class SeisTrc;
class LineKey;
class CtxtIOObj;
class StreamData;
class uiGenInput;
class uiIOObjSel;
class uiFileInput;
class SeisImporter;
class SeisTrcReader;
class SeisTrcWriter;
class SeisResampler;
namespace Seis { class SelData; }


class SeisIOSimple : public Executor
{
public:

    class Data
    {
    public:
			Data(const char*,Seis::GeomType);
			Data(const Data&);
			~Data();
	Data&		operator =(const Data&);

	BufferString	fname_;
	MultiID		seiskey_;

	bool		isasc_;
	Seis::GeomType	geom_;

	bool		havepos_;
	bool		isxy_;

	bool		havenr_;
	SamplingData<int> nrdef_;

	bool		havesd_;
	SamplingData<float> sd_;
	int		nrsamples_;

			// PS only
	bool		haveoffs_;
	SamplingData<float> offsdef_;
	int		nroffsperpos_;
	bool		haveazim_;

			// 3D only
	SamplingData<int> inldef_;
	SamplingData<int> crldef_;
	int		nrcrlperinl_;

			// 2D only
	Coord		startpos_;
	Coord		steppos_;
	LineKey&	linekey_;

	Scaler*		scaler_;
	SeisResampler*	resampler_;
	bool		remnull_;
	IOPar&		subselpars_;

	void		clear(bool survchanged);
	void		setScaler(Scaler*);
			//!< passed obj will be cloned
	void		setResampler(SeisResampler*);
			//!< passed obj will become mine
    };

			SeisIOSimple(const Data&,bool imp);
			~SeisIOSimple();

    int			nextStep();
    const char*		message() const;
    int			nrDone() const;
    int			totalNr() const;
    const char*		nrDoneText() const;

protected:

    Data		data_;
    bool		isimp_;
    bool		isps_;

    StreamData&		sd_;
    SeisTrc&		trc_;
    SeisTrcReader*	rdr_;
    SeisTrcWriter*	wrr_;
    SeisImporter*	importer_;
    bool		firsttrc_;
    int			nrdone_;
    int			offsnr_;
    int			prevnr_;
    BinID		prevbid_;
    BufferString	errmsg_;

    void		startImpRead();
    int			readImpTrc(SeisTrc&);
    int			readExpTrc();
    int			writeExpTrc();

    friend class	SeisIOSimpleImportReader;

};


#endif
