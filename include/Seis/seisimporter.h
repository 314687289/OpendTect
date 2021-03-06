#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2006
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "executor.h"
#include "bufstring.h"
class IOObj;
class Scaler;
class SeisTrc;
class SeisTrcBuf;
class BinIDSorting;
class SeisTrcWriter;
class SeisResampler;
class BinIDSortingAnalyser;
namespace Seis { class SelData; class Provider; }

/*!\brief Helps import or export of seismic data. */

mExpClass(Seis) SeisImporter : public Executor
{ mODTextTranslationClass(SeisImporter);
public:

    /*!<\brief provides traces from the import storage

      fetch() must return false at end or when an error occurs.
      On error, the errmsg_ must be filled.

      A SeisStdImporterReader based on SeisTrcReader is available.

      */

    mStruct(Seis) Reader
    {
	virtual			~Reader()			{}

	virtual const char*	name() const			= 0;
	virtual const char*	implName() const		= 0;
	virtual bool		fetch(SeisTrc&)			= 0;
	virtual od_int64	totalNr() const			{ return -1; }

	uiString		errmsg_;
    };


			SeisImporter(Reader*,SeisTrcWriter&,Seis::GeomType);
				//!< Reader becomes mine. Has to be non-null.
    virtual		~SeisImporter();

    uiString		message() const;
    od_int64		nrDone() const;
    uiString		nrDoneText() const;
    od_int64		totalNr() const;
    int			nextStep();
    int			Finished();
    uiString		errorMsg() const
			{ return errmsg_; }

    int			nrSkipped() const	{ return nrskipped_; }
    Reader&		reader()		{ return *rdr_; }
    SeisTrcWriter&	writer()		{ return wrr_; }

protected:

    enum State			{ ReadBuf, WriteBuf, ReadWrite };

    Reader*			rdr_;
    SeisTrcWriter&		wrr_;
    int				queueid_;
    int				maxqueuesize_;
    Threads::ConditionVar&	lock_;
    SeisTrcBuf&			buf_;
    SeisTrc&			trc_;
    BinID&			prevbid_;
    int				sort2ddir_;
    BinIDSorting*		sorting_;
    BinIDSortingAnalyser*	sortanal_;
    Seis::GeomType		geomtype_;
    State			state_;
    int				nrread_;
    int				nrwritten_;
    int				nrskipped_;

    bool			sortingOk(const SeisTrc&);
    int				doWrite(SeisTrc&);
    int				readIntoBuf();

    friend			class SeisImporterWriterTask;
    void			reportWrite(const uiString&);

    mutable uiString		errmsg_;
    mutable uiString	        hndlmsg_;
};


mExpClass(Seis) SeisStdImporterReader : public SeisImporter::Reader
{ mODTextTranslationClass(SeisStdImporterReader);
public:
			SeisStdImporterReader(const IOObj&,const char* nm);
			~SeisStdImporterReader();

    Seis::Provider*	provider()		{ return prov_; }

    const char*		name() const		{ return name_; }
    const char*		implName() const;
    bool		fetch(SeisTrc&);

    void		removeNull( bool yn )	{ remnull_ = yn; }
    void		setResampler(SeisResampler*);	//!< becomes mine
    void		setScaler(Scaler*);		//!< becomes mine
    void		setSelData(Seis::SelData*);	//!< becomes mine

    od_int64		totalNr() const;

protected:

    const BufferString	name_;
    Seis::Provider*	prov_;
    bool		remnull_;
    SeisResampler*	resampler_;
    Scaler*		scaler_;

};
