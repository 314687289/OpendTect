#ifndef visobject_h
#define visobject_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visdata.h"
#include "thread.h"
#include "visosg.h"

class Coord3;

namespace osg { class Switch; }

namespace visBase
{
class Material;
class Transformation;
class EventCatcher;

/*!
\ingroup visBase
\brief Base class for all objects that are visual on the scene.
*/

mExpClass(visBase) VisualObject : public DataObject
{
public:
    virtual bool		turnOn(bool)				= 0;
    virtual bool		isOn() const				= 0;

    virtual void		setMaterial( Material* )		= 0;
    virtual Material*		getMaterial()				= 0;

    virtual bool		getBoundingBox(Coord3& min,Coord3& max) const;
    virtual void		setSceneEventCatcher( EventCatcher* )	{}

    void			setPickable(bool);
    bool			isPickable() const;

    void			setSelectable(bool yn)	{ isselectable=yn; }
    bool			selectable() const 	{ return isselectable; }
    NotifierAccess*		selection() 		{ return &selnotifier; }
    NotifierAccess*		deSelection() 		{return &deselnotifier;}
    virtual NotifierAccess*	rightClicked()		{ return &rightClick; }
    const EventInfo*		rightClickedEventInfo() const{return rcevinfo;}
    const TypeSet<int>*		rightClickedPath() const;

protected:
    void			triggerSel()
    				{ if (isselectable) selnotifier.trigger(); }
    void			triggerDeSel()
    				{ if (isselectable) deselnotifier.trigger(); }
    void			triggerRightClick(const EventInfo*);
				VisualObject(bool selectable=false);
				~VisualObject();

private:
    bool			isselectable;
    Notifier<VisualObject>	selnotifier;
    Notifier<VisualObject>	deselnotifier;
    Notifier<VisualObject>	rightClick;
    const EventInfo*		rcevinfo;
};


mExpClass(visBase) VisualObjectImpl : public VisualObject
{
public:

    bool		turnOn(bool);
    bool		isOn() const;
    void		removeSwitch();
    void		setRightHandSystem(bool yn) { righthandsystem_=yn; }
    bool		isRightHandSystem() const { return righthandsystem_; }

    void		setLockable();
    			/*!<Will enable lock functionality.
			   \note Must be done before giving away the SoNode with
			   getInventorNode() to take effect */
    void		readLock();
    void		readUnLock();
    bool		tryReadLock();
    void		writeLock();
    void		writeUnLock();
    bool		tryWriteLock();

    void		setMaterial(Material*);
    const Material*	getMaterial() const { return material_; }
    Material*		getMaterial();

    static const char*	sKeyMaterialID(); //Remove
    static const char*	sKeyMaterial();
    static const char*	sKeyIsOn();

    virtual int		usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
   
protected:
    
    void		addChild(SoNode*);
    void		insertChild(int pos,SoNode*);
    void		removeChild(SoNode*);
    int			childIndex(const SoNode*) const;
    SoNode*		getChild(int);

    int			addChild(osg::Node*);
			//!<\returns new child index
    void		insertChild(int pos,osg::Node*);
    void		removeChild(osg::Node*);
    int			childIndex(const osg::Node*) const;


			VisualObjectImpl(bool selectable);
    virtual		~VisualObjectImpl();

    void		materialChangeCB(CallBacker*);

    Material*		material_;
    bool		righthandsystem_;

private:
    
    osg::Switch*	osgroot_;
};

mLockerClassImpl( visBase, VisualReadLockLocker, VisualObjectImpl,
		  readLock(), readUnLock(), tryReadLock() )
mLockerClassImpl( visBase, VisualWriteLockLocker, VisualObjectImpl,
		  writeLock(), writeUnLock(), tryWriteLock() )
};

#endif

