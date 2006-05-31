#ifndef attribslice_h
#define attribslice_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Apr 2002
 RCS:           $Id: attribslice.h,v 1.4 2006-05-31 18:53:37 cvsnanne Exp $
________________________________________________________________________

-*/

#include "sets.h"
#include "arrayndimpl.h"
#include "cubesampling.h"
#include "refcount.h"

/*!\brief Slice containing attribute values.
 
  The sliceidx determines the position of the slice in the requested cube,
  see AttribSliceSet for details.
 
 */

namespace Attrib
{

class Slice : public Array2DImpl<float>
{ mRefCountImplNoDestructor(Slice);
public:

    			Slice(int nrows,int ncols,float udfval=0,
			      bool file=false);
    float		undefValue() const;
    void		setUndefValue( float udfval, bool initdata=false );

protected:

    float		udfval_;

};


/*!\brief Set of attrib slices.

 The two array2d directions shall be filled following the CubeSampling
 convention. The slices will be in order of increasing inl, crl or Z.
 
 Slices can be null!
 
 */

class SliceSet : public ObjectSet<Slice>
{ mRefCountImpl(SliceSet);
public:

			SliceSet();

    CubeSampling::Dir	direction;
    CubeSampling	sampling;

    void		getIdx(int dimnr,int inl,int crl,float z,int&) const;
    void		getIdxs(int inl,int crl,float z,int&,int&,int&) const;

    Array3D<float>*	createArray(int inldim=0, int crlcim=1,
	    			    int depthdim=2) const;
    			/*!< Makes an array where the dims are as specified
			 */
    float               getValue(int inl,int crl,float z) const;

};

}; //namespace

#endif
