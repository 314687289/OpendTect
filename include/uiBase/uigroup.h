#ifndef uigroup_H
#define uigroup_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uigroup.h,v 1.30 2003-07-29 11:45:57 bert Exp $
________________________________________________________________________

-*/

#include <uiobj.h>
#include <uiparent.h>
#include <callback.h>
class IOPar;

class uiGroupBody;
class uiParentBody;

class uiGroup;
class uiGroupObjBody;
class uiGroupParentBody;

//class uiTabStack;
//class uiTab;

class QWidget;


class uiGroupObj : public uiObject
{ 	
friend class uiGroup;
protected:
			uiGroupObj( uiGroup*,uiParent*, const char*, bool );
public:

    virtual		~uiGroupObj();

protected:

    uiGroupObjBody*	body_;
    uiGroup*		uigrp_;

    void		bodyDel( CallBacker* );
    void		grpDel( CallBacker* );
};


class uiGroup : public uiParent
{ 	
friend class		uiGroupObjBody;
friend class		uiGroupParentBody;
friend class		uiGroupObj;
friend class		uiMainWin;
friend class		uiTabStack;
public:
			uiGroup( uiParent* , const char* nm="uiGroup", 
				 bool manage=true );
    virtual		~uiGroup();

    inline		operator const uiGroupObj*() const  { return grpobj_; }
    inline		operator uiGroupObj*() 		    { return grpobj_; }
    inline		operator const uiObject&() const    { return *grpobj_; }
    inline		operator uiObject&() 		    { return *grpobj_; }
    inline uiObject*	attachObj()			    { return grpobj_; }
    inline const uiObject* attachObj() const		    { return grpobj_; }

    void		setHSpacing( int ); 
    void		setVSpacing( int ); 
    void		setSpacing( int s=0 )	
			{ setHSpacing(s); setVSpacing(s); }
    void		setBorder( int ); 

    void		setFrame( bool yn=true );

    uiObject*		hAlignObj();
    void		setHAlignObj( uiObject* o );
    void		setHAlignObj( uiGroup* o )
			    { setHAlignObj(o->mainObject()); }
    uiObject*		hCentreObj();
    void		setHCentreObj( uiObject* o );
    void		setHCentreObj( uiGroup* o )
			    { setHCentreObj(o->mainObject()); }

    virtual bool	fillPar( IOPar& ) const		{ return true; }
    virtual void	usePar( const IOPar& )		{}

    //! internal use only. Tells the layout manager it's a toplevel mngr.
    void		setIsMain( bool ); 
    virtual uiMainWin*	mainwin()
			    { return mainObject() ? mainObject()->mainwin() :0;}

    static uiGroup*	gtDynamicCastToGrp( QWidget* );

protected:

    uiGroupObj*		grpobj_;
    uiGroupParentBody*	body_;

    virtual uiObject*	mainobject()			{ return grpobj_; }
    virtual void	attach_( constraintType, uiObject *oth, int margin=-1,
				bool reciprocal=true);

    virtual void	reDraw_( bool deep )		{}

    void		setShrinkAllowed(bool);
    bool		shrinkAllowed();

    void		bodyDel( CallBacker* );
    void		uiobjDel( CallBacker* );

    void		setFrameStyle(int);

};

class NotifierAccess;

class uiGroupCreater
{
public:

    virtual uiGroup*		create(uiParent*,NamedNotifierList* =0)	= 0;

};


#endif
