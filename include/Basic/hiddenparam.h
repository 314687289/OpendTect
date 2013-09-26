#ifndef hiddenparam_h
#define hiddenparam_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2011
 RCS:		$Id$
________________________________________________________________________

-*/

#include "sets.h"
#include "threadlock.h"


/*!
\brief Workaround manager when you cannot add class members to a class due to
binary compability issues.

  If you want to add the variable of type V to class O, do the following
  in the source-file:
  
  \code
  HiddenParam<O,V>	myparammanager( undef_val );
  \endcode
  
  You can then access the variable in the source-file by:
  
  \code
  myparammanager.setParam( this, new_value );
  \endcode
  
  and retrieve it by 
  
  \code
  V value = myparammanager.getParam( this );
  \endcode
  
  \note V cannot be boolean. Use char instead. V must be 'simple' enough to be
  stored in a type-set. Also note you may not be able to call the
  removeParam (if the class does not already have a destructor), so
  so don't use with objects that are created millions of times in those
  cases, as you will leak memory.
  Finally, note that you MUST set the parameter in the constructor. The
  undef value is not ment to be returned, it's merely to keep the compiler
  happy.
*/

template <class O, class V>
mClass(Basic) HiddenParam 
{
public:
    		HiddenParam( const V& undefval )
		    : undef_( undefval )		{}
    void	setParam( O* obj, const V& val );
    const V&	getParam( const O* obj ) const;

    bool	hasParam( const O* ) const;

    void	removeParam( O* );

protected:

    ObjectSet<O>		objects_;
    TypeSet<V>			params_;
    mutable Threads::Lock	lock_;
    V				undef_;

};


template <class O, class V>
void HiddenParam<O,V>::setParam( O* obj, const V& val )
{
    Threads::Locker locker( lock_ );
    const int idx = objects_.indexOf( obj );
    if ( idx==-1 )
    {
	objects_ += obj;
	params_ += val;
	return;
    }

    params_[idx] = val;
}


template <class O, class V>
const V& HiddenParam<O,V>::getParam( const O* obj ) const
{
    Threads::Locker locker( lock_ );
    const int idx = objects_.indexOf( obj );
    if ( !objects_.validIdx(idx) )
    {
	pErrMsg("Object not found");
	return undef_;
    }

    return params_[idx];
}


template <class O, class V>
bool HiddenParam<O,V>::hasParam( const O* obj ) const
{
    Threads::Locker locker( lock_ );
    return objects_.isPresent( obj );
}



template <class O, class V>
void HiddenParam<O,V>::removeParam( O* obj )
{
    Threads::Locker locker( lock_ );
    const int idx = objects_.indexOf( obj );
    if ( idx==-1 )
	return;

    params_.removeSingle( idx );
    objects_.removeSingle( idx );
}


#endif
