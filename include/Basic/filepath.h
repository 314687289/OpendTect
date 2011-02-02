#ifndef filepath_h
#define filepath_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Mar 2004
 RCS:		$Id: filepath.h,v 1.14 2011-02-02 09:26:05 cvsranojay Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"


/*!\brief File pathname tools.

  This class splits a full filename (with path) into the separate parts:
  directories and possibly a filename (the last part does not have to be a
  file name). That makes it easy to change the file name, or the path only,
  or the extension, add directories, etc.
  
  To get the result after manipulation, use the fullPath().

  fileName() returns the last part, pathOnly() returns all but the last part.
  setFileName() replaces the last part, setPath() replaces all but the last
  part.
 
 */

mClass FilePath
{
public:
    enum Style		{ Local, Unix, Windows };

    			FilePath(const char* fullinp=0);
			FilePath(const FilePath&);

    FilePath&		operator =(const FilePath&);
    FilePath&		operator =(const char* fullinp);
    bool		operator ==(const FilePath&) const;
    bool		operator ==(const char* fnm) const;
    bool		operator !=(const FilePath&) const;
    bool		operator !=(const char* fnm) const;

    FilePath&		set(const char* fullinp);
    FilePath&		add(const char*);	//!< at end
    FilePath&		insert(const char*);	//!< after prefix at begin
    void		setFileName(const char*); //!< pass null to remove level
    void		setPath(const char*);	//!< including prefix
    void		setExtension(const char*,bool replace=true);
    						//!< !replace => add

    bool		isAbsolute() const;
    bool		isSubDirOf(const FilePath&,FilePath* reldir = 0) const;
			/*!<If reldir is set, it will be filled with the
			    relative path. */
    bool		makeRelativeTo(const FilePath&);
    bool		makeCanonical();


    BufferString	fullPath(Style s=Local,bool cleanup=true) const;
    const char*		prefix() const;
    int			nrLevels() const;
    const char*		extension() const;	//!< may return null

    const BufferString& fileName() const;
    BufferString	pathOnly() const;
    BufferString	winDrive() const;

    const BufferString&	dir(int nr=-1) const;
    			//!< nr < 0 returns last dir name
    BufferString	dirUpTo(int) const;
    			//!< nr < 0 returns last dir, including prefix

    static BufferString	getTempDir();
    static BufferString	getTempName(const char* ext=0);
    static BufferString	mkCleanPath(const char* path, Style fmt);

    static const char*	dirSep(Style);
    static const char*	sPrefSep;

protected:

    bool		isabs_;
    BufferString	prefix_;
    BufferStringSet	lvls_;

    void		addPart(const char*);
    void		compress(int sl=0);
    void		trueDirIfLink();
};


#endif
