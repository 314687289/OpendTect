/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: visdataman.cc,v 1.32 2006-04-27 17:57:12 cvskris Exp $";

#include "visdataman.h"
#include "visdata.h"
#include "visselman.h"
#include "iopar.h"
#include "ptrman.h"
#include "errh.h"

#include "Inventor/SoPath.h"

namespace visBase
{

const char* DataManager::sKeyFreeID()		{ return "Free ID"; }
const char* DataManager::sKeySelManPrefix()	{ return "SelMan"; }

DataManager& DM()
{
    static DataManager* manager = 0;

    if ( !manager ) manager = new DataManager;

    return *manager;
}


DataManager::DataManager()
    : freeid_( 0 )
    , selman_( *new SelectionManager )
    , fact_( *new Factory )
    , removeallnotify( this )
{ }


DataManager::~DataManager()
{
    removeAll();
    delete &selman_;
    delete &fact_;
}


void DataManager::fillPar( IOPar& par, TypeSet<int>& storids ) const
{
    IOPar selmanpar;
    selman_.fillPar( selmanpar, storids );
    par.mergeComp( selmanpar, sKeySelManPrefix() );

    for ( int idx=0; idx<storids.size(); idx++ )
    {
	IOPar dataobjpar;
	const DataObject* dataobj = getObject( storids[idx] );
	if ( !dataobj ) continue;
	dataobj->fillPar( dataobjpar, storids );

	BufferString idstr = storids[idx];
	par.mergeComp( dataobjpar, idstr );
    }

    sort( storids );
    const int storedfreeid = storids.size() ? storids[storids.size()-1] + 1 : 0;
    par.set( sKeyFreeID(), storedfreeid );
}


bool DataManager::usePar( const IOPar& par )
{
    removeAll();

    if ( !par.get( sKeyFreeID(), freeid_ ))
	return false;

    TypeSet<int> lefttodo;
    for ( int idx=0; idx<par.size(); idx++ )
    {
	BufferString key = par.getKey( idx );
	char* ptr = strchr(key.buf(),'.');
	if ( !ptr ) continue;
	*ptr++ = '\0';
	const int id = atoi( key.buf() );
	if ( lefttodo.indexOf(id) < 0 ) lefttodo += id;
    }

    sort( lefttodo );

    ObjectSet<DataObject> createdobj;

    bool change = true;
    while ( lefttodo.size() && change )
    {
	change = false;
	for ( int idx=0; idx<lefttodo.size(); idx++ )
	{
	    PtrMan<IOPar> iopar = par.subselect( lefttodo[idx] );
	    if ( !iopar )
	    {
		lefttodo.remove( idx );
		idx--;
		change = true;
		continue;
	    }

	    const char* type = iopar->find( DataObject::sKeyType() );
	    DataObject* obj = factory().create( type );
	    if ( !obj ) { lefttodo.remove(idx); idx--; continue; }

	    int no = objects_.indexOf( obj );
	    obj->setID(lefttodo[idx]);  

	    int res = obj->usePar( *iopar );
	    if ( res==-1 )
		return false;
	    if ( res==0 )
	    {
		//Remove object			
		obj->ref();
		obj->unRef();
		continue;
	    }

	    createdobj += obj;
	    obj->ref();
	   
	    lefttodo.removeFast( idx );
	    idx--;
	    change = true;
	}
    }

    int maxid = -1;
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	if ( objects_[idx]->id()>maxid ) maxid=objects_[idx]->id();
    }

    freeid_ = maxid+1;

    for ( int idx=0;idx<createdobj.size(); idx++ )
	createdobj[idx]->unRefNoDelete();

    return change;
}


bool DataManager::removeAll(int nriterations)
{
    removeallnotify.trigger();

    bool objectsleft = false;
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	if ( !objects_[idx]->nrRefs() )
	{
	    objects_[idx]->ref();
	    objects_[idx]->unRef();
	    idx--;
	}
	else objectsleft = true;
    }

    if ( !objectsleft ) return true;
    if ( !nriterations )
    {
	pErrMsg("All objects not unreferenced");
	while ( objects_.size() )
	{
	    BufferString msg = "Forcing removal of ID: ";
	    msg += objects_[0]->id();
	    msg += objects_[0]->getClassName();
	    pErrMsg( msg );

	    while ( objects_.size() && objects_[0]->nrRefs() )
		objects_[0]->unRef();
	}

	return false;
    }

    return removeAll( nriterations-1 );
}


int DataManager::highestID() const
{
    int max = 0;

    const int nrobjects = objects_.size();
    for ( int idx=0; idx<nrobjects; idx++ )
    {
	if ( objects_[idx]->id()>max )
	    max = objects_[idx]->id();
    }

    return max;
}


DataObject* DataManager::getObject( int id ) 
{
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	if ( objects_[idx]->id()==id ) return objects_[idx];
    }

    return 0;
}


const DataObject* DataManager::getObject( int id ) const
{ return const_cast<DataManager*>(this)->getObject(id); }


void DataManager::addObject( DataObject* obj )
{
    if ( objects_.indexOf(obj)==-1 )
    {
	objects_ += obj;
	obj->setID(freeid_++);
    }
}


void DataManager::getIds( const SoPath* path, TypeSet<int>& res ) const
{
    res.erase();

    const int nrobjs = objects_.size();

    for ( int pathidx=path->getLength()-1; pathidx>=0; pathidx-- )
    {
	SoNode* node = path->getNode( pathidx );

	for ( int idx=0; idx<nrobjs; idx++ )
	{
	    const SoNode* objnode = objects_[idx]->getInventorNode();
	    if ( !objnode ) continue;

	    if ( objnode==node ) res += objects_[idx]->id();
	}
    }
}


void DataManager::getIds( const std::type_info& ti,
				   TypeSet<int>& res) const
{
    res.erase();

    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	if ( typeid(*objects_[idx]) == ti )
	    res += objects_[idx]->id();
    }
}

void DataManager::removeObject( DataObject* dobj )
{ objects_ -= dobj; }


}; //namespace
