/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: emmanager.cc,v 1.26 2003-11-24 08:39:52 kristofer Exp $";

#include "emmanager.h"

#include "ctxtioobj.h"
#include "emfaulttransl.h"
#include "emhistory.h"
#include "emhorizontransl.h"
#include "emsticksettransl.h"
#include "emobject.h"
#include "emsurfaceiodata.h"
#include "errh.h"
#include "executor.h"
#include "iodir.h"
#include "ioman.h"
#include "ptrman.h"


EM::EMManager& EM::EMM()
{
    static PtrMan<EMManager> emm = 0;

    if ( !emm ) emm = new EM::EMManager;
    return *emm;
}

EM::EMManager::EMManager()
    : history_( *new EM::History(*this) )
{
    init();
}


EM::EMManager::~EMManager()
{
    deepErase( objects );
    delete &history_;
}


const EM::History& EM::EMManager::history() const
{ return history_; }


EM::History& EM::EMManager::history()
{ return history_; }


void EM::EMManager::init()
{ } 


EM::ObjectID EM::EMManager::add( EM::EMManager::Type type, const char* name )
{
    PtrMan<CtxtIOObj> ctio;
    if ( type==EM::EMManager::Hor )
	ctio = mMkCtxtIOObj(EMHorizon);
    else if ( type==EM::EMManager::Fault )
	ctio = mMkCtxtIOObj(EMFault);
    else if ( type==EMManager::StickSet )
	ctio = mMkCtxtIOObj(EMStickSet);

    if ( !ctio ) return -1;

    ctio->ctxt.forread = false;
    ctio->ioobj = 0;
    ctio->setName( name );
    ctio->fillObj();
    if ( !ctio->ioobj ) return -1;

    EMObject* obj = EM::EMObject::create( *ctio->ioobj, *this );
    if ( !obj )
	{ delete ctio->ioobj; return -1; }

    objects += obj;
    refcounts += 0;

    PtrMan<Executor> saver = obj->saver();
    if ( saver )
	saver->execute();

    delete ctio->ioobj;
    return obj->id();
} 


EM::EMObject* EM::EMManager::getObject( const EM::ObjectID& id )
{
    const EM::ObjectID objid = multiID2ObjectID(id);
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id()==objid )
	    return objects[idx];
    }

    return 0;
}


const EM::EMObject* EM::EMManager::getObject( const EM::ObjectID& id ) const
{ return const_cast<EM::EMManager*>(this)->getObject(id); }


EM::ObjectID EM::EMManager::multiID2ObjectID( const MultiID& id )
{ return id.leafID(); }


void EM::EMManager::removeObject( const EM::ObjectID& id )
{
    const EM::ObjectID objid = multiID2ObjectID(id);
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id() == objid )
	{
	    delete objects[idx];
	    objects.remove( idx );
	    refcounts.remove( idx );
	    return;
	}
    }
}


void EM::EMManager::ref( const EM::ObjectID& id )
{
    const EM::ObjectID objid = multiID2ObjectID(id);
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id() == objid )
	{
	    refcounts[idx]++;
	    return;
	}
    }

    pErrMsg("Reference of id does not exist");
}


void EM::EMManager::unRef( const EM::ObjectID& id )
{
    const EM::ObjectID objid = multiID2ObjectID(id);
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id() == objid )
	{
	    if ( !refcounts[idx] )
		pErrMsg("Un-refing object that is not reffed");

	    refcounts[idx]--;
	    if ( !refcounts[idx] )
		removeObject( id );

	    return;
	}
    }

    pErrMsg("Reference of id does not exist");
}


void EM::EMManager::unRefNoDel( const EM::ObjectID& id )
{
    const EM::ObjectID objid = multiID2ObjectID(id);
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id() == objid )
	{
	    if ( !refcounts[idx] )
		pErrMsg("Un-refing object that is not reffed");

	    refcounts[idx]--;
	    return;
	}
    }

    pErrMsg("Reference of id does not exist");
}

/*
void EM::EMManager::addObject( EM::EMObject* obj )
{
    if ( !obj ) return;
    objects += obj;
    refcounts += 0;
}
*/


EM::EMObject* EM::EMManager::getTempObj( EM::EMManager::Type type )
{
    EMObject* res = 0;
    if ( type==EM::EMManager::Hor )
	res = new EM::Horizon( *this, -1 );
    else if ( type==EM::EMManager::Fault )
	res = new EM::Fault( *this, -1 );

    return res;
}


Executor* EM::EMManager::load( const MultiID& mid,
			       const SurfaceIODataSelection* iosel )
{
    EM::ObjectID id = multiID2ObjectID(mid);
    EMObject* obj = getObject( id );
   
    if ( !obj )
    {
	PtrMan<IOObj> ioobj = IOM().get( mid );
	if ( !ioobj ) return 0;

	obj = EM::EMObject::create( *ioobj, *this );
	if ( obj )
	{
	    objects += obj;
	    refcounts += 0;
	}
    }

    mDynamicCastGet(EM::Surface*,surface,obj)
    if ( surface )
	return surface->loader(iosel);
    else if ( obj )
	return obj->loader();

    return 0;
}


void EM::EMManager::getSurfaceData( const MultiID& id,
				    EM::SurfaceIOData& sd )
{
    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj ) return;

    const char* grpname = ioobj->group();
    if ( !strcmp( grpname, EMHorizonTranslatorGroup::keyword ))
    {
	PtrMan<EMHorizonTranslator> tr = mTranslCreate(EMHorizon,mDGBKey);
	if ( !tr->startRead( *ioobj ) )
	    return;

	const EM::SurfaceIOData& newsd = tr->selections().sd;
	sd.rg = newsd.rg;
	deepCopy( sd.patches, newsd.patches );
	deepCopy( sd.valnames, newsd.valnames );
    }
}
