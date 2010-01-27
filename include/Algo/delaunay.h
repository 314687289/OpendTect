#ifndef delaunay_h
#define delaunay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Y.C. Liu
 Date:          January 2008
 RCS:           $Id: delaunay.h,v 1.30 2010-01-27 23:00:43 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "odmemory.h"
#include "sets.h"
#include "task.h"
#include "thread.h"

/*!Reference: "Parallel Incremental Delaunay Triangulation", by Kohout J.2005.
   For the triangulation, it will skip undefined or duplicated points, all the 
   points should be in random order. We use Kohout's pessimistic method to
   triangulate. The problem is that the pessimistic method only give a 10% speed
   increase, while the locks slows it down. The paralell code is thus
   disabled with a macro.
*/

#define mDAGTriangleForceSingleThread
mClass DAGTriangleTree
{
public:
    			DAGTriangleTree();
    			DAGTriangleTree(const DAGTriangleTree&);
    virtual		~DAGTriangleTree();
    DAGTriangleTree&	operator=(const DAGTriangleTree&);

    static bool		computeCoordRanges(const TypeSet<Coord>&,
	    		    Interval<double>&,Interval<double>&);

    bool		setCoordList(const TypeSet<Coord>* OD);
    bool		setCoordList(TypeSet<Coord>*,OD::PtrPolicy);
    const TypeSet<Coord>& coordList() const { return *coordlist_; }

    bool		setBBox(const Interval<double>& xrg,
	    			const Interval<double>& yrg);

    bool		isOK() const { return triangles_.size(); }

    bool		init();

    bool		insertPoint(int pointidx, int& dupid);
    int			insertPoint(const Coord&, int& dupid);
    
    const Coord		getInitCoord(int vetexidx) const;
    bool		getTriangle(const Coord&,int& dupid,
	    			    TypeSet<int>& vertexindices) const;
    			/*!<search triangle contains the point.return crds. */
    bool		getCoordIndices(TypeSet<int>&) const;
    			/*!<Coord indices are sorted in threes, i.e
			    ci[0], ci[1], ci[2] is the first triangle
			    ci[3], ci[4], ci[5] is the second triangle. */
    bool		getSurroundingIndices(TypeSet<int>&) const;
    			/*!Points on the edge of the geometry shape. */

    bool		getConnections(int pointidx,TypeSet<int>&) const;
    bool		getWeights(int pointidx,const TypeSet<int>& conns,
				   TypeSet<double>& weights,
				   bool normailze=true) const;
    			/*!Calculate inverse distance weight for each conns.*/
    bool		getConnectionAndWeights(int ptidx,TypeSet<int>& conns,
	    				     TypeSet<double>& weights,
					     bool normailze=true) const;
    void		setEpsilon(double err)	{ epsilon_ = err; }

    void		dumpTo(std::ostream&) const;
    			//!<Dumps all triangles to stream;

    static int		cNoVertex()	{ return -1; }

protected:

    static char		cIsOnEdge() 	{ return 0; }
    static char		cNotOnEdge() 	{ return 1; }
    static char		cIsInside()	{ return 2; }
    static char		cIsDuplicate()	{ return 3; }
    static char		cIsOutside()	{ return 4; }
    static char		cError()	{ return -1; }

    static int		cNoTriangle()	{ return -1; }
    static int		cInitVertex0()	{ return -2; }
    static int		cInitVertex1()	{ return -3; }
    static int		cInitVertex2()	{ return -4; }

    char	getCommonEdge(int ti0,int ti1) const;
    		/*!return the common edge in ti0. */
    char	searchTriangle(const Coord& pt,int start, int& t0,int& t1,
	    		       int& dupid) const;
    char	searchFurther(const Coord& pt,int& nti0,int& nti1,
	    		      int& dupid) const;
    char	searchTriangleOnEdge(const Coord& pt,int ti,int& resti,
	    			     char& edge, int& did) const;
   		/*!<assume ci is on the edge of ti.*/
    int		searchNeighbor(int ti,char edge) const;

    void	splitTriangleInside(int ci,int ti);
    		/*!ci is assumed to be inside the triangle ti. */
    void	splitTriangleOnEdge(int ci,int ti0,int ti1);
    		/*!ci is on the shared edge of triangles ti0, ti1. */
    void	legalizeTriangles(TypeSet<char>& v0s,TypeSet<char>& v1s,
			TypeSet<int>& tis);
    		/*!Check neighbor triangle of the edge v0-v1 in ti, 
		   where v0, v1 are local vetex indices 0, 1, 2. */

    int		getNeighbor(int v0,int v1,int ti) const;
    int		searchChild(int v0,int v1,int ti) const;
    char	isOnEdge(const Coord& p,const Coord& a,const Coord& b,
			 bool& duponfirst,double& signedsqdist ) const;
    char	isInside(const Coord& pt,int ti,char& edge,double& disttoedge,
	    		 int& dupid) const;

    struct DAGTriangle
    {
			DAGTriangle();
	bool		operator==(const DAGTriangle&) const;
	DAGTriangle&	operator=(const DAGTriangle&);

	int		coordindices_[3];
	int		childindices_[3];
	int		neighbors_[3];

	bool		hasChildren() const;
    };

#ifndef mDAGTriangleForceSingleThread
    mutable Threads::ReadWriteLock	trianglelock_;
#endif
    double				epsilon_;
    TypeSet<DAGTriangle>		triangles_;

    TypeSet<Coord>*			coordlist_;
    bool				ownscoordlist_;

#ifndef mDAGTriangleForceSingleThread
    mutable Threads::ReadWriteLock	coordlock_;
#endif

    bool				planedirection_;
    					/*!All triangles have the same sign. */
    Coord				initialcoords_[3]; 
    					/*!<-2,-3,-4 are their indices.*/
};

