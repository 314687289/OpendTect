#ifndef bidvsetarrayadapter_h
#define bidvsetarrayadapter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	H.Huck
 Date:		March 2008
 RCS:		$Id: bidvsetarrayadapter.h,v 1.3 2009-07-22 16:01:15 cvsbert Exp $
________________________________________________________________________

-*/

#include "arraynd.h"
#include "arrayndinfo.h"
#include "binidvalset.h"


//!\brief an adapter between Array2D and a BinIDValueSet

mClass BIDValSetArrAdapter: 	public Array2D<float>
{
public:			
    			BIDValSetArrAdapter(const BinIDValueSet&,int);

    void		set(int,int,float);
    float		get(int,int) const;

    const Array2DInfo&	info() const			{ return arrinfo_; }
    Interval<int>	inlrg_;
    Interval<int>	crlrg_;
    

protected:

    Array2DInfoImpl	arrinfo_;
    BinIDValueSet	bidvs_;
    int			targetcolidx_;

};


#endif
