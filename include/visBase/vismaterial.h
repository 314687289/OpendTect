#ifndef vismaterial_h
#define vismaterial_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "color.h"
#include "visnodestate.h"

namespace osg {
    class Material;
    class Array;
};

class IOPar;

namespace visBase
{

/*!\brief


*/

mExpClass(visBase) Material : public NodeState
{
public:
    			Material();

    Notifier<Material>	change;

    void		setFrom(const Material&);
   
    void		setPropertiesFrom(const Material&);
			/*!< set materials by input material's properties */
    void		setColors(const TypeSet<Color>&,
				  bool synchronizing = true);
			/*!< set material's od colors by input colors. */


    enum ColorMode	{ Ambient, Diffuse, Specular, Emission,
			  AmbientAndDiffuse, Off };

    void		setColorMode( ColorMode );
    ColorMode		getColorMode() const;

    void		setColor(const Color&,int=0);
    const Color&	getColor(int matnr=0) const;

    void		removeColor(int idx);

    void		setDiffIntensity(float,int=0);
			/*!< Should be between 0 and 1 */
    float		getDiffIntensity(int=0) const;

    void		setAmbience(float);
			/*!< Should be between 0 and 1 */
    float		getAmbience() const;

    void		setSpecIntensity(float);
			/*!< Should be between 0 and 1 */
    float		getSpecIntensity() const;

    void		setEmmIntensity(float);
			/*!< Should be between 0 and 1 */
    float		getEmmIntensity() const;

    void		setShininess(float);
			/*!< Should be between 0 and 1 */
    float		getShininess() const;

    void		setTransparency(float,int idx=0);
			/*!< Should be between 0 and 1 */
    float		getTransparency(int idx=0) const;

    int			usePar(const IOPar&);
    void		fillPar(IOPar&) const;

    int			nrOfMaterial() const;

    void		clear();
    
    const osg::Array*	getColorArray() const;
    osg::Array*		getColorArray();

protected:
			~Material();
    void		setMinNrOfMaterials(int);
    void		updateOsgColor(int,bool trigger = true);
    void		createOsgColorArray();
    void		synchronizingOsgColorArray();

    TypeSet<Color>	colors_;
    TypeSet<float>	diffuseintensity_;
    TypeSet<float>	transparency_;
    
    float		ambience_;
    float		specularintensity_;
    float		emmissiveintensity_;
    float		shininess_;

    osg::Material*	material_;
    osg::Array*		osgcolorarray_;

    static const char*	sKeyColor();
    static const char*	sKeyAmbience();
    static const char*	sKeyDiffIntensity();
    static const char*	sKeySpectralIntensity();
    static const char*	sKeyEmmissiveIntensity();
    static const char*	sKeyShininess();
    static const char*	sKeyTransparency();

    friend class	OsgColorArrayUpdator;
};

} // namespace visBase


#endif

