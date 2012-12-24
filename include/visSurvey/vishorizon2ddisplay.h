#ifndef vishorizon2ddisplay_h
#define vishorizon2ddisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          May 2004
 RCS:           $Id$
________________________________________________________________________


-*/

#include "vissurveymod.h"
#include "emposid.h"
#include "multiid.h"
#include "visemobjdisplay.h"

namespace visBase { class PolyLine3D; class PointSet;  }
namespace EM { class Horizon2D; }
class ZAxisTransform;

namespace visSurvey
{

class Seis2DDisplay;

mClass(visSurvey) Horizon2DDisplay : public EMObjectDisplay
{
public:
    static Horizon2DDisplay*	create()
				mCreateDataObj(Horizon2DDisplay);
    void			setDisplayTransformation(const mVisTrans*);

    void			getMousePosInfo(const visBase::EventInfo& e,
	    					IOPar& i ) const
				{ return EMObjectDisplay::getMousePosInfo(e,i);}
    virtual void		getMousePosInfo(const visBase::EventInfo&,
						Coord3&,
						BufferString& val,
					       	BufferString& info) const;
    void			setLineStyle(const LineStyle&);

    EM::SectionID		getSectionID(int visid) const;
    TypeSet<EM::SectionID>	getSectionIDs() const{ return sids_; }

    bool			setZAxisTransform(ZAxisTransform*,TaskRunner*);

    const visBase::PointSet*	getPointSet(const EM::SectionID&) const;
    const visBase::PolyLine3D*	getLine(const EM::SectionID&) const;
    void			doOtherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,
				    int whichobj );

protected:
    friend			class Horizon2DDisplayUpdater;
    				~Horizon2DDisplay();
    void			removeSectionDisplay(const EM::SectionID&);
    bool			addSection(const EM::SectionID&,TaskRunner*);

    struct LineRanges
    {
	TypeSet<TypeSet<Interval<int> > >	trcrgs;
	TypeSet<TypeSet<Interval<float> > >	zrgs;
    };

    static bool			withinRanges(const RowCol&,float z,
					     const LineRanges& );
    void			updateSection(int idx,const LineRanges* lr=0);
					      
    void			emChangeCB(CallBacker*);
    bool			setEMObject(const EM::ObjectID&,TaskRunner*);

    void			otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,
				    int whichobj );
    void			updateLinesOnSections(
	    				const ObjectSet<const Seis2DDisplay>&);
    void			updateSeedsOnSections(
	    				const ObjectSet<const Seis2DDisplay>&);

    void			zAxisTransformChg(CallBacker*);

    void			fillPar(IOPar&) const;
    int				usePar(const IOPar&);

    ObjectSet<visBase::PolyLine3D>		lines_;
    ObjectSet<visBase::PointSet>		points_;
    TypeSet<EM::SectionID>			sids_;
};


};

#endif

