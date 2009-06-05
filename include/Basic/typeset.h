#ifndef typeset_h
#define typeset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert / many others
 Date:		Apr 1995 / Feb 2009
 RCS:		$Id: typeset.h,v 1.5 2009-06-05 19:03:53 cvskris Exp $
________________________________________________________________________

-*/

#ifndef odset_h
#include "odset.h"
#endif
#ifndef vectoraccess_h
#include "vectoraccess.h"
#endif

/*!\brief Set of (small) copyable elements

The TypeSet is meant for simple types or small objects that have a copy
constructor. The `-=' function will only remove the first occurrence that
matches using the `==' operator. The requirement of the presence of that
operator is actually not that bad: at least you can't forget it.

Do not make TypeSet<bool> (don't worry, it won't compile). Use the BoolTypeSet
typedef just after the class definition. See vectoraccess.h for details on why.

*/

template <class T>
class TypeSet : public OD::Set
{
public:

    inline			TypeSet();
    inline			TypeSet(int nr,T typ);
    inline			TypeSet(const TypeSet<T>&);
    inline virtual		~TypeSet();
    inline TypeSet<T>&		operator =(const TypeSet<T>&);

    inline int			size() const	{ return vec_.size(); }
    inline virtual int		nrItems() const	{ return size(); }
    inline virtual bool		setSize(int,T val=T());
				/*!<\param val value assigned to new items */
    inline virtual bool		setCapacity( int sz );
				/*!<Allocates mem only, no size() change */

    inline T&			operator[](int);
    inline const T&		operator[](int) const;
    inline T&			first();
    inline const T&		first() const;
    inline T&			last();
    inline const T&		last() const;
    inline virtual bool		validIdx(int) const;
    inline virtual int		indexOf(T,bool forward=true,int start=-1) const;
    inline bool			isPresent( const T& t ) const
    						{ return indexOf(t) >= 0; }

    inline TypeSet<T>&		operator +=(const T&);
    inline TypeSet<T>&		operator -=(const T&);
    inline virtual TypeSet<T>&	copy(const TypeSet<T>&);
    inline virtual bool		append(const TypeSet<T>&);
    inline bool			add(const T&);

    inline virtual void		swap(int,int);
    virtual inline void		createUnion(const TypeSet<T>&);
				/*!< Adds items not already there */
    virtual inline void		createIntersection(const TypeSet<T>&);
				//!< Only keeps common items
    virtual inline void		createDifference(const TypeSet<T>&,
	    				 bool must_preserve_order=false);
				//!< Removes all items present in other set.

    inline virtual bool		addIfNew(const T&);
    virtual void		fillWith(const T&);

    inline virtual void		erase();
    inline virtual void		remove(int,bool preserve_order=true);
    inline virtual void		remove(int from,int to);
    inline virtual void		insert(int,const T&);

				//! 3rd party access
    inline virtual T*		arr()		{ return gtArr(); }
    inline virtual const T*	arr() const	{ return gtArr(); }
    inline std::vector<T>&	vec();
    inline const std::vector<T>& vec() const;

protected:

    VectorAccess<T>		vec_;

    inline virtual T*		gtArr() const;

};

//! We need this because STL has a crazy specialisation of the vector<bool>
typedef char BoolTypeSetType;
typedef TypeSet<BoolTypeSetType> BoolTypeSet;
//!< This sux, BTW.



template <class T>
inline bool operator ==( const TypeSet<T>& a, const TypeSet<T>& b )
{
    if ( a.size() != b.size() ) return false;

    const int sz = a.size();
    for ( int idx=0; idx<sz; idx++ )
	if ( !(a[idx] == b[idx]) ) return false;

    return true;
}

template <class T>
inline bool operator !=( const TypeSet<T>& a, const TypeSet<T>& b )
{ return !(a == b); }


//! append allowing a different type to be merged into set
template <class T,class S>
inline bool append( TypeSet<T>& to, const TypeSet<S>& from )
{
    const int sz = from.size();
    if ( !to.setCapacity( sz + to.size() ) ) return false;
    for ( int idx=0; idx<sz; idx++ )
	to += from[idx];

    return true;
}


//! copy from different possibly different type into set
//! Note that there is no optimisation for equal size, as in member function.
template <class T,class S>
inline void copy( TypeSet<T>& to, const TypeSet<S>& from )
{
    if ( &to == &from ) return;
    to.erase();
    append( to, from );
}


//! Sort TypeSet. Must have operator > defined for elements
template <class T>
inline void sort( TypeSet<T>& ts )
{
    T tmp; const int sz = ts.size();
    for ( int d=sz/2; d>0; d=d/2 )
	for ( int i=d; i<sz; i++ )
	    for ( int j=i-d; j>=0 && ts[j]>ts[j+d]; j-=d )
		{ tmp = ts[j]; ts[j] = ts[j+d]; ts[j+d] = tmp; }
}


// Member function implementations
template <class T> inline
TypeSet<T>::TypeSet()
{}


template <class T> inline
TypeSet<T>::TypeSet( int nr, T typ )
{ setSize( nr, typ ); }


template <class T> inline
TypeSet<T>::TypeSet( const TypeSet<T>& t )
{ append( t ); }


template <class T> inline
TypeSet<T>::~TypeSet() {}


template <class T> inline
TypeSet<T>& TypeSet<T>::operator =( const TypeSet<T>& ts )
{ return copy( ts ); }


