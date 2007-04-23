#ifndef arrayndinfo_h
#define arrayndinfo_h
/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id: arrayndinfo.h,v 1.9 2007-04-23 10:48:48 cvshelene Exp $
________________________________________________________________________


*/

#include <gendefs.h>

/*!
Contains the information about the size of ArrayND, and
in what order the data is stored (if accessable via a pointer).
*/

class ArrayNDInfo
{
public:

    virtual ArrayNDInfo* clone() const					= 0;
    virtual		~ArrayNDInfo()					{} 

    virtual int		getNDim() const					= 0;
    virtual int		getSize(int dim) const				= 0;
    virtual bool	setSize(int dim,int sz);
 
    virtual uint64	getTotalSz() const;
    virtual uint64	getOffset(const int*) const;
    			/*!<Returns offset in a 'flat' array.*/
    virtual bool	validPos(const int*) const;
    			/*!<Checks if the position exists. */
    virtual void	getArrayPos(uint64, int*) const;
    			/*!<Given an offset, what is the ND position. */

protected:

    uint64		calcTotalSz() const;
};


inline bool operator ==( const ArrayNDInfo& a1, const ArrayNDInfo& a2 )
{
    int nd = a1.getNDim();
    if ( nd != a2.getNDim() ) return false;
    for ( int idx=0; idx<nd; idx++ )
	if ( a1.getSize(idx) != a2.getSize(idx) ) return false;
    return true;
}

inline bool operator !=( const ArrayNDInfo& a1, const ArrayNDInfo& a2 )
{ return !(a1 == a2); }


class Array1DInfo : public ArrayNDInfo
{
public:

    int			getNDim() const				{ return 1; }

    virtual uint64	getOffset(int) const;
    			/*!<Returns offset in a 'flat' array.*/
    virtual bool	validPos(int pos) const;

};


class Array2DInfo : public ArrayNDInfo
{
public:

    int				getNDim() const			{ return 2; }

    virtual uint64		getOffset(int,int) const;
				/*!<Returns offset in a 'flat' array.*/
    virtual bool		validPos(int,int) const;

};


class Array3DInfo : public ArrayNDInfo
{
public:

    int				getNDim() const			{ return 3; }

    virtual uint64		getOffset(int, int, int) const;
				/*!<Returns offset in a 'flat' array.*/
    virtual bool		validPos(int,int,int) const;

};


class Array1DInfoImpl : public Array1DInfo
{
public:
    Array1DInfo*	clone() const
			{ return new Array1DInfoImpl(*this); }

			Array1DInfoImpl(int nsz=0); 
			Array1DInfoImpl(const Array1DInfo&);

    inline int		getSize(int dim) const; 
    bool		setSize(int dim,int nsz);
    uint64		getTotalSz() const { return sz_; }

protected:

    int			sz_;
};


class Array2DInfoImpl : public Array2DInfo
{
public:

    Array2DInfo*	clone() const { return new Array2DInfoImpl(*this); }

			Array2DInfoImpl(int sz0=0, int sz1=0);
			Array2DInfoImpl(const Array2DInfo&);

    inline int		getSize(int dim) const;
    bool		setSize(int dim,int nsz);

    uint64		getTotalSz() const { return cachedtotalsz_; }
    
    virtual inline uint64	getOffset(int,int) const;
				/*!<Returns offset in a 'flat' array.*/

protected:

    int                 sz0_;
    int                 sz1_;
    uint64		cachedtotalsz_;
};


class Array3DInfoImpl : public Array3DInfo
{
public:

    Array3DInfo*	clone() const { return new Array3DInfoImpl(*this); }

			Array3DInfoImpl(int sz0=0, int sz1=0, int sz2=0);
			Array3DInfoImpl(const Array3DInfo&);

    inline int		getSize(int dim) const; 
    bool                setSize(int dim,int nsz);
    uint64		getTotalSz() const { return cachedtotalsz_; }

    virtual inline uint64	getOffset(int, int, int) const;
				/*!<Returns offset in a 'flat' array.*/

protected:

    int                 sz0_;
    int                 sz1_;
    int                 sz2_;
    uint64		cachedtotalsz_;
};  


class ArrayNDInfoImpl : public ArrayNDInfo
{
public:

    ArrayNDInfo*	clone() const;
    static ArrayNDInfo*	create( int ndim );

			ArrayNDInfoImpl(int ndim);
			ArrayNDInfoImpl(const ArrayNDInfo&);
			ArrayNDInfoImpl(const ArrayNDInfoImpl&);

			~ArrayNDInfoImpl();

    uint64		getTotalSz() const { return cachedtotalsz_; }
    int                 getNDim() const;
    int                 getSize(int dim) const;
    bool                setSize(int dim,int nsz);

protected:

    int*		sizes;
    int 		ndim;

    uint64		cachedtotalsz_;
};


/*!\brief ArrayNDIter is an object that is able to iterate through all samples
   in a ArrayND.
   \par
   ArrayNDIter will stand on the first position when initiated, and move to
   the second at the fist call to next(). next() will return false when
   no more positions are avaliable
*/

class ArrayNDIter
{
public:
				ArrayNDIter( const ArrayNDInfo& );
				~ArrayNDIter();

    bool			next();
    void			reset();

    template <class T> void inline	setPos( const T& idxabl );
    const int*			getPos() const { return position_; }
    int				operator[](int) const;

protected:
    bool			inc(int);

    int*			position_;
    const ArrayNDInfo&		sz_;
};


inline int Array1DInfoImpl::getSize( int dim ) const
{
    return dim ? 0 : sz_;
}


inline int Array2DInfoImpl::getSize( int dim ) const
{
    return dim>1 || dim<0 ? 0 : (&sz0_)[dim];
}


inline uint64 Array2DInfoImpl::getOffset( int p0, int p1 ) const
{
    return (uint64) p0 * sz1_ + p1;
}


inline int Array3DInfoImpl::getSize( int dim ) const
{
    return dim>2 || dim<0 ? 0 : (&sz0_)[dim];
}


inline uint64 Array3DInfoImpl::getOffset( int p0, int p1, int p2 ) const
{
    return (uint64) p0 * sz2_ * sz1_ + p1 * sz2_ + p2;
}


template <class T> inline void ArrayNDIter::setPos( const T& idxable )
{
    for ( int idx=sz_.getNDim()-1; idx>=0; idx-- )
	position_[idx] = idxable[idx];
}

#endif

