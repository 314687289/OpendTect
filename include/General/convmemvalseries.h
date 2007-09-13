#ifndef convmemvalseries_h
#define convmemvalseries_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kris Tingdahl
 Date:          Oct 2006
 RCS:           $Id: convmemvalseries.h,v 1.7 2007-09-13 19:38:39 cvsnanne Exp $
________________________________________________________________________

-*/

#include "valseries.h"
#include "datainterp.h"
#include "datachar.h"
#include "undefarray.h"


/*!ValueSeries that holds data in memory, but the memory may be of a different
format than T. I.e. a ValueSeries<float> can have it's values stored as 
chars. */


template <class T>
class ConvMemValueSeries : public ValueSeries<T>
{
public:

    inline	    	ConvMemValueSeries(od_int64 sz,
	    				   const BinDataDesc& stortype,
	    				   bool doundef=true);

    inline		~ConvMemValueSeries();
    inline bool		isOK() const;

    inline od_int64	size() const;
    inline bool		writable() const;
    inline T		value(od_int64 idx) const;
    inline void		setValue( od_int64 idx, T v );

    inline bool		reSizeable() const	{ return true; }
    inline bool		setSize(od_int64);

    inline const T*	arr() const;
    inline T*		arr();

    inline char*	storArr();
    inline const char*	storArr() const;

    inline BinDataDesc	dataDesc() const;
    bool		handlesUndef() const { return undefhandler_; }

protected:

    UndefArrayHandler*	undefhandler_;
    DataInterpreter<T>	interpreter_;
    BinDataDesc		rettype_;

    char*		ptr_;
    od_int64		size_;
};


template <class T> inline
ConvMemValueSeries<T>::ConvMemValueSeries( od_int64 sz,
					   const BinDataDesc& stortype, 
					   bool doundef)
    : ptr_( 0 )
    , size_( -1 )
    , interpreter_( DataCharacteristics(stortype) )
    , undefhandler_( doundef ? new UndefArrayHandler(stortype) : 0 )
{
    if ( undefhandler_ && !undefhandler_->isOK() )
    { delete undefhandler_; undefhandler_ = 0; }

    T dummy;
    rettype_ = BinDataDesc( dummy );
    setSize( sz );
}


template <class T> inline
ConvMemValueSeries<T>::~ConvMemValueSeries()
{
    delete [] ptr_;
    delete undefhandler_;
}


template <class T> inline
bool ConvMemValueSeries<T>::isOK() const
{
    if ( undefhandler_ && !undefhandler_->isOK() )
	return false;

    return ptr_;
}


template <class T> inline
T ConvMemValueSeries<T>::value(od_int64 idx) const
{
    if ( undefhandler_ && undefhandler_->isUdf( ptr_,idx ) )
	return mUdf(T);

    return interpreter_.get( ptr_, idx );
}


template <class T> inline
bool ConvMemValueSeries<T>::setSize( od_int64 sz )
{
    if ( sz==size_ ) return true;

    delete [] ptr_;
    ptr_ = new char[sz*interpreter_.nrBytes()];
    return ptr_;
}


template <class T> inline
bool ConvMemValueSeries<T>::writable() const
{ return true; }


template <class T> inline
od_int64 ConvMemValueSeries<T>::size() const
{ return size_; }


template <class T> inline
void ConvMemValueSeries<T>::setValue( od_int64 idx, T v )
{
    if ( undefhandler_ && mIsUdf(v) )
	undefhandler_->setUdf( ptr_, idx );
    else
    {
	interpreter_.put( ptr_, idx, v );
	if ( undefhandler_ ) undefhandler_->unSetUdf( ptr_, idx );
    }
}


template <class T> inline
const T* ConvMemValueSeries<T>::arr() const
{ return interpreter_.dataChar()==rettype_ ? (T*) ptr_ : 0; }


template <class T> inline
T* ConvMemValueSeries<T>::arr()
{ return interpreter_.dataChar()==rettype_ ? (T*) ptr_ : 0; }


template <class T> inline
char* ConvMemValueSeries<T>::storArr()
{ return ptr_; }


template <class T> inline
const char* ConvMemValueSeries<T>::storArr() const
{ return ptr_; }


template <class T> inline
BinDataDesc ConvMemValueSeries<T>::dataDesc() const
{ return interpreter_.dataChar(); }


#endif
