#ifndef mpeengine_h
#define mpeengine_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 RCS:           $Id: mpeengine.h,v 1.31 2006-05-29 15:12:55 cvsjaap Exp $
________________________________________________________________________

-*/

/*!\mainpage
MPE stands for Model, Predict, Edit and is where all tracking and editing
functions are located. The functionality is managed by the MPEEngine, and
a static inistanciation of that can be retrieved by MPE::engine().

*/

#include "bufstring.h"
#include "callback.h"
#include "color.h"
#include "cubesampling.h"
#include "emposid.h"
#include "trackplane.h"

class BufferStringSet;
class Executor;
class CubeSampling;
class MultiID;

namespace Attrib { class SelSpec; class DataCubes; }
namespace EM { class EMObject; };
namespace Geometry { class Element; };

namespace MPE
{

class EMTracker;
class TrackerFactory;
class EditorFactory;
class TrackPlane;
class ObjectEditor;

class Engine : public CallBacker
{
    friend Engine&		engine();

public:
    				Engine();
    virtual			~Engine();

    const CubeSampling&		activeVolume() const;
    void			setActiveVolume(const CubeSampling&);
    static CubeSampling		getDefaultActiveVolume();
    Notifier<Engine>		activevolumechange;

    const TrackPlane&		trackPlane() const;
    bool			setTrackPlane(const TrackPlane&,bool track);
    void			setTrackMode(TrackPlane::TrackMode);
    TrackPlane::TrackMode	getTrackMode()
    				{ return trackplane_.getTrackMode(); }
    Notifier<Engine>		trackplanechange;

    Notifier<Engine>		loadEMObject;
    MultiID			midtoload;

    bool			trackAtCurrentPlane();
    Executor*			trackInVolume();

    void			getAvailableTrackerTypes(BufferStringSet&)const;

    int				nrTrackersAlive() const;
    int				highestTrackerID() const;
    const EMTracker*		getTracker(int idx) const;
    EMTracker*			getTracker(int idx);
    int				getTrackerByObject(const EM::ObjectID&) const;
    int				getTrackerByObject(const char*) const;
    int				addTracker(EM::EMObject*);
    void			removeTracker(int idx);
    Notifier<Engine>		trackeraddremove;

    				/*Attribute stuff */
    void			getNeededAttribs(
				    ObjectSet<const Attrib::SelSpec>&) const;
    CubeSampling		getAttribCube(const Attrib::SelSpec&) const;
    				/*!< Returns the cube that is needed for
				     this attrib, given that the activearea
				     should be tracked. */
    const Attrib::DataCubes*	getAttribCache(const Attrib::SelSpec&) const;
    bool			setAttribData( const Attrib::SelSpec&,
					       const Attrib::DataCubes*);
    void			swapCacheAndItsBackup();

    				/*Editors */
    ObjectEditor*		getEditor(const EM::ObjectID&,bool create);
    void			removeEditor(const EM::ObjectID&);

    				/*Factories */
    void			addTrackerFactory(TrackerFactory*);
    void			addEditorFactory(EditorFactory*);

    const char*			errMsg() const;

    BufferString		setupFileName( const MultiID& ) const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:
    void			init();
    int				getFreeID();

    BufferString		errmsg_;
    CubeSampling		activevolume_;
    TrackPlane			trackplane_;

    ObjectSet<EMTracker>	trackers_;
    ObjectSet<ObjectEditor>	editors_;

    ObjectSet<const Attrib::DataCubes>	attribcache_;
    ObjectSet<Attrib::SelSpec>		attribcachespecs_;
    ObjectSet<const Attrib::DataCubes>	attribbackupcache_;
    ObjectSet<Attrib::SelSpec>		attribbackupcachespecs_;

    ObjectSet<TrackerFactory>	trackerfactories_;
    ObjectSet<EditorFactory>	editorfactories_;

    static const char*		sKeyNrTrackers(){ return "Nr Trackers"; }
    static const char*		sKeyObjectID()	{ return "ObjectID"; }
    static const char*		sKeyEnabled()	{ return "Is enabled"; }
    static const char*		sKeyTrackPlane(){ return "Track Plane"; }
};


void initStandardClasses();


};

#endif

