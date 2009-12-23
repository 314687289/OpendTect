#ifndef emhorizonpainter_h
#define emhorizonpainter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Mar 2009
 RCS:		$Id: emhorizonpainter.h,v 1.7 2009-12-23 04:28:11 cvsumesh Exp $
________________________________________________________________________

-*/

#include "callback.h"

#include "cubesampling.h"
#include "draw.h"
#include "emposid.h"
#include "flatview.h"
#include "multiid.h"
#include "position.h"


namespace EM
{
    
mClass EMObjPainterCallbackData
{
public:
    		    EMObjPainterCallbackData() 
		    {}

    ObjectID	    objid_;
    BufferString    name_;
    bool	    enabled_;
};
    
    
mClass HorizonPainter : public CallBacker
{
public:
    			HorizonPainter(FlatView::Viewer&);
			~HorizonPainter();

	mStruct HorizonInfo
    	{
		EM::ObjectID	id_;
		BufferString	name_;
		bool		lineenabled_;
		bool		seedenabled_;
    	};

    void		addHorizon(const MultiID&);
    void                addHorizon(const EM::ObjectID&);

    HorizonPainter::HorizonInfo* getHorizonInfo( const EM::ObjectID& );

    void                setHorizonIDs(const ObjectSet<MultiID>*);
    void                removeHorizon(const MultiID&);

    void		enableHorizonLine(const EM::ObjectID&,bool);
    void		enableHorizonSeed(const EM::ObjectID&,bool);

    void		setCubeSampling(const CubeSampling&,bool);

    void		setMarkerLineStyle(const LineStyle&);

    void		displayHorizon(const MultiID&,bool);
    bool		isDisplayed(const MultiID&) const;

    void		set2D(bool yn)		{ is2d_ = yn; }
    void		setLineName(const char*);
    TypeSet<int>&	getTrcNos()		{ return trcnos_; }
    TypeSet<float>&	getDistances()		{ return distances_; }

    CNotifier<HorizonPainter,EMObjPainterCallbackData>	horizonadded_;
    CNotifier<HorizonPainter,EMObjPainterCallbackData>	horizonremoved_;

protected:

    bool		addPolyLine(const EM::ObjectID&);
    void		changePolyLineColor(const EM::ObjectID&);
    void		changePolyLinePosition(const EM::ObjectID&,
	    				       const EM::PosID&);
    void		removeHorizon(int);
    void		removePolyLine(int);
    void		updateDisplay();
    void		repaintHorizon(const EM::ObjectID&);

    void		nrHorChangeCB(CallBacker*);
    virtual void	horChangeCB(CallBacker*);

    CubeSampling        cs_;
    Interval<float>     horrg_;

    LineStyle           markerlinestyle_;
    FlatView::Viewer&   viewer_;

    int			loadinghorcount_;
    EM::ObjectID	horidtoberepainted_;
    
    ObjectSet<HorizonInfo>  horizoninfos_;

    ObjectSet< ObjectSet<ObjectSet<FlatView::Annotation::AuxData> > > 	hormarkerlines_;

    ObjectSet<FlatView::Annotation::AuxData> horsmarkerseeds_;
    MarkerStyle2D	markerstyle_;

    bool		is2d_;
    const char*		linenm_;
    bool		isupdating_;
    TypeSet<int>	trcnos_;
    TypeSet<float>	distances_;    
};

}// namespace EM

#endif
