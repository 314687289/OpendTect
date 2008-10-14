#ifndef posinfodetector_h
#define posinfodetector_h
/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Oct 2008
 RCS:		$Id: posinfodetector.h,v 1.3 2008-10-14 12:11:14 cvsbert Exp $
________________________________________________________________________

*/

#include "posinfo.h"
class IOPar;
class HorSampling;
class BinIDSorting;
class BinIDSortingAnalyser;


namespace PosInfo
{

/*!\brief Just hold inl, crl, x, y and offs. For 2D, crl=nr. */

class CrdBidOffs
{
public:
		CrdBidOffs()
		    : coord_(0,0), binid_(1,0), offset_(0)	{}
		CrdBidOffs( const Coord& c, int nr, float o=0 )
		    : coord_(c), binid_(1,nr), offset_(o)	{}
		CrdBidOffs( const Coord& c, const BinID& b, float o=0 )
		    : coord_(c), binid_(b), offset_(o)		{}
    bool	operator ==( const CrdBidOffs& cbo ) const
		{ return binid_ == cbo.binid_
		      && mIsEqual(offset_,cbo.offset_,mDefEps); }

    Coord	coord_;
    BinID	binid_;
    float	offset_;
};


/*!\brief Determines many geometry parameters from a series of Coords with
    corresponding BinID or trace numbers and offsets if pre-stack. */


class Detector
{
public:

    struct Setup
    {
			Setup( bool istwod )
			    : is2d_(istwod), isps_(false)
			    , reqsorting_(false)		{}
	mDefSetupMemb(bool,is2d)
	mDefSetupMemb(bool,isps)
	mDefSetupMemb(bool,reqsorting)
    };

    			Detector(const Setup&);
    bool		is2D() const		{ return setup_.is2d_; }
    bool		isPS() const		{ return setup_.isps_; }
    void		reInit();

    bool		add(const Coord&,const BinID&);
    bool		add(const Coord&,const BinID&,float offs);
    bool		add(const Coord&,int nr);
    bool		add(const Coord&,int nr,float offs);
    bool		add(const Coord&,const BinID&,int nr,float offs);
    bool		add(const CrdBidOffs&);

    bool		finish();
    bool		usable() const		{ return !*errMsg(); }
    const char*		errMsg() const		{ return errmsg_.buf(); }

    int			nrPositions( bool uniq=true ) const
			{ return uniq ? nruniquepos_ : nrpos_; }

    			// available after finish()
    Coord		minCoord() const	{ return mincoord_; }
    Coord		maxCoord() const	{ return maxcoord_; }
    BinID		start() const		{ return start_; }
    BinID		stop() const		{ return stop_; }
    BinID		step() const		{ return step_; }
    Interval<float>	offsRg() const		{ return offsrg_; }
    float		avgDist() const		{ return avgdist_; }
    CrdBidOffs		firstPosition() const	{ return userCBO(firstcbo_); }
    CrdBidOffs		lastPosition() const	{ return userCBO(lastcbo_); }
    bool		haveGaps( bool inldir=false ) const
			{ return inldir ? inlirreg_ : crlirreg_; }

    void		report(IOPar&) const;

    const BinIDSorting&	sorting() const		{ return sorting_; }
    bool		inlSorted() const;
    bool		crlSorted() const;
    void		getCubeData(CubeData&) const;
    			//!< if crlSorted(), inl and crl are swapped
    const char*		getSurvInfo(HorSampling&,Coord crd[3]) const;

    StepInterval<int>	getRange(bool inldir=false) const;

protected:

    Setup		setup_;
    BinIDSorting&	sorting_;
    ObjectSet<LineData>	lds_;
    int			nruniquepos_;
    int			nrpos_;
    Coord		mincoord_;
    Coord		maxcoord_;
    Interval<float>	offsrg_;
    int			nroffsperpos_;
    bool		allstd_;
    BinID		start_;
    BinID		stop_;
    BinID		step_;
    bool		inlirreg_, crlirreg_;
    Interval<float>	distrg_;	//!< 2D
    float		avgdist_;	//!< 2D
    CrdBidOffs		firstcbo_;
    CrdBidOffs		lastcbo_;
    CrdBidOffs		llnstart_;	//!< in 3D, of longest line
    CrdBidOffs		llnstop_;	//!< in 3D, of longest line
    CrdBidOffs		firstduppos_;
    CrdBidOffs		firstaltnroffs_;

    BinIDSortingAnalyser* sortanal_;
    TypeSet<CrdBidOffs>	cbobuf_;
    int			curline_;
    int			curseg_;
    CrdBidOffs		curcbo_;
    CrdBidOffs		prevcbo_;
    CrdBidOffs		curlnstart_;
    int			nroffsthispos_;
    BufferString	errmsg_;

    bool		applySortAnal();
    void		addFirst(const PosInfo::CrdBidOffs&);
    bool		addNext(const PosInfo::CrdBidOffs&);
    void		addLine();
    void		addPos();
    void		setCur(const PosInfo::CrdBidOffs&);
    void		getBinIDRanges();
    int			getStep(bool inl) const; //!< smallest

    CrdBidOffs		workCBO(const CrdBidOffs&) const;
    CrdBidOffs		userCBO(const CrdBidOffs&) const;

};


} // namespace

#endif
