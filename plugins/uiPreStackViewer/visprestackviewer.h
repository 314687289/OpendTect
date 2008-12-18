#ifndef visprestackviewer_h
#define visprestackviewer_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		May 2007
 RCS:		$Id: visprestackviewer.h,v 1.21 2008-12-18 15:21:06 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "vissurvobj.h"
#include "visobject.h"
#include "iopar.h"

class IOObj;

namespace PreStack { class ProcessManager; }
namespace visBase 
{
    class DepthTabPlaneDragger;
    class FaceSet;
    class FlatViewer;
    class PickStyle;
};

namespace visSurvey 
{ 
    class PlaneDataDisplay; 
    class Seis2DDisplay;
};


namespace PreStackView
{

class PreStackViewer :	public visBase::VisualObjectImpl,
			public visSurvey::SurveyObject
{
public:

    static PreStackViewer*	create()
				mCreateDataObj( PreStackViewer );
    void			allowShading(bool yn);
    void			setMultiID(const MultiID& mid);
    BufferString		getObjectName() const;
    bool			isInlCrl() const 	{ return true; }
    bool			isOrientationInline() const;
    const Coord			getBaseDirection() const; 

    				//for 3D only at present
    bool			setPreProcessor(PreStack::ProcessManager*);
    DataPack::ID		preProcess();

    bool			is3DSeis() const;
    DataPack::ID		getDataPackID() const;

    visBase::FlatViewer*	flatViewer()	{ return flatviewer_; }
    PreStack::ProcessManager*	procMgr()	{ return preprocmgr_; }

    				//3D case
    bool			setPosition(const BinID&);
    const BinID&		getPosition() const;
    void			setSectionDisplay(visSurvey::PlaneDataDisplay*);
    const visSurvey::PlaneDataDisplay* getSectionDisplay() const;	
   
   				//2D case 
    const visSurvey::Seis2DDisplay*    getSeis2DDisplay() const;
    bool			setSeis2DData(const IOObj* ioobj); 
    bool			setSeis2DDisplay(visSurvey::Seis2DDisplay*,
	    					 int trcnr);
    void			setTraceNr(int trcnr);
    int				traceNr() const 	  { return trcnr_; }
    const char*			lineName();

    bool                        displayAutoWidth() const { return autowidth_; }
    void                        displaysAutoWidth(bool yn);
    bool                        displayOnPositiveSide() const {return posside_;}
    void                        displaysOnPositiveSide(bool yn);
    float                       getFactor() { return factor_; }
    void			setFactor(float scale);
    float                       getWidth() { return width_; }
    void			setWidth(float width);
    BinID			getBinID() { return bid_; }
    MultiID			getMultiID() { return mid_; }
    void			getMousePosInfo(const visBase::EventInfo&,
	    					const Coord3&,
				  		BufferString& val,
						BufferString& info) const;
    void			 otherObjectsMoved( 
	    				const ObjectSet<const SurveyObject>&, 
					int whichobj );

    void			fillPar(IOPar&, TypeSet<int>&) const;
    int				usePar(const IOPar&);

    static const char*		sKeyParent()	{ return "Parent"; }
    static const char*		sKeyFactor()	{ return "Factor"; }
    static const char*		sKeyWidth() 	{ return "Width"; }
    static const char*		sKeyAutoWidth() { return "AutoWidth"; }
    static const char*		sKeySide() 	{ return "ShowSide"; }

protected:
    					~PreStackViewer();
    void				setDisplayTransformation(mVisTrans*);
    void				dataChangedCB(CallBacker*);
    void				sectionMovedCB(CallBacker*);
    void				seis2DMovedCB(CallBacker*);
    bool				updateData();
    int					getNearTraceNr(const IOObj*,int) const;
    BinID				getNearBinID(const BinID& pos) const;

    void				draggerMotion(CallBacker*);
    void				finishedCB(CallBacker*);

    BinID				bid_;
    visBase::DepthTabPlaneDragger* 	planedragger_;
    visBase::FaceSet*                   draggerrect_;
    visBase::FlatViewer*		flatviewer_;
    visBase::Material*			draggermaterial_;
    visBase::PickStyle*			pickstyle_;
    PreStack::ProcessManager*		preprocmgr_;

    MultiID				mid_;
    visSurvey::PlaneDataDisplay*	section_;
    visSurvey::Seis2DDisplay*		seis2d_;
    int 				trcnr_;
    Coord				basedirection_;
    Coord				seis2dpos_;
    
    bool				posside_;
    bool				autowidth_;
    float				factor_;
    float				width_;
    Interval<float>			offsetrange_;
    Interval<float>			zrg_;
};

}; //namespace

#endif
