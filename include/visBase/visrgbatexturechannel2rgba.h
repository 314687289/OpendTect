#ifndef visrgbatexturechannel2rgba_h
#define visrgbatexturechannel2rgba_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		Oct 2008
 RCS:		$Id: visrgbatexturechannel2rgba.h,v 1.3 2008-12-04 13:45:59 cvskris Exp $
________________________________________________________________________


-*/

#include "vistexturechannel2rgba.h"
#include "coltabsequence.h"

class SoRGBATextureChannel2RGBA;

namespace visBase
{ 

/*!Converts texture channels with RGBA information to ... RGBA. 
Does also handle enable/disable of the channels. */


class RGBATextureChannel2RGBA : public TextureChannel2RGBA
{
public:
    static RGBATextureChannel2RGBA*	create()
				mCreateDataObj(RGBATextureChannel2RGBA);

    void			setEnabled(int ch,bool yn);
    bool			isEnabled(int ch) const;
    const ColTab::Sequence*	getSequence(int ch) const;

    bool			canUseShading() const	{ return false; }
    bool			usesShading() const	{ return false; }
    int				maxNrChannels() const	{ return 4; }
    int				minNrChannels() const	{ return 4; }

    bool			createRGBA(SbImage&) const;

protected:

    				~RGBATextureChannel2RGBA();
    SoNode*			getInventorNode();

    ColTab::Sequence		sequences_[4];

    SoRGBATextureChannel2RGBA*	converter_;
};


}; //namespace


#endif
