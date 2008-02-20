#ifndef explfaultsticksurface_h
#define explfaultsticksurface_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        J.C. Glas
 Date:          October 2007
 RCS:           $Id: explfaultsticksurface.h,v 1.3 2008-02-20 11:52:02 cvsjaap Exp $
________________________________________________________________________

-*/

#include "indexedshape.h"
#include "position.h"

class RCol;

namespace Geometry
{

class FaultStickSurface;

/*!A triangulated representation of a faultsticksurface */


class ExplFaultStickSurface: public Geometry::IndexedShape,
			     public CallBacker
{
public:
			ExplFaultStickSurface(FaultStickSurface*,float zscale);
    			~ExplFaultStickSurface();

    void		setSurface(FaultStickSurface*);
    FaultStickSurface*	getSurface()			{ return surface_; }
    const FaultStickSurface* getSurface() const		{ return surface_; }

    void		setZScale( float );
    
    void		updateAll();
    void		display(bool sticks,bool panels);

protected:
    friend		class ExplFaultStickSurfaceUpdater;    

    void		removeAll();
    void		insertAll();
    void		update();
    
    void		addToGeometries(IndexedGeometry*);
    void		addToGeometries(ObjectSet<IndexedGeometry>&);
    void		removeFromGeometries(const IndexedGeometry* first,
					     int total=1);

    void		emptyStick(int stickidx);
    void		fillStick(int stickidx);
    void		removeStick(int stickidx);
    void		insertStick(int stickidx);

    void		emptyPanel(int panelidx);
    void		fillPanel(int panelidx);
    void		removePanel(int panelidx);
    void		insertPanel(int panelidx);

    void		calcNormals(IndexedGeometry&,bool mirrored);

    void		surfaceChange(CallBacker*);


    bool		displaysticks_;
    bool		displaypanels_;

    FaultStickSurface*	surface_;
    Coord3		scalefacs_;

    ObjectSet<IndexedGeometry>			sticks_;
    ObjectSet<ObjectSet<IndexedGeometry> >	panels_;
};

};

#endif
