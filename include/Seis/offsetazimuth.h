#ifndef offsetazimuth_h
#define offsetazimuth_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		March 2007
 RCS:		$Id: offsetazimuth.h,v 1.3 2007-06-21 21:26:28 cvskris Exp $
________________________________________________________________________

-*/

#include "position.h"

/*!Stores offset and azimuth as an int, wich makes it easy to compare them
without having to think of epsilons when comparing.

The offset has a precision of 0.1 meter and have the range of -419430 and
419430 meters.  The azimuth has a about 1.5 bins per degree (511 bins per full
circle).
*/

class OffsetAzimuth
{
public:
			OffsetAzimuth() : offsetazi_( 0 ) 		{}
			OffsetAzimuth(float off,float azi);
    bool		operator==(const OffsetAzimuth&) const;
    bool		operator!=(const OffsetAzimuth&) const;

    int			asInt() const;
    void		setFrom(int);

    float		offset() const;
    float		azimuth() const;
    bool		isAzimuthUndef() const;
    bool		isOffsetUndef() const;
    void		setOffset(float);
    void		setAzimuth(float);
    float		distanceTo(const OffsetAzimuth&,bool sq=false) const;
    			/*!\If sq is true, the square distance is returned. */

    Coord		srcRcvPos(const Coord& center,bool add=true) const;
			/*!<\returns center + (or - depending on variable add)
			    the object's  offset and azimuth. */

protected:

    int			offsetazi_;

};


#endif
