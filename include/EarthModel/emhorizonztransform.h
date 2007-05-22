#ifndef emhorizonztransform_h
#define emhorizonztransform_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		April 2006
 RCS:		$Id: emhorizonztransform.h,v 1.7 2007-05-22 03:23:22 cvsnanne Exp $
________________________________________________________________________


-*/

#include "zaxistransform.h"

namespace EM
{
class Horizon3D;

/*!Z-transform that flatterns a horizon. Everything else will also be flatterned
accordingly. In case of reverse faulting, the area between the two patches will
not be included.  */

class HorizonZTransform : public ZAxisTransform
			, public CallBacker
{
public:
    static void		initClass();
    const char*		name() const		{ return sName(); }
    static const char*	sName()			{ return "HorizonZTransform"; }
    static const char*	sKeyHorizonID()		{ return "Horizon"; }

    			HorizonZTransform(const Horizon3D* = 0);
    void		setHorizon(const Horizon3D&);
    void		transform(const BinID&,const SamplingData<float>&,
				  int sz,float* res) const;
    void		transformBack(const BinID&,const SamplingData<float>&,
				  int sz,float* res) const;

    Interval<float>	getZInterval(bool from) const;
    float		getZIntervalCenter(bool from) const;
    bool		needsVolumeOfInterest() const { return false; }

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    static ZAxisTransform* create() { return new HorizonZTransform( 0 ); }
    			~HorizonZTransform();
    void		calculateHorizonRange();
    void		horChangeCB( CallBacker* );
    bool		getTopBottom(const BinID&,float&top,float&bottom) const;

    const Horizon3D*	horizon_;
    Interval<float>	depthrange_;
    bool		horchanged_;
};


}; // Namespace


#endif
