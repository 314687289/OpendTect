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


namespace osgVolume { class Volume; class VolumeTile; class ImageLayer; }
namespace osg { class Switch; class Image; class TransferFunction1D; }


namespace visBase
{
    class Material;

mExpClass(visBase) VolumeRenderScalarField : public DataObject
{
public:

    static VolumeRenderScalarField*	create()
	                        	mCreateDataObj(VolumeRenderScalarField);

    void			useShading(bool yn);	// obsolete

    bool			turnOn(bool);
    bool			isOn() const;

    void			setScalarField(const Array3D<float>*,bool mine,
	    				       TaskRunner*);

    void			setColTabSequence( const ColTab::Sequence&,
	    					   TaskRunner* tr );
    const ColTab::Sequence&	getColTabSequence();

    void			setColTabMapperSetup(const ColTab::MapperSetup&,
	    					   TaskRunner* tr );
    const ColTab::Mapper&	getColTabMapper();

//    void			setBlendColor(const Color&);
//    const Color&		getBlendColor() const;
    const TypeSet<float>&	getHistogram() const;

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

    const char* 		writeVolumeFile(od_ostream&) const;
				//!<\returns 0 on success, otherwise errmsg

protected:
    				~VolumeRenderScalarField();

    void			makeColorTables();
    void			makeIndices(bool doset,TaskRunner*);
    void			clipData(TaskRunner*);

    void			updateVolumeSlicing();

//    unsigned char		dummytexture_;

    ColTab::Sequence		sequence_;
    ColTab::Mapper		mapper_;

    int				sz0_, sz1_, sz2_;
    unsigned char*		indexcache_;
    bool			ownsindexcache_;
    const ValueSeries<float>*	datacache_;
    bool			ownsdatacache_;
    TypeSet<float>		histogram_;
//    Color			blendcolor_;
    Material*			material_;
    bool			useshading_;

    osgVolume::VolumeTile*	osgvoltile_;
    osg::Switch*		osgvolroot_;
    osgVolume::Volume*		osgvolume_;
    osgVolume::ImageLayer*	osgimagelayer_;
    osg::Image*			osgvoldata_;
    osg::TransferFunction1D*	osgtransfunc_;

public:
    static bool			isShadingSupported();
    void			allowShading(bool yn);
    bool			usesShading() const;
};

}

#endif

