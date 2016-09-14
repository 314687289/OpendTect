#ifndef survgeom2d_h
#define survgeom2d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2010
________________________________________________________________________

-*/


#include "basicmod.h"
#include "survgeom.h"

class DBKey;
namespace PosInfo { class Line2DData; }


namespace Survey
{

/*!\brief Geometry of a 2D Line.  */

mExpClass(Basic) Geometry2D : public Geometry
{
public:
				Geometry2D(PosInfo::Line2DData*);
				//!<Line2DData becomes mine
    virtual bool		is2D() const		{ return true; }
    virtual const char*		getName() const;

    virtual Coord		toCoord(int linenr,int tracenr) const;
    virtual TrcKey		nearestTrace(const Coord&,float* dist) const;

    virtual bool		includes(int linenr,int tracenr) const;

    PosInfo::Line2DData&	dataAdmin()		{ return data_; }
				//!<If data is changed, call touch afterwards
    void			touch();
    const PosInfo::Line2DData&	data() const		{ return data_; }

    StepInterval<float>		zRange() const;

    static BufferString		makeUniqueLineName(const char* lsnm,
						   const char* lnm);
    float			averageTrcDist() const;
    RelationType		compare(const Geometry&,bool usezrg) const;

    Geometry2D*			as2D()			{ return this; }
    static ID			getIDFrom(const DBKey&);

private:

				~Geometry2D();


    PosInfo::Line2DData&	data_;
    mutable float		trcdist_;
    mutable Threads::Lock	lock_;
};

} // namespace Survey

#endif