/*!<The parallel DTriangulation works for only one processor now.*/
mClass ParallelDTriangulator : public ParallelTask
{
public:
			ParallelDTriangulator(DAGTriangleTree&);
	
    bool		isDataRandom()		{ return israndom_; }
    void		dataIsRandom(bool yn)	{ israndom_ = yn; }
    void		setCalcScope(const Interval<int>& rg);

protected:

#ifdef mDAGTriangleForceSingleThread
    int			maxNrThreads() const { return 1; }
#endif
    od_int64		nrIterations() const;
    bool		doWork(od_int64,od_int64, int );
    bool		doPrepare(int);

    TypeSet<int>	permutation_;
    bool		israndom_;
    DAGTriangleTree&	tree_;
    Interval<int>	calcscope_;
};


/*For a given triangulated geometry(set of points), interpolating any point 
  located in or nearby the goemetry. If the point is located outside of the 
  boundary of the geometry, we compare azimuth to find related points and then
  apply inverse distance to calculate weights. */
class Triangle2DInterpolator
{
public:
    			Triangle2DInterpolator(const DAGTriangleTree&);

			/*The vertices are indices from the DAGTriangleTree 
			  coordlist, corresponding to the weights.*/
    bool		computeWeights(const Coord&,TypeSet<int>& vertices,
	    			       TypeSet<float>& weights,
				       double maxdist=mUdf(double),
				       bool dointerpolate=true);
    			/*If don't do interpolate, nearest node will be taken.*/

protected:

    bool			setFromAzimuth(const TypeSet<int>& tmpvertices,
	    				       const Coord&,
					       TypeSet<int>& vertices, 
					       TypeSet<float>& weights);
    const DAGTriangleTree&	triangles_;
    TypeSet<int>		corner0_;
    TypeSet<double>		cornerweights0_;
    TypeSet<int>		corner1_;
    TypeSet<double>		cornerweights1_;
    TypeSet<int>		corner2_;
    TypeSet<double>		cornerweights2_;
    Coord			initcenter_;

    TypeSet<int>		perimeter_;
    TypeSet<double>		perimeterazimuth_;
    double			initazimuth_[3];
    double			maxdist_;
};
    			


#endif

