#ifndef viswell_h
#define viswell_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id: viswell.h,v 1.17 2007-05-14 12:06:53 cvsraman Exp $
________________________________________________________________________

-*/


#include "visobject.h"

class Color;
class Coord3;
class Coord3Value;
class IOPar;
class LineStyle;
class SoPlaneWellLog;
template <class T> class Interval;

class SoSwitch;

namespace visBase
{
class DrawStyle;
class PolyLine;
class DataObjectGroup;
class Text2;
class Transformation;

/*! \brief 
Base class for well display
*/

class Well : public VisualObjectImpl
{
public:
    static Well*		create()
    				mCreateDataObj(Well);

    void			setTrack(const TypeSet<Coord3>&);

    void			setLineStyle(const LineStyle&);
    const LineStyle&		lineStyle() const;

    void			setWellName(const char*,Coord3,Coord3);
    void			showWellTopName(bool);
    void			showWellBotName(bool);
    bool			wellTopNameShown() const;
    bool			wellBotNameShown() const;

    void			addMarker(const Coord3&,const Color&,
	    				  const char*);
    void			removeAllMarkers();
    void			setMarkerScreenSize(int);
    int				markerScreenSize() const;
    bool			canShowMarkers() const;
    void			showMarkers(bool);
    bool			markersShown() const;
    void			showMarkerName(bool);
    bool			markerNameShown() const;

    void			setLogData(const TypeSet<Coord3Value>&,
	    			           const char* lognm,
					   const Interval<float>& rg,
					   bool scale,int nr);
    void			setLogColor(const Color&,int);
    const Color&		logColor(int) const;
    void			clearLog(int);
    void			setLogLineWidth(float,int);
    float			logLineWidth(int) const;
    void			setLogWidth(int);
    int				logWidth() const;
    void			showLogs(bool);
    void			showLog(bool,int);
    bool			logsShown() const;
    void			showLogName(bool);
    bool			logNameShown() const;

    void			setDisplayTransformation(Transformation*);
    Transformation*		getDisplayTransformation();

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar& par);

    static const char*		linestylestr;
    static const char*		showwelltopnmstr;
    static const char*		showwellbotnmstr;
    static const char*		showmarkerstr;
    static const char*		markerszstr;
    static const char*		showmarknmstr;
    static const char*		showlogsstr;
    static const char*		showlognmstr;
    static const char*		logwidthstr;

protected:
    				~Well();

    PolyLine*			track;
    DrawStyle*			drawstyle;
    Text2*			welltoptxt;
    Text2*			wellbottxt;
    DataObjectGroup*		markergroup;
    SoSwitch*			markernmswitch;
    DataObjectGroup*		markernames;
    SoSwitch*			lognmswitch;
    Text2*			lognmleft;
    Text2*			lognmright;
    SoPlaneWellLog*		log;
    Transformation*		transformation;

    bool			showmarkers;
    int				markersize;
};


};

#endif
