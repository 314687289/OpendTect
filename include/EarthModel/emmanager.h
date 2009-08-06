#ifndef emmanager_h
#define emmanager_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emmanager.h,v 1.48 2009-08-06 01:57:41 cvskris Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "bufstring.h"
#include "callback.h"
#include "factory.h"
#include "position.h"
#include "ptrman.h"
#include "ranges.h"
#include "multiid.h"
#include "emposid.h"

class Undo;
class IOObj;
class IOObjContext;
class TaskRunner;
class Executor;
class uiEMPartServer;

template <class T> class Selector;

namespace EM
{
class EMObject;
class SurfaceIOData;
class SurfaceIODataSelection;

/*!\brief


*/

mClass EMManager : public CallBacker
{
public:
			EMManager();
			~EMManager();

    void		empty();

    Undo&		undo();
    const Undo&		undo() const;

    int			nrLoadedObjects() const	{ return objects_.size(); }
    EM::ObjectID	objectID(int idx) const;
    Executor*		objectLoader(const MultiID&,
	    			     const SurfaceIODataSelection* =0);
    Executor*		objectLoader(const TypeSet<MultiID>&,
	    			     const SurfaceIODataSelection* =0);
    EMObject*		loadIfNotFullyLoaded(const MultiID&,TaskRunner* =0);
			/*!<If fully loaded, the loaded instance
			    will be returned. Otherwise, it will be loaded.
			    Returned object must be reffed by caller
			    (and eventually unreffed). */
    EM::ObjectID	createObject(const char* type,const char* name);
    			/*!< Creates a new object, saves it and loads it into
			     mem.
			     \note If an object already exist with that name,
			     it will be removed!! Check in advance with
			     findObject().
			*/

    Notifier<EMManager>				addRemove;
    CNotifier<EMManager,const ObjectID&>	syncGeomReq;

    void		sendRemovalSignal(const ObjectID&);
    BufferString	objectName(const MultiID&) const;
    			/*!<\returns the name of the object */
    const char*		objectType(const MultiID&) const;
    			/*!<\returns the type of the object */

    EMObject*		getObject(const ObjectID&);
    const EMObject*	getObject(const ObjectID&) const;

    EMObject*		createTempObject(const char* type);

    const char*		getSurfaceData(const MultiID&,SurfaceIOData&);
    			//!<\returns err msg or null if OK
    void		get2DHorizons(const MultiID& lineset,const char* linenm,
	    			      TypeSet<MultiID>&) const;

    			/*Interface from EMObject to report themselves */
    void		addObject(EMObject*);
    void		removeObject(const EMObject*);

    ObjectID		getObjectID(const MultiID&) const;
    			/*!<\note that the relationship between storage id 
			     (MultiID) and EarthModel id (ObjectID) may change
			     due to "Save as" operations and similar. */
    MultiID		getMultiID(const ObjectID&) const;
    			/*!<\note that the relationship between storage id 
			     (MultiID) and EarthModel id (ObjectID) may change
			     due to "Save as" operations and similar. */

    void		syncGeometry(const ObjectID&);
    void		burstAlertToAll(bool yn);

    void		sortedHorizonsList(TypeSet<MultiID>&,bool is2d) const;

    void		removeSelected(const ObjectID&,const Selector<Coord3>&,
	    			       TaskRunner*);
    
    IOPar*		getSurfacePars(const IOObj&) const;

    bool		readPars(const MultiID&,IOPar&) const;
    bool		writePars(const MultiID&,const IOPar&) const;

protected:
    Undo&			undo_;

    ObjectSet<EMObject>		objects_;
};


mDefineFactory1Param( EMObject, EMManager&, EMOF );

mGlobal EMManager& EMM();

}; // Namespace


#endif
