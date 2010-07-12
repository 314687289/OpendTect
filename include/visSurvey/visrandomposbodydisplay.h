#ifndef visrandomposbodydisplay_h
#define visrandomposbodydisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		January 2009
 RCS:		$Id: visrandomposbodydisplay.h,v 1.6 2010-07-12 22:52:41 cvskris Exp $
________________________________________________________________________


-*/

#include "emposid.h"
#include "visobject.h"
#include "vissurvobj.h"

namespace visBase { class RandomPos2Body; class Transformation; }
namespace EM { class RandomPosBody; }


namespace visSurvey
{
class Scene;

/*!\brief used for displaying a set of random picks in xyz coordinate.*/

mClass RandomPosBodyDisplay : public visBase::VisualObjectImpl,
       			      public visSurvey::SurveyObject
{
public:

    static RandomPosBodyDisplay* create()
				mCreateDataObj(RandomPosBodyDisplay);

    MultiID			getMultiID() const;
    bool			isInlCrl() const	{ return false; }

    bool			hasColor() const	{ return true; }
    Color			getColor() const;
    void			setColor(Color);
    bool			allowMaterialEdit() const { return true; }
    NotifierAccess*		materialChange();

    void			setDisplayTransformation(mVisTrans*);
    mVisTrans*			getDisplayTransformation();
    void			setRightHandSystem(bool);

    bool			setVisBody(visBase::RandomPos2Body*);
    				//!<Creates an EMObject for it.
    bool			setEMID(const EM::ObjectID&);
    EM::ObjectID		getEMID() const;
    EM::RandomPosBody*		getEMBody() const	{ return embody_; }

    const char*			errMsg() const { return errmsg_.str(); }
protected:

    static const char*		sKeyPSEarthModelID()	{ return "EM ID"; }
    virtual			~RandomPosBodyDisplay();
    
    void			updateVisFromEM();
    virtual void		fillPar(IOPar&,TypeSet<int>& saveids) const;
    virtual int			usePar(const IOPar&);

    visBase::Transformation*	transform_;
    visBase::RandomPos2Body*	displaybody_;
    EM::RandomPosBody*		embody_;
};

};


#endif
