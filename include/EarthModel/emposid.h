#ifndef emposid_h
#define emposid_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emposid.h,v 1.12 2003-11-24 08:39:49 kristofer Exp $
________________________________________________________________________


-*/

#include "multiid.h"
#include "geomposidholder.h"

#include "rowcol.h"


namespace EM
{

typedef int ObjectID;
typedef short PatchID;
typedef long long SubID;


/*!\brief
is an identifier for each position in the earthmodel. It has three parts,
- an ObjectID, wich identifies wich object is belongs to.
- a PatchID, wich identifies which patch of the object it belongs to.
- a SubID, wich identifies the position on the patch. 
*/

class PosID
{
public:
    				PosID( ObjectID emobj,
				       PatchID patch=0,
				       SubID subid=0);

    const ObjectID&		objectID() const;
    PatchID			patchID() const;
    SubID			subID() const;
    void			setObjectID(const ObjectID&);
    void			setPatchID(PatchID);
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
    PatchID			patch;
    SubID			subid;

    static const char*		emobjstr;
    static const char* 		patchstr;
    static const char*		subidstr;
};


inline PosID::PosID( ObjectID emobj_,
		       PatchID patch_,
		       SubID subid_ )
    : emobj( emobj_ )
    , patch( patch_ )
    , subid( subid_ )
{}


inline bool PosID::operator==(const PosID& b) const
{ return emobj==b.emobj && patch==b.patch && subid==b.subid; }


inline bool PosID::operator!=(const PosID& b) const
{ return !(*this==b); }

inline const ObjectID& PosID::objectID() const
{ return emobj; }


inline PatchID PosID::patchID() const
{ return patch; }


inline SubID PosID::subID() const
{ return subid; }

inline void PosID::setObjectID( const ObjectID& id )
{ emobj = id; }
inline void PosID::setPatchID( PatchID id )
{ patch = id; }
inline void PosID::setSubID( SubID id )
{ subid = id; }


}; // Namespace


#endif
