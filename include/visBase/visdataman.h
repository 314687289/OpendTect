#ifndef visdataman_h
#define visdataman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "sets.h"
#include "factory.h"
#include "callback.h"

namespace osg { class Node; }

class IOPar;

class SoPath;
class SoNode;

namespace visBase
{
class DataObject;
class SelectionManager;
class Factory;


/*!\brief


*/

mExpClass(visBase) DataManager : public CallBacker
{
public:
    			DataManager();
    virtual		~DataManager();

    const char*		errMsg() const;

    void		getIds(const SoPath*,TypeSet<int>&) const;
    			/*!< Gets the ids from lowest level to highest
			     (i.e. scene ) */

    void		getIds(const std::type_info&,TypeSet<int>&) const;
    int			highestID() const;

    DataObject*		getObject(int id);
    const DataObject*	getObject(int id) const;
    DataObject*		getObject(const SoNode*);
    const DataObject*	getObject(const SoNode*) const;

    int			nrObjects() const;
    DataObject*		getIndexedObject(int idx);
    const DataObject*	getIndexedObject(int idx) const;

    SelectionManager&	selMan() { return selman_; }

    Notifier<DataManager>	removeallnotify;

    mDefineFactoryInClass( DataObject, factory );
protected:

    friend class	DataObject;
    void		addObject( DataObject* );
    void		removeObject( DataObject* );

    ObjectSet<DataObject>	objects_;

    int				freeid_;
    SelectionManager&		selman_;
    BufferString		errmsg_;

    static const char*		sKeyFreeID();
    static const char*		sKeySelManPrefix();
};

mGlobal(visBase) DataManager& DM();

};


#endif

