#ifndef vispointsetdisplay_h
#define vispointsetdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		March 2009
 RCS:		$Id: vispointsetdisplay.h,v 1.8 2009-11-30 12:17:10 cvssatyaki Exp $
________________________________________________________________________


-*/

#include "color.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "vistransform.h"

class DataPointSet;

namespace visBase { class PointSet; }

namespace visSurvey
{


mClass PointSetDisplay : public visBase::VisualObjectImpl,
			 public visSurvey::SurveyObject
{
public:
    static PointSetDisplay*     create()
				mCreateDataObj(PointSetDisplay);
    				~PointSetDisplay();

    void			setPointSize(int);
    int				getPointSize() const;

    Color			getColor() const;
    void			setColor(Color);
    bool			hasColor() const 	{ return true; }

    void			update();
    bool			setDataPack(int);
    const DataPointSet*		getDataPack() const 	{ return data_; }
    void			setDisplayTransformation(mVisTrans*);
    mVisTrans*			getDisplayTransformation();

    void			removeSelection(const Selector<Coord3>&);
    bool			selectable()			{ return true; }
    bool			canRemoveSelecion()		{ return true; }
    bool			allowMaterialEdit() const	{ return true; }
protected:

    Color			color_;
    visBase::PointSet*		pointset_;
    DataPointSet*		data_;
};

};


#endif
