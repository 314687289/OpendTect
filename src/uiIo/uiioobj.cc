/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2006
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiioobj.cc,v 1.1 2006-05-29 09:20:06 cvsbert Exp $";

#include "uiioobj.h"
#include "uimsg.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "iodir.h"
#include "ioobj.h"
#include "filepath.h"


bool uiIOObj::removeImpl( bool rmentry )
{
    if ( !silent_ )
    {
	BufferString mess = "Remove ";
	if ( !rmentry ) mess += " existing ";
	mess += " data file(s), at\n'";
	if ( !ioobj_.isLink() )
	{
	    mess += ioobj_.fullUserExpr(true);
	    mess += "'?";
	}
	else
	{
	    BufferString fullexpr( ioobj_.fullUserExpr(true) );
	    mess += FilePath(fullexpr).fileName();
	    mess += "'\n- and everything in it! - ?";
	}
	if ( !uiMSG().askGoOn(mess) )
	    return false;
    }

    if ( !fullImplRemove(ioobj_) )
    {
	if ( !silent_ )
	{
	    BufferString mess = "Could not remove data file(s).\n";
	    mess += "Remove entry from list anyway?";
	    if ( !uiMSG().askGoOn(mess) )
		return false;
	}
    }

    if ( rmentry )
	IOM().permRemove( ioobj_.key() );

    return true;
}


bool uiIOObj::fillCtio( CtxtIOObj& ctio, bool warnifexist )
{
    if ( ctio.name() == "" )
    {
	if ( !ctio.ioobj )
	    return false;
	ctio.setName( ctio.ioobj->name() );
    }
    const char* nm = ctio.name().buf();

    IOM().to( ctio.ctxt.stdSelKey() );
    const IOObj* existioobj = (*IOM().dirPtr())[nm];
    if ( !existioobj )
    {
	ctio.setObj( 0 );
	IOM().getEntry( ctio );
	return ctio.ioobj;
    }

    if ( warnifexist )
    {
	BufferString msg( "Overwrite existing '" );
	msg += nm; msg += "'?";
	if ( !uiMSG().askGoOn(msg) )
	    return false;
    }

    ctio.setObj( existioobj->clone() );
    return true;
}
