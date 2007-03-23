#ifndef vismpeseedcatcher_h
#define vismpeseedcatcher_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vismpeseedcatcher.h,v 1.14 2007-03-23 11:34:53 cvsjaap Exp $
________________________________________________________________________


-*/

#include "visobject.h"

#include "attribdatacubes.h"
#include "cubesampling.h"
#include "emposid.h"
#include "geomelement.h"


namespace Geometry { class ElementEditor; }
namespace MPE { class ObjectEditor; }
namespace EM { class EdgeLineSet; }
namespace Attrib { class SelSpec; class DataCubes; class Data2DHolder; }
namespace visBase { class Marker; class Dragger; }


namespace visSurvey
{

/*!\brief
*/
class EMObjectDisplay;


class MPEClickInfo
{
    friend class MPEClickCatcher;
public:
    				MPEClickInfo();

    bool			isLegalClick() const;

    bool                        isCtrlClicked() const; 
    bool                        isShiftClicked() const;
    bool                        isAltClicked() const;
    const EM::PosID&		getNode() const;
    const Coord3&		getPos() const;
    int				getObjID() const;
    const CubeSampling&		getObjCS() const;
    const Attrib::DataCubes*	getObjData() const;
    const Attrib::SelSpec*	getObjDataSelSpec() const;

    const MultiID&		getObjLineSet() const;
    const char*			getObjLineName() const;
    const Attrib::Data2DHolder*	getObjLineData() const;

protected:
    void			clear();

    void			setLegalClick(bool);

    void			setCtrlClicked(bool); 
    void			setShiftClicked(bool);
    void			setAltClicked(bool);
    void			setNode(const EM::PosID&);
    void			setPos(const Coord3&);
    void			setObjID(int);
    void			setObjCS(const CubeSampling&);
    void			setObjData(const Attrib::DataCubes*);
    void			setObjDataSelSpec(const Attrib::SelSpec*);

    void			setObjLineSet(const MultiID&);
    void			setObjLineName(const char*);
    void			setObjLineData(const Attrib::Data2DHolder*);

    bool				legalclick_;
    bool				ctrlclicked_;
    bool				shiftclicked_;
    bool				altclicked_;
    EM::PosID				clickednode_;
    Coord3				clickedpos_;
    int					clickedobjid_;
    CubeSampling			clickedcs_;
    RefMan<const Attrib::DataCubes>	attrdata_;
    const Attrib::SelSpec*		attrsel_;

    RefMan<const Attrib::Data2DHolder>	linedata_;
    MultiID				lineset_;
    BufferString			linename_;
};


class MPEClickCatcher : public visBase::VisualObjectImpl
{
public:
    static MPEClickCatcher*	create()
				mCreateDataObj(MPEClickCatcher);

    void			setSceneEventCatcher(visBase::EventCatcher*);
    void			setDisplayTransformation(mVisTrans*);

    mVisTrans*			getDisplayTransformation();

    Notifier<MPEClickCatcher>	click;

    const MPEClickInfo&		info() const;
    MPEClickInfo&		info();

    void			setTrackerType(const char*);
    static bool			isClickable(const char* trackertype,int visid);

protected:
				~MPEClickCatcher();
    void			clickCB(CallBacker*);

    void 			sendUnderlying2DSeis(
					const EMObjectDisplay*,
					const visBase::EventInfo&);
    void 			sendUnderlyingPlanes(
					const EMObjectDisplay*,
					const visBase::EventInfo&);

    visBase::EventCatcher*	eventcatcher_;
    visBase::Transformation*	transformation_;

    MPEClickInfo		info_;
    const char*			trackertype_;
};



};

#endif
