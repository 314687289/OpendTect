#ifndef filesystemwatcher_h
#define filesystemwatcher_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2009
 RCS:           $Id: filesystemwatcher.h,v 1.1 2009-04-20 08:14:36 cvsnanne Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "bufstring.h"

class BufferStringSet;
class QFileSystemWatcher;
class QFileSystemWComm;

mClass FileSystemWatcher : public CallBacker 
{
public:
    friend class QFileSystemWComm;

				FileSystemWatcher();
				~FileSystemWatcher();

    void			addFile(const BufferString&);
    void			addFiles(const BufferStringSet&);
    void			removeFile(const BufferString&);
    void			removeFiles(const BufferStringSet&);

    const BufferString&		changedDir() const  { return chgddir_; }
    const BufferString&		changedFile() const { return chgdfile_; }

    Notifier<FileSystemWatcher>	directoryChanged;
    Notifier<FileSystemWatcher>	fileChanged;

protected:

    BufferString		chgddir_;
    BufferString		chgdfile_;

    QFileSystemWatcher*		qfswatcher_;
    QFileSystemWComm*		qfswatchercomm_;

};

mGlobal FileSystemWatcher& FSW();

#endif
