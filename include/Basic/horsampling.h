#ifndef horsampling_h
#define horsampling_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: horsampling.h,v 1.2 2008-10-07 10:12:09 cvsnanne Exp $
________________________________________________________________________

-*/

#include "ranges.h"
#include "position.h"

class IOPar;


/*\brief Horizontal sampling (inline and crossline range and steps) */

class HorSampling
{
public:
			HorSampling( bool settoSI=true ) { init(settoSI); }
    HorSampling&	set(const Interval<int>& inlrg,
	    		    const Interval<int>& crlrg);
    			//!< steps copied if available
    void		get(Interval<int>& inlrg,Interval<int>& crlrg) const;
    			//!< steps filled if available
			
    StepInterval<int>	inlRange() const;
    StepInterval<int>	crlRange() const;

    inline bool		includes( const BinID& bid ) const
			{ return inlOK(bid.inl) && crlOK(bid.crl); }

    inline bool		inlOK( int inl ) const
			{ return inl >= start.inl && inl <= stop.inl
			      && !( (inl-start.inl) % step.inl ); }
    inline bool		crlOK( int crl ) const
			{ return crl >= start.crl && crl <= stop.crl
			      && !( (crl-start.crl) % step.crl ); }

    inline void		include( const BinID& bid )
			{ includeInl(bid.inl); includeCrl(bid.crl); }
    void		includeInl( int inl );
    void		includeCrl( int crl );
    bool		isDefined() const;
    void		limitTo(const HorSampling&);
    void		limitToWithUdf(const HorSampling&);
    			/*!< handles undef values +returns reference horsampling
			     nearest limit if horsamplings do not intersect */

    inline int		inlIdx( int inl ) const
			{ return (inl - start.inl) / step.inl; }
    inline int		crlIdx( int crl ) const
			{ return (crl - start.crl) / step.crl; }
    BinID		atIndex( int i0, int i1 ) const
			{ return BinID( start.inl + i0*step.inl,
					start.crl + i1*step.crl ); }
    int			nrInl() const;
    int			nrCrl() const;
    inline int		totalNr() const	{ return nrInl() * nrCrl(); }
    inline bool		isEmpty() const { return nrInl() < 1 || nrCrl() < 1; }

    void		init(bool settoSI=true);
    			//!< Sets to survey values or mUdf(int) (but step 1)
    void		set2DDef();
    			//!< Sets ranges to 0-maxint
    void		normalise();
    			//!< Makes sure start<stop and steps are non-zero

    bool		getInterSection(const HorSampling&,HorSampling&) const;
    			//!< Returns false if intersection is empty

    void		snapToSurvey();
    			/*!< Checks if it is on valid bids. If not, it will
			     expand until it is */

    bool		operator==( const HorSampling& hs ) const
			{ return hs.start==start && hs.stop==stop 
			    			 && hs.step==step; }

    bool		usePar(const IOPar&);	//!< Keys as in keystrs.h
    void		fillPar(IOPar&) const;	//!< Keys as in keystrs.h
    static void		removeInfo(IOPar&);

    BinID		start;
    BinID		stop;
    BinID		step;

};


//\brief Finds next BinID in HorSampling; initializes to first position

class HorSamplingIterator
{
public:
    			HorSamplingIterator( const HorSampling& hs )
			    : hrg_(hs)	{ reset(); }

    void		reset()		{ firstpos_ = true; }
    bool		next(BinID&);

protected:

    const HorSampling	hrg_;
    bool		firstpos_;

};


#endif
