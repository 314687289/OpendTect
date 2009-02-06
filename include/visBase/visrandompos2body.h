#ifndef visrandompos2body_h
#define visrandompos2body_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		January 2009
 RCS:		$Id: visrandompos2body.h,v 1.2 2009-02-06 20:46:05 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "position.h"

class SoShapeHints;

namespace visBase
{
class TriangleStripSet;
class Transformation;

/*!This class displays the triangulated body based on a set of given points. 
   For example

   visBase::RandomPos2Body* nb = visBase::RandomPos2Body::create();
   nb->ref();
   nb->setPoints( known_points );

   will do the display work after add nb to your scene! 
*/

mClass RandomPos2Body : public VisualObjectImpl
{
public:
    static RandomPos2Body*	create()
    				mCreateDataObj(RandomPos2Body);
					
    bool			setPoints(const TypeSet<Coord3>& pts);
    const TypeSet<Coord3>&	getPoints() const { return picks_; }

    void			setDisplayTransformation(Transformation*);
    Transformation*		getDisplayTransformation();

    void			renderOneSide(int side);
    				/*!< 0 = visisble from both sides.
				     1 = visisble from positive side
				     -1 = visisble from negative side. */

protected:
    				~RandomPos2Body();

    TypeSet<Coord3>		picks_;			
    TriangleStripSet*		triset_;
    Transformation*		transformation_;
    SoShapeHints*		hints_;
};


};


#endif
