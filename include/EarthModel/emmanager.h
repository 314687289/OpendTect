#ifndef emmanager_h
#define emmanager_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "notify.h"
#include "factory.h"
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

/*!
\brief Manages the loaded/half loaded EM objects in OpendTect.
*/

mExpClass(EarthModel) EMManager : public CallBacker
{
public:
			EMManager();
			~EMManager();

    inline int		nrLoadedObjects() const	{ return objects_.size(); }
    inline int		size() const		{ return nrLoadedObjects(); }
    EM::ObjectID	objectID(int idx) const;
    bool		objectExists(const EMObject*) const;

    EMObject*		loadIfNotFullyLoaded(const MultiID&,TaskRunner* =0);
			/*!<If fully loaded, the loaded instance
			    will be returned. Otherwise, it will be loaded.
			    Returned object must be reffed by caller
			    (and eventually unreffed). */

    EMObject*		getObject(const ObjectID&);
    const EMObject*	getObject(const ObjectID&) const;
    EMObject*		createTempObject(const char* type);

    BufferString	objectName(const MultiID&) const;
			/*!<\returns the name of the object */
    const char*		objectType(const MultiID&) const;
			/*!<\returns the type of the object */

    ObjectID		getObjectID(const MultiID&) const;
			/*!<\note that the relationship between storage id
			     (MultiID) and EarthModel id (ObjectID) may change
			     due to "Save as" operations and similar. */
    MultiID		getMultiID(const ObjectID&) const;
			/*!<\note that the relationship between storage id
			     (MultiID) and EarthModel id (ObjectID) may change
			     due to "Save as" operations and similar. */

    void		burstAlertToAll(bool yn);

    void		removeSelected(const ObjectID&,const Selector<Coord3>&,
				       TaskRunner*);
    bool		readDisplayPars(const MultiID&,IOPar&) const;
    bool		writeDisplayPars(const MultiID&,const IOPar&) const;
    bool		getSurfaceData(const MultiID&,SurfaceIOData&,
				       uiString& errmsg) const;

    Notifier<EMManager>	addRemove;

protected:

    Undo&			undo_;

    ObjectSet<EMObject>		objects_;

    void		levelSetChgCB(CallBacker*);
    static const char*	displayparameterstr();

    bool		readParsFromDisplayInfoFile(const MultiID&,
						    IOPar&)const;
    bool		readParsFromGeometryInfoFile(const MultiID&,
						     IOPar&)const;

public:

    // Don't use unless you know what you are doing

    void		setEmpty();

    Executor*		objectLoader(const MultiID&,
				     const SurfaceIODataSelection* =0);
    Executor*		objectLoader(const TypeSet<MultiID>&,
				     const SurfaceIODataSelection* =0,
				     TypeSet<MultiID>* idstobeloaded =0);
		/*!< idstobeloaded are the ids for which the objects
			     will be actually loaded */

    EM::ObjectID	createObject(const char* type,const char* name);
			/*!< Creates a new object, saves it and loads it.
			     Removes any loaded object with the same name!  */

			/*Interface from EMObject to report themselves */
    void		addObject(EMObject*);
    void		removeObject(const EMObject*);

    Undo&		undo();
    const Undo&		undo() const;

};


mDefineFactory1Param( EarthModel, EMObject, EMManager&, EMOF );

mGlobal(EarthModel) EMManager& EMM();
mGlobal(EarthModel) bool canOverwrite(const MultiID&);

} // namespace EM

#endif
