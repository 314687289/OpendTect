#ifndef visgeomindexedshape_h
#define visgeomindexedshape_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		September 2007
 RCS:		$Id: visgeomindexedshape.h,v 1.9 2009-05-20 21:45:21 cvskris Exp $
________________________________________________________________________

-*/

#include "valseries.h"
#include "visobject.h"
#include "coltabsequence.h"
#include "coltabmapper.h"

namespace Geometry { class IndexedShape; class IndexedGeometry; }

class SoMaterial;
class SoShapeHints;
class SoIndexedShape;
class TaskRunner;
class DataPointSet;

namespace visBase
{

class Coordinates;
class Normals;
class TextureCoords;

/*!Visualisation for Geometry::IndexedShape. */

mClass GeomIndexedShape : public VisualObjectImpl
{
public:
    static GeomIndexedShape*	create()
				mCreateDataObj(GeomIndexedShape);

    void			setDisplayTransformation(mVisTrans*);
    mVisTrans*			getDisplayTransformation();

    void			setSurface(Geometry::IndexedShape*,
	    				   TaskRunner* = 0);
    				//!<Does not become mine, should remain
				//!<in memory
    void			setRightHandSystem(bool);

    void			touch(bool forall,TaskRunner* =0);

    void			set3DLineRadius(float radius,
	    					bool constantonscreen=true,
						float maxworldsize=-1);
    				/*!<If radius is less than 0, a normal
				    line will be drawn. */
    void			renderOneSide(int side);
    				/*!< 0 = visisble from both sides.
				     1 = visisble from positive side
				     -1 = visisble from negative side. */

    void			createColTab();
    void			enableColTab(bool);
    void			setDataMapper(const ColTab::MapperSetup&);
    const ColTab::MapperSetup*	getDataMapper() const;
    void			setDataSequence(const ColTab::Sequence&);
    const ColTab::Sequence*	getDataSequence() const;

    void			getAttribPositions(DataPointSet&) const;
    void			setAttribData(const DataPointSet&);

protected:
				~GeomIndexedShape();
    void			reClip();
    void			reMap();

    mClass			ColTabMaterial
    {
    public:
					ColTabMaterial();
					~ColTabMaterial();
	ColTab::Mapper			mapper_;
	ColTab::Sequence                sequence_;

	SoMaterial*                     coltab_;
	ArrayValueSeries<float,float>	cache_;
    };

    static const char*			sKeyCoordIndex() { return "CoordIndex";}

    ColTabMaterial*				ctab_;

    SoShapeHints*				hints_;
    Coordinates*				coords_;
    Normals*					normals_;
    TextureCoords*				texturecoords_;

    ObjectSet<SoIndexedShape>			strips_;
    ObjectSet<const Geometry::IndexedGeometry>	stripgeoms_;

    float					lineradius_;
    bool					lineconstantonscreen_;
    float					linemaxsize_;
    ObjectSet<SoIndexedShape>			lines_;
    ObjectSet<const Geometry::IndexedGeometry>	linegeoms_;

    ObjectSet<SoIndexedShape>			fans_;
    ObjectSet<const Geometry::IndexedGeometry>	fangeoms_;

    Geometry::IndexedShape*			shape_;
};

};
	
#endif
