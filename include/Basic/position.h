#ifndef position_h
#define position_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		21-6-1996
 Contents:	Positions: Inline/crossline and Coordinate
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"
#include "rcol.h"
#include "geometry.h"
#include "undefval.h"

class BufferString;
class RowCol;


/*!
\brief A cartesian coordinate in 2D space.
*/

mExpClass(Basic) Coord : public Geom::Point2D<double>
{
public:
		Coord( const Geom::Point2D<double>& p )
		    :  Geom::Point2D<double>( p )			{}
		Coord() :  Geom::Point2D<double>( 0, 0 )		{}
		Coord( double cx, double cy )	
		    :  Geom::Point2D<double>( cx, cy )			{}

    bool	operator==( const Coord& crd ) const
		{ return mIsEqual(x,crd.x,mDefEps)
		      && mIsEqual(y,crd.y,mDefEps); }
    bool	operator!=( const Coord& crd ) const
		{ return ! (crd == *this); }
    bool	operator<(const Coord&crd) const
		{ return x<crd.x || (x==crd.x && y<crd.y); }
    bool	operator>(const Coord&crd) const
		{ return x>crd.x || (x==crd.x && y>crd.y); }

    double	angle(const Coord& from,const Coord& to) const;
    double	cosAngle(const Coord& from,const Coord& to) const;
    		//!< saves the expensive acos() call
		//
    Coord	normalize() const;
    double	dot(const Coord&) const;

    const char*	getUsrStr() const;
    bool	parseUsrStr(const char*);
    
    static const Coord&		udf();
    inline bool isUdf() const		{ return !isDefined(); }
};

bool getDirectionStr( const Coord&, BufferString& );
/*!< Returns strings like 'South-West', NorthEast depending on the given
     coord that is assumed to have the x-axis pointing northwards, and the
     y axis pointing eastwards
*/


/*!
\brief A cartesian coordinate in 3D space.
*/

mExpClass(Basic) Coord3 : public Coord
{
public:

			Coord3() : z(0)					{}
			Coord3(const Coord& a, double z_ )
			    : Coord(a), z(z_)				{}
			Coord3(const Coord3& xyz )
			    : Coord( xyz.x, xyz.y )
			    , z( xyz.z )				{}
    			Coord3( double x_, double y_, double z_ )
			    : Coord(x_,y_), z(z_)			{}

    double&		operator[]( int idx )
			{ return idx ? (idx==1 ? y : z) : x; }
    double		operator[]( int idx ) const
			{ return idx ? (idx==1 ? y : z) : x; }

    inline Coord3	operator+(const Coord3&) const;
    inline Coord3	operator-(const Coord3&) const;
    inline Coord3	operator-() const;
    inline Coord3	operator*(double) const;
    inline Coord3	operator/(double) const;
    inline Coord3	scaleBy( const Coord3& ) const;
    inline Coord3	unScaleBy( const Coord3& ) const;

    inline Coord3&	operator+=(const Coord3&);
    inline Coord3&	operator-=(const Coord3&);
    inline Coord3&	operator/=(double);
    inline Coord3&	operator*=(double);
    inline Coord&	coord()				{ return *this; }
    inline const Coord&	coord() const			{ return *this; }

    inline bool		operator==(const Coord3&) const;
    inline bool		operator!=(const Coord3&) const;
    inline bool		isDefined() const;
    inline bool		isUdf() const		{ return !isDefined(); }
    double		distTo(const Coord3&) const;
    double		sqDistTo(const Coord3&) const;

    inline double	dot( const Coord3& b ) const;
    inline Coord3	cross( const Coord3& ) const;
    double		abs() const;
    double		sqAbs() const;
    inline Coord3	normalize() const;

    const char*		getUsrStr() const;
    bool		parseUsrStr(const char*);

    double		z;

    static const Coord3& udf();

    inline double	distTo( const Geom::Point2D<double>& b ) const
			{ return Geom::Point2D<double>::distTo(b); }
    inline double	sqDistTo( const Geom::Point2D<double>& b ) const
			{ return Geom::Point2D<double>::sqDistTo(b); }
};


inline Coord3 operator*( double f, const Coord3& b )
{ return Coord3(b.x*f, b.y*f, b.z*f ); }


/*!
\brief 3D coordinate and a value.
*/

