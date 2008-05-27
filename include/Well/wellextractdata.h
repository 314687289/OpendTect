#ifndef wellextractdata_h
#define wellextractdata_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		May 2004
 RCS:		$Id: wellextractdata.h,v 1.18 2008-05-27 11:50:48 cvsbert Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "bufstringset.h"
#include "position.h"
#include "ranges.h"
#include "enums.h"

class MultiID;
class DataPointSet;
class IODirEntryList;


namespace Well
{
class Log;
class Info;
class Data;
class Track;
class Marker;

/*!\brief Collects info about all wells in store */

class InfoCollector : public ::Executor
{
public:

			InfoCollector(bool wellloginfo=true,bool markers=true);
			~InfoCollector();

    int			nextStep();
    const char*		message() const		{ return curmsg_.buf(); }
    const char*		nrDoneText() const	{ return "Wells inspected"; }
    int			nrDone() const		{ return curidx_; }
    int			totalNr() const		{ return totalnr_; }

    typedef ObjectSet<Marker>	MarkerSet;

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

class TrackSampler : public ::Executor
{
public:

			TrackSampler(const BufferStringSet& ioobjids,
				     ObjectSet<DataPointSet>&,
				     bool zvalsintime);

    BufferString	topmrkr;
    BufferString	botmrkr;
    BufferStringSet	lognms;
    float		above;
    float		below;
    float		locradius;
    bool		for2d;
    bool		minidps;
    bool		mkdahcol;

    void		usePar(const IOPar&);

    int			nextStep();
    const char*		message() const	   { return "Scanning well tracks"; }
    const char*		nrDoneText() const { return "Wells inspected"; }
    int			nrDone() const	   { return curid; }
    int			totalNr() const	   { return ids.size(); }

    const BufferStringSet&	ioObjIds() const	{ return ids; }
    ObjectSet<DataPointSet>&	dataPointSets()		{ return dpss; }

    static const char*	sKeyTopMrk;
    static const char*	sKeyBotMrk;
    static const char*	sKeyLimits;
    static const char*	sKeySelRadius;
    static const char*	sKeyDataStart;
    static const char*	sKeyDataEnd;
    static const char*	sKeyLogNm;
    static const char*	sKeyFor2D;
    static const char*	sKeyDahCol;

protected:

    const BufferStringSet&	ids;
    ObjectSet<DataPointSet>&	dpss;
    int				curid;
    const bool			zistime;
    Interval<float>		fulldahrg;
    int				dahcolnr;

    void		getData(const Data&,DataPointSet&);
    void		getLimitPos(const ObjectSet<Marker>&,bool,float&) const;
    bool		getSnapPos(const Data&,float,BinIDValue&,int&,
	    			   Coord3&) const;
    void		addPosns(DataPointSet&,const BinIDValue&,
				 const Coord3&,float dah) const;
};


/*!\brief Collects positions along selected well tracks. Will add column
   to the DataPointSet. */

class LogDataExtracter : public ::Executor
{
public:

    enum SamplePol	{ Avg, Med, MostFreq, Nearest };
    			DeclareEnumUtils(SamplePol)

			LogDataExtracter(const BufferStringSet& ioobjids,
					 ObjectSet<DataPointSet>&,
					 bool zvalsintime);

    BufferString	lognm_;
    SamplePol		samppol_;
    static const char*	sKeySamplePol;
    static const char*	sKeyLogNm; //!< equals address of TrackSampler's

    void		usePar(const IOPar&);

    int			nextStep();
    const char*		message() const	   { return msg_.buf(); }
    const char*		nrDoneText() const { return "Wells handled"; }
    int			nrDone() const	   { return curid_; }
    int			totalNr() const	   { return ids_.size(); }

    const BufferStringSet&	ioObjIds() const	{ return ids_; }

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
    float		calcVal(const Log&,float,float) const;
    float		findNearest(const Track&,const BinIDValue&,
	    			    float,float,float) const;
};


}; // namespace Well


#endif
