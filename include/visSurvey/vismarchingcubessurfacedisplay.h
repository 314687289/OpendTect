#ifndef vismarchingcubessurfacedisplay_h
#define vismarchingcubessurfacedisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "vissurveymod.h"
#include "attribsel.h"
#include "emposid.h"
#include "visobject.h"
#include "vissurvobj.h"


namespace Geometry { class ExplicitIndexedShape; }
namespace EM { class MarchingCubesSurface; class ImplicitBody; }
namespace visBase { class GeomIndexedShape; class MarchingCubesSurface; 
		    class Transformation; };


namespace visSurvey
{


mExpClass(visSurvey) MarchingCubesDisplay : public visBase::VisualObjectImpl,
			      public visSurvey::SurveyObject
{
public:
    static MarchingCubesDisplay*create()
				mCreateDataObj(MarchingCubesDisplay);

    MultiID			getMultiID() const;
    bool			isInlCrl() const	{ return true; }

    bool			hasColor() const	{ return true; }
    Color			getColor() const;
    void			setColor(Color);
    bool			allowMaterialEdit() const { return true; }
    NotifierAccess*		materialChange();

    void			useTexture(bool yn);
    bool			usesTexture() const;

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    bool			setVisSurface(visBase::MarchingCubesSurface*);
    				//!<Creates an EMObject for it.
    bool			setEMID(const EM::ObjectID&,TaskRunner*);
    EM::ObjectID		getEMID() const;

    const char*			errMsg() const { return errmsg_.str(); }

    SurveyObject::AttribFormat	getAttributeFormat(int) const;
    int				nrAttribs() const;
    bool			canAddAttrib(int) const;
    bool			canRemoveAttrib() const;
    bool			canHandleColTabSeqTrans(int) const;
    const ColTab::MapperSetup*	getColTabMapperSetup(int,int) const;

    void			setColTabMapperSetup(int,
	    				const ColTab::MapperSetup&,TaskRunner*);
    const ColTab::Sequence*	getColTabSequence(int) const;
    bool			canSetColTabSequence() const;
    void                	setColTabSequence(int,const ColTab::Sequence&,
	    				TaskRunner*);
    void                   	setSelSpec(int,const Attrib::SelSpec&);
    const Attrib::SelSpec*	getSelSpec(int attrib) const;
    void			setDepthAsAttrib(int);
    void			setIsoPatch(int);

    void			getRandomPos(DataPointSet&,TaskRunner*) const;
    void			setRandomPosData( int attrib,
	    				const DataPointSet*, TaskRunner*);

    void			displayIntersections(bool yn);
    bool			areIntersectionsDisplayed() const;

    bool			canRemoveSelection() const	{ return true; }
    void			removeSelection(const Selector<Coord3>&,
	    					TaskRunner*);    
    EM::MarchingCubesSurface*	getMCSurface() const { return emsurface_; }

protected:

    virtual			~MarchingCubesDisplay();
    bool			updateVisFromEM(bool onlyshape,TaskRunner*);
    virtual void		fillPar(IOPar&) const;
    virtual int			usePar(const IOPar&);
    void			materialChangeCB(CallBacker*);

    void			getMousePosInfo(const visBase::EventInfo& ei,
	    				IOPar& iop ) const;
    void			getMousePosInfo(const visBase::EventInfo&,
	     				Coord3& xyzpos, BufferString& val,
	     			    	BufferString& info) const;
    void			otherObjectsMoved(
	    				const ObjectSet<const SurveyObject>&,
					int whichobj);
    void			updateIntersectionDisplay();
    
    static const char*	sKeyEarthModelID()	{ return "EM ID"; }
    static const char*	sKeyAttribSelSpec()	{ return "Attrib SelSpec"; }
    static const char*	sKeyColTabMapper()	{ return "Coltab mapper"; }
    static const char*	sKeyColTabSequence()	{ return "Coltab sequence"; }   

    visBase::MarchingCubesSurface*		displaysurface_;
    EM::MarchingCubesSurface*			emsurface_;
    Attrib::SelSpec				selspec_;
    ObjectSet<DataPointSet>			cache_;

    EM::ImplicitBody*				impbody_;
    bool					displayintersections_;

    struct PlaneIntersectInfo
    {
						PlaneIntersectInfo();
						~PlaneIntersectInfo();
	visBase::GeomIndexedShape*		visshape_;
	Geometry::ExplicitIndexedShape*		shape_;
	
	int					planeid_;
	char					planeorientation_;
	float					planepos_;

	bool					computed_;
    };

    ObjectSet<PlaneIntersectInfo>		intsinfo_;
    visBase::Transformation*			model2displayspacetransform_;
};

};


#endif

