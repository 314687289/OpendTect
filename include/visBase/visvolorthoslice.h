#ifndef visvolorthoslice_h
#define visvolorthoslice_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          November 2002
 RCS:           $Id: visvolorthoslice.h,v 1.1 2007-01-03 18:20:11 cvskris Exp $
________________________________________________________________________

-*/

#include "visobject.h"
#include "position.h"

class SoOrthoSlice;
template <class T> class Interval;


namespace visBase
{
class DepthTabPlaneDragger; class PickStyle;

/*!\brief
Slice that cuts orthogonal through a VolumeData.
*/

class OrthogonalSlice : public visBase::VisualObjectImpl
{
public:
    static OrthogonalSlice*	create()
				mCreateDataObj(OrthogonalSlice);

    void			setVolumeDataSize(int xsz,int ysz,int zsz);
    void			setSpaceLimits(const Interval<float>& x,
					       const Interval<float>& y,
					       const Interval<float>& z);

    int				getDim() const;
    void			setDim(int);

    int				getSliceNr() const;
    void			setSliceNr( int );

    float			getPosition() const;

    virtual void		fillPar(IOPar&,TypeSet<int>&) const;
    virtual int			usePar(const IOPar&);

    Notifier<OrthogonalSlice>	motion;

protected:
					~OrthogonalSlice();

    void			draggerMovementCB(CallBacker*);
    void			getSliceInfo(int&,Interval<float>&) const;
    
    visBase::DepthTabPlaneDragger* dragger;
    visBase::PickStyle*		pickstyle;
    SoOrthoSlice*		slice;
    int				xdatasz,ydatasz,zdatasz;

    static const char*		dimstr;
    static const char*		slicestr;
};


};
	
#endif
