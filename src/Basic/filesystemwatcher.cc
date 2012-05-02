/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          2009
________________________________________________________________________

-*/
static const char* mUnusedVar rcsID = "$Id: filesystemwatcher.cc,v 1.3 2012-05-02 11:53:01 cvskris Exp $";

#include "filesystemwatcher.h"
#include "qfilesystemcomm.h"
#include "bufstringset.h"


FileSystemWatcher::FileSystemWatcher()
    : directoryChanged(this)
    , fileChanged(this)
{
    qfswatcher_ = new QFileSystemWatcher;
    qfswatchercomm_ = new QFileSystemWComm( qfswatcher_, this );
}


FileSystemWatcher::~FileSystemWatcher()
{
    delete qfswatchercomm_;
    delete qfswatcher_;
}


void FileSystemWatcher::addFile( const BufferString& fnm )
{ qfswatcher_->addPath( fnm.buf() ); }


void FileSystemWatcher::addFiles( const BufferStringSet& fnms )
{
    for ( int idx=0; idx<fnms.size(); idx++ )
	addFile( fnms.get(idx) );
}


void FileSystemWatcher::removeFile( const BufferString& fnm )
{ qfswatcher_->removePath( fnm.buf() ); }


void FileSystemWatcher::removeFiles( const BufferStringSet& fnms )
{
    for ( int idx=0; idx<fnms.size(); idx++ )
	removeFile( fnms.get(idx) );
}


FileSystemWatcher& FSW()
{
    static FileSystemWatcher* fsw = 0;
    if ( !fsw ) fsw = new FileSystemWatcher;
    return *fsw;
}
