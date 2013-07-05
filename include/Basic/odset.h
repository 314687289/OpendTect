#ifndef odset_h
#define odset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2009
 RCS:		$Id$
________________________________________________________________________

-*/

#ifndef gendefs_h
# include "gendefs.h"
#endif

namespace OD
{

/*!
\brief Base class for all sets used in OpendTect. 
*/

mExpClass(Basic) Set
{
public:

    virtual		~Set()					{}

    virtual od_int64	nrItems() const				= 0;
    virtual bool	validIdx(od_int64) const		= 0;
    virtual void	swap(od_int64,od_int64)			= 0;
    virtual void	erase()					= 0;
    virtual void	removeRange(od_int64 start,od_int64 stop)  = 0;

    inline bool		isEmpty() const		{ return nrItems() <= 0; }
    inline void		setEmpty()		{ erase(); }

};

} // namespace

#define mODSetApplyToAll( tp, os, op ) \
    for ( tp idx=(tp) os.nrItems()-1; idx>=0; idx-- ) \
        op


/*!\brief Adds all names from a set to another set with an add() function
  	(typically a BufferStringSet)

Note: will only work for sets with int indexes. This will be fixed after od4.6.

 */

template <class ODSET,class WITHADD>
inline void addNames( const ODSET& inp, WITHADD& withadd )
{
    const int sz = (int)(inp.size());
    for ( int idx=0; idx<sz; idx++ )
	withadd.add( inp[idx]->name() );
}


#endif

