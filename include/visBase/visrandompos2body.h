#ifndef visrandompos2body_h
#define visrandompos2body_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		January 2009
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visobject.h"
#include "position.h"

namespace visBase
{
class TriangleStripSet;
class Transformation;
class VertexShape;

/*!This class displays the triangulated body based on a set of given points. 
   For example

   visBase::RandomPos2Body* nb = visBase::RandomPos2Body::create();
   nb->ref();
   nb->setPoints( known_points );

   will do the display work after add nb to your scene! 
*/

mExpClass(visBase) RandomPos2Body : public VisualObjectImpl
{
public:
    static RandomPos2Body*	create()
    				mCreateDataObj(RandomPos2Body);
					
    bool			setPoints(const TypeSet<Coord3>& pts);
    const TypeSet<Coord3>&	getPoints() const { return picks_; }

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    void			renderOneSide(int side);
    				/*!< 0 = visible from both sides.
				   1 = visible from positive side
				  -1 = visible from negative side. */
protected:
    				~RandomPos2Body();

    TypeSet<Coord3>		picks_;			
    VertexShape*		vtxshape_;
    const mVisTrans*		transformation_;
};


};


#endif

