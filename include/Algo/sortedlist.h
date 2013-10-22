#ifndef sortedlist_h
#define sortedlist_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris
 Date:		19-4-2000
 Contents:	Array sorting
 RCS:		$Id$
________________________________________________________________________

-*/

#include "gendefs.h"
#include "vectoraccess.h"

/*!
\brief A SortedList is a list where all objects are stored in ascending order.
The objects should be capable of doing <,> and ==. If allowmultiples is true,
multiple objects with the same value are allowed in the list. 

  A SortedList can be used together with all other lists that have a []
  operator, such as TypeSets.
*/

template <class T>
mClass(Algo) SortedList
{
public:

    typedef int		size_type;
    typedef T		object_type;

    			SortedList( bool allowmultiples_ )
			    : allowmultiples(allowmultiples_) {}

    size_type 		size() const			{ return tvec.size(); }
    const T&		operator[](size_type idx) const { return (T&)tvec[idx];}
    size_type		indexOf(const T&) const;
    			/*!< Returns -1 if not found */

    SortedList<T>&	operator +=(const T&);
    SortedList<T>&	operator -=(const T&);

    // The following functions handle external indexables: classes or
    // arrays - things that support the [] operator.

    template <class U> SortedList<T>&	copy(const U&);
    template <class U> SortedList<T>&	operator =(const U&);
    template <class U> SortedList<T>&	operator +=(const U&);
    template <class U> SortedList<T>&	operator -=(const U&);
    template <class U> void		intersect(const U&);
					/*!< Remove all entries not present
					     in both lists. */

    void		erase();
    void		removeSingle(size_type);
    void		removeRange(size_type,size_type);

    std::vector<T>&	vec()		{ return tvec.vec(); }
    const std::vector<T>& vec() const	{ return tvec.vec(); }
    T*			arr()		{ return size() ? &(*this)[0] : 0; }
    const T*		arr() const	{ return size() ? &(*this)[0] : 0; }

protected:

    size_type		getPos( const T& ) const;
    			/*!< If not found, it will return position of the
			     item just above, and size() if val is higher than
			     highest val
			 */

    bool		allowmultiples;
    VectorAccess<T,size_type>	tvec;

};


template <class T> inline
typename SortedList<T>::size_type SortedList<T>::getPos( const T& typ ) const
{
    const size_type sz = size();
    if ( !sz ) return 0;

    size_type start = 0; size_type stop = sz-1;
    T startval = (*this)[start]; T stopval = (*this)[stop];
    if ( typ > stopval ) return sz;

    while ( stopval > startval )
    {
	if ( stop-start==1 )
	    return typ>startval ? stop : start;

	size_type middle = (start+stop)>>1;
	T middleval = (*this)[middle];

	if ( middleval > typ )
	{
	    stopval = middleval;
	    stop = middle;
	}
	else
	{
	    startval = middleval;
	    start = middle;
	}
    }

    return start;
}


template <class T> inline
typename SortedList<T>::size_type SortedList<T>::indexOf( const T& typ ) const
{
    if ( !size() ) return -1;

    size_type pos = getPos( typ );

    if ( pos>=size() || pos<0 || (*this)[pos]!=typ )
        return -1;

    return pos;
}


template <class T> inline
SortedList<T>&	SortedList<T>::operator +=( const T& nv )
{
    size_type newpos = getPos( nv );

    if ( newpos == size() )
    {
	tvec.push_back( nv );
	return *this;
    }

    if ( !allowmultiples && (*this)[newpos] == nv )
	return *this;

    tvec.insert( newpos, nv );
    return *this;
}


template <class T> inline
SortedList<T>&	SortedList<T>::operator -=( const T& nv )
{
    const size_type sz = size();
    if ( !sz ) return *this;

    const size_type pos = indexOf( nv );

    if ( pos == -1 ) return *this;

    tvec.removeSingle( pos );
    return *this;
}


template <class T> template <class U> inline
void  SortedList<T>::intersect( const U& b )
{
    for ( size_type idx=0; idx<size(); idx++ )
    {
	if ( b.indexOf( (*this)[idx]) == -1 )
	{
	    removeSingle( idx );
	    idx--;
	}
    }
}


template <class T> template <class U> inline
SortedList<T>& SortedList<T>::copy( const U& array )
{
    erase();
    const size_type sz = array.size();
    for ( size_type idx=0; idx<sz; idx++ )
	(*this) += array[idx];
    return *this;
}


template <class T> template <class U> inline
SortedList<T>&	SortedList<T>::operator =( const U& array )
{ return copy(array); }


template <class T> template <class U> inline
SortedList<T>&  SortedList<T>::operator +=( const U& array )
{
    const size_type sz = array.size();
    for ( size_type idx=0; idx<sz; idx++ )
	(*this) += array[idx];
    return *this;
}


template <class T> template <class U> inline
SortedList<T>&  SortedList<T>::operator -=( const U& array )
{
    const size_type sz = array.size();
    for ( size_type idx=0; idx<sz; idx++ )
	(*this) -= array[idx];
    return *this;
}


template <class T> inline
void SortedList<T>::erase() { tvec.erase(); }


template <class T> inline
void SortedList<T>::removeSingle( size_type pos )
{
    tvec.remove( pos );
}

template <class T> inline
void SortedList<T>::removeRange( size_type p1, size_type p2 )
{
    tvec.remove( p1, p2 );
}


#endif
