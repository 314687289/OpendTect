#ifndef marchingcubes_h
#define marchingcubes_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          March 2006
 RCS:           $Id: marchingcubes.h,v 1.4 2007-09-13 20:06:14 cvskris Exp $
________________________________________________________________________

-*/

#include "basictask.h"
#include "callback.h"
#include "multidimstorage.h"
#include "ranges.h"
#include "refcount.h"
#include "thread.h"

template <class T> class Array3D;
template <class T> class DataInterpreter;

class Executor;
class MarchingCuebs2ImplicitFloodFiller;
class SeedBasedFloodFiller;

/*!Representation of a triangulated MarchingCubessurface in a volume defined by
   eight voxels.

   If the MarchingCubessurface crosses any of x,y or z axes within the volume, a
   coordinate of that intersectionpoint is created in a coordinate list. Those
   positions are used by the triangles in this and neighboring volumes.

   Each of the voxels are on either side of the threshold. Since there are
   eight voxels, 256 different configurations are possible.

   Depending on the configuration of the voxels, one of 256 models are chosen,
   and some of those models have multiple submodels.

   Each model/submodel corresponds to static configuration of triangles. The
   trianlges are drawn between this volume's coordinates, and the neighbor
   volume's coordinates.
*/

class MarchingCubesModel
{
public:
				MarchingCubesModel();
				MarchingCubesModel(const MarchingCubesModel&);

    MarchingCubesModel&		operator=(const MarchingCubesModel&);
    bool 			operator==(const MarchingCubesModel&) const;

    bool			set(const Array3D<float>& arr,
	    		    	    int i0,int i1, int i2,float threshold);

    bool			isEmpty() const;

    static unsigned char 	determineModel( bool c000, bool c001, bool c010,
						bool c011, bool c100, bool c101,
						bool c110, bool c111 );

    static bool			getCornerSign(unsigned char model, int corner);
    bool			writeTo(std::ostream&,bool binary=true) const;
    bool			readFrom(std::istream&,bool binary=true);

    static const unsigned char	cUdfAxisPos;
    static const unsigned char	cMaxAxisPos;
    static const unsigned char	cAxisSpacing;

    unsigned char	model_;		//Don't change the order of these
    unsigned char	submodel_;	//since they are written to 
    unsigned char	axispos_[3]; 	//the stream in this order
};


class MarchingCubesSurface : public CallBacker
{ mRefCountImpl(MarchingCubesSurface);
public:

    			MarchingCubesSurface();

    bool		setVolumeData(int xorigin,int yorigin,int zorigin,
	    			      const Array3D<float>&,float threshold);
    			/*!<Replaces the surface within the array3d's volume
			    with an isosurface from the array and its
			    threshold. */

    void		removeAll();
    bool		isEmpty() const;

    bool		getModel(const int* pos, unsigned char& model,
	    			 unsigned char& submodel) const;

    Executor*		writeTo(std::ostream&,bool binary=true) const;
    Executor*		readFrom(std::istream&,
	    			 const DataInterpreter<od_int32>*);

    MultiDimStorage<MarchingCubesModel>		models_;
    mutable Threads::ReadWriteLock		modelslock_;

    Notifier<MarchingCubesSurface>		change;
    bool					allchanged_;
    						//!<set when change is trig.
    Interval<int>				changepos_[3];
    						//!<set when change is trig.
};


class Implicit2MarchingCubes : public ParallelTask
{
public:
    		Implicit2MarchingCubes(int posx, int posy, int posz,
				const Array3D<float>&, float threshold,
				MarchingCubesSurface&);
		~Implicit2MarchingCubes();

    int		nrTimes() const;
    bool	doWork(int,int,int);

protected:
    MarchingCubesSurface&	surface_;

    const Array3D<float>&	array_;
    float			threshold_;

    int				xorigin_;
    int				yorigin_;
    int				zorigin_;
};


class SeedBasedImplicit2MarchingCubes
{
public:
			SeedBasedImplicit2MarchingCubes(MarchingCubesSurface*,
				const Array3D<float>&,const float threshold,					int originx,int originy,int originz);
			~SeedBasedImplicit2MarchingCubes();
    bool		updateCubesFrom(int posx,int posy,int posz);

private:
    void 		seedBasedFloodFill();
    void		addMarchingCube(int idx,int idy,int idz, bool contin);
    void		processMarchingCube(int idx,int idy,int idz);

    friend				class SeedBasedFloodFiller;
    MarchingCubesSurface* 		surface_;
    const Array3D<float>*		array_;
    Array3D<unsigned char>*		visitedlocations_;
    float				threshold_;

    Threads::Mutex  			newfloodfillerslock_;
    ObjectSet<SeedBasedFloodFiller> 	newfloodfillers_;
    ObjectSet<SeedBasedFloodFiller> 	oldfloodfillers_;
    ObjectSet<SeedBasedFloodFiller> 	activefloodfillers_;

    int					xorigin_;
    int					yorigin_;
    int					zorigin_;
};


class MarchingCubes2Implicit
{
public:
		MarchingCubes2Implicit(const MarchingCubesSurface&,
					Array3D<int>&,
					int originx,int originy,int originz);
		~MarchingCubes2Implicit();
    bool	compute();
    float	threshold() const { return 0; }

protected:
    friend	class MarchingCuebs2ImplicitFloodFiller;
    friend	class MarchingCubes2ImplicitDistGen; 
    
    bool        floodFill();
    void        setValue(int xpos,int ypos,int zpos,int newval);
    static bool shouldSetResult(int newval, int oldval);

    const MarchingCubesSurface&			surface_;
    Threads::ReadWriteLock      		resultlock_;
    Array3D<int>&               		result_;

    ObjectSet<MarchingCuebs2ImplicitFloodFiller> newfloodfillers_;
    ObjectSet<MarchingCuebs2ImplicitFloodFiller> activefloodfillers_;
    ObjectSet<MarchingCuebs2ImplicitFloodFiller> oldfloodfillers_;

    int						originx_;
    int						originy_;
    int						originz_;
};


#endif
