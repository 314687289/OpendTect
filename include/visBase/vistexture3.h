#ifndef vistexture3_h
#define vistexture3_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vistexture3.h,v 1.6 2004-05-06 21:44:35 kristofer Exp $
________________________________________________________________________


-*/

#include "vistexture.h"

class SoTexture3;
class SoGroup;
template <class T> class Array3D;

namespace visBase
{

/*!\brief

*/

class Texture3 : public Texture
{
public:
    static Texture3*	create()
			mCreateDataObj( Texture3 );

    void		setTextureSize(int, int, int );
    int			getTextureSize(int dim) const;

    void		setData( const Array3D<float>*, DataType=Color );
    			/*!< Sets data to texture.
			\param sel=Color	Sets color on texture
			\param sel=Transperancy Sets transperencydata,
						colortable transperancy will
						be overridden.
			\param sel=Hue		The hue of the colortable is
						multiplied with the data.
			\param sel=Saturation	The saturation of the
						colortable is multiplied with
						the data.
			\param sel=Brightness	The brightness of the
						colortable is multiplied with
						the data.
			*/

protected:
    			~Texture3();

    unsigned char*	getTexturePtr();
    void		finishEditing();

    SoTexture3*		texture;
    int			x0sz, x1sz, x2sz;
};

};

#endif
