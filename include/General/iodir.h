#ifndef iodir_H
#define iodir_H

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		31-7-1995
 RCS:		$Id$
________________________________________________________________________

-*/
 
 
#include "generalmod.h"
#include "multiid.h"
#include "objectset.h"
#include "namedobj.h"
#include "od_iosfwd.h"
class IOObj;


/*\brief 'Directory' of IOObj objects.

The IODir class is responsible for finding all IOObj's in the system. An IODir
instance will actually load all IOObj's, provides access to keys and allows
searching. It has a key of its own.

Few operation are done through the IODir directly: usually, IOMan will be
the service access point.

*/


mExpClass(General) IODir : public NamedObject
{
public:
			IODir(const char*);
			IODir(const MultiID&);
			~IODir();
    void		reRead();
    bool		isBad() const		{ return !isok_; }
    const MultiID&	key() const		{ return key_; }

    const IOObj*	main() const;
    const char*		dirName() const		{ return dirname_; }

    const ObjectSet<IOObj>& getObjs() const { return objs_; }
    int			size() const		  { return objs_.size(); }
    const IOObj*	operator[](const MultiID&) const;
    const IOObj*	operator[]( const char* str ) const;
    const IOObj*	operator[]( int idx ) const	{ return objs_[idx]; }
    const IOObj*	operator[]( IOObj* o ) const	{ return objs_[o]; }

    bool		addObj(IOObj*,bool immediate_store=true);
			    //!< after call, IOObj is mine
    bool		commitChanges(const IOObj*);
			    //!< after call, assume pointer will be invalid
    bool		permRemove(const MultiID&);
    bool		mkUniqueName(IOObj*);

    static IOObj*	getObj(const MultiID&);
    static IOObj*	getMain(const char*);

			// Use this if you know there's no contingency
			// Therefore, only in special-purpose programs
    bool		doWrite() const;

private:

    ObjectSet<IOObj>	objs_;
    FileNameString	dirname_;
    MultiID		key_;
    bool		isok_;
    mutable int		curid_;

			IODir();
    static bool		create(const char* dirnm,const MultiID&,IOObj* mainobj);
    static IOObj*	doRead(const char*,IODir*,int id=-1);
    static void		setDirName(IOObj&,const char*);
    static IOObj*	readOmf(od_istream&,const char*,IODir*,int);

    bool		build();
    bool		wrOmf(od_ostream&) const;

    MultiID		newKey() const;

    friend class	IOMan;
    friend class	IOObj;

};


#endif