mExpClass(Basic) Coord3Value
{
public:
    		Coord3Value( double x=0, double y=0, double z=0, 
			     float v=mUdf(float) )
		    : coord(x,y,z), value(v) 	{}
		Coord3Value( const Coord3& c, float v=mUdf(float) )
		    : coord(c), value(v)		{}
    bool	operator==( const Coord3Value& cv ) const
		{ return cv.coord == coord; }
    bool	operator!=( const Coord3Value& cv ) const
		{ return cv.coord != coord; }

    Coord3	coord;
    float	value;
};


/*!
\brief Positioning in a seismic survey: inline/crossline. Most functions are
identical to RowCol.
*/

mExpClass(Basic) BinID
{
public:
    inline			BinID(int r,int c);
    				BinID(const RowCol&);
    inline			BinID(const BinID&);
    inline			BinID();

    inline bool			operator==(const BinID&) const;
    inline bool			operator!=(const BinID&) const;
    inline BinID		operator+(const BinID&) const;
    inline BinID		operator-(const BinID&) const;
    inline BinID		operator+() const;
    inline BinID		operator-() const;
    inline BinID		operator*(const BinID&) const;
    inline BinID		operator*(int) const;
    inline BinID		operator/(const BinID&) const;
    inline BinID		operator/(int) const;
    inline const BinID&		operator+=(const BinID&);
    inline const BinID&		operator-=(const BinID&);
    inline const BinID&		operator*=(const BinID&);
    inline const BinID&		operator*=(int);
    inline const BinID&		operator/=(const BinID&);
    inline int&			operator[](int idx);
    inline int			operator[](int idx) const;
    inline int			toInt32() const;
    
    inline static BinID		fromInt64(od_int64);
    inline static BinID		fromInt32(int);
    inline int			sqDistTo(const BinID&) const;

    const char*			getUsrStr(bool is2d=false) const;
    bool			parseUsrStr(const char*);
    inline od_int64		toInt64() const;
    bool                        isNeighborTo(const BinID&,const BinID&,
					     bool eightconnectivity=true) const;

    int				inl;
    int				crl;

    static const BinID&		udf();
    
    int&			trcNr()		{ return crl; }
    int				trcNr() const	{ return crl; }
    int&			lineNr()	{ return inl; }
    int				lineNr() const	{ return inl; }


    // od_int64			getSerialized() const;
    				//!<Legacy. Use toInt64 instead.
    // void			setSerialized(od_int64);
    				//!<Legacy. Use fromInt64 instead.
};



mImplInlineRowColFunctions(BinID, inl, crl);

/*!
\brief Represents a trace position, with the geometry (2D or 3D) and position in
the geometry.
*/

mExpClass(Basic) TraceID
{
public:
				typedef int GeomID;
    
				TraceID(const BinID& bid);
				TraceID(GeomID geomid,int linenr,int trcnr);

    static GeomID		std3DGeomID();
    static GeomID		cUndefGeomID();
    
    bool			is2D() const
    				{ return geomid_!=std3DGeomID(); }
    int&			trcNr()		{ return pos_.trcNr(); }
    int				trcNr() const	{ return pos_.trcNr(); }
    int&			lineNr()	{ return pos_.lineNr(); }
    int				lineNr() const	{ return pos_.lineNr(); }
    
    bool			operator ==( const TraceID& a ) const
    				{ return a.geomid_==geomid_ && a.pos_==pos_; }

    static const TraceID&	udf();
    
    bool			isUdf() const { return mIsUdf(pos_.trcNr()); }

    GeomID			getGeomID() const	{ return geomid_; }
    bool			setGeomID(GeomID gid)
    				    { geomid_ = gid; return true; }
    
    GeomID			geomid_;
				/*!<std3DGeomID refers to the default 3d survey
				    setup. Any other value refers to the
				    geometry.*/
    BinID			pos_;
};

class BinIDValues;


/*!
\brief BinID and a value.
*/

mClass(Basic) BinIDValue
{
public:
		BinIDValue( int inl=0, int crl=0, float v=mUdf(float) )
		: binid(inl,crl), value(v)	{}
		BinIDValue( const BinID& b, float v=mUdf(float) )
		: binid(b), value(v)		{}
		BinIDValue(const BinIDValues&,int);

    bool	operator==( const BinIDValue& biv ) const
		{ return biv.binid == binid
		      && mIsEqual(value,biv.value,compareEpsilon()); }
    bool	operator!=( const BinIDValue& biv ) const
		{ return !(*this == biv); }

    BinID	binid;
    float	value;

    static float compareEpsilon()		{ return 1e-5; }
};


/*!
\brief BinID and values. If one of the values is Z, make it the first one.
*/

