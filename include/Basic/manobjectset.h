#ifndef manobjectset_h
#define manobjectset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Feb 2009
 RCS:		$Id: manobjectset.h,v 1.1 2009-02-13 13:31:14 cvsbert Exp $
________________________________________________________________________

-*/

#include "objectset.h"


/*!\brief ObjectSet where the objects contained are owned by this set. */

template <class T>
class ManagedObjectSet : public ObjectSet<T>
{
public:

    inline 			ManagedObjectSet(bool objs_are_arrs);
    inline			ManagedObjectSet(const ManagedObjectSet<T>&);
    inline virtual		~ManagedObjectSet();
    inline ManagedObjectSet<T>&	operator =(const ObjectSet<T>&);
    inline ManagedObjectSet<T>&	operator =(const ManagedObjectSet<T>&);
    virtual bool		isManaged() const	{ return true; }

    inline virtual ManagedObjectSet<T>& operator -=( T* ptr );
    inline virtual void		append(const ObjectSet<T>&);

    inline virtual void		erase();
    inline virtual void		remove(int,int);
    inline virtual T*		remove( int idx, bool kporder=true )
				{ return ObjectSet<T>::remove(idx,kporder); }

protected:

    bool	isarr_;

};


//ObjectSet implementation
template <class T> inline
ManagedObjectSet<T>::ManagedObjectSet( bool isarr ) : isarr_(isarr)
{}


template <class T> inline
ManagedObjectSet<T>::ManagedObjectSet( const ManagedObjectSet<T>& t )
{ *this = t; }


template <class T> inline
ManagedObjectSet<T>::~ManagedObjectSet()
{ erase(); }


template <class T> inline
ManagedObjectSet<T>& ManagedObjectSet<T>::operator =( const ObjectSet<T>& os )
{
    if ( &os != this ) deepCopy( *this, os );
    return *this;
}

template <class T> inline
ManagedObjectSet<T>& ManagedObjectSet<T>::operator =(
					const ManagedObjectSet<T>& os )
{
    if ( &os != this ) { isarr_ = os.isarr_; deepCopy( *this, os ); }
    return *this;
}


template <class T> inline
ManagedObjectSet<T>& ManagedObjectSet<T>::operator -=( T* ptr )
{
    if ( !ptr ) return *this;
    this->vec_.erase( (void*)ptr );
    if ( isarr_ )	delete [] ptr;
    else		delete ptr;
    return *this;
}


template <class T> inline
void ManagedObjectSet<T>::append( const ObjectSet<T>& os )
{
    deepAppend( *this, os );
}


template <class T> inline
void ManagedObjectSet<T>::erase()
{
    if ( isarr_ )	deepEraseArr( *this );
    else		deepErase( *this );
}


template <class T> inline
void ManagedObjectSet<T>::remove( int i1, int i2 )
{
    for ( int idx=i1; idx<=i2; idx++ )
    {
	if ( isarr_ )
	    delete [] (*this)[idx];
	else
	    delete (*this)[idx];
    }
    ObjectSet<T>::remove( i1, i2 );
}


#endif
