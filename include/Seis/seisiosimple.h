#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2003
-*/

#include "seiscommon.h"
#include "samplingdata.h"
#include "dbkey.h"
#include "position.h"
#include "executor.h"
#include "od_iosfwd.h"
#include "uistring.h"
class Scaler;
class SeisTrc;
class SeisImporter;
class SeisTrcWriter;
class SeisResampler;
namespace Seis { class SelData; class Provider; }


mExpClass(Seis) SeisIOSimple : public Executor
{ mODTextTranslationClass(SeisIOSimple);
public:

    mExpClass(Seis) Data
    {
    public:
			Data(const char*,Seis::GeomType);
			Data(const Data&);
			~Data();
	Data&		operator =(const Data&);

	BufferString	fname_;
	DBKey		seiskey_;

	bool		isasc_;
	Seis::GeomType	geom_;

	bool		havepos_;
	bool		isxy_;

	bool		havenr_;
	SamplingData<int> nrdef_;

	bool		haverefnr_;

	bool		havesd_;
	SamplingData<float> sd_;
	int		nrsamples_;
	int		compidx_;

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
	Pos::GeomID	geomid_;

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
    uiString		message() const;
    od_int64		nrDone() const;
    od_int64		totalNr() const;
    uiString		nrDoneText() const;

protected:

    Data		data_;
    bool		isimp_;
    bool		isps_;

    SeisTrc&		trc_;
    od_stream*		strm_;
    Seis::Provider*	prov_;
    SeisTrcWriter*	wrr_;
    SeisImporter*	importer_;
    bool		firsttrc_;
    int			nrdone_;
    int			offsnr_;
    int			prevnr_;
    BinID		prevbid_;
    uiString		errmsg_;
    const bool		zistm_;

    void		startImpRead();
    int			readImpTrc(SeisTrc&);
    int			readExpTrc();
    int			writeExpTrc();
    od_istream&		iStream();
    od_ostream&		oStream();

    friend class	SeisIOSimpleImportReader;

};
