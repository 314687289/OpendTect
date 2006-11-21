/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
-*/

static const char* rcsID = "$Id: seispsioprov.cc,v 1.6 2006-11-21 14:00:07 cvsbert Exp $";

#include "seispsioprov.h"
#include "seispsfact.h"
#include "filegen.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"

SeisPSIOProviderFactory& SPSIOPF()
{
    static SeisPSIOProviderFactory* theinst = 0;
    if ( !theinst ) theinst = new SeisPSIOProviderFactory;
    return *theinst;
}


const SeisPSIOProvider* SeisPSIOProviderFactory::provider( const char* t ) const
{
    if ( provs_.isEmpty() )	return 0;
    else if ( !t )		return provs_[0];

    for ( int idx=0; idx<provs_.size(); idx++ )
	if ( !strcmp(t,provs_[idx]->type()) )
	    return provs_[idx];

    return 0;
}


SeisPSReader* SeisPSIOProviderFactory::getReader( const IOObj& ioobj,
						  int inl ) const
{
    if ( provs_.isEmpty() ) return 0;
    const SeisPSIOProvider* prov = provider( ioobj.translator() );
    return prov ? prov->makeReader( ioobj.fullUserExpr(true), inl ) : 0;
}


SeisPSWriter* SeisPSIOProviderFactory::getWriter( const IOObj& ioobj ) const
{
    if ( provs_.isEmpty() ) return 0;
    const SeisPSIOProvider* prov = provider( ioobj.translator() );
    return prov ? prov->makeWriter( ioobj.fullUserExpr(false) ) : 0;
}


mDefSimpleTranslatorSelector(SeisPS,sKeySeisPSTranslatorGroup)
mDefSimpleTranslatorioContext(SeisPS,Seis)


bool CBVSSeisPSTranslator::implRemove( const IOObj* ioobj ) const
{
    if ( !ioobj ) return false;
    BufferString fnm( ioobj->fullUserExpr(true) );
    if ( File_exists(fnm) )
	File_remove( fnm, File_isDirectory(fnm) );
    return File_exists(fnm);
}
