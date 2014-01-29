#ifndef iosubdir_h
#define iosubdir_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2010
 RCS:		$Id$
________________________________________________________________________

*/

#include "ioobj.h"


class IOSubDir : public IOObj
{
public:
		IOSubDir(const char* subdirnm);
		IOSubDir(const IOSubDir&);
    bool	isSubdir() const	{ return true; }
    bool	isBad() const		{ return isbad_; }
    void	copyFrom(const IOObj*)	{}
    static IOSubDir* get(ascistream&,const char* rootdirnm);

    const char*	fullUserExpr(bool) const;
    bool	implExists(bool) const	{ return !isbad_; }
    bool	implReadOnly() const	{ return false; }
    bool	implRemove() const	{ return false; }
    bool	implShouldRemove() const { return false; }
    bool	implRename(const char*,const CallBack*)
					{ return false; }
    bool        implSetReadOnly(bool) const { return false; }
    bool	removeQuery() const	{ return true; }

    const char*	connType() const	{ return 0; }
    Conn*	getConn( bool forread ) const { return 0; }

    void	genDefaultImpl()	{}
    const char*	dirName() const		{ return fullUserExpr(true); }

protected:

    bool	isbad_;

    bool	getFrom(ascistream&)	{ return true; }
    bool	putTo(ascostream&) const;

   friend class	IOMan;

};


#endif
