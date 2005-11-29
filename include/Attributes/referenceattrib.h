#ifndef referenceattrib_h
#define referenceattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene PAYRAUDEAU
 Date:          July 2005
 RCS:           $Id: referenceattrib.h,v 1.5 2005-11-29 19:35:42 cvsnanne Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"


/*!\brief Reference Attribute

Provides the reference indication at every position :

Outputs 3D:
0		X position
1		Y position
2		Z (time/depth) position
3		Inline position
4		Crossline position
5		absolute sample number
6		inline index 		( taken from the first inline / 
7		crossline index		crossline / 
8		z index			z sample of the desired volume chosen )


Outputs 2D:
0		X position
1		Y position
2		Z (time/depth) position
3		Trace position
4		absolute sample number
5		Trace index 		( taken from the first trace / 
6		z index			z sample of the desired volume chosen )
*/
    

namespace Attrib
{

class Reference: public Provider
{
public:
    static void		initClass();
			Reference(Desc&);

    static const char*  attribName()		{ return "Reference"; }
    static const char*  is2DStr()       	{ return "is2D"; }

protected:
    static Provider*    createInstance(Desc&);
    static void         updateDesc( Desc& );
    
    static Provider*    internalCreate(Desc&,ObjectSet<Provider>& existing);

    bool                getInputOutput(int input,TypeSet<int>& res) const;
    bool                getInputData(const BinID&, int idx);
    bool		computeData(const DataHolder&,const BinID& relpos,
				    int t0,int nrsamples) const;

    bool		is2d_;
    
    const DataHolder*	inputdata_;
};

}; // namespace Attrib

#endif
