#ifndef datachar_H
#define datachar_H

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Nov 2000
 Contents:	Binary data interpretation
 RCS:		$Id: datachar.h,v 1.9 2003-12-12 14:47:48 bert Exp $
________________________________________________________________________

*/


#include <bindatadesc.h>
#include <enums.h>


/*!\brief byte-level data characteristics of stored data.

Used for the interpretation (read or write) of data in buffers that are read
directly from disk into buffer. In that case cross-platform issues arise:
byte-ordering and int/float layout.
The Ibm Format is only supported for the types that are used in SEG-Y sample
data handling. SGI is a future option.

*/

#define mDeclConstr(T,ii,is) \
DataCharacteristics( const T* ) \
: BinDataDesc(ii,is,sizeof(T)), fmt(Ieee), littleendian(__islittle__) {} \
DataCharacteristics( const T& ) \
: BinDataDesc(ii,is,sizeof(T)), fmt(Ieee), littleendian(__islittle__) {}


class DataCharacteristics : public BinDataDesc
{
public:

    enum Format		{ Ieee, Ibm };

    Format		fmt;
    bool		littleendian;

			DataCharacteristics( bool ii=false, bool is=true,
					     ByteCount n=N4, Format f=Ieee,
					     bool l=__islittle__ )
			: BinDataDesc(ii,is,n)
			, fmt(f), littleendian(l)		{}
			DataCharacteristics( const BinDataDesc& bd )
			: BinDataDesc(bd)
			, fmt(Ieee), littleendian(__islittle__)	{}

    inline bool		isIeee() const		{ return fmt == Ieee; }

			DataCharacteristics( unsigned char c1,unsigned char c2 )
						{ set(c1,c2); }
			DataCharacteristics( const char* s )	{ set(s); }

    virtual int         maxStringifiedSize() const      { return 50; }
    virtual void	toString(char*) const;
    virtual void	set(const char*);
    virtual void	dump(unsigned char&,unsigned char&) const;
    virtual void	set(unsigned char,unsigned char);

			mDeclConstr(signed char,true,true)
			mDeclConstr(short,true,true)
			mDeclConstr(int,true,true)
			mDeclConstr(unsigned char,true,false)
			mDeclConstr(unsigned short,true,false)
			mDeclConstr(unsigned int,true,false)
			mDeclConstr(float,false,true)
			mDeclConstr(double,false,true)
			mDeclConstr(long long,true,true)

    bool		operator ==( const DataCharacteristics& dc ) const
			{ return isEqual(dc); }
    bool		operator !=( const DataCharacteristics& dc ) const
			{ return !isEqual(dc); }
    DataCharacteristics& operator =( const BinDataDesc& bd )
			{ BinDataDesc::operator=(bd); return *this; }

    bool		needSwap() const
			{ return (int)nrbytes > 1
			      && littleendian != __islittle__; }

    enum UserType	{ Auto=0, SI8, UI8, SI16, UI16, SI32, UI32, F32,
			  F64, SI64 };
			DeclareEnumUtils(UserType)
			DataCharacteristics(UserType);

};

#undef mDeclConstr

#endif
