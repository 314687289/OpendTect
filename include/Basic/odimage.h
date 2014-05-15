#ifndef odimage_h
#define odimage_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          August 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "color.h"

namespace OD
{

/*!
\brief Class for Red, Green, Blue image.
*/

mExpClass(Basic) RGBImage
{
public:
    virtual		~RGBImage()			{}

    virtual char	nrComponents() const		= 0;
			/*!<\retval 1 grayscale
			    \retval 2 grayscale+alpha
			    \retval 3 rgb
			    \retval 4 rgb+alpha */
    virtual bool	hasAlpha() const;
    virtual bool	setSize(int,int)		= 0;
    virtual int		getSize(bool xdir) const	= 0;
    virtual Color	get(int,int) const		= 0;
    virtual bool	set(int,int,const Color&)	= 0;
	    
    virtual int		bufferSize() const;
    virtual void	fill(unsigned char*) const;
			/*!Fills array with content. Each
			   pixel's components are the fastest
			   dimension, slowest is xdir. Caller
			   must ensure sufficient mem is
			   allocated. */
    virtual bool	put(const unsigned char*,bool xdir_slowest=true,
			    bool with_opacity=false);
			/*!Fills image with data from array.param xdir_slowest 
			   False if ydir is the slowest dimension, param 
			   with_opacity If true, eventual 4th component will be
			   treated as a opacity instead of a transparency.*/
   virtual bool		blendWith(const RGBImage& sourceimage,
				  bool blendequaltransparency = false,
				  bool with_opacity=false);
			/*! <Blends image with another image of same size. The
			     provided images' transparency will be used to blend 
			     the two images proportionally.
			     \param blendequaltransparency if false,the color 
			     will be not blended when sourceimage has same 
			     transparency as this image has. 
			     \param with_opacity if true,eventual 4th component 
			     will be treated as a opacity instead of a 
			     transparency.
			  */
    virtual bool	putFromBitmap(const unsigned char* bitmap, 
				      const unsigned char* mask = 0);
    
    virtual const unsigned char*	getData() const		{ return 0; }
    virtual unsigned char*		getData() 		{ return 0; }
};

};

#endif

