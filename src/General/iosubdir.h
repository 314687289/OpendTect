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
#include "keystrs.h"

mClass( General )  IOSubDir : public IOObj
{
public:
		IOSubDir(const char* subdirnm);
		IOSubDir(const IOSubDir&);
    bool	isSubdir() const	{ return true; }
    bool	bad() const		{ return isbad_; }
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

    FixedString	connType() const	{ return sKey::EmptyString(); }
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
