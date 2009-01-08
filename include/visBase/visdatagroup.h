#ifndef visdataobjgroup_h
#define visdataobjgroup_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visdatagroup.h,v 1.7 2009-01-08 10:15:41 cvsranojay Exp $
________________________________________________________________________

-*/

#include "sets.h"
#include "visdata.h"

class SoGroup;
class SoSeparator;

namespace visBase
{


mClass DataObjectGroup : public DataObject
{
public:
    static DataObjectGroup*	create()
				mCreateDataObj(DataObjectGroup);

    void			setSeparate( bool yn=true ) { separate_=yn; }
    				/*! Set directly after creation, othersize
				    it won't have effect */

    virtual int			size() const;
    virtual void		addObject( DataObject* );
    void			addObject( int id );
    				/*!< Calls addObject( DataObject* ) */

    virtual void		insertObject( int idx, DataObject* );

    virtual int			getFirstIdx( int id ) const;
    				/*!<\returns	the first index (there might be
						many instances) of the id in
						the group, or -1 if not found 
				*/
    virtual int			getFirstIdx( const DataObject* ) const;
    				/*!<\returns	the first index (there might be
						many instances) of the object in
						the group, or -1 if not found 
				*/

    virtual void		removeObject( int idx );

    virtual void		removeAll();
    virtual DataObject*		getObject( int idx )
    				{return objects_.size()>idx ? objects_[idx] :0;}
    virtual const DataObject*	getObject( int idx ) const
				{return objects_.size()>idx ? objects_[idx] :0;}

    virtual SoNode*		getInventorNode();

    void			setDisplayTransformation(Transformation*);
    Transformation*		getDisplayTransformation();
    				/*!\returns the trans of the first child
					    with a trans, or null if none of
					    the childrens has a trans */
    void			setRightHandSystem(bool);
    bool			isRightHandSystem() const;

    Notifier<DataObjectGroup>	change; //triggers on add/insert/remove
    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);

protected:
    virtual			~DataObjectGroup();

    static const char*		nokidsstr;
    static const char*		kidprefix;
    virtual SoGroup*		createGroup();
    void			ensureGroup();
    SoGroup*			group_;

    bool			righthandsystem_;

private:
    ObjectSet<DataObject>	objects_;
    ObjectSet<SoNode>		nodes_;

    bool			separate_;
};
}; //namespace

#endif
