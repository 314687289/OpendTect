#ifndef emobject_h
#define emobject_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emobject.h,v 1.66 2007-07-06 14:11:05 cvskris Exp $
________________________________________________________________________


-*/

#include "bufstring.h"
#include "callback.h"
#include "emposid.h"
#include "multiid.h"
#include "position.h"
#include "refcount.h"
#include "draw.h"

class IOObj;
class Executor;
struct CubeSampling;
class IOObjContext;
class Color;

namespace Geometry { class Element; }

namespace EM
{
class EMManager;

class EMObjectCallbackData
{
public:
    		EMObjectCallbackData() 
		    : pid0( 0, 0, 0 )
		    , pid1( 0, 0, 0 )
		    , attrib( -1 )
		    , event( EMObjectCallbackData::Undef )
		{}

    enum Event { Undef, PositionChange, PosIDChange, PrefColorChange, Removal,
   		 AttribChange, SectionChange, BurstAlert } event;

    EM::PosID	pid0;
    EM::PosID	pid1;	//Only used in PosIDChange
    int		attrib; //Used only with AttribChange
};


/*! Iterator that iterates a number of positions (normally all) on an EMObject.
The object is created by EMObject::createIterator, and the next() function is
called until no more positions can be found. */


class EMObjectIterator
{
public:
    virtual		~EMObjectIterator() {}
    virtual EM::PosID	next() 		= 0;
    			/*!<posid.objectID()==-1 when there are no more pids*/
    virtual int		aproximateSize() const { return -1; }
    virtual int		maximumSize() const { return -1; }
};


class PosAttrib
{
public:
    			PosAttrib()
			    : style_(MarkerStyle3D::Cube,5,Color::White) {}

    enum Type		{ PermanentControlNode, TemporaryControlNode,
			  EdgeControlNode, TerminationNode, SeedNode };

    Type		type_;
    TypeSet<PosID>	posids_;

    MarkerStyle3D	style_;

    bool		locked_;    
};



/*!\brief Earth Model Object */

class EMObject : public CallBacker
{
mRefCountImplWithDestructor(EMObject,virtual ~EMObject(),
{ prepareForDelete(); delete this; } );
public:
    const ObjectID&		id() const		{ return id_; }
    virtual const char*		getTypeStr() const			= 0;
    const MultiID&		multiID() const		{ return storageid_; }
    void			setMultiID(const MultiID&);

    BufferString		name() const;

    virtual int			nrSections() const 			= 0;
    virtual SectionID		sectionID(int) const			= 0;
    virtual BufferString	sectionName(const SectionID&) const;
    virtual bool		canSetSectionName() const;
    virtual bool		setSectionName(const SectionID&,const char*,
	    				       bool addtohistory);
    virtual int			sectionIndex(const SectionID&) const;
    virtual bool		removeSection(SectionID,bool hist )
    					{ return false; }

    const Geometry::Element*	sectionGeometry(const SectionID&) const;
    Geometry::Element*		sectionGeometry(const SectionID&);

    const Color&		preferredColor() const;
    void			setPreferredColor(const Color&);
    void			setBurstAlert(bool yn);

    virtual Coord3		getPos(const EM::PosID&) const;
    virtual Coord3		getPos(const EM::SectionID&,
	    			       const EM::SubID&) const;
    virtual bool		isDefined(const EM::PosID&) const;
    virtual bool		isDefined(const EM::SectionID&,
					  const EM::SubID&) const;
    virtual bool		setPos(const EM::PosID&,const Coord3&,
				       bool addtohistory);
    virtual bool		setPos(const EM::SectionID&,const EM::SubID&,
	    			       const Coord3&,bool addtohistory);
    virtual bool		unSetPos(const EM::PosID&,bool addtohistory);
    virtual bool		unSetPos(const EM::SectionID&,const EM::SubID&,
					 bool addtohistory);


    virtual bool		enableGeometryChecks(bool);
    virtual bool		isGeometryChecksEnabled() const;

    virtual bool		isAtEdge(const EM::PosID&) const;

    void			changePosID(const EM::PosID& from, 
	    				    const EM::PosID& to,
					    bool addtohistory);
    				/*!<Tells the object that the node former known
				    as from is now called to. Function will also
				    exchange set the position of to to the 
				    posion of from. */

    virtual void		getLinkedPos(const EM::PosID& posid,
					  TypeSet<EM::PosID>&) const
    					{ return; }
    				/*!< Gives positions on the object that are
				     linked to the posid given
				*/

