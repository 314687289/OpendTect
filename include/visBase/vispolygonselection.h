#ifndef vispolygonselection_h
#define vispolygonselection_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		June 2008
 RCS:		$Id: vispolygonselection.h,v 1.8 2009-12-11 15:35:52 cvsjaap Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "selector.h"
#include "draw.h"
#include "thread.h"

class SoPolygonSelect;
class SoSeparator;
template <class T> class ODPolygon;

namespace visBase
{
class Material;
class DrawStyle;

/*!
Paints a polygon or a rectangle just in front of near-clipping plane driven
by mouse- movement. Once drawn, queries can be made whether points are
inside or outside the polygon.
*/

mClass PolygonSelection : public VisualObjectImpl
{
public:
    static PolygonSelection*	create()
				mCreateDataObj(PolygonSelection);

    enum			SelectionType { Off, Rectangle, Polygon };
    void			setSelectionType(SelectionType);
    SelectionType		getSelectionType() const;

    void			setLineStyle(const LineStyle&);
    const LineStyle&		getLineStyle() const;

    bool			hasPolygon() const;
    bool			isSelfIntersecting() const;
    bool			isInside(const Coord3&,
	    				 bool displayspace=false) const;

    char			includesRange(const Coord3& start,
	    				      const Coord3& stop,
					      bool displayspace ) const;
    				/*!< 0: projected box fully outside polygon
				     1: projected box partially outside polygon
				     2: projected box fully inside polygon
				     3: all box points behind projection plane
				     4: some box points behind projection plane
				*/

    void			setDisplayTransformation( Transformation* );
    Transformation*		getDisplayTransformation();

    static Notifier<PolygonSelection> polygonfinished;

protected:

    static void				polygonChangeCB(void*,SoPolygonSelect*);
    static void				paintStopCB(void*,SoPolygonSelect*);

					~PolygonSelection();

    Transformation*			transformation_;

    DrawStyle*				drawstyle_;
    mutable ODPolygon<double>*		polygon_;
    mutable Threads::ReadWriteLock	polygonlock_;

    SoPolygonSelect*			selector_;
};


mClass PolygonCoord3Selector : public Selector<Coord3>
{
public:
				PolygonCoord3Selector(const PolygonSelection&);
				~PolygonCoord3Selector();

    Selector<Coord3>*		clone() const;
    const char*			selectorType() const;
    bool			isOK() const;
    bool			hasPolygon() const;
    bool			includes(const Coord3&) const;
    bool			canDoRange() const	{ return true; }
    char			includesRange(const Coord3& start,
	    				      const Coord3& stop) const;

protected:
    bool			isEq(const Selector<Coord3>&) const;

    const PolygonSelection&	vissel_;
};

};


#endif
