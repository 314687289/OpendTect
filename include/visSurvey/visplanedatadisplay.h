#ifndef visplanedatadisplay_h
#define visplanedatadisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visplanedatadisplay.h,v 1.62 2005-12-21 12:13:25 cvshelene Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"

class CubeSampling;
template <class T> class Array2D;

namespace visBase { class TextureRect; class VisColorTab; };
namespace Attrib { class SelSpec; class ColorSelSpec; class DataCubes; }

namespace visSurvey
{

class Scene;

/*!\brief Used for displaying an inline, crossline or timeslice.

    A PlaneDataDisplay object is the front-end object for displaying an inline,
    crossline or timeslice.  Use <code>setType(Type)</code> for setting the 
    requested orientation of the slice.
*/

class PlaneDataDisplay :  public visBase::VisualObject,
			  public visSurvey::SurveyObject
{
public:

    bool			isInlCrl() const { return true; }

    enum Type			{ Inline, Crossline, Timeslice };

    static PlaneDataDisplay*	create()
				mCreateDataObj(PlaneDataDisplay);

    void			setType(Type type);
    Type			getType() const { return type; }

    void			setGeometry(bool manip=false,bool init_=false);
    void			setRanges(const StepInterval<float>&,
					  const StepInterval<float>&,
					  const StepInterval<float>&,
					  bool manip=false);
    				//!< Sets the maximum range in each direction

    void			setOrigo(const Coord3&);
    void			setWidth(const Coord3&);

    void			resetDraggerSizes( float appvel );
    				//!< Should be called when appvel has changed

    bool			canDuplicate() const		{ return true; }
    SurveyObject*		duplicate() const;
    
    void			showManipulator(bool);
    bool			isManipulatorShown() const;
    bool			isManipulated() const;
    bool			canResetManipulation() const	{ return true; }
    void			resetManipulation();
    void			acceptManipulation();
    BufferString		getManipulationString() const;
    NotifierAccess*		getManipulationNotifier(){return &manipulating;}
    NotifierAccess*		getMovementNotification(){return &moving;}
    BufferString		getManipulationPos() const;

    bool			allowMaterialEdit() const	{ return true; }
    void			setMaterial(visBase::Material*);
    const visBase::Material*	getMaterial() const;
    visBase::Material*		getMaterial();

    int				nrResolutions() const;
    int				getResolution() const;
    void			setResolution(int);

    int				getAttributeFormat() const 	{ return 0; }

    bool			hasColorAttribute() const	{ return true; }
    const Attrib::SelSpec*	getSelSpec() const;
    void			setSelSpec(const Attrib::SelSpec&);
    void			setColorSelSpec(const Attrib::ColorSelSpec&);
    const Attrib::ColorSelSpec*	getColorSelSpec() const;
    const TypeSet<float>*	getHistogram() const;
    int				getColTabID() const;

    CubeSampling		getCubeSampling() const;
    void			setCubeSampling(CubeSampling);
    bool			setDataVolume( bool color,
	    				       const Attrib::DataCubes* );
    const Attrib::DataCubes*	getCacheVolume(bool color) const;
   
    bool			canHaveMultipleTextures() const { return true; }
    int				nrTextures() const;
    void			selectTexture(int);

    void			turnOn(bool);
    bool			isOn() const;

    void			getMousePosInfo(const visBase::EventInfo&,
	    					const Coord3&,
	    					float& val,
	    					BufferString& info) const;

    SoNode*			getInventorNode();

    virtual void		fillPar( IOPar&, TypeSet<int>& ) const;
    virtual int			usePar( const IOPar& );

    virtual float		calcDist( const Coord3& ) const;
    virtual float		maxDist() const;
    virtual bool		allowPicks() const		{ return true; }

    bool			setDataTransform( ZAxisTransform* );

protected:
				~PlaneDataDisplay();

    void			setUpConnections();
    void			setTextureRect(visBase::TextureRect*);
    void			setData(const Attrib::DataCubes*, int datatype);
    void			zScaleChanged(CallBacker*);
    void			manipChanged(CallBacker*);
    void			coltabChanged(CallBacker*);
    CubeSampling		getCubeSampling(bool manippos) const;
    void			checkCubeSampling(const CubeSampling&);

    visBase::TextureRect*	trect;

    Type			type;
    Attrib::SelSpec&		as;
    Attrib::ColorSelSpec&	colas;

    const Attrib::DataCubes*	cache;
    const Attrib::DataCubes*	colcache;

    BinID			curicstep;
    float			curzstep;
    ZAxisTransform*		datatransform;
    int				datatransformvoihandle;

    static const char*		trectstr;
    Notifier<PlaneDataDisplay>	manipulating;
    Notifier<PlaneDataDisplay>	moving;
};

};


#endif
