#ifndef viscube_h
#define viscube_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visshape.h"
#include "position.h"

class SoCube;
class SoTranslation;

namespace Geometry { class Pos; };

namespace visBase
{
class Scene;

/*!\brief

Cube is a basic cube that is settable in size.

*/

mClass(visBase) Cube : public Shape
{
public:
    static Cube*	create()
			mCreateDataObj(Cube);

    void		setCenterPos( const Coord3& );
    Coord3		centerPos() const;
    
    void		setWidth( const Coord3& );
    Coord3		width() const;

    void		setDisplayTransformation( const mVisTrans* );
    const mVisTrans*	getDisplayTransformation() const { return transformation; }

protected:
    			~Cube();
    const mVisTrans*	transformation;
    SoTranslation*	position;
};

};


#endif

