/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Translator functions
-*/

static const char* rcsID = "$Id: iox.cc,v 1.11 2003-10-27 23:10:02 bert Exp $";

#include "iox.h"
#include "iostrm.h"
#include "iolink.h"
#include "ioman.h"
#include "ascstream.h"
#include "conn.h"

class IOXProducer : public IOObjProducer
{
    bool	canMake( const char* typ ) const
		{ return !strcmp(typ,XConn::sType); }
    IOObj*	make( const char* nm, const MultiID& ky, bool fd ) const
		{ return new IOX(nm,ky,fd); }
};

int IOX::prodid = IOObj::addProducer( new IOXProducer );


IOX::IOX( const char* nm, const char* ky, bool )
	: IOObject(nm,ky)
	, ownkey_("")
{
}


IOX::~IOX()
{
}


void IOX::setOwnKey( const MultiID& ky )
{
    ownkey_ = ky;
}


const char* IOX::connType() const
{
    return XConn::sType;
}


bool IOX::bad() const
{
    return ownkey_ == "";
}


void IOX::copyFrom( const IOObj* obj )
{
    if ( !obj ) return;
    if ( obj->isLink() ) obj = ((IOLink*)obj)->link();
    if ( !obj ) return;

    IOObj::copyFrom(obj);
    mDynamicCastGet(const IOX*,trobj,obj)
    if ( trobj )
	ownkey_ = trobj->ownkey_;
}


const char* IOX::fullUserExpr( bool i ) const
{
    IOObj* ioobj = IOM().get( ownkey_ );
    if ( !ioobj ) return "<invalid>";
    const char* s = ioobj->fullUserExpr(i);
    delete ioobj;
    return s;
}


bool IOX::slowOpen() const
{
    IOObj* ioobj = IOM().get( ownkey_ );
    if ( !ioobj ) return false;
    bool ret = ioobj->slowOpen();
    delete ioobj;
    return ret;
}


bool IOX::implExists( bool i ) const
{
    IOObj* ioobj = IOM().get( ownkey_ );
    if ( !ioobj ) return false;
    bool yn = ioobj->implExists(i);
    delete ioobj;
    return yn;
}


Conn* IOX::getConn( Conn::State rw ) const
{
    IOObj* ioobj = getIOObj();
    if ( !ioobj ) return 0;

    XConn* xconn = new XConn;
    xconn->conn_ = ioobj->getConn( rw );
    if ( xconn->conn_ ) xconn->conn_->ioobj = 0;
    xconn->ioobj = const_cast<IOX*>( this );

    delete ioobj;
    return xconn;
}


IOObj* IOX::getIOObj() const
{
    return ownkey_ == "" ? 0 : IOM().get( ownkey_ );
}


int IOX::getFrom( ascistream& stream )
{
    ownkey_ = stream.value();
    stream.next();
    return YES;
}


int IOX::putTo( ascostream& stream ) const
{
    stream.stream() << '$';
    stream.put( "ID", ownkey_ );
    return YES;
}
