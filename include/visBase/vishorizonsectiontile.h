#ifndef vishorizonsectiontile_h
#define vishorizonsectiontile_h

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

#include "callback.h"
#include "thread.h"
#include "position.h"
#include "color.h"
#include "rowcol.h"
#include "visdata.h"
#include "vishorizonsection.h"

#if defined(visBase_EXPORTS) || defined(VISBASE_EXPORTS)
#include <osg/BoundingBox>
#endif

//class TileTesselator;

namespace osg
{
    class CullStack;
    class StateSet;
    class Array;
    class Switch;
    class Geode;
    class Geometry;
    class DrawElementsUShort;
}

namespace visBase
{
    class HorizonSectionTilePosSetup;
    class HorizonSection;  
    class TileResolutionData;
    class HorizonSectionTileGlue;
    class Coordinates;
//A tile with 65x65 nodes.
class HorizonSectionTile : CallBacker
{
public:
    HorizonSectionTile(const visBase::HorizonSection&,const RowCol& origin);
    ~HorizonSectionTile();
    char			getActualResolution() const;
    void			updateAutoResolution(const osg::CullStack*);
				/*<Update only when the resolution is -1. */
    void			setPos(int row,int col, const Coord3&,int res);
    void			setPositions(const TypeSet<Coord3>&);
				//Call by the end of each render
				//Makes object ready for render
    void			updateNormals( char res);
    void			tesselateResolution(char,bool onlyifabsness);
    void			applyTesselation(char res);
				//!<Should be called from rendering thread
    void			ensureGlueTesselated();
    void			setWireframeColor(Color& color);
    void			setTexture( const Coord& origin, 
					    const Coord& opposite );
				//!<Sets origin and opposite in global texture
    void			addTileTesselator( int res );
    void			addTileGlueTesselator();
    void			setDisplayTransformation(const mVisTrans*);

protected:

    void			setNeighbor(int neighbor,HorizonSectionTile*);
				//!<The neighbor is numbered from 0 to 8

    void			setResolution(char);
				/*!<Resolution -1 means it is automatic. */

    bool			allNormalsInvalid(char res) const;
    void			setAllNormalsInvalid(char res,bool yn);
    void			emptyInvalidNormalsList(char res);

    bool			hasDefinedCoordinates(int idx) const;
				/*!<idx is the index of coordinates 
				in the highest resolution vertices. */

    const HorizonSectionTile*	getNeighborTile(int idx) const;
				//!<idx is bewteen 0 and 8

    void			enableGeometryTypeDisplay(GeometryType type, 
							  bool yn);
				/*!<type 0 is triangle,1 is line, 
				2 is point, 3 is wire frame */

    void			updatePrimitiveSets();
    const visBase::Coordinates* getHighestResolutionCoordinates();
    void			dirtyGeometry();

protected:

    friend class		HorizonSectionTilePosSetup;
    friend class		TileTesselator;		
    friend class		HorizonSection;  
    friend class		TileResolutionData;
    friend class		HorTilesCreatorAndUpdator;
    friend class		HorizonSectionTileGlue;
    friend class		HorizonTextureHandler;

    void			updateBBox();
    void			buildOsgGeometries();
    void			setActualResolution(char);
    char			getAutoResolution(const osg::CullStack*);

    HorizonSectionTile*		neighbors_[9];

    HorizonSectionTileGlue*	righttileglue_;
    HorizonSectionTileGlue*	bottomtileglue_;

    osg::BoundingBox		bbox_;
    const RowCol		origin_;
    const HorizonSection&	hrsection_;
    bool			wireframedisplayed_;

    char			desiredresolution_;
    int				nrdefinedvertices_;

    bool			resolutionhaschanged_;
    bool			needsupdatebbox_;

    int				tesselationqueueid_;
    char			glueneedsretesselation_;
				//!<0 - updated, 1 - needs update, 2 - dont disp

    osg::StateSet*		stateset_;
    TypeSet<int>		txunits_;

    ObjectSet<TileResolutionData>tileresolutiondata_;
    osg::Switch*		osgswitchnode_;
    Threads::Mutex		datalock_;
    bool			updatenewpoint_;

};

}
#endif
