#ifndef basemap_h
#define basemap_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Sep 2009
 RCS:		$Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "namedobj.h"
#include "threadlock.h"

namespace OD { class RGBImage; }

class MarkerStyle2D;
class LineStyle;

/*!Object that can be painted in a basemap. */


mExpClass(General) BaseMapObject : public NamedObject
{
public:

				BaseMapObject(const char* nm);

    virtual const char*		getType() const				= 0;

    Threads::Lock		lock_;
    virtual void		updateGeometry()			{}

    virtual void		setDepth(int val)	{ depth_ = val; }
    virtual int			getDepth() const	{ return depth_; }
				/*!<Determines what should be painted ontop of
				    what */

    virtual int			nrShapes() const;
    virtual const char*		getShapeName(int shapeidx) const;
    virtual void		getPoints(int shapeidx,TypeSet<Coord>&) const;
				/*!<Returns a number of coordinates that
				    may form a be connected or filled. */

    virtual void		setMarkerStyle(int idx,const MarkerStyle2D&) {}
    virtual const MarkerStyle2D* getMarkerStyle(int shapeidx) const { return 0;}

    virtual void		setLineStyle(int idx,const LineStyle&)	    {}
    virtual const LineStyle*	getLineStyle(int shapeidx) const { return 0; }
    virtual bool		fill(int shapeidx) const	{ return false;}
    virtual bool		close(int shapeidx) const	{ return false;}

    virtual const OD::RGBImage*	getImage(Coord& origin,Coord& p11) const;
				/*!<Returns image in xy plane. p11 is the
				    coordinate of the corner opposite of the
				    origin. */
    virtual const OD::RGBImage*	getPreview(int approxdiagonal) const;
				/*!<Returns a preview image that has
				    approximately the size of the specified
				    diagonal. */

    Notifier<BaseMapObject>	changed;

protected:

    int				depth_;

};


/*!Base class for a Basemap. */
mExpClass(General) BaseMap
{
public:

    virtual void		addObject(BaseMapObject*)		= 0;
				/*!<Object maintained by caller. Adding an
				    existing will trigger update */

    virtual void		removeObject(const BaseMapObject*)	= 0;

};


#endif

