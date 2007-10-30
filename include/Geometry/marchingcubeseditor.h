#ifndef marchingcubeseditor_h
#define marchingcubeseditor_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          August 2007
 RCS:           $Id: marchingcubeseditor.h,v 1.5 2007-10-30 16:53:35 cvskris Exp $
________________________________________________________________________

-*/

#include "task.h"
#include "callback.h"
#include "position.h"

class MarchingCubesSurface;
template <class T> class Interval;
template <class T> class Array3D;

/*!
Editor for MarchingCubesSurfaces. It operates by converting a part of the
MarchingCubesSurface to an implicit represetation (a scalar field and a
threshold), modifies that scalar field and re-generates the
MarchingCubesSurfaces.

The modification of the implicit representation is guided by:

NewImplicitShape = OriginalShape + Kernel * factor

The Kernel is a scalar field (values between 0 and 255) represents the shape of 
the movement, and the factor is set by from the outside.

*/


class MarchingCubesSurfaceEditor : private ParallelTask, public CallBacker
{
public:
    			MarchingCubesSurfaceEditor(MarchingCubesSurface&);
    virtual		~MarchingCubesSurfaceEditor();

    bool		setKernel(const Array3D<unsigned char>&,
	    			  int xpos, int ypos, int zpos );
    			//!<Kernel becomes mine

    bool		setFactor(int); //!<a value of 255 is one voxel if
    					//!<the kernel is 255
    int			getFactor() const { return factor_; }

    const Coord3&	getCenterNormal() const;

    virtual bool	affectedVolume(Interval<int>& xrg,
	    			       Interval<int>& yrg,
				       Interval<int>& zrg) const;

    Notifier<MarchingCubesSurfaceEditor>	shapeChange;
    
protected:
    void			reportShapeChange(bool kernelchange);
    				/*!<Should be called by inheriting class
				    if it has changed kernel size or
				    kernel position. */

    MarchingCubesSurface&	surface_;
    int				factor_;
    int				prevfactor_;
    Array3D<unsigned char>*	kernel_;
    Array3D<int>*		changedsurface_;
    Array3D<int>*		originalsurface_;
    float			threshold_;
    Coord3			centernormal_;

    int				xorigin_;
    int				yorigin_;
    int				zorigin_;

private:
    int				totalNr() const;
    bool			doPrepare(int);
    bool			doWork(int,int,int);
    bool			doFinish(bool);
};

#endif
