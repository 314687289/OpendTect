#ifndef stratunitref_h
#define stratunitref_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: stratunitref.h,v 1.13 2007-09-12 09:16:17 cvshelene Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "compoundkey.h"
class Property;
class PropertyRef;

namespace Strat
{

class NodeUnitRef;

/*!\brief Reference data for a stratigraphic unit

  Every stratigraphy is a tree of units. A stratigraphy consists of reference
  units - every part of the subsurface can be attached to a reference unit.

  If properties for this reference unit have a fixed value or calculation,
  a property can be added to it. Any concrete unit should use this property
  rather than define a definition itself. For example, salt units can be
  defined to have porosity zero here.

 */

class UnitRef
{
public:

			UnitRef( NodeUnitRef* up, const char* unitcode,
			      const char* descr=0 )
			: code_(unitcode)
			, upnode_(up)
			, desc_(descr)			{}
    virtual		~UnitRef();

    virtual bool	isLeaf() const			= 0;
    CompoundKey		fullCode() const;

    const BufferString&	code() const			{ return code_; }
    void		setCode( const char* c )	{ code_ = c; }
    const BufferString&	description() const		{ return desc_; }
    void		setDescription( const char* d )	{ desc_ = d; }

    NodeUnitRef*	upNode(int skip=0);
    const NodeUnitRef*	upNode( int skip=0 ) const
    			{ return ((UnitRef*)this)->upNode( skip ); }
    NodeUnitRef*	topNode();
    const NodeUnitRef*	topNode() const;

    int			treeDepth() const;
    bool		isBelow(const UnitRef*) const;
    			//!< is given ref parent, grandparent, grandgrand... 
    bool		precedes(const UnitRef&) const;
    			//!< in terms of iterating through tree

    virtual void	fill(BufferString&) const; //!< Without Unit code
    virtual bool	use(const char*); //!< a string produced by fill()

    void		add( Property* p )
    			{ properties_ += p; }
    Property*		property( const PropertyRef* p )
    			{ return gtProp(p); }
    const Property*	property( const PropertyRef* p ) const
    			{ return gtProp(p); }
    Property*		property( int propidx )
    			{ return properties_[propidx]; }
    int			nrProperties() const
			{ return properties_.size(); }

    //! Iterator. When constructed, returns unit itself (regardless of Pol).
    //!< First next() goes to first (valid) unit.
    class Iter
    {
    public:

	enum Pol	{ All, Nodes, Leaves };

			Iter(const NodeUnitRef&,Pol p=All);

	void		reset();
	bool		next();
	UnitRef*	unit()		{ return gtUnit(); }
	const UnitRef*	unit() const	{ return gtUnit(); }

    protected:

	Pol		pol_;
	NodeUnitRef*	itnode_;
	NodeUnitRef*	curnode_;
	int		curidx_;
	UnitRef*	gtUnit() const;
	bool		toNext();

    };

protected:

    NodeUnitRef*	upnode_;
    BufferString	code_;
    BufferString	desc_;

    ObjectSet<Property>	properties_;
    Property*		gtProp(const PropertyRef* p) const;

};


/*!\brief UnitRef for units containing other units only */

class NodeUnitRef : public UnitRef
{
public:

			NodeUnitRef( NodeUnitRef* up, const char* c,
				     const char* d=0 )
			: UnitRef(up,c,d)		{}
			~NodeUnitRef();

    virtual bool	isLeaf() const			{ return false; }
    static const NodeUnitRef& undef();

    int			nrRefs() const			{ return refs_.size(); }
    UnitRef&		ref( int idx )			{ return *refs_[idx]; }
    const UnitRef&	ref( int idx ) const		{ return *refs_[idx]; }
    int			indexOf( const UnitRef* ur ) const
    						{ return refs_.indexOf(ur); }

    UnitRef*		find( const char* urcode )	{ return fnd(urcode); }
    const UnitRef*	find( const char* urcode ) const{ return fnd(urcode); }

    void		add(UnitRef*,bool rev =false);
    void		remove( int uridx ) 
    			{ UnitRef* r = refs_[uridx]; refs_ -= r; delete r; }
    UnitRef*		replace( int uridx, UnitRef* newur )
			{ return refs_.replace( uridx, newur); }

protected:

    ObjectSet<UnitRef>	refs_;

    UnitRef*		fnd(const char*) const;

};


/*!\brief UnitRef for layers */

class LeafUnitRef : public UnitRef
{
public:

			LeafUnitRef( NodeUnitRef* up, const char* c,
				     int lithidx=-1, const char* d=0 )
			: UnitRef(up,c,d)
			, lith_(lithidx)	{}

    virtual bool	isLeaf() const		{ return true; }
    int			lithology() const	{ return lith_; }

    static const LeafUnitRef& undef();

    virtual void	fill(BufferString&) const;
    virtual bool	use(const char*);

protected:

    int			lith_;

};


inline int UnitRef::treeDepth() const
{ return upnode_ ? upnode_->treeDepth() + 1 : 0; }
inline NodeUnitRef* UnitRef::topNode()
{ return upnode_ ? upnode_->topNode() : (NodeUnitRef*)this; }
inline const NodeUnitRef* UnitRef::topNode() const
{ return upnode_ ? upnode_->topNode() : (NodeUnitRef*)this; }


}; // namespace Strat

#endif