mExpClass(Basic) BinIDValues
{
public:
			BinIDValues( int inl=0, int crl=0, int n=2 )
			: binid(inl,crl), vals(0), sz(0) { setSize(n); }
			BinIDValues( const BinID& b, int n=2 )
			: binid(b), vals(0), sz(0)	{ setSize(n); }
			BinIDValues( const BinIDValues& biv )
			: vals(0), sz(0)		{ *this = biv; }
			BinIDValues( const BinIDValue& biv )
			: binid(biv.binid), vals(0), sz(0)
					{ setSize(1); value(0) = biv.value; }
			~BinIDValues();

    BinIDValues&	operator =(const BinIDValues&);

    bool		operator==( const BinIDValues& biv ) const;
    			//!< uses BinIDValue::compareepsilon
    inline bool		operator!=( const BinIDValues& biv ) const
			{ return !(*this == biv); }

    BinID		binid;
    int			size() const			{ return sz; }
    float&		value( int idx )		{ return vals[idx]; }
    float		value( int idx ) const		{ return vals[idx]; }
    float*		values()			{ return vals; }
    const float*	values() const			{ return vals; }

    void		setSize(int,bool kpvals=false);
    void		setVals(const float*);

protected:

    float*		vals;
    int			sz;
    static float	udf;

};

namespace Values {

/*!
\brief Undefined Coord.
*/

template<>
mClass(Basic) Undef<Coord>
{
public:
    static Coord	val()			{ return Coord::udf(); }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( Coord crd )	{ return !crd.isDefined(); }
    static void		setUdf( Coord& crd )	{ crd = Coord::udf(); }
};


/*!
\brief Undefined Coord3.
*/

template<>
mClass(Basic) Undef<Coord3>
{
public:
    static Coord3	val()			{ return Coord3::udf(); }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( Coord3 crd )	{ return !crd.isDefined(); }
    static void		setUdf( Coord3& crd )	{ crd = Coord3::udf(); }
};

} // namespace Values


inline bool Coord3::operator==( const Coord3& b ) const
{
    const double dx = x-b.x; const double dy = y-b.y; const double dz = z-b.z;
    return mIsZero(dx,mDefEps) && mIsZero(dy,mDefEps) && mIsZero(dz,mDefEps);
}


inline bool Coord3::operator!=( const Coord3& b ) const
{
    return !(b==*this);
}

inline bool Coord3::isDefined() const
{
    return !Values::isUdf(z) && Geom::Point2D<double>::isDefined();
}


inline Coord3 Coord3::operator+( const Coord3& p ) const
{
    return Coord3( x+p.x, y+p.y, z+p.z );
}


inline Coord3 Coord3::operator-( const Coord3& p ) const
{
    return Coord3( x-p.x, y-p.y, z-p.z );
}


inline Coord3 Coord3::operator-() const
{
    return Coord3( -x, -y, -z );
}


inline Coord3 Coord3::operator*( double factor ) const
{ return Coord3( x*factor, y*factor, z*factor ); }


inline Coord3 Coord3::operator/( double denominator ) const
{ return Coord3( x/denominator, y/denominator, z/denominator ); }


inline Coord3 Coord3::scaleBy( const Coord3& factor ) const
{ return Coord3( x*factor.x, y*factor.y, z*factor.z ); }


inline Coord3 Coord3::unScaleBy( const Coord3& denominator ) const
{ return Coord3( x/denominator.x, y/denominator.y, z/denominator.z ); }


inline Coord3& Coord3::operator+=( const Coord3& p )
{
    x += p.x; y += p.y; z += p.z;
    return *this;
}


inline Coord3& Coord3::operator-=( const Coord3& p )
{
    x -= p.x; y -= p.y; z -= p.z;
    return *this;
}


inline Coord3& Coord3::operator*=( double factor )
{
    x *= factor; y *= factor; z *= factor;
    return *this;
}


inline Coord3& Coord3::operator/=( double denominator )
{
    x /= denominator; y /= denominator; z /= denominator;
    return *this;
}


inline double Coord3::dot(const Coord3& b) const
{ return x*b.x + y*b.y + z*b.z; }


inline Coord3 Coord3::cross(const Coord3& b) const
{ return Coord3( y*b.z-z*b.y, z*b.x-x*b.z, x*b.y-y*b.x ); }


inline Coord3 Coord3::normalize() const
{
    const double absval = abs();
    if ( absval < 1e-10 )
	return *this;

    return *this / absval;
}

#endif

