#ifndef geeditorimpl_h
#define geeditorimpl_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: geeditorimpl.h,v 1.2 2005-01-14 13:11:55 kristofer Exp $
________________________________________________________________________

-*/

#include "geeditor.h"

namespace Geometry
{

class TrackPlane;


class ElementEditorImpl : public ElementEditor
{
public:
    		ElementEditorImpl( Element& elem,
			const Coord3& dir1d=Coord3::udf(),
			const Coord3& norm2d=Coord3::udf(),
			bool allow3d=false );
		~ElementEditorImpl();

    bool	mayTranslate1D( GeomPosID ) const;
    Coord3	translation1DDirection( GeomPosID ) const;

    bool	mayTranslate2D( GeomPosID ) const;
    Coord3	translation2DNormal( GeomPosID ) const;

    bool	mayTranslate3D( GeomPosID ) const;

protected:
    void	addedKnots(CallBacker*);

    Coord3	translate1ddir;
    Coord3	translation2dnormal;
    bool	maytranslate3d;
};


class BinIDElementEditor : public ElementEditorImpl
{
public:
	    BinIDElementEditor( Geometry::Element& elem)
		: ElementEditorImpl( elem,  Coord3(0,0,1) ) {}
};


class PlaneElementEditor : public ElementEditorImpl
{
public:
	    PlaneElementEditor( Element& elem, const Coord3& normal )
		: ElementEditorImpl( elem,  Coord3::udf(), normal ) {}
};
};

#endif

