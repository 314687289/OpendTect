#ifndef SoKrisSurface_h
#define SoKrisSurface_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoLODMeshSurface.h,v 1.1 2005-03-22 14:38:15 cvskris Exp $
________________________________________________________________________


-*/

#include "Inventor/nodes/SoShape.h"
#include "Inventor/fields/SoSFShort.h"
#include "Inventor/fields/SoMFInt32.h"
#include "Inventor/fields/SoMFVec3f.h"
#include "Inventor/fields/SoSFInt32.h"
#include "Inventor/fields/SoSFBool.h"
#include "Inventor/fields/SoSFFloat.h"
#include "Inventor/fields/SoMFShort.h"
#include "Inventor/lists/SbList.h"
#include "Inventor/nodes/SoSubNode.h"

class SbVec3f;

namespace MeshSurfImpl { class MeshSurfacePart; };

/*!
A class for displaying mesh-surfaces with LOD functionality


meshStyle

0    ------	Auto - the tesselation is based on the data
     |\XX/|
     |X\/X|
     |X/\X|
     |/XX\|
     ------

1    ------
     |\XXX|
     |X\XX|
     |XX\X|
     |XXX\|
     ------

2    ------
     |XXX/|
     |XX/X|
     |X/XX|
     |/XXX|
     ------

3    ------
     |\   |
     |X\  |
     |XX\ |
     |XXX\|
     ------

4    ------
     |   /|
     |  /X|
     | /XX|
     |/XXX|
     ------

5    ------
     |\XXX|
     | \XX|
     |  \X|
     |   \|
     ------

6    ------
     |XXX/|
     |XX/ |
     |X/  |
     |/   |
     ------

7    ------
     |    |
     |    |
     |    |
     |    |
     ------

*/


class SoLODMeshSurface : public SoShape
{
    SO_NODE_HEADER(SoLODMeshSurface);
public:
    SoMFVec3f		coordinates;
    			/* If the x component of a coordinate less than
			   1e29, the coordinate is considered defined */
    SoMFInt32		materialIndex;
    SoMFShort		meshStyle;
    			
    SoSFShort		nrColumns;
    SoSFShort		resolution;
    SoSFBool		wireframe;
    SoSFFloat		wireframeLift;

    void		insertColumns( bool beforeexisting, int nr );
    			/*!<Inserts nr columns before or after the current
			    mesh. The function will fill the new space with
			    -1s and will not trigger a invalidation of caches.
			    if not neccessary.
			*/
    void		insertRowsBefore(int nr);
    			/*!<Inserts nr rows before the current mesh. The
			    function will fill the new space with -1s and
			    will not trigger a invalidation of caches.
			*/

    void		turnOnOwnValidation(bool yn);
    void		touch( int, int, bool undef );

    void		generatePrimitives(SoAction*) {}
    void		computeBBox(SoAction*, SbBox3f&, SbVec3f&);
    void		rayPick( SoRayPickAction*);
    void 		GLRender(SoGLRenderAction*);
    void		getBoundingBox(SoGetBoundingBoxAction *action);

    int			getIndex( int, int ) const;

    			SoLODMeshSurface();
    static void		initClass();

    int			nrRows() const;

protected:
    void		notify( SoNotList* );

    void		adjustNrOfParts();

    SbList<MeshSurfImpl::MeshSurfacePart*>	parts;
    bool					useownvalidation;
    int						nrcolparts;
    int						sidesize;
};



#endif
    
