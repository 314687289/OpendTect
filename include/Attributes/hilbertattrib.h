#ifndef hilbertattrib_h
#define hilbertattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: hilbertattrib.h,v 1.13 2008-11-28 08:53:22 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"

namespace Attrib
{

/*!\brief Hilbert attribute.

  Calculates Hilbert transform
*/


class Hilbert : public Provider
{
public:
    static void			initClass();
				Hilbert(Desc&);

    static const char*		attribName()	{ return "Hilbert"; }
    static const char*		halflenStr()	{ return "halflen"; }

protected:
    static Provider*		createInstance(Desc&);

    bool			getInputOutput(int inp,TypeSet<int>& res) const;
    bool			getInputData(const BinID&, int);
    bool			computeData(const DataHolder&,const BinID& pos,
					    int z0,int nrsamples,
					    int threadid) const;

    bool			allowParallelComputation() const
    				{ return false; }
    const Interval<int>*        desZSampMargin(int input,int output) const
				{ return &zmargin_; }

    const DataHolder*		inputdata_;
    int				dataidx_;

    Interval<int>		zmargin_;
    int				halflen_;
};

}; // namespace Attrib

#endif