    virtual EMObjectIterator*	createIterator(const EM::SectionID&, 
	    				       const CubeSampling* =0) const
				{ return 0; }
    				/*!< creates an iterator. If the sectionid is
				     -1, all sections will be traversed. */

    virtual int			nrPosAttribs() const;
    virtual int			posAttrib(int idx) const;
    virtual void		addPosAttrib(int attr);
    virtual void		removePosAttribList(int attr,
						    bool addtohistory=true);
    virtual void		setPosAttrib(const EM::PosID&,
				    int attr,bool yn,bool addtohistory=true);
    				//!<Sets/unsets the posattrib depending on yn.
    virtual bool		isPosAttrib(const EM::PosID&,int attr) const;
    virtual const char*		posAttribName(int) const;
    virtual int			addPosAttribName(const char*);
    const TypeSet<PosID>*	getPosAttribList(int attr) const;
    const MarkerStyle3D&	getPosAttrMarkerStyle(int attr);
    void			setPosAttrMarkerStyle(int attr, 
						      const MarkerStyle3D&);
    virtual void		lockPosAttrib(int attr,bool yn);
    virtual bool		isPosAttribLocked(int attr) const;

    CNotifier<EMObject,const EMObjectCallbackData&>	change;

    virtual Executor*		loader()		{ return 0; }
    virtual bool		isLoaded() const	{ return false; }
    virtual Executor*		saver()			{ return 0; }
    virtual bool		isChanged() const	{ return changed_; }
    virtual bool		isEmpty() const;
    virtual void		resetChangedFlag()	{ changed_=false; }
    bool			isFullyLoaded() const	{ return fullyloaded_; }
    void			setFullyLoaded(bool yn) { fullyloaded_=yn; }

    virtual bool		isLocked() const	{ return locked_; }
    virtual void		lock(bool yn)		{ locked_=yn;}

    const char*			errMsg() const;

    virtual bool		usePar( const IOPar& );
    virtual void		fillPar( IOPar& ) const;

    static int			sPermanentControlNode;
    static int			sTemporaryControlNode;
    static int			sEdgeControlNode;
    static int			sTerminationNode;
    static int			sSeedNode;

    virtual const IOObjContext&	getIOObjContext() const = 0;

protected:
    				EMObject( EMManager& );
    				//!<must be called after creation
    virtual Geometry::Element*	sectionGeometryInternal(const SectionID&);
    virtual void		prepareForDelete() const;

    void			posIDChangeCB(CallBacker*);
    ObjectID			id_;
    MultiID			storageid_;
    class EMManager&		manager_;
    BufferString		errmsg_;


    Color&			preferredcolor_;

    ObjectSet<PosAttrib>	posattribs_;
    TypeSet<int>		attribs_;

    bool			changed_;
    bool			fullyloaded_;
    bool			locked_;
    int				burstalertcount_;

    static const char*		prefcolorstr;
    static const char*		nrposattrstr;
    static const char*		posattrprefixstr;
    static const char*		posattrsectionstr;
    static const char*		posattrposidstr;
    
    static const char*		markerstylestr;
};


}; // Namespace

#define mDefineEMObjFuncs( clss ) \
public: \
    static void			initClass(); \
    static EMObject*		create( EM::EMManager& emm ); \
    static const char*		typeStr(); \
    const char*			getTypeStr() const; \
protected: \
				clss( EM::EMManager& ); \
				~clss()

#define mImplementEMObjFuncs( clss, typenm ) \
void clss::initClass() \
{ \
    EMOF().addCreator( create, typeStr() ); \
} \
 \
 \
EMObject* clss::create( EM::EMManager& emm ) \
{ \
    EMObject* obj = new clss( emm ); \
    emm.addObject( obj ); \
    return obj; \
} \
 \
 \
const char* clss::typeStr() { return typenm; } \
const char* clss::getTypeStr() const { return typeStr(); }

/*!\mainpage Earth Model objects

  Objects like horizons, well tracks and bodies are all earth model objects.
  Such objects can be described in various ways, and the EM way is the
  way we have chosen for OpendTect.

  A big part of this module deals with surfaces of some kind. Horizons, faults,
  and sets of (fault-)sticks each have their own peculiarities.

  Most interesting classes:

  - EM::EMObject, base class for the EarthModel objects
  - EM::EMManager, responsible for the loading and deleting of the objects

  Earth models have the nasty habit of changing over time. Therefore, edit
  history matters are handled by the Undo objects.

*/

#endif
