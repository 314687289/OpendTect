#ifndef welltrack_h
#define welltrack_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id$
________________________________________________________________________


-*/

#include "wellmod.h"
#include "welldahobj.h"
#include "position.h"

namespace Well
{

class D2TModel;

/*!
\brief Well track
*/

mExpClass(Well) Track : public DahObj
{
public:

			Track( const char* nm=0 )
			: DahObj(nm), zistime_(false) {}
			Track( const Track& t )
			: DahObj("")		{ *this = t; }
    Track&		operator =(const Track&);

    const Coord3&	pos( int idx ) const	{ return pos_[idx]; }
    float		value( int idx ) const	{ return (float) pos_[idx].z; }
    float		getKbElev() const	{ return dah_[0] - value(0); }
    int			nrPoints() const	{ return pos_.size(); }
    bool		zIsTime() const		{ return zistime_; }
    const Interval<float> zRange() const;
    			//!< returns (0, 0) for empty track
    const Interval<float> dahRange() const;
    			//!< returns (0, 0) for empty track

    int			insertPoint(const Coord&,float z);
    			//!< a 'good' place will be found
    			//!< If inserted at top, z will be used as first dah
    			//!< returns position index of the new point
    void		addPoint(const Coord&,float z,float dah=mUdf(float));
    			//!< Point must be further down track. No checks.
    void		setPoint(int,const Coord&,float z);
    			//!< Will correct all dahs below point
    void		insertAfterIdx(int,const Coord3&);
    			//!< Know what you're doing - not used normally
    void		removePoint(int);
    			//!< Will correct all dahs below point

    Coord3		getPos(float d_ah) const;
    const TypeSet<Coord3>& getAllPos() const { return pos_; }

    float		getDahForTVD(float,float prevdah=mUdf(float)) const;
    			//!< Non-unique. previous DAH may be helpful
    			//!< Don't use if track is in time
    float		nearestDah(const Coord3&) const;
    			// If zIsTime() z must be time

    			// If you know what you're doing:
    Coord3		coordAfterIdx(float d_ah,int) const;
    			//!< Beware: no bounds check on index.

    bool		insertAtDah(float dah,float zpos);
    			//!< will interpolate x,y coords

    bool		alwaysDownward() const;
    void		toTime(const D2TModel&, const Track&);

protected:


    TypeSet<Coord3>	pos_;
    bool		zistime_;

    void		removeAux( int idx )	{ pos_.removeSingle(idx); }
    void		eraseAux()		{ pos_.erase(); }

};


}; // namespace Well

#endif

