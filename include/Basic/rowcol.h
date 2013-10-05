#ifndef rowcol_h
#define rowcol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		12-8-1997
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "rcol.h"

template <class T> class TypeSet;
class BinID;
class BufferString;

/*!
\brief Object with row and col. RowCol has most functions in common with
BinID, so template-based functions can be based on both classes.
*/

mExpClass(Basic) RowCol
{
public:
    inline			RowCol(int r,int c);
    inline			RowCol(const RowCol&);
				RowCol(const BinID&);
    inline			RowCol();

    inline bool			operator==(const RowCol&) const;
    inline bool			operator!=(const RowCol&) const;
    inline RowCol		operator+(const RowCol&) const;
    inline RowCol	 	operator-(const RowCol&) const;
    inline RowCol		operator+() const;
    inline RowCol		operator-() const;
    inline RowCol		operator*(const RowCol&) const;
    inline RowCol		operator*(int) const;
    inline RowCol		operator/(const RowCol&) const;
    inline RowCol		operator/(int) const;
    inline const RowCol&	operator+=(const RowCol&);
    inline const RowCol&	operator-=(const RowCol&);
    inline const RowCol&	operator*=(const RowCol&);
    inline const RowCol&	operator*=(int);
    inline const RowCol&	operator/=(const RowCol&);
    inline int&			operator[](int idx);
    inline int			operator[](int idx) const;
    void			fill(BufferString&) const;
    bool			use(const char*);
    inline od_int64		toInt64() const;
    static inline RowCol	fromInt64(od_int64);
    inline int			toInt32() const;
    static inline RowCol	fromInt32(int);
    int				sqDistTo(const RowCol&) const;
    bool			isNeighborTo(const RowCol&,const RowCol&,
					     bool eightconnectivity=true) const;

    RowCol			getDirection() const;
    		/*!<\returns a rowcol where row/col are either -1, 0 or 1 where
		    depending on if row/col of the object is negative, zero or
		    positive. */

    float			angleTo(const RowCol&) const;
		/*!<\returns the smallest angle between the vector
		      going from 0,0 to the object and the vector
		      going from 0,0 to rc.*/
    float			clockwiseAngleTo(const RowCol& rc) const;
    		/*!<\returns the angle between the vector going from
		     0,0 to the object and the vector going from 0,0
		     to rc in the clockwise direction.*/
    float			counterClockwiseAngleTo(const RowCol&) const;
		/*!<\returns the angle between the vector going from
		      0,0 to the object and the vector going from 0,0
		      to rc in the counterclockwise direction.*/


    int				row;
    int				col;

    static const TypeSet<RowCol>&	clockWiseSequence();

};


mImplInlineRowColFunctions(RowCol, row, col);


#endif

