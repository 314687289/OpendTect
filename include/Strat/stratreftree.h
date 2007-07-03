#ifndef stratreftree_h
#define stratreftree_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: stratreftree.h,v 1.1 2007-07-03 10:38:02 cvsbert Exp $
________________________________________________________________________

-*/

#include "stratunitref.h"
#include "repos.h"

namespace Strat
{

class Lithology;
class Level;



/*!\brief Tree of UnitRef's  */

class RefTree : public NodeUnitRef
{
public:

    			RefTree( const char* nm, Repos::Source src )
			: NodeUnitRef(0,"","Top Node")
			, treename_(nm)
			, src_(src)			{}
			~RefTree();

    const BufferString&	treeName() const		{ return treename_; }
    void		setTreeName( const char* nm )	{ treename_ = nm; }
    Repos::Source	source() const			{ return src_; }

    bool		addUnit(const char* fullcode,const char* unit_dump);
    void		removeEmptyNodes(); //!< recommended after add

    void		addLevel( Level* l )		{ lvls_ += l; }
    int			nrLevels() const		{ return lvls_.size(); }
    const Level*	level( int idx ) const		{ return lvls_[idx]; }
    const Level*	getLevel(const char*) const;
    const Level*	getLevel(const UnitRef*,bool top=true) const;
    void		remove(const Level*&);
    			//!< the pointer will be set to null if found

    bool		write(std::ostream&) const;
    				//!< for printing, export or something.
    				//!< otherwise, use UnitRepository::write()

protected:

    Repos::Source	src_;
    BufferString	treename_;
    ObjectSet<Level>	lvls_;

};


}; // namespace Strat


#endif
