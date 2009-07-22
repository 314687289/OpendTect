#ifndef explplaneintersection_h
#define explplaneintersection_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          May 2008
 RCS:           $Id: explplaneintersection.h,v 1.3 2009-07-22 16:01:16 cvsbert Exp $
________________________________________________________________________

-*/

#include "indexedshape.h"
#include "position.h"

class RCol;
class Plane3;

namespace Geometry
{

/*! Class that represents the line in the intersection between an
    IndexedShape and one ore many planes. */


mClass ExplPlaneIntersection: public Geometry::IndexedShape,
			     public CallBacker
{
public:
				ExplPlaneIntersection();
				~ExplPlaneIntersection();

    int				nrPlanes() const;
    int				planeID(int idx) const;
    const Coord3&		planeNormal(int id) const;
    const TypeSet<Coord3>&	planePolygon(int id) const;

    int				addPlane(const Coord3& normal,
	    				 const TypeSet<Coord3>&);
    bool			setPlane(int id, const Coord3&,
	    				 const TypeSet<Coord3>&);
    void			removePlane(int id);

    void			setShape(const IndexedShape&);
    				/*!<Is assumed to remain in memory. */
    const IndexedShape*		getShape() const;
    				

    bool			needsUpdate() const;
    void			removeAll();

    void			setZScale(float nz)	{ zscale_ = nz; }
    float			getZScale() const	{ return zscale_; }

protected:
    bool			update(bool forceall,TaskRunner*);
    
    const IndexedShape*				shape_;
    int						shapeversion_;
    bool					needsupdate_;

    IndexedGeometry*				intersection_;

    TypeSet<int>				planeids_;
    ObjectSet<TypeSet<Coord3> >			planepts_;
    TypeSet<Coord3>				planenormals_;

    float					zscale_;
};

};

#endif
