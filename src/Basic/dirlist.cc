/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 3-8-1994
-*/

static const char* rcsID = "$Id: dirlist.cc,v 1.22 2011-12-14 13:16:41 cvsbert Exp $";

#include "dirlist.h"

#include "file.h"
#include "filepath.h"
#include "staticstring.h"

#include <QDir>

DirList::DirList( const char* dirname, DirList::Type t, const char* msk )
	: dir_(dirname?dirname:".")
	, type_(t)
    	, mask_(msk)
{
    update();
}


void DirList::update()
{
    erase();
    const bool havemask = !mask_.isEmpty();

    QDir qdir( dir_.buf() );
    if ( havemask )
    {
	QStringList filters;
	filters << mask_.buf();
	qdir.setNameFilters( filters );
    }

    QDir::Filters filters;
    if ( type_ == FilesOnly )
	filters = QDir::Files;
    else if ( type_ == DirsOnly )
	filters = QDir::Dirs | QDir::NoDotAndDotDot;
    else
	filters = QDir::Dirs | QDir::NoDotAndDotDot | QDir::Files;

    QStringList qlist = qdir.entryList( filters );
    
    for ( int idx=0; idx<qlist.size(); idx++ )
    {	
	BufferString dirnm = qlist[idx].toAscii().constData();
#ifdef __win__
	int sz = dirnm.size() - 4;
	if ( strstr( dirnm.buf(), ".lnk" ) )
	    dirnm[sz] = '\0'; 
#endif
	add( dirnm );
    }

    sort();
}


const char* DirList::fullPath( int idx ) const
{
    static StaticStringManager stm;
    BufferString& ret = stm.getString();
    if ( idx < size() )
	ret = FilePath( dirName(), (*this).get(idx) ).fullPath();
    else
	ret.setEmpty();
    return ret.buf();
}
