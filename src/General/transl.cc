/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Translator functions
-*/

#include "transl.h"
#include "streamconn.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "iopar.h"
#include "errh.h"
#include "debugmasks.h"
#include <iostream>

static const char* rcsID = "$Id: transl.cc,v 1.21 2006-08-16 10:51:20 cvsbert Exp $";


TranslatorGroup::TranslatorGroup( const char* clssnm, const char* usrnm )
    : clssname_(clssnm)
    , usrname_(usrnm)
    , selhist_(0)
{ mRefCountConstructor;}


int defaultSelector( const char* mytyp, const char* typ )
{
    if ( !mytyp && !typ ) return 2;
    if ( !mytyp || !typ ) return 0;

    if ( !strcmp(mytyp,typ) ) return 2;

    return 0;
}


TranslatorGroup::~TranslatorGroup()
{
    delete selhist_;
    for ( int idx=0; idx<templs_.size(); idx++ )
	delete const_cast<Translator*>( templs_[idx] );

    while ( getGroups().indexOf(this)!=-1 )
	getGroups() -= this;
}


ObjectSet<TranslatorGroup>& TranslatorGroup::getGroups()
{
    static ObjectSet<TranslatorGroup>* allgrps = 0;
    if ( !allgrps )
	allgrps = new ObjectSet< TranslatorGroup >;
    return *allgrps;
}


int TranslatorGroup::add( Translator* tr )
{
    if ( !tr ) return -1;

    for ( int idx=0; idx<templs_.size(); idx++ )
    {
	const Translator* oldtr = templs_[idx];
	if ( oldtr->userName() == tr->userName() )
	{
	    if ( DBG::isOn(DBG_IO) )
	    {
		BufferString msg( "Translator already in list: '" );
		msg += tr->userName();
		msg += "' - replacing old.";
		DBG::message( msg );
	    }
	    templs_ -= oldtr; break;
	}
    }

    tr->setGroup( this );
    templs_ += tr;

    return templs_.size() - 1;
}


class EmptyTrGroup : public TranslatorGroup
{
public:

   EmptyTrGroup() : TranslatorGroup("",""), ctxt(0,"")	{}
   const IOObjContext& ioCtxt() const		{ return ctxt; }
   int objSelector( const char* ) const		{ return mObjSelUnrelated; }

   IOObjContext ctxt;

};


static TranslatorGroup* findGroup( const ObjectSet<TranslatorGroup>& grps,
				   const char* nm, bool user, bool iserr )
{
    if ( !nm || !*nm )
	{ pFreeFnErrMsg("nm empty","findGroup"); return 0; }

    for ( int idx=0; idx<grps.size(); idx++ )
    {
	if ( ( user && grps[idx]->userName() == nm)
	  || (!user && grps[idx]->clssName() == nm) )
	    return const_cast<TranslatorGroup*>( grps[idx] );
    }

    if ( iserr )
    {
	BufferString errmsg( "Cannot find '" );
	errmsg += nm;
	errmsg += "' TranslatorGroup";
	pFreeFnErrMsg(errmsg,"findGroup");
    }
    return 0;
}

static EmptyTrGroup* emptytrgroup = 0;
#define mRetEG \
{ if ( !emptytrgroup ) emptytrgroup = new EmptyTrGroup; return *emptytrgroup; }

TranslatorGroup& TranslatorGroup::getGroup( const char* nm, bool user )
{
    TranslatorGroup* ret = findGroup( groups(), nm, user, true );
    if ( !ret ) mRetEG
    return *ret;
}


TranslatorGroup& TranslatorGroup::addGroup( TranslatorGroup* newgrp )
{
    if ( DBG::isOn(DBG_IO) )
    {
	BufferString msg( "Translator group to be added: '" );
	if ( !newgrp )
	    msg += "(null)' . Major problem!";
	else
	{
	    msg += newgrp->userName();
	    msg += "' = class ";
	    msg += newgrp->clssName();
	}
	DBG::message( msg );
    }

    if ( !newgrp ) mRetEG

    TranslatorGroup* grp = findGroup( getGroups(), newgrp->clssName(),
	    				false, false );
    if ( grp )
    {
	if ( DBG::isOn(DBG_IO) )
	{
	    BufferString msg( "Not adding '" ); msg += newgrp->userName();
	    msg += "' again";
	    DBG::message( msg );
	}

	newgrp->ref(); newgrp->unRef();

	return *grp;
    }

    getGroups() += newgrp;
    return *newgrp;
}


bool TranslatorGroup::hasConnType( const char* ct ) const
{
    for ( int idx=0; idx<templs_.size(); idx++ )
    {
	if ( !strcmp(templs_[idx]->connType(),ct) )
	    return true;
    }

    return false;
}


static const BufferString& gtNm( const Translator* tr, bool user )
{
    return user ? tr->userName() : tr->typeName();
}


Translator* TranslatorGroup::make( const char* nm, bool usr ) const
{
    const Translator* tr = getTemplate( nm, usr );
    return tr ? tr->getNew() : 0;
}


const Translator* TranslatorGroup::getTemplate( const char* nm, bool usr ) const
{
    if ( !nm || !*nm ) return 0;

    // Direct match is OK - just return it
    const Translator* tr = 0;
    for ( int idx=0; idx<templs_.size(); idx++ )
    {
	if ( gtNm(templs_[idx],usr) == nm )
	    return templs_[idx];
    }

    // Now try to match only given string - may be part of full name
    for ( int idx=0; idx<templs_.size(); idx++ )
    {
	if ( matchString(nm,gtNm(templs_[idx],usr).buf()) )
	{
	    if ( tr ) // more than one match
		return 0;
	    tr = templs_[idx];
	}
    }

    return tr;
}


const char* Translator::connType() const
{
    return StreamConn::sType;
}


IOPar& TranslatorGroup::selHist()
{
    if ( !selhist_ )
    {
	BufferString parnm = userName();
	parnm += " selection history";
	selhist_ = new IOPar( parnm );
    }
    return *selhist_;
}


void TranslatorGroup::clearSelHist()
{
    if ( selhist_ ) selhist_->clear();
}


bool Translator::implExists( const IOObj* ioobj, int forread ) const
{
    if ( !ioobj ) return false;
    return ioobj->implExists( forread );
}
bool Translator::implReadOnly( const IOObj* ioobj ) const
{
    if ( !ioobj ) return false;
    return ioobj->implReadOnly();
}
bool Translator::implRemove( const IOObj* ioobj ) const
{
    if ( !ioobj ) return false;
    return ioobj->implRemove();
}
bool Translator::implRename( const IOObj* ioobj, const char* newnm,
			     const CallBack* cb ) const
{
    if ( !ioobj ) return false;
    return const_cast<IOObj*>(ioobj)->implRename( newnm, cb );
}
bool Translator::implSetReadOnly( const IOObj* ioobj, bool yn ) const
{
    if ( !ioobj ) return false;
    return ioobj->implSetReadOnly( yn );
}
