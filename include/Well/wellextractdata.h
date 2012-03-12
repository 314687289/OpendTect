#ifndef wellextractdata_h
#define wellextractdata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		May 2004
 RCS:		$Id: wellextractdata.h,v 1.30 2012-03-12 07:59:27 cvsbruno Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "bufstringset.h"
#include "position.h"
#include "ranges.h"
#include "enums.h"
#include "stattype.h"
#include "survinfo.h"

class MultiID;
class DataPointSet;
class IODirEntryList;
class IOObj;
template <class T> class Array2DImpl;


namespace Well
{
class Log;
class LogSet;
class Info;
class D2TModel;
class Data;
class Track;
class Marker;
class MarkerSet;


/*!\brief parameters (zrg, sampling method) to extract well data */

mClass ExtractParams
{
public :
    			ExtractParams() 
			    : zrg_( SI().zRange(true) ) { setEmpty(); }
    			ExtractParams(const ExtractParams&);

    static const char*	sKeyTopMrk();
    static const char*	sKeyBotMrk();
    static const char*	sKeyDataStart();
    static const char*	sKeyDataEnd();
    static const char*	sKeyLimits();
    static const char*	sKeySamplePol();
    static const char*	sKeyZExtractInTime();
    static const char*	sKeyZSelection();
    static const char*	sKeyZRange();

    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

    void		setEmpty();
    bool		isOK(BufferString* errmsg=0) const;

    BufferString	topmrkr_;
    BufferString	botmrkr_;
    float		above_;
    float		below_;
    bool		extractzintime_;

    enum		ZSelection { Markers, Depths, Times };
			DeclareEnumUtils(ZSelection);
    ZSelection		zselection_;
    StepInterval<float> zrg_;

    Stats::UpscaleType	samppol_;

    StepInterval<float>	calcFrom(const IOObj&,const BufferStringSet& lgs) const;
    StepInterval<float>	calcFrom(const Data&,const BufferStringSet& logs) const;

protected:
    void		getMarkerRange(const Data&,Interval<float>&) const;
    void		getLimitPos(const MarkerSet&,const D2TModel*,
				bool,float&,const Interval<float>&) const;
};



/*!\brief Collects info about all wells in store */

mClass InfoCollector : public ::Executor
{
public:

			InfoCollector(bool wellloginfo=true,bool markers=true);
			~InfoCollector();

    int			nextStep();
    const char*		message() const		{ return curmsg_.buf(); }
    const char*		nrDoneText() const	{ return "Wells inspected"; }
    od_int64		nrDone() const		{ return curidx_; }
    od_int64		totalNr() const		{ return totalnr_; }

    const ObjectSet<MultiID>&	ids() const	{ return ids_; }
    const ObjectSet<Info>&	infos() const	{ return infos_; }
    				//!< Same size as ids()
    const ObjectSet<MarkerSet>&	markers() const	{ return markers_; }
    				//!< If selected, same size as ids()
    const ObjectSet<BufferStringSet>& logs() const { return logs_; }
    				//!< If selected, same size as ids()


protected:

    ObjectSet<MultiID>		ids_;
    ObjectSet<Info>		infos_;
    ObjectSet<MarkerSet>	markers_;
    ObjectSet<BufferStringSet>	logs_;
    IODirEntryList*		direntries_;
    int				totalnr_;
    int				curidx_;
    BufferString		curmsg_;
    bool			domrkrs_;
    bool			dologs_;

};



/*!\brief Collects positions along selected well tracks. The DataPointSets will
  get new rows with the positions along the track. */

mClass TrackSampler : public ::Executor
{
public:

			TrackSampler(const BufferStringSet& ioobjids,
				     ObjectSet<DataPointSet>&,
				     bool zvalsintime);

    float		locradius_;
    bool		for2d_;
    bool		minidps_;
    bool		mkdahcol_;
    BufferStringSet	lognms_;

    ExtractParams	params_;

    void		usePar(const IOPar&);

    int			nextStep();
    const char*		message() const	   { return "Scanning well tracks"; }
    const char*		nrDoneText() const { return "Wells inspected"; }
    od_int64		nrDone() const	   { return curid_; }
    od_int64		totalNr() const	   { return ids_.size(); }

    const BufferStringSet&	ioObjIds() const	{ return ids_; }
    ObjectSet<DataPointSet>&	dataPointSets()		{ return dpss_; }

    static const char*	sKeySelRadius();
    static const char*	sKeyDahCol();
    static const char*	sKeyFor2D();
    static const char*	sKeyLogNm();

protected:

    const BufferStringSet&	ids_;
    ObjectSet<DataPointSet>&	dpss_;
    int				curid_;
    const bool			zistime_;
    Interval<float>		welldahrg_;
    int				dahcolnr_;

    void		getData(const Data&,DataPointSet&);
    bool		getSnapPos(const Data&,float,BinIDValue&,int&,
	    			   Coord3&) const;
    void		addPosns(DataPointSet&,const BinIDValue&,
				 const Coord3&,float dah) const;
};


/*!\brief Collects positions along selected well tracks. Will add column
   to the DataPointSet. */

mClass LogDataExtracter : public ::Executor
{
public:

			LogDataExtracter(const BufferStringSet& ioobjids,
					 ObjectSet<DataPointSet>&,
					 bool zvalsintime);

    BufferString	lognm_;
    Stats::UpscaleType	samppol_;
    static const char*	sKeyLogNm(); //!< equals address of TrackSampler's

    void		usePar(const IOPar&);

    int			nextStep();
    const char*		message() const	   { return msg_.buf(); }
    const char*		nrDoneText() const { return "Wells handled"; }
    od_int64		nrDone() const	   { return curid_; }
    od_int64		totalNr() const	   { return ids_.size(); }

    const BufferStringSet&	ioObjIds() const	{ return ids_; }

    static float	calcVal(const Log&,float dah,float winsz,
	    				Stats::UpscaleType samppol); 

protected:

    const BufferStringSet&	ids_;
    ObjectSet<DataPointSet>&	dpss_;
    int				curid_;
    const bool			zistime_;
    BufferString		msg_;

    void		getData(DataPointSet&,const Data&,const Track&);
    void		getGenTrackData(DataPointSet&,const Track&,const Log&,
	    				int,int);
    void		addValAtDah(float,const Log&,float,
	    			    DataPointSet&,int,int) const;
    float		findNearest(const Track&,const BinIDValue&,
	    			    float,float,float) const;
};



mClass SimpleTrackSampler : public Executor
{
public:
			SimpleTrackSampler(const Well::Track&,
					  const Well::D2TModel*,
					  bool extrapolate_ = false,
					  bool stayinsidesurvey = false);

    void                setSampling(const StepInterval<float>& intv)
			{ extrintv_ = intv; } //In time if d2TModel is provided

    int                 nextStep();
    od_int64            totalNr() const         { return extrintv_.nrSteps(); }
    od_int64            nrDone() const          { return nrdone_; }
    const char*         message() const         { return "Computing..."; }
    const char*         nrDoneText() const      { return "Points done"; }

    void		getBIDs(TypeSet<BinID>& bs) const { bs = bidset_; }
    void		getCoords(TypeSet<Coord>& cs) const { cs = coords_; }

protected:
    StepInterval<float> extrintv_;

    TypeSet<BinID>      bidset_;
    TypeSet<Coord>      coords_;

    bool		isinsidesurvey_;
    bool		extrapolate_;

    Interval<float>     tracklimits_;
    const Well::Track&  track_;
    const Well::D2TModel* d2t_;
    int                 nrdone_;
};



/*! brief Log resampler, extracts all the logs given by log names along a z time or dah axis !*/

mClass LogSampler : public ParallelTask
{
public:
			LogSampler(const Well::Data& wd,
				const Well::ExtractParams&,
				const BufferStringSet& lognms);

			LogSampler(const Well::Data& wd,
				const StepInterval<float>& zrg,bool extrintime,
				Stats::UpscaleType samppol,
				const BufferStringSet& lognms);
			~LogSampler();

    //avalaible after execution
    float		getDah(int idz) const;
    float		getDah(float zpos) const;

    float		getLogVal(int logidx,int idz) const;
    float		getLogVal(int logidx,float zpos) const;
    float		getLogVal(const char* lognm,int idx) const;

    const char*		errMsg() const 
			{ return errmsg_.isEmpty() ? 0 : errmsg_.buf(); }

protected:

    od_int64            	nrIterations() const;

    bool 			doPrepare(int);
    bool 			doWork(od_int64,od_int64,int);
    bool			doLog(int logidx);

    const Well::Data&		wd_;
    Array2DImpl<float>*		data_;

    const Well::D2TModel* 	d2t_;
    StepInterval<float>		zrg_;
    bool 			extrintime_;
    const BufferStringSet& 	lognms_;

    BufferString		errmsg_;
    Stats::UpscaleType 		samppol_;
};

}; // namespace Well


#endif
