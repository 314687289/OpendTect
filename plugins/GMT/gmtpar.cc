/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          July 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "gmtpar.h"

#include "debug.h"
#include "envvars.h"
#include "keystrs.h"
#include "oddirs.h"
#include "strmprov.h"
#include "od_ostream.h"
#include <iostream>


GMTParFactory& GMTPF()
{
    mDefineStaticLocalObject( GMTParFactory, inst, );
    return inst;
}


int GMTParFactory::add( const char* nm, GMTParCreateFunc fn )
{
    Entry* entry = getEntry( nm );
    if ( entry )
	entry->crfn_ = fn;
    else
    {
	entry = new Entry( nm, fn );
	entries_ += entry;
    }

    return entries_.size() - 1;
}


GMTPar* GMTParFactory::create( const IOPar& iop ) const
{
    const char* grpname = iop.find( ODGMT::sKeyGroupName() );
    if ( !grpname || !*grpname ) return 0;

    Entry* entry = getEntry( grpname );
    if ( !entry ) return 0;

    GMTPar* grp = entry->crfn_( iop );
    return grp;
}


const char* GMTParFactory::name( int idx ) const
{
    if ( idx < 0 || idx >= entries_.size() )
	return 0;

    return entries_[idx]->name_.buf();
}


GMTParFactory::Entry* GMTParFactory::getEntry( const char* nm ) const
{
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	if ( entries_[idx]->name_ == nm )
	    return const_cast<GMTParFactory*>( this )->entries_[idx];
    }

    return 0;
}


BufferString GMTPar::fileName( const char* fnm ) const
{
    BufferString fnmchg;
    if ( __iswin__ ) fnmchg += "\"";
    fnmchg += fnm;
    if ( __iswin__ ) fnmchg += "\"";
    return fnmchg;
}


bool GMTPar::execCmd( const BufferString& comm, od_ostream& strm )
{
    DBG::message( comm );
    BufferString cmd;
    const char* errfilenm = GetProcFileName( "gmterr.err" );
    const char* shellnm = GetOSEnvVar( "SHELL" );
    const bool needsbash = shellnm && *shellnm && !firstOcc(shellnm,"bash");
    if ( needsbash )
	cmd += "bash -c \'";

    const char* commstr = comm.buf();
    if ( commstr && commstr[0] == '@' )
	commstr++;

    cmd += commstr;
    cmd += " 2> \"";
    cmd += errfilenm;
    cmd += "\"";
    if ( needsbash )
	cmd += "\'";

    if ( system(cmd) )
    {
	StreamData sd = StreamProvider( errfilenm ).makeIStream();
	if ( !sd.usable() )
	    return false;

	char buf[256];
	strm << od_endl;
	while ( sd.istrm->getline(buf,256) )
	    strm << buf << od_endl;

	sd.close();
	return false;
    }

    return true;
}


StreamData GMTPar::makeOStream( const BufferString& comm, od_ostream& strm )
{
    DBG::message( comm );
    BufferString cmd;
    const char* errfilenm = GetProcFileName( "gmterr.err" );
    const char* shellnm = GetOSEnvVar( "SHELL" );
    const bool needsbash = shellnm && *shellnm && !firstOcc(shellnm,"bash");
    const char* commptr = comm.buf();
    if ( needsbash )
    {
	cmd += "@bash -c \'";
	if ( commptr[0] == '@' )
	    commptr ++;
    }

    cmd += commptr;
    cmd += " 2> \"";
    cmd += errfilenm;
    cmd += "\"";
    if ( needsbash )
	cmd += "\'";

    StreamData sd = StreamProvider(cmd).makeOStream();
    if ( !sd.usable() )
    {
	StreamData errsd = StreamProvider( errfilenm ).makeIStream();
	if ( !errsd.usable() )
	    return sd;

	char buf[256];
	strm << od_endl;
	while ( errsd.istrm->getline(buf,256) )
	    strm << buf << od_endl;

	errsd.close();
    }

    return sd;
}

