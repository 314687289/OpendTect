/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: odsession.cc,v 1.14 2006-12-15 14:35:22 cvsnanne Exp $";

#include "odsession.h"
#include "ptrman.h"
#include "ioobj.h"
#include "iopar.h"
#include "ascstream.h"


const char* ODSession::visprefix = "Vis";
const char* ODSession::sceneprefix = "Scene";
const char* ODSession::attrprefix = "Attribs";	//backward comp 2.4
const char* ODSession::attr2dprefix = "2D.Attribs";
const char* ODSession::attr3dprefix = "3D.Attribs";
const char* ODSession::nlaprefix = "NLA";
const char* ODSession::trackprefix = "Tracking";
const char* ODSession::pluginprefix = "Plugins";


void ODSession::clear()
{
    vispars_.clear();
    scenepars_.clear();
    attrpars_.clear();				//backward comp 2.4
    attrpars2d_.clear();
    attrpars3d_.clear();
    nlapars_.clear();
    mpepars_.clear();
    pluginpars_.clear();
}


ODSession& ODSession::operator=( const ODSession& sess )
{
    if ( &sess != this )
    {
	vispars_ = sess.vispars_;
	scenepars_ = sess.scenepars_;
	attrpars_ = sess.attrpars_;		//backward comp 2.4
	attrpars2d_ = sess.attrpars2d_;
	attrpars3d_ = sess.attrpars3d_;
	nlapars_ = sess.nlapars_;
	mpepars_ = sess.mpepars_;
	pluginpars_ = sess.pluginpars_;
    }
    return *this;
}


bool ODSession::operator==( const ODSession& sess ) const
{
    return vispars_ == sess.vispars_
	&& attrpars_ == sess.attrpars_		//backward comp 2.4
	&& attrpars2d_ == sess.attrpars2d_
	&& attrpars3d_ == sess.attrpars3d_
	&& nlapars_ == sess.nlapars_
	&& mpepars_ == sess.mpepars_
	&& pluginpars_ == sess.pluginpars_;
}
    

bool ODSession::usePar( const IOPar& par )
{
    PtrMan<IOPar> vissubpars = par.subselect(visprefix);
    if ( !vissubpars ) return false;
    vispars_ = *vissubpars;

    PtrMan<IOPar> scenesubpars = par.subselect(sceneprefix);
    if ( scenesubpars )
        scenepars_ = *scenesubpars;

    PtrMan<IOPar> attrsubpars = par.subselect(attrprefix);
    if ( attrsubpars )
	attrpars_ = *attrsubpars;		//backward comp 2.4

    PtrMan<IOPar> attr2dsubpars = par.subselect(attr2dprefix);
    if ( attr2dsubpars )
	attrpars2d_ = *attr2dsubpars;

    PtrMan<IOPar> attr3dsubpars = par.subselect(attr3dprefix);
    if ( attr3dsubpars )
	attrpars3d_ = *attr3dsubpars;
    
    PtrMan<IOPar> nlasubpars = par.subselect(nlaprefix);
    if ( nlasubpars )
	nlapars_ = *nlasubpars;

    PtrMan<IOPar> mpesubpars = par.subselect(trackprefix);
    if ( mpesubpars )
	mpepars_ = *mpesubpars;

    PtrMan<IOPar> pluginsubpars = par.subselect(pluginprefix);
    if ( pluginsubpars )
        pluginpars_ = *pluginsubpars;

    return true;
}


void ODSession::fillPar( IOPar& par ) const
{
    par.mergeComp( vispars_, visprefix );
    par.mergeComp( scenepars_, sceneprefix );
    par.mergeComp( attrpars_, attrprefix );	//backward comp 2.4
    par.mergeComp( attrpars2d_, attr2dprefix );
    par.mergeComp( attrpars3d_, attr3dprefix );
    par.mergeComp( nlapars_, nlaprefix );
    par.mergeComp( mpepars_, trackprefix );
    par.mergeComp( pluginpars_, pluginprefix );
}


IOPar& ODSession::attrpars( bool is2d )
{
    if ( !attrpars2d_.size() && !attrpars3d_.size() && attrpars_.size() )
	return attrpars_;			//backward comp 2.4
    else if ( is2d )
	return attrpars2d_;
    else
	return attrpars3d_;
}


int ODSessionTranslatorGroup::selector( const char* key )
{
    int retval = defaultSelector( theInst().userName(), key );
    if ( retval ) return retval;

    if ( defaultSelector(ODSessionTranslator::keyword,key) ) return 1;
    return 0;
}

mDefSimpleTranslatorioContext(ODSession,Misc)


bool ODSessionTranslator::retrieve( ODSession& session,
				    const IOObj* ioobj, BufferString& err )
{
    if ( !ioobj ) { err = "Cannot find object in data base"; return false; }
    PtrMan<ODSessionTranslator> tr =
		dynamic_cast<ODSessionTranslator*>(ioobj->getTranslator());
    if ( !tr ) { err = "Selected object is not an Session"; return false; }
    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
	{ err = "Cannot open "; err += ioobj->fullUserExpr(true); return false;}
    err = tr->read( session, *conn );
    bool rv = err.isEmpty();
    if ( rv ) err = tr->warningMsg();
    return rv;
}


bool ODSessionTranslator::store( const ODSession& session,
				 const IOObj* ioobj, BufferString& err )
{
    if ( !ioobj ) { err = "No object to store set in data base"; return false; }
    PtrMan<ODSessionTranslator> tr
	 = dynamic_cast<ODSessionTranslator*>(ioobj->getTranslator());
    if ( !tr )
	{ err = "Selected object is not an OpendTect Session"; return false; }
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
    { err = "Cannot open "; err += ioobj->fullUserExpr(false); return false; }
    err = tr->write( session, *conn );
    return err.isEmpty();
}


const char* ODSessionTranslator::keyword = "Session setup";


const char* dgbODSessionTranslator::read( ODSession& session, Conn& conn )
{
    warningmsg = "";
    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    ascistream astream( ((StreamConn&)conn).iStream() );
    const bool isoldhdr = !strcmp(astream.projName(),"dGB-dTect");
    const int nr = astream.minorVersion() + astream.majorVersion() * 100;
    if ( isoldhdr && nr < 200 )
	return "Cannot read session files older than d-Tect V2.0";


    IOPar iopar( astream );
    if ( iopar.isEmpty() )
	return "Empty input file";

    if ( !session.usePar( iopar ))
	return "Could not read session-file";

    return 0;
}


const char* dgbODSessionTranslator::write( const ODSession& session, Conn& conn)
{
    warningmsg = "";
    if ( !conn.forWrite() || !conn.isStream() )
	return "Internal error: bad connection";

    IOPar iop( ODSessionTranslator::keyword );
    session.fillPar( iop );
    if ( !iop.write(((StreamConn&)conn).oStream(),mTranslGroupName(ODSession)) )
	return "Cannot write d-Tect session to file";
    return 0;
}
