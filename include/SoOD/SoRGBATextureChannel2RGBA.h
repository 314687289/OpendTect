#ifndef SoRGBATextureChannel2RGBA_h
#define SoRGBATextureChannel2RGBA_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          Dec 2006
 RCS:           $Id: SoRGBATextureChannel2RGBA.h,v 1.1 2008-10-31 18:28:50 cvskris Exp $
________________________________________________________________________


-*/


#include "SoMFImage.h"
#include <Inventor/fields/SoMFBool.h>
#include <Inventor/fields/SoMFShort.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSubNode.h>

class SoFieldSensor;
class SoSensor;
class SoElement;
class SoState;

/*!
Reads any number of texture channels from the state, merges them with one
colorsequence per channel, and outputs 4 texture channels (RGBA) on the state.
*/


class SoRGBATextureChannel2RGBA : public SoNode
{ SO_NODE_HEADER(SoRGBATextureChannel2RGBA);
public:
    static		void initClass();
			SoRGBATextureChannel2RGBA();

    SoMFBool		enabled;

    const SbImage&	getRGBA(int rgba) const;
			/*!<\param rgba is 0 for red, 1 for green, 2 for
			           blue and 3 for opacity. */
protected:
			~SoRGBATextureChannel2RGBA();
    void		sendRGBA(SoState*);
    void		getTransparencyStatus( const SbImage* channels,
	    		    long size, int channelidx, char& fullyopaque,
			    char& fullytrans) const;
    void		computeRGBA( const SbImage* channels,
	    			     int start, int stop,
				     int firstchannel, int lastchannel );
    static void		fieldChangeCB( void* data, SoSensor* );

    void		GLRender(SoGLRenderAction*);

    SbImage		rgba_[4];
    char		ti_;
};

#endif
