#ifndef vismarker_h
#define vismarker_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		July 2002
 RCS:		$Id: vismarker.h,v 1.16 2004-11-03 08:39:08 kristofer Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "trigonometry.h"
#include "draw.h"

class SoGroup;
class SoShapeScale;
class SoRotation;
class SoTranslation;
class UTMPosition;

namespace visBase
{
class Transformation;

/*!\brief

Marker is a basic pickmarker with a constant size on screen. 
Size and shape are settable.

*/

class Marker : public VisualObjectImpl
{
public:
    static Marker*	create()
			mCreateDataObj(Marker);

    void		setMarkerStyle(const MarkerStyle3D&);
    			/*!<Sets predefined shape and size. */
    const MarkerStyle3D& getMarkerStyle() const	{ return markerstyle; }
    void		setType(MarkerStyle3D::Type);
    			/*!<Sets predefined shape. */
    MarkerStyle3D::Type	getType() const;

    void		setMarkerShape(SoNode*);
    			/*!< Sets user-defined shape. The shape is expected
			     to be about two units large and centered at 
			     origo, i.e. in box (-1,-1,-1) to (1,1,1).
			     \note If set, the getType() and getMarkerStyle()
			     will return the wrong shape
			*/
 
    void		setCenterPos(const Coord3&);
    Coord3		centerPos(bool displayspace=false) const;
   
    void		setScreenSize(const float);
    			/*!<If a nonzero value is given, the object will
			    try to have the same size (in pixels) on the screen
			    at all times. */
    float		getScreenSize() const;

    void		doFaceCamera(bool yn);
    			/*!<If true, the maker will always be rotated so the
			    same part of the marker always faces the camera. */
    bool		facesCamera() const;

    void		doRestoreProportions(bool yn);
			/*!<If true, the shape will make sure that the shape's
			    proportions is unchanged, regardless of previous
			    scalings in the scenegraph. */
    bool		restoresProportions() const;		

    void		setRotation(const Coord3&,float);
    void		setDirection(const ::Sphere& d)	{ direction = d; }
    const ::Sphere&	getDirection() const		{ return direction; }

    void		setTransformation( Transformation* );
    Transformation*	getTransformation();
    
    int			usePar(const IOPar&);
    void		fillPar(IOPar&,TypeSet<int>&) const;

protected:
			~Marker();
    Transformation*	transformation;

    SoShapeScale*	markerscale;
    UTMPosition*	xytranslation;
    SoTranslation*	translation;
    SoNode*		shape;
    SoRotation*		rotation;

    MarkerStyle3D	markerstyle;

    ::Sphere		direction;
    void		setArrowDir(const ::Sphere&);

    static const char*  centerposstr;
};

};


#endif
