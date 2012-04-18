#ifndef vislight_h
#define vislight_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vislight.h,v 1.13 2012-04-18 11:15:56 cvskris Exp $
________________________________________________________________________


-*/

#include "visdata.h"


class SoLight;
class SoLightModel;

namespace visBase
{

/*!\brief
Base class for all lights

*/

mClass Light : public DataObject
{
public:

    void		turnOn(bool);
    bool		isOn() const;

    void		setIntensity(float);
    			/*!< 0 = nada, 1 = full light */
    float		intensity() const;

    virtual void	fillPar( IOPar&, TypeSet<int>& ) const;
    virtual int		usePar( const IOPar& );
protected:
    			Light(SoLight* light_);
    virtual		~Light();

    SoLight*		light_;	
    bool		ison_;
    float		intensity_;

    static const char*	isonstr();
    static const char*	intensitystr();

    virtual SoNode*	gtInvntrNode();

};

/*!\brief
A point that illuminates light

*/

mClass PointLight : public Light
{
public:
    static PointLight*	create() mCreateDataObj( PointLight );

    void		setPosition(float,float,float);
    float		position(int dim) const;

    void		fillPar( IOPar&, TypeSet<int>& ) const;
    int			usePar( const IOPar& );

protected:
    static const char*	positionstr();

};


/*!\brief
A light in a certain direction from a position at an infinite distance

*/


mClass DirectionalLight : public Light
{
public:
    static DirectionalLight*	create() mCreateDataObj( DirectionalLight );

    void		setDirection(float,float,float);
    float		direction(int dim) const;

    void		fillPar( IOPar&, TypeSet<int>& ) const;
    int			usePar( const IOPar& );

protected:
    static const char*	directionstr();

};

/*!\brief


*/


mClass SpotLight : public Light
{
public:
    static SpotLight*	create() mCreateDataObj( SpotLight );

    void		setDirection(float,float,float);
    float		direction(int dim) const;
    
    void		setPosition(float,float,float);
    float		position(int dim) const;

    void		setConeAngle(float);
    			//!< In radians, from one side of the cone to the other
    float		coneAngle() const;

    void		setDropOffRate(float);
    			// 0=smooth, 1=sharp
    float		dropOffRate() const;

    void		fillPar( IOPar&, TypeSet<int>& ) const;
    int			usePar( const IOPar& );

protected:
    static const char*	directionstr();
    static const char*	positionstr();
    static const char*	coneanglestr();
    static const char*	dropoffratestr();
};


mClass LightModel : public DataObject
{
public:

    static LightModel*	create() 
			mCreateDataObj(LightModel);

    enum Type		{ BaseColor, Phong };
    void		setModel(Type);
    Type		getModel() const;

protected:

			~LightModel();
    SoLightModel*	lightmodel_;

    virtual SoNode*	gtInvntrNode();

};

} //visBase

#endif