template <class T> inline
bool TypeSet<T>::setSize( int sz, T val )
{ return vec_.setSize( sz, val ); }


template <class T> inline
bool TypeSet<T>::setCapacity( int sz )
{ return vec_.setCapacity( sz ); }


template <class T> inline
void TypeSet<T>::swap( int idx0, int idx1 )
{
    if ( !validIdx(idx0) || !validIdx(idx1) )
	return;

    T tmp = vec_[idx0];
    vec_[idx0] = vec_[idx1];
    vec_[idx1] = tmp;
}


template <class T> inline
bool TypeSet<T>::validIdx( int idx ) const
{ return idx>=0 && idx<size(); }


template <class T> inline
T& TypeSet<T>::operator[]( int idx )
{ return vec_[idx]; }


template <class T> inline
const T& TypeSet<T>::operator[]( int idx ) const
{ return vec_[idx]; }


template <class T> inline
T& TypeSet<T>::first()			{ return vec_[0]; }

template <class T> inline
const T& TypeSet<T>::first() const	{ return vec_[0]; }

template <class T> inline
T& TypeSet<T>::last()			{ return vec_[size()-1]; }

template <class T> inline
const T& TypeSet<T>::last() const	{ return vec_[size()-1]; }


template <class T> inline
int TypeSet<T>::indexOf( T typ, bool forward, int start ) const
{
    const T* ptr = arr();
    if ( forward )
    {
	const unsigned int sz = size();
	if ( start<0 || start>=sz ) start = 0;
	for ( unsigned int idx=start; idx<sz; idx++ )
	    if ( ptr[idx] == typ ) return idx;
    }
    else
    {
	const unsigned int sz = size();
	if ( start<0 || start>=sz ) start = sz-1;
	for ( int idx=start; idx>=0; idx-- )
	    if ( ptr[idx] == typ ) return idx;
    }

    return -1;
}


template <class T> inline
TypeSet<T>& TypeSet<T>::operator +=( const T& typ )
{ vec_.push_back( typ ); return *this; }


template <class T> inline
TypeSet<T>& TypeSet<T>::operator -=( const T& typ )
{ vec_.erase( typ ); return *this; }


template <class T> inline
TypeSet<T>& TypeSet<T>::copy( const TypeSet<T>& ts )
{
    if ( &ts != this )
    {
	const unsigned int sz = size();
	if ( sz != ts.size() )
	    { erase(); append(ts); }
	else
	{
	    for ( unsigned int idx=0; idx<sz; idx++ )
		(*this)[idx] = ts[idx];
	}
    }

    return *this;
}


template <class T> inline
bool TypeSet<T>::add( const T& t )
{
    return vec_.push_back( t );
}


template <class T> inline
bool TypeSet<T>::append( const TypeSet<T>& ts )
{
    const unsigned int sz = ts.size();
    if ( !sz ) return true;

    if ( !setCapacity( sz+size() ) )
	return false;

    for ( unsigned int idx=0; idx<sz; idx++ )
	*this += ts[idx];

    return true;
}


template <class T>
inline void TypeSet<T>::createUnion( const TypeSet<T>& ts )
{
    const unsigned int sz = ts.size();
    const T* ptr = ts.arr();
    for ( unsigned int idx=0; idx<sz; idx++, ptr++ )
	addIfNew( *ptr );
}


template <class T>
inline void TypeSet<T>::createIntersection( const TypeSet<T>& ts )
{
    for ( unsigned int idx=0; idx<size(); idx++ )
    {
	if ( ts.indexOf((*this)[idx]) != -1 )
	    continue;
	remove( idx--, false );
    }
}


template <class T>
inline void TypeSet<T>::createDifference( const TypeSet<T>& ts, bool kporder )
{
    const unsigned int sz = ts.size();
    for ( unsigned int idx=0; idx<sz; idx++ )
    {
	const T typ = ts[idx];
	for ( int idy=0; idy<size(); idy++ )
	{
	    if ( vec_[idy] == typ )
		remove( idy--, kporder );
	}
    }
}


template <class T> inline
bool TypeSet<T>::addIfNew( const T& typ )
{
    if ( indexOf(typ) < 0 ) { *this += typ; return true; }
    return false;
}


template <class T> inline
void TypeSet<T>::fillWith( const T& t )
{ vec_.fillWith(t); }


template <class T> inline
void TypeSet<T>::erase()
{ vec_.erase(); }


template <class T>
inline void TypeSet<T>::remove( int idx, bool kporder )
{
    if ( kporder )
	vec_.remove( idx );
    else
    {
	const int lastidx = size()-1;
	if ( idx != lastidx )
	    vec_[idx] = vec_[lastidx];
	vec_.remove( lastidx );
    }
}


template <class T> inline
void TypeSet<T>::remove( int i1, int i2 )
{ vec_.remove( i1, i2 ); }


template <class T> inline
void TypeSet<T>::insert( int idx, const T& typ )
{ vec_.insert( idx, typ );}


template <class T> inline
std::vector<T>&	 TypeSet<T>::vec()
{ return vec_.vec(); }


template <class T> inline
const std::vector<T>& TypeSet<T>::vec() const
{ return vec_.vec(); }


template <class T> inline
T* TypeSet<T>::gtArr() const
{ return size() ? const_cast<T*>(&(*this)[0]) : 0; }


#endif
