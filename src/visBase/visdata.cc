/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUnusedVar = "$Id: visdata.cc,v 1.40 2012-05-22 14:48:42 cvskris Exp $";

#include "visdata.h"

#include "errh.h"
#include "iopar.h"
#include "keystrs.h"
#include "visdataman.h"
#include "visselman.h"

#include <Inventor/nodes/SoNode.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/SoOutput.h>

#include <osg/Node>
#include <osgDB/WriteFile>

namespace visBase
{


bool DataObject::doosg_ = false;

void DataObject::setOsg()
{ doosg_ = true; }

bool DataObject::doOsg()
{ return doosg_; }

const char* DataObject::name() const
{
    return !name_ || name_->isEmpty() ? 0 : name_->buf();
}


void DataObject::setName( const char* nm )
{
    SoNode* node = getInventorNode();
    if ( node )
	node->setName( nm );
    osg::ref_ptr<osg::Node> osgnode = osgNode();
    if ( osgnode )
	osgnode->setName( nm );

    if ( !name_ ) name_ = new BufferString;
    (*name_) = nm;
}


DataObject::DataObject()
    : id_( -1 )
    , name_( 0 )
    , saveinsessions_(true)
{}


DataObject::~DataObject()
{
    DM().removeObject( this );
    delete name_;
}


void DataObject::select() const
{ DM().selMan().select( id() ); }


void DataObject::deSelect() const
{ DM().selMan().deSelect( id() ); }


bool DataObject::isSelected() const
{ return selectable() && DM().selMan().selected().indexOf( id()) != -1; }


void DataObject::setDisplayTransformation( const mVisTrans* trans )
{   
    if ( trans!=getDisplayTransformation() )
	pErrMsg("Not implemented");
}   
    

void DataObject::fillPar( IOPar& par, TypeSet<int>& ) const
{
    par.set( sKey::Type(), getClassName() );

    const char* nm = name();
    if ( nm )
	par.set( sKey::Name(), nm );
}


bool DataObject::serialize( const char* filename, bool binary )
{
    if ( doOsg() && osgNode() )
    {
	return osgDB::writeNodeFile( *osgNode(), std::string( filename ) );
    }

    SoNode* node = getInventorNode();
    if ( !node ) return false;

    SoWriteAction writeaction;
    if ( !writeaction.getOutput()->openFile(filename) )
	return false;

    writeaction.getOutput()->setBinary(binary);
    writeaction.apply( node );
    writeaction.getOutput()->closeFile();
    return true;
}


int DataObject::usePar( const IOPar& par )
{
    const char* nm = par.find( sKey::Name() );
    if ( nm )
	setName( nm );

    return 1;
}


bool DataObject::_init()
{
    DM().addObject( this );
    return true;
}


}; // namespace visBase
