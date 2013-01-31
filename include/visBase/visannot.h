#ifndef visannot_h
#define visannot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id$
________________________________________________________________________


-*/


#include "visbasemod.h"
#include "visosg.h"
#include "visobject.h"
#include "color.h"
#include "cubesampling.h"
#include "position.h"
#include "sets.h"

class AxisInfo;
class FontData;
class Color;

namespace osg
{
    class Geode;
    class Array;
    class Group;
    class Geometry;
    class Vec3f;
}

namespace osgGeo
{
    class OneSideRenderNode;
}

namespace visBase
{
class Text2;
class DataObjectGroup;

/*!\brief
    Annotation is a simple wireframe box with text on its
    axis.
*/

mExpClass(visBase) Annotation : public VisualObjectImpl
{
public:
    static Annotation*		create()
				mCreateDataObj(Annotation);

    void			showText(bool yn);
    bool			isTextShown() const;

    void			showScale(bool yn);
    bool			isScaleShown() const;

    bool			canShowGridLines() const;
    void			showGridLines(bool yn);
    bool			isGridLinesShown() const;
    
    const FontData&		getFont() const;
    void			setFont(const FontData&);

    void			setCubeSampling(const CubeSampling&);

    void			setAnnotScale(int dim,int scale);

    void			setText( int dim, const char * );
    void			fillPar( IOPar& ) const;
    int				usePar( const IOPar& );
    
    void			setDisplayTransformation(const mVisTrans*);

protected:
    				~Annotation();
    void			initGridLines();
    void			updateGridLines();
    void			updateTextPos();
    void			updateTextColor(CallBacker*);
    void			getAxisCoords(int,osg::Vec3f&,osg::Vec3f&) const;
    void			setCorner( int, float, float, float );
    
    int				annotscale_[3];

    CubeSampling		cs_;
    
    osg::Geometry*		box_;
    
    osg::Array*			gridlinecoords_;
            
    osg::Geode*			geode_;
    osgGeo::OneSideRenderNode*	gridlines_;
    RefMan<Text2>		axisnames_;
    RefMan<Text2>		axisannot_;
    
    const mVisTrans*		displaytrans_;
        
    Color			annotcolor_;

    static const char*		textprefixstr();
    static const char*		cornerprefixstr();
    static const char*		showtextstr();
    static const char*		showscalestr();
};

};

#endif

