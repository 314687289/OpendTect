#ifndef zaxistransformdatapack_h
#define zaxistransformdatapack_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2007
 RCS:		$Id: zaxistransformdatapack.h,v 1.8 2009-07-22 16:01:16 cvsbert Exp $
________________________________________________________________________

-*/

#include "datapackbase.h"

#include "cubesampling.h"

template <class T> class Array2D;
template <class T> class Array2DSlice;
template <class T> class Array3D;
class FlatPosData;
class ZAxisTransform;


/*!\brief DataPack for ZAxis transformed data.
*/


mClass ZAxisTransformDataPack : public FlatDataPack
{
public:
    				ZAxisTransformDataPack(const FlatDataPack&,
						       const CubeSampling&,
						       ZAxisTransform&);
				~ZAxisTransformDataPack();

    void			setOutputCS(const CubeSampling&);

    bool			transform();

    void			setInterpolate( bool yn ) { interpolate_ = yn; }
    bool			getInterpolate() const	 { return interpolate_;}


    const ZAxisTransform&	getTransform() const { return transform_; }

    Array2D<float>&		data();
    const Array2D<float>&	data() const;
    virtual void       		dumpInfo(IOPar&) const;

    virtual const char*		dimName(bool) const;

protected:

    const FlatDataPack&		inputdp_;
    CubeSampling		inputcs_;
    CubeSampling*		outputcs_;

    ZAxisTransform&		transform_;
    bool			interpolate_;

    const Array3D<float>*	array3d_;
    Array2DSlice<float>*	array2dsl_;
};


#endif
