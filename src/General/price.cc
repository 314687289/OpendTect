/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2011
-*/

static const char* mUnusedVar rcsID = "$Id: price.cc,v 1.2 2012-05-02 11:53:11 cvskris Exp $";

#include "price.h"
#include "bufstringset.h"

ManagedObjectSet<const Currency> Currency::repository_( false );

const Currency* Currency::getCurrency( const char* abrevation )
{
    for ( int idx=0; idx<repository_.size(); idx++ )
    {
	if ( repository_[idx]->abrevation_==abrevation )
	    return  repository_[idx];
    }

    return 0;
}


void Currency::getCurrencyStrings( BufferStringSet& abrevations )
{
    abrevations.erase();

    for ( int idx=0; idx<repository_.size(); idx++ )
	abrevations.add( repository_[idx]->abrevation_ );
}
