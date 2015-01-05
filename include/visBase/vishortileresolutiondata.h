#ifndef vishortileresolutiondata_h
#define vishortileresolutiondata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		March 2009
 RCS:		$Id$
________________________________________________________________________


-*/

// this header file only be used in the classes related to Horzonsection . 
// don't include it in somewhere else !!!


#include "typeset.h"
#include "vistransform.h"
#include "zaxistransform.h"
#include "vishorizonsectiontile.h"

#include "thread.h"

#if defined(visBase_EXPORTS) || defined(VISBASE_EXPORTS)
#include <osg/BoundingBox>
#endif


namespace osg
{
    class Geometry;
    class Geode;
    class UserDataContainer;
    class Array;
    class StateSet;
    class DrawElementsUShort;
}

namespace osgUtil { class CullVisitor; }

namespace osgGeo { class LayeredTexture;  }

namespace visBase
{
    class HorizonSectionTile;
    class TextureChannels;
    class Coordinates;

class TileResolutionData
{
public:
    TileResolutionData( const HorizonSectionTile* sectile, 
			char resolution );
    ~TileResolutionData();

    void		setSingleVertex(int row, int col,const Coord3& pos,
					bool& dohide);
    void		setDisplayTransformation( const mVisTrans* t ); 
    void		setTexture(const unsigned int unit, osg::Array* arr,
				    osg::StateSet* stateset);
    void		enableGeometryTypeDisplay(GeometryType type, bool yn);

    void		calcNormals(bool allownormalinvalid = true );
    bool		tesselateResolution(bool onlyifabsness);

    osg::BoundingBox&	updateBBox();
    void		updatePrimitiveSets();
    void		setWireframeColor(Color& color);
    visBase::Coordinates*	getCoordinates() { return vertices_; };
    void		dirtyGeometry();
    bool		hasDefinedCoordinates(int idx) const;
			/*!<idx is the index of coordinates in the 
			highest resolution vertices.*/

    void		setVerticesPositions(TypeSet<Coord3>* positions = 0 );

protected:

    friend class HorizonSectionTile;
    friend class HorizonSectionTileGlue;

    const HorizonSectionTile*	sectile_;
    osg::Switch*    		osgswitch_;
    osg::UserDataContainer*	geodes_;

    visBase::Coordinates*	vertices_;
    osg::Array*			normals_;
    std::vector<osg::Array*>	txcoords_;
    osg::Array*			linecolor_;

    osg::DrawElementsUShort*	trianglesps_;
    osg::DrawElementsUShort*	linesps_;
    osg::DrawElementsUShort*	pointsps_;
    osg::DrawElementsUShort*	wireframesps_;

    osg::DrawElementsUShort*    trianglesosgps_;
    osg::DrawElementsUShort*	linesosgps_;
    osg::DrawElementsUShort*	pointsosgps_;
    osg::DrawElementsUShort*	wireframesosgps_;

    osg::BoundingBox		bbox_;
    Threads::Mutex 		tesselatemutex_;

    bool			updateprimitiveset_;
    bool			allnormalsinvalid_;
    char			needsretesselation_;
    char			resolution_;
    TypeSet<int>		invalidnormals_;
    int				nrdefinedvertices_;
    int				nrverticesperside_;
    double			cosanglexinl_;
    double			sinanglexinl_;
    bool			needsetposition_;
    int				dispgeometrytype_;

private:

    void			buildOsgGeometres();
    void			initVertices();
    void			initTexCoordLayers();
    void			setNrTexCoordLayers(int);
    void			setPrimitiveSet(unsigned int, 
						osg::DrawElementsUShort*);

    void			tesselateCell(int cellidx);
    void			computeNormal(int nmidx, osg::Vec3&);
    double			calcGradient(int row,int col,
					     const StepInterval<int>& rcrange,
					     bool isrow);
    void			refOsgPrimitiveSets();
    void			unRefOsgPrimitiveSets();
    void			createPrimitiveSets();
    void			buildLineGeometry(int idx, int width);
    void			buildTraingleGeometry(int idx);
    void			buildPointGeometry(int idx);
    void			setInvalidNormal(int row,int col);
    bool			setVerticesFromHighestResolution();
    void			hideFromDisplay();
    bool			detectIsolatedLine(int crdidx,char direction);
    void			setGeometryTexture(const unsigned int unit, 
						   const osg::Array* arr,
						   osg::StateSet*stateset,
						   int geometrytype);
    void			dirtyGeometry(int type);

public:
    bool			getTextureCoordinates(unsigned int unit,
						      TypeSet<Coord>&) const;
    const osg::PrimitiveSet*	getPrimitiveSet(GeometryType) const;
    const osg::Vec3Array*	getOsgCoordinates()const;
    const osg::Vec3Array*	getNormals() const;
};

}
#endif
