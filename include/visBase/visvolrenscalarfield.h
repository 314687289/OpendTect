#ifndef visvolrenscalarfield_h
#define visvolrenscalarfield_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          January 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "color.h"
#include "ranges.h"
#include "visdata.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "visosg.h"

class TaskRunner;
template <class T> class Array3D;
template <class T> class ValueSeries;


namespace osgVolume { class Volume; class VolumeTile; class ImageLayer;
		      class TransparencyProperty; }
namespace osg { class Switch; class Image; class TransferFunction1D; }
namespace osgGeo { class RayTracedTechnique; }



namespace visBase
{
    class Material;
    class TextureChannel2RGBA;

mExpClass(visBase) VolumeRenderScalarField : public DataObject
{
public:

    static VolumeRenderScalarField*	create()
	                        	mCreateDataObj(VolumeRenderScalarField);

    void			setChannels2RGBA(visBase::TextureChannel2RGBA*);
    TextureChannel2RGBA*	getChannels2RGBA();
    const TextureChannel2RGBA*	getChannels2RGBA() const;

    void			setScalarField(int attr,const Array3D<float>*,
					       bool mine,TaskRunner*);

    void			setColTabMapperSetup(int attr,
						     const ColTab::MapperSetup&,
						     TaskRunner* tr );
    const ColTab::Mapper&	getColTabMapper(int attr);

    const TypeSet<float>&	getHistogram(int attr) const;

    const char*			writeVolumeFile(int attr,od_ostream&) const;
				//!<\returns 0 on success, otherwise errmsg

    void			useShading(bool yn);

    bool			turnOn(bool);
    bool			isOn() const;

    void			setTexVolumeTransform(const Coord3& trans,
				    const Coord3& rotvec,double rotangle,
				    const Coord3& scale);
    void			setROIVolumeTransform(const Coord3& trans,
				    const Coord3& rotvec,double rotangle,
				    const Coord3& scale);
				/*!< Use these instead of parent transformation
				    node, because of normal rescaling issue at
				    fixed function technique! */

    bool			textureInterpolationEnabled() const;
    void			enableTextureInterpolation(bool);

    void			setMaterial(Material*);

				/*!< Auxiliary functions to be called to force
				processing of TextureChannel2RGBA changes */
    void			makeColorTables(int attr);
    void			enableAttrib(int attr,bool yn);
    void			swapAttribs(int attr0,int attr1);
    void			setAttribTransparency(int attr,unsigned char);

protected:
    				~VolumeRenderScalarField();

    void			makeIndices(int attr,bool doset,TaskRunner*);
    void			clipData(int attr,TaskRunner*);

    void			updateFragShaderType();
    void			updateVolumeSlicing();
    void			updateTransparencyRescaling();

    void			setDefaultRGBAValue(int channel);

    struct AttribData
    {
					AttribData();
					~AttribData();

	od_int64			totalSz() const;
	bool				isInVolumeCache() const;

	int				sz0_, sz1_, sz2_;
	ColTab::Mapper			mapper_;
	unsigned char*			indexcache_;
	int				indexcachestep_;
	bool				ownsindexcache_;
	const ValueSeries<float>*	datacache_;
	bool				ownsdatacache_;
	TypeSet<float>			histogram_;
    };

    ObjectSet<AttribData>		attribs_;

    TextureChannel2RGBA*		channels2rgba_;
    bool				isrgba_;

    Material*				material_;
    bool				useshading_;

    osgVolume::VolumeTile*		osgvoltile_;
    osg::Switch*			osgvolroot_;
    osgVolume::Volume*			osgvolume_;
    osgVolume::ImageLayer*		osgimagelayer_;
    osg::Image*				osgvoldata_;
    osg::TransferFunction1D*		osgtransfunc_;
    osgVolume::TransparencyProperty*	osgtransprop_;
    osgGeo::RayTracedTechnique*		raytt_;
};

}

#endif

