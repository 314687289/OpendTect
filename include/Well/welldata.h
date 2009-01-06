#ifndef welldata_h
#define welldata_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: welldata.h,v 1.14 2009-01-06 10:57:11 cvsranojay Exp $
________________________________________________________________________

-*/

#include "sets.h"
#include "position.h"
#include "namedobj.h"
#include "callback.h"

class IOPar;

namespace Well
{

class Track;
class LogSet;
class D2TModel;
class Marker;
class DisplayProperties;

/*!\brief Infomation about a certain well */

mClass Info : public ::NamedObject
{
public:

			Info( const char* nm )
			    : ::NamedObject(nm), surfaceelev(0)	{}

    void                fillPar(IOPar&) const;
    void                usePar(const IOPar&);

    BufferString	uwid;
    BufferString	oper;
    BufferString	state;
    BufferString	county;

    Coord		surfacecoord;
    float		surfaceelev;

    static const char*	sKeyuwid;
    static const char*	sKeyoper;
    static const char*	sKeystate;
    static const char*	sKeycounty;
    static const char*	sKeycoord;
    static const char*	sKeyelev;

};


/*!\brief The holder of all data concerning a certain well.
 
 Note that a well is not a POSC well in the sense that it describes the
 data for one well bore. Thus, a well has a single track.
 This may mean duplication when more well tracks share an upper part.

*/

mClass Data : public CallBacker
{
public:

				Data(const char* nm=0);
				~Data();

    const char*			name() const		{ return info_.name(); }
    const Info&			info() const		{ return info_; }
    Info&			info()			{ return info_; }
    const Track&		track() const		{ return track_; }
    Track&			track()			{ return track_; }
    const LogSet&		logs() const		{ return logs_; }
    LogSet&			logs()			{ return logs_; }
    const ObjectSet<Marker>&	markers() const		{ return markers_; }
    ObjectSet<Marker>&		markers()		{ return markers_; }
    const D2TModel*		d2TModel() const	{ return d2tmodel_; }
    D2TModel*			d2TModel()		{ return d2tmodel_; }
    void			setD2TModel(D2TModel*);	//!< becomes mine
    DisplayProperties&		displayProperties()	{ return disp_; }
    const DisplayProperties&	displayProperties() const { return disp_; }

    Notifier<Well::Data>	d2tchanged;
    Notifier<Well::Data>	markerschanged;
    Notifier<Well::Data>	dispparschanged;

protected:

    Info			info_;
    Track&			track_;
    LogSet&			logs_;
    D2TModel*			d2tmodel_;
    ObjectSet<Marker>		markers_;
    DisplayProperties&		disp_;

};

}; // namespace Well


/*!\mainpage Wells
 
 The OpendTect Well object is a single track, logs and markers bearing thing
 with some global data and possibly a Depth vs Time model attached.
 Therefore, it is not a POSC well in the sense that it describes the
 data for one well bore only. This may mean duplication when more well
 tracks share an upper part.

 The well track is a 3D line-segment geometry. The traditional description of
 the Z value in true vertical depth (TVD) breaks down miserably when
 horizontal well tracks (and worse) come into play. A much better single
 coordinate to describe the position in a well is the depth-along-hole (or
 more correctly distance-along-hole). This is why everything in this module
 is related to the DAH, not TVD. There are facilities to work with TVD, though.

 Further it seems to me that anyone familiar with wells, logs and that kind of
 concepts should find this module's object model fairly intuitive.

*/


#endif
