#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2016
________________________________________________________________________

-*/

#include "generalmod.h"
#include "sharedobject.h"
#include "dbkey.h"
#include "iopar.h"
#include "uistring.h"
class IOObj;
class TaskRunner;


/*!\brief Object that can be saved at any time. */

mExpClass(General) Saveable : public Monitorable
{ mODTextTranslationClass(Saveable)
public:

			Saveable(const SharedObject&);
			~Saveable();
			mDeclAbstractMonitorableAssignment(Saveable);

    const SharedObject* object() const;
    bool		objectAlive() const	{ return objectalive_; }
    void		setObject(const SharedObject&);

    mImplSimpleMonitoredGetSet(inline,key,setKey,DBKey,storekey_,0)
    mImplSimpleMonitoredGetSet(inline,ioObjPars,setIOObjPars,IOPar,ioobjpars_,0)
			// The pars will be merge()'d with the IOObj's current

    virtual uiRetVal	save(TaskRunner* tskr=0) const;
    virtual uiRetVal	store(const IOObj&,TaskRunner* tskr=0) const;

    bool		needsSave() const;
    virtual void	setJustSaved() const;

    DirtyCountType	lastSavedDirtyCount() const
			{ return lastsavedirtycount_; }
    DirtyCountType	curDirtyCount() const;

    static ChangeType	cSaveSucceededChangeType()	{ return 1; }
    static ChangeType	cSaveFailedChangeType()		{ return 2; }

protected:

    const SharedObject* object_;
    Threads::Atomic<bool> objectalive_;
    DBKey		storekey_;
    IOPar		ioobjpars_;
    mutable DirtyCounter lastsavedirtycount_;

			// This function can be called from any thread
    virtual uiRetVal	doStore(const IOObj&,TaskRunner* tskr=0) const	= 0;
    bool		isSave(const IOObj&) const;

private:

    void		attachCBToObj();
    void		detachCBFromObj();
    void		objDelCB(CallBacker*);

};
