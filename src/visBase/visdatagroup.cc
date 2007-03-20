/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Jan 2002
 RCS:           $Id: visdatagroup.cc,v 1.7 2007-03-20 20:52:52 cvskris Exp $
________________________________________________________________________

-*/

#include "visdatagroup.h"
#include "visdataman.h"
#include "iopar.h"

#include <Inventor/nodes/SoSeparator.h>

mCreateFactoryEntry( visBase::DataObjectGroup );

namespace visBase
{

const char* DataObjectGroup::nokidsstr = "Number of Children";
const char* DataObjectGroup::kidprefix = "Child ";

DataObjectGroup::DataObjectGroup()
    : group_ ( 0 )
    , separate_( true )
{ }


DataObjectGroup::~DataObjectGroup()
{
    const int sz = size();

    for ( int idx=0; idx<sz; idx++ )
	objects_[idx]->unRef();

    group_->unref();
}


void DataObjectGroup::ensureGroup()
{
    if ( group_ ) return;
    group_ = createGroup();
    group_->ref();
}


SoGroup* DataObjectGroup::createGroup()
{
    return separate_ ? new SoSeparator : new SoGroup;
}


int DataObjectGroup::size() const
{ return objects_.size(); }


void DataObjectGroup::addObject( DataObject* no )
{
    ensureGroup();
    objects_ += no;
    group_->addChild( no->getInventorNode() );
    nodes_ += no->getInventorNode();

    no->ref();
}


void DataObjectGroup::setDisplayTransformation( Transformation* nt )
{
    for ( int idx=0; idx<objects_.size(); idx++ )
	objects_[idx]->setDisplayTransformation(nt);
}


Transformation* DataObjectGroup::getDisplayTransformation()
{
    for ( int idx=0; idx<objects_.size(); idx++ )
	if ( objects_[idx]->getDisplayTransformation() )
	    return objects_[idx]->getDisplayTransformation();

    return 0;
}


void DataObjectGroup::addObject(int nid)
{
    DataObject* no =
	dynamic_cast<DataObject*>( DM().getObject(nid) );

    if ( !no ) return;
    addObject( no );
}


void DataObjectGroup::insertObject( int insertpos, DataObject* no )
{
    if ( insertpos>=size() ) return addObject( no );

    objects_.insertAt( no, insertpos );
    nodes_.insertAt(no->getInventorNode(), insertpos );
    ensureGroup();
    group_->insertChild( no->getInventorNode(), insertpos );
    no->ref();
}


int DataObjectGroup::getFirstIdx( int nid ) const
{
    const DataObject* sceneobj =
	(const DataObject*) DM().getObject(nid);

    if ( !sceneobj ) return -1;

    return objects_.indexOf(sceneobj);
}


int DataObjectGroup::getFirstIdx( const DataObject* sceneobj ) const
{ return objects_.indexOf(sceneobj); }


void DataObjectGroup::removeObject( int idx )
{
    DataObject* sceneobject = objects_[idx];
    SoNode* node = nodes_[idx];
    group_->removeChild( node );

    nodes_.remove( idx );
    objects_.remove( idx );

    sceneobject->unRef();
}


void DataObjectGroup::removeAll()
{
    while ( size() ) removeObject( 0 );
}


SoNode*  DataObjectGroup::getInventorNode()
{
    ensureGroup();
    return group_;
}


void DataObjectGroup::fillPar( IOPar& par, TypeSet<int>& saveids)const
{
    DataObject::fillPar( par, saveids );
    
    par.set( nokidsstr, objects_.size() );
    
    BufferString key;
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	key = kidprefix;
	key += idx;

	int saveid = objects_[idx]->id();
	if ( saveids.indexOf( saveid )==-1 ) saveids += saveid;

	par.set( key, saveid );
    }
}


int DataObjectGroup::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res!= 1 ) return res;

    int nrkids;
    if ( !par.get( nokidsstr, nrkids ) )
	return -1;

    BufferString key;
    TypeSet<int> ids;
    for ( int idx=0; idx<nrkids; idx++ )
    {
	key = kidprefix;
	key += idx;

	int newid;
	if ( !par.get( key, newid ) )
	    return -1;

	if ( !DM().getObject( newid ) )
	{
	    res = 0;
	    continue;
	}

	ids += newid;
    }

    for ( int idx=0; idx<ids.size(); idx++ )
	addObject( ids[idx] );

    return res;
}

}; // namespace visBase
