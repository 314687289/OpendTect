#ifndef stratunitrepos_h
#define stratunitrepos_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: stratunitrepos.h,v 1.15 2007-08-13 15:16:39 cvshelene Exp $
________________________________________________________________________

-*/

#include "stratreftree.h"
#include "callback.h"

namespace Strat
{

class Lithology;
class UnitRepository;

const UnitRepository& UnRepo();
const RefTree& RT();


/*!\brief Repository of all stratigraphic descriptions defining the building
	  blocks of subsurface descriptions */

class UnitRepository : public CallBacker
{
public:

    int			nrTrees() const		{ return trees_.size(); }
    const RefTree*	tree(int idx=0) const	{ return trees_[idx]; }
    RefTree*		tree(int idx=0)		{ return trees_[idx]; }
    int			indexOf(const char* treename) const;
    inline void		setCurrentTree( int idx ) { curtreeidx_ = idx; }
    inline int		currentTree() const	{ return curtreeidx_; }

    UnitRef*		find( const char* code )	{ return fnd(code); }
    const UnitRef*	find( const char* code ) const	{ return fnd(code); }
    UnitRef*		find( const char* c, int idx )	{ return fnd(c,idx);}
    const UnitRef*	find( const char* c, int i ) const { return fnd(c,i);}
    UnitRef*		findAny( const char* code )	{ return fndAny(code); }
    const UnitRef*	findAny( const char* c ) const	{ return fndAny(c); }
    int			treeOf(const char* code) const;

    int			nrLiths() const			{ return liths_.size();}
    const Lithology&	lith( int idx ) const		{ return *liths_[idx]; }
    Lithology&		lith( int idx )			{ return *liths_[idx]; }
    int			findLith(const char*) const;
    int			findLith(int) const;
    BufferString	getLithName(int lithid) const;
    static const char*	sKeyLith;

    void		copyCurTreeAtLoc(Repos::Source);
    const RefTree*	getTreeFromSource(Repos::Source) const;
    bool		write(Repos::Source) const;
    void		reRead();

protected:

    			UnitRepository();
    virtual		~UnitRepository();

    ObjectSet<RefTree>	trees_;
    ObjectSet<Lithology> liths_;
    int			curtreeidx_;

    UnitRef*		fnd(const char*) const;
    UnitRef*		fnd(const char*,int) const;
    UnitRef*		fndAny(const char*) const;
    void		addLith(const char*,Repos::Source);

    void		survChg(CallBacker*);

private:

    friend const UnitRepository& UnRepo();
    friend const RefTree& RT();

    ObjectSet<Lithology> unusedliths_;
    void		addTreeFromFile(const Repos::FileProvider&,
	    				Repos::Source);

    inline const RefTree&	curTree() const	{ return *tree(curtreeidx_); }

};

inline const RefTree& RT()
{
    return UnRepo().curTree();
}


inline UnitRepository& eUnRepo() //!< editable UnRepo
{
    return const_cast<UnitRepository&>( UnRepo() );
}


}; // namespace Strat


/*!\mainpage Stratigraphy

This module supports hierarchical naming of units, with supporting tools like
lithology selection.

Although a stratigraphic framework can be used independently of any geometry,
an easy way to think about it is how it could be used for wells. Every layer
in a well can be interpreted as being a specific instantiation of an abstract
unit defined in the framework. For example, a sand layer in the upper
cretaceous could be labeled cret.upp.sand . Two layers further a similar sand
layer can have the same label. Both are then instantiations of this
cret.upp.sand 'template unit'.

One of the main problems in this area is that there are many possible ways of
classifying subsurface units. Not only litho- vs chronostratigraphy, but
also specific local problem-driven classifications can be thought of. This
is the topic of the UnitRepository, which manages several trees of units.

Such a classification system invariably has a tree structure. The nodes in the
tree are stratigraphic units, the leaves are lithologies. Every unit can have
a number of properties (porosity, sand/shale ratio, etc.).

A well interpretation defines a linear sequence of instantiated units, i.e.
instantiations of the stratigraphic framework are often simply lists of units.
Keep in mind that this is not necessary: any network of connections (not just
the array-like ordering) between instantiations may be interesting in certain
problems.

To implement these thoughts the separation 'reference data' and 'actual
subsurface unit' has been made. A layer is a unit that holds a pointer to 
a conceptual unit (e.g. there are a couple of sand layers connected to
the cret.upp.sand reference data).

*/

#endif
