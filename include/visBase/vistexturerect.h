#ifndef vistexturerect_h
#define vistexturerect_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id$
________________________________________________________________________


-*/


#include "visbasemod.h"
#include "visobject.h"
#include "vistransform.h"

class CubeSampling;

namespace osgGeo { class TexturePlaneNode; }

namespace visBase
{

class TextureChannels;

/*!\brief
    A TextureRectangle is a Rectangle with a datatexture.  The data is set via setData.
*/

mExpClass(visBase) TextureRectangle : public VisualObjectImpl
{
public:
    static TextureRectangle*	create()
				mCreateDataObj(TextureRectangle);

    void			setTextureChannels(visBase::TextureChannels*);
    void			setCenter(const Coord3& center);
    void			setWidth(const Coord3& width);
    				//!<One dim must be 0
    Coord3			getWidth() const;
    Coord3			getCenter() const;

    void			swapTextureAxes(bool yn=true);
    bool			areTextureAxesSwapped() const;
    
    void			setDisplayTransformation(const mVisTrans*);

protected:
    				~TextureRectangle();
    
    RefMan<const mVisTrans>	displaytrans_;

    osgGeo::TexturePlaneNode*	textureplane_;
    RefMan<TextureChannels>	channels_;
};

};

#endif

