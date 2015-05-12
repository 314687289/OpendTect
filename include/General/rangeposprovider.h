#ifndef rangeposprovider_h
#define rangeposprovider_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id$
________________________________________________________________________


-*/

#include "generalmod.h"
#include "posprovider.h"

namespace PosInfo { class Line2DData; }

namespace Pos
{

/*!\brief 3D provider based on CubeSampling */

mExpClass(General) RangeProvider3D : public Provider3D
{
public:

			RangeProvider3D();
			RangeProvider3D(const RangeProvider3D&);
			~RangeProvider3D();
    RangeProvider3D&	operator =(const RangeProvider3D&);
    const char*		type() const;	//!< sKey::Range()
    const char*		factoryKeyword() const { return type(); }
    virtual Provider*	clone() const	{ return new RangeProvider3D(*this); }

    virtual void	reset();

    virtual bool	toNextPos();
    virtual bool	toNextZ();

    virtual BinID	curBinID() const	{ return curbid_; }
    virtual float	curZ() const;		
    virtual bool	includes(const BinID&,float z=mUdf(float)) const;
    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
    virtual void	getSummary(BufferString&) const;

    virtual void	getExtent(BinID& start,BinID& stop) const;
    virtual void	getZRange(Interval<float>&) const;
    virtual od_int64	estNrPos() const;
    virtual int		estNrZPerPos() const;

    CubeSampling&	sampling()		{ return cs_; }
    const CubeSampling&	sampling() const	{ return cs_; }
	void		setSampling( const CubeSampling& cs ) const;

    virtual bool	includes( const Coord& c, float z=mUdf(float) ) const
			{ return Pos::Provider3D::includes(c,z); }

protected:

    CubeSampling&	cs_;
    BinID		curbid_;
    int			curzidx_;

public:

    static void		initClass();
    static Provider3D*	create()	{ return new RangeProvider3D; }

};


/*!\brief 2D provider based on StepInterval<int>.

Can only be used if Line2DData is filled.

 */

mExpClass(General) RangeProvider2D : public Provider2D
{
public:

			RangeProvider2D();
			RangeProvider2D(const RangeProvider2D&);
			~RangeProvider2D();

    RangeProvider2D&	operator =(const RangeProvider2D&);
    const char*		type() const;	//!< sKey::Range()
    const char*		factoryKeyword() const { return type(); }
    virtual Provider*	clone() const	{ return new RangeProvider2D(*this); }

    virtual void	reset();

    virtual bool	toNextPos();
    virtual bool	toNextZ();

    virtual int		curNr() const;
    virtual float	curZ() const;
    virtual Coord	curCoord() const;
    virtual bool	includes(int,float z=mUdf(float),int lidx=0) const;
    virtual bool	includes(const Coord&,float z=mUdf(float)) const;
    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
    virtual void	getSummary(BufferString&) const;

    virtual void	getExtent( Interval<int>& rg, int lidx=-1 ) const;
    virtual void	getZRange( Interval<float>& rg, int lidx ) const;
    virtual od_int64	estNrPos() const;
    virtual int		estNrZPerPos() const;

    StepInterval<int>&		trcRange(int lidx)
    				{ return trcrgs_[lidx];}
    const StepInterval<int>&	trcRange(int lidx) const
    				{return trcrgs_[lidx];}
    
    StepInterval<float>&	zRange(int lidx=0)
    				{ return zrgs_[lidx]; }
    const StepInterval<float>&	zRange(int lidx=0) const
    				{return zrgs_[lidx];}

protected:

    TypeSet< StepInterval<int> > trcrgs_;
    TypeSet< StepInterval<float> > zrgs_;
    int			curtrcidx_;
    int			curlineidx_;
    int			curzidx_;

    PosInfo::Line2DData*	curlinegeom_;

    const PosInfo::Line2DData*	curGeom() const;
    StepInterval<float>		curZRange() const;
    StepInterval<int>		curTrcRange() const;

public:

    static void		initClass();
    static Provider2D*	create()	{ return new RangeProvider2D; }

};

} // namespace

#endif
