#ifndef emposid_h
#define emposid_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emposid.h,v 1.17 2004-12-20 12:31:20 nanne Exp $
________________________________________________________________________


-*/

#include "multiid.h"
#include "geomposidholder.h"
#include "rowcol.h"


namespace EM
{

typedef int32 ObjectID;
typedef int16 SectionID;
typedef int64 SubID;


/*!\brief
is an identifier for each position in the earthmodel. It has three parts,
- an ObjectID, wich identifies wich object is belongs to.
- a SectionID, wich identifies which section of the object it belongs to.
- a SubID, wich identifies the position on the section. 
*/

class PosID
{
public:
    				PosID( ObjectID emobj,
				       SectionID section=0,
				       SubID subid=0);

    const ObjectID&		objectID() const;
    SectionID			sectionID() const;
    SubID			subID() const;
    void			setObjectID(const ObjectID&);
    void			setSectionID(SectionID);
    void			setSubID(SubID);

    RowCol			getRowCol() const;
    				/*!< Should not be used, only for db
				     purposes (it makes it possible to db
				     SubID
				 */

    bool			operator==(const PosID& b) const;
    bool			operator!=(const PosID& b) const;

    void			fillPar( IOPar& ) const;
    bool			usePar( const IOPar& );

protected:

    ObjectID			emobj;
    SectionID			section;
    SubID			subid;

    static const char*		emobjStr();
    static const char* 		sectionStr();
    static const char*		subidStr();
};


inline PosID::PosID( ObjectID emobj_,
		       SectionID section_,
		       SubID subid_ )
    : emobj( emobj_ )
    , section( section_ )
    , subid( subid_ )
{}


inline bool PosID::operator==(const PosID& b) const
{ return emobj==b.emobj && section==b.section && subid==b.subid; }


inline bool PosID::operator!=(const PosID& b) const
{ return !(*this==b); }

inline const ObjectID& PosID::objectID() const
{ return emobj; }


inline SectionID PosID::sectionID() const
{ return section; }



inline SubID PosID::subID() const
{ return subid; }

inline void PosID::setObjectID( const ObjectID& id )
{ emobj = id; }
inline void PosID::setSectionID( SectionID id )
{ section = id; }
inline void PosID::setSubID( SubID id )
{ subid = id; }


}; // Namespace


#endif
