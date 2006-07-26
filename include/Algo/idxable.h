#ifndef idxable_h
#define idxable_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert & Kris
 Date:		Mar 2006
 RCS:		$Id: idxable.h,v 1.6 2006-07-26 07:21:51 cvsbert Exp $
________________________________________________________________________

*/

#include "gendefs.h"
#include "interpol1d.h"
#include "sets.h"
#include "sorting.h"

/*!\brief Position-sorted indexable objects

 These are objects that return a value of type T when the [] operator is
 applied. Can range from simple arrays and TypeSets to whatever supports
 the [] operator. The goal is to interpolate between the values. Therefore,
 the position of the values must be known from either the fact that the
 values are regular samples or by specifying another indexable object that
 provides the positions (in float or double).
*/

namespace IdxAble
{

/*!>
  Find value in indexable
*/

template <class T1,class T2,class T3>
inline T3 indexOf( const T1& arr, T3 sz, const T2& val, T3 notfoundval )
{
    for ( T3 idx=0; idx<sz; idx++ )
    {
	if ( arr[idx] == val )
	    return idx;
    }
    return notfoundval;
}

/*!>
  Find value in indexable filled with pointers.
*/

template <class T1,class T2,class T3>
inline T3 derefIndexOf( const T1& arr, T3 sz, const T2& val, T3 notfoundval )
{
    for ( T3 idx=0; idx<sz; idx++ )
    {
	if ( *arr[idx] == val )
	    return idx;
    }
    return notfoundval;
}


/*!>
  Find value in sorted array of positions.
  Equality is tested by == operator -> not for float/double!
  The 'arr' indexable object must return the positions.
  The return par 'idx' may be -1, which means that 'pos' is before the first
  position.
  Return value tells whether there is an exact match. If false, index of
  array member below pos is returned.
*/

template <class T1,class T2,class T3>
bool findPos( const T1& posarr, T3 sz, T2 pos, T3 beforefirst, T3& idx )
{
    idx = beforefirst;
    if ( sz < 1 || pos < posarr[0] )
	return false;

    if ( pos == posarr[0] )
	{ idx = 0; return true; }
    else if ( pos >= posarr[sz-1] )
    	{ idx = sz-1; return pos == posarr[sz-1]; }

    T3 idx1 = 0;
    T3 idx2 = sz-1;
    while ( idx2 - idx1 > 1 )
    {
	idx = (idx2 + idx1) / 2;
	T2 arrval = posarr[idx];
	if ( arrval == pos )		return true;
	else if ( arrval > pos )	idx2 = idx;
	else				idx1 = idx;
    }

    idx = idx1;
    return posarr[idx] == pos;
}


/*!>
  Find value in sorted array of floating point positions.
  Equality is tested by mIsZero.
  The 'arr' indexable object must return the positions.
  The return par 'idx' may be beforefirst, which means that 'pos' is before
  the first position.
  Return value tells whether there is an exact match. If false, index of
  array member below pos is returned.
*/

template <class T1,class T2,class T3>
bool findFPPos( const T1& posarr, T3 sz, T2 pos, T3 beforefirst, T3& idx,
		T2 eps=mDefEps )
{
    idx = beforefirst;
    if ( !sz ) return false;
    if ( sz < 2 || pos <= posarr[0] )
    {
	if ( mIsEqual(pos,posarr[0],eps) )
	    { idx = 0; return true; }
	else
	    return false;
    }
    else if ( pos >= posarr[sz-1] )
    	{ idx = sz-1; return mIsEqual(pos,posarr[sz-1],eps); }

    T3 idx1 = 0;
    T3 idx2 = sz-1;

    while ( idx2 - idx1 > 1 )
    {
	idx = (idx2 + idx1) / 2;
	T2 diff = posarr[idx] - pos;
	if ( mIsZero(diff,eps) )		return true;
	else if ( diff > 0  )		idx2 = idx;
	else				idx1 = idx;
    }

    idx = idx1;
    return mIsEqual(pos,posarr[idx],eps);
}

/*!>
Find index of nearest point below a given position.
The 'x' must return the positions.
The return value may be -1, which means that 'pos' is before the first
position.
*/

template <class X>
inline int getLowIdx( const X& x, int sz, double pos )
{
    int idx; findFPPos( x, sz, pos, -1, idx ); return idx;
}


/*!>
 Irregular interpolation.
 The 'x' must return the X-positions of the 'y' values.
*/

template <class X,class Y,class RT>
inline void interpolatePositioned( const X& x, const Y& y, int sz,
				   float desiredx,
				   RT& ret, bool extrapolate=false )
{
    if ( sz < 1 )
	ret = mUdf(RT);
    else if ( sz == 1 )
	ret = extrapolate ? y[0] : mUdf(RT);

    else if ( sz == 2 )
	ret = Interpolate::linear1D( x[0], y[0], x[1], y[1], desiredx );
    else if ( desiredx < x[0] || desiredx > x[sz-1] )
    {
	if ( !extrapolate )
	    ret = mUdf(RT);
	else
	    ret = desiredx < x[0]
		? Interpolate::linear1D( x[0], y[0], x[1], y[1], desiredx )
		: Interpolate::linear1D( x[sz-2], y[sz-2], x[sz-1], y[sz-1],
					 desiredx );
	return;
    }

    int prevpos = getLowIdx( x, sz, desiredx );
    int nextpos = prevpos + 1;

    if ( sz == 3 )
	ret = Interpolate::linear1D( x[prevpos], y[prevpos],
				     x[nextpos], y[nextpos], desiredx );
    else
    {
	if ( prevpos == 0 )
	    { prevpos++; nextpos++; } 
	else if ( nextpos == sz-1 )
	    { prevpos--; nextpos--; } 

	ret = Interpolate::poly1D( x[prevpos-1], y[prevpos-1],
				   x[prevpos], y[prevpos],
				   x[nextpos], y[nextpos],
				   x[nextpos+1], y[nextpos+1],
				   desiredx );
    }
}


template <class X,class Y>
inline float interpolatePositioned( const X& x, const Y& y, int sz, float pos, 
				    bool extrapolate=false )
{
    float ret = mUdf(float);
    interpolatePositioned( x, y, sz, pos, ret, extrapolate );
    return ret;
}


template <class T>
inline int getInterpolateIdxs( const T& idxabl, int sz, float pos, bool extrap,
			       float snapdist, int p[4] )
{
    if ( sz < 1
      || (!extrap && (pos < -snapdist || pos > sz - 1 + snapdist)) )
	return -1;

    const int intpos = mNINT( pos );
    const float dist = pos - intpos;
    if( dist > -snapdist && dist < snapdist && intpos > -1 && intpos < sz ) 
	{ p[0] = intpos; return 0; }

    p[1] = dist > 0 ? intpos : intpos - 1;
    if ( p[1] < 0 ) p[1] = 0;
    if ( p[1] >= sz ) p[1] = sz - 1;
    p[0] = p[1] < 1 ? p[1] : p[1] - 1;
    p[2] = p[1] < sz-1 ? p[1] + 1 : p[1];
    p[3] = p[2] < sz-1 ? p[2] + 1 : p[2];
    return 1;
}


template <class T,class RT>
inline bool interpolateReg( const T& idxabl, int sz, float pos, RT& ret,
			    bool extrapolate=false, float snapdist=mDefEps )
{
    int p[4];
    int res = getInterpolateIdxs( idxabl, sz, pos, extrapolate, snapdist, p );
    if ( res < 0 )
	{ ret = mUdf(RT); return false; }
    else if ( res == 0 )
	{ ret = idxabl[p[0]]; return true; }

    const float relpos = pos - p[1];
    ret = Interpolate::polyReg1D( idxabl[p[0]], idxabl[p[1]], idxabl[p[2]],
				  idxabl[p[3]], relpos );
    return true;
}


template <class T,class RT>
inline bool interpolateRegWithUdf( const T& idxabl, int sz, float pos, RT& ret,
			    bool extrapolate=false, float snapdist=mDefEps )
{
    int p[4];
    int res = getInterpolateIdxs( idxabl, sz, pos, extrapolate, snapdist, p );
    if ( res < 0 )
	{ ret = mUdf(RT); return false; }
    else if ( res == 0 )
	{ ret = idxabl[p[0]]; return true; }

    const float relpos = pos - p[1];
    ret = Interpolate::polyReg1DWithUdf( idxabl[p[0]], idxabl[p[1]],
	    				 idxabl[p[2]], idxabl[p[3]], relpos );
    return true;
}


template <class T>
inline float interpolateReg( const T& idxabl, int sz, float pos,
			     bool extrapolate=false, float snapdist=mDefEps )
{
    float ret = mUdf(float);
    interpolateReg( idxabl, sz, pos, ret, extrapolate, snapdist );
    return ret;
}


template <class T>
inline float interpolateRegWithUdf( const T& idxabl, int sz, float pos,
				    bool extrapolate=false,
				    float snapdist=mDefEps )
{
    float ret = mUdf(float);
    interpolateRegWithUdf( idxabl, sz, pos, ret, extrapolate, snapdist );
    return ret;
}


/*\brief finds the bendpoints in (X,Y) series

  Can be used to de-interpolate.
 
 */

template <class T1, class T2, class RT>
class BendPointFinder
{
public:

BendPointFinder( const T1& x, const T2& y, int sz, RT eps,
		 TypeSet<int>& bndidxs )
    	: x_(x)
    	, y_(y)
    	, sz_(sz)
    	, epssq_(eps*eps)
    	, bendidxs_(bndidxs)
{
    bendidxs_.erase();
}

void findAll()
{
    findInPart( 0, sz_-1 );
}

void findInPart( int idx0, int idx1 )
{
    bendidxs_ += idx0; bendidxs_ += idx1;
    findInSegment( idx0, idx1 );
    sort( bendidxs_ );
}

protected:


void findInSegment( int idx0, int idx1 )
{
    if ( idx1 < idx0 + 2 )
	return; // First stop criterion

    const RT x0( x_[idx0] ); const RT y0( y_[idx0] );
    const RT x1( x_[idx1] ); const RT y1( y_[idx1] );
    const RT dx( x1 - x0 ); const RT dy( y1 - y0 );
    RT dsqmax = 0; int idx;
    const RT dxsq = dx * dx; RT dysq = dy * dy;
    if ( dxsq < epssq_ )
	dsqmax = getMaxDxsqOnly( idx0, idx1, idx );
    else
    {
	for ( int ipt=idx0+1; ipt<idx1; ipt++ )
	{
	    RT px = x_[ipt] - x0; RT py = y_[ipt] - y0;
	    RT u = (dx * px + dy * py) / (dxsq + dysq);
	    RT plinex = u * dx; RT pliney = u * dy;
	    RT dsq = (plinex-px) * (plinex-px) + (pliney-py) * (pliney-py);
	    if ( dsq > dsqmax )
		{ dsqmax = dsq; idx = ipt; }
	}
    }

    if ( dsqmax < epssq_ )
	return; // Second stop criterion

    bendidxs_ += idx;
    findInSegment( idx0, idx );
    findInSegment( idx, idx1 );
}


RT getMaxDxsqOnly( int idx0, int idx1, int& idx )
{
    const RT x = (x_[idx0] + x_[idx1]) / 2;
    RT dsqmax = 0; idx = idx0;
    for ( int ipt=idx0+1; ipt<idx1; ipt++ )
    {
	const RT dx = x_[ipt] - x;
	const RT dxsq = dx * dx;
	if ( dxsq > dsqmax )
	    { dsqmax = dxsq; idx = ipt; }
    }
    return dsqmax;
}

    const int		sz_;
    const RT		epssq_;
    const T1&		x_;
    const T2&		y_;
    TypeSet<int>&	bendidxs_;

};

template <class T1, class T2, class RT>
void getBendPoints( const T1& x, const T2& y, int sz, RT eps,
		    TypeSet<int>& bidxs )
{
    BendPointFinder<T1,T2,RT> bpf( x, y, sz, eps, bidxs );
    bpf.findAll();
}


} // namespace IdxAble

#endif
